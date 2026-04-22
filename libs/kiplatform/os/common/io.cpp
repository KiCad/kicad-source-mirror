/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright The KiCad Developers, see AUTHORS.txt for contributors.
*
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <kiplatform/io.h>

#include <wx/crt.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/string.h>

#include <atomic>
#include <cstdio>
#include <stdexcept>
#include <string>

#if defined( _WIN32 )
#include <process.h>
#else
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif


wxString KIPLATFORM::IO::MakeSiblingTempPath( const wxString& aTargetPath )
{
    // Keeping the temp file on the same filesystem as the final target is required:
    // rename() across filesystems is not atomic, and MoveFileEx on Windows will fail or
    // fall back to copy+delete across volumes.
    static std::atomic<unsigned> s_counter{ 0 };

    unsigned counter = s_counter.fetch_add( 1, std::memory_order_relaxed );

#if defined( _WIN32 )
    unsigned pid = static_cast<unsigned>( _getpid() );
#else
    unsigned pid = static_cast<unsigned>( getpid() );
#endif

    return aTargetPath + wxString::Format( wxT( ".kicad-save-%u-%u" ), pid, counter );
}


#if !defined( _WIN32 )

FILE* KIPLATFORM::IO::OpenUniqueSiblingTempFile( const wxString& aTargetPath, const wxString& aMode,
                                                 wxString* aTempPathOut, wxString* aError )
{
    // Exclusive-create closes the TOCTOU window: if another process pre-created a file
    // at the candidate path, O_EXCL fails and we retry with a new counter value.
    for( unsigned attempt = 0; attempt < 32; ++attempt )
    {
        wxString candidate = MakeSiblingTempPath( aTargetPath );
        int      fd = open( candidate.fn_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600 );

        if( fd >= 0 )
        {
            FILE* fp = fdopen( fd, aMode.mb_str() );

            if( !fp )
            {
                int err = errno;
                close( fd );
                unlink( candidate.fn_str() );

                if( aError )
                {
                    *aError = wxString::Format( wxT( "fdopen failed for temp file '%s': %s" ),
                                                candidate, wxString::FromUTF8( strerror( err ) ) );
                }

                return nullptr;
            }

            if( aTempPathOut )
                *aTempPathOut = candidate;

            return fp;
        }

        if( errno != EEXIST )
        {
            if( aError )
            {
                *aError = wxString::Format( wxT( "Cannot create temp file '%s': %s" ), candidate,
                                            wxString::FromUTF8( strerror( errno ) ) );
            }

            return nullptr;
        }
    }

    if( aError )
        *aError = wxT( "Exhausted temp-file retry budget" );

    return nullptr;
}


wxString KIPLATFORM::IO::ResolveSymlinkTarget( const wxString& aPath )
{
    // Users commonly symlink shared config/project files into their working directory.
    // Atomic rename would replace the symlink with a regular file; resolve so the save
    // lands on the referent instead.
    struct stat st;

    if( lstat( aPath.fn_str(), &st ) != 0 || !S_ISLNK( st.st_mode ) )
        return aPath;

    char resolved[PATH_MAX];

    if( realpath( aPath.fn_str(), resolved ) )
        return wxString::FromUTF8( resolved );

    return aPath;
}


bool KIPLATFORM::IO::FlushDirectory( const wxString& aDirPath )
{
    int fd = open( aDirPath.fn_str(), O_RDONLY
#if defined( O_DIRECTORY )
                                          | O_DIRECTORY
#endif
    );

    if( fd < 0 )
    {
        // NFS and some FUSE mounts reject O_DIRECTORY or reject fsync on directories;
        // treat those as non-fatal since the file's own fsync is what matters most.
        return errno == EINVAL || errno == ENOTSUP;
    }

    int rc = fsync( fd );
    int err = errno;
    close( fd );

    return rc == 0 || err == EINVAL;
}


bool KIPLATFORM::IO::AtomicRename( const wxString& aSrc, const wxString& aDst, wxString* aError )
{
    if( rename( aSrc.fn_str(), aDst.fn_str() ) == 0 )
        return true;

    if( aError )
        *aError = wxString::FromUTF8( strerror( errno ) );

    return false;
}

#endif // !_WIN32


bool KIPLATFORM::IO::CommitTempFile( const wxString& aTempPath, const wxString& aTargetPath,
                                     wxString* aError )
{
    TARGET_ATTRS snapshot;
    const bool   targetExists = wxFileName::FileExists( aTargetPath );

    if( targetExists )
    {
        // Snapshot first so we can re-apply after rename. DuplicatePermissions must
        // read target's original mode (POSIX) before any mutation, so it follows the
        // snapshot and precedes MakeWriteable.
        snapshot = CaptureTargetAttributes( aTargetPath );

        if( !DuplicatePermissions( aTargetPath, aTempPath ) )
        {
            // Failing here means the new file would land with creation-default
            // permissions instead of the target's. Bail out while the rename hasn't
            // happened yet so the user's original file is still untouched.
            if( aError )
            {
                *aError = wxString::Format( wxT( "Cannot copy permissions from '%s' to '%s'" ),
                                            aTargetPath, aTempPath );
            }

            return false;
        }

#if defined( _WIN32 )
        // Cloud-sync mounts (OneDrive/Drive/Dropbox) can reject MoveFileEx when the
        // target has FILE_ATTRIBUTE_HIDDEN. Clear blocking bits here; the snapshot
        // restores them below. POSIX rename() requires write on the containing
        // directory only, not on the target file, so MakeWriteable is unnecessary.
        MakeWriteable( aTargetPath );
#endif
    }

    wxString   renameError;
    const bool renamed = AtomicRename( aTempPath, aTargetPath, &renameError );

    if( targetExists )
    {
        // Unconditional re-apply. On rename failure this rolls back the MakeWriteable
        // mutation on the original target. On rename success it restores HIDDEN/
        // READONLY bits to the new file, since SetFileSecurity (used by
        // DuplicatePermissions) copies ACLs but not those attribute bits. A failure
        // here is logged but not fatal: the rename has already committed (or was
        // going to be reported as failed below), so the user's data is consistent;
        // only the attribute bits are off.
        if( !ApplyTargetAttributes( aTargetPath, snapshot ) )
        {
            wxLogWarning( wxT( "Could not restore file attributes on '%s' after save" ),
                          aTargetPath );
        }
    }

    if( !renamed )
    {
        if( aError )
        {
            *aError = wxString::Format( wxT( "Cannot rename temp file over '%s': %s" ),
                                        aTargetPath, renameError );
        }

        return false;
    }

    wxFileName dst( aTargetPath );
    wxString   dirPath = dst.GetPath();

    // A bare filename has no directory component; fall back to CWD so the dir fsync
    // lands on the filesystem that actually holds the file.
    if( dirPath.IsEmpty() )
        dirPath = wxT( "." );

    if( !FlushDirectory( dirPath ) )
    {
        // The rename has already committed, but without a dir fsync it may not survive
        // power loss. Report so callers can warn the user; the file itself is present.
        if( aError )
            *aError = wxString::Format( wxT( "Cannot flush directory '%s' to disk" ), dirPath );

        return false;
    }

    return true;
}


bool KIPLATFORM::IO::AtomicWriteFile( const wxString& aTargetPath, const void* aData, size_t aSize,
                                      wxString* aError )
{
    wxString target = ResolveSymlinkTarget( aTargetPath );
    wxString tempPath;
    FILE*    fp = OpenUniqueSiblingTempFile( target, wxT( "wb" ), &tempPath, aError );

    if( !fp )
        return false;

    if( aSize > 0 && std::fwrite( aData, 1, aSize, fp ) != aSize )
    {
        if( aError )
            *aError = wxString::Format( wxT( "Write failed to '%s'" ), tempPath );

        std::fclose( fp );
        wxRemoveFile( tempPath );
        return false;
    }

    if( !FlushToDisk( fp ) )
    {
        if( aError )
            *aError = wxString::Format( wxT( "fsync failed on '%s'" ), tempPath );

        std::fclose( fp );
        wxRemoveFile( tempPath );
        return false;
    }

    std::fclose( fp );

    if( !CommitTempFile( tempPath, target, aError ) )
    {
        // CommitTempFile can fail after a successful rename (e.g. dir fsync error),
        // in which case tempPath no longer exists. Suppress the expected log noise.
        wxLogNull logNoise;
        wxRemoveFile( tempPath );
        return false;
    }

    return true;
}





void KIPLATFORM::IO::MAPPED_FILE::readIntoBuffer( const wxString& aFileName )
{
    FILE* fp = wxFopen( aFileName, wxS( "rb" ) );

    if( !fp )
        throw std::runtime_error( std::string( "Cannot open file: " ) + aFileName.ToStdString() );

    fseek( fp, 0, SEEK_END );
    long len = ftell( fp );

    if( len < 0 )
    {
        fclose( fp );
        throw std::runtime_error( std::string( "Cannot determine file size: " )
                                  + aFileName.ToStdString() );
    }

    m_fallbackBuffer.resize( static_cast<size_t>( len ) );
    fseek( fp, 0, SEEK_SET );

    size_t bytesRead = fread( m_fallbackBuffer.data(), 1, static_cast<size_t>( len ), fp );
    fclose( fp );

    if( bytesRead != static_cast<size_t>( len ) )
    {
        throw std::runtime_error( std::string( "Failed to read file: " )
                                  + aFileName.ToStdString() );
    }

    m_data = m_fallbackBuffer.data();
    m_size = m_fallbackBuffer.size();
}

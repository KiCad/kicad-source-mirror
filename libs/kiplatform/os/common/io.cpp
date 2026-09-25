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
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <kiplatform/io.h>

#include <wx/crt.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/string.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <string>

#if defined( _WIN32 )
#include <process.h>
#else
#include <climits>
#include <cstdlib>
#include <fcntl.h>
#include <sys/file.h>
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
    // A new target keeps the umask default fopen would give it; an existing one gets its mode
    // copied at commit, so the temp stays private until then
    const mode_t createMode = wxFileName::FileExists( aTargetPath ) ? 0600 : 0666;

    // Exclusive-create closes the TOCTOU window: if another process pre-created a file
    // at the candidate path, O_EXCL fails and we retry with a new counter value.
    for( unsigned attempt = 0; attempt < 32; ++attempt )
    {
        wxString candidate = MakeSiblingTempPath( aTargetPath );
        int      fd = open( candidate.fn_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, createMode );

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


bool KIPLATFORM::IO::CopyToNewFile( const wxString& aSrc, const wxString& aDst, bool* aExists,
                                    wxString* aError )
{
    if( aExists )
        *aExists = false;

    auto fail = [&]( int aErrno )
    {
        if( aExists && aErrno == EEXIST )
            *aExists = true;

        if( aError )
            *aError = wxString::FromUTF8( strerror( aErrno ) );

        return false;
    };

    const int in = open( aSrc.fn_str(), O_RDONLY | O_CLOEXEC );

    if( in < 0 )
        return fail( errno );

    struct stat source;

    if( fstat( in, &source ) != 0 )
    {
        const int err = errno;
        close( in );
        return fail( err );
    }

    const int out = open( aDst.fn_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, source.st_mode & 0777 );

    if( out < 0 )
    {
        const int err = errno;
        close( in );
        return fail( err );
    }

    struct stat created;
    const bool  identified = fstat( out, &created ) == 0;

    // Another process may have replaced our file by now, and that one must survive
    auto removeCreated = [&]()
    {
        struct stat current;

        if( identified && lstat( aDst.fn_str(), &current ) == 0 && current.st_dev == created.st_dev
            && current.st_ino == created.st_ino )
        {
            unlink( aDst.fn_str() );
        }
    };

    auto abandon = [&]( int aErrno )
    {
        close( in );
        close( out );
        removeCreated();
        return fail( aErrno );
    };

    if( !identified )
        return abandon( errno );

    char buffer[65536];

    for( ;; )
    {
        const ssize_t got = read( in, buffer, sizeof( buffer ) );

        if( got < 0 && errno == EINTR )
            continue;

        if( got < 0 )
            return abandon( errno );

        if( got == 0 )
            break;

        for( ssize_t done = 0; done < got; )
        {
            const ssize_t put = write( out, buffer + done, static_cast<size_t>( got - done ) );

            if( put < 0 && errno == EINTR )
                continue;

            if( put < 0 )
                return abandon( errno );

            done += put;
        }
    }

    // Some FUSE and network mounts cannot sync, and the data is still written
    if( fsync( out ) != 0 && errno != EINVAL && errno != ENOSYS && errno != ENOTSUP )
        return abandon( errno );

    close( in );

    if( close( out ) != 0 )
    {
        const int err = errno;
        removeCreated();
        return fail( err );
    }

    return true;
}


bool KIPLATFORM::IO::RenameNoReplace( const wxString& aSrc, const wxString& aDst, bool* aExists,
                                      wxString* aError )
{
    if( aExists )
        *aExists = false;

    auto fail = [&]( int aErrno )
    {
        if( aExists && aErrno == EEXIST )
            *aExists = true;

        if( aError )
            *aError = wxString::FromUTF8( strerror( aErrno ) );

        return false;
    };

#if defined( __APPLE__ )
    if( renamex_np( aSrc.fn_str(), aDst.fn_str(), RENAME_EXCL ) == 0 )
        return true;

    // Volumes such as FAT and SMB shares may not take the flag
    if( errno != ENOTSUP && errno != EINVAL )
        return fail( errno );
#elif defined( RENAME_NOREPLACE )
    if( renameat2( AT_FDCWD, aSrc.fn_str(), AT_FDCWD, aDst.fn_str(), RENAME_NOREPLACE ) == 0 )
        return true;

    // Older kernels lack the call, and filesystems such as NFS reject the flag
    if( errno != ENOSYS && errno != EINVAL )
        return fail( errno );
#endif

    // A hard link is made only where no file exists, so this never replaces either
    if( link( aSrc.fn_str(), aDst.fn_str() ) != 0 )
    {
        const int linkErrno = errno;

        switch( linkErrno )
        {
        case ENOTSUP:
#if EOPNOTSUPP != ENOTSUP
        case EOPNOTSUPP:
#endif
        case ENOSYS:
        case EINVAL:
        case EPERM:
        case EXDEV: break;
        default: return fail( linkErrno );
        }

        // FAT and SSHFS have no hard links, and an exclusive create still claims the name before the data lands
        if( !CopyToNewFile( aSrc, aDst, aExists, aError ) )
            return false;

        unlink( aSrc.fn_str() );
        return true;
    }

    // The copy is already in place, so a source left behind is only an extra name for it
    unlink( aSrc.fn_str() );
    return true;
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

        // Best effort, volumes without ACL or mode support (cloud drives, FAT) reject the copy
        // and the temp then keeps the permissions it was created with
        if( !DuplicatePermissions( aTargetPath, aTempPath ) )
        {
            wxLogTrace( wxT( "KICAD_ATOMIC_SAVE" ), wxT( "Cannot copy permissions from '%s' to '%s'" ),
                        aTargetPath, aTempPath );
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
    return AtomicWriteFile( aTargetPath, aData, aSize, aError, {} );
}


bool KIPLATFORM::IO::AtomicWriteFile( const wxString& aTargetPath, const void* aData, size_t aSize, wxString* aError,
                                      const std::function<bool()>& aCommit )
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

    // Buffered writes on NFS and quota'd volumes can surface their errors at close, not
    // at write time, so an unchecked close could rename a short file into place.
    if( std::fclose( fp ) != 0 )
    {
        int err = errno;

        if( aError )
        {
            *aError = wxString::Format( wxT( "Cannot close temp file '%s': %s" ), tempPath,
                                        wxString::FromUTF8( strerror( err ) ) );
        }

        wxRemoveFile( tempPath );
        return false;
    }

    if( aCommit && !aCommit() )
    {
        wxRemoveFile( tempPath );
        return false;
    }

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


#if !defined( _WIN32 )


KIPLATFORM::IO::FILE_LOCK::STATE KIPLATFORM::IO::FILE_LOCK::Acquire( const wxString& aPath,
                                                                     bool& aCreated )
{
    Release();

    int fd = open( aPath.fn_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0666 );

    aCreated = fd >= 0;

    if( !aCreated )
        fd = open( aPath.fn_str(), O_RDWR | O_CLOEXEC );

    if( fd < 0 )
    {
        // Fall back to read-only so we can still report the lock owner
        m_fd = open( aPath.fn_str(), O_RDONLY | O_CLOEXEC );

        if( m_fd >= 0 )
            m_state = STATE::UNSUPPORTED;

        return m_state;
    }

    m_fd = fd;

    if( flock( fd, LOCK_EX | LOCK_NB ) == 0 )
        m_state = STATE::HELD;
    else if( errno == EWOULDBLOCK )
        m_state = STATE::BUSY;
    else
        m_state = STATE::UNSUPPORTED;

    return m_state;
}


bool KIPLATFORM::IO::FILE_LOCK::OpenForInspect( const wxString& aPath, bool& aHeldByAnother )
{
    Release();

    aHeldByAnother = false;

    m_fd = open( aPath.fn_str(), O_RDONLY | O_CLOEXEC );

    if( m_fd < 0 )
        return false;

    // Briefly take the lock to test for a holder, then release; m_state stays NONE
    if( flock( m_fd, LOCK_EX | LOCK_NB ) == 0 )
        flock( m_fd, LOCK_UN );
    else if( errno == EWOULDBLOCK )
        aHeldByAnother = true;

    return true;
}


bool KIPLATFORM::IO::FILE_LOCK::IsOpen() const
{
    return m_fd >= 0;
}


bool KIPLATFORM::IO::FILE_LOCK::ReadAll( std::string& aContents ) const
{
    if( !IsOpen() || lseek( m_fd, 0, SEEK_SET ) < 0 )
        return false;

    aContents.clear();

    char    buffer[4096];
    ssize_t bytes;

    while( ( bytes = read( m_fd, buffer, sizeof( buffer ) ) ) > 0 )
        aContents.append( buffer, static_cast<size_t>( bytes ) );

    return bytes >= 0;
}


bool KIPLATFORM::IO::FILE_LOCK::Rewrite( const std::string& aContents )
{
    if( !IsOpen() || ftruncate( m_fd, 0 ) < 0 || lseek( m_fd, 0, SEEK_SET ) < 0 )
        return false;

    size_t written = 0;

    while( written < aContents.size() )
    {
        ssize_t bytes = write( m_fd, aContents.data() + written, aContents.size() - written );

        if( bytes <= 0 )
            return false;

        written += static_cast<size_t>( bytes );
    }

    return true;
}


void KIPLATFORM::IO::FILE_LOCK::Release()
{
    if( IsOpen() )
    {
        // Closing the descriptor releases the lock, same as process death would
        close( m_fd );
        m_fd = -1;
    }

    m_state = STATE::NONE;
}

#endif // !_WIN32



KIPLATFORM::IO::FILE_LOCK::~FILE_LOCK()
{
    Release();
}


KIPLATFORM::IO::FILE_LOCK::FILE_LOCK( FILE_LOCK&& aOther ) noexcept
{
    *this = std::move( aOther );
}


KIPLATFORM::IO::FILE_LOCK& KIPLATFORM::IO::FILE_LOCK::operator=( FILE_LOCK&& aOther ) noexcept
{
    if( this == &aOther )
        return *this;

    Release();

#ifdef _WIN32
    m_handle = aOther.m_handle;
    aOther.m_handle = nullptr;
#else
    m_fd = aOther.m_fd;
    aOther.m_fd = -1;
#endif

    m_state = aOther.m_state;
    aOther.m_state = STATE::NONE;

    return *this;
}

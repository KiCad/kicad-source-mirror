/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <mdw_runtime/mdw_file_io.h>

#include "mdw_runtime_internal.h"

#include <algorithm>
#include <fstream>
#include <random>

#if defined( _WIN32 )
#include <windows.h>
#include <vector>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace
{

#if defined( _WIN32 )
/// Rename aTemp over aPath, keeping the DACL of a file being replaced
std::error_code replaceWithTemp( const fs::path& aTemp, const fs::path& aPath, bool aReplacing )
{
    DWORD size = 0;

    // Only the DACL, since setting the owner needs a privilege the user may not hold. Best effort, since a
    // volume without ACLs refuses the copy
    if( aReplacing && !GetFileSecurityW( aPath.c_str(), DACL_SECURITY_INFORMATION, nullptr, 0, &size )
        && GetLastError() == ERROR_INSUFFICIENT_BUFFER )
    {
        std::vector<BYTE> descriptor( size );

        if( GetFileSecurityW( aPath.c_str(), DACL_SECURITY_INFORMATION, descriptor.data(), size, &size ) )
            SetFileSecurityW( aTemp.c_str(), DACL_SECURITY_INFORMATION, descriptor.data() );
    }

    DWORD error = 0;

    // Antivirus, indexers and cloud sync hold files briefly, which surfaces as a sharing or access error
    for( int attempt = 0; attempt < 10; ++attempt )
    {
        if( MoveFileExW( aTemp.c_str(), aPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
            return std::error_code();

        error = GetLastError();

        if( error != ERROR_SHARING_VIOLATION && error != ERROR_ACCESS_DENIED && error != ERROR_LOCK_VIOLATION )
            break;

        Sleep( 50 );
    }

    // ReplaceFileW succeeds where the destination is open for reading with FILE_SHARE_DELETE
    if( aReplacing && ReplaceFileW( aPath.c_str(), aTemp.c_str(), nullptr, REPLACEFILE_WRITE_THROUGH, nullptr,
                                    nullptr ) )
    {
        return std::error_code();
    }

    return std::error_code( static_cast<int>( error ), std::system_category() );
}
#else
/// fsync, which on macOS leaves the data in the drive's cache unless F_FULLFSYNC is asked for
int syncToDisk( int aFd )
{
#if defined( __APPLE__ )
    // Some volumes, remote ones among them, refuse F_FULLFSYNC
    if( ::fcntl( aFd, F_FULLFSYNC ) == 0 )
        return 0;
#endif

    return ::fsync( aFd );
}
#endif


// Created exclusively and synced, so an existing name is never opened and a failed write leaves nothing
std::error_code writeNewFile( const fs::path& aPath, const std::string& aData, unsigned aPosixMode )
{
#if defined( _WIN32 )
    (void) aPosixMode;

    HANDLE file = CreateFileW( aPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr );

    if( file == INVALID_HANDLE_VALUE )
    {
        DWORD error = GetLastError();

        if( error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS )
            return std::make_error_code( std::errc::file_exists );

        return std::error_code( static_cast<int>( error ), std::system_category() );
    }

    std::size_t written = 0;
    bool        ok = true;

    while( ok && written < aData.size() )
    {
        DWORD chunk = static_cast<DWORD>( std::min<std::size_t>( aData.size() - written, 1 << 20 ) );
        DWORD count = 0;

        ok = WriteFile( file, aData.data() + written, chunk, &count, nullptr ) && count > 0;
        written += count;
    }

    ok = ok && FlushFileBuffers( file );

    std::error_code result = ok ? std::error_code()
                                : std::error_code( static_cast<int>( GetLastError() ), std::system_category() );

    if( !CloseHandle( file ) && !result )
        result = std::error_code( static_cast<int>( GetLastError() ), std::system_category() );
#else
    int fd = ::open( aPath.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, static_cast<mode_t>( aPosixMode ) );

    if( fd < 0 )
        return std::error_code( errno, std::generic_category() );

    std::size_t     written = 0;
    std::error_code result;

    while( !result && written < aData.size() )
    {
        ssize_t count = ::write( fd, aData.data() + written, aData.size() - written );

        if( count < 0 && errno == EINTR )
            continue;

        if( count <= 0 )
            result = std::error_code( count < 0 ? errno : EIO, std::generic_category() );
        else
            written += static_cast<std::size_t>( count );
    }

    if( !result && syncToDisk( fd ) != 0 )
        result = std::error_code( errno, std::generic_category() );

    if( ::close( fd ) != 0 && !result )
        result = std::error_code( errno, std::generic_category() );
#endif

    // The file is ours, since creation was exclusive, so a partial one never outlives the failure
    if( result )
    {
        std::error_code ignored;
        fs::remove( aPath, ignored );
    }

    return result;
}


/// Replace aPath with aData, or leave it when aCommit is given and answers false
std::error_code replaceFile( const fs::path& aPath, const std::string& aData, const std::function<bool()>& aCommit,
                             unsigned aPosixMode )
{
    thread_local std::mt19937_64 random( std::random_device{}() );

    std::error_code ec;
    fs::file_status existing = fs::status( aPath, ec );
    const bool      replacing = fs::exists( existing );
    fs::path        temp = aPath;

    temp += ".kicad-mdw-" + std::to_string( random() ) + ".tmp";

    // A file being replaced keeps its own mode, so a private copy never turns world-readable
    if( std::error_code written = writeNewFile( temp, aData, replacing ? 0600 : aPosixMode ) )
        return written;

    ec.clear();

    // Asked after the slow write, so a save made meanwhile is seen and only the rename itself is left to race
    if( aCommit && !aCommit() )
        ec = std::make_error_code( std::errc::operation_canceled );

#if defined( _WIN32 )
    if( !ec )
        ec = replaceWithTemp( temp, aPath, replacing );
#else
    if( !ec && replacing )
        fs::permissions( temp, existing.permissions(), ec );

    if( !ec )
        fs::rename( temp, aPath, ec );
#endif

    if( ec )
    {
        std::error_code ignored;
        fs::remove( temp, ignored );
    }

    return ec;
}

} // namespace


std::optional<std::string> MDW_ReadFile( const fs::path& aPath )
{
    std::ifstream in( aPath, std::ios::binary );

    if( !in )
        return std::nullopt;

    std::string bytes( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );

    if( in.bad() )
        return std::nullopt;

    return bytes;
}


std::error_code MDW_ReplaceFile( const fs::path& aPath, const std::string& aData, unsigned aPosixMode )
{
    return replaceFile( aPath, aData, nullptr, aPosixMode );
}


std::error_code MDW_ReplaceFileIf( const fs::path& aPath, const std::string& aData,
                                   const std::function<bool()>& aCommit, unsigned aPosixMode )
{
    return replaceFile( aPath, aData, aCommit, aPosixMode );
}


bool MDW_SyncDirectory( const fs::path& aDir )
{
#if defined( _WIN32 )
    (void) aDir;
    return true;
#else
    int fd = ::open( aDir.c_str(), O_RDONLY | O_CLOEXEC );

    // NFS and some FUSE mounts refuse to open or fsync a directory, and the file's own sync is what counts
    if( fd < 0 )
        return errno == EINVAL || errno == ENOTSUP;

    int rc = ::fsync( fd );
    int err = errno;
    ::close( fd );

    return rc == 0 || err == EINVAL;
#endif
}


struct MDW_FILE_LOCK::IMPL
{
#if defined( _WIN32 )
    HANDLE file = INVALID_HANDLE_VALUE;
#else
    int fd = -1;
#endif
};


MDW_FILE_LOCK::MDW_FILE_LOCK( const fs::path& aPath, bool aWait ) :
        m_impl( std::make_unique<IMPL>() )
{
#if defined( _WIN32 )
    m_impl->file = CreateFileW( aPath.c_str(), GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, nullptr );

    if( m_impl->file == INVALID_HANDLE_VALUE )
        return;

    OVERLAPPED whole{};
    DWORD      flags = LOCKFILE_EXCLUSIVE_LOCK | ( aWait ? 0 : LOCKFILE_FAIL_IMMEDIATELY );
    m_locked = LockFileEx( m_impl->file, flags, 0, 1, 0, &whole ) != FALSE;
    m_busy = !m_locked && GetLastError() == ERROR_LOCK_VIOLATION;
#else
    m_impl->fd = ::open( aPath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600 );

    if( m_impl->fd < 0 )
        return;

    // flock, unlike fcntl locks, also excludes other threads of this process
    int rc;

    do
    {
        rc = ::flock( m_impl->fd, LOCK_EX | ( aWait ? 0 : LOCK_NB ) );
    } while( rc != 0 && errno == EINTR );

    m_locked = rc == 0;
    m_busy = !m_locked && errno == EWOULDBLOCK;
#endif
}


MDW_FILE_LOCK::~MDW_FILE_LOCK()
{
#if defined( _WIN32 )
    if( m_impl->file != INVALID_HANDLE_VALUE )
    {
        if( m_locked )
        {
            OVERLAPPED whole{};
            UnlockFileEx( m_impl->file, 0, 1, 0, &whole );
        }

        CloseHandle( m_impl->file );
    }
#else
    if( m_impl->fd >= 0 )
        ::close( m_impl->fd );
#endif
}

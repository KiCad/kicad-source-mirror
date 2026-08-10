/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "kigit_lock_retry.h"

#include <git/kicad_git_compat.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <optional>
#include <system_error>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <io.h>
#include <sys/locking.h>
#include <sys/stat.h>
#else
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <fcntl.h>

namespace fs = std::filesystem;


namespace
{

// The base delay before retry aAttempt (0-based), 25, 50, 100, 200, 400, then 800 ms
std::chrono::milliseconds LockRetryDelay( int aAttempt )
{
    return std::chrono::milliseconds( 25 << std::min( std::max( aAttempt, 0 ), 5 ) );
}

/// git's lock on one file, <file>.lock, created only if no other writer holds it and renamed over the file to
/// publish new contents. A lock that is not committed is removed when this goes away
class FILE_LOCK
{
public:
    explicit FILE_LOCK( const fs::path& aTarget ) :
            m_target( aTarget ),
            m_lock( aTarget.native() + fs::path( ".lock" ).native() )
    {
#ifdef _WIN32
        m_fd = _wopen( m_lock.c_str(), _O_CREAT | _O_EXCL | _O_WRONLY | _O_BINARY, _S_IREAD | _S_IWRITE );
#else
        m_fd = open( m_lock.c_str(), O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC, 0666 );
#endif

        if( m_fd < 0 )
        {
            m_result = errno == EEXIST ? GIT_ELOCKED : GIT_ERROR;
            git_error_set_str( m_result == GIT_ELOCKED ? GIT_ERROR_OS : GIT_ERROR_FILESYSTEM,
                               "cannot create the lock file" );
        }
    }

    ~FILE_LOCK()
    {
        closeFile();

        if( m_result == 0 && !m_committed )
        {
            std::error_code ec;
            fs::remove( m_lock, ec );
        }
    }

    FILE_LOCK( const FILE_LOCK& ) = delete;
    FILE_LOCK& operator=( const FILE_LOCK& ) = delete;

    /// 0 when the lock is held, GIT_ELOCKED when another writer holds it, GIT_ERROR otherwise
    int Result() const { return m_result; }

    /// Replace the target with the file aContents, which must sit in the same folder
    int Commit( const fs::path& aContents )
    {
        if( m_result != 0 || m_committed )
        {
            git_error_set_str( GIT_ERROR_INVALID, "the lock is not held" );
            return GIT_ERROR;
        }

        closeFile();

        std::error_code ec;

        // Two renames, so the target only ever holds the old contents or the whole new ones
        fs::rename( aContents, m_lock, ec );

        if( !ec )
            fs::rename( m_lock, m_target, ec );

        // Set as libgit2 would, so a sharing violation on Windows is retried like libgit2's own
        if( ec )
        {
            git_error_set_str( GIT_ERROR_OS, ec.message().c_str() );
            return GIT_ERROR;
        }

        m_committed = true;
        return 0;
    }

private:
    void closeFile()
    {
        if( m_fd < 0 )
            return;

#ifdef _WIN32
        _close( m_fd );
#else
        close( m_fd );
#endif

        m_fd = -1;
    }

    fs::path m_target;
    fs::path m_lock;
    int      m_fd = -1;
    int      m_result = 0;
    bool     m_committed = false;
};


/// aPath's modification time in whole seconds, or nothing when it cannot be read
std::optional<std::int64_t> mtimeSeconds( const fs::path& aPath )
{
#ifdef _WIN32
    struct _stat64 st;

    if( _wstat64( aPath.c_str(), &st ) != 0 )
        return std::nullopt;
#else
    struct stat st;

    if( stat( aPath.c_str(), &st ) != 0 )
        return std::nullopt;
#endif

    return static_cast<std::int64_t>( st.st_mtime );
}


/// Zero the size and time of entries stamped no earlier than aStamp, so a status reads them rather than trusting a
/// stat an edit in that second could match. libgit2 does this only for an index the repository owns
int smudgeRacyEntries( git_index* aIndex, std::int64_t aStamp )
{
    std::vector<std::pair<std::string, git_index_entry>> racy;

    for( size_t i = 0; i < git_index_entrycount( aIndex ); ++i )
    {
        const git_index_entry* entry = git_index_get_byindex( aIndex, i );

        if( entry && GIT_INDEX_ENTRY_STAGE( entry ) == 0 && entry->mode != GIT_FILEMODE_COMMIT
            && entry->mtime.seconds >= static_cast<std::int32_t>( aStamp ) )
        {
            racy.emplace_back( entry->path, *entry );
        }
    }

    for( auto& [path, entry] : racy )
    {
        entry.path = path.c_str();
        entry.file_size = 0;

        // A size of zero alone matches a file emptied in that second
        entry.mtime.seconds = 0;
        entry.mtime.nanoseconds = 0;

        if( int rc = git_index_add( aIndex, &entry ) )
            return rc;
    }

    return 0;
}


/// Removes a scratch file however the edit ends
struct SCRATCH_FILE
{
    fs::path path;

    ~SCRATCH_FILE()
    {
        std::error_code ignored;
        fs::remove( path, ignored );
    }
};

} // namespace


namespace KIGIT
{

int EditIndexLocked( git_repository* aRepo, const std::function<int( git_index* aIndex )>& aEdit )
{
    git_index* repoIndex = nullptr;

    if( !aRepo )
        return GIT_EINVALIDSPEC;

    if( int rc = git_repository_index( &repoIndex, aRepo ) )
        return rc;

    std::unique_ptr<git_index, decltype( &git_index_free )> repoIndexPtr( repoIndex, &git_index_free );
    const char*                                             indexPathUtf8 = git_index_path( repoIndex );

    if( !indexPathUtf8 )
        return GIT_ENOTFOUND;

    const fs::path indexPath( reinterpret_cast<const char8_t*>( indexPathUtf8 ) );
    int            result = 0;

    {
        FILE_LOCK lock( indexPath );

        if( lock.Result() )
            return lock.Result();

        SCRATCH_FILE                      scratch{ indexPath.native() + fs::path( ".kicad-new" ).native() };
        std::error_code                   ec;
        const std::optional<std::int64_t> stamp = mtimeSeconds( indexPath );

        // No index file yet reads as an empty index
        if( fs::exists( indexPath, ec ) )
            fs::copy_file( indexPath, scratch.path, fs::copy_options::overwrite_existing, ec );

        if( ec )
        {
            git_error_set_str( GIT_ERROR_OS, ec.message().c_str() );
            return GIT_ERROR;
        }

        git_index*          copy = nullptr;
        const std::u8string scratchUtf8 = scratch.path.u8string();

        if( int rc = git_index_open( &copy, reinterpret_cast<const char*>( scratchUtf8.c_str() ) ) )
            return rc;

        std::unique_ptr<git_index, decltype( &git_index_free )> copyPtr( copy, &git_index_free );

        result = aEdit( copy );

        // The new index is stamped later, which would vouch for entries the old one could not
        if( result == 0 && stamp )
            result = smudgeRacyEntries( copy, *stamp );

        if( result == 0 )
            result = git_index_write( copy );

        if( result == 0 )
            result = lock.Commit( scratch.path );
    }

    git_index_read( repoIndex, 1 );
    return result;
}


bool IsLockContention( int aGitError )
{
    if( aGitError == GIT_ELOCKED || aGitError == GIT_EMODIFIED )
        return true;

#ifdef _WIN32
    // libgit2 clears the thread's Win32 error once it records its own, so the class is all that is left
    const git_error* error = git_error_last();

    if( aGitError == GIT_ERROR && error && error->klass == GIT_ERROR_OS )
        return true;
#endif

    return false;
}


int WithLockRetry( std::chrono::milliseconds aBudget, const std::function<int()>& aOp )
{
    using CLOCK = std::chrono::steady_clock;

    thread_local std::minstd_rand jitter( std::random_device{}() );

    CLOCK::time_point deadline = CLOCK::now() + aBudget;

    for( int attempt = 0;; ++attempt )
    {
        int result = aOp();

        if( !IsLockContention( result ) )
            return result;

        std::chrono::milliseconds base = LockRetryDelay( attempt );
        std::chrono::milliseconds delay = base / 2 + std::chrono::milliseconds( jitter() % ( base.count() + 1 ) );

        if( CLOCK::now() + delay > deadline )
            return result;

        std::this_thread::sleep_for( delay );
    }
}


REPO_ACTION_LOCK::REPO_ACTION_LOCK( const fs::path& aCommonDir, const char* aName ) :
        m_path( aCommonDir / aName )
{
}


std::unique_ptr<REPO_ACTION_LOCK> RepoActionLock( git_repository* aRepo, const char* aName )
{
    const char* commonDir = aRepo ? git_repository_commondir( aRepo ) : nullptr;

    if( !commonDir )
        return nullptr;

    return std::make_unique<REPO_ACTION_LOCK>( fs::path( reinterpret_cast<const char8_t*>( commonDir ) ), aName );
}


REPO_ACTION_LOCK::~REPO_ACTION_LOCK()
{
    Release();
}


int REPO_ACTION_LOCK::TryAcquire()
{
    if( m_held )
        return 0;

    m_error.clear();

    if( m_fd < 0 )
    {
#ifdef _WIN32
        m_fd = _wopen( m_path.c_str(), _O_CREAT | _O_RDWR | _O_BINARY | _O_NOINHERIT, _S_IREAD | _S_IWRITE );

        // Another user's lock file may be read only to us, which still locks
        if( m_fd < 0 )
            m_fd = _wopen( m_path.c_str(), _O_RDONLY | _O_BINARY | _O_NOINHERIT );
#else
        m_fd = open( m_path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0666 );

        // Another user's lock file may be read only to us, which still locks
        if( m_fd < 0 )
            m_fd = open( m_path.c_str(), O_RDONLY | O_CLOEXEC );
#endif

        if( m_fd < 0 )
        {
            m_error = std::generic_category().message( errno );
            git_error_set_str( GIT_ERROR_FILESYSTEM, ( "cannot create the repository lock file: " + m_error ).c_str() );
            return GIT_ERROR;
        }
    }

#ifdef _WIN32
    _lseek( m_fd, 0, SEEK_SET );
    int locked = _locking( m_fd, _LK_NBLCK, 1 );
    int contended = EACCES;
#else
    int locked = flock( m_fd, LOCK_EX | LOCK_NB );
    int contended = EWOULDBLOCK;
#endif

    if( locked != 0 )
    {
        int error = errno;

        if( error == contended )
        {
            git_error_set_str( GIT_ERROR_OS, "another git operation holds the repository" );
            return GIT_ELOCKED;
        }

        m_error = std::generic_category().message( error );
        git_error_set_str( GIT_ERROR_FILESYSTEM, ( "cannot lock the repository lock file: " + m_error ).c_str() );
        return GIT_ERROR;
    }

    m_held = true;
    return 0;
}


int REPO_ACTION_LOCK::Acquire( std::chrono::milliseconds aBudget )
{
    return WithLockRetry( aBudget,
                          [this]()
                          {
                              return TryAcquire();
                          } );
}


void REPO_ACTION_LOCK::Release()
{
    if( m_fd < 0 )
        return;

#ifdef _WIN32
    if( m_held )
    {
        _lseek( m_fd, 0, SEEK_SET );
        _locking( m_fd, _LK_UNLCK, 1 );
    }

    _close( m_fd );
#else
    close( m_fd );
#endif

    m_fd = -1;
    m_held = false;
}

} // namespace KIGIT

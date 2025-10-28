/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <history_lock.h>
#include <lockfile.h>
#include <advanced_config.h>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/log.h>
#include <wx/datetime.h>

#include <git2.h>

#define HISTORY_LOCK_TRACE wxT( "KICAD_HISTORY_LOCK" )


static wxString historyPath( const wxString& aProjectPath )
{
    wxFileName p( aProjectPath, wxEmptyString );
    p.AppendDir( wxS( ".history" ) );
    return p.GetPath();
}


HISTORY_LOCK_MANAGER::HISTORY_LOCK_MANAGER( const wxString& aProjectPath, int aStaleTimeoutSec ) :
        m_projectPath( aProjectPath ),
        m_historyPath( historyPath( aProjectPath ) ),
        m_repo( nullptr ),
        m_index( nullptr ),
        m_repoOwned( false ),
        m_indexOwned( false ),
        m_staleTimeoutSec( aStaleTimeoutSec <= 0 ? ADVANCED_CFG::GetCfg().m_HistoryLockStaleTimeout : aStaleTimeoutSec )
{
    wxLogTrace( HISTORY_LOCK_TRACE, "Attempting to acquire lock for project: %s (timeout: %d sec)",
               aProjectPath, m_staleTimeoutSec );

    // Layer 1: Acquire file-based lock to prevent cross-instance conflicts
    if( !acquireFileLock() )
    {
        wxLogTrace( HISTORY_LOCK_TRACE, "Failed to acquire file lock: %s", m_lockError );
        return;
    }

    // Layer 2: Open git repository
    if( !openRepository() )
    {
        wxLogTrace( HISTORY_LOCK_TRACE, "Failed to open repository: %s", m_lockError );
        return;
    }

    // Layer 3: Acquire git index lock for fine-grained operation safety
    if( !acquireIndexLock() )
    {
        wxLogTrace( HISTORY_LOCK_TRACE, "Failed to acquire index lock: %s", m_lockError );
        return;
    }

    wxLogTrace( HISTORY_LOCK_TRACE, "Successfully acquired all locks for project: %s", aProjectPath );
}


HISTORY_LOCK_MANAGER::~HISTORY_LOCK_MANAGER()
{
    wxLogTrace( HISTORY_LOCK_TRACE, "Releasing locks for project: %s", m_projectPath );

    // Release in reverse order of acquisition
    if( m_indexOwned && m_index )
    {
        git_index_free( m_index );
        m_index = nullptr;
    }

    if( m_repoOwned && m_repo )
    {
        git_repository_free( m_repo );
        m_repo = nullptr;
    }

    // m_fileLock automatically releases via RAII destructor
}


bool HISTORY_LOCK_MANAGER::IsLocked() const
{
    return m_fileLock && m_fileLock->Locked() && m_repo && m_index;
}


wxString HISTORY_LOCK_MANAGER::GetLockError() const
{
    return m_lockError;
}


wxString HISTORY_LOCK_MANAGER::GetLockHolder() const
{
    if( m_fileLock && !m_fileLock->Locked() )
    {
        return wxString::Format( wxS( "%s@%s" ),
                                m_fileLock->GetUsername(),
                                m_fileLock->GetHostname() );
    }
    return wxEmptyString;
}


bool HISTORY_LOCK_MANAGER::acquireFileLock()
{
    // Ensure history directory exists
    if( !wxDirExists( m_historyPath ) )
    {
        if( !wxFileName::Mkdir( m_historyPath, 0777, wxPATH_MKDIR_FULL ) )
        {
            m_lockError = wxString::Format(
                _( "Cannot create history directory: %s" ), m_historyPath );
            return false;
        }
    }

    // Check for stale lock before attempting acquisition
    if( IsLockStale( m_projectPath, m_staleTimeoutSec ) )
    {
        wxLogWarning( "Detected stale lock file, removing it" );
        BreakStaleLock( m_projectPath );
    }

    // Create lock file path: .history/.repo.lock
    wxFileName lockPath( m_historyPath, wxS( ".repo" ) );

    // Use existing LOCKFILE infrastructure
    m_fileLock = std::make_unique<LOCKFILE>( lockPath.GetFullPath(), true );

    if( !m_fileLock->Locked() )
    {
        m_lockError = wxString::Format(
            _( "History repository is locked by %s@%s" ),
            m_fileLock->GetUsername(),
            m_fileLock->GetHostname() );
        return false;
    }

    return true;
}


bool HISTORY_LOCK_MANAGER::openRepository()
{
    if( !wxDirExists( m_historyPath ) )
    {
        m_lockError = wxString::Format(
            _( "History directory does not exist: %s" ), m_historyPath );
        return false;
    }

    // Attempt to open existing repository
    int rc = git_repository_open( &m_repo, m_historyPath.mb_str().data() );

    if( rc != 0 )
    {
        const git_error* err = git_error_last();
        m_lockError = wxString::Format(
            _( "Failed to open git repository: %s" ),
            err ? wxString::FromUTF8( err->message ) : wxString( "Unknown error" ) );
        return false;
    }

    m_repoOwned = true;
    return true;
}


bool HISTORY_LOCK_MANAGER::acquireIndexLock()
{
    if( !m_repo )
    {
        m_lockError = _( "Cannot acquire index lock: repository not open" );
        return false;
    }

    // git_repository_index will fail if another process has .git/index.lock
    int rc = git_repository_index( &m_index, m_repo );

    if( rc != 0 )
    {
        const git_error* err = git_error_last();
        m_lockError = wxString::Format(
            _( "Failed to acquire git index lock (another operation in progress?): %s" ),
            err ? wxString::FromUTF8( err->message ) : wxString( "Unknown error" ) );
        return false;
    }

    m_indexOwned = true;
    return true;
}


bool HISTORY_LOCK_MANAGER::IsLockStale( const wxString& aProjectPath, int aStaleTimeoutSec )
{
    // Use config value if not explicitly specified
    if( aStaleTimeoutSec <= 0 )
        aStaleTimeoutSec = ADVANCED_CFG::GetCfg().m_HistoryLockStaleTimeout;

    wxString histPath = historyPath( aProjectPath );
    wxFileName lockPath( histPath, wxS( ".repo.lock" ) );

    if( !lockPath.FileExists() )
        return false; // No lock file exists

    wxDateTime modTime;
    if( !lockPath.GetTimes( nullptr, &modTime, nullptr ) )
        return false; // Can't determine age

    wxDateTime now = wxDateTime::Now();
    wxTimeSpan age = now - modTime;

    wxLogTrace( HISTORY_LOCK_TRACE, "Lock file age: %d seconds (stale threshold: %d)",
               (int)age.GetSeconds().ToLong(), aStaleTimeoutSec );

    return age.GetSeconds().ToLong() > aStaleTimeoutSec;
}


bool HISTORY_LOCK_MANAGER::BreakStaleLock( const wxString& aProjectPath )
{
    wxString histPath = historyPath( aProjectPath );
    wxFileName lockPath( histPath, wxS( ".repo.lock" ) );

    if( !lockPath.FileExists() )
        return true; // Already removed

    // Also remove the LOCKFILE-style lock file if it exists
    wxFileName lockFilePath( histPath, wxS( ".repo" ) );
    lockFilePath.SetName( FILEEXT::LockFilePrefix + lockFilePath.GetName() );
    lockFilePath.SetExt( lockFilePath.GetExt() + wxS( "." ) + FILEEXT::LockFileExtension );

    bool result = true;

    if( lockPath.FileExists() )
    {
        result = wxRemoveFile( lockPath.GetFullPath() );
        if( result )
            wxLogTrace( HISTORY_LOCK_TRACE, "Removed stale lock: %s", lockPath.GetFullPath() );
        else
            wxLogError( "Failed to remove stale lock: %s", lockPath.GetFullPath() );
    }

    if( lockFilePath.FileExists() )
    {
        bool lockFileResult = wxRemoveFile( lockFilePath.GetFullPath() );
        if( lockFileResult )
            wxLogTrace( HISTORY_LOCK_TRACE, "Removed stale lockfile: %s", lockFilePath.GetFullPath() );
        result = result && lockFileResult;
    }

    return result;
}

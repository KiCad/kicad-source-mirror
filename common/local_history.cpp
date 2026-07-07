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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <local_history.h>
#include <dialogs/dialog_restore_local_history.h>
#include <history_lock.h>
#include <paths.h>
#include <io/kicad/kicad_io_utils.h>
#include <lockfile.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <pgm_base.h>
#include <thread_pool.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>
#include <confirm.h>
#include <progress_reporter.h>

#include <kiplatform/io.h>

#include <git2.h>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/dir.h>
#include <wx/datetime.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <set>
#include <map>
#include <functional>
#include <cstring>

// Resolve the local-history storage directory for @p aProjectPath, honoring the
static wxString historyPath( const wxString& aProjectPath )
{
    return Pgm().GetSettingsManager().GetLocalHistoryDirForPath( aProjectPath );
}


// Join a saver-supplied relative path with the on-disk storage root for the
// active backup format and location.  Forward slashes in @p aRelativePath are
// preserved so libgit2 paths remain platform-neutral.
static wxString joinHistoryDestination( const wxString& aHistoryRoot,
                                        const wxString& aRelativePath )
{
    wxFileName fn( aRelativePath );

    if( fn.IsAbsolute() )
        return fn.GetFullPath();  // Defensive: should not happen with the new contract.

    // Prepend the history root while preserving any subdirectories supplied by the saver
    // (e.g. hierarchical sheet "sub/sheet.kicad_sch" must land at
    // "<root>/sub/sheet.kicad_sch", not "<root>/sheet.kicad_sch").
    wxArrayString dirs = fn.GetDirs();

    wxFileName dst;
    dst.AssignDir( aHistoryRoot );

    for( const wxString& d : dirs )
        dst.AppendDir( d );

    dst.SetFullName( fn.GetFullName() );
    return dst.GetFullPath();
}


static const wxString AUTOSAVE_PREFIX = wxS( "_autosave-" );


// Compare two files byte-for-byte.
static bool filesContentEqual( const wxString& aPathA, const wxString& aPathB )
{
    wxFFile fileA( aPathA, wxS( "rb" ) );
    wxFFile fileB( aPathB, wxS( "rb" ) );

    if( !fileA.IsOpened() || !fileB.IsOpened() )
        return false;

    wxFileOffset lenA = fileA.Length();
    wxFileOffset lenB = fileB.Length();

    if( lenA < 0 || lenB < 0 || lenA != lenB )
        return false;

    constexpr size_t  chunkSize = 64 * 1024;
    std::vector<char> bufA( chunkSize );
    std::vector<char> bufB( chunkSize );

    while( !fileA.Eof() )
    {
        size_t readA = fileA.Read( bufA.data(), chunkSize );
        size_t readB = fileB.Read( bufB.data(), chunkSize );

        if( readA != readB )
            return false;

        if( readA > 0 && std::memcmp( bufA.data(), bufB.data(), readA ) != 0 )
            return false;

        if( fileA.Error() || fileB.Error() )
            return false;
    }

    return true;
}


// Resolve the autosave-file destination for a given relative path.  In PROJECT_DIR
// mode the file lives next to the original (or under the same subdir for nested
// schematic sheets) with an "_autosave-" prefix on the basename.  In USER_DIR mode
// the file mirrors the project tree under the user data root with no name munging
// -- the per-project hash subdirectory already isolates autosave content.
static wxString resolveAutosaveDestination( const wxString& aAutosaveRoot,
                                            const wxString& aRelativePath,
                                            BACKUP_LOCATION aLocation )
{
    wxFileName rel( aRelativePath );
    wxFileName dst;
    dst.AssignDir( aAutosaveRoot );

    for( const wxString& d : rel.GetDirs() )
        dst.AppendDir( d );

    if( aLocation == BACKUP_LOCATION::PROJECT_DIR )
        dst.SetFullName( AUTOSAVE_PREFIX + rel.GetFullName() );
    else
        dst.SetFullName( rel.GetFullName() );

    return dst.GetFullPath();
}


// Compute the source-file path that an autosave destination corresponds to.
// In PROJECT_DIR mode the source is the same directory minus the "_autosave-"
// prefix.  In USER_DIR mode the source is the original under the project tree.
static wxString sourceForAutosaveFile( const wxString& aAutosavePath,
                                       const wxString& aProjectPath,
                                       const wxString& aAutosaveRoot,
                                       BACKUP_LOCATION aLocation )
{
    wxFileName autosave( aAutosavePath );

    if( aLocation == BACKUP_LOCATION::PROJECT_DIR )
    {
        wxString name = autosave.GetFullName();

        if( !name.StartsWith( AUTOSAVE_PREFIX ) )
            return wxEmptyString;

        autosave.SetFullName( name.Mid( AUTOSAVE_PREFIX.length() ) );
        return autosave.GetFullPath();
    }

    if( !aAutosavePath.StartsWith( aAutosaveRoot ) )
        return wxEmptyString;

    wxString rel = aAutosavePath.Mid( aAutosaveRoot.length() );
    wxFileName projFn( aProjectPath, wxEmptyString );

    return projFn.GetPathWithSep() + rel;
}


static bool commitSnapshotForProject( const wxString& aProjectPath, const std::vector<wxString>& aFiles,
                                      const wxString& aTitle );


// Single point of control: git local history is active only when backups are enabled and
// the backup format is incremental.  In zip mode we leave any pre-existing .history
// dormant on disk and skip all write/commit operations so we do not keep extending a
// history the user has switched off.  Read-only paths (HistoryExists, RestoreCommit,
// ShowRestoreDialog) intentionally bypass this gate so users can still browse dormant
// history after switching back.
static bool localHistoryEnabled()
{
    return Pgm().GetCommonSettings()->AutosaveUsesLocalHistory();
}


// Local history is project-scoped. When pcbnew or eeschema is launched
// standalone without a project, save paths can land anywhere on the
// filesystem (e.g. /tmp), and walking those directories to feed libgit2
// would be catastrophic.
static bool isProjectDirectory( const wxString& aProjectPath )
{
    if( aProjectPath.IsEmpty() || !wxDirExists( aProjectPath ) )
        return false;

    wxDir    dir( aProjectPath );
    wxString name;

    return dir.IsOpened()
           && dir.GetFirst( &name, wxString( wxS( "*." ) ) + FILEEXT::ProjectFileExtension, wxDIR_FILES );
}


// Top-level project entries that must survive a restore unchanged: git/history metadata, the
// transient restore staging directories (current and any timestamped retained copies), and
// the per-project zip backup directory produced by SETTINGS_MANAGER::BackupProject (named
// "<projectname>-backups").
static bool isRestoreProtectedEntry( const wxString& aName )
{
    return aName == wxS( ".history" ) || aName == wxS( ".git" ) || aName == wxS( "_restore_backup" )
           || aName.StartsWith( wxS( "_restore_backup_" ) ) || aName == wxS( "_restore_temp" )
           || aName == wxS( "_restore_discard" ) || aName.EndsWith( PROJECT_BACKUPS_DIR_SUFFIX );
}

LOCAL_HISTORY::LOCAL_HISTORY()
{
}

LOCAL_HISTORY::~LOCAL_HISTORY()
{
    WaitForPendingSave();
}

void LOCAL_HISTORY::NoteFileChange( const wxString& aFile )
{
    wxFileName fn( aFile );

    if( fn.GetFullName() == wxS( "fp-info-cache" ) || !localHistoryEnabled() )
        return;

    m_pendingFiles.insert( fn.GetFullPath() );
}


void LOCAL_HISTORY::RegisterSaver(
        const void* aSaverObject,
        const std::function<void( const wxString&, std::vector<HISTORY_FILE_DATA>& )>& aSaver )
{
    if( m_savers.find( aSaverObject ) != m_savers.end() )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] Saver %p already registered, skipping" ), aSaverObject );
        return;
    }

    m_savers[aSaverObject] = aSaver;
    wxLogTrace( traceAutoSave, wxS( "[history] Registered saver %p (total=%zu)" ), aSaverObject, m_savers.size() );
}


void LOCAL_HISTORY::UnregisterSaver( const void* aSaverObject )
{
    WaitForPendingSave();

    auto it = m_savers.find( aSaverObject );

    if( it != m_savers.end() )
    {
        m_savers.erase( it );
        wxLogTrace( traceAutoSave, wxS( "[history] Unregistered saver %p (total=%zu)" ),
                    aSaverObject, m_savers.size() );
    }
}


void LOCAL_HISTORY::ClearAllSavers()
{
    WaitForPendingSave();
    m_savers.clear();
    wxLogTrace( traceAutoSave, wxS( "[history] Cleared all savers" ) );
}


bool LOCAL_HISTORY::RunRegisteredSaversAndCommit( const wxString& aProjectPath, const wxString& aTitle,
                                                  const wxString& aTagFileType )
{
    if( !localHistoryEnabled() )
    {
        wxLogTrace( traceAutoSave, wxS( "Local history disabled, returning" ) );
        return true;
    }

    if( !isProjectDirectory( aProjectPath ) )
        return false;

    Init( aProjectPath );

    wxLogTrace( traceAutoSave,
                wxS( "[history] RunRegisteredSaversAndCommit start project='%s' title='%s' savers=%zu tag='%s'" ),
                aProjectPath, aTitle, m_savers.size(), aTagFileType );

    if( m_savers.empty() )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] no savers registered; skipping") );
        return false;
    }

    // Manual save must land; autosave is droppable because another tick will retry.
    if( !aTagFileType.IsEmpty() )
    {
        WaitForPendingSave();
    }
    else if( m_saveInProgress.load( std::memory_order_acquire ) )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] previous save still in progress; skipping cycle" ) );
        return false;
    }

    // Phase 1 (UI thread): call savers to collect serialized data
    std::vector<HISTORY_FILE_DATA> fileData;

    for( const auto& [saverObject, saver] : m_savers )
    {
        size_t before = fileData.size();
        saver( aProjectPath, fileData );
        wxLogTrace( traceAutoSave, wxS( "[history] saver %p produced %zu entries (total=%zu)" ),
                    saverObject, fileData.size() - before, fileData.size() );
    }

    // Reject entries with an empty or absolute relativePath; the saver contract requires
    // a project-relative path so we can dispatch to either the .history mirror or the
    // autosave-files root without ambiguity.
    fileData.erase( std::remove_if( fileData.begin(), fileData.end(),
            []( const HISTORY_FILE_DATA& entry )
            {
                if( entry.relativePath.IsEmpty() || wxFileName( entry.relativePath ).IsAbsolute() )
                {
                    wxLogTrace( traceAutoSave, wxS( "[history] filtered out entry with invalid path: '%s'" ),
                                entry.relativePath );
                    return true;
                }
                return false;
            } ),
            fileData.end() );

    if( fileData.empty() )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] saver set produced no entries; skipping" ) );
        return false;
    }

    // Phase 2: submit Prettify + file I/O + git to background thread
    m_saveInProgress.store( true, std::memory_order_release );

    m_pendingFuture = GetKiCadThreadPool().submit_task(
            [this, projectPath = aProjectPath, title = aTitle, tagFileType = aTagFileType,
             data = std::move( fileData )]() mutable -> bool
            {
                bool result = commitInBackground( projectPath, title, data, !tagFileType.IsEmpty() );

                if( !tagFileType.IsEmpty() )
                    TagSave( projectPath, tagFileType );

                m_saveInProgress.store( false, std::memory_order_release );
                return result;
            } );

    // Manual save must complete (commit + tag)
    if( !aTagFileType.IsEmpty() )
        WaitForPendingSave();

    return true;
}


bool LOCAL_HISTORY::RunRegisteredSaversAsAutosaveFiles( const wxString& aProjectPath )
{
    if( !Pgm().GetCommonSettings()->m_Backup.enabled )
        return true;

    if( m_savers.empty() )
    {
        wxLogTrace( traceAutoSave, wxS( "[autosave] no savers registered; skipping" ) );
        return false;
    }

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    BACKUP_LOCATION   location = mgr.GetCommonSettings()->m_Backup.location;
    wxString          autosaveRoot = mgr.GetAutosaveRootForProject( mgr.GetProjectForPath( aProjectPath ) );

    if( !PATHS::EnsurePathExists( autosaveRoot ) )
    {
        wxLogTrace( traceAutoSave, wxS( "[autosave] cannot create autosave root '%s'" ), autosaveRoot );
        return false;
    }

    std::vector<HISTORY_FILE_DATA> fileData;

    for( const auto& [saverObject, saver] : m_savers )
        saver( aProjectPath, fileData );

    bool anyWritten = false;

    for( HISTORY_FILE_DATA& entry : fileData )
    {
        if( entry.relativePath.IsEmpty() || wxFileName( entry.relativePath ).IsAbsolute() )
            continue;

        wxString dst = resolveAutosaveDestination( autosaveRoot, entry.relativePath, location );
        wxFileName dstFn( dst );

        if( !PATHS::EnsurePathExists( dstFn.GetPath() ) )
        {
            wxLogTrace( traceAutoSave, wxS( "[autosave] cannot create dir '%s'" ), dstFn.GetPath() );
            continue;
        }

        std::string buf;

        if( !entry.content.empty() )
        {
            buf = std::move( entry.content );

            if( entry.prettify )
                KICAD_FORMAT::Prettify( buf, entry.formatMode );
        }
        else if( !entry.sourcePath.IsEmpty() )
        {
            wxFFile src( entry.sourcePath, wxS( "rb" ) );

            if( !src.IsOpened() )
                continue;

            wxFileOffset len = src.Length();

            if( len < 0 )
                continue;

            buf.resize( static_cast<size_t>( len ) );

            if( len > 0 && src.Read( buf.data(), buf.size() ) != buf.size() )
            {
                buf.clear();
                continue;
            }
        }
        else
        {
            continue;
        }

        wxString err;

        if( KIPLATFORM::IO::AtomicWriteFile( dst, buf.data(), buf.size(), &err ) )
        {
            anyWritten = true;
            wxLogTrace( traceAutoSave, wxS( "[autosave] wrote %zu bytes to '%s'" ), buf.size(), dst );
        }
        else
        {
            wxLogTrace( traceAutoSave, wxS( "[autosave] write failed for '%s': %s" ), dst, err );
        }
    }

    return anyWritten;
}


// Enumerate every (autosave, source) pair under the per-project autosave root, without
// any modification-time filter.  Callers that want only files newer than their source
// (the recovery-prompt path) apply that filter themselves; cleanup callers want the
// full list so they can remove leftover autosave files even when the source has been
// re-saved and is newer.
static std::vector<std::pair<wxString, wxString>>
findAutosaveFilePairs( const wxString& aProjectPath )
{
    std::vector<std::pair<wxString, wxString>> results;

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    BACKUP_LOCATION   location = mgr.GetCommonSettings()->m_Backup.location;
    wxString          autosaveRoot = mgr.GetAutosaveRootForProject( mgr.GetProjectForPath( aProjectPath ) );

    if( !wxDirExists( autosaveRoot ) )
        return results;

    std::function<void( const wxString& )> walk = [&]( const wxString& aDir )
    {
        wxDir d( aDir );

        if( !d.IsOpened() )
            return;

        wxString name;
        bool cont = d.GetFirst( &name );

        while( cont )
        {
            wxFileName fn( aDir, name );
            wxString fullPath = fn.GetFullPath();

            if( wxDirExists( fullPath ) )
            {
                if( location == BACKUP_LOCATION::PROJECT_DIR
                    && ( name == wxS( ".history" ) || name.EndsWith( wxS( "-backups" ) ) ) )
                {
                    cont = d.GetNext( &name );
                    continue;
                }

                walk( fullPath );
            }
            else if( location != BACKUP_LOCATION::PROJECT_DIR
                     || fn.GetFullName().StartsWith( AUTOSAVE_PREFIX ) )
            {
                wxString src = sourceForAutosaveFile( fullPath, aProjectPath, autosaveRoot,
                                                     location );

                if( !src.IsEmpty() )
                    results.emplace_back( fullPath, src );
            }

            cont = d.GetNext( &name );
        }
    };

    walk( autosaveRoot );
    return results;
}


std::vector<std::pair<wxString, wxString>>
LOCAL_HISTORY::FindStaleAutosaveFiles( const wxString& aProjectPath, const std::vector<wxString>& aExtensions ) const
{
    std::vector<std::pair<wxString, wxString>> results;

    if( aExtensions.empty() )
        return results;

    for( auto& pair : findAutosaveFilePairs( aProjectPath ) )
    {
        wxFileName srcFn( pair.second );
        bool       match = false;

        for( const wxString& ext : aExtensions )
        {
            if( srcFn.GetExt().IsSameAs( ext, false ) )
            {
                match = true;
                break;
            }
        }

        if( !match )
            continue;

        wxDateTime srcTime;

        if( srcFn.FileExists() )
            srcTime = srcFn.GetModificationTime();

        wxDateTime autosaveTime = wxFileName( pair.first ).GetModificationTime();

        // mtime is only a pre-filter; cloud-sync clients bump the byte-identical autosave's
        // mtime past the source, so confirm the content actually diverges (issue 24126).
        bool stale = !srcTime.IsValid()
                     || ( autosaveTime.IsLaterThan( srcTime )
                          && !filesContentEqual( pair.first, pair.second ) );

        if( stale )
            results.emplace_back( std::move( pair ) );
    }

    return results;
}


void LOCAL_HISTORY::RemoveAutosaveFiles( const wxString& aProjectPath ) const
{
    // After a successful manual save the source typically has a newer mtime than its
    // autosave, so we cannot rely on FindStaleAutosaveFiles() here -- we need to remove
    // every autosave file associated with the project regardless of mtime.
    for( const auto& [autosavePath, srcPath] : findAutosaveFilePairs( aProjectPath ) )
    {
        if( wxFileExists( autosavePath ) )
            wxRemoveFile( autosavePath );
    }
}


void LOCAL_HISTORY::RemoveAutosaveFiles( const wxString& aProjectPath,
                                         const std::vector<wxString>& aSourcePaths ) const
{
    if( aSourcePaths.empty() )
        return;

    std::vector<wxFileName> targets;
    targets.reserve( aSourcePaths.size() );

    for( const wxString& src : aSourcePaths )
    {
        if( !src.IsEmpty() )
            targets.emplace_back( src );
    }

    if( targets.empty() )
        return;

    for( const auto& [autosavePath, srcPath] : findAutosaveFilePairs( aProjectPath ) )
    {
        wxFileName srcFn( srcPath );
        bool       match = false;

        for( const wxFileName& target : targets )
        {
            if( srcFn.SameAs( target ) )
            {
                match = true;
                break;
            }
        }

        if( match && wxFileExists( autosavePath ) )
            wxRemoveFile( autosavePath );
    }
}


bool LOCAL_HISTORY::commitInBackground( const wxString& aProjectPath, const wxString& aTitle,
                                        const std::vector<HISTORY_FILE_DATA>& aFileData, bool aIsManualSave )
{
    wxLogTrace( traceAutoSave, wxS( "[history] background: writing %zu entries for '%s'" ),
                aFileData.size(), aProjectPath );

    wxString hist = historyPath( aProjectPath );

    if( !PATHS::EnsurePathExists( hist ) )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] background: cannot create history root '%s'" ), hist );
        return false;
    }

    for( const HISTORY_FILE_DATA& entry : aFileData )
    {
        wxString dst = joinHistoryDestination( hist, entry.relativePath );
        wxFileName dstFn( dst );
        wxString   parent = dstFn.GetPath();

        if( !parent.IsEmpty() && !PATHS::EnsurePathExists( parent ) )
        {
            wxLogTrace( traceAutoSave, wxS( "[history] background: cannot create dir '%s'" ), parent );
            continue;
        }

        if( !entry.content.empty() )
        {
            std::string buf = entry.content;

            if( entry.prettify )
                KICAD_FORMAT::Prettify( buf, entry.formatMode );

            wxFFile fp( dst, wxS( "wb" ) );

            if( fp.IsOpened() )
            {
                fp.Write( buf.data(), buf.size() );
                fp.Close();
                wxLogTrace( traceAutoSave, wxS( "[history] background: wrote %zu bytes to '%s'" ), buf.size(), dst );
            }
            else
            {
                wxLogTrace( traceAutoSave, wxS( "[history] background: failed to open '%s' for writing" ), dst );
            }
        }
        else if( !entry.sourcePath.IsEmpty() )
        {
            wxCopyFile( entry.sourcePath, dst, true );
            wxLogTrace( traceAutoSave, wxS( "[history] background: copied '%s' -> '%s'" ), entry.sourcePath, dst );
        }
    }

    // Acquire locks using hybrid locking strategy
    HISTORY_LOCK_MANAGER lock( aProjectPath );

    if( !lock.IsLocked() )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] background: failed to acquire lock: %s" ), lock.GetLockError() );
        return false;
    }

    git_repository* repo = lock.GetRepository();
    git_index* index = lock.GetIndex();

    git_repository_set_workdir( repo, hist.mb_str().data(), false );

    // Stage all written files using their project-relative paths.  libgit2 needs forward
    // slashes on every platform, so normalize before adding to the index.
    for( const HISTORY_FILE_DATA& entry : aFileData )
    {
        wxString rel = entry.relativePath;
        rel.Replace( wxS( "\\" ), wxS( "/" ) );

        wxString abs = joinHistoryDestination( hist, entry.relativePath );

        if( !wxFileExists( abs ) )
            continue;

        git_index_add_bypath( index, rel.ToStdString().c_str() );
    }

    // Compare index to HEAD; if no diff -> abort to avoid empty commit.
    git_oid     head_oid;
    git_commit* head_commit = nullptr;
    git_tree*   head_tree = nullptr;

    bool headExists = ( git_reference_name_to_id( &head_oid, repo, "HEAD" ) == 0 )
                      && ( git_commit_lookup( &head_commit, repo, &head_oid ) == 0 )
                      && ( git_commit_tree( &head_tree, head_commit ) == 0 );

    git_tree* rawIndexTree = nullptr;
    git_oid   index_tree_oid;

    if( git_index_write_tree( &index_tree_oid, index ) != 0 )
    {
        if( head_tree )
            git_tree_free( head_tree );

        if( head_commit )
            git_commit_free( head_commit );

        wxLogTrace( traceAutoSave, wxS("[history] background: failed to write index tree" ) );
        return false;
    }

    git_tree_lookup( &rawIndexTree, repo, &index_tree_oid );
    std::unique_ptr<git_tree, decltype( &git_tree_free )> indexTree( rawIndexTree, &git_tree_free );

    bool hasChanges = true;

    if( headExists )
    {
        git_diff* diff = nullptr;

        if( git_diff_tree_to_tree( &diff, repo, head_tree, indexTree.get(), nullptr ) == 0 )
        {
            hasChanges = git_diff_num_deltas( diff ) > 0;
            wxLogTrace( traceAutoSave, wxS( "[history] background: diff deltas=%u" ),
                        (unsigned) git_diff_num_deltas( diff ) );
            git_diff_free( diff );
        }
    }
    else
    {
        // No HEAD: skip commit if staged matches disk, so an idle autosave on a fresh
        // project doesn't leave an untagged HEAD that triggers a no-op restore prompt.
        bool stagedMatchesDisk = true;

        for( const HISTORY_FILE_DATA& entry : aFileData )
        {
            wxString diskPath = aProjectPath + wxFileName::GetPathSeparator() + entry.relativePath;
            wxString histPath = joinHistoryDestination( hist, entry.relativePath );

            if( !wxFileExists( diskPath ) || !wxFileExists( histPath ) )
            {
                stagedMatchesDisk = false;
                break;
            }

            wxFFile diskFile( diskPath, wxT( "rb" ) );
            wxFFile histFile( histPath, wxT( "rb" ) );

            if( !diskFile.IsOpened() || !histFile.IsOpened() || diskFile.Length() != histFile.Length() )
            {
                stagedMatchesDisk = false;
                break;
            }

            size_t      len = static_cast<size_t>( diskFile.Length() );
            std::string diskBuf( len, '\0' );
            std::string histBuf( len, '\0' );

            if( diskFile.Read( diskBuf.data(), len ) != len
                    || histFile.Read( histBuf.data(), len ) != len
                    || diskBuf != histBuf )
            {
                stagedMatchesDisk = false;
                break;
            }
        }

        if( stagedMatchesDisk && !aIsManualSave )
        {
            wxLogTrace( traceAutoSave, wxS( "[history] background: first commit; staged matches disk -- skipping" ) );
            hasChanges = false;
        }
    }

    if( head_tree )
        git_tree_free( head_tree );

    if( head_commit )
        git_commit_free( head_commit );

    if( !hasChanges )
    {
        wxLogTrace( traceAutoSave, wxS("[history] background: no changes detected; no commit") );

        // Manual save matching HEAD: amend the prior message so the user's explicit save
        // shows in the history dialog. Skip if HEAD already has this title.
        if( !aTitle.IsEmpty() && aTitle != wxS( "Autosave" ) )
        {
            git_oid head_oid_amend;

            if( git_reference_name_to_id( &head_oid_amend, repo, "HEAD" ) == 0 )
            {
                git_commit* head_commit_amend = nullptr;

                if( git_commit_lookup( &head_commit_amend, repo, &head_oid_amend ) == 0 )
                {
                    wxString existingMsg = wxString::FromUTF8( git_commit_message( head_commit_amend ) );
                    existingMsg.Trim( true ).Trim( false );

                    if( existingMsg != aTitle )
                    {
                        git_oid amended_oid;
                        int     amend_rc = git_commit_amend( &amended_oid, head_commit_amend, "HEAD", nullptr, nullptr,
                                                             nullptr, aTitle.mb_str().data(), nullptr );

                        if( amend_rc == 0 )
                            wxLogTrace( traceAutoSave, wxS( "[history] background: amended HEAD message '%s' -> '%s'" ),
                                        existingMsg, aTitle );
                        else
                            wxLogTrace( traceAutoSave, wxS( "[history] background: amend failed rc=%d" ), amend_rc );
                    }

                    git_commit_free( head_commit_amend );
                }
            }
        }

        return false; // Nothing new; skip commit.
    }

    git_signature* rawSig = nullptr;
    git_signature_now( &rawSig, "KiCad", "noreply@kicad.org" );
    std::unique_ptr<git_signature, decltype( &git_signature_free )> sig( rawSig, &git_signature_free );

    git_commit* parent = nullptr;
    git_oid     parent_id;
    int         parents = 0;

    if( git_reference_name_to_id( &parent_id, repo, "HEAD" ) == 0 )
    {
        if( git_commit_lookup( &parent, repo, &parent_id ) == 0 )
            parents = 1;
    }

    wxString          msg = aTitle.IsEmpty() ? wxString( "Autosave" ) : aTitle;
    git_oid           commit_id;
    const git_commit* constParent = parent;

    int rc = git_commit_create( &commit_id, repo, "HEAD", sig.get(), sig.get(), nullptr,
                       msg.mb_str().data(), indexTree.get(), parents,
                       parents ? &constParent : nullptr );

    if( rc == 0 )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] background: commit created %s (%s entries=%zu)" ),
                    wxString::FromUTF8( git_oid_tostr_s( &commit_id ) ), msg, aFileData.size() );
    }
    else
    {
        wxLogTrace( traceAutoSave, wxS( "[history] background: commit failed rc=%d" ), rc );
    }

    if( parent )
        git_commit_free( parent );

    git_index_write( index );
    return rc == 0;
}


void LOCAL_HISTORY::WaitForPendingSave()
{
    if( m_pendingFuture.valid() )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] waiting for pending background save" ) );
        m_pendingFuture.get();
    }
}


bool LOCAL_HISTORY::CommitPending()
{
    std::vector<wxString> files( m_pendingFiles.begin(), m_pendingFiles.end() );
    m_pendingFiles.clear();
    return CommitSnapshot( files, wxS( "Autosave" ) );
}


bool LOCAL_HISTORY::Init( const wxString& aProjectPath )
{
    if( !isProjectDirectory( aProjectPath ) )
        return false;

    if( !localHistoryEnabled() )
        return true;

    wxString hist = historyPath( aProjectPath );

    if( !wxDirExists( hist ) )
    {
        // EnsurePathExists creates intermediate directories as needed, which is required
        // for USER_DIR mode where the parent (e.g., ~/.config/kicad/<ver>/local_history/)
        // may not yet exist.  In PROJECT_DIR mode it falls back to a single mkdir.
        if( !PATHS::EnsurePathExists( hist ) )
            return false;
    }

    git_repository* rawRepo = nullptr;

    if( git_repository_open( &rawRepo, hist.mb_str().data() ) != 0 )
    {
        if( git_repository_init( &rawRepo, hist.mb_str().data(), 0 ) != 0 )
            return false;

        wxFileName ignoreFile( hist, wxS( ".gitignore" ) );
        if( !ignoreFile.FileExists() )
        {
            wxFFile f( ignoreFile.GetFullPath(), wxT( "w" ) );
            if( f.IsOpened() )
            {
                f.Write( wxS( "# KiCad local history exclusions. Edit to add your own rules.\n"
                              "fp-info-cache\n"
                              "*-backups/\n" ) );
                f.Close();
            }
        }

        wxFileName readmeFile( hist, wxS( "README.txt" ) );

        if( !readmeFile.FileExists() )
        {
            wxFFile f( readmeFile.GetFullPath(), wxT( "w" ) );

            if( f.IsOpened() )
            {
                f.Write( wxS( "KiCad Local History Directory\n"
                              "=============================\n\n"
                              "This directory contains automatic snapshots of your project files.\n"
                              "KiCad periodically saves copies of your work here, allowing you to\n"
                              "recover from accidental changes or data loss.\n\n"
                              "You can browse and restore previous versions through KiCad's\n"
                              "File > Local History menu.\n\n"
                              "To disable this feature:\n"
                              "  Preferences > Common > Project Backup > Enable automatic backups\n\n"
                              "This directory can be safely deleted if you no longer need the\n"
                              "history, but doing so will permanently remove all saved snapshots.\n" ) );
                f.Close();
            }
        }
    }

    git_repository_free( rawRepo );

    return true;
}


// Helper function to commit files using an already-acquired lock
enum class SNAPSHOT_COMMIT_RESULT
{
    Error,
    NoChanges,
    Committed
};


static SNAPSHOT_COMMIT_RESULT commitSnapshotWithLock( git_repository* repo, git_index* index,
                                                      const wxString& aHistoryPath, const wxString& aProjectPath,
                                                      const std::vector<wxString>& aFiles, const wxString& aTitle )
{
    std::vector<std::string> filesArrStr;

    for( const wxString& file : aFiles )
    {
        wxFileName src( file );
        wxString   relPath;

        if( src.GetFullPath().StartsWith( aProjectPath + wxFILE_SEP_PATH ) )
            relPath = src.GetFullPath().Mid( aProjectPath.length() + 1 );
        else
            relPath = src.GetFullName(); // Fallback (should not normally happen)

        relPath.Replace( "\\", "/" ); // libgit2 needs forward slashes on all platforms
        std::string relPathStr = relPath.ToStdString();

        unsigned int status = 0;
        int          rc = git_status_file( &status, repo, relPathStr.data() );

        if( rc == 0 && status != 0 )
        {
            wxLogTrace( traceAutoSave, wxS( "File %s status %d " ), relPath, status );
            filesArrStr.emplace_back( relPathStr );
        }
        else if( rc != 0 )
        {
            wxLogTrace( traceAutoSave, wxS( "File %s status error %d " ), relPath, rc );
            filesArrStr.emplace_back( relPathStr ); // Add anyway even if the file is untracked.
        }
    }

    std::vector<char*> cStrings( filesArrStr.size() );

    for( size_t i = 0; i < filesArrStr.size(); i++ )
        cStrings[i] = filesArrStr[i].data();

    git_strarray filesArrGit;
    filesArrGit.count = filesArrStr.size();
    filesArrGit.strings = cStrings.data();

    if( filesArrStr.size() == 0 )
    {
        wxLogTrace( traceAutoSave, wxS( "No changes, skipping" ) );
        return SNAPSHOT_COMMIT_RESULT::NoChanges;
    }

    int rc = git_index_add_all( index, &filesArrGit, GIT_INDEX_ADD_DISABLE_PATHSPEC_MATCH | GIT_INDEX_ADD_FORCE, NULL,
                                NULL );
    wxLogTrace( traceAutoSave, wxS( "Adding %zu files, rc %d" ), filesArrStr.size(), rc );

    if( rc != 0 )
        return SNAPSHOT_COMMIT_RESULT::Error;

    git_oid tree_id;
    if( git_index_write_tree( &tree_id, index ) != 0 )
        return SNAPSHOT_COMMIT_RESULT::Error;

    git_tree* rawTree = nullptr;
    git_tree_lookup( &rawTree, repo, &tree_id );
    std::unique_ptr<git_tree, decltype( &git_tree_free )> tree( rawTree, &git_tree_free );

    git_signature* rawSig = nullptr;
    git_signature_now( &rawSig, "KiCad", "noreply@kicad.org" );
    std::unique_ptr<git_signature, decltype( &git_signature_free )> sig( rawSig,
                                                                         &git_signature_free );

    git_commit* rawParent = nullptr;
    git_oid     parent_id;
    int         parents = 0;

    if( git_reference_name_to_id( &parent_id, repo, "HEAD" ) == 0 )
    {
        git_commit_lookup( &rawParent, repo, &parent_id );
        parents = 1;
    }

    std::unique_ptr<git_commit, decltype( &git_commit_free )> parent( rawParent,
                                                                       &git_commit_free );

    git_tree* rawParentTree = nullptr;

    if( parent )
        git_commit_tree( &rawParentTree, parent.get() );

    std::unique_ptr<git_tree, decltype( &git_tree_free )> parentTree( rawParentTree, &git_tree_free );

    git_diff* rawDiff = nullptr;
    git_diff_tree_to_index( &rawDiff, repo, parentTree.get(), index, nullptr );
    std::unique_ptr<git_diff, decltype( &git_diff_free )> diff( rawDiff, &git_diff_free );

    size_t numChangedFiles = git_diff_num_deltas( diff.get() );

    if( numChangedFiles == 0 )
    {
        wxLogTrace( traceAutoSave, wxS( "No actual changes in tree, skipping commit" ) );
        return SNAPSHOT_COMMIT_RESULT::NoChanges;
    }

    wxString msg;

    if( !aTitle.IsEmpty() )
        msg << aTitle << wxS( ": " );

    msg << numChangedFiles << wxS( " files changed" );

    for( size_t i = 0; i < numChangedFiles; ++i )
    {
        const git_diff_delta* delta = git_diff_get_delta( diff.get(), i );
        git_patch* rawPatch = nullptr;
        git_patch_from_diff( &rawPatch, diff.get(), i );
        std::unique_ptr<git_patch, decltype( &git_patch_free )> patch( rawPatch,
                                                                       &git_patch_free );
        size_t context = 0, adds = 0, dels = 0;
        git_patch_line_stats( &context, &adds, &dels, patch.get() );
        size_t updated = std::min( adds, dels );
        adds -= updated;
        dels -= updated;
        msg << wxS( "\n" ) << wxString::FromUTF8( delta->new_file.path )
            << wxS( " " ) << adds << wxS( "/" ) << dels << wxS( "/" ) << updated;
    }

    git_oid commit_id;
    git_commit* parentPtr = parent.get();
    const git_commit* constParentPtr = parentPtr;
    if( git_commit_create( &commit_id, repo, "HEAD", sig.get(), sig.get(), nullptr, msg.mb_str().data(), tree.get(),
                           parents, parentPtr ? &constParentPtr : nullptr )
        != 0 )
    {
        return SNAPSHOT_COMMIT_RESULT::Error;
    }

    git_index_write( index );
    return SNAPSHOT_COMMIT_RESULT::Committed;
}


// Internal entry point used when the project root is already known. The public
// CommitSnapshot() derives the project from aFiles[0], which is unsafe when the
// caller has collected files recursively (the first entry can live in a subdirectory).
static bool commitSnapshotForProject( const wxString& aProjectPath, const std::vector<wxString>& aFiles,
                                      const wxString& aTitle )
{
    wxString hist = historyPath( aProjectPath );

    HISTORY_LOCK_MANAGER lock( aProjectPath );

    if( !lock.IsLocked() )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] commitSnapshotForProject failed to acquire lock: %s" ),
                    lock.GetLockError() );
        return false;
    }

    return commitSnapshotWithLock( lock.GetRepository(), lock.GetIndex(), hist, aProjectPath, aFiles, aTitle )
           == SNAPSHOT_COMMIT_RESULT::Committed;
}


bool LOCAL_HISTORY::CommitSnapshot( const std::vector<wxString>& aFiles, const wxString& aTitle )
{
    if( aFiles.empty() || !localHistoryEnabled() )
    {
        return true;
    }

    wxString proj = wxFileName( aFiles[0] ).GetPath();

    if( !isProjectDirectory( proj ) )
        return false;

    Init( proj );
    return commitSnapshotForProject( proj, aFiles, aTitle );
}


// Limit snapshots to KiCad project artifacts (kicad_* extensions and the no-extension
// lib-tables) so unrelated files in the project dir don't end up in .history.
static bool isKiCadProjectFile( const wxFileName& aFile )
{
    wxString name = aFile.GetFullName();

    if( name == wxS( "sym-lib-table" ) || name == wxS( "fp-lib-table" ) )
        return true;

    return aFile.GetExt().StartsWith( wxS( "kicad_" ) );
}


// Helper to collect KiCad project files (excluding .history, backups, transient caches,
// and any non-KiCad files such as user PDFs or notes).
// Skips subtrees that contain a kicad_pro file since they belong to nested projects.
static void collectProjectFiles( const wxString& aProjectPath, std::vector<wxString>& aFiles )
{
    wxDir dir( aProjectPath );

    if( !dir.IsOpened() )
        return;

    // Collect recursively. Flag top-level to avoid hitting the same logic for nested projects
    std::function<void( const wxString&, bool )> collect =
            [&]( const wxString& path, bool topLevel )
    {
        if( !topLevel && isProjectDirectory( path ) )
        {
            wxLogTrace( traceAutoSave,
                        wxS( "[history] collectProjectFiles: Skipping nested project at %s" ),
                        path );
            return;
        }

        wxString name;
        wxDir    d( path );

        if( !d.IsOpened() )
            return;

        bool cont = d.GetFirst( &name );

        while( cont )
        {
            if( topLevel && isRestoreProtectedEntry( name ) )
            {
                cont = d.GetNext( &name );
                continue;
            }

            wxFileName fn( path, name );
            wxString   fullPath = fn.GetFullPath();

            if( wxFileName::DirExists( fullPath ) )
            {
                collect( fullPath, false );
            }
            else if( fn.FileExists() && fn.GetFullName() != wxS( "fp-info-cache" ) && isKiCadProjectFile( fn ) )
            {
                aFiles.push_back( fn.GetFullPath() );
            }

            cont = d.GetNext( &name );
        }
    };

    collect( aProjectPath, true );
}


bool LOCAL_HISTORY::CommitFullProjectSnapshot( const wxString& aProjectPath, const wxString& aTitle )
{
    if( !isProjectDirectory( aProjectPath ) || !localHistoryEnabled() )
        return false;

    std::vector<wxString> files;
    collectProjectFiles( aProjectPath, files );

    if( files.empty() )
        return false;

    Init( aProjectPath );
    return commitSnapshotForProject( aProjectPath, files, aTitle );
}

bool LOCAL_HISTORY::HistoryExists( const wxString& aProjectPath )
{
    return wxDirExists( historyPath( aProjectPath ) );
}

// Add a Save_<type>_N tag and move Last_Save_<type> to the current HEAD using an already-open
// repo. Shared by TagSave and the restore path, the latter holds the history lock itself and so
// cannot go through TagSave (which would try to re-acquire it).
static bool tagSaveAtHead( git_repository* repo, const wxString& aFileType )
{
    if( !repo )
        return false;

    git_oid head;
    if( git_reference_name_to_id( &head, repo, "HEAD" ) != 0 )
        return false;

    wxString tagName;
    int i = 1;
    git_reference* ref = nullptr;
    do
    {
        tagName.Printf( wxS( "Save_%s_%d" ), aFileType, i++ );
    } while( git_reference_lookup( &ref, repo, ( wxS( "refs/tags/" ) + tagName ).mb_str().data() ) == 0 );

    git_oid tag_oid;
    git_object* head_obj = nullptr;
    git_object_lookup( &head_obj, repo, &head, GIT_OBJECT_COMMIT );
    git_tag_create_lightweight( &tag_oid, repo, tagName.mb_str().data(), head_obj, 0 );
    git_object_free( head_obj );

    wxString lastName;
    lastName.Printf( wxS( "Last_Save_%s" ), aFileType );
    if( git_reference_lookup( &ref, repo, ( wxS( "refs/tags/" ) + lastName ).mb_str().data() ) == 0 )
    {
        git_reference_delete( ref );
        git_reference_free( ref );
    }

    git_oid last_tag_oid;
    git_object* head_obj2 = nullptr;
    git_object_lookup( &head_obj2, repo, &head, GIT_OBJECT_COMMIT );
    git_tag_create_lightweight( &last_tag_oid, repo, lastName.mb_str().data(), head_obj2, 0 );
    git_object_free( head_obj2 );

    return true;
}


bool LOCAL_HISTORY::TagSave( const wxString& aProjectPath, const wxString& aFileType )
{
    if( !localHistoryEnabled() )
        return true;

    if( !isProjectDirectory( aProjectPath ) )
        return false;

    HISTORY_LOCK_MANAGER lock( aProjectPath );

    if( !lock.IsLocked() )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] TagSave: Failed to acquire lock for %s" ), aProjectPath );
        return false;
    }

    return tagSaveAtHead( lock.GetRepository(), aFileType );
}

bool LOCAL_HISTORY::HeadNewerThanLastSave( const wxString& aProjectPath )
{
    wxString hist = historyPath( aProjectPath );
    git_repository* repo = nullptr;

    if( git_repository_open( &repo, hist.mb_str().data() ) != 0 )
        return false;

    git_oid head_oid;
    if( git_reference_name_to_id( &head_oid, repo, "HEAD" ) != 0 )
    {
        git_repository_free( repo );
        return false;
    }

    git_commit* head_commit = nullptr;
    git_commit_lookup( &head_commit, repo, &head_oid );
    git_time_t head_time = git_commit_time( head_commit );

    git_strarray tags;
    git_tag_list_match( &tags, "Last_Save_*", repo );
    git_time_t save_time = 0;

    for( size_t i = 0; i < tags.count; ++i )
    {
    git_reference* ref = nullptr;
        if( git_reference_lookup( &ref, repo,
                                  ( wxS( "refs/tags/" ) +
                                    wxString::FromUTF8( tags.strings[i] ) ).mb_str().data() ) == 0 )
        {
            const git_oid* oid = git_reference_target( ref );
            git_commit* c = nullptr;
            if( git_commit_lookup( &c, repo, oid ) == 0 )
            {
                git_time_t t = git_commit_time( c );
                if( t > save_time )
                    save_time = t;
                git_commit_free( c );
            }
            git_reference_free( ref );
        }
    }

    git_strarray_free( &tags );
    git_commit_free( head_commit );
    git_repository_free( repo );

    // If there are no Last_Save tags but there IS a HEAD commit, we have autosaved
    // data that was never explicitly saved - offer to restore
    if( save_time == 0 )
        return true;

    return head_time > save_time;
}

bool LOCAL_HISTORY::CommitDuplicateOfLastSave( const wxString& aProjectPath, const wxString& aFileType,
                                               const wxString& aMessage )
{
    if( !localHistoryEnabled() )
        return true;

    if( !isProjectDirectory( aProjectPath ) )
        return false;

    HISTORY_LOCK_MANAGER lock( aProjectPath );

    if( !lock.IsLocked() )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] CommitDuplicateOfLastSave: Failed to acquire lock for %s" ), aProjectPath );
        return false;
    }

    git_repository* repo = lock.GetRepository();

    if( !repo )
        return false;

    wxString lastName; lastName.Printf( wxS("Last_Save_%s"), aFileType );
    git_reference* lastRef = nullptr;
    if( git_reference_lookup( &lastRef, repo, ( wxS("refs/tags/") + lastName ).mb_str().data() ) != 0 )
        return false; // no tag to duplicate
    std::unique_ptr<git_reference, decltype( &git_reference_free )> lastRefPtr( lastRef, &git_reference_free );

    const git_oid* lastOid = git_reference_target( lastRef );
    git_commit* lastCommit = nullptr;
    if( git_commit_lookup( &lastCommit, repo, lastOid ) != 0 )
        return false;
    std::unique_ptr<git_commit, decltype( &git_commit_free )> lastCommitPtr( lastCommit, &git_commit_free );

    git_tree* lastTree = nullptr;
    git_commit_tree( &lastTree, lastCommit );
    std::unique_ptr<git_tree, decltype( &git_tree_free )> lastTreePtr( lastTree, &git_tree_free );

    // Parent will be current HEAD (to keep linear history)
    git_oid headOid;
    git_commit* headCommit = nullptr;
    int parents = 0;
    const git_commit* parentArray[1];
    if( git_reference_name_to_id( &headOid, repo, "HEAD" ) == 0 &&
        git_commit_lookup( &headCommit, repo, &headOid ) == 0 )
    {
        parentArray[0] = headCommit;
        parents = 1;
    }

    git_signature* sigRaw = nullptr;
    git_signature_now( &sigRaw, "KiCad", "noreply@kicad.org" );
    std::unique_ptr<git_signature, decltype( &git_signature_free )> sig( sigRaw, &git_signature_free );

    wxString msg = aMessage.IsEmpty() ? wxS("Discard unsaved ") + aFileType : aMessage;
    git_oid newCommitOid;
    int rc = git_commit_create( &newCommitOid, repo, "HEAD", sig.get(), sig.get(), nullptr,
                                msg.mb_str().data(), lastTree, parents, parents ? parentArray : nullptr );
    if( headCommit ) git_commit_free( headCommit );
    if( rc != 0 )
        return false;

    // Move Last_Save tag to new commit
    git_reference* existing = nullptr;
    if( git_reference_lookup( &existing, repo, ( wxS("refs/tags/") + lastName ).mb_str().data() ) == 0 )
    {
        git_reference_delete( existing );
        git_reference_free( existing );
    }
    git_object* newCommitObj = nullptr;
    if( git_object_lookup( &newCommitObj, repo, &newCommitOid, GIT_OBJECT_COMMIT ) == 0 )
    {
        git_tag_create_lightweight( &newCommitOid, repo, lastName.mb_str().data(), newCommitObj, 0 );
        git_object_free( newCommitObj );
    }
    return true;
}

static size_t dirSizeRecursive( const wxString& path )
{
    size_t total = 0;
    wxDir dir( path );
    if( !dir.IsOpened() )
        return 0;
    wxString name;
    bool cont = dir.GetFirst( &name );
    while( cont )
    {
        wxFileName fn( path, name );
        wxString   fullPath = fn.GetFullPath();

        if( wxFileName::DirExists( fullPath ) )
            total += dirSizeRecursive( fullPath );
        else if( fn.FileExists() )
            total += (size_t) fn.GetSize().GetValue();
        cont = dir.GetNext( &name );
    }
    return total;
}

// Copy tree and all blob objects directly between ODBs
static bool copyTreeObjects( git_repository* aSrcRepo, git_odb* aSrcOdb, git_odb* aDstOdb, const git_oid* aTreeOid,
                             std::set<git_oid, bool ( * )( const git_oid&, const git_oid& )>& aCopied )
{
    if( aCopied.count( *aTreeOid ) )
        return true;

    git_odb_object* obj = nullptr;

    if( git_odb_read( &obj, aSrcOdb, aTreeOid ) != 0 )
        return false;

    git_oid written;
    int     err = git_odb_write( &written, aDstOdb, git_odb_object_data( obj ), git_odb_object_size( obj ),
                                 git_odb_object_type( obj ) );
    git_odb_object_free( obj );

    if( err != 0 )
        return false;

    aCopied.insert( *aTreeOid );

    git_tree* tree = nullptr;

    if( git_tree_lookup( &tree, aSrcRepo, aTreeOid ) != 0 )
        return false;

    size_t cnt = git_tree_entrycount( tree );

    for( size_t i = 0; i < cnt; ++i )
    {
        const git_tree_entry* entry = git_tree_entry_byindex( tree, i );
        const git_oid*        entryId = git_tree_entry_id( entry );

        if( aCopied.count( *entryId ) )
            continue;

        if( git_tree_entry_type( entry ) == GIT_OBJECT_TREE )
        {
            if( !copyTreeObjects( aSrcRepo, aSrcOdb, aDstOdb, entryId, aCopied ) )
            {
                git_tree_free( tree );
                return false;
            }
        }
        else if( git_tree_entry_type( entry ) == GIT_OBJECT_BLOB )
        {
            git_odb_object* blobObj = nullptr;

            if( git_odb_read( &blobObj, aSrcOdb, entryId ) == 0 )
            {
                git_oid blobWritten;

                if( git_odb_write( &blobWritten, aDstOdb, git_odb_object_data( blobObj ),
                                   git_odb_object_size( blobObj ), git_odb_object_type( blobObj ) )
                    != 0 )
                {
                    git_odb_object_free( blobObj );
                    git_tree_free( tree );
                    return false;
                }

                git_odb_object_free( blobObj );
                aCopied.insert( *entryId );
            }
        }
    }

    git_tree_free( tree );
    return true;
}


// Compact loose objects into a packfile and remove the originals.
// Equivalent to git gc
static bool compactRepository( git_repository* aRepo, PROGRESS_REPORTER* aReporter = nullptr )
{
    git_packbuilder* pb = nullptr;

    if( git_packbuilder_new( &pb, aRepo ) != 0 )
        return false;

    git_revwalk* walk = nullptr;

    if( git_revwalk_new( &walk, aRepo ) != 0 )
    {
        git_packbuilder_free( pb );
        return false;
    }

    git_revwalk_push_head( walk );
    git_oid oid;

    while( git_revwalk_next( &oid, walk ) == 0 )
    {
        if( git_packbuilder_insert_commit( pb, &oid ) != 0 )
        {
            git_revwalk_free( walk );
            git_packbuilder_free( pb );
            return false;
        }
    }

    git_revwalk_free( walk );

    if( aReporter )
    {
        git_packbuilder_set_callbacks(
                pb,
                []( int aStage, uint32_t aCurrent, uint32_t aTotal, void* aPayload )
                {
                    auto* reporter = static_cast<PROGRESS_REPORTER*>( aPayload );

                    if( aTotal > 0 )
                        reporter->SetCurrentProgress( (double) aCurrent / aTotal );

                    reporter->KeepRefreshing();
                    return 0;
                },
                aReporter );
    }

    if( git_packbuilder_write( pb, nullptr, 0, nullptr, nullptr ) != 0 )
    {
        git_packbuilder_free( pb );
        return false;
    }

    git_packbuilder_free( pb );

    wxString objPath = wxString::FromUTF8( git_repository_path( aRepo ) ) + wxS( "objects" );
    wxDir    objDir( objPath );

    if( objDir.IsOpened() )
    {
        wxArrayString toRemove;
        wxString      name;
        bool          cont = objDir.GetFirst( &name, wxEmptyString, wxDIR_DIRS );

        while( cont )
        {
            if( name.length() == 2 )
                toRemove.Add( objPath + wxFileName::GetPathSeparator() + name );

            cont = objDir.GetNext( &name );
        }

        for( const wxString& dir : toRemove )
            wxFileName::Rmdir( dir, wxPATH_RMDIR_RECURSIVE );
    }

    return true;
}


bool LOCAL_HISTORY::EnforceSizeLimit( const wxString& aProjectPath, size_t aMaxBytes, PROGRESS_REPORTER* aReporter )
{
    if( aMaxBytes == 0 )
        return false;

    wxString hist = historyPath( aProjectPath );

    if( !wxDirExists( hist ) )
        return false;

    size_t current = dirSizeRecursive( hist );

    if( current <= aMaxBytes )
        return true; // within limit

    HISTORY_LOCK_MANAGER lock( aProjectPath );

    if( !lock.IsLocked() )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] EnforceSizeLimit: Failed to acquire lock for %s" ), aProjectPath );
        return false;
    }

    git_repository* repo = lock.GetRepository();

    if( !repo )
        return false;

    if( aReporter )
        aReporter->Report( _( "Compacting local history..." ) );

    // Pack loose objects first. Can bring size within limit without a full rebuild.
    compactRepository( repo, aReporter );

    current = dirSizeRecursive( hist );

    if( current <= aMaxBytes )
        return true; // within limit after compaction

    // Collect commits newest-first using revwalk
    git_revwalk* walk = nullptr;
    git_revwalk_new( &walk, repo );
    git_revwalk_sorting( walk, GIT_SORT_TIME );
    git_revwalk_push_head( walk );
    std::vector<git_oid> commits;
    git_oid oid;

    while( git_revwalk_next( &oid, walk ) == 0 )
        commits.push_back( oid );

    git_revwalk_free( walk );

    if( commits.empty() )
        return true;

    // Determine set of newest commits to keep based on blob sizes.
    std::set<git_oid, bool ( * )( const git_oid&, const git_oid& )> seenBlobs(
            []( const git_oid& a, const git_oid& b )
            {
                return memcmp( &a, &b, sizeof( git_oid ) ) < 0;
            } );

    size_t keptBytes = 0;
    std::vector<git_oid> keep;

    git_odb* odb = nullptr;
    git_repository_odb( &odb, repo );

    std::function<size_t( git_tree* )> accountTree = [&]( git_tree* tree )
    {
        size_t added = 0;
        size_t cnt = git_tree_entrycount( tree );

        for( size_t i = 0; i < cnt; ++i )
        {
            const git_tree_entry* entry = git_tree_entry_byindex( tree, i );

            if( git_tree_entry_type( entry ) == GIT_OBJECT_BLOB )
            {
                const git_oid* bid = git_tree_entry_id( entry );

                if( seenBlobs.find( *bid ) == seenBlobs.end() )
                {
                    size_t         len = 0;
                    git_object_t   type = GIT_OBJECT_ANY;

                    if( odb && git_odb_read_header( &len, &type, odb, bid ) == 0 )
                        added += len;

                    seenBlobs.insert( *bid );
                }
            }
            else if( git_tree_entry_type( entry ) == GIT_OBJECT_TREE )
            {
                git_tree* sub = nullptr;

                if( git_tree_lookup( &sub, repo, git_tree_entry_id( entry ) ) == 0 )
                {
                    added += accountTree( sub );
                    git_tree_free( sub );
                }
            }
        }

        return added;
    };

    for( const git_oid& cOid : commits )
    {
        git_commit* c = nullptr;

        if( git_commit_lookup( &c, repo, &cOid ) != 0 )
            continue;

        git_tree* tree = nullptr;
        git_commit_tree( &tree, c );
        size_t add = accountTree( tree );
        git_tree_free( tree );
        git_commit_free( c );

        if( keep.empty() || keptBytes + add <= aMaxBytes )
        {
            keep.push_back( cOid );
            keptBytes += add;
        }
        else
            break; // stop once limit exceeded
    }

    if( keep.empty() )
        keep.push_back( commits.front() );

    // Collect tags we want to preserve (Save_*/Last_Save_*). We'll recreate them if their
    // target commit is retained. Also ensure tagged commits are ALWAYS kept.
    std::vector<std::pair<wxString, git_oid>> tagTargets;
    std::set<git_oid, bool ( * )( const git_oid&, const git_oid& )> taggedCommits(
            []( const git_oid& a, const git_oid& b )
            {
                return memcmp( &a, &b, sizeof( git_oid ) ) < 0;
            } );
    git_strarray tagList;

    if( git_tag_list( &tagList, repo ) == 0 )
    {
        for( size_t i = 0; i < tagList.count; ++i )
        {
            wxString name = wxString::FromUTF8( tagList.strings[i] );
            if( name.StartsWith( wxS("Save_") ) || name.StartsWith( wxS("Last_Save_") ) )
            {
                git_reference* tref = nullptr;

                if( git_reference_lookup( &tref, repo, ( wxS( "refs/tags/" ) + name ).mb_str().data() ) == 0 )
                {
                    const git_oid* toid = git_reference_target( tref );

                    if( toid )
                    {
                        tagTargets.emplace_back( name, *toid );
                        taggedCommits.insert( *toid );

                        // Ensure this tagged commit is in the keep list
                        bool found = false;
                        for( const auto& k : keep )
                        {
                            if( memcmp( &k, toid, sizeof( git_oid ) ) == 0 )
                            {
                                found = true;
                                break;
                            }
                        }

                        if( !found )
                        {
                            // Add tagged commit to keep list (even if it exceeds size limit)
                            keep.push_back( *toid );
                            wxLogTrace( traceAutoSave, wxS( "[history] EnforceSizeLimit: Preserving tagged commit %s" ),
                                       name );
                        }
                    }

                    git_reference_free( tref );
                }
            }
        }
        git_strarray_free( &tagList );
    }

    // Rebuild trimmed repo in temp dir
    wxFileName trimFn( hist + wxS("_trim"), wxEmptyString );
    wxString trimPath = trimFn.GetPath();

    if( wxDirExists( trimPath ) )
        wxFileName::Rmdir( trimPath, wxPATH_RMDIR_RECURSIVE );

    wxMkdir( trimPath );
    git_repository* newRepo = nullptr;

    if( git_repository_init( &newRepo, trimPath.mb_str().data(), 0 ) != 0 )
    {
        git_odb_free( odb );
        return false;
    }

    git_odb* dstOdb = nullptr;

    if( git_repository_odb( &dstOdb, newRepo ) != 0 )
    {
        git_repository_free( newRepo );
        git_odb_free( odb );
        return false;
    }

    std::set<git_oid, bool ( * )( const git_oid&, const git_oid& )> copiedObjects(
            []( const git_oid& a, const git_oid& b )
            {
                return memcmp( &a, &b, sizeof( git_oid ) ) < 0;
            } );

    // Replay kept commits chronologically (oldest first) to preserve order.
    std::reverse( keep.begin(), keep.end() );
    git_commit* parent = nullptr;
    struct MAP_ENTRY { git_oid orig; git_oid neu; };
    std::vector<MAP_ENTRY> commitMap;

    if( aReporter )
    {
        aReporter->AdvancePhase( _( "Trimming local history..." ) );
        aReporter->SetCurrentProgress( 0 );
    }

    for( size_t idx = 0; idx < keep.size(); ++idx )
    {
        if( aReporter )
            aReporter->SetCurrentProgress( (double) idx / keep.size() );

        const git_oid& co = keep[idx];
        git_commit* orig = nullptr;

        if( git_commit_lookup( &orig, repo, &co ) != 0 )
            continue;

        git_tree* tree = nullptr;
        git_commit_tree( &tree, orig );

        copyTreeObjects( repo, odb, dstOdb, git_tree_id( tree ), copiedObjects );

        git_tree* newTree = nullptr;
        git_tree_lookup( &newTree, newRepo, git_tree_id( tree ) );

        git_tree_free( tree );

        // Recreate original author/committer signatures preserving timestamp.
        const git_signature* origAuthor = git_commit_author( orig );
        const git_signature* origCommitter = git_commit_committer( orig );
        git_signature*       sigAuthor = nullptr;
        git_signature*       sigCommitter = nullptr;

        git_signature_new( &sigAuthor, origAuthor->name, origAuthor->email,
                           origAuthor->when.time, origAuthor->when.offset );
        git_signature_new( &sigCommitter, origCommitter->name, origCommitter->email,
                           origCommitter->when.time, origCommitter->when.offset );

        const git_commit* parents[1];
        int               parentCount = 0;

        if( parent )
        {
            parents[0] = parent;
            parentCount = 1;
        }

        git_oid newCommitOid;
        git_commit_create( &newCommitOid, newRepo, "HEAD", sigAuthor, sigCommitter, nullptr, git_commit_message( orig ),
                           newTree, parentCount, parentCount ? parents : nullptr );

        if( parent )
            git_commit_free( parent );

        git_commit_lookup( &parent, newRepo, &newCommitOid );

        commitMap.emplace_back( co, newCommitOid );

        git_signature_free( sigAuthor );
        git_signature_free( sigCommitter );
        git_tree_free( newTree );
        git_commit_free( orig );
    }

    if( parent )
        git_commit_free( parent );

    // Recreate preserved tags pointing to new commit OIDs where possible.
    for( const auto& tt : tagTargets )
    {
        // Find mapping
        const git_oid* newOid = nullptr;

        for( const auto& m : commitMap )
        {
            if( memcmp( &m.orig, &tt.second, sizeof( git_oid ) ) == 0 )
            {
                newOid = &m.neu;
                break;
            }
        }

        if( !newOid )
            continue; // commit trimmed away

        git_object* obj = nullptr;

        if( git_object_lookup( &obj, newRepo, newOid, GIT_OBJECT_COMMIT ) == 0 )
        {
            git_oid tag_oid; git_tag_create_lightweight( &tag_oid, newRepo, tt.first.mb_str().data(), obj, 0 );
            git_object_free( obj );
        }
    }

    if( aReporter )
        aReporter->AdvancePhase( _( "Compacting trimmed history..." ) );

    compactRepository( newRepo, aReporter );

    // Free ODBs and close repos before swapping directories to avoid file locking issues.
    // Note: The lock manager will automatically free the original repo when it goes out of scope,
    // but we need to manually free the ODBs and new trimmed repo we created.
    git_odb_free( dstOdb );
    git_odb_free( odb );
    git_repository_free( newRepo );

    lock.ReleaseRepository();

    // Replace old history dir with trimmed one
    wxString backupOld = hist + wxS("_old");
    wxRenameFile( hist, backupOld );
    wxRenameFile( trimPath, hist );
    wxFileName::Rmdir( backupOld, wxPATH_RMDIR_RECURSIVE );
    return true;
}

wxString LOCAL_HISTORY::GetHeadHash( const wxString& aProjectPath )
{
    wxString hist = historyPath( aProjectPath );
    git_repository* repo = nullptr;

    if( git_repository_open( &repo, hist.mb_str().data() ) != 0 )
        return wxEmptyString;

    git_oid head_oid;
    if( git_reference_name_to_id( &head_oid, repo, "HEAD" ) != 0 )
    {
        git_repository_free( repo );
        return wxEmptyString;
    }

    wxString hash = wxString::FromUTF8( git_oid_tostr_s( &head_oid ) );
    git_repository_free( repo );
    return hash;
}


// Helper functions for RestoreCommit
namespace
{

/**
 * Check if any files in the project are locked (open in editors).
 */
bool checkForLockedFiles( const wxString& aProjectPath, std::vector<wxString>& aLockedFiles )
{
    std::function<void( const wxString& )> findLocks = [&]( const wxString& dirPath )
    {
        wxDir dir( dirPath );
        if( !dir.IsOpened() )
            return;

        wxString filename;
        bool cont = dir.GetFirst( &filename );

        while( cont )
        {
            wxFileName fullPath( dirPath, filename );

            // Skip special directories
            if( filename == wxS(".history") || filename == wxS(".git") )
            {
                cont = dir.GetNext( &filename );
                continue;
            }

            if( fullPath.DirExists() )
            {
                findLocks( fullPath.GetFullPath() );
            }
            else if( fullPath.FileExists()
                     && filename.StartsWith( FILEEXT::LockFilePrefix )
                     && filename.EndsWith( wxString( wxS( "." ) ) + FILEEXT::LockFileExtension ) )
            {
                // Reconstruct the original filename from the lock file name
                // Lock files are: ~<original>.<ext>.lck -> need to get <original>.<ext>
                wxString baseName = filename.Mid( FILEEXT::LockFilePrefix.length() );
                baseName = baseName.BeforeLast( '.' );  // Remove .lck
                wxFileName originalFile( dirPath, baseName );

                // Check if this is a valid LOCKFILE (not stale and not ours)
                LOCKFILE testLock( originalFile.GetFullPath() );
                if( testLock.Valid() && !testLock.IsLockedByMe() )
                {
                    aLockedFiles.push_back( fullPath.GetFullPath() );
                }
            }

            cont = dir.GetNext( &filename );
        }
    };

    findLocks( aProjectPath );
    return aLockedFiles.empty();
}


/**
 * Extract a git tree to a temporary directory.
 */
bool extractCommitToTemp( git_repository* aRepo, git_tree* aTree, const wxString& aTempPath )
{
    bool extractSuccess = true;

    std::function<void( git_tree*, const wxString& )> extractTree =
        [&]( git_tree* t, const wxString& prefix )
    {
        if( !extractSuccess )
            return;

        size_t cnt = git_tree_entrycount( t );
        for( size_t i = 0; i < cnt; ++i )
        {
            const git_tree_entry* entry = git_tree_entry_byindex( t, i );
            wxString name = wxString::FromUTF8( git_tree_entry_name( entry ) );
            wxString fullPath = prefix.IsEmpty() ? name : prefix + wxS("/") + name;

            if( git_tree_entry_type( entry ) == GIT_OBJECT_TREE )
            {
                wxFileName dirPath( aTempPath + wxFileName::GetPathSeparator() + fullPath,
                                   wxEmptyString );
                if( !wxFileName::Mkdir( dirPath.GetPath(), 0777, wxPATH_MKDIR_FULL ) )
                {
                    wxLogTrace( traceAutoSave,
                               wxS( "[history] extractCommitToTemp: Failed to create directory '%s'" ),
                               dirPath.GetPath() );
                    extractSuccess = false;
                    return;
                }

                git_tree* sub = nullptr;
                if( git_tree_lookup( &sub, aRepo, git_tree_entry_id( entry ) ) == 0 )
                {
                    extractTree( sub, fullPath );
                    git_tree_free( sub );
                }
            }
            else if( git_tree_entry_type( entry ) == GIT_OBJECT_BLOB )
            {
                git_blob* blob = nullptr;
                if( git_blob_lookup( &blob, aRepo, git_tree_entry_id( entry ) ) == 0 )
                {
                    wxFileName dst( aTempPath + wxFileName::GetPathSeparator() + fullPath );

                    wxFileName dstDir( dst );
                    dstDir.SetFullName( wxEmptyString );
                    wxFileName::Mkdir( dstDir.GetPath(), 0777, wxPATH_MKDIR_FULL );

                    wxFFile f( dst.GetFullPath(), wxT( "wb" ) );
                    if( f.IsOpened() )
                    {
                        f.Write( git_blob_rawcontent( blob ), git_blob_rawsize( blob ) );
                        f.Close();
                    }
                    else
                    {
                        wxLogTrace( traceAutoSave,
                                   wxS( "[history] extractCommitToTemp: Failed to write '%s'" ),
                                   dst.GetFullPath() );
                        extractSuccess = false;
                        git_blob_free( blob );
                        return;
                    }

                    git_blob_free( blob );
                }
            }
        }
    };

    extractTree( aTree, wxEmptyString );
    return extractSuccess;
}


/**
 * Recursively list every regular file under aDir, as paths relative to aRoot with
 * forward-slash separators. Walks real on-disk directories via wxDirExists rather than
 * wxFileName::IsDir (purely structural, never true for a real entry, so the previous
 * recursive collector silently skipped every subdirectory).
 */
void collectRelativeFiles( const wxString& aRoot, const wxString& aDir, std::vector<wxString>& aOut )
{
    wxDir dir( aDir );

    if( !dir.IsOpened() )
        return;

    wxString name;

    for( bool cont = dir.GetFirst( &name ); cont; cont = dir.GetNext( &name ) )
    {
        wxString full = aDir + wxFILE_SEP_PATH + name;

        if( wxDirExists( full ) )
        {
            collectRelativeFiles( aRoot, full, aOut );
        }
        else if( wxFileExists( full ) )
        {
            wxString rel = full.Mid( aRoot.length() + 1 );
            rel.Replace( wxS( "\\" ), wxS( "/" ) );
            aOut.push_back( rel );
        }
    }
}


/**
 * Overlay the extracted snapshot onto the working copy. Restore is additive, every file in the
 * snapshot is written over its working-copy counterpart and files absent from the snapshot are
 * left untouched, so a partial per-editor commit can never wipe the schematic, outputs, or
 * libraries. Overwritten originals are copied into aBackupPath first so the restore can be
 * undone, nothing is ever deleted. Files are listed up front so we never iterate a directory
 * while renaming entries out of it.
 */
bool overlaySnapshotFiles( const wxString& aTempRestorePath, const wxString& aProjectPath, const wxString& aBackupPath )
{
    std::vector<wxString> relPaths;
    collectRelativeFiles( aTempRestorePath, aTempRestorePath, relPaths );

    for( const wxString& rel : relPaths )
    {
        wxFileName src( aTempRestorePath + wxFILE_SEP_PATH + rel );
        wxFileName dst( aProjectPath + wxFILE_SEP_PATH + rel );

        if( dst.FileExists() )
        {
            wxFileName bak( aBackupPath + wxFILE_SEP_PATH + rel );

            if( !wxFileName::Mkdir( bak.GetPath(), 0777, wxPATH_MKDIR_FULL )
                || !wxCopyFile( dst.GetFullPath(), bak.GetFullPath(), true ) )
            {
                return false;
            }
        }

        if( !wxFileName::Mkdir( dst.GetPath(), 0777, wxPATH_MKDIR_FULL )
            || !wxCopyFile( src.GetFullPath(), dst.GetFullPath(), true ) )
        {
            return false;
        }
    }

    return true;
}


}  // namespace


bool LOCAL_HISTORY::RestoreCommit( const wxString& aProjectPath, const wxString& aHash, wxWindow* aParent,
                                   bool aConfirm )
{
    // STEP 1: Verify no files are open by checking for LOCKFILEs
    wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Checking for open files in %s" ),
               aProjectPath );

    std::vector<wxString> lockedFiles;
    if( !checkForLockedFiles( aProjectPath, lockedFiles ) )
    {
        wxString lockList;
        for( const auto& f : lockedFiles )
            lockList += wxS("\n  - ") + f;

        wxLogTrace( traceAutoSave,
                   wxS( "[history] RestoreCommit: Cannot restore - files are open:%s" ),
                   lockList );

        // Show user-visible warning dialog
        if( aParent )
        {
            wxString msg = _( "Cannot restore - the following files are open by another user:" );
            msg += lockList;
            wxMessageBox( msg, _( "Restore Failed" ), wxOK | wxICON_WARNING, aParent );
        }
        return false;
    }

    // STEP 2: Acquire history lock and verify target commit
    HISTORY_LOCK_MANAGER lock( aProjectPath );

    if( !lock.IsLocked() )
    {
        wxLogTrace( traceAutoSave,
                   wxS( "[history] RestoreCommit: Failed to acquire lock for %s" ),
                   aProjectPath );
        return false;
    }

    git_repository* repo = lock.GetRepository();
    if( !repo )
        return false;

    // Verify the target commit exists
    git_oid oid;
    if( git_oid_fromstr( &oid, aHash.mb_str().data() ) != 0 )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Invalid hash %s" ), aHash );
        return false;
    }

    git_commit* commit = nullptr;
    if( git_commit_lookup( &commit, repo, &oid ) != 0 )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Commit not found %s" ), aHash );
        return false;
    }

    git_tree* tree = nullptr;
    git_commit_tree( &tree, commit );

    // Confirm before overwriting working files. The recovery prompt already asked, so it passes
    // aConfirm = false. Nothing is changed yet, so cancel just returns.
    if( aConfirm && aParent )
    {
        wxDateTime when( (time_t) git_commit_time( commit ) );

        KICAD_MESSAGE_DIALOG dlg( aParent,
                                  wxString::Format( _( "Restore the project to the version from %s?" ),
                                                    when.Format( wxS( "%Y-%m-%d %H:%M:%S" ) ) ),
                                  _( "Restore Version" ), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );

        dlg.SetYesNoLabels( _( "Restore" ), _( "Cancel" ) );
        dlg.SetExtendedMessage( _( "Your current files are backed up first so you can undo the "
                                   "restore. Files that are not part of this version are left "
                                   "untouched." ) );

        if( dlg.ShowModal() != wxID_YES )
        {
            wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: User cancelled at confirm" ) );
            git_tree_free( tree );
            git_commit_free( commit );
            return false;
        }
    }

    // Create pre-restore backup snapshot using the existing lock
    wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Creating pre-restore backup" ) );

    std::vector<wxString> backupFiles;
    collectProjectFiles( aProjectPath, backupFiles );

    if( !backupFiles.empty() )
    {
        wxString hist = historyPath( aProjectPath );
        SNAPSHOT_COMMIT_RESULT backupResult = commitSnapshotWithLock( repo, lock.GetIndex(), hist, aProjectPath,
                                                                      backupFiles, wxS( "Pre-restore backup" ) );

        if( backupResult == SNAPSHOT_COMMIT_RESULT::Error )
        {
            wxLogTrace( traceAutoSave,
                       wxS( "[history] RestoreCommit: Failed to create pre-restore backup" ) );
            git_tree_free( tree );
            git_commit_free( commit );
            return false;
        }

        if( backupResult == SNAPSHOT_COMMIT_RESULT::NoChanges )
        {
            wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Current state already matches HEAD; "
                                            "continuing without a new backup commit" ) );
        }
    }

    // STEP 3: Extract commit to temporary location
    wxString tempRestorePath = aProjectPath + wxS("_restore_temp");

    if( wxDirExists( tempRestorePath ) )
        wxFileName::Rmdir( tempRestorePath, wxPATH_RMDIR_RECURSIVE );

    if( !wxFileName::Mkdir( tempRestorePath, 0777, wxPATH_MKDIR_FULL ) )
    {
        wxLogTrace( traceAutoSave,
                   wxS( "[history] RestoreCommit: Failed to create temp directory %s" ),
                   tempRestorePath );
        git_tree_free( tree );
        git_commit_free( commit );
        return false;
    }

    wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Extracting to temp location %s" ),
               tempRestorePath );

    if( !extractCommitToTemp( repo, tree, tempRestorePath ) )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Extraction failed, cleaning up" ) );
        wxFileName::Rmdir( tempRestorePath, wxPATH_RMDIR_RECURSIVE );
        git_tree_free( tree );
        git_commit_free( commit );
        return false;
    }

    // STEP 4: Overlay the snapshot onto the working copy. Restore never removes files that are
    // absent from the snapshot, so restoring a partial per-editor commit (for example the HEAD
    // autosave from a board-only session) cannot delete the schematic, project file, outputs, or
    // libraries. Overwritten files are archived to backupPath for manual recovery, and the
    // pre-restore commit created above is the full undo point.
    wxString backupPath =
            aProjectPath + wxS( "_restore_backup_" )
            + wxDateTime::UNow().Format( wxS( "%Y-%m-%dT%H-%M-%S-%l" ) );

    if( !overlaySnapshotFiles( tempRestorePath, aProjectPath, backupPath ) )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Overlay failed, rolling back from backup" ) );

        // Put back whatever we already overwrote, then drop the partial backups.
        if( wxDirExists( backupPath ) )
        {
            wxString discard = aProjectPath + wxS( "_restore_discard" );
            overlaySnapshotFiles( backupPath, aProjectPath, discard );
            wxFileName::Rmdir( discard, wxPATH_RMDIR_RECURSIVE );
            wxFileName::Rmdir( backupPath, wxPATH_RMDIR_RECURSIVE );
        }

        wxFileName::Rmdir( tempRestorePath, wxPATH_RMDIR_RECURSIVE );
        git_tree_free( tree );
        git_commit_free( commit );
        return false;
    }

    // The backup directory is retained so the user can recover any displaced file.
    wxLogTrace( traceAutoSave,
                wxS( "[history] RestoreCommit: Restore successful, backup retained at %s" ),
                backupPath );
    wxFileName::Rmdir( tempRestorePath, wxPATH_RMDIR_RECURSIVE );

    // Commit the full post-overlay project so HEAD and the saved baseline match the disk.
    std::vector<wxString> resultFiles;
    collectProjectFiles( aProjectPath, resultFiles );
    commitSnapshotWithLock( repo, lock.GetIndex(), historyPath( aProjectPath ), aProjectPath, resultFiles,
                            wxString::Format( wxS( "Restored from %s" ), aHash ) );

    // Anchor the saved baseline so reopening does not re-prompt.
    tagSaveAtHead( repo, wxS( "project" ) );

    git_tree_free( tree );
    git_commit_free( commit );

    wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Complete" ) );
    return true;
}

void LOCAL_HISTORY::ShowRestoreDialog( const wxString& aProjectPath, wxWindow* aParent )
{
    if( !HistoryExists( aProjectPath ) )
        return;

    std::vector<LOCAL_HISTORY_SNAPSHOT_INFO> snapshots = LoadSnapshots( aProjectPath );

    if( snapshots.empty() )
        return;

    DIALOG_RESTORE_LOCAL_HISTORY dlg( aParent, snapshots );

    if( dlg.ShowModal() == wxID_OK )
    {
        wxString selectedHash = dlg.GetSelectedHash();

        if( !selectedHash.IsEmpty() )
            RestoreCommit( aProjectPath, selectedHash, aParent );
    }
}

std::vector<LOCAL_HISTORY_SNAPSHOT_INFO> LOCAL_HISTORY::LoadSnapshots( const wxString& aProjectPath )
{
    std::vector<LOCAL_HISTORY_SNAPSHOT_INFO> snapshots;

    wxString hist = historyPath( aProjectPath );
    git_repository* repo = nullptr;

    if( git_repository_open( &repo, hist.mb_str().data() ) != 0 )
        return snapshots;

    git_revwalk* walk = nullptr;
    if( git_revwalk_new( &walk, repo ) != 0 )
    {
        git_repository_free( repo );
        return snapshots;
    }

    git_revwalk_sorting( walk, GIT_SORT_TIME );
    git_revwalk_push_head( walk );

    git_oid oid;

    while( git_revwalk_next( &oid, walk ) == 0 )
    {
        git_commit* commit = nullptr;

        if( git_commit_lookup( &commit, repo, &oid ) != 0 )
            continue;

        LOCAL_HISTORY_SNAPSHOT_INFO info;
        info.hash = wxString::FromUTF8( git_oid_tostr_s( &oid ) );
        info.date = wxDateTime( static_cast<time_t>( git_commit_time( commit ) ) );
        info.message = wxString::FromUTF8( git_commit_message( commit ) );

        wxString firstLine = info.message.BeforeFirst( '\n' );

        long     parsedCount = 0;
        wxString remainder;
        firstLine.BeforeFirst( ':', &remainder );
        remainder.Trim( true ).Trim( false );

        if( remainder.EndsWith( wxS( "files changed" ) ) )
        {
            wxString countText = remainder.BeforeFirst( ' ' );

            if( countText.ToLong( &parsedCount ) )
                info.filesChanged = static_cast<int>( parsedCount );
        }

        info.summary = firstLine.BeforeFirst( ':' );

        wxString rest;
        info.message.BeforeFirst( '\n', &rest );
        wxArrayString lines = wxSplit( rest, '\n', '\0' );

        for( const wxString& line : lines )
        {
            if( !line.IsEmpty() )
                info.changedFiles.Add( line );
        }

        snapshots.push_back( std::move( info ) );
        git_commit_free( commit );
    }

    git_revwalk_free( walk );
    git_repository_free( repo );
    return snapshots;
}


std::vector<LOCAL_HISTORY_SNAPSHOT_INFO> LOCAL_HISTORY::GetSnapshots( const wxString& aProjectPath )
{
    return LoadSnapshots( aProjectPath );
}


wxString LOCAL_HISTORY::TreeFingerprint( const wxString& aProjectPath, const wxString& aHash,
                                         const wxString& aExtension )
{
    wxString        hist = historyPath( aProjectPath );
    git_repository* repo = nullptr;

    if( git_repository_open( &repo, hist.mb_str().data() ) != 0 )
        return wxEmptyString;

    git_oid     oid;
    git_commit* commit = nullptr;
    git_tree*   tree = nullptr;

    if( git_oid_fromstr( &oid, aHash.mb_str().data() ) != 0 || git_commit_lookup( &commit, repo, &oid ) != 0 )
    {
        git_repository_free( repo );
        return wxEmptyString;
    }

    if( git_commit_tree( &tree, commit ) != 0 )
    {
        git_commit_free( commit );
        git_repository_free( repo );
        return wxEmptyString;
    }

    struct WALK_CTX
    {
        wxString              ext;
        std::vector<wxString> entries;
    } ctx{ aExtension, {} };

    auto collect = []( const char* aRoot, const git_tree_entry* aEntry, void* aPayload ) -> int
    {
        WALK_CTX* c = static_cast<WALK_CTX*>( aPayload );

        if( git_tree_entry_type( aEntry ) != GIT_OBJECT_BLOB )
            return 0;

        wxString name = wxString::FromUTF8( git_tree_entry_name( aEntry ) );

        if( !name.EndsWith( c->ext ) )
            return 0;

        wxString path = wxString::FromUTF8( aRoot ) + name;
        c->entries.push_back( path + wxS( ":" )
                              + wxString::FromUTF8( git_oid_tostr_s( git_tree_entry_id( aEntry ) ) ) );
        return 0;
    };

    git_tree_walk( tree, GIT_TREEWALK_PRE, collect, &ctx );

    git_tree_free( tree );
    git_commit_free( commit );
    git_repository_free( repo );

    std::sort( ctx.entries.begin(), ctx.entries.end() );

    wxString fingerprint;

    for( const wxString& entry : ctx.entries )
        fingerprint << entry << wxS( "|" );

    return fingerprint;
}


bool LOCAL_HISTORY::ExtractAllFilesAtCommit( const wxString& aProjectPath, const wxString& aHash,
                                             const wxString& aDestDir, const std::vector<wxString>& aExtensions )
{
    wxString        hist = historyPath( aProjectPath );
    git_repository* repo = nullptr;

    if( git_repository_open( &repo, hist.mb_str().data() ) != 0 )
        return false;

    git_oid     oid;
    git_commit* commit = nullptr;
    git_tree*   tree = nullptr;

    if( git_oid_fromstr( &oid, aHash.mb_str().data() ) != 0 || git_commit_lookup( &commit, repo, &oid ) != 0 )
    {
        git_repository_free( repo );
        return false;
    }

    if( git_commit_tree( &tree, commit ) != 0 )
    {
        git_commit_free( commit );
        git_repository_free( repo );
        return false;
    }

    struct WALK_CTX
    {
        git_repository*              repo;
        wxString                     destDir;
        const std::vector<wxString>* extensions;
        bool                         ok;
    } ctx{ repo, aDestDir, &aExtensions, true };

    // Non-capturing so it converts to the libgit2 C callback; state goes via payload.
    auto writeEntry = []( const char* aRoot, const git_tree_entry* aEntry, void* aPayload ) -> int
    {
        WALK_CTX* c = static_cast<WALK_CTX*>( aPayload );

        if( git_tree_entry_type( aEntry ) != GIT_OBJECT_BLOB )
            return 0;

        wxString name = wxString::FromUTF8( git_tree_entry_name( aEntry ) );

        if( !c->extensions->empty() )
        {
            bool match = false;

            for( const wxString& ext : *c->extensions )
            {
                if( name.EndsWith( ext ) )
                {
                    match = true;
                    break;
                }
            }

            if( !match )
                return 0;
        }

        wxString   rel = wxString::FromUTF8( aRoot ) + name;
        wxFileName outFn( c->destDir + wxS( "/" ) + rel );

        if( !wxFileName::Mkdir( outFn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            c->ok = false;
            return 0;
        }

        git_blob* blob = nullptr;

        if( git_blob_lookup( &blob, c->repo, git_tree_entry_id( aEntry ) ) == 0 )
        {
            const void*  data = git_blob_rawcontent( blob );
            const size_t size = static_cast<size_t>( git_blob_rawsize( blob ) );
            wxFFile      out( outFn.GetFullPath(), wxS( "wb" ) );

            if( !( data && out.IsOpened() && out.Write( data, size ) == size ) )
                c->ok = false;

            git_blob_free( blob );
        }
        else
        {
            c->ok = false;
        }

        return 0;
    };

    git_tree_walk( tree, GIT_TREEWALK_PRE, writeEntry, &ctx );

    git_tree_free( tree );
    git_commit_free( commit );
    git_repository_free( repo );
    return ctx.ok;
}

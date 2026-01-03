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

#include <local_history.h>
#include <history_lock.h>
#include <lockfile.h>
#include <settings/common_settings.h>
#include <pgm_base.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>

#include <git2.h>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/dir.h>
#include <wx/datetime.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/choicdlg.h>

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <set>
#include <map>
#include <functional>
#include <cstring>

static wxString historyPath( const wxString& aProjectPath )
{
    wxFileName p( aProjectPath, wxEmptyString );
    p.AppendDir( wxS( ".history" ) );
    return p.GetPath();
}

LOCAL_HISTORY::LOCAL_HISTORY()
{
}

LOCAL_HISTORY::~LOCAL_HISTORY()
{
}

void LOCAL_HISTORY::NoteFileChange( const wxString& aFile )
{
    wxFileName fn( aFile );

    if( fn.GetFullName() == wxS( "fp-info-cache" ) || !Pgm().GetCommonSettings()->m_Backup.enabled )
        return;

    m_pendingFiles.insert( fn.GetFullPath() );
}


void LOCAL_HISTORY::RegisterSaver( const void* aSaverObject,
                                   const std::function<void( const wxString&, std::vector<wxString>& )>& aSaver )
{
    if( m_savers.find( aSaverObject ) != m_savers.end() )
    {
        wxLogTrace( traceAutoSave, wxS("[history] Saver %p already registered, skipping"), aSaverObject );
        return;
    }

    m_savers[aSaverObject] = aSaver;
    wxLogTrace( traceAutoSave, wxS("[history] Registered saver %p (total=%zu)"), aSaverObject, m_savers.size() );
}


void LOCAL_HISTORY::UnregisterSaver( const void* aSaverObject )
{
    auto it = m_savers.find( aSaverObject );

    if( it != m_savers.end() )
    {
        m_savers.erase( it );
        wxLogTrace( traceAutoSave, wxS("[history] Unregistered saver %p (total=%zu)"), aSaverObject, m_savers.size() );
    }
}


void LOCAL_HISTORY::ClearAllSavers()
{
    m_savers.clear();
    wxLogTrace( traceAutoSave, wxS("[history] Cleared all savers") );
}


bool LOCAL_HISTORY::RunRegisteredSaversAndCommit( const wxString& aProjectPath, const wxString& aTitle )
{
    if( !Pgm().GetCommonSettings()->m_Backup.enabled )
    {
        wxLogTrace( traceAutoSave, wxS("Autosave disabled, returning" ) );
        return true;
    }

    wxLogTrace( traceAutoSave, wxS("[history] RunRegisteredSaversAndCommit start project='%s' title='%s' savers=%zu"),
                aProjectPath, aTitle, m_savers.size() );

    if( m_savers.empty() )
    {
        wxLogTrace( traceAutoSave, wxS("[history] no savers registered; skipping") );
        return false;
    }

    std::vector<wxString> files;

    for( const auto& [saverObject, saver] : m_savers )
    {
        size_t before = files.size();
        saver( aProjectPath, files );
        wxLogTrace( traceAutoSave, wxS("[history] saver %p added %zu files (total=%zu)"),
                    saverObject, files.size() - before, files.size() );
    }

    // Filter out any files not within the project directory
    wxString projectDir = aProjectPath;
    if( !projectDir.EndsWith( wxFileName::GetPathSeparator() ) )
        projectDir += wxFileName::GetPathSeparator();

    auto it = std::remove_if( files.begin(), files.end(),
        [&projectDir]( const wxString& file )
        {
            if( !file.StartsWith( projectDir ) )
            {
                wxLogTrace( traceAutoSave, wxS("[history] filtered out file outside project: %s"), file );
                return true;
            }
            return false;
        } );
    files.erase( it, files.end() );

    if( files.empty() )
    {
        wxLogTrace( traceAutoSave, wxS("[history] saver set produced no files; skipping") );
        return false;
    }

    // Acquire locks using hybrid locking strategy
    HISTORY_LOCK_MANAGER lock( aProjectPath );

    if( !lock.IsLocked() )
    {
        wxLogTrace( traceAutoSave, wxS("[history] failed to acquire lock: %s"), lock.GetLockError() );
        return false;
    }

    git_repository* repo = lock.GetRepository();
    git_index* index = lock.GetIndex();
    wxString hist = historyPath( aProjectPath );

    // Stage selected files (mirroring logic from CommitSnapshot but limited to given files)
    for( const wxString& file : files )
    {
        wxFileName src( file );

        if( !src.FileExists() )
        {
            wxLogTrace( traceAutoSave, wxS("[history] skip missing '%s'"), file );
            continue;
        }

        // If saver already produced a path inside history mirror, just stage it.
        if( src.GetFullPath().StartsWith( hist + wxFILE_SEP_PATH ) )
        {
            std::string relHist = src.GetFullPath().ToStdString().substr( hist.length() + 1 );
            git_index_add_bypath( index, relHist.c_str() );
            wxLogTrace( traceAutoSave, wxS("[history] staged pre-mirrored '%s'"), file );
            continue;
        }

        wxString relStr;
        wxString proj = wxFileName( aProjectPath ).GetFullPath();

        if( src.GetFullPath().StartsWith( proj + wxFILE_SEP_PATH ) )
            relStr = src.GetFullPath().Mid( proj.length() + 1 );
        else
            relStr = src.GetFullName();

        wxFileName dst( hist + wxFILE_SEP_PATH + relStr );
        wxFileName dstDir( dst );
        dstDir.SetFullName( wxEmptyString );
        wxFileName::Mkdir( dstDir.GetPath(), 0777, wxPATH_MKDIR_FULL );
        wxCopyFile( src.GetFullPath(), dst.GetFullPath(), true );
        std::string rel = dst.GetFullPath().ToStdString().substr( hist.length() + 1 );
        git_index_add_bypath( index, rel.c_str() );
        wxLogTrace( traceAutoSave, wxS("[history] staged '%s' as '%s'"), file, wxString::FromUTF8( rel ) );
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
        if( head_tree ) git_tree_free( head_tree );
        if( head_commit ) git_commit_free( head_commit );
        wxLogTrace( traceAutoSave, wxS("[history] failed to write index tree" ) );
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
            wxLogTrace( traceAutoSave, wxS("[history] diff deltas=%u"), (unsigned) git_diff_num_deltas( diff ) );
            git_diff_free( diff );
        }
    }

    if( head_tree ) git_tree_free( head_tree );
    if( head_commit ) git_commit_free( head_commit );

    if( !hasChanges )
    {
        wxLogTrace( traceAutoSave, wxS("[history] no changes detected; no commit") );
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
        wxLogTrace( traceAutoSave, wxS("[history] commit created %s (%s files=%zu)"),
                    wxString::FromUTF8( git_oid_tostr_s( &commit_id ) ), msg, files.size() );
    else
        wxLogTrace( traceAutoSave, wxS("[history] commit failed rc=%d"), rc );

    if( parent ) git_commit_free( parent );

    git_index_write( index );
    return rc == 0;
}


bool LOCAL_HISTORY::CommitPending()
{
    std::vector<wxString> files( m_pendingFiles.begin(), m_pendingFiles.end() );
    m_pendingFiles.clear();
    return CommitSnapshot( files, wxS( "Autosave" ) );
}


bool LOCAL_HISTORY::Init( const wxString& aProjectPath )
{
    if( aProjectPath.IsEmpty() )
        return false;

    if( !Pgm().GetCommonSettings()->m_Backup.enabled )
        return true;

    wxString hist = historyPath( aProjectPath );

    if( !wxDirExists( hist ) )
    {
        if( wxIsWritable( aProjectPath ) )
        {
            if( !wxMkdir( hist ) )
            {
                return false;
            }
        }
    }

    git_repository* rawRepo = nullptr;
    bool isNewRepo = false;

    if( git_repository_open( &rawRepo, hist.mb_str().data() ) != 0 )
    {
        if( git_repository_init( &rawRepo, hist.mb_str().data(), 0 ) != 0 )
            return false;

        isNewRepo = true;

        wxFileName ignoreFile( hist, wxS( ".gitignore" ) );
        if( !ignoreFile.FileExists() )
        {
            wxFFile f( ignoreFile.GetFullPath(), wxT( "w" ) );
            if( f.IsOpened() )
            {
                f.Write( wxS( "fp-info-cache\n*-backups\nREADME.txt\n" ) );
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

    // If this is a newly initialized repository with no commits, create an initial snapshot
    // of all existing project files
    if( isNewRepo )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] Init: New repository created, collecting existing files" ) );

        // Collect all files in the project directory (excluding backups and hidden files)
        wxArrayString files;
        std::function<void( const wxString& )> collect = [&]( const wxString& path )
        {
            wxString name;
            wxDir d( path );

            if( !d.IsOpened() )
                return;

            bool cont = d.GetFirst( &name );

            while( cont )
            {
                // Skip hidden files/directories and backup directories
                if( name.StartsWith( wxS( "." ) ) || name.EndsWith( wxS( "-backups" ) ) )
                {
                    cont = d.GetNext( &name );
                    continue;
                }

                wxFileName fn( path, name );

                if( fn.DirExists() )
                {
                    collect( fn.GetFullPath() );
                }
                else if( fn.FileExists() )
                {
                    // Skip transient files
                    if( fn.GetFullName() != wxS( "fp-info-cache" ) )
                        files.Add( fn.GetFullPath() );
                }

                cont = d.GetNext( &name );
            }
        };

        collect( aProjectPath );

        if( files.GetCount() > 0 )
        {
            std::vector<wxString> vec;
            vec.reserve( files.GetCount() );

            for( unsigned i = 0; i < files.GetCount(); ++i )
                vec.push_back( files[i] );

            wxLogTrace( traceAutoSave, wxS( "[history] Init: Creating initial snapshot with %zu files" ), vec.size() );
            CommitSnapshot( vec, wxS( "Initial snapshot" ) );
        }
        else
        {
            wxLogTrace( traceAutoSave, wxS( "[history] Init: No files found to add to initial snapshot" ) );
        }
    }

    return true;
}


// Helper function to commit files using an already-acquired lock
static bool commitSnapshotWithLock( git_repository* repo, git_index* index,
                                    const wxString& aHistoryPath, const wxString& aProjectPath,
                                    const std::vector<wxString>& aFiles, const wxString& aTitle )
{
    for( const wxString& file : aFiles )
    {
        wxFileName src( file );
        wxString relStr;

        if( src.GetFullPath().StartsWith( aProjectPath + wxFILE_SEP_PATH ) )
            relStr = src.GetFullPath().Mid( aProjectPath.length() + 1 );
        else
            relStr = src.GetFullName(); // Fallback (should not normally happen)

        wxFileName dst( aHistoryPath + wxFILE_SEP_PATH + relStr );

        // Ensure directory tree exists.
        wxFileName dstDir( dst );
        dstDir.SetFullName( wxEmptyString );
        wxFileName::Mkdir( dstDir.GetPath(), 0777, wxPATH_MKDIR_FULL );

        // Copy file into history mirror.
        wxCopyFile( src.GetFullPath(), dst.GetFullPath(), true );

        // Path inside repo (strip hist + '/').
        std::string rel = dst.GetFullPath().ToStdString().substr( aHistoryPath.length() + 1 );
        git_index_add_bypath( index, rel.c_str() );
    }

    git_oid tree_id;
    if( git_index_write_tree( &tree_id, index ) != 0 )
        return false;

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

    wxString msg;

    if( !aTitle.IsEmpty() )
        msg << aTitle << wxS( ": " );

    msg << aFiles.size() << wxS( " files changed" );

    for( size_t i = 0; i < git_diff_num_deltas( diff.get() ); ++i )
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
    git_commit_create( &commit_id, repo, "HEAD", sig.get(), sig.get(), nullptr,
                       msg.mb_str().data(), tree.get(), parents,
                       parentPtr ? &constParentPtr : nullptr );
    git_index_write( index );
    return true;
}


bool LOCAL_HISTORY::CommitSnapshot( const std::vector<wxString>& aFiles, const wxString& aTitle )
{
    if( aFiles.empty() || !Pgm().GetCommonSettings()->m_Backup.enabled )
        return true;

    wxString        proj = wxFileName( aFiles[0] ).GetPath();
    wxString        hist = historyPath( proj );

    // Acquire locks using hybrid locking strategy
    HISTORY_LOCK_MANAGER lock( proj );

    if( !lock.IsLocked() )
    {
        wxLogTrace( traceAutoSave, wxS("[history] CommitSnapshot failed to acquire lock: %s"),
                   lock.GetLockError() );
        return false;
    }

    git_repository* repo = lock.GetRepository();
    git_index* index = lock.GetIndex();

    return commitSnapshotWithLock( repo, index, hist, proj, aFiles, aTitle );
}


// Helper to collect all project files (excluding .history, backups, and transient files)
static void collectProjectFiles( const wxString& aProjectPath, std::vector<wxString>& aFiles )
{
    wxDir dir( aProjectPath );

    if( !dir.IsOpened() )
        return;

    // Collect recursively.
    std::function<void(const wxString&)> collect = [&]( const wxString& path )
    {
        wxString name;
        wxDir d( path );

        if( !d.IsOpened() )
            return;

        bool cont = d.GetFirst( &name );

        while( cont )
        {
            if( name == wxS( ".history" ) || name.EndsWith( wxS( "-backups" ) ) )
            {
                cont = d.GetNext( &name );
                continue; // Skip history repo itself.
            }

            wxFileName fn( path, name );

            if( fn.IsDir() && fn.DirExists() )
            {
                collect( fn.GetFullPath() );
            }
            else if( fn.FileExists() )
            {
                // Reuse NoteFileChange filters implicitly by skipping the transient names.
                if( fn.GetFullName() != wxS( "fp-info-cache" ) )
                    aFiles.push_back( fn.GetFullPath() );
            }

            cont = d.GetNext( &name );
        }
    };

    collect( aProjectPath );
}


bool LOCAL_HISTORY::CommitFullProjectSnapshot( const wxString& aProjectPath, const wxString& aTitle )
{
    std::vector<wxString> files;
    collectProjectFiles( aProjectPath, files );

    if( files.empty() )
        return false;

    return CommitSnapshot( files, aTitle );
}

bool LOCAL_HISTORY::HistoryExists( const wxString& aProjectPath )
{
    return wxDirExists( historyPath( aProjectPath ) );
}

bool LOCAL_HISTORY::TagSave( const wxString& aProjectPath, const wxString& aFileType )
{
    HISTORY_LOCK_MANAGER lock( aProjectPath );

    if( !lock.IsLocked() )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] TagSave: Failed to acquire lock for %s" ), aProjectPath );
        return false;
    }

    git_repository* repo = lock.GetRepository();

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
        if( fn.DirExists() )
            total += dirSizeRecursive( fn.GetFullPath() );
        else if( fn.FileExists() )
            total += (size_t) fn.GetSize().GetValue();
        cont = dir.GetNext( &name );
    }
    return total;
}

bool LOCAL_HISTORY::EnforceSizeLimit( const wxString& aProjectPath, size_t aMaxBytes )
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
                    git_blob* blob = nullptr;

                    if( git_blob_lookup( &blob, repo, bid ) == 0 )
                    {
                        added += git_blob_rawsize( blob );
                        git_blob_free( blob );
                    }

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
        keep.push_back( commits.front() ); // ensure at least head

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
        return false;

    // We will materialize each kept commit chronologically (oldest first) to preserve order.
    std::reverse( keep.begin(), keep.end() );
    git_commit* parent = nullptr;
    struct MAP_ENTRY { git_oid orig; git_oid neu; };
    std::vector<MAP_ENTRY> commitMap;

    for( const git_oid& co : keep )
    {
        git_commit* orig = nullptr;

        if( git_commit_lookup( &orig, repo, &co ) != 0 )
            continue;

        git_tree* tree = nullptr;
        git_commit_tree( &tree, orig );

        // Checkout tree into filesystem (simple extractor)
        // Remove all files first (except .git).
        wxArrayString toDelete;
        wxDir d( trimPath );
        wxString nm;
        bool cont = d.GetFirst( &nm );
        while( cont )
        {
            if( nm != wxS(".git") )
                toDelete.Add( nm );
            cont = d.GetNext( &nm );
        }

        for( auto& del : toDelete )
        {
            wxFileName f( trimPath, del );
            if( f.DirExists() )
                wxFileName::Rmdir( f.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
            else if( f.FileExists() )
                wxRemoveFile( f.GetFullPath() );
        }

        // Recursively write tree entries
        std::function<void(git_tree*, const wxString&)> writeTree = [&]( git_tree* t, const wxString& base )
        {
            size_t ecnt = git_tree_entrycount( t );
            for( size_t i = 0; i < ecnt; ++i )
            {
                const git_tree_entry* e = git_tree_entry_byindex( t, i );
                wxString name = wxString::FromUTF8( git_tree_entry_name( e ) );

                if( git_tree_entry_type( e ) == GIT_OBJECT_TREE )
                {
                    wxFileName dir( base, name );
                    wxMkdir( dir.GetFullPath() );
                    git_tree* sub = nullptr;

                    if( git_tree_lookup( &sub, repo, git_tree_entry_id( e ) ) == 0 )
                    {
                        writeTree( sub, dir.GetFullPath() );
                        git_tree_free( sub );
                    }
                }
                else if( git_tree_entry_type( e ) == GIT_OBJECT_BLOB )
                {
                    git_blob* blob = nullptr;

                    if( git_blob_lookup( &blob, repo, git_tree_entry_id( e ) ) == 0 )
                    {
                        wxFileName file( base, name );
                        wxFFile f( file.GetFullPath(), wxT("wb") );

                        if( f.IsOpened() )
                        {
                            f.Write( (const char*) git_blob_rawcontent( blob ), git_blob_rawsize( blob ) );
                            f.Close();
                        }
                        git_blob_free( blob );
                    }
                }
            }
        };

        writeTree( tree, trimPath );

        git_index* newIndex = nullptr;
        git_repository_index( &newIndex, newRepo );
        git_index_add_all( newIndex, nullptr, 0, nullptr, nullptr );
        git_index_write( newIndex );
        git_oid newTreeOid;
        git_index_write_tree( &newTreeOid, newIndex );
        git_tree* newTree = nullptr;
        git_tree_lookup( &newTree, newRepo, &newTreeOid );

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

        commitMap.emplace_back(  co, newCommitOid  );

        git_signature_free( sigAuthor );
        git_signature_free( sigCommitter );
        git_tree_free( newTree );
        git_index_free( newIndex );
        git_tree_free( tree );
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

    // Close both repositories before swapping directories to avoid file locking issues.
    // Note: The lock manager will automatically free the original repo when it goes out of scope,
    // but we need to manually free the new trimmed repo we created.
    git_repository_free( newRepo );

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
 * Collect all files in a directory into a set (recursively).
 */
void collectFilesInDirectory( const wxString& aRootPath, const wxString& aSearchPath,
                              std::set<wxString>& aFiles )
{
    wxDir dir( aSearchPath );
    if( !dir.IsOpened() )
        return;

    wxString filename;
    bool cont = dir.GetFirst( &filename );

    while( cont )
    {
        wxFileName fullPath( aSearchPath, filename );
        wxString relativePath = fullPath.GetFullPath().Mid( aRootPath.Length() + 1 );

        if( fullPath.IsDir() && fullPath.DirExists() )
        {
            collectFilesInDirectory( aRootPath, fullPath.GetFullPath(), aFiles );
        }
        else if( fullPath.FileExists() )
        {
            aFiles.insert( relativePath );
        }

        cont = dir.GetNext( &filename );
    }
}


/**
 * Check if a file should be excluded from backup (and thus not deleted during restore).
 */
bool shouldExcludeFromBackup( const wxString& aFilename )
{
    // Files explicitly excluded from backup should not be deleted during restore
    return aFilename == wxS( "fp-info-cache" );
}


/**
 * Find files in current project that won't exist in the restored version.
 */
void findFilesToDelete( const wxString& aProjectPath, const std::set<wxString>& aRestoredFiles,
                       std::vector<wxString>& aFilesToDelete )
{
    std::function<void( const wxString&, const wxString& )> scanDirectory =
        [&]( const wxString& dirPath, const wxString& relativeBase )
    {
        wxDir dir( dirPath );
        if( !dir.IsOpened() )
            return;

        wxString filename;
        bool cont = dir.GetFirst( &filename );

        while( cont )
        {
            // Skip special directories
            if( filename == wxS(".history") || filename == wxS(".git") ||
                filename == wxS("_restore_backup") || filename == wxS("_restore_temp") )
            {
                cont = dir.GetNext( &filename );
                continue;
            }

            wxFileName fullPath( dirPath, filename );
            wxString relativePath = relativeBase.IsEmpty() ? filename :
                                   relativeBase + wxS("/") + filename;

            if( fullPath.IsDir() && fullPath.DirExists() )
            {
                scanDirectory( fullPath.GetFullPath(), relativePath );
            }
            else if( fullPath.FileExists() )
            {
                // Check if this file exists in the restored commit
                if( aRestoredFiles.find( relativePath ) == aRestoredFiles.end() )
                {
                    // Don't propose deletion of files that were never in backup scope
                    if( !shouldExcludeFromBackup( filename ) )
                        aFilesToDelete.push_back( relativePath );
                }
            }

            cont = dir.GetNext( &filename );
        }
    };

    scanDirectory( aProjectPath, wxEmptyString );
}


/**
 * Show confirmation dialog for files that will be deleted.
 * Returns true to proceed, false to abort. Sets aKeepAllFiles based on user choice.
 */
bool confirmFileDeletion( wxWindow* aParent, const std::vector<wxString>& aFilesToDelete,
                         bool& aKeepAllFiles )
{
    if( aFilesToDelete.empty() || !aParent )
    {
        aKeepAllFiles = false;
        return true;
    }

    wxString message = _( "The following files will be deleted when restoring this commit:\n\n" );

    // Limit display to first 20 files to avoid overwhelming dialog
    size_t displayCount = std::min( aFilesToDelete.size(), size_t(20) );
    for( size_t i = 0; i < displayCount; ++i )
    {
        message += wxS("  • ") + aFilesToDelete[i] + wxS("\n");
    }

    if( aFilesToDelete.size() > displayCount )
    {
        message += wxString::Format( _( "\n... and %zu more files\n" ),
                                     aFilesToDelete.size() - displayCount );
    }

    wxMessageDialog dlg( aParent, message, _( "Delete Files during Restore" ),
                        wxYES_NO | wxCANCEL | wxICON_QUESTION );
    dlg.SetYesNoCancelLabels( _( "Proceed" ), _( "Keep All Files" ), _( "Abort" ) );
    dlg.SetExtendedMessage(
        _( "Choosing 'Keep All Files' will restore the selected commit but retain any existing "
           "files in the project directory. Choosing 'Proceed' will delete files that are not "
           "present in the restored commit." ) );

    int choice = dlg.ShowModal();

    if( choice == wxID_CANCEL )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] User cancelled restore" ) );
        return false;
    }
    else if( choice == wxID_NO )  // Keep All Files
    {
        wxLogTrace( traceAutoSave, wxS( "[history] User chose to keep all files" ) );
        aKeepAllFiles = true;
    }
    else  // Proceed with deletion
    {
        wxLogTrace( traceAutoSave, wxS( "[history] User chose to proceed with deletion" ) );
        aKeepAllFiles = false;
    }

    return true;
}


/**
 * Backup current project files before restore.
 */
bool backupCurrentFiles( const wxString& aProjectPath, const wxString& aBackupPath,
                        const wxString& aTempRestorePath, bool aKeepAllFiles,
                        std::set<wxString>& aBackedUpFiles )
{
    wxDir currentDir( aProjectPath );
    if( !currentDir.IsOpened() )
        return false;

    wxString filename;
    bool cont = currentDir.GetFirst( &filename );

    while( cont )
    {
        if( filename != wxS( ".history" ) && filename != wxS( ".git" ) &&
            filename != wxS( "_restore_backup" ) && filename != wxS( "_restore_temp" ) )
        {
            // If keepAllFiles is true, only backup files that will be overwritten
            bool shouldBackup = !aKeepAllFiles;

            if( aKeepAllFiles )
            {
                // Check if this file exists in the restored commit
                wxFileName testPath( aTempRestorePath, filename );
                shouldBackup = testPath.Exists();
            }

            if( shouldBackup )
            {
                wxFileName source( aProjectPath, filename );
                wxFileName dest( aBackupPath, filename );

                // Create backup directory if needed
                if( !wxDirExists( aBackupPath ) )
                {
                    wxLogTrace( traceAutoSave,
                               wxS( "[history] backupCurrentFiles: Creating backup directory %s" ),
                               aBackupPath );
                    wxFileName::Mkdir( aBackupPath, 0777, wxPATH_MKDIR_FULL );
                }

                wxLogTrace( traceAutoSave,
                           wxS( "[history] backupCurrentFiles: Backing up '%s' to '%s'" ),
                           source.GetFullPath(), dest.GetFullPath() );

                if( !wxRenameFile( source.GetFullPath(), dest.GetFullPath() ) )
                {
                    wxLogTrace( traceAutoSave,
                               wxS( "[history] backupCurrentFiles: Failed to backup '%s'" ),
                               source.GetFullPath() );
                    return false;
                }

                aBackedUpFiles.insert( filename );
            }
        }
        cont = currentDir.GetNext( &filename );
    }

    return true;
}


/**
 * Restore files from temporary location to project directory.
 */
bool restoreFilesFromTemp( const wxString& aTempRestorePath, const wxString& aProjectPath,
                           std::set<wxString>& aRestoredFiles )
{
    wxDir tempDir( aTempRestorePath );
    if( !tempDir.IsOpened() )
        return false;

    wxString filename;
    bool cont = tempDir.GetFirst( &filename );

    while( cont )
    {
        wxFileName source( aTempRestorePath, filename );
        wxFileName dest( aProjectPath, filename );

        wxLogTrace( traceAutoSave,
                   wxS( "[history] restoreFilesFromTemp: Restoring '%s' to '%s'" ),
                   source.GetFullPath(), dest.GetFullPath() );

        if( !wxRenameFile( source.GetFullPath(), dest.GetFullPath() ) )
        {
            wxLogTrace( traceAutoSave,
                       wxS( "[history] restoreFilesFromTemp: Failed to move '%s'" ),
                       source.GetFullPath() );
            return false;
        }

        aRestoredFiles.insert( filename );
        cont = tempDir.GetNext( &filename );
    }

    return true;
}


/**
 * Rollback a failed restore operation.
 */
void rollbackRestore( const wxString& aProjectPath, const wxString& aBackupPath,
                     const wxString& aTempRestorePath, const std::set<wxString>& aBackedUpFiles,
                     const std::set<wxString>& aRestoredFiles )
{
    wxLogTrace( traceAutoSave, wxS( "[history] rollbackRestore: Rolling back due to failure" ) );

    // Remove ONLY the files we successfully moved from temp directory
    // This preserves any files that were NOT in the backup (never tracked in history)
    for( const wxString& filename : aRestoredFiles )
    {
        wxFileName toRemove( aProjectPath, filename );
        wxLogTrace( traceAutoSave, wxS( "[history] rollbackRestore: Removing '%s'" ),
                   toRemove.GetFullPath() );

        if( toRemove.DirExists() )
        {
            wxFileName::Rmdir( toRemove.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
        }
        else if( toRemove.FileExists() )
        {
            wxRemoveFile( toRemove.GetFullPath() );
        }
    }

    // Restore from backup - put back only what we moved
    if( wxDirExists( aBackupPath ) )
    {
        for( const wxString& filename : aBackedUpFiles )
        {
            wxFileName source( aBackupPath, filename );
            wxFileName dest( aProjectPath, filename );

            if( source.Exists() )
            {
                wxRenameFile( source.GetFullPath(), dest.GetFullPath() );
                wxLogTrace( traceAutoSave, wxS( "[history] rollbackRestore: Restored '%s'" ),
                           dest.GetFullPath() );
            }
        }
    }

    // Clean up temporary directories
    wxFileName::Rmdir( aTempRestorePath, wxPATH_RMDIR_RECURSIVE );
    wxFileName::Rmdir( aBackupPath, wxPATH_RMDIR_RECURSIVE );
}


/**
 * Record the restore operation in git history.
 */
bool recordRestoreInHistory( git_repository* aRepo, git_commit* aCommit, git_tree* aTree,
                            const wxString& aHash )
{
    git_time_t t = git_commit_time( aCommit );
    wxDateTime dt( (time_t) t );
    git_signature* sig = nullptr;
    git_signature_now( &sig, "KiCad", "noreply@kicad.org" );
    git_commit* parent = nullptr;
    git_oid parent_id;

    if( git_reference_name_to_id( &parent_id, aRepo, "HEAD" ) == 0 )
        git_commit_lookup( &parent, aRepo, &parent_id );

    wxString msg;
    msg.Printf( wxS( "Restored from %s %s" ), aHash, dt.FormatISOCombined().c_str() );

    git_oid new_id;
    const git_commit* constParent = parent;
    int result = git_commit_create( &new_id, aRepo, "HEAD", sig, sig, nullptr,
                                    msg.mb_str().data(), aTree, parent ? 1 : 0,
                                    parent ? &constParent : nullptr );

    if( parent )
        git_commit_free( parent );
    git_signature_free( sig );

    return result == 0;
}

}  // namespace


bool LOCAL_HISTORY::RestoreCommit( const wxString& aProjectPath, const wxString& aHash,
                                   wxWindow* aParent )
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

    // Create pre-restore backup snapshot using the existing lock
    wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Creating pre-restore backup" ) );

    std::vector<wxString> backupFiles;
    collectProjectFiles( aProjectPath, backupFiles );

    if( !backupFiles.empty() )
    {
        wxString hist = historyPath( aProjectPath );
        if( !commitSnapshotWithLock( repo, lock.GetIndex(), hist, aProjectPath, backupFiles,
                                     wxS( "Pre-restore backup" ) ) )
        {
            wxLogTrace( traceAutoSave,
                       wxS( "[history] RestoreCommit: Failed to create pre-restore backup" ) );
            git_tree_free( tree );
            git_commit_free( commit );
            return false;
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

    // STEP 4: Determine which files will be deleted and ask for confirmation
    std::set<wxString> restoredFiles;
    collectFilesInDirectory( tempRestorePath, tempRestorePath, restoredFiles );

    std::vector<wxString> filesToDelete;
    findFilesToDelete( aProjectPath, restoredFiles, filesToDelete );

    bool keepAllFiles = true;
    if( !confirmFileDeletion( aParent, filesToDelete, keepAllFiles ) )
    {
        // User cancelled
        wxFileName::Rmdir( tempRestorePath, wxPATH_RMDIR_RECURSIVE );
        git_tree_free( tree );
        git_commit_free( commit );
        return false;
    }

    // STEP 5: Perform atomic swap - backup current, move temp to current
    wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Performing atomic swap" ) );

    wxString backupPath = aProjectPath + wxS("_restore_backup");

    // Remove old backup if exists
    if( wxDirExists( backupPath ) )
    {
        wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Removing old backup %s" ),
                   backupPath );
        wxFileName::Rmdir( backupPath, wxPATH_RMDIR_RECURSIVE );
    }

    // Track which files we moved to backup and restored (for rollback)
    std::set<wxString> backedUpFiles;
    std::set<wxString> restoredFilesSet;

    // Backup current files
    if( !backupCurrentFiles( aProjectPath, backupPath, tempRestorePath, keepAllFiles,
                            backedUpFiles ) )
    {
        rollbackRestore( aProjectPath, backupPath, tempRestorePath, backedUpFiles,
                        restoredFilesSet );
        git_tree_free( tree );
        git_commit_free( commit );
        return false;
    }

    // Restore files from temp
    if( !restoreFilesFromTemp( tempRestorePath, aProjectPath, restoredFilesSet ) )
    {
        rollbackRestore( aProjectPath, backupPath, tempRestorePath, backedUpFiles,
                        restoredFilesSet );
        git_tree_free( tree );
        git_commit_free( commit );
        return false;
    }

    // SUCCESS - Clean up temp and backup directories
    wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Restore successful, cleaning up" ) );
    wxFileName::Rmdir( tempRestorePath, wxPATH_RMDIR_RECURSIVE );
    wxFileName::Rmdir( backupPath, wxPATH_RMDIR_RECURSIVE );

    // Record the restore in history
    recordRestoreInHistory( repo, commit, tree, aHash );

    git_tree_free( tree );
    git_commit_free( commit );

    wxLogTrace( traceAutoSave, wxS( "[history] RestoreCommit: Complete" ) );
    return true;
}

void LOCAL_HISTORY::ShowRestoreDialog( const wxString& aProjectPath, wxWindow* aParent )
{
    if( !HistoryExists( aProjectPath ) )
        return;

    wxString hist = historyPath( aProjectPath );
    git_repository* repo = nullptr;

    if( git_repository_open( &repo, hist.mb_str().data() ) != 0 )
        return;

    git_revwalk* walk = nullptr;
    git_revwalk_new( &walk, repo );
    git_revwalk_push_head( walk );

    std::vector<wxString> choices;
    std::vector<wxString> hashes;
    git_oid oid;

    while( git_revwalk_next( &oid, walk ) == 0 )
    {
        git_commit* commit = nullptr;
        git_commit_lookup( &commit, repo, &oid );

        git_time_t t = git_commit_time( commit );
        wxDateTime dt( (time_t) t );
        wxString   line;

        line.Printf( wxS( "%s %s" ), dt.FormatISOCombined().c_str(),
                     wxString::FromUTF8( git_commit_summary( commit ) ) );
        choices.push_back( line );
        hashes.push_back( wxString::FromUTF8( git_oid_tostr_s( &oid ) ) );
        git_commit_free( commit );
    }

    git_revwalk_free( walk );
    git_repository_free( repo );

    if( choices.empty() )
        return;

    int index = wxGetSingleChoiceIndex( _( "Select snapshot" ), _( "Restore" ),
                                        (int) choices.size(), &choices[0], aParent );

    if( index != wxNOT_FOUND )
        RestoreCommit( aProjectPath, hashes[index] );
}


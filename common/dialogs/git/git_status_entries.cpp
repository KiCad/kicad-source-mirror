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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/git/git_status_entries.h>

#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sys/stat.h>
#include <mutex>
#include <vector>

#include <wx/choicdlg.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/richmsgdlg.h>
#include <wx/weakref.h>

#include <reporter.h>

#include <confirm.h>
#include <dialogs/git/dialog_text_diff.h>
#include <dialogs/git/git_transfer.h>
#include <dialogs/git/git_ui_utils.h>
#include <diff_merge/diff_doc_kind.h>
#include <diff_merge/diff_scene.h>
#include <diff_merge/merge_dispatch.h>
#include <git/text_diff.h>
#include <git/git_compare_handler.h>
#include <git/git_branch_handler.h>
#include <eda_base_frame.h>
#include <frame_type.h>
#include <git/kicad_git_common.h>
#include <git/git_status_handler.h>
#include <git/project_git_utils.h>
#include <i18n_utility.h>
#include <kiway.h>
#include <kiway_player.h>
#include <mail_type.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>


// Fill the status label, icon and color. Return false for changes the checklist does not show.
static bool statusToDisplay( KIGIT_COMMON::GIT_STATUS aStatus, wxString& aWord, BITMAPS& aIcon, wxColour& aColour,
                             const KICAD_DIFF::DIFF_COLOR_THEME& aTheme )
{
    switch( aStatus )
    {
    case KIGIT_COMMON::GIT_STATUS::GIT_STATUS_MODIFIED:
        aWord = _( "Modified" );
        aIcon = BITMAPS::git_modified;
        aColour = aTheme.modified.ToColour();
        return true;
    case KIGIT_COMMON::GIT_STATUS::GIT_STATUS_ADDED:
    case KIGIT_COMMON::GIT_STATUS::GIT_STATUS_UNTRACKED:
        aWord = _( "New" );
        aIcon = BITMAPS::git_add;
        aColour = aTheme.added.ToColour();
        return true;
    case KIGIT_COMMON::GIT_STATUS::GIT_STATUS_DELETED:
        aWord = _( "Deleted" );
        aIcon = BITMAPS::git_delete;
        aColour = aTheme.removed.ToColour();
        return true;
    case KIGIT_COMMON::GIT_STATUS::GIT_STATUS_CONFLICTED:
        aWord = _( "Conflict" );
        aIcon = BITMAPS::git_conflict;
        aColour = aTheme.conflict.ToColour();
        return true;
    default: return false;
    }
}


std::vector<FILE_CHECKLIST_ENTRY> BuildGitChangeEntries( KIGIT_COMMON& aCommon, const wxString& aProjectPath,
                                                         bool aDarkTheme )
{
    std::vector<FILE_CHECKLIST_ENTRY> entries;

    const std::optional<wxString> prefix = aCommon.GetProjectPrefix();

    if( !prefix )
        return entries;

    GIT_STATUS_HANDLER handler( &aCommon );
    wxString           workDir = handler.GetWorkingDirectory();

    std::map<wxString, FileStatus> statusMap = handler.GetFileStatus( *prefix, /* changed only */ true );
    const KICAD_DIFF::DIFF_COLOR_THEME theme = KICAD_DIFF::DIFF_COLOR_THEME::ForText( aDarkTheme );

    for( const auto& [path, fileStatus] : statusMap )
    {
        wxString word;
        BITMAPS  icon = BITMAPS::INVALID_BITMAP;
        wxColour colour;

        if( !statusToDisplay( fileStatus.status, word, icon, colour, theme ) )
            continue;

        if( !KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( path, aProjectPath ) )
            continue;

        // The commit handler stages paths relative to the working directory.
        wxString relativePath = path;

        if( relativePath.StartsWith( workDir ) )
            relativePath = relativePath.Mid( workDir.length() );

#ifdef __WINDOWS__
        relativePath.Replace( wxS( "\\" ), wxS( "/" ) );
#endif

        // Only files git does not know at all are hidden. One that is tracked, or staged to be,
        // is carried by a commit and blocks a merge, so hiding it would leave the user with
        // nothing to commit and no way forward.
        bool untracked = ( fileStatus.gitStatus & GIT_STATUS_WT_NEW )
                         && !( fileStatus.gitStatus & GIT_STATUS_INDEX_NEW );

        if( untracked )
        {
            // Use the session prefix so a project opened through a symlink has the same paths.
            const wxString pathPrefix = relativePath.Left( prefix->length() );

#ifdef __WINDOWS__
            bool inProject = pathPrefix.CmpNoCase( *prefix ) == 0;
#else
            bool inProject = pathPrefix == *prefix;
#endif

            if( inProject
                && KIGIT::PROJECT_GIT_UTILS::IsTransientGitFile( wxFileName( relativePath.Mid( prefix->length() ) ) ) )
            {
                continue;
            }
        }

        FILE_CHECKLIST_ENTRY entry;
        entry.section = _( "Changes" );
        entry.path = relativePath;
        entry.label = wxFileName( relativePath ).GetFullName();
        entry.status = word;
        entry.statusColour = colour;
        entry.icon = icon;
        entry.checked = true;
        entries.push_back( entry );
    }

    return entries;
}


namespace
{

struct EDITOR_DOC
{
    FRAME_T frameType;
    MAIL_T  reloadMail;
    MAIL_T  settingsMail;
};


const EDITOR_DOC g_editorDocs[] = {
    { FRAME_SCH, MAIL_SCH_RELOAD, MAIL_SCH_PROJECT_SETTINGS_CHANGED },
    { FRAME_PCB_EDITOR, MAIL_PCB_RELOAD, MAIL_PCB_PROJECT_SETTINGS_CHANGED },
};


// Nanosecond stamp on POSIX, because wxFileName truncates to whole seconds there. A same-size
// rewrite in the same second as the save before it would otherwise be missed.
bool makeFileStamp( const wxFileName& aFn, FILE_STAMP& aOut )
{
    if( !aFn.IsOk() || !aFn.FileExists() )
        return false;

    wxString path = aFn.GetFullPath();

#ifdef __WINDOWS__
    aOut.modified = aFn.GetModificationTime().GetValue().GetValue();
#else
    struct stat st;

    if( stat( path.fn_str(), &st ) != 0 )
        return false;

#ifdef __APPLE__
    aOut.modified = static_cast<long long>( st.st_mtimespec.tv_sec ) * 1000000000LL + st.st_mtimespec.tv_nsec;
#else
    aOut.modified = static_cast<long long>( st.st_mtim.tv_sec ) * 1000000000LL + st.st_mtim.tv_nsec;
#endif
#endif

    aOut.size = static_cast<long long>( aFn.GetSize().GetValue() );
    return true;
}


// Add aFn to the snapshot when it exists on disk.
void addFileToSnapshot( DOCUMENT_SNAPSHOT& aSnapshot, const wxFileName& aFn )
{
    FILE_STAMP stamp;

    if( makeFileStamp( aFn, stamp ) )
        aSnapshot.files[aFn.GetFullPath()] = stamp;
}


// A schematic document spans every sheet file, not just the root the frame names.
DOCUMENT_SNAPSHOT documentSnapshot( KIWAY& aKiway, FRAME_T aFrameType )
{
    DOCUMENT_SNAPSHOT result;
    KIWAY_PLAYER*     player = aKiway.Player( aFrameType, false );

    if( !player )
        return result;

    addFileToSnapshot( result, wxFileName( player->GetCurrentFileName() ) );

    if( aFrameType == FRAME_SCH )
    {
        // A shared hierarchy can reference sheets above the project directory, and a pull can
        // rewrite anything in the repository, so scan from the repository root.
        wxString scanRoot = player->Prj().GetGitCommon()->GetProjectDir();

        if( scanRoot.IsEmpty() )
            scanRoot = player->Prj().GetProjectPath();

        if( !scanRoot.IsEmpty() )
        {
            wxArrayString sheetFiles;
            wxDir::GetAllFiles( scanRoot, &sheetFiles, wxS( "*.kicad_sch" ), wxDIR_FILES | wxDIR_DIRS );
            sheetFiles.Sort();

            for( const wxString& sheet : sheetFiles )
                addFileToSnapshot( result, wxFileName( sheet ) );
        }
    }

    return result;
}

// A pull or branch switch can change the settings files on their own, and the loaded settings
// object would write the old values back on the next save.
DOCUMENT_SNAPSHOT settingsSnapshot()
{
    DOCUMENT_SNAPSHOT result;
    PROJECT&          prj = Pgm().GetSettingsManager().Prj();

    if( prj.GetProjectFullName().IsEmpty() )
        return result;

    wxFileName prl( prj.GetProjectFullName() );
    prl.SetExt( FILEEXT::ProjectLocalSettingsFileExtension );

    addFileToSnapshot( result, wxFileName( prj.GetProjectFullName() ) );
    addFileToSnapshot( result, prl );
    return result;
}


// The files whose on-disk identity differs, including those present in only one snapshot.
std::set<wxString> changedFilesBetween( const DOCUMENT_SNAPSHOT& aBefore, const DOCUMENT_SNAPSHOT& aAfter )
{
    std::set<wxString> changed;

    for( const auto& [path, stamp] : aBefore.files )
    {
        auto it = aAfter.files.find( path );

        if( it == aAfter.files.end() || it->second != stamp )
            changed.insert( path );
    }

    for( const auto& [path, stamp] : aAfter.files )
    {
        if( !aBefore.files.count( path ) )
            changed.insert( path );
    }

    return changed;
}

} // namespace


EDITOR_SNAPSHOT CaptureOpenEditorDocs( KIWAY& aKiway )
{
    EDITOR_SNAPSHOT before;

    for( const EDITOR_DOC& doc : g_editorDocs )
        before.editors[doc.frameType] = documentSnapshot( aKiway, doc.frameType );

    before.settings = settingsSnapshot();

    return before;
}


void ReloadOpenEditors( KIWAY& aKiway, const EDITOR_SNAPSHOT& aBefore )
{
    // Reread the settings first, so a reloading editor gets the fresh values and the next save
    // does not write the old ones back.
    bool settingsChanged = !changedFilesBetween( aBefore.settings, settingsSnapshot() ).empty();

    if( settingsChanged )
    {
        PROJECT& prj = Pgm().GetSettingsManager().Prj();

        prj.GetProjectFile().LoadFromFile( prj.GetProjectPath() );
        prj.GetLocalSettings().LoadFromFile( prj.GetProjectPath() );
    }

    const DOCUMENT_SNAPSHOT noFiles;

    for( const EDITOR_DOC& doc : g_editorDocs )
    {
        if( !aKiway.Player( doc.frameType, false ) )
            continue;

        // Reloading throws away undo history and the view state, so only do it for a document
        // whose file actually changed on disk.
        auto                     it = aBefore.editors.find( doc.frameType );
        const DOCUMENT_SNAPSHOT& before = it == aBefore.editors.end() ? noFiles : it->second;
        std::set<wxString>       changed = changedFilesBetween( before, documentSnapshot( aKiway, doc.frameType ) );

        if( changed.empty() )
        {
            // Settings that changed on their own still have to reach the live editor: the board
            // caches resolved netclasses per net, and routing would keep using the old values.
            if( settingsChanged )
            {
                std::string settingsPayload;
                aKiway.ExpressMail( doc.frameType, doc.settingsMail, settingsPayload );
            }

            continue;
        }

        // The editor skips the reload when no changed file belongs to its document. The schematic
        // snapshot spans the repository, and another project's change must not cost this one its undo.
        wxString joined;

        for( const wxString& path : changed )
            joined += path + wxS( "\n" );

        std::string payload = joined.utf8_string();
        aKiway.ExpressMail( doc.frameType, doc.reloadMail, payload );
    }
}


namespace
{

// Keep window trackers together on the UI thread. Worker dispatch copies only this context's
// shared pointer, so it never adds or removes weak-reference nodes on a live window.
struct WORKTREE_QUERY_CONTEXT
{
    WORKTREE_QUERY_CONTEXT( EDA_BASE_FRAME* aFrame, wxWindow* aParent ) :
            frame( aFrame ),
            parent( aParent ? aParent : aFrame ),
            session( aFrame->Prj().GetGitCommon() )
    {
    }

    bool IsAlive() const
    {
        return frame.get() && parent.get() && !frame->IsBeingDeleted() && !parent->IsBeingDeleted();
    }

    bool IsCurrent() const
    {
        return IsAlive() && !session->IsShuttingDown() && frame->Prj().GetGitCommon() == session
               && Pgm().GetCommonSettings()->m_Git.enableGit
               && !frame->Prj().GetLocalSettings().m_GitIntegrationDisabled;
    }

    wxWeakRef<EDA_BASE_FRAME>     frame;
    wxWeakRef<wxWindow>           parent;
    std::shared_ptr<KIGIT_COMMON> session;
};


// These checks precede UI confirmations, so they are snapshots rather than part of a write
// transaction. The backend still validates the working tree under the mutation's own lock.
void checkProjectWorktree( EDA_BASE_FRAME* aFrame, wxWindow* aParent, KIGIT_QUERY& aQuery,
                           std::function<void( const wxString&, const GIT_WORKTREE_STATUS& )> aReady,
                           std::function<void()>                                              aFinished = {} )
{
    struct OUTCOME
    {
        GIT_WORKTREE_STATUS worktree;
        wxString            currentBranch;
    };

    wxString projectPath = aFrame->Prj().GetProjectPath();
    auto     context = std::make_shared<WORKTREE_QUERY_CONTEXT>( aFrame, aParent );
    auto     outcome = std::make_shared<OUTCOME>();

    aQuery.Run(
            context->session,
            [outcome, projectPath]( KIGIT_COMMON& aCommon )
            {
                outcome->currentBranch = aCommon.GetCurrentBranchName();
                outcome->worktree = ReadGitWorktreeStatus( aCommon, projectPath );
            },
            [context, outcome, aReady, aFinished]()
            {
                auto proceed = [&]()
                {
                    if( !context->IsCurrent() )
                    {
                        return;
                    }

                    if( !ProjectSafeToRewrite( context->frame, context->parent, outcome->worktree ) )
                        return;

                    aReady( outcome->currentBranch, outcome->worktree );
                };

                proceed();

                if( aFinished && context->IsAlive() )
                {
                    aFinished();
                }
            } );
}

} // namespace


void ProjectClearOfUncommittedWork( EDA_BASE_FRAME* aFrame, wxWindow* aParent, KIGIT_QUERY& aQuery,
                                    std::function<void( const GIT_WORKTREE_STATUS& )> aReady )
{
    checkProjectWorktree( aFrame, aParent, aQuery,
                          [aReady]( const wxString&, const GIT_WORKTREE_STATUS& aStatus )
                          {
                              aReady( aStatus );
                          } );
}


void SwitchProjectToBranch( EDA_BASE_FRAME* aFrame, wxWindow* aParent, const wxString& aBranch, KIGIT_QUERY& aQuery,
                            std::function<void()> aFinished )
{
    if( aBranch.IsEmpty() )
    {
        if( aFinished )
            aFinished();

        return;
    }

    auto context = std::make_shared<WORKTREE_QUERY_CONTEXT>( aFrame, aParent );

    checkProjectWorktree(
            aFrame, aParent, aQuery,
            [context, aBranch]( const wxString& aCurrentBranch, const GIT_WORKTREE_STATUS& aStatus )
            {
                const wxWeakRef<EDA_BASE_FRAME>&     frame = context->frame;
                const wxWeakRef<wxWindow>&           parent = context->parent;
                const std::shared_ptr<KIGIT_COMMON>& common = context->session;

                if( aBranch == aCurrentBranch )
                    return;

                if( !IsOK( parent, wxString::Format( _( "Switch to branch '%s'? This replaces the project files "
                                                        "on disk with that branch's version." ),
                                                     aBranch ) ) )
                {
                    return;
                }

                // The confirmation runs a nested event loop and may have closed the project.
                if( !context->IsCurrent() )
                {
                    return;
                }

                // The confirmation may have allowed new edits. Saving them here would create
                // the uncommitted files that the checkout refuses to overwrite.
                if( !ProjectSafeToRewrite( frame, parent, aStatus ) )
                    return;

                if( !context->IsCurrent() )
                {
                    return;
                }

                EDITOR_SNAPSHOT docsBefore = CaptureOpenEditorDocs( frame->Kiway() );
                BranchResult        result = BranchResult::Error;
                KIGIT_ERROR_MESSAGE resultError;

                {
                    std::unique_lock<std::mutex> lock = KIGIT::TryLockAction( *common, parent );

                    if( !lock.owns_lock() )
                        return;

                    GIT_BRANCH_HANDLER handler( common.get() );
                    result = handler.SwitchToBranch( aBranch );
                    resultError = handler.PopError();

                    // The next transfer must use the new branch's upstream and credentials.
                    // Update them before releasing the same lock that protects the checkout.
                    if( result == BranchResult::Success )
                        common->UpdateCurrentBranchInfo();
                }

                if( result == BranchResult::Conflict )
                {
                    DisplayInfoMessage( parent, resultError.summary );
                    return;
                }

                if( result != BranchResult::Success )
                {
                    if( resultError.summary.IsEmpty() )
                    {
                        DisplayErrorMessage( parent,
                                             wxString::Format( _( "Could not switch to branch '%s'." ), aBranch ),
                                             KIGIT_COMMON::GetLastGitError() );
                    }
                    else
                    {
                        DisplayErrorMessage( parent, resultError.summary, resultError.details );
                    }

                    return;
                }

                ReloadOpenEditors( frame->Kiway(), docsBefore );
                common->NotifyRefreshListeners();
            },
            std::move( aFinished ) );
}


namespace
{

enum class EDITOR_SCOPE
{
    DOCUMENTS,
    LIBRARIES,
    ALL
};


struct GIT_EDITOR
{
    FRAME_T               frameType;
    const wxChar*         label;
    std::optional<MAIL_T> saveMail; ///< Library editors cannot be saved by mail.
};


const GIT_EDITOR g_gitEditors[] = {
    { FRAME_SCH, _HKI( "Schematic Editor" ), MAIL_SCH_SAVE },
    { FRAME_PCB_EDITOR, _HKI( "PCB Editor" ), MAIL_PCB_SAVE },
    { FRAME_SCH_SYMBOL_EDITOR, _HKI( "Symbol Editor" ), std::nullopt },
    { FRAME_FOOTPRINT_EDITOR, _HKI( "Footprint Editor" ), std::nullopt },
};


// Keep editor identifiers across prompts, since a prompt can outlive an editor window.
std::vector<const GIT_EDITOR*> dirtyEditors( KIWAY& aKiway, EDITOR_SCOPE aScope )
{
    std::vector<const GIT_EDITOR*> dirty;

    for( const GIT_EDITOR& editor : g_gitEditors )
    {
        bool document = editor.saveMail.has_value();

        if( ( aScope == EDITOR_SCOPE::DOCUMENTS && !document ) || ( aScope == EDITOR_SCOPE::LIBRARIES && document ) )
        {
            continue;
        }

        KIWAY_PLAYER* player = aKiway.Player( editor.frameType, false );

        if( player && player->IsContentModified() )
            dirty.push_back( &editor );
    }

    return dirty;
}


std::vector<wxString> dirtyEditorNames( KIWAY& aKiway, EDITOR_SCOPE aScope )
{
    std::vector<wxString> names;

    for( const GIT_EDITOR* editor : dirtyEditors( aKiway, aScope ) )
        names.push_back( wxGetTranslation( editor->label ) );

    return names;
}

} // namespace


bool DocumentEditorsDirty( KIWAY& aKiway )
{
    return !dirtyEditors( aKiway, EDITOR_SCOPE::DOCUMENTS ).empty();
}


bool EditorsDirty( KIWAY& aKiway )
{
    return !dirtyEditors( aKiway, EDITOR_SCOPE::ALL ).empty();
}


std::vector<wxString> LibraryEditorsWithUnsavedChanges( KIWAY& aKiway )
{
    return dirtyEditorNames( aKiway, EDITOR_SCOPE::LIBRARIES );
}


std::vector<wxString> ChangedTrackedFilesOutsideProject( KIGIT_COMMON& aCommon, const wxString& aProjectPath )
{
    std::vector<wxString> paths;

    if( !aCommon.GetRepo() )
        return paths;

    GIT_STATUS_HANDLER handler( &aCommon );
    wxString           workDir = handler.GetWorkingDirectory();

    for( const auto& [path, fileStatus] : handler.GetFileStatus( wxEmptyString, /* changed only */ true ) )
    {
        // The display status groups untracked and staged new files together. Only raw flags
        // distinguish changes to tracked files from untracked siblings that do not block this.
        if( ( fileStatus.gitStatus & ~( GIT_STATUS_WT_NEW | GIT_STATUS_IGNORED ) ) == GIT_STATUS_CURRENT )
        {
            continue;
        }

        if( KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( path, aProjectPath ) )
            continue;

        wxString relativePath = path;

        if( relativePath.StartsWith( workDir ) )
            relativePath = relativePath.Mid( workDir.length() );

        paths.push_back( relativePath );
    }

    return paths;
}


GIT_WORKTREE_STATUS ReadGitWorktreeStatus( KIGIT_COMMON& aCommon, const wxString& aProjectPath )
{
    GIT_WORKTREE_STATUS status;

    if( !aCommon.GetRepo() )
        return status;

    status.hasTrackedChanges = aCommon.HasUncommittedTrackedChanges( &status.checked );
    status.lfsMessage = KIGIT::PROJECT_GIT_UTILS::LfsRefusal( &aCommon );

    // A sibling project's tracked edits block the same repository-wide rewrite.
    if( status.hasTrackedChanges )
        status.outsideProject = ChangedTrackedFilesOutsideProject( aCommon, aProjectPath );

    return status;
}


wxString GIT_WORKTREE_STATUS::RefusalMessage( const std::vector<wxString>& aUnsavedEditors,
                                              bool                         aAllowTrackedChanges ) const
{
    if( !aUnsavedEditors.empty() )
    {
        wxString listed;

        for( const wxString& name : aUnsavedEditors )
            listed += wxString::Format( wxS( "\n    - %s" ), name );

        wxString message = aAllowTrackedChanges
                                   ? _( "The following editors have unsaved changes:%s\n\n"
                                        "Save or discard those changes before stashing." )
                                   : _( "The following editors have unsaved changes:%s\n\n"
                                        "Save or discard those changes first. If saving leaves uncommitted "
                                        "files, commit or stash them before trying again." );

        return wxString::Format( message, listed );
    }

    if( !checked )
        return _( "Could not check the working files. Try again before rewriting project files." );

    if( !lfsMessage.IsEmpty() )
        return lfsMessage;

    if( !hasTrackedChanges || aAllowTrackedChanges )
        return wxEmptyString;

    if( !outsideProject.empty() )
    {
        wxString listed;

        for( const wxString& path : outsideProject )
            listed += wxS( "\n    " ) + path;

        return wxString::Format( _( "There are uncommitted changes elsewhere in this repository:%s\n\n"
                                    "Commit or discard them from that project first, then try again." ),
                                 listed );
    }

    return _( "There are changes that are not committed yet, and this would run over them. "
              "Commit them first, or set them aside with Stash Changes." );
}


bool ProjectSafeToRewrite( EDA_BASE_FRAME* aFrame, wxWindow* aParent, const GIT_WORKTREE_STATUS& aStatus,
                           bool aAllowTrackedChanges )
{
    std::vector<wxString> unsaved = dirtyEditorNames( aFrame->Kiway(), EDITOR_SCOPE::ALL );
    wxString message = aStatus.RefusalMessage( unsaved, aAllowTrackedChanges );

    if( message.IsEmpty() )
        return true;

    DisplayInfoMessage( aParent ? aParent : aFrame, message );
    return false;
}


bool PrepareEditorsForGitOperation( EDA_BASE_FRAME* aFrame, GIT_EDITOR_PREP aPurpose, bool* aSavedEditors )
{
    if( aSavedEditors )
        *aSavedEditors = false;

    // Rewriting a library an editor holds would let its next save overwrite what arrived.
    if( aPurpose == GIT_EDITOR_PREP::WORKING_TREE )
    {
        std::vector<wxString> libraryEditors = LibraryEditorsWithUnsavedChanges( aFrame->Kiway() );

        if( !libraryEditors.empty() )
        {
            wxString listed;

            for( const wxString& name : libraryEditors )
                listed += wxString::Format( wxS( "\n    - %s" ), name );

            DisplayInfoMessage( aFrame, wxString::Format( _( "The following editors have unsaved library "
                                                             "changes:%s\n\nThis rewrites files on disk. Save or "
                                                             "discard those changes first, then try again." ),
                                                          listed ) );
            return false;
        }
    }

    std::vector<const GIT_EDITOR*> dirty = dirtyEditors( aFrame->Kiway(), EDITOR_SCOPE::DOCUMENTS );

    if( !dirty.empty() )
    {
        wxString listed;

        for( const GIT_EDITOR* editor : dirty )
            listed += wxString::Format( wxS( "\n    - %s" ), wxGetTranslation( editor->label ) );

        wxString message;
        long     style = wxYES_DEFAULT | wxICON_WARNING | wxCENTER;

        if( aPurpose == GIT_EDITOR_PREP::WORKING_TREE )
        {
            message = wxString::Format( _( "The following editors have unsaved changes:%s\n\n"
                                           "This rewrites those files on disk. They must be saved "
                                           "first, otherwise the next save would overwrite what "
                                           "arrives." ),
                                        listed );
            style |= wxYES_NO;
        }
        else
        {
            message = wxString::Format( _( "The following editors have unsaved changes:%s\n\n"
                                           "Save them and review the updated file list before committing?" ),
                                        listed );
            style |= wxYES_NO | wxCANCEL;
        }

        wxRichMessageDialog dlg( aFrame, message, _( "Unsaved Changes" ), style );

        if( aPurpose == GIT_EDITOR_PREP::WORKING_TREE )
            dlg.SetYesNoLabels( _( "&Save and Continue" ), _( "Cancel" ) );
        else
            dlg.SetYesNoCancelLabels( _( "&Save and Review" ), _( "Commit &Anyway" ), _( "Cancel" ) );

        int answer = dlg.ShowModal();

        if( answer == wxID_CANCEL )
            return false;

        // Rewriting the working tree offers no "continue anyway", so anything else is a cancel.
        if( aPurpose == GIT_EDITOR_PREP::WORKING_TREE && answer != wxID_YES )
            return false;

        if( answer == wxID_YES )
        {
            for( const GIT_EDITOR* editor : dirty )
            {
                std::string payload;
                aFrame->Kiway().ExpressMail( editor->frameType, *editor->saveMail, payload );

                if( payload != "success" )
                {
                    DisplayErrorMessage(
                            aFrame, wxString::Format( _( "Could not save %s." ), wxGetTranslation( editor->label ) ) );
                    return false;
                }
            }

            // Include project changes from saving editors only when the user chose to save.
            if( aPurpose == GIT_EDITOR_PREP::COMMIT )
                aFrame->GetSettingsManager()->SaveProject();

            if( aSavedEditors )
                *aSavedEditors = true;
        }
    }

    return true;
}


GIT_DIFF_WORKSPACE::GIT_DIFF_WORKSPACE()
{
    m_root.MakeDirectory( wxS( "kicad_gitdiff_" ), 0700 );
}


wxString GIT_DIFF_WORKSPACE::Materialize( git_repository* aRepo, const wxString& aRef, const wxString& aRelPath,
                                          KICAD_DIFF::DOC_KIND aKind )
{
    if( !IsOk() )
        return wxEmptyString;

    const wxString sep = wxFileName::GetPathSeparator();

    // A schematic root sheet references its sub-sheet files by relative path, so check out the
    // whole tree and the sub-sheets come along. A board is self-contained, so write just the blob.
    if( aKind == KICAD_DIFF::DOC_KIND::SCH )
    {
        wxString dir = Root() + sep + wxString::Format( wxS( "s%d" ), m_slot++ );

        if( !KIGIT::PROJECT_GIT_UTILS::WriteTreeToDir( aRepo, aRef, dir ) )
            return wxEmptyString;

        wxString path = dir + sep + aRelPath;

        return wxFileName::FileExists( path ) ? path : wxString( wxEmptyString );
    }

    wxString path = Root() + sep + wxString::Format( wxS( "s%d_" ), m_slot++ ) + wxFileName( aRelPath ).GetFullName();

    if( !KIGIT::PROJECT_GIT_UTILS::WriteBlobToFile( aRepo, aRef, aRelPath, path ) )
        return wxEmptyString;

    return path;
}


namespace
{

/**
 * One full comparison request. The completion owns this object until its query finishes.
 * Widgets are weak references used only on the UI thread, so closing either window drops
 * the result without waiting for extraction.
 */
struct MODAL_GIT_DIFF_REQUEST
{
    MODAL_GIT_DIFF_REQUEST( EDA_BASE_FRAME* aFrame, wxWindow* aParent ) :
            frame( aFrame ),
            parent( aParent ),
            session( aFrame->Prj().GetGitCommon() )
    {
    }

    bool IsCurrent() const
    {
        return frame.get() && parent.get() && !frame->IsBeingDeleted() && !parent->IsBeingDeleted()
               && frame->Prj().GetGitCommon() == session && !session->IsShuttingDown();
    }

    wxWeakRef<EDA_BASE_FRAME>     frame;
    wxWeakRef<wxWindow>           parent;
    std::shared_ptr<KIGIT_COMMON> session;
    KIGIT_QUERY                   query;
};


struct MODAL_GIT_DIFF_RESULT
{
    std::unique_ptr<GIT_DIFF_WORKSPACE> workspace;
    GIT_FILE_SIDES                      sides;
    std::vector<KIGIT::TEXT_DIFF_LINE>  textLines;
    bool                                ready = false;
};


std::string readDiffText( const wxString& aPath )
{
    if( aPath.IsEmpty() )
        return {};

    std::ifstream in( aPath.fn_str(), std::ios::binary );
    return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
}


// A named file can be loaded directly. Otherwise ShowGitCommitDiff obtains the choice first.
// An empty commit hash compares HEAD with the working copy.
void loadModalGitDiff( const std::shared_ptr<MODAL_GIT_DIFF_REQUEST>& aRequest, const wxString& aRelativePath,
                       const wxString& aCommitHash )
{
    auto result = std::make_shared<MODAL_GIT_DIFF_RESULT>();
    result->sides.error = _( "Could not compare this file." );

    aRequest->query.Run(
            aRequest->session,
            [result, aRelativePath, aCommitHash]( KIGIT_COMMON& aCommon )
            {
                result->workspace = std::make_unique<GIT_DIFF_WORKSPACE>();

                if( !result->workspace->IsOk() )
                {
                    result->sides.error = _( "Could not create a temporary directory for the diff." );
                    return;
                }

                bool materialized =
                        aCommitHash.IsEmpty()
                                ? MaterializeWorkingFile( aCommon, *result->workspace, aRelativePath, result->sides )
                                : MaterializeCommitFile( aCommon, *result->workspace, aCommitHash, aRelativePath,
                                                         result->sides );

                if( !materialized )
                    return;

                if( result->sides.kind == KICAD_DIFF::DOC_KIND::UNKNOWN )
                {
                    std::string before = readDiffText( result->sides.before );
                    std::string after = readDiffText( result->sides.after );

                    if( KIGIT::TextLooksBinary( before ) || KIGIT::TextLooksBinary( after ) )
                    {
                        result->sides.error = wxString::Format(
                                _( "'%s' is not a plain text file. KiCad compares design files and text files "
                                   "only, so there is no comparison to show." ),
                                aRelativePath );
                        return;
                    }

                    result->textLines =
                            KIGIT::BuildTextDiff( KIGIT::SplitTextLines( before ), KIGIT::SplitTextLines( after ) );
                }

                result->ready = true;
            },
            [aRequest, result, aRelativePath, aCommitHash]()
            {
                if( !aRequest->IsCurrent() )
                    return;

                wxWindow* parent = aRequest->parent.get();

                if( !result->ready )
                {
                    if( !result->sides.error.IsEmpty() )
                        DisplayInfoMessage( parent, result->sides.error );

                    return;
                }

                if( result->sides.kind == KICAD_DIFF::DOC_KIND::UNKNOWN )
                {
                    DIALOG_TEXT_DIFF dlg( parent, aRelativePath, result->textLines );
                    dlg.ShowModal();
                    return;
                }

                wxString beforeLabel = aCommitHash.IsEmpty()
                                               ? _( "BASE (HEAD)" )
                                               : wxString::Format( _( "BASE (parent of %s)" ), aCommitHash );
                wxString afterLabel = aCommitHash.IsEmpty() ? _( "WORKING (changes)" )
                                                            : wxString::Format( _( "WORKING (%s)" ), aCommitHash );

                // The owning kiface loads and displays documents on the UI thread. Keep the
                // scratch files alive until the modal comparison has closed.
                WX_STRING_REPORTER reporter;
                KICAD_DIFF::DispatchOpenDiffDialog( aRequest->frame->Kiway(), result->sides.kind, result->sides.before,
                                                    result->sides.after, beforeLabel, afterLabel, parent, &reporter );

                if( aRequest->IsCurrent() && reporter.HasMessage() )
                    DisplayErrorMessage( parent, _( "Could not open the diff." ), reporter.GetMessages() );
            } );
}

} // namespace


void ShowGitFileDiff( EDA_BASE_FRAME* aFrame, const wxString& aRelativePath )
{
    auto request = std::make_shared<MODAL_GIT_DIFF_REQUEST>( aFrame, aFrame );
    loadModalGitDiff( request, aRelativePath, wxEmptyString );
}


bool MaterializeWorkingFile( KIGIT_COMMON& aCommon, GIT_DIFF_WORKSPACE& aWorkspace, const wxString& aRelativePath,
                             GIT_FILE_SIDES& aSides )
{
    git_repository* repo = aCommon.GetRepo();

    if( !repo )
        return false;

    aSides.kind = KICAD_DIFF::DocKindFromExtension( aRelativePath );

    GIT_STATUS_HANDLER handler( &aCommon );
    aSides.after = handler.GetWorkingDirectory() + aRelativePath;

    // A new file has no committed version, so that side stays empty and shows as added.
    bool inHead = aCommon.IsPathInHead( aRelativePath );

    if( inHead )
        aSides.before = aWorkspace.Materialize( repo, wxT( "HEAD" ), aRelativePath, aSides.kind );

    if( inHead && aSides.before.IsEmpty() )
    {
        aSides.error = wxString::Format( _( "Could not read the committed version of '%s'." ), aRelativePath );
        return false;
    }

    // A deleted file has no working copy, so an empty path makes the loader synthesize an empty
    // document instead of failing on a file that is gone.
    if( !wxFileName::FileExists( aSides.after ) )
        aSides.after.clear();

    return true;
}


void ShowGitCommitDiff( EDA_BASE_FRAME* aFrame, const wxString& aCommitHash, wxWindow* aParentWindow,
                        const wxString& aRelativePath )
{
    auto request = std::make_shared<MODAL_GIT_DIFF_REQUEST>( aFrame, aParentWindow ? aParentWindow : aFrame );

    // A caller that named a file knows its own list, which for a stash also holds the files
    // that were untracked. Those are not in the parent-to-commit diff used by the chooser.
    if( !aRelativePath.IsEmpty() )
    {
        loadModalGitDiff( request, aRelativePath, aCommitHash );
        return;
    }

    auto changedPaths = std::make_shared<std::vector<wxString>>();

    request->query.Run(
            request->session,
            [changedPaths, aCommitHash]( KIGIT_COMMON& aCommon )
            {
                git_repository* repo = aCommon.GetRepo();
                wxString        parentRef;
                git_object*     commitObj = nullptr;

                // A root commit has no parent, so compare it against the empty tree.
                if( git_revparse_single( &commitObj, repo, aCommitHash.utf8_string().c_str() ) == GIT_OK )
                {
                    if( git_object_type( commitObj ) == GIT_OBJECT_COMMIT
                        && git_commit_parentcount( reinterpret_cast<git_commit*>( commitObj ) ) > 0 )
                    {
                        parentRef = aCommitHash + wxT( "^" );
                    }

                    git_object_free( commitObj );
                }

                for( const KIGIT::CHANGED_FILE& file : KIGIT::CompareRefs( repo, parentRef, aCommitHash ) )
                    changedPaths->push_back( file.path );
            },
            [request, changedPaths, aCommitHash]()
            {
                if( !request->IsCurrent() )
                    return;

                wxWindow* parent = request->parent.get();

                if( changedPaths->empty() )
                {
                    DisplayInfoMessage( parent, _( "This commit has no file changes to show." ) );
                    return;
                }

                wxString chosen = changedPaths->front();

                if( changedPaths->size() > 1 )
                {
                    wxArrayString choices;

                    for( const wxString& path : *changedPaths )
                        choices.Add( path );

                    int sel = wxGetSingleChoiceIndex( _( "This commit changed more than one file. "
                                                         "Select one to compare:" ),
                                                      _( "Select File to Compare" ), choices, parent );

                    if( sel < 0 )
                        return;

                    chosen = ( *changedPaths )[sel];
                }

                // A modal chooser may have processed a project close or switch.
                if( request->IsCurrent() )
                    loadModalGitDiff( request, chosen, aCommitHash );
            } );
}


bool MaterializeCommitFile( KIGIT_COMMON& aCommon, GIT_DIFF_WORKSPACE& aWorkspace, const wxString& aCommitHash,
                            const wxString& aRelativePath, GIT_FILE_SIDES& aSides )
{
    // The caller may have run a chooser since it last looked, so the project may have closed.
    git_repository* repo = aCommon.GetRepo();

    if( !repo )
        return false;

    // A root commit has no "<hash>^". An empty base ref compares it against the empty tree, so
    // its files show as added.
    wxString    parentRef;
    git_object* commitObj = nullptr;
    unsigned    parentCount = 0;

    if( git_revparse_single( &commitObj, repo, aCommitHash.utf8_string().c_str() ) == GIT_OK )
    {
        if( git_object_type( commitObj ) == GIT_OBJECT_COMMIT )
            parentCount = git_commit_parentcount( reinterpret_cast<git_commit*>( commitObj ) );

        if( parentCount > 0 )
            parentRef = aCommitHash + wxT( "^" );

        git_object_free( commitObj );
    }

    for( const KIGIT::CHANGED_FILE& file : KIGIT::CompareRefs( repo, parentRef, aCommitHash ) )
    {
        if( file.path != aRelativePath )
            continue;

        aSides.kind = KICAD_DIFF::DocKindFromExtension( file.path );

        // Only the side that exists is written out.
        bool hasParent = file.status != KIGIT::FILE_CHANGE_STATUS::ADDED;
        bool hasCommit = file.status != KIGIT::FILE_CHANGE_STATUS::REMOVED;

        if( hasParent )
        {
            wxString parentPath = file.oldPath.IsEmpty() ? file.path : file.oldPath;
            aSides.before = aWorkspace.Materialize( repo, parentRef, parentPath, aSides.kind );
        }

        if( hasCommit )
            aSides.after = aWorkspace.Materialize( repo, aCommitHash, file.path, aSides.kind );

        if( ( hasParent && aSides.before.IsEmpty() ) || ( hasCommit && aSides.after.IsEmpty() ) )
        {
            aSides.error = _( "Could not read this commit's file contents." );
            return false;
        }

        return true;
    }

    // A stash keeps the files that were untracked when it was taken in a third parent, so they
    // are missing from the diff above. Nothing came before them, so only one side has content.
    if( parentCount > 2 )
    {
        aSides.kind = KICAD_DIFF::DocKindFromExtension( aRelativePath );
        aSides.after = aWorkspace.Materialize( repo, aCommitHash + wxT( "^3" ), aRelativePath, aSides.kind );

        if( !aSides.after.IsEmpty() )
            return true;
    }

    aSides.error = wxString::Format( _( "This commit did not change '%s'." ), aRelativePath );
    return false;
}

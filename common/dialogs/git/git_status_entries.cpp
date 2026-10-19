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
#include <set>
#include <sys/stat.h>
#include <mutex>
#include <vector>

#include <wx/choicdlg.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/richmsgdlg.h>
#include <wx/stdpaths.h>
#include <wx/time.h>
#include <wx/utils.h>

#include <reporter.h>

#include <confirm.h>
#include <dialogs/git/dialog_text_diff.h>
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
#include <kiway.h>
#include <kiway_player.h>
#include <kiplatform/ui.h>
#include <mail_type.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>


// False for a status that is not a working-tree change worth showing.
// The status word, its icon, and its colour, which is the one the diff draws that kind of
// change with, lifted a little on a dark background.
static bool statusToDisplay( KIGIT_COMMON::GIT_STATUS aStatus, wxString& aWord, BITMAPS& aIcon, wxColour& aColour )
{
    KICAD_DIFF::DIFF_COLOR_THEME theme;
    bool                         dark = KIPLATFORM::UI::IsDarkTheme();

    auto colour = [dark]( const KIGFX::COLOR4D& aBase )
    {
        return ( dark ? aBase.Brightened( 0.4 ) : aBase ).ToColour();
    };

    switch( aStatus )
    {
    case KIGIT_COMMON::GIT_STATUS::GIT_STATUS_MODIFIED:
        aWord = _( "Modified" );
        aIcon = BITMAPS::git_modified;
        aColour = colour( theme.modified );
        return true;
    case KIGIT_COMMON::GIT_STATUS::GIT_STATUS_ADDED:
    case KIGIT_COMMON::GIT_STATUS::GIT_STATUS_UNTRACKED:
        aWord = _( "New" );
        aIcon = BITMAPS::git_add;
        aColour = colour( theme.added );
        return true;
    case KIGIT_COMMON::GIT_STATUS::GIT_STATUS_DELETED:
        aWord = _( "Deleted" );
        aIcon = BITMAPS::git_delete;
        aColour = colour( theme.removed );
        return true;
    case KIGIT_COMMON::GIT_STATUS::GIT_STATUS_CONFLICTED:
        aWord = _( "Conflict" );
        aIcon = BITMAPS::git_conflict;
        aColour = colour( theme.conflict );
        return true;
    default: return false;
    }
}


std::vector<FILE_CHECKLIST_ENTRY> BuildGitChangeEntries( PROJECT& aProject )
{
    std::vector<FILE_CHECKLIST_ENTRY> entries;

    std::shared_ptr<KIGIT_COMMON> common = aProject.GetGitCommon();

    if( !common->GetRepo() )
        return entries;

    GIT_STATUS_HANDLER handler( common.get() );
    wxString           workDir = handler.GetWorkingDirectory();
    wxString           projectPath = aProject.GetProjectPath();
    wxString           projectName = aProject.GetProjectName();

    // Walk only the project folder, since the rest of the repository is filtered out below
    // anyway. The trailing slash keeps a sibling folder sharing the prefix out. An empty scope
    // means the project is the whole repository, or its folder could not be worked out.
    wxString scope;

    {
        wxString project = projectPath;
        wxString root = workDir;

        project.Replace( wxS( "\\" ), wxS( "/" ) );
        root.Replace( wxS( "\\" ), wxS( "/" ) );

        if( !root.EndsWith( wxS( "/" ) ) )
            root += wxS( "/" );

        if( project.StartsWith( root ) )
        {
            scope = project.Mid( root.length() );

            if( !scope.IsEmpty() && !scope.EndsWith( wxS( "/" ) ) )
                scope += wxS( "/" );
        }
    }

    std::map<wxString, FileStatus> statusMap = handler.GetFileStatus( scope, /* changed only */ true );

    for( const auto& [path, fileStatus] : statusMap )
    {
        wxString word;
        BITMAPS  icon = BITMAPS::INVALID_BITMAP;
        wxColour colour;

        if( !statusToDisplay( fileStatus.status, word, icon, colour ) )
            continue;

        if( !KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( path, projectPath ) )
            continue;

        wxFileName fn( path );

        // Only files git does not know at all are hidden. One that is tracked, or staged to be,
        // is carried by a commit and blocks a merge, so hiding it would leave the user with
        // nothing to commit and no way forward.
        bool untracked = ( fileStatus.gitStatus & GIT_STATUS_WT_NEW )
                         && !( fileStatus.gitStatus & GIT_STATUS_INDEX_NEW );

        if( untracked && KIGIT::PROJECT_GIT_UTILS::IsTransientGitFile( fn, projectName ) )
            continue;

        // A local settings file nobody has committed is this user's own, not a project change.
        if( untracked && fn.GetExt().CmpNoCase( FILEEXT::ProjectLocalSettingsFileExtension ) == 0 )
            continue;

        // The commit handler stages paths relative to the working directory.
        wxString relativePath = path;

        if( relativePath.StartsWith( workDir ) )
            relativePath = relativePath.Mid( workDir.length() );

#ifdef __WINDOWS__
        relativePath.Replace( wxS( "\\" ), wxS( "/" ) );
#endif

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


bool ProjectClearOfUncommittedWork( EDA_BASE_FRAME* aFrame, wxWindow* aParent )
{
    std::shared_ptr<KIGIT_COMMON> common = aFrame->Prj().GetGitCommon();
    bool                          dirty = false;

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( aParent, KIGIT::BusyMessage() );
            return false;
        }

        dirty = common->HasUncommittedTrackedChanges();
    }

    if( dirty || DocumentEditorsDirty( aFrame->Kiway() )
        || !LibraryEditorsWithUnsavedChanges( aFrame->Kiway() ).empty() )
    {
        DisplayInfoMessage( aParent, _( "There are changes that are not committed yet, and this would run over "
                                        "them. Commit them first, or set them aside with Stash Changes." ) );
        return false;
    }

    return true;
}


bool SwitchProjectToBranch( EDA_BASE_FRAME* aFrame, wxWindow* aParent, const wxString& aBranch )
{
    if( aBranch.IsEmpty() )
        return false;

    std::shared_ptr<KIGIT_COMMON> common = aFrame->Prj().GetGitCommon();
    wxString                      current;

    // Reading the current branch touches the repository, so do it under the lock too.
    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( aParent, KIGIT::BusyMessage() );
            return false;
        }

        current = common->GetCurrentBranchName();
    }

    if( aBranch == current )
        return false;

    // Switching refuses over uncommitted work, so saving the editors first would only turn
    // unsaved edits into the very changes that block it. Ask for a commit up front instead.
    if( !ProjectClearOfUncommittedWork( aFrame, aParent ) )
        return false;

    if( !IsOK( aParent, wxString::Format( _( "Switch to branch '%s'? This replaces the project files "
                                             "on disk with that branch's version." ),
                                          aBranch ) ) )
    {
        return false;
    }

    // Save rather than close: a close would take the frame hosting the caller with it.
    if( !PrepareEditorsForGitOperation( aFrame, GIT_EDITOR_PREP::WORKING_TREE ) )
        return false;

    EDITOR_SNAPSHOT docsBefore = CaptureOpenEditorDocs( aFrame->Kiway() );
    BranchResult    result = BranchResult::Error;
    wxString        resultError;

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( aParent, KIGIT::BusyMessage() );
            return false;
        }

        GIT_BRANCH_HANDLER handler( common.get() );
        result = handler.SwitchToBranch( aBranch );
        resultError = handler.GetErrorString();
    }

    if( result == BranchResult::Conflict )
    {
        DisplayInfoMessage( aParent, resultError );
        return false;
    }

    if( result != BranchResult::Success )
    {
        DisplayErrorMessage( aParent, wxString::Format( _( "Could not switch to branch '%s'." ), aBranch ),
                             resultError.IsEmpty() ? KIGIT_COMMON::GetLastGitError() : resultError );
        return false;
    }

    ReloadOpenEditors( aFrame->Kiway(), docsBefore );
    common->NotifyRefreshListeners();
    return true;
}


bool DocumentEditorsDirty( KIWAY& aKiway )
{
    for( FRAME_T type : { FRAME_SCH, FRAME_PCB_EDITOR } )
    {
        KIWAY_PLAYER* player = aKiway.Player( type, false );

        if( player && player->IsContentModified() )
            return true;
    }

    return false;
}


std::vector<wxString> LibraryEditorsWithUnsavedChanges( KIWAY& aKiway )
{
    std::vector<wxString> names;

    const std::pair<FRAME_T, wxString> editors[] = {
        { FRAME_SCH_SYMBOL_EDITOR, _( "Symbol Editor" ) },
        { FRAME_FOOTPRINT_EDITOR, _( "Footprint Editor" ) },
    };

    for( const auto& [type, name] : editors )
    {
        KIWAY_PLAYER* player = aKiway.Player( type, false );

        if( player && player->IsContentModified() )
            names.push_back( name );
    }

    return names;
}


std::vector<wxString> ChangedTrackedFilesOutsideProject( PROJECT& aProject )
{
    std::vector<wxString>         paths;
    std::shared_ptr<KIGIT_COMMON> common = aProject.GetGitCommon();

    if( !common->GetRepo() )
        return paths;

    GIT_STATUS_HANDLER handler( common.get() );
    wxString           workDir = handler.GetWorkingDirectory();
    wxString           projectPath = aProject.GetProjectPath();

    for( const auto& [path, fileStatus] : handler.GetFileStatus( wxEmptyString, /* changed only */ true ) )
    {
        if( fileStatus.status == KIGIT_COMMON::GIT_STATUS::GIT_STATUS_UNTRACKED
            || fileStatus.status == KIGIT_COMMON::GIT_STATUS::GIT_STATUS_IGNORED
            || fileStatus.status == KIGIT_COMMON::GIT_STATUS::GIT_STATUS_CURRENT )
        {
            continue;
        }

        if( KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( path, projectPath ) )
            continue;

        wxString relativePath = path;

        if( relativePath.StartsWith( workDir ) )
            relativePath = relativePath.Mid( workDir.length() );

        paths.push_back( relativePath );
    }

    return paths;
}


bool PrepareEditorsForGitOperation( EDA_BASE_FRAME* aFrame, GIT_EDITOR_PREP aPurpose )
{
    wxString lfsMessage;

    if( std::shared_ptr<KIGIT_COMMON> common = aFrame->Prj().GetGitCommon() )
    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( aFrame, KIGIT::BusyMessage() );
            return false;
        }

        if( common->GetRepo() )
        {
            lfsMessage = KIGIT::PROJECT_GIT_UTILS::LfsRefusal( common.get(), aFrame->Prj() );
        }
    }

    if( !lfsMessage.IsEmpty() )
    {
        DisplayErrorMessage( aFrame, lfsMessage );
        return false;
    }

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

    struct DIRTY_EDITOR
    {
        FRAME_T  frameType;
        MAIL_T   saveMail;
        wxString label;
    };

    const DIRTY_EDITOR candidates[] = {
        { FRAME_SCH, MAIL_SCH_SAVE, _( "Schematic Editor" ) },
        { FRAME_PCB_EDITOR, MAIL_PCB_SAVE, _( "PCB Editor" ) },
    };

    std::vector<const DIRTY_EDITOR*> dirty;

    for( const DIRTY_EDITOR& d : candidates )
    {
        KIWAY_PLAYER* frame = aFrame->Kiway().Player( d.frameType, false );

        if( frame && frame->IsContentModified() )
            dirty.push_back( &d );
    }

    if( !dirty.empty() )
    {
        wxString listed;

        for( const DIRTY_EDITOR* d : dirty )
            listed += wxString::Format( wxS( "\n    - %s" ), d->label );

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
                                           "Save them before committing?" ),
                                        listed );
            style |= wxYES_NO | wxCANCEL;
        }

        wxRichMessageDialog dlg( aFrame, message, _( "Unsaved Changes" ), style );

        if( aPurpose == GIT_EDITOR_PREP::WORKING_TREE )
            dlg.SetYesNoLabels( _( "&Save and Continue" ), _( "Cancel" ) );
        else
            dlg.SetYesNoCancelLabels( _( "&Save and Commit" ), _( "Commit &Anyway" ), _( "Cancel" ) );

        int answer = dlg.ShowModal();

        if( answer == wxID_CANCEL )
            return false;

        // Rewriting the working tree offers no "continue anyway", so anything else is a cancel.
        if( aPurpose == GIT_EDITOR_PREP::WORKING_TREE && answer != wxID_YES )
            return false;

        if( answer == wxID_YES )
        {
            for( const DIRTY_EDITOR* d : dirty )
            {
                std::string payload;
                aFrame->Kiway().ExpressMail( d->frameType, d->saveMail, payload );

                if( payload != "success" )
                {
                    DisplayErrorMessage( aFrame, wxString::Format( _( "Could not save %s." ), d->label ) );
                    return false;
                }
            }

            // Saving editors can update the project file; a commit may include that change.
            // Commit Anyway leaves both the editors and the project file untouched.
            if( aPurpose == GIT_EDITOR_PREP::COMMIT )
                aFrame->GetSettingsManager()->SaveProject();
        }
    }

    return true;
}


GIT_DIFF_WORKSPACE::GIT_DIFF_WORKSPACE()
{
    // Create the scratch directory with a non-recursive Mkdir so its final component cannot be
    // an attacker-precreated symlink. Retry on collision rather than reusing a predictable name.
    const wxString base =
            wxStandardPaths::Get().GetTempDir() + wxFileName::GetPathSeparator()
            + wxString::Format( wxS( "kicad_gitdiff_%lu_" ), static_cast<unsigned long>( wxGetProcessId() ) );
    const unsigned millis = static_cast<unsigned>( wxGetLocalTimeMillis().GetLo() );

    for( int attempt = 0; attempt < 16 && m_root.IsEmpty(); ++attempt )
    {
        wxString candidate = base + wxString::Format( wxS( "%d_%u" ), attempt, millis );

        if( wxFileName::Mkdir( candidate, 0700, 0 ) )
            m_root = candidate;
    }
}


GIT_DIFF_WORKSPACE::~GIT_DIFF_WORKSPACE()
{
    if( !m_root.IsEmpty() && wxFileName::DirExists( m_root ) )
        wxFileName::Rmdir( m_root, wxPATH_RMDIR_RECURSIVE );
}


wxString GIT_DIFF_WORKSPACE::Materialize( git_repository* aRepo, const wxString& aRef, const wxString& aRelPath,
                                          KICAD_DIFF::DOC_KIND aKind )
{
    if( m_root.IsEmpty() )
        return wxEmptyString;

    const wxString sep = wxFileName::GetPathSeparator();

    // A schematic root sheet references its sub-sheet files by relative path, so check out the
    // whole tree and the sub-sheets come along. A board is self-contained, so write just the blob.
    if( aKind == KICAD_DIFF::DOC_KIND::SCH )
    {
        wxString dir = m_root + sep + wxString::Format( wxS( "s%d" ), m_slot++ );

        if( !KIGIT::PROJECT_GIT_UTILS::WriteTreeToDir( aRepo, aRef, dir ) )
            return wxEmptyString;

        wxString path = dir + sep + aRelPath;

        return wxFileName::FileExists( path ) ? path : wxString( wxEmptyString );
    }

    wxString path = m_root + sep + wxString::Format( wxS( "s%d_" ), m_slot++ ) + wxFileName( aRelPath ).GetFullName();

    if( !KIGIT::PROJECT_GIT_UTILS::WriteBlobToFile( aRepo, aRef, aRelPath, path ) )
        return wxEmptyString;

    return path;
}


static void showTextFileDiff( wxWindow* aParent, const wxString& aRelativePath, const wxString& aFileA,
                              const wxString& aFileB )
{
    auto readAll = []( const wxString& aPath ) -> std::string
    {
        if( aPath.IsEmpty() )
            return std::string();

        std::ifstream in( aPath.fn_str(), std::ios::binary );
        return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    };

    std::string contentA = readAll( aFileA );
    std::string contentB = readAll( aFileB );

    if( KIGIT::TextLooksBinary( contentA ) || KIGIT::TextLooksBinary( contentB ) )
    {
        DisplayInfoMessage( aParent, wxString::Format( _( "'%s' is not a plain text file. KiCad compares "
                                                          "design files and text files only, so there "
                                                          "is no comparison to show." ),
                                                       aRelativePath ) );
        return;
    }

    std::vector<KIGIT::TEXT_DIFF_LINE> lines =
            KIGIT::BuildTextDiff( KIGIT::SplitTextLines( contentA ), KIGIT::SplitTextLines( contentB ) );

    DIALOG_TEXT_DIFF dlg( aParent, aRelativePath, lines );
    dlg.ShowModal();
}


// Shared by the working-copy and history diffs. An empty path on either side means that side
// of the document does not exist.
static void presentFileDiff( KIWAY& aKiway, KICAD_DIFF::DOC_KIND aKind, const wxString& aRelativePath,
                             const wxString& aFileA, const wxString& aFileB, const wxString& aLabelA,
                             const wxString& aLabelB, wxWindow* aParent )
{
    if( aKind == KICAD_DIFF::DOC_KIND::UNKNOWN )
    {
        showTextFileDiff( aParent, aRelativePath, aFileA, aFileB );
        return;
    }

    WX_STRING_REPORTER reporter;
    KICAD_DIFF::DispatchOpenDiffDialog( aKiway, aKind, aFileA, aFileB, aLabelA, aLabelB, aParent, &reporter );

    if( reporter.HasMessage() )
        DisplayErrorMessage( aParent, _( "Could not open the diff." ), reporter.GetMessages() );
}


void ShowGitFileDiff( EDA_BASE_FRAME* aFrame, const wxString& aRelativePath )
{
    if( !aFrame->Prj().GetGitCommon()->GetRepo() )
        return;

    GIT_DIFF_WORKSPACE workspace;

    if( !workspace.IsOk() )
    {
        DisplayInfoMessage( aFrame, _( "Could not create a temporary directory for the diff." ) );
        return;
    }

    GIT_FILE_SIDES sides;

    if( !MaterializeWorkingFile( aFrame, workspace, aRelativePath, sides ) )
    {
        DisplayInfoMessage( aFrame, sides.error );
        return;
    }

    presentFileDiff( aFrame->Kiway(), sides.kind, aRelativePath, sides.before, sides.after, _( "BASE (HEAD)" ),
                     _( "WORKING (changes)" ), aFrame );
}


bool MaterializeWorkingFile( EDA_BASE_FRAME* aFrame, GIT_DIFF_WORKSPACE& aWorkspace, const wxString& aRelativePath,
                             GIT_FILE_SIDES& aSides )
{
    std::shared_ptr<KIGIT_COMMON> common = aFrame->Prj().GetGitCommon();

    aSides.kind = KICAD_DIFF::DocKindFromExtension( aRelativePath );

    bool inHead = true;

    {
        // Hold the git action lock only while reading, then release it before any modal dialog.
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            aSides.error = KIGIT::BusyMessage();
            return false;
        }

        git_repository* repo = common->GetRepo();

        if( !repo )
            return false;

        GIT_STATUS_HANDLER handler( common.get() );
        aSides.after = handler.GetWorkingDirectory() + aRelativePath;

        // A new file has no committed version, so that side stays empty and shows as added.
        if( common->IsPathInHead( aRelativePath ) )
            aSides.before = aWorkspace.Materialize( repo, wxT( "HEAD" ), aRelativePath, aSides.kind );
        else
            inHead = false;
    }

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
    std::shared_ptr<KIGIT_COMMON> common = aFrame->Prj().GetGitCommon();
    git_repository*               repo = common->GetRepo();

    if( !repo )
        return;

    GIT_DIFF_WORKSPACE workspace;

    if( !workspace.IsOk() )
    {
        DisplayInfoMessage( aFrame, _( "Could not create a temporary directory for the diff." ) );
        return;
    }

    std::vector<wxString> changedPaths;
    wxString              parentRef;
    bool                  busy = false;

    {
        // Hold the git action lock only while reading, then release it before any modal dialog.
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            busy = true;
        }
        else
        {
            // A root commit has no "<hash>^". An empty base ref compares it against the empty
            // tree, so its files show as added.
            git_object* commitObj = nullptr;

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
                changedPaths.push_back( file.path );
        }
    }

    if( busy )
    {
        DisplayInfoMessage( aFrame, KIGIT::BusyMessage() );
        return;
    }

    // A caller that named a file knows its own list, which for a stash also holds the files
    // that were untracked. Those are not in the diff above.
    if( aRelativePath.IsEmpty() && changedPaths.empty() )
    {
        DisplayInfoMessage( aFrame, _( "This commit has no file changes to show." ) );
        return;
    }

    wxWindow* diffParent = aParentWindow ? aParentWindow : static_cast<wxWindow*>( aFrame );

    // The diff viewer shows one file at a time, so let the user pick which one unless the
    // caller already did.
    wxString chosen = aRelativePath.IsEmpty() ? changedPaths.front() : aRelativePath;

    if( aRelativePath.IsEmpty() && changedPaths.size() > 1 )
    {
        wxArrayString choices;

        for( const wxString& path : changedPaths )
            choices.Add( path );

        int sel = wxGetSingleChoiceIndex( _( "This commit changed more than one file. "
                                             "Select one to compare:" ),
                                          _( "Select File to Compare" ), choices, diffParent );

        if( sel < 0 )
            return;

        chosen = changedPaths[sel];
    }

    GIT_FILE_SIDES sides;

    if( !MaterializeCommitFile( aFrame, workspace, aCommitHash, chosen, sides ) )
    {
        if( !sides.error.IsEmpty() )
            DisplayInfoMessage( diffParent, sides.error );

        return;
    }

    presentFileDiff( aFrame->Kiway(), sides.kind, chosen, sides.before, sides.after,
                     wxString::Format( _( "BASE (parent of %s)" ), aCommitHash ),
                     wxString::Format( _( "WORKING (%s)" ), aCommitHash ), diffParent );
}


bool MaterializeCommitFile( EDA_BASE_FRAME* aFrame, GIT_DIFF_WORKSPACE& aWorkspace, const wxString& aCommitHash,
                            const wxString& aRelativePath, GIT_FILE_SIDES& aSides )
{
    std::shared_ptr<KIGIT_COMMON> common = aFrame->Prj().GetGitCommon();
    std::unique_lock<std::mutex>  lock = common->TryLockAction();

    if( !lock.owns_lock() )
    {
        aSides.error = KIGIT::BusyMessage();
        return false;
    }

    // The caller may have run a chooser since it last looked, so the project may have closed.
    git_repository* repo = common->GetRepo();

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

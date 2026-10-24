/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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

#include <stack>
#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/git_transfer.h>
#include <git/git_backend.h>

#include <wx/regex.h>
#include <wx/richmsgdlg.h>
#include <wx/stdpaths.h>
#include <wx/string.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/timer.h>
#include <wx/wupdlock.h>
#include <wx/log.h>
#include <wx/dcclient.h>
#include <wx/progdlg.h>
#include <wx/settings.h>
#include <wx/weakref.h>

#include <frame_type.h>
#include <kiway.h>
#include <mail_type.h>
#include <settings/settings_manager.h>

#include <settings/common_settings.h>
#include <advanced_config.h>
#include <bitmaps.h>
#include <bitmap_store.h>
#include <confirm.h>
#include <dialogs/git/dialog_git_credentials.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <dialogs/git/git_ui_utils.h>
#include <widgets/wx_progress_reporters.h>
#include <dialogs/git/dialog_git_switch.h>
#include <gestfich.h>
#include <macros.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>
#include <kiplatform/environment.h>
#include <core/kicad_algo.h>
#include <paths.h>
#include <project/project_local_settings.h>
#include <scoped_set_reset.h>
#include <string_utils.h>
#include <thread_pool.h>
#include <launch_ext.h>
#include <wx_log_utils.h>

#include <git/git_pull_handler.h>
#include <git/kicad_git_compat.h>
#include <git/git_init_handler.h>
#include <git/git_status_handler.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/project_git_utils.h>

#include <dialogs/git/dialog_git_repository.h>

#include "project_tree_item.h"
#include "project_tree.h"
#include "pgm_kicad.h"
#include "kicad_id.h"
#include "kicad_manager_frame.h"

#include "project_tree_pane.h"
#include <widgets/kistatusbar.h>

#include <kiplatform/io.h>
#include <kiplatform/secrets.h>


/* Note about the project tree build process:
 * Building the project tree can be *very* long if there are a lot of subdirectories in the
 * working directory.  Unfortunately, this happens easily if the project file *.pro is in the
 * user's home directory.
 * So the tree project is built "on demand":
 * First the tree is built from the current directory and shows files and subdirs.
 *   > First level subdirs trees are built (i.e subdirs contents are not read)
 *   > When expanding a subdir, each subdir contains is read, and the corresponding sub tree is
 *     populated on the fly.
 */

// The pool's future does not block on destruction, so replacing one still running would abandon
// the task rather than wait for it.
static bool gitTaskStillRunning( const std::future<void>& aTask )
{
    return aTask.valid() && aTask.wait_for( std::chrono::seconds( 0 ) ) != std::future_status::ready;
}


// list of files extensions listed in the tree project window
// Add extensions in a compatible regex format to see others files types
static const wxChar* s_allowedExtensionsToList[] =
{
    wxT( "^.*\\.pro$" ),
    wxT( "^.*\\.kicad_pro$" ),
    wxT( "^.*\\.pdf$" ),
    wxT( "^.*\\.sch$" ),           // Legacy Eeschema files
    wxT( "^.*\\.kicad_sch$" ),     // S-expr Eeschema files
    wxT( "^[^$].*\\.brd$" ),       // Legacy Pcbnew files
    wxT( "^[^$].*\\.kicad_pcb$" ), // S format Pcbnew board files
    wxT( "^[^$].*\\.kicad_dru$" ), // Design rule files
    wxT( "^[^$].*\\.kicad_wks$" ), // S format kicad drawing sheet files
    wxT( "^[^$].*\\.kicad_mod$" ), // S format kicad footprint files, currently not listed
    wxT( "^.*\\.net$" ),           // pcbnew netlist file
    wxT( "^.*\\.cir$" ),           // Spice netlist file
    wxT( "^.*\\.lib$" ),           // Legacy schematic library file
    wxT( "^.*\\.kicad_sym$" ),     // S-expr symbol libraries
    wxT( "^.*\\.txt$" ),           // Text files
    wxT( "^.*\\.md$" ),            // Markdown files
    wxT( "^.*\\.pho$" ),           // Gerber file (Old Kicad extension)
    wxT( "^.*\\.gbr$" ),           // Gerber file
    wxT( "^.*\\.gbrjob$" ),        // Gerber job file
    wxT( "^.*\\.gb[alops]$" ),     // Gerber back (or bottom) layer file (deprecated Protel ext)
    wxT( "^.*\\.gt[alops]$" ),     // Gerber front (or top) layer file (deprecated Protel ext)
    wxT( "^.*\\.g[0-9]{1,2}$" ),   // Gerber inner layer file (deprecated Protel ext)
    wxT( "^.*\\.gm[0-9]{1,2}$" ),  // Gerber mechanical layer file (deprecated Protel ext)
    wxT( "^.*\\.gko$" ),           // Gerber keepout layer file (deprecated Protel ext)
    wxT( "^.*\\.odt$" ),
    wxT( "^.*\\.htm$" ),
    wxT( "^.*\\.html$" ),
    wxT( "^.*\\.rpt$" ),           // Report files
    wxT( "^.*\\.csv$" ),           // Report files in comma separated format
    wxT( "^.*\\.pos$" ),           // Footprint position files
    wxT( "^.*\\.cmp$" ),           // CvPcb cmp/footprint link files
    wxT( "^.*\\.drl$" ),           // Excellon drill files
    wxT( "^.*\\.nc$" ),            // Excellon NC drill files (alternate file ext)
    wxT( "^.*\\.xnc$" ),           // Excellon NC drill files (alternate file ext)
    wxT( "^.*\\.svg$" ),           // SVG print/plot files
    wxT( "^.*\\.ps$" ),            // PostScript plot files
    wxT( "^.*\\.zip$" ),           // Zip archive files
    wxT( "^.*\\.kicad_jobset" ),   // KiCad jobs file
    nullptr                        // end of list
};


/**
 * The frame that shows the tree list of files and subdirectories inside the working directory.
 *
 * Files are filtered (see s_allowedExtensionsToList) so only useful files are shown.
 */


enum project_tree_ids
{
    ID_PROJECT_TXTEDIT = 8700, // Start well above wxIDs
    ID_PROJECT_SWITCH_TO_OTHER,
    ID_PROJECT_NEWDIR,
    ID_PROJECT_OPEN_DIR,
    ID_PROJECT_DELETE,
    ID_PROJECT_RENAME,

    ID_GIT_INITIALIZE_PROJECT, // Initialize a new git repository in an existing project
    ID_GIT_REMOTE_SETTINGS,    // Configure (or set) the default remote on an existing repo
    ID_GIT_REMOVE_VCS,         // Toggle Git integration for this project (preference in kicad_prl)
    ID_GIT_SWITCH_BRANCH,      // Switch the local repository to a different branch
    ID_GIT_SWITCH_QUICK1,      // Switch the local repository to the first quick branch
    ID_GIT_SWITCH_QUICK2,      // Switch the local repository to the second quick branch
    ID_GIT_SWITCH_QUICK3,      // Switch the local repository to the third quick branch
    ID_GIT_SWITCH_QUICK4,      // Switch the local repository to the fourth quick branch
    ID_GIT_SWITCH_QUICK5,      // Switch the local repository to the fifth quick branch

    ID_JOBS_RUN,
};


BEGIN_EVENT_TABLE( PROJECT_TREE_PANE, wxSashLayoutWindow )
    EVT_TREE_ITEM_ACTIVATED( ID_PROJECT_TREE, PROJECT_TREE_PANE::onSelect )
    EVT_TREE_ITEM_EXPANDED( ID_PROJECT_TREE, PROJECT_TREE_PANE::onExpand )
    EVT_TREE_ITEM_RIGHT_CLICK( ID_PROJECT_TREE, PROJECT_TREE_PANE::onRight )
    EVT_MENU( ID_PROJECT_TXTEDIT, PROJECT_TREE_PANE::onOpenSelectedFileWithTextEditor )
    EVT_MENU( ID_PROJECT_SWITCH_TO_OTHER, PROJECT_TREE_PANE::onSwitchToSelectedProject )
    EVT_MENU( ID_PROJECT_NEWDIR, PROJECT_TREE_PANE::onCreateNewDirectory )
    EVT_MENU( ID_PROJECT_OPEN_DIR, PROJECT_TREE_PANE::onOpenDirectory )
    EVT_MENU( ID_PROJECT_DELETE, PROJECT_TREE_PANE::onDeleteFile )
    EVT_MENU( ID_PROJECT_RENAME, PROJECT_TREE_PANE::onRenameFile )

    EVT_MENU( ID_GIT_INITIALIZE_PROJECT, PROJECT_TREE_PANE::onGitInitializeProject )
    EVT_MENU( ID_GIT_REMOTE_SETTINGS, PROJECT_TREE_PANE::onGitRemoteSettings )
    EVT_MENU( ID_GIT_SWITCH_BRANCH, PROJECT_TREE_PANE::onGitSwitchBranch )
    EVT_MENU_RANGE( ID_GIT_SWITCH_QUICK1, ID_GIT_SWITCH_QUICK5, PROJECT_TREE_PANE::onGitSwitchBranch )
    EVT_MENU( ID_GIT_REMOVE_VCS, PROJECT_TREE_PANE::onGitRemoveVCS )

    EVT_MENU( ID_JOBS_RUN, PROJECT_TREE_PANE::onRunSelectedJobsFile )

    EVT_IDLE( PROJECT_TREE_PANE::onIdle )
    EVT_PAINT( PROJECT_TREE_PANE::onPaint )
END_EVENT_TABLE()


wxDECLARE_EVENT( UPDATE_ICONS, wxCommandEvent );

PROJECT_TREE_PANE::PROJECT_TREE_PANE( KICAD_MANAGER_FRAME* parent ) :
        wxSashLayoutWindow( parent, ID_LEFT_FRAME, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxTAB_TRAVERSAL ),
        m_branchQuery( std::make_unique<KIGIT_QUERY>() )
{
    m_Parent = parent;
    m_TreeProject = nullptr;
    m_isRenaming = false;
    m_selectedItem = nullptr;
    m_watcherNeedReset = false;
    m_watcher = nullptr;
    m_gitIconsInitialized = false;

    Bind( wxEVT_FSWATCHER,
             wxFileSystemWatcherEventHandler( PROJECT_TREE_PANE::onFileSystemEvent ), this );

    Bind( wxEVT_SYS_COLOUR_CHANGED,
          wxSysColourChangedEventHandler( PROJECT_TREE_PANE::onThemeChanged ), this );

    m_gitSyncTimer.SetOwner( this );
    m_gitStatusTimer.SetOwner( this );
    Bind( wxEVT_TIMER, wxTimerEventHandler( PROJECT_TREE_PANE::onGitSyncTimer ), this, m_gitSyncTimer.GetId() );
    Bind( wxEVT_TIMER, wxTimerEventHandler( PROJECT_TREE_PANE::onGitStatusTimer ), this, m_gitStatusTimer.GetId() );
    /*
     * Filtering is now inverted: the filters are actually used to _enable_ support
     * for a given file type.
     */
    for( int ii = 0; s_allowedExtensionsToList[ii] != nullptr; ii++ )
        m_filters.emplace_back( s_allowedExtensionsToList[ii] );

    m_filters.emplace_back( wxT( "^no KiCad files found" ) );

    ReCreateTreePrj();
}


PROJECT_TREE_PANE::~PROJECT_TREE_PANE()
{
    m_branchQuery->Cancel();

    Unbind( wxEVT_FSWATCHER,
            wxFileSystemWatcherEventHandler( PROJECT_TREE_PANE::onFileSystemEvent ), this );
    Unbind( wxEVT_SYS_COLOUR_CHANGED,
            wxSysColourChangedEventHandler( PROJECT_TREE_PANE::onThemeChanged ), this );

    m_gitSyncTimer.Stop();
    m_gitStatusTimer.Stop();
    Unbind( wxEVT_TIMER, wxTimerEventHandler( PROJECT_TREE_PANE::onGitSyncTimer ), this, m_gitSyncTimer.GetId() );
    Unbind( wxEVT_TIMER, wxTimerEventHandler( PROJECT_TREE_PANE::onGitStatusTimer ), this, m_gitStatusTimer.GetId() );
    shutdownFileWatcher();

    if( std::shared_ptr<KIGIT_COMMON> common = m_gitListenerSession.lock() )
        common->RemoveRefreshListener( this );

    if( m_gitSyncTask.valid() )
        m_gitSyncTask.wait();

    if( m_gitStatusIconTask.valid() )
        m_gitStatusIconTask.wait();
}


void PROJECT_TREE_PANE::shutdownFileWatcher()
{
    if( m_watcher )
    {
        m_watcher->RemoveAll();
        m_watcher->SetOwner( nullptr );
        delete m_watcher;
        m_watcher = nullptr;
    }
}


void PROJECT_TREE_PANE::onSwitchToSelectedProject( wxCommandEvent& event )
{
    std::vector<PROJECT_TREE_ITEM*> tree_data = GetSelectedData();

    if( tree_data.size() != 1 )
        return;

    wxString prj_filename = tree_data[0]->GetFileName();

    m_Parent->LoadProject( prj_filename );
}


void PROJECT_TREE_PANE::onOpenDirectory( wxCommandEvent& event )
{
    // Get the root directory name:
    std::vector<PROJECT_TREE_ITEM*> tree_data = GetSelectedData();

    for( PROJECT_TREE_ITEM* item_data : tree_data )
    {
        // Ask for the new sub directory name
        wxString curr_dir = item_data->GetDir();

        if( curr_dir.IsEmpty() )
        {
            // Use project path if the tree view path was empty.
            curr_dir = wxPathOnly( m_Parent->GetProjectFileName() );

            // As a last resort use the user's documents folder.
            if( curr_dir.IsEmpty() || !wxFileName::DirExists( curr_dir ) )
                curr_dir = PATHS::GetDefaultUserProjectsPath();

            if( !curr_dir.IsEmpty() )
                curr_dir += wxFileName::GetPathSeparator();
        }

        LaunchExternal( curr_dir );
    }
}


void PROJECT_TREE_PANE::onCreateNewDirectory( wxCommandEvent& event )
{
    // Get the root directory name:
    std::vector<PROJECT_TREE_ITEM*> tree_data = GetSelectedData();

    for( PROJECT_TREE_ITEM* item_data : tree_data )
    {
        wxString prj_dir = wxPathOnly( m_Parent->GetProjectFileName() );

        // Ask for the new sub directory name
        wxString curr_dir = item_data->GetDir();

        if( curr_dir.IsEmpty() )
            curr_dir = prj_dir;

        wxString new_dir = wxGetTextFromUser( _( "Directory name:" ), _( "Create New Directory" ) );

        if( new_dir.IsEmpty() )
            return;

        wxString full_dirname = curr_dir + wxFileName::GetPathSeparator() + new_dir;

        if( !wxMkdir( full_dirname ) )
            return;

        addItemToProjectTree( full_dirname, item_data->GetId(), nullptr, false );
    }
}


wxString PROJECT_TREE_PANE::GetFileExt( TREE_FILE_TYPE type )
{
    switch( type )
    {
    case TREE_FILE_TYPE::LEGACY_PROJECT:        return FILEEXT::LegacyProjectFileExtension;
    case TREE_FILE_TYPE::JSON_PROJECT:          return FILEEXT::ProjectFileExtension;
    case TREE_FILE_TYPE::LEGACY_SCHEMATIC:      return FILEEXT::LegacySchematicFileExtension;
    case TREE_FILE_TYPE::SEXPR_SCHEMATIC:       return FILEEXT::KiCadSchematicFileExtension;
    case TREE_FILE_TYPE::LEGACY_PCB:            return FILEEXT::LegacyPcbFileExtension;
    case TREE_FILE_TYPE::SEXPR_PCB:             return FILEEXT::KiCadPcbFileExtension;
    case TREE_FILE_TYPE::GERBER:                return FILEEXT::GerberFileExtensionsRegex;
    case TREE_FILE_TYPE::GERBER_JOB_FILE:       return FILEEXT::GerberJobFileExtension;
    case TREE_FILE_TYPE::HTML:                  return FILEEXT::HtmlFileExtension;
    case TREE_FILE_TYPE::PDF:                   return FILEEXT::PdfFileExtension;
    case TREE_FILE_TYPE::TXT:                   return FILEEXT::TextFileExtension;
    case TREE_FILE_TYPE::MD:                    return FILEEXT::MarkdownFileExtension;
    case TREE_FILE_TYPE::NET:                   return FILEEXT::NetlistFileExtension;
    case TREE_FILE_TYPE::NET_SPICE:             return FILEEXT::SpiceFileExtension;
    case TREE_FILE_TYPE::CMP_LINK:              return FILEEXT::FootprintAssignmentFileExtension;
    case TREE_FILE_TYPE::REPORT:                return FILEEXT::ReportFileExtension;
    case TREE_FILE_TYPE::FP_PLACE:              return FILEEXT::FootprintPlaceFileExtension;
    case TREE_FILE_TYPE::DRILL:                 return FILEEXT::DrillFileExtension;
    case TREE_FILE_TYPE::DRILL_NC:              return "nc";
    case TREE_FILE_TYPE::DRILL_XNC:             return "xnc";
    case TREE_FILE_TYPE::SVG:                   return FILEEXT::SVGFileExtension;
    case TREE_FILE_TYPE::CSV:                   return FILEEXT::CsvFileExtension;
    case TREE_FILE_TYPE::DRAWING_SHEET:         return FILEEXT::DrawingSheetFileExtension;
    case TREE_FILE_TYPE::FOOTPRINT_FILE:        return FILEEXT::KiCadFootprintFileExtension;
    case TREE_FILE_TYPE::SCHEMATIC_LIBFILE:     return FILEEXT::LegacySymbolLibFileExtension;
    case TREE_FILE_TYPE::SEXPR_SYMBOL_LIB_FILE: return FILEEXT::KiCadSymbolLibFileExtension;
    case TREE_FILE_TYPE::DESIGN_RULES:          return FILEEXT::DesignRulesFileExtension;
    case TREE_FILE_TYPE::ZIP_ARCHIVE:           return FILEEXT::ArchiveFileExtension;
    case TREE_FILE_TYPE::JOBSET_FILE:           return FILEEXT::KiCadJobSetFileExtension;

    case TREE_FILE_TYPE::ROOT:
    case TREE_FILE_TYPE::UNKNOWN:
    case TREE_FILE_TYPE::MAX:
    case TREE_FILE_TYPE::DIRECTORY:             break;
    }

    return wxEmptyString;
}


std::vector<wxString> getProjects( const wxDir& dir )
{
    std::vector<wxString> projects;
    wxString              dir_filename;
    bool                  haveFile = dir.GetFirst( &dir_filename );

    while( haveFile  )
    {
        wxFileName file( dir_filename );

        if( file.GetExt() == FILEEXT::LegacyProjectFileExtension
            || file.GetExt() == FILEEXT::ProjectFileExtension )
            projects.push_back( file.GetName() );

        haveFile = dir.GetNext( &dir_filename );
    }

    return projects;
}





wxTreeItemId PROJECT_TREE_PANE::addItemToProjectTree( const wxString& aName,
                                                      const wxTreeItemId& aParent,
                                                      std::vector<wxString>* aProjectNames,
                                                      bool aRecurse )
{
    TREE_FILE_TYPE type = TREE_FILE_TYPE::UNKNOWN;
    wxFileName     fn( aName );

    if( KIPLATFORM::IO::IsFileHidden( aName ) )
        return wxTreeItemId();

    // Windows only hides .git when it carries the hidden attribute
    if( fn.GetFullName().IsSameAs( wxS( ".git" ), false ) )
        return wxTreeItemId();

    if( wxDirExists( aName ) )
    {
        type = TREE_FILE_TYPE::DIRECTORY;
    }
    else
    {
        // Filter
        wxRegEx reg;
        bool    addFile = false;

        for( const wxString& m_filter : m_filters )
        {
            wxCHECK2_MSG( reg.Compile( m_filter, wxRE_ICASE ), continue,
                          wxString::Format( "Regex %s failed to compile.", m_filter ) );

            if( reg.Matches( aName ) )
            {
                addFile = true;
                break;
            }
        }

        if( !addFile )
            return wxTreeItemId();

        for( int i = static_cast<int>( TREE_FILE_TYPE::LEGACY_PROJECT );
                i < static_cast<int>( TREE_FILE_TYPE::MAX ); i++ )
        {
            wxString ext = GetFileExt( (TREE_FILE_TYPE) i );

            if( ext == wxT( "" ) )
                continue;

            if( reg.Compile( wxString::FromAscii( "^.*\\." ) + ext + wxString::FromAscii( "$" ), wxRE_ICASE )
                    && reg.Matches( aName ) )
            {
                type = (TREE_FILE_TYPE) i;
                break;
            }
        }
    }

    wxString   file = wxFileNameFromPath( aName );
    wxFileName currfile( file );
    wxFileName project( m_Parent->GetProjectFileName() );
    bool       showAllSchematics = gitRepo() != nullptr;

    // Ignore legacy projects with the same name as the current project
    if( ( type == TREE_FILE_TYPE::LEGACY_PROJECT )
            && ( currfile.GetName().CmpNoCase( project.GetName() ) == 0 ) )
    {
        return wxTreeItemId();
    }

    if( !showAllSchematics && ( currfile.GetExt() == GetFileExt( TREE_FILE_TYPE::LEGACY_SCHEMATIC )
                                || currfile.GetExt() == GetFileExt( TREE_FILE_TYPE::SEXPR_SCHEMATIC ) ) )
    {
        if( aProjectNames )
        {
            if( !alg::contains( *aProjectNames, currfile.GetName() ) )
                return wxTreeItemId();
        }
        else
        {
            PROJECT_TREE_ITEM* parentTreeItem = GetItemIdData( aParent );

            if( !parentTreeItem )
                return wxTreeItemId();

            wxDir                 parentDir( parentTreeItem->GetDir() );
            std::vector<wxString> projects = getProjects( parentDir );

            if( !alg::contains( projects, currfile.GetName() ) )
                return wxTreeItemId();
        }
    }

    // also check to see if it is already there.
    wxTreeItemIdValue cookie;
    wxTreeItemId      kid = m_TreeProject->GetFirstChild( aParent, cookie );

    while( kid.IsOk() )
    {
        PROJECT_TREE_ITEM* itemData = GetItemIdData( kid );

        if( itemData && itemData->GetFileName() == aName )
            return itemData->GetId();    // well, we would have added it, but it is already here!

        kid = m_TreeProject->GetNextChild( aParent, cookie );
    }

    // Only show current files if both legacy and current files are present
    if( type == TREE_FILE_TYPE::LEGACY_PROJECT
            || type == TREE_FILE_TYPE::JSON_PROJECT
            || type == TREE_FILE_TYPE::LEGACY_SCHEMATIC
            || type == TREE_FILE_TYPE::SEXPR_SCHEMATIC )
    {
        kid = m_TreeProject->GetFirstChild( aParent, cookie );

        while( kid.IsOk() )
        {
            PROJECT_TREE_ITEM* itemData = GetItemIdData( kid );

            if( itemData )
            {
                wxFileName fname( itemData->GetFileName() );

                if( fname.GetName().CmpNoCase( currfile.GetName() ) == 0 )
                {
                    switch( type )
                    {
                    case TREE_FILE_TYPE::LEGACY_PROJECT:
                        if( itemData->GetType() == TREE_FILE_TYPE::JSON_PROJECT )
                            return wxTreeItemId();

                        break;

                    case TREE_FILE_TYPE::LEGACY_SCHEMATIC:
                        if( itemData->GetType() == TREE_FILE_TYPE::SEXPR_SCHEMATIC )
                            return wxTreeItemId();

                        break;

                    case TREE_FILE_TYPE::JSON_PROJECT:
                        if( itemData->GetType() == TREE_FILE_TYPE::LEGACY_PROJECT )
                            m_TreeProject->Delete( kid );

                        break;

                    case TREE_FILE_TYPE::SEXPR_SCHEMATIC:
                        if( itemData->GetType() == TREE_FILE_TYPE::LEGACY_SCHEMATIC )
                            m_TreeProject->Delete( kid );

                        break;

                    default:
                        break;
                    }
                }
            }

            kid = m_TreeProject->GetNextChild( aParent, cookie );
        }
    }

    // Append the item (only appending the filename not the full path):
    wxTreeItemId       newItemId = m_TreeProject->AppendItem( aParent, file );
    PROJECT_TREE_ITEM* data = new PROJECT_TREE_ITEM( type, aName, m_TreeProject );

    m_TreeProject->SetItemData( newItemId, data );
    data->SetState( 0 );

    // Mark root files (files which have the same aName as the project)
    wxString fileName = currfile.GetName().Lower();
    wxString projName = project.GetName().Lower();

    if( fileName == projName || fileName.StartsWith( projName + "-" ) )
        data->SetRootFile( true );

#ifndef __WINDOWS__
    bool subdir_populated = false;
#endif

    // This section adds dirs and files found in the subdirs
    // in this case AddFile is recursive, but for the first level only.
    if( TREE_FILE_TYPE::DIRECTORY == type && aRecurse )
    {
        wxDir dir( aName );

        if( dir.IsOpened() )    // protected dirs will not open properly.
        {
            std::vector<wxString> projects = getProjects( dir );
            wxString              dir_filename;
            bool                  haveFile = dir.GetFirst( &dir_filename );

            data->SetPopulated( true );

#ifndef __WINDOWS__
            subdir_populated = aRecurse;
#endif

            while( haveFile  )
            {
                // Add name in tree, but do not recurse
                wxString path = aName + wxFileName::GetPathSeparator() + dir_filename;
                addItemToProjectTree( path, newItemId, &projects, false );

                haveFile = dir.GetNext( &dir_filename );
            }
        }

        // Sort filenames by alphabetic order
        m_TreeProject->SortChildren( newItemId );
    }

#ifndef __WINDOWS__
    if( subdir_populated )
        m_watcherNeedReset = true;
#endif

    return newItemId;
}


void PROJECT_TREE_PANE::ReCreateTreePrj()
{
    std::lock_guard<std::mutex> lock1( m_gitStatusMutex );
    std::lock_guard<std::mutex> lock2( m_gitTreeCacheMutex );

    m_gitStatusTimer.Stop();
    m_gitSyncTimer.Stop();

    m_gitTreeCache.clear();
    m_gitStatusIcons.clear();

    wxString pro_dir = m_Parent->GetProjectFileName();

    if( !m_TreeProject )
        m_TreeProject = new PROJECT_TREE( this );
    else
        m_TreeProject->DeleteAllItems();

    if( !pro_dir )  // This is empty from PROJECT_TREE_PANE constructor
        return;

    // Releasing here would cancel a push or pull running from an editor panel on every tree
    // refresh, and a project switch already disposes of the session.
    if( gitRepo() )
    {
        if( !Pgm().GetCommonSettings()->m_Git.enableGit )
            releaseGitRepo();

        m_gitIconsInitialized = false;
    }

    wxFileName fn = pro_dir;
    bool prjReset = false;

    if( !fn.IsOk() )
    {
        fn.Clear();
        fn.SetPath( PATHS::GetDefaultUserProjectsPath() );
        fn.SetName( NAMELESS_PROJECT );
        fn.SetExt( FILEEXT::ProjectFileExtension );
        prjReset = true;
    }

    bool prjOpened = fn.FileExists();

    if( Pgm().GetCommonSettings()->m_Git.enableGit )
    {
        Prj().AttachGitRepository();
        syncGitListener();
    }

    // We may have opened a legacy project, in which case GetProjectFileName will return the
    // name of the migrated (new format) file, which may not have been saved to disk yet.
    if( !prjOpened && !prjReset )
    {
        fn.SetExt( FILEEXT::LegacyProjectFileExtension );
        prjOpened = fn.FileExists();

        // Set the ext back so that in the tree view we see the (not-yet-saved) new file
        fn.SetExt( FILEEXT::ProjectFileExtension );
    }

    // root tree:
    m_root = m_TreeProject->AddRoot( fn.GetFullName(), static_cast<int>( TREE_FILE_TYPE::ROOT ),
                                     static_cast<int>( TREE_FILE_TYPE::ROOT ) );
    m_TreeProject->SetItemBold( m_root, true );

    // The main project file is now a JSON file
    PROJECT_TREE_ITEM* data = new PROJECT_TREE_ITEM( TREE_FILE_TYPE::JSON_PROJECT, fn.GetFullPath(),
                                                     m_TreeProject );

    m_TreeProject->SetItemData( m_root, data );

    // Now adding all current files if available
    if( prjOpened )
    {
        pro_dir = wxPathOnly( m_Parent->GetProjectFileName() );
        wxDir dir( pro_dir );

        if( dir.IsOpened() )    // protected dirs will not open, see "man opendir()"
        {
            std::vector<wxString> projects = getProjects( dir );
            wxString              filename;
            bool                  haveFile = dir.GetFirst( &filename );

            while( haveFile )
            {
                if( filename != fn.GetFullName() )
                {
                    wxString name = dir.GetName() + wxFileName::GetPathSeparator() + filename;

                    // Add items living in the project directory, and populate the item
                    // if it is a directory (sub directories will be not populated)
                    addItemToProjectTree( name, m_root, &projects, true );
                }

                haveFile = dir.GetNext( &filename );
            }
        }
    }
    else
    {
        m_TreeProject->AppendItem( m_root, wxT( "Empty project" ) );
    }

    m_TreeProject->Expand( m_root );

    // Sort filenames by alphabetic order
    m_TreeProject->SortChildren( m_root );

    CallAfter(
            [this] ()
            {
                wxLogTrace( traceGit, "PROJECT_TREE_PANE::ReCreateTreePrj: starting timers" );
                m_gitSyncTimer.Start( 100, wxTIMER_ONE_SHOT );
                m_gitStatusTimer.Start( 500, wxTIMER_ONE_SHOT );
            } );
}


void PROJECT_TREE_PANE::onRight( wxTreeEvent& Event )
{
    wxTreeItemId curr_item = Event.GetItem();

    // Ensure item is selected (Under Windows right click does not select the item)
    m_TreeProject->SelectItem( curr_item );

    // The tree control still uses the clicked item after this returns, so a menu command
    // that closes the project must not run inside the click.
    CallAfter(
            [this]
            {
                showContextMenu();
            } );
}


void PROJECT_TREE_PANE::showContextMenu()
{
    std::vector<PROJECT_TREE_ITEM*> selection = GetSelectedData();

    bool can_switch_to_project = true;
    bool can_create_new_directory = true;
    bool can_open_this_directory = true;
    bool can_edit = true;
    bool can_rename = true;
    bool can_delete = true;
    bool run_jobs = false;

    bool gitIntegrationDisabled = Prj().GetLocalSettings().m_GitIntegrationDisabled;
    bool vcs_enabled = !gitIntegrationDisabled;
    bool vcs_has_repo = gitRepo() != nullptr;
    bool vcs_can_init = vcs_enabled && !vcs_has_repo;
    bool vcs_can_set_remote = vcs_enabled && vcs_has_repo;
    // Disable/Enable is a per-project preference toggle, so it's available whenever we
    // detected a repository for this project or integration is currently disabled.
    bool vcs_can_remove = vcs_has_repo || gitIntegrationDisabled;
    bool vcs_can_switch = vcs_enabled && vcs_has_repo;
    bool vcs_menu        = Pgm().GetCommonSettings()->m_Git.enableGit;

    // Check if the libgit2 library is available via backend
    bool libgit_init = GetGitBackend() && GetGitBackend()->IsLibraryAvailable();

    vcs_menu &= libgit_init;

    if( selection.size() == 0 )
        return;

    // Remove things that don't make sense for multiple selections
    if( selection.size() != 1 )
    {
        can_switch_to_project = false;
        can_create_new_directory = false;
        can_rename = false;
    }

    for( PROJECT_TREE_ITEM* item : selection )
    {
        // Check for empty project
        if( !item )
        {
            can_switch_to_project = false;
            can_edit = false;
            can_rename = false;
            continue;
        }

        can_delete = item->CanDelete();
        can_rename = item->CanRename();

        switch( item->GetType() )
        {
        case TREE_FILE_TYPE::JSON_PROJECT:
        case TREE_FILE_TYPE::LEGACY_PROJECT:
            can_rename = false;

            if( item->GetId() == m_TreeProject->GetRootItem() )
            {
                can_switch_to_project = false;
            }
            else
            {
                can_create_new_directory = false;
                can_open_this_directory = false;
            }
            break;

        case TREE_FILE_TYPE::DIRECTORY:
            can_switch_to_project = false;
            can_edit = false;
            break;

        case TREE_FILE_TYPE::ZIP_ARCHIVE:
        case TREE_FILE_TYPE::PDF:
            can_edit = false;
            can_switch_to_project = false;
            can_create_new_directory = false;
            can_open_this_directory = false;
            break;

        case TREE_FILE_TYPE::JOBSET_FILE:
            run_jobs = true;
            can_edit = false;
            KI_FALLTHROUGH;

        case TREE_FILE_TYPE::SEXPR_SCHEMATIC:
        case TREE_FILE_TYPE::SEXPR_PCB:
            KI_FALLTHROUGH;

        default:
            can_switch_to_project = false;
            can_create_new_directory = false;
            can_open_this_directory = false;

            break;
        }
    }

    wxMenu   popup_menu;
    wxString text;
    wxString help_text;

    if( can_switch_to_project )
    {
        KIUI::AddMenuItem( &popup_menu, ID_PROJECT_SWITCH_TO_OTHER, _( "Switch to this Project" ),
                           _( "Close all editors, and switch to the selected project" ),
                           KiBitmap( BITMAPS::open_project ) );
        popup_menu.AppendSeparator();
    }

    if( can_create_new_directory )
    {
        KIUI::AddMenuItem( &popup_menu, ID_PROJECT_NEWDIR, _( "New Directory..." ),
                           _( "Create a New Directory" ), KiBitmap( BITMAPS::directory ) );
    }

    if( can_open_this_directory )
    {
        if( selection.size() == 1 )
        {
#ifdef __APPLE__
            text = _( "Reveal in Finder" );
            help_text = _( "Reveals the directory in a Finder window" );
#else
            text = _( "Open Directory in File Explorer" );
            help_text = _( "Opens the directory in the default system file manager" );
#endif
        }
        else
        {
#ifdef __APPLE__
            text = _( "Reveal in Finder" );
            help_text = _( "Reveals the directories in a Finder window" );
#else
            text = _( "Open Directories in File Explorer" );
            help_text = _( "Opens the directories in the default system file manager" );
#endif
        }

        KIUI::AddMenuItem( &popup_menu, ID_PROJECT_OPEN_DIR, text, help_text,
                           KiBitmap( BITMAPS::directory_browser ) );
    }

    if( can_edit )
    {
        if( selection.size() == 1 )
            help_text = _( "Open the file in a Text Editor" );
        else
            help_text = _( "Open files in a Text Editor" );

        KIUI::AddMenuItem( &popup_menu, ID_PROJECT_TXTEDIT, _( "Edit in a Text Editor" ), help_text,
                           KiBitmap( BITMAPS::editor ) );
    }

    if( run_jobs && selection.size() == 1 )
    {
        KIUI::AddMenuItem( &popup_menu, ID_JOBS_RUN, _( "Run Jobs" ), help_text,
                           KiBitmap( BITMAPS::exchange ) );
    }

    if( can_rename )
    {
        if( selection.size() == 1 )
        {
            text = _( "Rename File..." );
            help_text = _( "Rename file" );
        }
        else
        {
            text = _( "Rename Files..." );
            help_text = _( "Rename files" );
        }

        KIUI::AddMenuItem( &popup_menu, ID_PROJECT_RENAME, text, help_text,
                           KiBitmap( BITMAPS::right ) );
    }

    if( can_delete )
    {
        if( selection.size() == 1 )
            help_text = _( "Delete the file and its content" );
        else
            help_text = _( "Delete the files and their contents" );

        if( can_switch_to_project
                || can_create_new_directory
                || can_open_this_directory
                || can_edit
                || can_rename )
        {
            popup_menu.AppendSeparator();
        }

#ifdef __WINDOWS__
        KIUI::AddMenuItem( &popup_menu, ID_PROJECT_DELETE, _( "Delete" ), help_text,
                           KiBitmap( BITMAPS::trash ) );
#else
        KIUI::AddMenuItem( &popup_menu, ID_PROJECT_DELETE, _( "Move to Trash" ), help_text,
                           KiBitmap( BITMAPS::trash ) );
#endif
    }

    if( vcs_menu )
    {
        wxMenu*     vcs_submenu = new wxMenu();
        wxMenu*     branch_submenu = new wxMenu();
        wxMenuItem* vcs_menuitem = nullptr;

        vcs_menuitem = vcs_submenu->Append( ID_GIT_INITIALIZE_PROJECT,
                                            _( "Add Project to Version Control..." ),
                                            _( "Initialize a new repository" ) );
        vcs_menuitem->Enable( vcs_can_init );

        vcs_menuitem = vcs_submenu->Append( ID_GIT_REMOTE_SETTINGS, _( "Configure Default Remote..." ),
                                            _( "Set or change the default remote URL and credentials" ) );
        vcs_menuitem->Enable( vcs_can_set_remote );

        // Committing, pushing and pulling are done from the docked Version Control panel, so
        // the version-control submenu keeps only repository setup and branch operations.
        vcs_submenu->AppendSeparator();

        // Reading the branch list walks the repository. On contention the submenu is built
        // without the shortcuts rather than reading it concurrently.
        std::shared_ptr<KIGIT_COMMON> common = gitCommon();
        m_gitMenuBranches.clear();
        m_gitMenuSession = common;
        std::vector<KIGIT_BRANCH_INFO> branches;

        {
            std::unique_lock<std::mutex> branchLock = common->TryLockAction();

            if( branchLock.owns_lock() )
                branches = common->GetBranches();
        }

        KIGIT::SortBranchesForDisplay( branches );

        for( const KIGIT_BRANCH_INFO& branch : branches )
        {
            if( branch.isRemote || branch.isCurrent )
                continue;

            m_gitMenuBranches.push_back( branch.name );

            if( m_gitMenuBranches.size() == 5 )
                break;
        }

        for( size_t ii = 0; ii < m_gitMenuBranches.size(); ++ii )
        {
            wxString msg = _( "Switch to branch " ) + m_gitMenuBranches[ii];
            vcs_menuitem = branch_submenu->Append( ID_GIT_SWITCH_QUICK1 + ii, m_gitMenuBranches[ii], msg );
            vcs_menuitem->Enable( vcs_can_switch );
        }

        vcs_menuitem = branch_submenu->Append( ID_GIT_SWITCH_BRANCH, _( "Other..." ),
                                     _( "Switch to a different branch" ) );
        vcs_menuitem->Enable( vcs_can_switch );

        vcs_menuitem = vcs_submenu->Append( ID_GIT_SWITCH_BRANCH, _( "Switch to Branch" ), branch_submenu );
        vcs_menuitem->Enable( vcs_can_switch );

        vcs_submenu->AppendSeparator();

        if( gitIntegrationDisabled )
        {
            vcs_menuitem = vcs_submenu->Append( ID_GIT_REMOVE_VCS, _( "Enable Git Integration" ),
                                 _( "Re-enable Git integration for this project" ) );
        }
        else
        {
            vcs_menuitem = vcs_submenu->Append( ID_GIT_REMOVE_VCS, _( "Disable Git Integration" ),
                                 _( "Disable Git integration for this project" ) );
        }

        vcs_menuitem->Enable( vcs_can_remove );

        popup_menu.AppendSeparator();
        popup_menu.AppendSubMenu( vcs_submenu, _( "Version Control" ) );
    }

    if( popup_menu.GetMenuItemCount() > 0 )
        PopupMenu( &popup_menu );
}


void PROJECT_TREE_PANE::onOpenSelectedFileWithTextEditor( wxCommandEvent& event )
{
    wxString editorname = Pgm().GetTextEditor();

    if( editorname.IsEmpty() )
    {
        wxMessageBox( _( "No text editor selected in KiCad.  Please choose one." ) );
        return;
    }

    std::vector<PROJECT_TREE_ITEM*> tree_data = GetSelectedData();

    for( PROJECT_TREE_ITEM* item_data : tree_data )
    {
        wxString fullFileName = item_data->GetFileName();

        if( !fullFileName.IsEmpty() )
        {
            ExecuteFile( editorname, fullFileName.wc_str(), nullptr, false );
        }
    }
}


void PROJECT_TREE_PANE::onDeleteFile( wxCommandEvent& event )
{
    std::vector<PROJECT_TREE_ITEM*> tree_data = GetSelectedData();

    for( PROJECT_TREE_ITEM* item_data : tree_data )
        item_data->Delete();
}


void PROJECT_TREE_PANE::onRenameFile( wxCommandEvent& event )
{
    wxTreeItemId                    curr_item = m_TreeProject->GetFocusedItem();
    std::vector<PROJECT_TREE_ITEM*> tree_data = GetSelectedData();

    // XXX: Unnecessary?
    if( tree_data.size() != 1 )
        return;

    wxString buffer = m_TreeProject->GetItemText( curr_item );
    wxString msg = wxString::Format( _( "Change filename: '%s'" ),
                                     tree_data[0]->GetFileName() );
    wxTextEntryDialog dlg( wxGetTopLevelParent( this ), msg, _( "Change filename" ), buffer );

    if( dlg.ShowModal() != wxID_OK )
        return; // canceled by user

    buffer = dlg.GetValue();
    buffer.Trim( true );
    buffer.Trim( false );

    if( buffer.IsEmpty() )
        return; // empty file name not allowed

    tree_data[0]->Rename( buffer, true );
    m_isRenaming = true;
}


void PROJECT_TREE_PANE::onSelect( wxTreeEvent& Event )
{
    std::vector<PROJECT_TREE_ITEM*> tree_data = GetSelectedData();

    if( tree_data.size() != 1 )
        return;

    // Bookmark the selected item but don't try and activate it until later.  If we do it now,
    // there will be more events at least on Windows in this frame that will steal focus from
    // any newly launched windows
    m_selectedItem = tree_data[0];
}


void PROJECT_TREE_PANE::onIdle( wxIdleEvent& aEvent )
{
    // Idle executes once all other events finished processing.  This makes it ideal to launch
    // a new window without starting Focus wars.
    if( m_watcherNeedReset )
    {
        m_selectedItem = nullptr;
        FileWatcherReset();
    }

    if( m_selectedItem != nullptr && m_TreeProject->GetRootItem().IsOk() )
    {
        // Make sure m_selectedItem still exists in the tree before activating it.
        std::vector<wxTreeItemId> validItemIds;
        m_TreeProject->GetItemsRecursively( m_TreeProject->GetRootItem(), validItemIds );

        for( wxTreeItemId id : validItemIds )
        {
            if( GetItemIdData( id ) == m_selectedItem )
            {
                // Activate launches a window which may run the event loop on top of us and cause
                // onIdle to get called again, so be sure to null out m_selectedItem first.
                PROJECT_TREE_ITEM* item = m_selectedItem;
                m_selectedItem          = nullptr;

                item->Activate( this );
                break;
            }
        }
    }
}


void PROJECT_TREE_PANE::onExpand( wxTreeEvent& Event )
{
    wxTreeItemId       itemId    = Event.GetItem();
    PROJECT_TREE_ITEM* tree_data = GetItemIdData( itemId );

    if( !tree_data )
        return;

    if( tree_data->GetType() != TREE_FILE_TYPE::DIRECTORY )
        return;

    // explore list of non populated subdirs, and populate them
    wxTreeItemIdValue   cookie;
    wxTreeItemId        kid = m_TreeProject->GetFirstChild( itemId, cookie );

#ifndef __WINDOWS__
    bool subdir_populated = false;
#endif

    for( ; kid.IsOk(); kid = m_TreeProject->GetNextChild( itemId, cookie ) )
    {
        PROJECT_TREE_ITEM* itemData = GetItemIdData( kid );

        if( !itemData || itemData->GetType() != TREE_FILE_TYPE::DIRECTORY )
            continue;

        if( itemData->IsPopulated() )
            continue;

        wxString    fileName = itemData->GetFileName();
        wxDir       dir( fileName );

        if( dir.IsOpened() )
        {
            std::vector<wxString> projects = getProjects( dir );
            wxString              dir_filename;
            bool                  haveFile = dir.GetFirst( &dir_filename );

            while( haveFile )
            {
                // Add name to tree item, but do not recurse in subdirs:
                wxString name = fileName + wxFileName::GetPathSeparator() + dir_filename;
                addItemToProjectTree( name, kid, &projects, false );

                haveFile = dir.GetNext( &dir_filename );
            }

            itemData->SetPopulated( true );       // set state to populated

#ifndef __WINDOWS__
            subdir_populated = true;
#endif
        }

        // Sort filenames by alphabetic order
        m_TreeProject->SortChildren( kid );
    }

#ifndef __WINDOWS__
    if( subdir_populated )
        m_watcherNeedReset = true;
#endif
}


std::vector<PROJECT_TREE_ITEM*> PROJECT_TREE_PANE::GetSelectedData()
{
    wxArrayTreeItemIds              selection;
    std::vector<PROJECT_TREE_ITEM*> data;

    m_TreeProject->GetSelections( selection );

    for( const wxTreeItemId itemId : selection )
    {
        PROJECT_TREE_ITEM* item = GetItemIdData( itemId );

        if( !item )
        {
            wxLogTrace( traceGit, wxS( "Null tree item returned for selection, dynamic_cast failed?" ) );
            continue;
        }

        data.push_back( item );
    }

    return data;
}


PROJECT_TREE_ITEM* PROJECT_TREE_PANE::GetItemIdData( wxTreeItemId aId )
{
    return dynamic_cast<PROJECT_TREE_ITEM*>( m_TreeProject->GetItemData( aId ) );
}


wxTreeItemId PROJECT_TREE_PANE::findSubdirTreeItem( const wxString& aSubDir )
{
    wxString prj_dir = wxPathOnly( m_Parent->GetProjectFileName() );

    // If the subdir is the current working directory, return m_root
    // in main list:
    if( prj_dir == aSubDir )
        return m_root;

    // The subdir is in the main tree or in a subdir: Locate it
    wxTreeItemIdValue        cookie;
    wxTreeItemId             root_id = m_root;
    std::stack<wxTreeItemId> subdirs_id;

    wxTreeItemId child = m_TreeProject->GetFirstChild( root_id, cookie );

    while( true )
    {
        if( ! child.IsOk() )
        {
            if( subdirs_id.empty() )    // all items were explored
            {
                root_id = child;          // Not found: return an invalid wxTreeItemId
                break;
            }
            else
            {
                root_id = subdirs_id.top();
                subdirs_id.pop();
                child = m_TreeProject->GetFirstChild( root_id, cookie );

                if( !child.IsOk() )
                    continue;
            }
        }

        PROJECT_TREE_ITEM* itemData = GetItemIdData( child );

        if( itemData && ( itemData->GetType() == TREE_FILE_TYPE::DIRECTORY ) )
        {
            if( itemData->GetFileName() == aSubDir )    // Found!
            {
                root_id = child;
                break;
            }

            // child is a subdir, push in list to explore it later
            if( itemData->IsPopulated() )
                subdirs_id.push( child );
        }

        child = m_TreeProject->GetNextChild( root_id, cookie );
    }

    return root_id;
}


void PROJECT_TREE_PANE::onFileSystemEvent( wxFileSystemWatcherEvent& event )
{
    // No need to process events when we're shutting down
    if( !m_watcher )
        return;

    // Ignore events that are not file creation, deletion, renaming or modification because
    // they are not relevant to the project tree.
    if( !( event.GetChangeType() & ( wxFSW_EVENT_CREATE |
                                     wxFSW_EVENT_DELETE |
                                     wxFSW_EVENT_RENAME |
                                     wxFSW_EVENT_MODIFY ) ) )
    {
        return;
    }

    const wxFileName& pathModified = event.GetPath();

    // Ignore events from .history directory (local backup)
    if( pathModified.GetFullPath().Contains( wxS( ".history" ) ) )
        return;

    wxString subdir = pathModified.GetPath();
    wxString fn = pathModified.GetFullPath();

    // Adjust directories to look like a file item (path and name).
    if( pathModified.GetFullName().IsEmpty() )
    {
        subdir = subdir.BeforeLast( '/' );
        fn = fn.BeforeLast( '/' );
    }

    wxTreeItemId root_id = findSubdirTreeItem( subdir );

    if( !root_id.IsOk() )
        return;

    CallAfter( [this] ()
    {
        wxLogTrace( traceGit, wxS( "File system event detected, updating tree cache" ) );
        m_gitStatusTimer.Start( 1500, wxTIMER_ONE_SHOT );
    } );

    wxTreeItemIdValue  cookie;  // dummy variable needed by GetFirstChild()
    wxTreeItemId kid = m_TreeProject->GetFirstChild( root_id, cookie );

    switch( event.GetChangeType() )
    {
    case wxFSW_EVENT_CREATE:
    {
        wxTreeItemId newitem = addItemToProjectTree( fn, root_id, nullptr, true );

        // If we are in the process of renaming a file, select the new one
        // This is needed for MSW and OSX, since we don't get RENAME events from them, just a
        // pair of DELETE and CREATE events.
        if( m_isRenaming && newitem.IsOk() )
        {
            m_TreeProject->SelectItem( newitem );
            m_isRenaming = false;
        }
    }
        break;

    case wxFSW_EVENT_DELETE:
        while( kid.IsOk() )
        {
            PROJECT_TREE_ITEM* itemData = GetItemIdData( kid );

            if( itemData && itemData->GetFileName() == fn )
            {
                m_TreeProject->Delete( kid );
                return;
            }
            kid = m_TreeProject->GetNextChild( root_id, cookie );
        }
        break;

    case wxFSW_EVENT_RENAME :
    {
        const wxFileName& newpath = event.GetNewPath();
        wxString newdir = newpath.GetPath();
        wxString newfn = newpath.GetFullPath();

        while( kid.IsOk() )
        {
            PROJECT_TREE_ITEM* itemData = GetItemIdData( kid );

            if( itemData && itemData->GetFileName() == fn )
            {
                m_TreeProject->Delete( kid );
                break;
            }

            kid = m_TreeProject->GetNextChild( root_id, cookie );
        }

        // Add the new item only if it is not the current project file (root item).
        // Remember: this code is called by a wxFileSystemWatcherEvent event, and not always
        // called after an actual file rename, and the cleanup code does not explore the
        // root item, because it cannot be renamed by the user. Also, ensure the new file
        // actually exists on the file system before it is readded. On Linux, moving a file
        // to the trash can cause the same path to be returned in both the old and new paths
        // of the event, even though the file isn't there anymore.
        PROJECT_TREE_ITEM* rootData = GetItemIdData( root_id );

        if( rootData && newpath.Exists() && ( newfn != rootData->GetFileName() ) )
        {
            wxTreeItemId newroot_id = findSubdirTreeItem( newdir );
            wxTreeItemId newitem = addItemToProjectTree( newfn, newroot_id, nullptr, true );

            // If the item exists, select it
            if( newitem.IsOk() )
                m_TreeProject->SelectItem( newitem );
        }

        m_isRenaming = false;
    }
        break;

    default:
        return;
    }

    // Sort filenames by alphabetic order
    m_TreeProject->SortChildren( root_id );
}


void PROJECT_TREE_PANE::FileWatcherReset()
{
    m_watcherNeedReset = false;

    wxString prj_dir = wxPathOnly( m_Parent->GetProjectFileName() );

#if defined( _WIN32 )
    if( KIPLATFORM::ENV::IsNetworkPath( prj_dir ) )
    {
        // Due to a combination of a bug in SAMBA sending bad change event IDs and wxWidgets
        // choosing to fault on an invalid event ID instead of sanely ignoring them we need to
        // avoid spawning a filewatcher. Unfortunately this punishes corporate environments with
        // Windows Server shares :/
        m_Parent->m_FileWatcherInfo = _( "Network path: not monitoring folder changes" );
        m_Parent->SetStatusText( m_Parent->m_FileWatcherInfo, 1 );
        return;
    }
    else
    {
        m_Parent->m_FileWatcherInfo = _( "Local path: monitoring folder changes" );
        m_Parent->SetStatusText( m_Parent->m_FileWatcherInfo, 1 );
    }
#endif

    // Prepare file watcher:
    if( m_watcher )
    {
        m_watcher->RemoveAll();
    }
    else
    {
        // Create a wxWidgets log handler to catch errors during watcher creation
        // We need to to this because we cannot get error codes from the wxFileSystemWatcher
        // constructor.  On Linux, if inotify cannot be initialized (usually due to resource limits),
        // wxWidgets will throw a system error and then, we throw another error below, trying to
        // add paths to a null watcher.  We skip this by installing a temporary log handler that
        // catches errors during watcher creation and aborts if any error is detected.
        class WatcherLogHandler : public wxLog
        {
        public:
            explicit WatcherLogHandler( bool* err ) :
                    m_err( err )
            {
                if( m_err )
                    *m_err = false;
            }

        protected:
            void DoLogTextAtLevel( wxLogLevel level, const wxString& text ) override
            {
                if( m_err && ( level == wxLOG_Error || level == wxLOG_FatalError ) )
                    *m_err = true;
            }

        private:
            bool* m_err;
        };

        bool watcherHasError = false;
        WatcherLogHandler tmpLog( &watcherHasError );

        {
            SCOPED_WXLOG_TARGET logOverride( &tmpLog );

            m_watcher = new wxFileSystemWatcher();
            m_watcher->SetOwner( this );
        }

        if( watcherHasError )
        {
            return;
        }
    }

    // We can see wxString under a debugger, not a wxFileName
    wxFileName fn;
    fn.AssignDir( prj_dir );
    fn.DontFollowLink();

    // Add directories which should be monitored.
    // under windows, we add the curr dir and all subdirs
    // under unix, we add only the curr dir and the populated subdirs
    // see  http://docs.wxwidgets.org/trunk/classwx_file_system_watcher.htm
    // under unix, the file watcher needs more work to be efficient
    // moreover, under wxWidgets 2.9.4, AddTree does not work properly.
    {
        wxLogNull logNo;    // avoid log messages
#ifdef __WINDOWS__
        if( ! m_watcher->AddTree( fn ) )
        {
            wxLogTrace( tracePathsAndFiles, "%s: failed to add '%s'\n", __func__,
                        TO_UTF8( fn.GetFullPath() ) );
            return;
        }
    }
#else
        if( !m_watcher->Add( fn ) )
        {
            wxLogTrace( tracePathsAndFiles, "%s: failed to add '%s'\n", __func__,
                        TO_UTF8( fn.GetFullPath() ) );
            return;
        }
    }

    if( m_TreeProject->IsEmpty() )
        return;

    // Add subdirs
    wxTreeItemIdValue  cookie;
    wxTreeItemId       root_id = m_root;

    std::stack < wxTreeItemId > subdirs_id;

    wxTreeItemId kid = m_TreeProject->GetFirstChild( root_id, cookie );
    int total_watch_count = 0;

    while( total_watch_count < ADVANCED_CFG::GetCfg().m_MaxFilesystemWatchers )
    {
        if( !kid.IsOk() )
        {
            if( subdirs_id.empty() )    // all items were explored
            {
                break;
            }
            else
            {
                root_id = subdirs_id.top();
                subdirs_id.pop();
                kid = m_TreeProject->GetFirstChild( root_id, cookie );

                if( !kid.IsOk() )
                    continue;
            }
        }

        PROJECT_TREE_ITEM* itemData = GetItemIdData( kid );

        if( itemData && itemData->GetType() == TREE_FILE_TYPE::DIRECTORY )
        {
            // we can see wxString under a debugger, not a wxFileName
            const wxString& path = itemData->GetFileName();

            // Skip .history directory and its descendants (local backup)
            if( path.Contains( wxS( ".history" ) ) )
            {
                kid = m_TreeProject->GetNextChild( root_id, cookie );
                continue;
            }

            wxLogTrace( tracePathsAndFiles, "%s: add '%s'\n", __func__, TO_UTF8( path ) );

            if( wxFileName::IsDirReadable( path ) )   // linux whines about watching protected dir
            {
                {
                    // Silence OS errors that come from the watcher
                    wxLogNull silence;
                    fn.AssignDir( path );
                    m_watcher->Add( fn );
                    total_watch_count++;
                }

                // if kid is a subdir, push in list to explore it later
                if( itemData->IsPopulated() && m_TreeProject->GetChildrenCount( kid ) )
                    subdirs_id.push( kid );
            }
        }

        kid = m_TreeProject->GetNextChild( root_id, cookie );
    }

    if( total_watch_count >= ADVANCED_CFG::GetCfg().m_MaxFilesystemWatchers )
        wxLogTrace( tracePathsAndFiles, "%s: too many directories to watch\n", __func__ );
#endif

#if defined(DEBUG) && 1
    wxArrayString paths;
    m_watcher->GetWatchedPaths( &paths );
    wxLogTrace( tracePathsAndFiles, "%s: watched paths:", __func__ );

    for( unsigned ii = 0; ii < paths.GetCount(); ii++ )
        wxLogTrace( tracePathsAndFiles, " %s\n", TO_UTF8( paths[ii] ) );
#endif
    }


void PROJECT_TREE_PANE::EmptyTreePrj()
{
    m_branchQuery->Cancel();
    m_gitMenuBranches.clear();
    m_gitMenuSession.reset();
    KIGIT::ClearSyncWarnings( m_Parent );

    // Make sure we don't try to inspect the tree after we've deleted its items.
    shutdownFileWatcher();

    m_TreeProject->DeleteAllItems();
}


void PROJECT_TREE_PANE::WaitForGitOperations()
{
    m_branchQuery->Cancel();

    if( gitRepo() )
    {
        std::shared_ptr<KIGIT_COMMON> common = gitCommon();
        common->SetShuttingDown( true );

        std::unique_lock<std::mutex> lock = common->TryLockAction();

        constexpr auto kGraceMs = std::chrono::seconds( 2 );
        auto           graceEnd = std::chrono::steady_clock::now() + kGraceMs;

        while( !lock.owns_lock() && std::chrono::steady_clock::now() < graceEnd )
        {
            if( lock.try_lock() )
                break;
            std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
        }

        constexpr auto kCheckInterval = std::chrono::seconds( 30 );
        bool           userAbandoned  = false;

        while( !lock.owns_lock() && !userAbandoned )
        {
            auto intervalEnd = std::chrono::steady_clock::now() + kCheckInterval;

            {
                wxProgressDialog progress( _( "Please wait" ),
                                           _( "Closing project..." ),
                                           100, this,
                                           wxPD_APP_MODAL | wxPD_SMOOTH );

                while( !lock.try_lock()
                       && std::chrono::steady_clock::now() < intervalEnd )
                {
                    progress.Pulse();
                    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
                    wxYield();
                }
            }

            if( lock.owns_lock() )
                break;

            wxMessageDialog ask( this,
                                 _( "A Git operation is still running.\n"
                                    "Keep waiting, or abandon?" ),
                                 _( "Git Operation Delayed" ),
                                 wxYES_NO | wxICON_QUESTION );
            ask.SetYesNoLabels( _( "Keep Waiting" ), _( "Abandon" ) );

            if( ask.ShowModal() == wxID_NO )
                userAbandoned = true;
        }

        if( userAbandoned )
        {
            // Cancellation was requested above, so a running fetch exits at its next
            // callback. The release defers the actual free to the orphan registry.
            common.reset();
            Prj().ReleaseGitCommon();
        }
        else
        {
            git_repository* repo = common->GetRepo();
            KIGIT::PROJECT_GIT_UTILS::RemoveVCS( repo );
            common->SetRepo( nullptr );
        }
    }
}


void PROJECT_TREE_PANE::releaseGitRepo()
{
    m_branchQuery->Cancel();
    m_gitMenuBranches.clear();
    m_gitMenuSession.reset();
    Prj().ReleaseGitCommon();
}


std::shared_ptr<KIGIT_COMMON> PROJECT_TREE_PANE::gitCommon() const
{
    return Prj().GetGitCommon();
}


void PROJECT_TREE_PANE::syncGitListener()
{
    std::shared_ptr<KIGIT_COMMON> common = gitCommon();

    if( m_gitListenerSession.lock() == common )
        return;

    m_branchQuery->Cancel();
    m_gitMenuBranches.clear();
    m_gitMenuSession.reset();
    KIGIT::ClearSyncWarnings( m_Parent );

    if( std::shared_ptr<KIGIT_COMMON> previous = m_gitListenerSession.lock() )
        previous->RemoveRefreshListener( this );

    // The status timer coalesces several notifications into one pass, and reads the repository
    // locally without fetching.
    common->AddRefreshListener( this,
                                [this]()
                                {
                                    if( m_TreeProject )
                                        m_gitStatusTimer.Start( 500, wxTIMER_ONE_SHOT );
                                } );

    m_gitListenerSession = common;
}


git_repository* PROJECT_TREE_PANE::gitRepo() const
{
    return gitCommon()->GetRepo();
}


void PROJECT_TREE_PANE::onThemeChanged( wxSysColourChangedEvent &aEvent )
{
    GetBitmapStore()->ThemeChanged();
    m_TreeProject->LoadIcons();
    m_TreeProject->Refresh();

    aEvent.Skip();
}


void PROJECT_TREE_PANE::onPaint( wxPaintEvent& event )
{
    wxRect    rect( wxPoint( 0, 0 ), GetClientSize() );
    wxPaintDC dc( this );

    dc.SetBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_FRAMEBK ) );
    dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ), 1 ) );

    dc.DrawLine( rect.GetLeft(), rect.GetTop(), rect.GetLeft(), rect.GetBottom() );
    dc.DrawLine( rect.GetRight(), rect.GetTop(), rect.GetRight(), rect.GetBottom() );
}


void KICAD_MANAGER_FRAME::OnChangeWatchedPaths( wxCommandEvent& aEvent )
{
    m_projectTreePane->FileWatcherReset();
}


// Prompts for git credentials after an auth failure. Returns false if cancelled.
static bool promptForGitCredentials( wxWindow* aParent, KIGIT_COMMON* aCommon, const wxString& aExplanation )
{
    return KIGIT::PromptForCredentials( aParent, aCommon, &Prj(), aExplanation );
}


// Persist a remote configuration the user accepted, to the secret store and the project
// local settings. Runs only after the remote itself was set up successfully.
static void saveAcceptedRemoteSettings( PROJECT& aProject, const RemoteConfig& aConfig )
{
    // Keyed like the attach-time lookup, which never sees credentials embedded in the address
    KIPLATFORM::SECRETS::StoreSecret( KIGIT_COMMON::ParseRemoteURL( aConfig.url ).url, aConfig.username,
                                      aConfig.password );
    aProject.GetLocalSettings().m_GitRepoUsername = aConfig.username;
    aProject.GetLocalSettings().m_GitSSHKey = aConfig.sshKey;

    if( aConfig.connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
        aProject.GetLocalSettings().m_GitRepoType = "ssh";
    else if( aConfig.connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS )
        aProject.GetLocalSettings().m_GitRepoType = "https";
    else
        aProject.GetLocalSettings().m_GitRepoType = "local";
}


void PROJECT_TREE_PANE::onGitInitializeProject( wxCommandEvent& aEvent )
{
    PROJECT_TREE_ITEM* tree_data = GetItemIdData( m_TreeProject->GetRootItem() );

    wxString dir = tree_data->GetDir();

    if( dir.empty() )
    {
        wxLogError( "Failed to initialize git project: project directory is empty." );
        return;
    }

    GIT_INIT_HANDLER initHandler( gitCommon().get() );
    wxWindow*        topLevelParent = wxGetTopLevelParent( this );

    if( initHandler.IsRepository( dir ) )
    {
        DisplayInfoMessage( topLevelParent,
                            _( "The selected directory is already a Git project." ) );
        return;
    }

    InitResult result = initHandler.InitializeRepository( dir );
    if( result != InitResult::Success )
    {
        KIGIT_ERROR_MESSAGE error = initHandler.PopError();
        DisplayErrorMessage( m_parent, error.summary, error.details );
        return;
    }

    KIGIT::ClearSyncWarnings( m_Parent );

    gitCommon()->SetShuttingDown( false );

    const char* canonicalWorkDir = git_repository_workdir( initHandler.GetRepo() );

    if( canonicalWorkDir )
    {
        wxString symlinkWorkDir = KIGIT::PROJECT_GIT_UTILS::ComputeSymlinkPreservingWorkDir(
                dir, wxString::FromUTF8( canonicalWorkDir ) );
        gitCommon()->SetProjectDir( symlinkWorkDir );
    }

    DIALOG_GIT_REPOSITORY dlg( topLevelParent, initHandler.GetRepo() );
    dlg.SetTitle( _( "Set default remote" ) );
    dlg.SetSkipButtonLabel( _( "Skip" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Set up the remote
    RemoteConfig remoteConfig;
    remoteConfig.url = dlg.GetRepoURL();
    remoteConfig.username = dlg.GetUsername();
    remoteConfig.password = dlg.GetPassword();
    remoteConfig.sshKey = dlg.GetRepoSSHPath();
    remoteConfig.connType = dlg.GetRepoType();

    if( !initHandler.SetupRemote( remoteConfig ) )
    {
        KIGIT_ERROR_MESSAGE error = initHandler.PopError();
        DisplayErrorMessage( m_parent, error.summary, error.details );
        return;
    }

    KIGIT::ClearSyncWarnings( m_Parent );

    // Before the fetch, so a corrected login from the prompt below is the one that stays saved
    saveAcceptedRemoteSettings( Prj(), remoteConfig );

    KIGIT_COMMON* common = gitCommon().get();
    bool          retriedAfterTrust = false;

    while( true )
    {
        GIT_PULL_HANDLER handler( common );
        handler.SetProgressReporter(
                std::make_unique<WX_PROGRESS_REPORTER>( this, _( "Fetch Remote" ), 1, PR_NO_ABORT ) );

        // A status read can hold the lock briefly, and the login or key just confirmed must not be dropped
        handler.SetActionLockWait( KIGIT_COMMON::ACTION_LOCK_WAIT );

        if( handler.PerformFetch() )
            break;

        if( std::optional<KIGIT::HOST_KEY_FAILURE> hostKey = handler.GetHostKeyFailure() )
        {
            if( !KIGIT::PromptToTrustHost( m_parent, *hostKey, retriedAfterTrust ) )
                break;

            retriedAfterTrust = true;
            continue;
        }

        KIGIT_ERROR_MESSAGE error = handler.PopError();

        if( handler.WasAuthFailure() )
        {
            if( promptForGitCredentials( m_parent, common, error.summary ) )
                continue;

            break;
        }

        // The remote is saved either way, so this only says why its first fetch did not happen. A cancelled
        // fetch needs no dialog
        if( !error.summary.IsEmpty() && handler.GetFailureKind() != KIGIT_FAILURE::CANCELLED )
            DisplayErrorMessage( m_parent, error.summary, error.details );

        break;
    }
}


void PROJECT_TREE_PANE::onGitRemoteSettings( wxCommandEvent& aEvent )
{
    KIGIT_COMMON*   common = gitCommon().get();
    git_repository* repo = common ? common->GetRepo() : nullptr;

    if( !repo )
        return;

    wxString existingURL;

    {
        // Read the remote under the lock, then release it before opening the dialog.
        std::unique_lock<std::mutex> lock = KIGIT::TryLockAction( *common, this );

        if( !lock.owns_lock() )
            return;

        git_remote* remote = nullptr;

        if( git_remote_lookup( &remote, repo, "origin" ) == GIT_OK )
        {
            KIGIT::GitRemotePtr remotePtr( remote );

            if( const char* url = git_remote_url( remote ) )
                existingURL = wxString::FromUTF8( url );
        }
    }

    wxWindow*             topLevelParent = wxGetTopLevelParent( this );
    DIALOG_GIT_REPOSITORY dlg( topLevelParent, repo, existingURL );
    dlg.SetTitle( _( "Configure Default Remote" ) );

    // Credentials are stored separately from the remote URL.
    KIGIT_COMMON credentials( nullptr );
    credentials.SetUsername( common->GetUsername() );
    credentials.SetRemote( existingURL );
    dlg.SetUsername( credentials.GetUsername() );
    dlg.SetPassword( credentials.GetPassword() );
    dlg.SetRepoSSHPath( Prj().GetLocalSettings().m_GitSSHKey );

    if( dlg.ShowModal() != wxID_OK )
        return;

    GIT_INIT_HANDLER initHandler( common );

    RemoteConfig remoteConfig;
    remoteConfig.url = dlg.GetRepoURL();
    remoteConfig.username = dlg.GetUsername();
    remoteConfig.password = dlg.GetPassword();
    remoteConfig.sshKey = dlg.GetRepoSSHPath();
    remoteConfig.connType = dlg.GetRepoType();

    bool     setupOk = false;
    wxString setupError;
    wxString setupDetails;

    {
        // Update the remote config and cached branch info under the same lock.
        std::unique_lock<std::mutex> lock = KIGIT::WaitForActionLock( *common, this );

        if( !lock.owns_lock() )
            return;

        setupOk = initHandler.SetupRemote( remoteConfig );
        KIGIT_ERROR_MESSAGE error = initHandler.PopError();
        setupError = error.summary;
        setupDetails = error.details;

        if( setupOk )
            common->UpdateCurrentBranchInfo();
    }

    if( !setupOk )
    {
        DisplayErrorMessage( topLevelParent, setupError, setupDetails );
        return;
    }

    saveAcceptedRemoteSettings( Prj(), remoteConfig );

    m_gitStatusTimer.Start( 500, wxTIMER_ONE_SHOT );
}


void PROJECT_TREE_PANE::onGitSwitchBranch( wxCommandEvent& aEvent )
{
    std::shared_ptr<KIGIT_COMMON> common = gitCommon();

    if( !common->GetRepo() || common->IsShuttingDown() )
        return;

    if( aEvent.GetId() == ID_GIT_SWITCH_BRANCH )
    {
        struct BRANCHES
        {
            std::vector<KIGIT_BRANCH_INFO> entries;
            bool                           loaded = false;
        };

        auto                         branches = std::make_shared<BRANCHES>();
        wxWeakRef<PROJECT_TREE_PANE> self( this );

        m_branchQuery->Run(
                common,
                [branches]( KIGIT_COMMON& aCommon )
                {
                    branches->entries = aCommon.GetBranches();
                    branches->loaded = true;
                },
                [self, common, branches]()
                {
                    if( !branches->loaded || !self || self->IsBeingDeleted() || self->m_Parent->IsBeingDeleted()
                        || common->IsShuttingDown() || self->gitCommon() != common
                        || !Pgm().GetCommonSettings()->m_Git.enableGit
                        || Prj().GetLocalSettings().m_GitIntegrationDisabled )
                    {
                        return;
                    }

                    DIALOG_GIT_SWITCH dlg( wxGetTopLevelParent( self->m_Parent ), std::move( branches->entries ) );
                    int               retval = dlg.ShowModal();

                    if( !self || self->IsBeingDeleted() || common->IsShuttingDown() || self->gitCommon() != common )
                        return;

                    if( retval == wxID_OK || retval == wxID_ADD )
                        self->switchGitBranch( common, dlg.GetBranchName(), retval == wxID_ADD );
                } );

        return;
    }

    int branchIndex = aEvent.GetId() - ID_GIT_SWITCH_QUICK1;

    if( m_gitMenuSession.lock() != common || branchIndex < 0
        || static_cast<size_t>( branchIndex ) >= m_gitMenuBranches.size() )
    {
        return;
    }

    switchGitBranch( common, m_gitMenuBranches[branchIndex], false );
}


void PROJECT_TREE_PANE::switchGitBranch( std::shared_ptr<KIGIT_COMMON> aCommon, const wxString& aBranchName,
                                         bool aCreate )
{
    if( IsBeingDeleted() || m_Parent->IsBeingDeleted() || gitCommon() != aCommon || aCommon->IsShuttingDown()
        || !Pgm().GetCommonSettings()->m_Git.enableGit || Prj().GetLocalSettings().m_GitIntegrationDisabled )
    {
        return;
    }

    if( !aCreate )
    {
        SwitchProjectToBranch( m_Parent, this, aBranchName, *m_branchQuery );
        return;
    }

    if( wxString nameError = KIGIT::BranchNameError( aBranchName ); !nameError.IsEmpty() )
    {
        DisplayErrorMessage( this, nameError );
        return;
    }

    // Keep Add subject to the same worktree checks before creating the branch. The shared
    // switch helper then owns confirmation, editor checks, checkout, and reload.
    auto createBranch = [this, aCommon, aBranchName]( const GIT_WORKTREE_STATUS& )
    {
        if( IsBeingDeleted() || m_Parent->IsBeingDeleted() || gitCommon() != aCommon || aCommon->IsShuttingDown()
            || !Pgm().GetCommonSettings()->m_Git.enableGit || Prj().GetLocalSettings().m_GitIntegrationDisabled )
        {
            return;
        }

        int      result;
        wxString error;

        {
            std::unique_lock<std::mutex> lock = KIGIT::WaitForActionLock( *aCommon, this );

            if( !lock.owns_lock() )
                return;

            result = KIGIT::PROJECT_GIT_UTILS::CreateBranch( aCommon->GetRepo(), aBranchName );
            error = KIGIT_COMMON::GetLastGitError();
        }

        if( result != GIT_OK )
        {
            DisplayErrorMessage( this, _( "Could not create the branch." ), error );
            return;
        }

        SwitchProjectToBranch( m_Parent, this, aBranchName, *m_branchQuery );
        aCommon->NotifyRefreshListeners();
    };

    ProjectClearOfUncommittedWork( m_Parent, this, *m_branchQuery, std::move( createBranch ) );
}


void PROJECT_TREE_PANE::onGitRemoveVCS( wxCommandEvent& aEvent )
{
    PROJECT_LOCAL_SETTINGS& localSettings = Prj().GetLocalSettings();

    // Toggle the Git integration disabled preference
    localSettings.m_GitIntegrationDisabled = !localSettings.m_GitIntegrationDisabled;

    wxLogTrace( traceGit, wxS( "onGitRemoveVCS: Git integration %s" ),
                localSettings.m_GitIntegrationDisabled ? wxS( "disabled" ) : wxS( "enabled" ) );

    if( localSettings.m_GitIntegrationDisabled )
    {
        // Disabling Git integration - release the repo and clear item states
        releaseGitRepo();
        m_gitIconsInitialized = false;

        // Clear all item states to remove git status icons
        std::stack<wxTreeItemId> items;
        items.push( m_TreeProject->GetRootItem() );

        while( !items.empty() )
        {
            wxTreeItemId current = items.top();
            items.pop();

            m_TreeProject->SetItemState( current, wxTREE_ITEMSTATE_NONE );

            wxTreeItemIdValue cookie;
            wxTreeItemId      child = m_TreeProject->GetFirstChild( current, cookie );

            while( child.IsOk() )
            {
                items.push( child );
                child = m_TreeProject->GetNextChild( current, cookie );
            }
        }
    }
    else
    {
        // Re-enabling Git integration - try to find and connect to the repository
        Prj().AttachGitRepository();
    }

    // Save the preference to the project local settings file
    localSettings.SaveToFile( Prj().GetProjectPath() );
}


void PROJECT_TREE_PANE::updateGitStatusIcons()
{
    wxLogTrace( traceGit, wxS( "updateGitStatusIcons: Updating git status icons" ) );
    std::unique_lock<std::mutex> lock( m_gitStatusMutex, std::try_to_lock );

    if( !lock.owns_lock() )
    {
        wxLogTrace( traceGit, wxS( "updateGitStatusIcons: Failed to acquire lock for git status icon update" ) );
        m_gitStatusTimer.Start( 500, wxTIMER_ONE_SHOT );
        return;
    }

    if( !Pgm().GetCommonSettings()->m_Git.enableGit || !m_TreeProject )
    {
        wxLogTrace( traceGit, wxS( "updateGitStatusIcons: Git is disabled or tree control is null" ) );
        return;
    }

    std::stack<wxTreeItemId> items;
    items.push(m_TreeProject->GetRootItem());

    while( !items.empty() )
    {
        wxTreeItemId current = items.top();
        items.pop();

        if( current.IsOk() && m_TreeProject->ItemHasChildren( current ) )
        {
            wxTreeItemIdValue cookie;
            wxTreeItemId      child = m_TreeProject->GetFirstChild( current, cookie );

            while( child.IsOk() )
            {
                items.push( child );

                if( auto it = m_gitStatusIcons.find( child ); it != m_gitStatusIcons.end() )
                    m_TreeProject->SetItemState( child, static_cast<int>( it->second ) );

                child = m_TreeProject->GetNextChild( current, cookie );
            }
        }
    }

    if( !m_gitCurrentBranchName.empty() )
    {
        wxTreeItemId       kid = m_TreeProject->GetRootItem();
        PROJECT_TREE_ITEM* rootItem = kid.IsOk() ? GetItemIdData( kid ) : nullptr;

        if( rootItem )
        {
            wxString filename = wxFileNameFromPath( rootItem->GetFileName() );
            m_TreeProject->SetItemText( kid, filename + " [" + m_gitCurrentBranchName + "]" );
            m_gitIconsInitialized = true;
        }
    }

    wxLogTrace( traceGit, wxS( "updateGitStatusIcons: Git status icons updated" ) );
}


void PROJECT_TREE_PANE::updateTreeCache()
{
    wxLogTrace( traceGit, wxS( "updateTreeCache: Updating tree cache" ) );

    std::unique_lock<std::mutex> lock( m_gitTreeCacheMutex, std::try_to_lock );

    if( !lock.owns_lock() )
    {
        wxLogTrace( traceGit, wxS( "updateTreeCache: Failed to acquire lock for tree cache update" ) );
        return;
    }

    if( !m_TreeProject )
    {
        wxLogTrace( traceGit, wxS( "updateTreeCache: Tree control is null" ) );
        return;
    }

    wxTreeItemId kid = m_TreeProject->GetRootItem();

    if( !kid.IsOk() )
        return;

    // Collect a map to easily set the state of each item
    m_gitTreeCache.clear();
    std::stack<wxTreeItemId> items;
    items.push( kid );

    while( !items.empty() )
    {
        kid = items.top();
        items.pop();

        PROJECT_TREE_ITEM* nextItem = GetItemIdData( kid );

        if( !nextItem )
            continue;

        wxString gitAbsPath = nextItem->GetFileName();
#ifdef _WIN32
        gitAbsPath.Replace( wxS( "\\" ), wxS( "/" ) );
#endif
        m_gitTreeCache[gitAbsPath] = kid;

        wxTreeItemIdValue cookie;
        wxTreeItemId      child = m_TreeProject->GetFirstChild( kid, cookie );

        while( child.IsOk() )
        {
            items.push( child );
            child = m_TreeProject->GetNextChild( kid, cookie );
        }
    }
}


void PROJECT_TREE_PANE::updateGitStatusIconMap( std::shared_ptr<KIGIT_COMMON> aGitCommon,
                                                const wxString&               aProjectFullName )
{
    wxLogTrace( traceGit, wxS( "updateGitStatusIconMap: Updating git status icons" ) );
#if defined( _WIN32 )
    int refresh = ADVANCED_CFG::GetCfg().m_GitIconRefreshInterval;

    if( refresh != 0 && KIPLATFORM::ENV::IsNetworkPath( wxPathOnly( aProjectFullName ) ) )
    {
        // Need to treat windows network paths special here until we get the samba bug fixed
        // https://github.com/wxWidgets/wxWidgets/issues/18953
        CallAfter(
                [this, refresh]()
                {
                    m_gitStatusTimer.Start( refresh, wxTIMER_ONE_SHOT );
                } );
    }

#endif

    if( !Pgm().GetCommonSettings()->m_Git.enableGit || !m_TreeProject )
        return;

    std::unique_lock<std::mutex> lock1( m_gitStatusMutex, std::try_to_lock );
    std::unique_lock<std::mutex> lock2( m_gitTreeCacheMutex, std::try_to_lock );

    if( !lock1.owns_lock() || !lock2.owns_lock() )
    {
        wxLogTrace( traceGit, wxS( "updateGitStatusIconMap: Failed to acquire locks for git status icon update" ) );
        return;
    }

    // The caller pinned the session on the UI thread, so a concurrent release cannot
    // free it under us.
    if( !aGitCommon || !aGitCommon->GetRepo() )
    {
        wxLogTrace( traceGit, wxS( "updateGitStatusIconMap: No git repository found" ) );
        return;
    }

    // Acquire the git action mutex to synchronize with EmptyTreePrj() shutdown.
    // This ensures the repository isn't freed while we're using it.
    std::unique_lock<std::mutex> gitLock = aGitCommon->TryLockAction();

    if( !gitLock.owns_lock() )
    {
        wxLogTrace( traceGit, wxS( "updateGitStatusIconMap: Failed to acquire git action mutex" ) );
        return;
    }

    // Check if cancellation was requested (e.g., during shutdown)
    if( aGitCommon->IsShuttingDown() )
    {
        wxLogTrace( traceGit, wxS( "updateGitStatusIconMap: Cancelled" ) );
        return;
    }

    GIT_STATUS_HANDLER statusHandler( aGitCommon.get() );

    // Set up pathspec for project files. The name is passed in from the UI thread
    // because Prj() must not be read here, the project can be unloaded concurrently.
    wxFileName         rootFilename( aProjectFullName );
    wxString           repoWorkDir = statusHandler.GetWorkingDirectory();

    wxFileName relative = rootFilename;
    relative.MakeRelativeTo( repoWorkDir );
    wxString pathspecStr = relative.GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR );

#ifdef _WIN32
    pathspecStr.Replace( wxS( "\\" ), wxS( "/" ) );
#endif

    // Get file status
    auto fileStatusMap = statusHandler.GetFileStatus( pathspecStr );
    auto [localChanges, remoteChanges] = aGitCommon->GetDifferentFiles();
    statusHandler.UpdateRemoteStatus( localChanges, remoteChanges, fileStatusMap );

    bool updated = false;

    // Update status icons based on file status
    for( const auto& [absPath, fileStatus] : fileStatusMap )
    {
        auto iter = m_gitTreeCache.find( absPath );
        if( iter == m_gitTreeCache.end() )
        {
            wxLogTrace( traceGit, wxS( "File '%s' not found in tree cache" ), absPath );
            continue;
        }

        auto [it, inserted] = m_gitStatusIcons.try_emplace( iter->second, fileStatus.status );
        if( inserted || it->second != fileStatus.status )
            updated = true;
        it->second = fileStatus.status;
    }

    // Get the current branch name
    wxString branchName = statusHandler.GetCurrentBranchName();

    if( branchName != m_gitCurrentBranchName )
        updated = true;

    m_gitCurrentBranchName = branchName;

    wxLogTrace( traceGit, wxS( "updateGitStatusIconMap: Updated git status icons" ) );

    // Update UI if icons changed
    if( updated || !m_gitIconsInitialized )
    {
        CallAfter(
                [this, aGitCommon]()
                {
                    updateGitStatusIcons();

                    // The docked panels read the same status, so they are stale until they
                    // are told. Without this a file edited outside KiCad shows as changed in
                    // the tree while the panel still says there is nothing to commit.
                    aGitCommon->NotifyRefreshListeners();
                } );
    }
}


void PROJECT_TREE_PANE::onRunSelectedJobsFile( wxCommandEvent& event )
{
}


void PROJECT_TREE_PANE::onGitSyncTimer( wxTimerEvent& aEvent )
{
    wxLogTrace( traceGit, "onGitSyncTimer" );
    COMMON_SETTINGS::GIT& gitSettings = Pgm().GetCommonSettings()->m_Git;

    if( !gitSettings.enableGit || gitSettings.updatInterval <= 0 || !m_TreeProject )
        return;

    // Skip the tick rather than stack a second fetch. Assigning over a running std::async
    // future blocks in its destructor, here on the UI thread.
    if( gitTaskStillRunning( m_gitSyncTask ) )
    {
        wxLogTrace( traceGit, "onGitSyncTimer: previous fetch still running, skipping" );
        m_gitSyncTimer.Start( gitSettings.updatInterval * 60 * 1000, wxTIMER_ONE_SHOT );
        return;
    }

    // Pin the session on the UI thread, so a concurrent release cannot free it while
    // PerformFetch() is inside it.
    std::shared_ptr<KIGIT_COMMON> common = gitCommon();

    // Never the shared compute pool. A libgit2 network read parked there stalls DRC, zone
    // filling and library loading.
    m_gitSyncTask = std::async( std::launch::async,
                                [this, common]()
                                {
                                    if( !common->GetRepo() )
                                    {
                                        wxLogTrace( traceGit, "onGitSyncTimer: No git repository found" );
                                        return;
                                    }

                                    // Check if cancellation was requested (e.g., during shutdown)
                                    if( common->IsShuttingDown() )
                                    {
                                        wxLogTrace( traceGit, "onGitSyncTimer: Cancelled" );
                                        return;
                                    }

                                    GIT_PULL_HANDLER handler( common.get() );
                                    bool                         fetched = handler.PerformFetch();
                                    std::optional<KIGIT_FAILURE> kind = handler.GetFailureKind();
                                    wxString                     summary = handler.PopError().summary;

                                    // Only schedule the follow-up work if not cancelled
                                    if( !common->IsShuttingDown() )
                                        CallAfter(
                                                [this, common, fetched, kind, summary]()
                                                {
                                                    // The project may have closed or changed while queued
                                                    if( common->IsShuttingDown() || common != gitCommon() )
                                                        return;

                                                    reportGitSyncResult( fetched, kind, summary );
                                                    gitStatusTimerHandler();

                                                    // The panels show whether the remote is ahead, and the fetch
                                                    // just moved the remote refs.
                                                    common->NotifyRefreshListeners();
                                                } );
                                } );

    wxLogTrace( traceGit, "onGitSyncTimer: Restarting git sync timer" );
    // We store the timer interval in minutes but wxTimer uses milliseconds
    m_gitSyncTimer.Start( gitSettings.updatInterval * 60 * 1000, wxTIMER_ONE_SHOT );
}


void PROJECT_TREE_PANE::reportGitSyncResult( bool aFetched, std::optional<KIGIT_FAILURE> aKind,
                                             const wxString& aSummary )
{
    KISTATUSBAR* statusBar = dynamic_cast<KISTATUSBAR*>( m_Parent->GetStatusBar() );

    if( aFetched )
    {
        KIGIT::ClearSyncWarnings( m_Parent );
        return;
    }

    // A busy repository or a closing project is not something to warn about
    if( !statusBar || !aKind || *aKind == KIGIT_FAILURE::CANCELLED )
        return;

    // The timer fires every few minutes, and the same failure each time would bury every other warning. The list
    // itself is the record, so anything that clears it lets the next failure show again
    std::map<wxString, std::vector<KI_ERROR>> shown = statusBar->GetWarningMessages();

    for( const KI_ERROR& existing : shown[KIGIT::SYNC_WARNING_SOURCE] )
    {
        if( existing.GetTitle() == aSummary )
            return;
    }

    KI_ERROR warning;
    warning.SetTitle( aSummary );
    warning.SetSeverity( RPT_SEVERITY_WARNING );
    statusBar->AddWarningMessages( KIGIT::SYNC_WARNING_SOURCE, { warning } );
}


void PROJECT_TREE_PANE::gitStatusTimerHandler()
{
    if( !m_TreeProject )
        return;

    // Pin the session on the UI thread, so a concurrent release cannot free it while
    // the status worker is using it.
    std::shared_ptr<KIGIT_COMMON> common = gitCommon();

    if( common->IsShuttingDown() )
        return;

    // See onGitSyncTimer: overwriting a still-running task's future abandons a task that holds
    // a raw pointer to this pane, and the destructor would no longer wait for it.
    if( gitTaskStillRunning( m_gitStatusIconTask ) )
        return;

    updateTreeCache();
    thread_pool& tp = GetKiCadThreadPool();

    wxString projectFullName = Prj().GetProjectFullName();

    m_gitStatusIconTask = tp.submit_task(
            [this, common, projectFullName]()
            {
                updateGitStatusIconMap( common, projectFullName );
            } );
}

void PROJECT_TREE_PANE::onGitStatusTimer( wxTimerEvent& aEvent )
{
    wxLogTrace( traceGit, "onGitStatusTimer" );

    if( !Pgm().GetCommonSettings()->m_Git.enableGit || !m_TreeProject )
        return;

    gitStatusTimerHandler();
}

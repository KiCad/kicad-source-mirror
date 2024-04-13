/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <stack>
#include <git2.h>

#include <wx/regex.h>
#include <wx/stdpaths.h>
#include <wx/string.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#include <advanced_config.h>
#include <bitmaps.h>
#include <bitmap_store.h>
#include <confirm.h>
#include <dialogs/git/dialog_git_commit.h>
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
#include <launch_ext.h>
#include <wx/dcclient.h>
#include <wx/settings.h>

#include <git/git_commit_handler.h>
#include <git/git_pull_handler.h>
#include <git/git_push_handler.h>
#include <git/git_resolve_conflict_handler.h>
#include <git/git_revert_handler.h>
#include <git/git_switch_branch_handler.h>
#include <git/git_compare_handler.h>
#include <git/git_remove_vcs_handler.h>
#include <git/git_add_to_index_handler.h>
#include <git/git_remove_from_index_handler.h>
#include <git/git_sync_handler.h>
#include <git/git_clone_handler.h>
#include <git/kicad_git_compat.h>


#include <dialogs/git/dialog_git_repository.h>

#include "project_tree_item.h"
#include "project_tree.h"
#include "pgm_kicad.h"
#include "kicad_id.h"
#include "kicad_manager_frame.h"

#include "project_tree_pane.h"
#include <widgets/kistatusbar.h>

#include <kiplatform/io.h>


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

// list of files extensions listed in the tree project window
// Add extensions in a compatible regex format to see others files types
static const wxChar* s_allowedExtensionsToList[] = {
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
    nullptr                        // end of list
};


/**
 * The frame that shows the tree list of files and subdirectories inside the working directory.
 *
 * Files are filtered (see s_allowedExtensionsToList) so only useful files are shown.
 */


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
    EVT_MENU( ID_GIT_COMMIT_PROJECT, PROJECT_TREE_PANE::onGitCommit )
    EVT_MENU( ID_GIT_COMMIT_FILE, PROJECT_TREE_PANE::onGitCommit )
    EVT_MENU( ID_GIT_SYNC_PROJECT, PROJECT_TREE_PANE::onGitSyncProject )
    EVT_MENU( ID_GIT_FETCH, PROJECT_TREE_PANE::onGitFetch )
    EVT_MENU( ID_GIT_PUSH, PROJECT_TREE_PANE::onGitPushProject )
    EVT_MENU( ID_GIT_PULL, PROJECT_TREE_PANE::onGitPullProject )
    EVT_MENU( ID_GIT_RESOLVE_CONFLICT, PROJECT_TREE_PANE::onGitResolveConflict )
    EVT_MENU( ID_GIT_REVERT_LOCAL, PROJECT_TREE_PANE::onGitRevertLocal )
    EVT_MENU( ID_GIT_SWITCH_BRANCH, PROJECT_TREE_PANE::onGitSwitchBranch )
    EVT_MENU( ID_GIT_COMPARE, PROJECT_TREE_PANE::onGitCompare )
    EVT_MENU( ID_GIT_REMOVE_VCS, PROJECT_TREE_PANE::onGitRemoveVCS )
    EVT_MENU( ID_GIT_ADD_TO_INDEX, PROJECT_TREE_PANE::onGitAddToIndex )
    EVT_MENU( ID_GIT_REMOVE_FROM_INDEX, PROJECT_TREE_PANE::onGitRemoveFromIndex )


    EVT_IDLE( PROJECT_TREE_PANE::onIdle )
    EVT_PAINT( PROJECT_TREE_PANE::onPaint )
END_EVENT_TABLE()


PROJECT_TREE_PANE::PROJECT_TREE_PANE( KICAD_MANAGER_FRAME* parent ) :
        wxSashLayoutWindow( parent, ID_LEFT_FRAME, wxDefaultPosition, wxDefaultSize,
                            wxNO_BORDER | wxTAB_TRAVERSAL )
{
    m_Parent = parent;
    m_TreeProject = nullptr;
    m_isRenaming = false;
    m_selectedItem = nullptr;
    m_watcherNeedReset = false;
    m_lastGitStatusUpdate = wxDateTime::Now();
    m_gitLastError = GIT_ERROR_NONE;

    m_watcher = nullptr;
    Connect( wxEVT_FSWATCHER,
             wxFileSystemWatcherEventHandler( PROJECT_TREE_PANE::onFileSystemEvent ) );

    Bind( wxEVT_SYS_COLOUR_CHANGED,
          wxSysColourChangedEventHandler( PROJECT_TREE_PANE::onThemeChanged ), this );

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
    shutdownFileWatcher();
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
    case TREE_FILE_TYPE::DRAWING_SHEET:         return FILEEXT::DrawingSheetFileExtension;
    case TREE_FILE_TYPE::FOOTPRINT_FILE:        return FILEEXT::KiCadFootprintFileExtension;
    case TREE_FILE_TYPE::SCHEMATIC_LIBFILE:     return FILEEXT::LegacySymbolLibFileExtension;
    case TREE_FILE_TYPE::SEXPR_SYMBOL_LIB_FILE: return FILEEXT::KiCadSymbolLibFileExtension;
    case TREE_FILE_TYPE::DESIGN_RULES:          return FILEEXT::DesignRulesFileExtension;
    case TREE_FILE_TYPE::ZIP_ARCHIVE:           return FILEEXT::ArchiveFileExtension;

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

static git_repository* get_git_repository_for_file( const char* filename )
{
    git_repository* repo = nullptr;
    git_buf         repo_path = GIT_BUF_INIT;

    // Find the repository path for the given file
    if( git_repository_discover( &repo_path, filename, 0, NULL ) )
    {
        #if 0
        printf( "get_git_repository_for_file: %s\n", git_error_last()->message ); fflush( 0 );
        #endif
        return nullptr;
    }

    if( git_repository_open( &repo, repo_path.ptr ) )
    {
        git_buf_free( &repo_path );
        return nullptr;
    }

    // Free the git_buf memory
    git_buf_free( &repo_path );

    return repo;
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

            if( reg.Compile( wxString::FromAscii( "^.*\\." ) + ext + wxString::FromAscii( "$" ),
                         wxRE_ICASE ) && reg.Matches( aName ) )
            {
                type = (TREE_FILE_TYPE) i;
                break;
            }
        }
    }

    wxString   file = wxFileNameFromPath( aName );
    wxFileName currfile( file );
    wxFileName project( m_Parent->GetProjectFileName() );

    // Ignore legacy projects with the same name as the current project
    if( ( type == TREE_FILE_TYPE::LEGACY_PROJECT )
            && ( currfile.GetName().CmpNoCase( project.GetName() ) == 0 ) )
    {
        return wxTreeItemId();
    }

    if( currfile.GetExt() == GetFileExt( TREE_FILE_TYPE::LEGACY_SCHEMATIC )
                || currfile.GetExt() == GetFileExt( TREE_FILE_TYPE::SEXPR_SCHEMATIC ) )
    {
        if( aProjectNames )
        {
            if( !alg::contains( *aProjectNames, currfile.GetName() ) )
                return wxTreeItemId();
        }
        else
        {
            PROJECT_TREE_ITEM*    parentTreeItem = GetItemIdData( aParent );
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
    if( type == TREE_FILE_TYPE::LEGACY_PROJECT || type == TREE_FILE_TYPE::JSON_PROJECT
        || type == TREE_FILE_TYPE::LEGACY_SCHEMATIC || type == TREE_FILE_TYPE::SEXPR_SCHEMATIC )
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
    wxString pro_dir = m_Parent->GetProjectFileName();

    if( !m_TreeProject )
        m_TreeProject = new PROJECT_TREE( this );
    else
        m_TreeProject->DeleteAllItems();

    if( !pro_dir )  // This is empty from PROJECT_TREE_PANE constructor
        return;

    if( m_TreeProject->GetGitRepo() )
    {
        git_repository_free( m_TreeProject->GetGitRepo() );
        m_TreeProject->SetGitRepo( nullptr );
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

    // Bind the git repository to the project tree (if it exists)
    if( ADVANCED_CFG::GetCfg().m_EnableGit )
    {
        m_TreeProject->SetGitRepo( get_git_repository_for_file( fn.GetPath().c_str() ) );
        m_TreeProject->GitCommon()->SetPassword( Prj().GetLocalSettings().m_GitRepoPassword );
        m_TreeProject->GitCommon()->SetUsername( Prj().GetLocalSettings().m_GitRepoUsername );
        m_TreeProject->GitCommon()->SetSSHKey( Prj().GetLocalSettings().m_GitSSHKey );

        wxString conn_type = Prj().GetLocalSettings().m_GitRepoType;

        if( conn_type == "https" )
            m_TreeProject->GitCommon()->SetConnType( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS );
        else if( conn_type == "ssh" )
            m_TreeProject->GitCommon()->SetConnType( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH );
        else
            m_TreeProject->GitCommon()->SetConnType( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL );
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
    PROJECT_TREE_ITEM* data = new PROJECT_TREE_ITEM( TREE_FILE_TYPE::JSON_PROJECT,
                                                  fn.GetFullPath(), m_TreeProject );

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
    updateGitStatusIcons();
}


bool PROJECT_TREE_PANE::hasChangedFiles()
{
    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo )
        return false;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
#pragma clang diagnostic pop

    opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX
                 | GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

    git_status_list* status_list = nullptr;
    int              error = git_status_list_new( &status_list, repo, &opts );

    if( error != GIT_OK )
        return false;

    bool has_changed_files = git_status_list_entrycount( status_list ) > 0;
    git_status_list_free( status_list );
    return has_changed_files;
}


void PROJECT_TREE_PANE::onRight( wxTreeEvent& Event )
{
    wxTreeItemId curr_item = Event.GetItem();

    // Ensure item is selected (Under Windows right click does not select the item)
    m_TreeProject->SelectItem( curr_item );

    std::vector<PROJECT_TREE_ITEM*> selection = GetSelectedData();

    bool can_switch_to_project = true;
    bool can_create_new_directory = true;
    bool can_open_this_directory = true;
    bool can_edit = true;
    bool can_rename = true;
    bool can_delete = true;

    bool vcs_has_repo    = m_TreeProject->GetGitRepo() != nullptr;
    bool vcs_can_commit  = hasChangedFiles();
    bool vcs_can_init    = !vcs_has_repo;
    bool vcs_can_remove  = vcs_has_repo;
    bool vcs_can_fetch   = vcs_has_repo && m_TreeProject->GitCommon()->HasPushAndPullRemote();
    bool vcs_can_push    = vcs_can_fetch && m_TreeProject->GitCommon()->HasLocalCommits();
    bool vcs_can_pull    = vcs_can_fetch;
    bool vcs_can_switch  = vcs_has_repo;
    bool vcs_menu        = ADVANCED_CFG::GetCfg().m_EnableGit;

    // Check if the libgit2 library has been successfully initialized
#if ( LIBGIT2_VER_MAJOR >= 1 ) || ( LIBGIT2_VER_MINOR >= 99 )
    int major, minor, rev;
    bool libgit_init = ( git_libgit2_version( &major, &minor, &rev ) == GIT_OK );
#else
    //Work around libgit2 API change for supporting older platforms
    bool libgit_init = true;
#endif

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


        vcs_menuitem = vcs_submenu->Append( ID_GIT_COMMIT_PROJECT, _( "Commit Project..." ),
                                            _( "Commit changes to the local repository" ) );
        vcs_menuitem->Enable( vcs_can_commit );

        vcs_menuitem = vcs_submenu->Append( ID_GIT_PUSH, _( "Push" ),
                                            _( "Push committed local changes to remote repository" ) );
        vcs_menuitem->Enable( vcs_can_push );

        vcs_menuitem = vcs_submenu->Append( ID_GIT_PULL, _( "Pull" ),
                                            _( "Pull changes from remote repository into local" ) );
        vcs_menuitem->Enable( vcs_can_pull );

        vcs_submenu->AppendSeparator();

        vcs_menuitem = vcs_submenu->Append( ID_GIT_COMMIT_FILE, _( "Commit File..." ),
                                            _( "Commit changes to the local repository" ) );
        vcs_menuitem->Enable( vcs_can_commit );

        vcs_submenu->AppendSeparator();

        // vcs_menuitem = vcs_submenu->Append( ID_GIT_COMPARE, _( "Diff" ),
        //                              _( "Show changes between the repository and working tree" ) );
        // vcs_menuitem->Enable( vcs_can_diff );

        std::vector<wxString> branchNames = m_TreeProject->GitCommon()->GetBranchNames();

        // Skip the first one (that is the current branch)
        for( size_t ii = 1; ii < branchNames.size() && ii < 6; ++ii )
        {
            wxString msg = _( "Switch to branch " ) + branchNames[ii];
            vcs_menuitem = branch_submenu->Append( ID_GIT_SWITCH_BRANCH + ii, branchNames[ii], msg );
            vcs_menuitem->Enable( vcs_can_switch );
        }

        vcs_menuitem = branch_submenu->Append( ID_GIT_SWITCH_BRANCH, _( "Other..." ),
                                     _( "Switch to a different branch" ) );
        vcs_menuitem->Enable( vcs_can_switch );

        vcs_submenu->Append( ID_GIT_SWITCH_BRANCH, _( "Switch to Branch" ), branch_submenu );

        vcs_submenu->AppendSeparator();

        vcs_menuitem = vcs_submenu->Append( ID_GIT_REMOVE_VCS, _( "Remove Version Control" ),
                             _( "Delete all version control files from the project directory." ) );
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

    if( m_selectedItem != nullptr )
    {
        // Activate launches a window which may run the event loop on top of us and cause OnIdle
        // to get called again, so be sure to block off the activation condition first.
        PROJECT_TREE_ITEM* item = m_selectedItem;
        m_selectedItem          = nullptr;

        item->Activate( this );
    }

    // Inside this routine, we rate limit to once per 2 seconds
    updateGitStatusIcons();
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
    wxArrayTreeItemIds             selection;
    std::vector<PROJECT_TREE_ITEM*> data;

    m_TreeProject->GetSelections( selection );

    for( auto it = selection.begin(); it != selection.end(); it++ )
    {
        PROJECT_TREE_ITEM* item = GetItemIdData( *it );
        if( !item )
        {
            wxLogDebug( "Null tree item returned for selection, dynamic_cast failed?" );
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

    const wxFileName& pathModified = event.GetPath();
    wxString subdir = pathModified.GetPath();
    wxString fn = pathModified.GetFullPath();

    switch( event.GetChangeType() )
    {
    case wxFSW_EVENT_DELETE:
    case wxFSW_EVENT_CREATE:
    case wxFSW_EVENT_RENAME:
        CallAfter( &PROJECT_TREE_PANE::updateGitStatusIcons );
        break;

    case wxFSW_EVENT_MODIFY:
        CallAfter( &PROJECT_TREE_PANE::updateGitStatusIcons );
        KI_FALLTHROUGH;
    case wxFSW_EVENT_ACCESS:
    default:
        return;
    }

    wxTreeItemId root_id = findSubdirTreeItem( subdir );

    if( !root_id.IsOk() )
        return;

    wxTreeItemIdValue  cookie;  // dummy variable needed by GetFirstChild()
    wxTreeItemId kid = m_TreeProject->GetFirstChild( root_id, cookie );

    switch( event.GetChangeType() )
    {
    case wxFSW_EVENT_CREATE:
    {
        wxTreeItemId newitem =
                addItemToProjectTree( pathModified.GetFullPath(), root_id, nullptr, true );

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
    }

    // Sort filenames by alphabetic order
    m_TreeProject->SortChildren( root_id );
}


void PROJECT_TREE_PANE::FileWatcherReset()
{
    m_watcherNeedReset = false;

    wxString prj_dir = wxPathOnly( m_Parent->GetProjectFileName() );

#if defined( _WIN32 )
    KISTATUSBAR* statusBar = static_cast<KISTATUSBAR*>( m_Parent->GetStatusBar() );

    if( KIPLATFORM::ENV::IsNetworkPath( prj_dir ) )
    {
        // Due to a combination of a bug in SAMBA sending bad change event IDs and wxWidgets
        // choosing to fault on an invalid event ID instead of sanely ignoring them we need to
        // avoid spawning a filewatcher. Unfortunately this punishes corporate environments with
        // Windows Server shares :/
        m_Parent->m_FileWatcherInfo = _( "Network path: not monitoring folder changes" );
        statusBar->SetEllipsedTextField( m_Parent->m_FileWatcherInfo, 1 );
        return;
    }
    else
    {
        m_Parent->m_FileWatcherInfo = _( "Local path: monitoring folder changes" );
        statusBar->SetEllipsedTextField( m_Parent->m_FileWatcherInfo, 1 );
    }
#endif

    // Prepare file watcher:
    if( m_watcher )
    {
        m_watcher->RemoveAll();
    }
    else
    {
        m_watcher = new wxFileSystemWatcher();
        m_watcher->SetOwner( this );
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
#ifdef __WINDOWS__
    m_watcher->AddTree( fn );
#else
    m_watcher->Add( fn );

    if( m_TreeProject->IsEmpty() )
        return;

    // Add subdirs
    wxTreeItemIdValue  cookie;
    wxTreeItemId       root_id = m_root;

    std::stack < wxTreeItemId > subdirs_id;

    wxTreeItemId kid = m_TreeProject->GetFirstChild( root_id, cookie );

    while( true )
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

            wxLogTrace( tracePathsAndFiles, "%s: add '%s'\n", __func__, TO_UTF8( path ) );

            if( wxFileName::IsDirReadable( path ) )   // linux whines about watching protected dir
            {
                fn.AssignDir( path );
                m_watcher->Add( fn );

                // if kid is a subdir, push in list to explore it later
                if( itemData->IsPopulated() && m_TreeProject->GetChildrenCount( kid ) )
                    subdirs_id.push( kid );
            }
        }

        kid = m_TreeProject->GetNextChild( root_id, cookie );
    }
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
    // Make sure we don't try to inspect the tree after we've deleted its items.
    shutdownFileWatcher();

    m_TreeProject->DeleteAllItems();

    // Remove the git repository when the project is unloaded
    if( m_TreeProject->GetGitRepo() )
    {
        git_repository_free( m_TreeProject->GetGitRepo() );
        m_TreeProject->SetGitRepo( nullptr );
    }
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
    m_leftWin->FileWatcherReset();
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

    // Check if the directory is already a git repository
    git_repository* repo = nullptr;
    int error = git_repository_open(&repo, dir.mb_str());

    if( error == 0 )
    {
        // Directory is already a git repository
        wxWindow* topLevelParent = wxGetTopLevelParent( this );

        DisplayInfoMessage( topLevelParent,
                            _( "The selected directory is already a git project." ) );
        git_repository_free( repo );
        return;
    }
    else
    {
        // Directory is not a git repository
        error = git_repository_init( &repo, dir.mb_str(), 0 );

        if( error != 0 )
        {
            git_repository_free( repo );

            if( m_gitLastError != git_error_last()->klass )
            {
                m_gitLastError = git_error_last()->klass;
                DisplayErrorMessage( m_parent, _( "Failed to initialize git project." ),
                                     git_error_last()->message );
            }

            return;
        }
        else
        {
            m_TreeProject->SetGitRepo( repo );
            m_gitLastError = GIT_ERROR_NONE;
        }
    }

    DIALOG_GIT_REPOSITORY dlg( wxGetTopLevelParent( this ), repo );

    dlg.SetTitle( _( "Set default remote" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    //Set up the git remote

    m_TreeProject->GitCommon()->SetConnType( dlg.GetRepoType() );
    m_TreeProject->GitCommon()->SetPassword( dlg.GetPassword() );
    m_TreeProject->GitCommon()->SetUsername( dlg.GetUsername() );
    m_TreeProject->GitCommon()->SetSSHKey( dlg.GetRepoSSHPath() );

    git_remote* remote = nullptr;
    wxString fullURL;

    if( dlg.GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
    {
        fullURL = dlg.GetUsername() + "@" + dlg.GetRepoURL();
    }
    else if( dlg.GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS )
    {
        fullURL = dlg.GetRepoURL().StartsWith( "https" ) ? "https://" : "http://";

        if( !dlg.GetUsername().empty() )
        {
            fullURL.append( dlg.GetUsername() );

            if( !dlg.GetPassword().empty() )
            {
                fullURL.append( wxS( ":" ) );
                fullURL.append( dlg.GetPassword() );
            }

            fullURL.append( wxS( "@" ) );
        }

        fullURL.append( dlg.GetBareRepoURL() );
    }
    else
    {
        fullURL = dlg.GetRepoURL();
    }


    error = git_remote_create_with_fetchspec( &remote, repo, "origin",
                                              fullURL.ToStdString().c_str(),
                                              "+refs/heads/*:refs/remotes/origin/*" );

    if( error != GIT_OK )
    {
        if( m_gitLastError != git_error_last()->klass )
        {
            m_gitLastError = git_error_last()->klass;
            DisplayErrorMessage( m_parent, _( "Failed to set default remote." ),
                                 git_error_last()->message );
        }

        return;
    }

    m_gitLastError = GIT_ERROR_NONE;

    GIT_PULL_HANDLER handler( repo );

    handler.SetConnType( m_TreeProject->GitCommon()->GetConnType() );
    handler.SetUsername( m_TreeProject->GitCommon()->GetUsername() );
    handler.SetPassword( m_TreeProject->GitCommon()->GetPassword() );
    handler.SetSSHKey( m_TreeProject->GitCommon()->GetSSHKey() );

    handler.SetProgressReporter( std::make_unique<WX_PROGRESS_REPORTER>( this,
                                                                         _( "Fetching Remote" ),
                                                                         1 ) );

    handler.PerformFetch();

    Prj().GetLocalSettings().m_GitRepoPassword = dlg.GetPassword();
    Prj().GetLocalSettings().m_GitRepoUsername = dlg.GetUsername();
    Prj().GetLocalSettings().m_GitSSHKey = dlg.GetRepoSSHPath();

    if( dlg.GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
        Prj().GetLocalSettings().m_GitRepoType = "ssh";
    else if( dlg.GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS )
        Prj().GetLocalSettings().m_GitRepoType = "https";
    else
        Prj().GetLocalSettings().m_GitRepoType = "local";
}


void PROJECT_TREE_PANE::onGitCompare( wxCommandEvent& aEvent )
{

}


void PROJECT_TREE_PANE::onGitPullProject( wxCommandEvent& aEvent )
{
    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo )
        return;

    GIT_PULL_HANDLER handler( repo );

    handler.SetConnType( m_TreeProject->GitCommon()->GetConnType() );
    handler.SetUsername( m_TreeProject->GitCommon()->GetUsername() );
    handler.SetPassword( m_TreeProject->GitCommon()->GetPassword() );
    handler.SetSSHKey( m_TreeProject->GitCommon()->GetSSHKey() );

    handler.SetProgressReporter( std::make_unique<WX_PROGRESS_REPORTER>( this,
                                                                         _( "Fetching Remote" ),
                                                                         1 ) );

    handler.PerformPull();
}


void PROJECT_TREE_PANE::onGitPushProject( wxCommandEvent& aEvent )
{
    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo )
        return;

    GIT_PUSH_HANDLER handler( repo );

    handler.SetConnType( m_TreeProject->GitCommon()->GetConnType() );
    handler.SetUsername( m_TreeProject->GitCommon()->GetUsername() );
    handler.SetPassword( m_TreeProject->GitCommon()->GetPassword() );
    handler.SetSSHKey( m_TreeProject->GitCommon()->GetSSHKey() );

    handler.SetProgressReporter( std::make_unique<WX_PROGRESS_REPORTER>( this,
                                                                         _( "Fetching Remote" ),
                                                                         1 ) );

    if( handler.PerformPush() != PushResult::Success )
    {
        wxString errorMessage = handler.GetErrorString();

        DisplayErrorMessage( m_parent, _( "Failed to push project" ), errorMessage );
    }
}


static int git_create_branch( git_repository* aRepo, wxString& aBranchName )
{
    git_oid        head_oid;

    if( int error = git_reference_name_to_id( &head_oid, aRepo, "HEAD" ) != 0 )
    {
        wxLogError( "Failed to lookup HEAD reference" );
        return error;
    }

    // Lookup the current commit object
    git_commit* commit = nullptr;
    if( int error = git_commit_lookup( &commit, aRepo, &head_oid ) != GIT_OK )
    {
        wxLogError( "Failed to lookup commit" );
        return error;
    }

    git_reference* branchRef = nullptr;

    if( git_branch_create( &branchRef, aRepo, aBranchName.mb_str(), commit, 0 ) != 0 )
    {
        wxLogError( "Failed to create branch" );
        git_commit_free( commit );
        return -1;
    }

    git_commit_free( commit );
    git_reference_free( branchRef );

    return 0;
}


void PROJECT_TREE_PANE::onGitSwitchBranch( wxCommandEvent& aEvent )
{
    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo )
        return;

    DIALOG_GIT_SWITCH dlg( wxGetTopLevelParent( this ), repo );

    int retval = dlg.ShowModal();
    wxString branchName = dlg.GetBranchName();

    if( retval == wxID_ADD )
        git_create_branch( repo, branchName );
    else if( retval != wxID_OK )
        return;

    // Retrieve the reference to the existing branch using libgit2
    git_reference* branchRef = nullptr;

    if( git_reference_lookup( &branchRef, repo, branchName.mb_str() ) != GIT_OK &&
        git_reference_dwim( &branchRef, repo, branchName.mb_str() ) != GIT_OK )
    {
        wxString errorMessage = wxString::Format( _( "Failed to lookup branch '%s': %s" ),
                                                  branchName, giterr_last()->message );
        DisplayError( m_parent, errorMessage );
        return;
    }

    const char* branchRefName = git_reference_name( branchRef );

    git_object* branchObj = nullptr;

    if( git_revparse_single( &branchObj, repo, branchName.mb_str() ) != 0 )
    {
        wxString errorMessage =
                wxString::Format( _( "Failed to find branch head for '%s'" ), branchName );
        DisplayError( m_parent, errorMessage );
        git_reference_free( branchRef );
        return;
    }


    // Switch to the branch
    if( git_checkout_tree( repo, branchObj, nullptr ) != 0 )
    {
        wxString errorMessage =
                wxString::Format( _( "Failed to switch to branch '%s'" ), branchName );
        DisplayError( m_parent, errorMessage );
        git_reference_free( branchRef );
        git_object_free( branchObj );
        return;
    }

    // Update the HEAD reference
    if( git_repository_set_head( repo, branchRefName ) != 0 )
    {
        wxString errorMessage = wxString::Format(
                _( "Failed to update HEAD reference for branch '%s'" ), branchName );
        DisplayError( m_parent, errorMessage );
        git_reference_free( branchRef );
        git_object_free( branchObj );
        return;
    }

    // Free resources
    git_reference_free( branchRef );
    git_object_free( branchObj );
}


void PROJECT_TREE_PANE::onGitRemoveVCS( wxCommandEvent& aEvent )
{
    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo
      || !IsOK( wxGetTopLevelParent( this ),
                _( "Are you sure you want to remove git tracking from this project?" ) ) )
    {
        return;
    }

    // Remove the VCS (git) from the project directory
    git_repository_free( repo );
    m_TreeProject->SetGitRepo( nullptr );

    // Remove the .git directory
    wxFileName fn( m_Parent->GetProjectFileName() );
    fn.AppendDir( ".git" );

    wxString errors;

    if( !RmDirRecursive( fn.GetPath(), &errors ) )
    {
        DisplayErrorMessage( m_parent, _( "Failed to remove git directory" ), errors );
    }

    // Clear all item states
    std::stack<wxTreeItemId> items;
    items.push( m_TreeProject->GetRootItem() );

    while( !items.empty() )
    {
        wxTreeItemId current = items.top();
        items.pop();

        // Process the current item
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


void PROJECT_TREE_PANE::updateGitStatusIcons()
{
    if( ADVANCED_CFG::GetCfg().m_EnableGit == false )
        return;

    if( !m_TreeProject )
        return;

    wxTimeSpan timeSinceLastUpdate = wxDateTime::Now() - m_lastGitStatusUpdate;

    if( timeSinceLastUpdate.Abs() < wxTimeSpan::Seconds( 2 ) )
        return;

    m_lastGitStatusUpdate = wxDateTime::Now();

    wxTreeItemId      kid = m_TreeProject->GetRootItem();

    if( !kid.IsOk() )
        return;

    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo )
        return;

    // Get Current Branch

    git_reference* currentBranchReference = nullptr;
    git_repository_head( &currentBranchReference, repo );

    // Get the current branch name
    if( currentBranchReference )
    {
        PROJECT_TREE_ITEM* rootItem = GetItemIdData( kid );
        wxString filename = wxFileNameFromPath( rootItem->GetFileName() );
        wxString branchName = git_reference_shorthand( currentBranchReference );

        m_TreeProject->SetItemText( kid, filename + " [" + branchName + "]" );
        git_reference_free( currentBranchReference );
    }
    else
    {
        if( giterr_last()->klass != m_gitLastError )
            wxLogError( "Failed to lookup current branch: %s", giterr_last()->message );

        m_gitLastError = giterr_last()->klass;
    }

    // Collect a map to easily set the state of each item
    std::map<wxString, wxTreeItemId> branchMap;
    {
        std::stack<wxTreeItemId> items;
        items.push( kid );

        while( !items.empty() )
        {
            kid = items.top();
            items.pop();

            PROJECT_TREE_ITEM* nextItem = GetItemIdData( kid );

            branchMap[nextItem->GetFileName()] = kid;

            wxTreeItemIdValue cookie;
            wxTreeItemId      child = m_TreeProject->GetFirstChild( kid, cookie );

            while( child.IsOk() )
            {
                items.push( child );
                child = m_TreeProject->GetNextChild( kid, cookie );
            }
        }
    }

    git_status_options status_options = GIT_STATUS_OPTIONS_INIT;
    status_options.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    status_options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_INCLUDE_UNMODIFIED;

    git_index* index = nullptr;

    if( git_repository_index( &index, repo ) != GIT_OK )
    {
        m_gitLastError = giterr_last()->klass;
        wxLogDebug( "Failed to get git index: %s", giterr_last()->message );
        return;
    }

    git_status_list* status_list = nullptr;

    if( git_status_list_new( &status_list, repo, &status_options ) != GIT_OK )
    {
        wxLogDebug( "Failed to get git status list: %s", giterr_last()->message );
        git_index_free( index );
        return;
    }

    auto [ localChanges, remoteChanges ] = m_TreeProject->GitCommon()->GetDifferentFiles();

    size_t count = git_status_list_entrycount( status_list );

    for( size_t ii = 0; ii < count; ++ii )
    {
        const git_status_entry* entry = git_status_byindex( status_list, ii );
        std::string path( entry->head_to_index? entry->head_to_index->old_file.path
                        : entry->index_to_workdir->old_file.path );
        wxFileName fn( path );
        fn.MakeAbsolute( git_repository_workdir( repo ) );

        auto iter = branchMap.find( fn.GetFullPath() );

        if( iter == branchMap.end() )
            continue;

        // If we are current, don't continue because we still need to check to see if the
        // current commit is ahead/behind the remote.  If the file is modified/added/deleted,
        // that is the main status we want to show.
        if( entry->status == GIT_STATUS_CURRENT )
        {
            m_TreeProject->SetItemState(
                    iter->second,
                    static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_CURRENT ) );
        }
        else if( entry->status & ( GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_WT_MODIFIED ) )
        {
            m_TreeProject->SetItemState(
                    iter->second,
                    static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_MODIFIED ) );
            continue;
        }
        else if( entry->status & ( GIT_STATUS_INDEX_NEW | GIT_STATUS_WT_NEW ) )
        {
            m_TreeProject->SetItemState(
                    iter->second,
                    static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_ADDED ) );
            continue;
        }
        else if( entry->status & ( GIT_STATUS_INDEX_DELETED | GIT_STATUS_WT_DELETED ) )
        {
            m_TreeProject->SetItemState(
                    iter->second,
                    static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_DELETED ) );
            continue;
        }

        // Check if file is up to date with the remote
        if( localChanges.count( path ) )
        {
            m_TreeProject->SetItemState(
                    iter->second,
                    static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_AHEAD ) );
            continue;
        }
        else if( remoteChanges.count( path ) )
        {
            m_TreeProject->SetItemState(
                    iter->second,
                    static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_BEHIND ) );
            continue;
        }
        else
        {
            m_TreeProject->SetItemState(
                    iter->second,
                    static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_CURRENT ) );
            continue;
        }
    }

    git_status_list_free( status_list );
    git_index_free( index );
}


void PROJECT_TREE_PANE::onGitCommit( wxCommandEvent& aEvent )
{
    std::vector<PROJECT_TREE_ITEM*> tree_data = GetSelectedData();

    git_repository* repo = m_TreeProject->GetGitRepo();

    if( repo == nullptr )
    {
        wxMessageBox( "The selected directory is not a git project." );
        return;
    }

    git_config* config = nullptr;
    git_repository_config( &config, repo );

    // Read relevant data from the git config
    wxString authorName;
    wxString authorEmail;

    // Read author name
    git_config_entry* name_c = nullptr;
    git_config_entry* email_c = nullptr;
    int authorNameError = git_config_get_entry( &name_c, config, "user.name" );

    if( authorNameError != 0 || name_c == nullptr )
    {
        authorName = Pgm().GetCommonSettings()->m_Git.authorName;
    }
    else
    {
        authorName = name_c->value;
        git_config_entry_free( name_c );
    }

    // Read author email
    int authorEmailError = git_config_get_entry( &email_c, config, "user.email" );

    if( authorEmailError != 0 || email_c == nullptr )
    {
        authorEmail = Pgm().GetCommonSettings()->m_Git.authorEmail;
    }
    else
    {
        authorEmail = email_c->value;
        git_config_entry_free( email_c );
    }

    // Free the config object
    git_config_free( config );

    // Collect modified files in the repository
    git_status_options status_options = GIT_STATUS_OPTIONS_INIT;
    status_options.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    status_options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED;

    git_status_list* status_list = nullptr;
    git_status_list_new( &status_list, repo, &status_options );

    std::map<wxString, int> modifiedFiles;

    size_t count = git_status_list_entrycount( status_list );

    std::set<wxString> selected_files;

    for( PROJECT_TREE_ITEM* item : tree_data )
    {
        if( item->GetType() != TREE_FILE_TYPE::DIRECTORY )
            selected_files.emplace( item->GetFileName() );
    }

    for( size_t i = 0; i < count; ++i )
    {
        const git_status_entry* entry = git_status_byindex( status_list, i );

        // Check if the file is modified (index or workdir changes)
        if( entry->status == GIT_STATUS_CURRENT
            || ( entry->status & ( GIT_STATUS_CONFLICTED | GIT_STATUS_IGNORED ) ) )
        {
            continue;
        }

        wxFileName fn( entry->index_to_workdir->old_file.path );
        fn.MakeAbsolute( git_repository_workdir( repo ) );

        wxString filePath( entry->index_to_workdir->old_file.path, wxConvUTF8 );

        if( aEvent.GetId() == ID_GIT_COMMIT_PROJECT )
        {
            modifiedFiles.emplace( filePath, entry->status );
        }
        else if( selected_files.count( fn.GetFullPath() ) )
        {
            modifiedFiles.emplace( filePath, entry->status );
        }
    }

    git_status_list_free( status_list );

    // Create a commit dialog
    DIALOG_GIT_COMMIT dlg( wxGetTopLevelParent( this ), repo, authorName, authorEmail,
                           modifiedFiles );
    auto              ret = dlg.ShowModal();

    if( ret == wxID_OK )
    {
        // Commit the changes
        git_oid     tree_id;
        git_tree*   tree = nullptr;
        git_commit* parent = nullptr;
        git_index*  index = nullptr;

        std::vector<wxString> files = dlg.GetSelectedFiles();

        if( dlg.GetCommitMessage().IsEmpty() )
        {
            wxMessageBox( _( "Discarding commit due to empty commit message." ) );
            return;
        }

        if( files.empty() )
        {
            wxMessageBox( _( "Discarding commit due to empty file selection." ) );
            return;
        }

        if( git_repository_index( &index, repo ) != 0 )
        {
            wxMessageBox( _( "Failed to get repository index: %s" ), giterr_last()->message );
            return;
        }

        for( wxString& file :files )
        {
            if( git_index_add_bypath( index, file.mb_str() ) != 0 )
            {
                wxMessageBox( _( "Failed to add file to index: %s" ), giterr_last()->message );
                git_index_free( index );
                return;
            }
        }

        if( git_index_write( index ) != 0 )
        {
            wxMessageBox( _( "Failed to write index: %s" ), giterr_last()->message );
            git_index_free( index );
            return;
        }

        if (git_index_write_tree( &tree_id, index ) != 0)
        {
            wxMessageBox( _( "Failed to write tree: %s" ), giterr_last()->message );
            git_index_free( index );
            return;
        }

        git_index_free( index );

        if( git_tree_lookup( &tree, repo, &tree_id ) != 0 )
        {
            wxMessageBox( _( "Failed to lookup tree: %s" ), giterr_last()->message );
            return;
        }

        git_reference* headRef = nullptr;

        if( git_repository_head( &headRef, repo ) != 0 )
        {
            wxMessageBox( _( "Failed to get HEAD reference: %s" ), giterr_last()->message );
            git_index_free( index );
            return;
        }

        if( git_reference_peel( (git_object**) &parent, headRef, GIT_OBJECT_COMMIT ) != 0 )
        {
            wxMessageBox( _( "Failed to get commit: %s" ), giterr_last()->message );
            git_reference_free( headRef );
            git_index_free( index );
            return;
        }

        git_reference_free( headRef );

        const wxString& commit_msg = dlg.GetCommitMessage();
        const wxString& author_name = dlg.GetAuthorName();
        const wxString& author_email = dlg.GetAuthorEmail();

        git_signature* author = nullptr;

        if( git_signature_now( &author, author_name.mb_str(), author_email.mb_str() ) != 0 )
        {
            wxMessageBox( _( "Failed to create author signature: %s" ), giterr_last()->message );
            return;
        }

        git_oid           oid;
        // Check if the libgit2 library version is 1.8.0 or higher
#if( LIBGIT2_VER_MAJOR > 1 ) || ( LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR >= 8 )
        // For libgit2 version 1.8.0 and above
        git_commit* const parents[1] = { parent };
#else
        // For libgit2 versions older than 1.8.0
        const git_commit* parents[1] = { parent };
#endif

        if( git_commit_create( &oid, repo, "HEAD", author, author, nullptr, commit_msg.mb_str(), tree,
                           1, parents ) != 0 )
        {
            wxMessageBox( _( "Failed to create commit: %s" ), giterr_last()->message );
            return;
        }

        git_signature_free( author );
        git_commit_free( parent );
        git_tree_free( tree );
    }
}

void PROJECT_TREE_PANE::onGitAddToIndex( wxCommandEvent& aEvent )
{

}


bool PROJECT_TREE_PANE::canFileBeAddedToVCS( const wxString& aFile )
{
    git_index *index;
    size_t entry_pos;

    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo )
        return false;

    if( git_repository_index( &index, repo ) != 0 )
        return false;

    // If we successfully find the file in the index, we may not add it to the VCS
    if( git_index_find( &entry_pos, index, aFile.mb_str() ) == 0 )
    {
        git_index_free( index );
        return false;
    }

    git_index_free( index );
    return true;
}


void PROJECT_TREE_PANE::onGitSyncProject( wxCommandEvent& aEvent )
{
    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo )
        return;

    GIT_SYNC_HANDLER handler( repo );
    handler.PerformSync();
}


void PROJECT_TREE_PANE::onGitFetch( wxCommandEvent& aEvent )
{
    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo )
        return;

    GIT_PULL_HANDLER handler( repo );
    handler.PerformFetch();
}

void PROJECT_TREE_PANE::onGitResolveConflict( wxCommandEvent& aEvent )
{
    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo )
        return;

    GIT_RESOLVE_CONFLICT_HANDLER handler( repo );
    handler.PerformResolveConflict();
}

void PROJECT_TREE_PANE::onGitRevertLocal( wxCommandEvent& aEvent )
{
    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo )
        return;

    GIT_REVERT_HANDLER handler( repo );
    handler.PerformRevert();
}

void PROJECT_TREE_PANE::onGitRemoveFromIndex( wxCommandEvent& aEvent )
{
    git_repository* repo = m_TreeProject->GetGitRepo();

    if( !repo )
        return;

    GIT_REMOVE_FROM_INDEX_HANDLER handler( repo );
    handler.PerformRemoveFromIndex();
}




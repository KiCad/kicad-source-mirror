/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/regex.h>
#include <wx/stdpaths.h>
#include <wx/string.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#include <bitmaps.h>
#include <bitmap_store.h>
#include <gestfich.h>
#include <macros.h>
#include <menus_helpers.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>
#include <kiplatform/environment.h>
#include <core/kicad_algo.h>
#include <paths.h>
#include <launch_ext.h>
#include <wx/dcclient.h>
#include <wx/settings.h>

#include "project_tree_item.h"
#include "project_tree.h"
#include "pgm_kicad.h"
#include "kicad_id.h"
#include "kicad_manager_frame.h"

#include "project_tree_pane.h"


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
    wxT( "^.*\\.txt$" ),
    wxT( "^.*\\.pho$" ),           // Gerber file (Old Kicad extension)
    wxT( "^.*\\.gbr$" ),           // Gerber file
    wxT( "^.*\\.gbrjob$" ),        // Gerber job file
    wxT( "^.*\\.gb[alops]$" ),     // Gerber back (or bottom) layer file (deprecated Protel ext)
    wxT( "^.*\\.gt[alops]$" ),     // Gerber front (or top) layer file (deprecated Protel ext)
    wxT( "^.*\\.g[0-9]{1,2}$" ),   // Gerber inner layer file (deprecated Protel ext)
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
    nullptr                        // end of list
};


/* TODO: Check if these file extension and wildcard definitions are used
 *       in any of the other KiCad programs and move them into the common
 *       library as required.
 */

// Gerber file extension wildcard.
const wxString GerberFileExtensionWildCard( ".((gbr|gbrjob|(gb|gt)[alops])|pho)" );


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

        wxMkdir( full_dirname );
        addItemToProjectTree( full_dirname, item_data->GetId(), nullptr, false );
    }
}


wxString PROJECT_TREE_PANE::GetFileExt( TREE_FILE_TYPE type )
{
    switch( type )
    {
    case TREE_FILE_TYPE::LEGACY_PROJECT:        return LegacyProjectFileExtension;
    case TREE_FILE_TYPE::JSON_PROJECT:          return ProjectFileExtension;
    case TREE_FILE_TYPE::LEGACY_SCHEMATIC:      return LegacySchematicFileExtension;
    case TREE_FILE_TYPE::SEXPR_SCHEMATIC:       return KiCadSchematicFileExtension;
    case TREE_FILE_TYPE::LEGACY_PCB:            return LegacyPcbFileExtension;
    case TREE_FILE_TYPE::SEXPR_PCB:             return KiCadPcbFileExtension;
    case TREE_FILE_TYPE::GERBER:                return GerberFileExtensionWildCard;
    case TREE_FILE_TYPE::GERBER_JOB_FILE:       return GerberJobFileExtension;
    case TREE_FILE_TYPE::HTML:                  return HtmlFileExtension;
    case TREE_FILE_TYPE::PDF:                   return PdfFileExtension;
    case TREE_FILE_TYPE::TXT:                   return TextFileExtension;
    case TREE_FILE_TYPE::NET:                   return NetlistFileExtension;
    case TREE_FILE_TYPE::CMP_LINK:              return FootprintAssignmentFileExtension;
    case TREE_FILE_TYPE::REPORT:                return ReportFileExtension;
    case TREE_FILE_TYPE::FP_PLACE:              return FootprintPlaceFileExtension;
    case TREE_FILE_TYPE::DRILL:                 return DrillFileExtension;
    case TREE_FILE_TYPE::DRILL_NC:              return "nc";
    case TREE_FILE_TYPE::DRILL_XNC:             return "xnc";
    case TREE_FILE_TYPE::SVG:                   return SVGFileExtension;
    case TREE_FILE_TYPE::DRAWING_SHEET:         return DrawingSheetFileExtension;
    case TREE_FILE_TYPE::FOOTPRINT_FILE:        return KiCadFootprintFileExtension;
    case TREE_FILE_TYPE::SCHEMATIC_LIBFILE:     return LegacySymbolLibFileExtension;
    case TREE_FILE_TYPE::SEXPR_SYMBOL_LIB_FILE: return KiCadSymbolLibFileExtension;
    case TREE_FILE_TYPE::DESIGN_RULES:          return DesignRulesFileExtension;
    default:                                    return wxEmptyString;
    }
}


std::vector<wxString> getProjects( const wxDir& dir )
{
    std::vector<wxString> projects;
    wxString              dir_filename;
    bool                  haveFile = dir.GetFirst( &dir_filename );

    while( haveFile  )
    {
        wxFileName file( dir_filename );

        if( file.GetExt() == LegacyProjectFileExtension || file.GetExt() == ProjectFileExtension )
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

    // Files/dirs names starting by "." are not visible files under unices.
    // Skip them also under Windows
    if( fn.GetName().StartsWith( wxT( "." ) ) )
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

            // For gerber files, the official ext is gbr
            if( i == static_cast<int>( TREE_FILE_TYPE::GERBER ) )
                ext = "gbr";

            reg.Compile( wxString::FromAscii( "^.*\\." ) + ext + wxString::FromAscii( "$" ),
                         wxRE_ICASE );

            if( reg.Matches( aName ) )
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
    data->SetRootFile( fileName == projName || fileName.StartsWith( projName + "-" ) );

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

    wxFileName fn = pro_dir;
    bool prjReset = false;

    if( !fn.IsOk() )
    {
        fn.Clear();
        fn.SetPath( PATHS::GetDefaultUserProjectsPath() );
        fn.SetName( NAMELESS_PROJECT );
        fn.SetExt( ProjectFileExtension );
        prjReset = true;
    }

    bool prjOpened = fn.FileExists();

    // We may have opened a legacy project, in which case GetProjectFileName will return the
    // name of the migrated (new format) file, which may not have been saved to disk yet.
    if( !prjOpened && !prjReset )
    {
        fn.SetExt( LegacyProjectFileExtension );
        prjOpened = fn.FileExists();

        // Set the ext back so that in the tree view we see the (not-yet-saved) new file
        fn.SetExt( ProjectFileExtension );
    }

    // root tree:
    m_root = m_TreeProject->AddRoot( fn.GetFullName(), static_cast<int>( TREE_FILE_TYPE::ROOT ),
                                     static_cast<int>( TREE_FILE_TYPE::ROOT ) );
    m_TreeProject->SetItemBold( m_root, true );

    // The main project file is now a JSON file
    m_TreeProject->SetItemData( m_root, new PROJECT_TREE_ITEM( TREE_FILE_TYPE::JSON_PROJECT,
                                                               fn.GetFullPath(), m_TreeProject ) );

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

        wxString full_file_name = item->GetFileName();

        switch( item->GetType() )
        {
        case TREE_FILE_TYPE::LEGACY_PROJECT:
        case TREE_FILE_TYPE::JSON_PROJECT:
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
        AddMenuItem( &popup_menu, ID_PROJECT_SWITCH_TO_OTHER,
                     _( "Switch to this Project" ),
                     _( "Close all editors, and switch to the selected project" ),
                     KiBitmap( BITMAPS::open_project ) );
        popup_menu.AppendSeparator();
    }

    if( can_create_new_directory )
    {
        AddMenuItem( &popup_menu, ID_PROJECT_NEWDIR, _( "New Directory..." ),
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

        AddMenuItem( &popup_menu, ID_PROJECT_OPEN_DIR, text, help_text,
                     KiBitmap( BITMAPS::directory_browser ) );
    }

    if( can_edit )
    {
        if( selection.size() == 1 )
            help_text = _( "Open the file in a Text Editor" );
        else
            help_text = _( "Open files in a Text Editor" );

        AddMenuItem( &popup_menu, ID_PROJECT_TXTEDIT, _( "Edit in a Text Editor" ),
                     help_text, KiBitmap( BITMAPS::editor ) );
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

        AddMenuItem( &popup_menu, ID_PROJECT_RENAME, text, help_text, KiBitmap( BITMAPS::right ) );
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
        AddMenuItem( &popup_menu, ID_PROJECT_DELETE, _( "Delete" ), help_text,
                     KiBitmap( BITMAPS::trash ) );
#else
        AddMenuItem( &popup_menu, ID_PROJECT_DELETE, _( "Move to Trash" ), help_text,
                     KiBitmap( BITMAPS::trash ) );
#endif
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
    wxString files;

    for( PROJECT_TREE_ITEM* item_data : tree_data )
    {
        wxString fullFileName = item_data->GetFileName();

        if( !files.IsEmpty() )
            files += " ";

        files += fullFileName;
    }

    ExecuteFile( editorname, files );
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
    wxTextEntryDialog dlg( this, msg, _( "Change filename" ), buffer );

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
        data.push_back( GetItemIdData( *it ) );

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
        break;

    case wxFSW_EVENT_MODIFY:
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

        if( newpath.Exists() && ( newfn != rootData->GetFileName() ) )
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
    if( KIPLATFORM::ENV::IsNetworkPath( prj_dir ) )
    {
        // Due to a combination of a bug in SAMBA sending bad change event IDs and wxWidgets
        // choosing to fault on an invalid event ID instead of sanely ignoring them we need to
        // avoid spawning a filewatcher. Unfortunately this punishes corporate environments with
        // Windows Server shares :/
        m_Parent->SetStatusText( _( "Network path: not monitoring folder changes" ), 1 );
        return;
    }
    else
    {
        m_Parent->SetStatusText( _( "Local path: monitoring folder changes" ), 1 );
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



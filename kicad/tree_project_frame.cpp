/**
 * @file tree_project_frame.cpp
 * @brief Function to build the tree of files in the current project directory
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <confirm.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <macros.h>

#include <tree_project_frame.h>
#include <class_treeprojectfiles.h>
#include <class_treeproject_item.h>
#include <wildcards_and_files_ext.h>

#include <wx/regex.h>
#include <wx/dir.h>
#include <wx/imaglist.h>
#include <menus_helpers.h>
#include <stack>


/* Note about the tree project build process:
 * Building the tree project can be *very* long if there are a lot of subdirectories
 * in the working directory.
 * Unfortunately, this happens easily if the project file *.pro is in the home directory
 * So the tree project is built "on demand":
 * First the tree is built from the current directory and shows files and subdirs.
 *   > First level subdirs trees are built (i.e subdirs contents are not read)
 *   > When expanding a subdir, each subdir contains is read,
 *     and the corresponding sub tree is populated on the fly.
 */

// list of files extensions listed in the tree project window
// *.sch files are always allowed, do not add here
// Add extensions in a compatible regex format to see others files types
static const wxChar* s_allowedExtensionsToList[] =
{
    wxT( "^.*\\.pro$" ),
    wxT( "^.*\\.pdf$" ),
    wxT( "^[^$].*\\.brd$" ),        // Legacy Pcbnew files
    wxT( "^[^$].*\\.kicad_pcb$" ),  // S format Pcbnew board files
    wxT( "^[^$].*\\.kicad_wks$" ),  // S format kicad page layout descr files
    wxT( "^[^$].*\\.kicad_mod$" ),  // S format kicad footprint files, currently not listed
    wxT( "^.*\\.net$" ),
    wxT( "^.*\\.lib$" ),            // Schematic library file
    wxT( "^.*\\.txt$" ),
    wxT( "^.*\\.pho$" ),            // Gerber file (Old Kicad extension)
    wxT( "^.*\\.gbr$" ),            // Gerber file
    wxT( "^.*\\.gb[alops]$" ),      // Gerber back (or bottom) layer file
    wxT( "^.*\\.gt[alops]$" ),      // Gerber front (or top) layer file
    wxT( "^.*\\.g[0-9]{1,2}$" ),    // Gerber inner layer file
    wxT( "^.*\\.odt$" ),
    wxT( "^.*\\.htm$" ),
    wxT( "^.*\\.html$" ),
    wxT( "^.*\\.rpt$" ),            // Report files
    wxT( "^.*\\.csv$" ),            // Report files in comma separated format
    wxT( "^.*\\.pos$" ),            // Footprint position files
    wxT( "^.*\\.cmp$" ),            // Cvpcb cmp/footprint link files
    wxT( "^.*\\.drl$" ),            // Excellon drill files
    wxT( "^.*\\.svg$" ),            // SVG print/plot files
    NULL                            // end of list
};


/* TODO: Check if these file extension and wildcard definitions are used
 *       in any of the other KiCad programs and move them into the common
 *       library as required.
 */

// File extension definitions.
const wxChar  TextFileExtension[] = wxT( "txt" );

// File wildcard definitions.
const wxChar  TextFileWildcard[] = wxT( "Text files (*.txt)|*.txt" );


/**
 * @brief class TREE_PROJECT_FRAME is the frame that shows the tree list
 * of files and subdirs inside the working directory
 * Files are filtered (see s_allowedExtensionsToList) so
 * only useful files are shown.
 */


BEGIN_EVENT_TABLE( TREE_PROJECT_FRAME, wxSashLayoutWindow )
    EVT_TREE_ITEM_ACTIVATED( ID_PROJECT_TREE, TREE_PROJECT_FRAME::OnSelect )
    EVT_TREE_ITEM_EXPANDED( ID_PROJECT_TREE, TREE_PROJECT_FRAME::OnExpand )
    EVT_TREE_ITEM_RIGHT_CLICK( ID_PROJECT_TREE, TREE_PROJECT_FRAME::OnRight )
    EVT_MENU( ID_PROJECT_TXTEDIT, TREE_PROJECT_FRAME::OnOpenSelectedFileWithTextEditor )
    EVT_MENU( ID_PROJECT_NEWDIR, TREE_PROJECT_FRAME::OnCreateNewDirectory )
    EVT_MENU( ID_PROJECT_DELETE, TREE_PROJECT_FRAME::OnDeleteFile )
    EVT_MENU( ID_PROJECT_RENAME, TREE_PROJECT_FRAME::OnRenameFile )
END_EVENT_TABLE()


TREE_PROJECT_FRAME::TREE_PROJECT_FRAME( KICAD_MANAGER_FRAME* parent ) :
    wxSashLayoutWindow( parent,
                        ID_LEFT_FRAME,
                        wxDefaultPosition,
                        wxDefaultSize,
                        wxNO_BORDER | wxSW_3D | wxTAB_TRAVERSAL )
{
    m_Parent = parent;
    m_TreeProject = NULL;

    m_watcher = NULL;
    Connect( wxEVT_FSWATCHER,
             wxFileSystemWatcherEventHandler( TREE_PROJECT_FRAME::OnFileSystemEvent ) );

    /*
     * Filtering is now inverted: the filters are actually used to _enable_ support
     * for a given file type.
     */

    // NOTE: sch filter must be first because of a test in AddFile() below
    m_filters.push_back( wxT( "^.*\\.sch$" ) );

    for( int ii = 0; s_allowedExtensionsToList[ii] != NULL; ii++ )
        m_filters.push_back( s_allowedExtensionsToList[ii] );

    m_filters.push_back( wxT( "^no KiCad files found" ) );

    ReCreateTreePrj();
}


TREE_PROJECT_FRAME::~TREE_PROJECT_FRAME()
{
    delete m_watcher;
}


void TREE_PROJECT_FRAME::RemoveFilter( const wxString& filter )
{
    for( unsigned int i = 0; i < m_filters.size(); i++ )
    {
        if( filter == m_filters[i] )
        {
            m_filters.erase( m_filters.begin() + i );
            return;
        }
    }
}


void TREE_PROJECT_FRAME::OnCreateNewDirectory( wxCommandEvent& event )
{
    // Get the root directory name:
    TREEPROJECT_ITEM* treeData = GetSelectedData();

    if( !treeData )
        return;

    TreeFileType    rootType = treeData->GetType();
    wxTreeItemId    root;

    if( TREE_DIRECTORY == rootType )
    {
        root = m_TreeProject->GetSelection();
    }
    else
    {
        root = m_TreeProject->GetItemParent( m_TreeProject->GetSelection() );

        if( !root.IsOk() )
            root = m_TreeProject->GetSelection();
    }

    wxString prj_dir = wxPathOnly( m_Parent->GetProjectFileName() );

    // Ask for the new sub directory name
    wxString curr_dir = treeData->GetDir();

    if( !curr_dir.IsEmpty() )    // A subdir is selected
    {
        // Make this subdir name relative to the current path.
        // It will be more easy to read by the user, in the next dialog
        wxFileName fn;
        fn.AssignDir( curr_dir );
        fn.MakeRelativeTo( prj_dir );
        curr_dir = fn.GetPath();

        if( !curr_dir.IsEmpty() )
            curr_dir += wxFileName::GetPathSeparator();
    }

    wxString    msg = wxString::Format( _( "Current project directory:\n%s" ), GetChars( prj_dir ) );
    wxString    subdir = wxGetTextFromUser( msg, _( "Create New Directory" ), curr_dir );

    if( subdir.IsEmpty() )
        return;

    wxString full_dirname = prj_dir + wxFileName::GetPathSeparator() + subdir;

    if( wxMkdir( full_dirname ) )
    {
        // the new itel will be added by the file watcher
        // AddItemToTreeProject( subdir, root );
    }
}


wxString TREE_PROJECT_FRAME::GetFileExt( TreeFileType type )
{
    wxString ext;

    switch( type )
    {
    case TREE_PROJECT:
        ext = ProjectFileExtension;
        break;

    case TREE_SCHEMA:
        ext = SchematicFileExtension;
        break;

    case TREE_LEGACY_PCB:
        ext = LegacyPcbFileExtension;
        break;

    case TREE_SEXP_PCB:
        ext = KiCadPcbFileExtension;
        break;

    case TREE_GERBER:
        ext = GerberFileExtension;
        break;

    case TREE_HTML:
        ext = HtmlFileExtension;
        break;

    case TREE_PDF:
        ext = PdfFileExtension;
        break;

    case TREE_TXT:
        ext = TextFileExtension;
        break;

    case TREE_NET:
        ext = NetlistFileExtension;
        break;

    case TREE_CMP_LINK:
        ext = ComponentFileExtension;
        break;

    case TREE_REPORT:
        ext = ReportFileExtension;
        break;

    case TREE_FP_PLACE:
        ext = FootprintPlaceFileExtension;
        break;

    case TREE_DRILL:
        ext = DrillFileExtension;
        break;

    case TREE_SVG:
        ext = SVGFileExtension;
        break;

    case TREE_PAGE_LAYOUT_DESCR:
        ext = PageLayoutDescrFileExtension;
        break;

    case TREE_FOOTPRINT_FILE:
        ext = KiCadFootprintFileExtension;
        break;

    case TREE_SCHEMATIC_LIBFILE:
        ext = SchematicLibraryFileExtension;
        break;

    default:                       // Eliminates unnecessary GCC warning.
        break;
    }

    return ext;
}


/*
 * Return the wxFileDialog wildcard string for the selected file type.
 */
wxString TREE_PROJECT_FRAME::GetFileWildcard( TreeFileType type )
{
    wxString ext;

    switch( type )
    {
    case TREE_PROJECT:
        ext = ProjectFileWildcard;
        break;

    case TREE_SCHEMA:
        ext = SchematicFileWildcard;
        break;

    case TREE_LEGACY_PCB:
    case TREE_SEXP_PCB:
        ext = PcbFileWildcard;
        break;

    case TREE_GERBER:
        ext = GerberFileWildcard;
        break;

    case TREE_HTML:
        ext = HtmlFileWildcard;
        break;

    case TREE_PDF:
        ext = PdfFileWildcard;
        break;

    case TREE_TXT:
        ext = TextFileWildcard;
        break;

    case TREE_NET:
        ext = NetlistFileWildcard;
        break;

    case TREE_CMP_LINK:
        ext = ComponentFileWildcard;
        break;

    case TREE_REPORT:
        ext = ReportFileWildcard;
        break;

    case TREE_FP_PLACE:
        ext = FootprintPlaceFileWildcard;
        break;

    case TREE_DRILL:
        ext = DrillFileWildcard;
        break;

    case TREE_SVG:
        ext = SVGFileWildcard;
        break;

    case TREE_PAGE_LAYOUT_DESCR:
        ext = PageLayoutDescrFileWildcard;
        break;

    case TREE_FOOTPRINT_FILE:
        ext = KiCadFootprintLibFileWildcard;
        break;

    case TREE_SCHEMATIC_LIBFILE:
        ext = SchematicLibraryFileWildcard;
        break;

     default:                       // Eliminates unnecessary GCC warning.
        break;
    }

    return ext;
}


bool TREE_PROJECT_FRAME::AddItemToTreeProject( const wxString& aName,
                                        wxTreeItemId& aRoot, bool aRecurse )
{
    wxTreeItemId    cellule;

    // Check the file type
    TreeFileType    type = TREE_UNKNOWN;

    // Skip not visible files and dirs
    wxFileName      fn( aName );

    // Files/dirs names starting by "." are not visible files under unices.
    // Skip them also under Windows
    if( fn.GetName().StartsWith( wxT( "." ) ) )
        return false;

    if( wxDirExists( aName ) )
    {
        type = TREE_DIRECTORY;
    }
    else
    {
        // Filter
        wxRegEx reg;

        bool    isSchematic = false;
        bool    addFile     = false;

        for( unsigned i = 0; i < m_filters.size(); i++ )
        {
            wxCHECK2_MSG( reg.Compile( m_filters[i], wxRE_ICASE ), continue,
                          wxT( "Regular expression " ) + m_filters[i] +
                          wxT( " failed to compile." ) );

            if( reg.Matches( aName ) )
            {
                addFile = true;

                if( i==0 )
                    isSchematic = true;

                break;
            }
        }

        if( !addFile )
            return false;

        // only show the schematic if it is a top level schematic.  Eeschema
        // cannot open a schematic and display it properly unless it starts
        // at the top of the hierarchy.  The schematic is top level only if
        // there is a line in the header saying:
        // "Sheet 1 "
        // However if the file has the same name as the project, it is always
        // shown, because it is expected the root sheet.
        // (and to fix an issue (under XP but could exist under other OS),
        // when a .sch file is created, the file
        // create is sent to the wxFileSystemWatcher, but the file still has 0 byte
        // so it cannot detected as root sheet
        // This is an ugly fix.
        if( isSchematic )
        {
            wxString    fullFileName = aName.BeforeLast( '.' );
            wxString    rootName;
            TREEPROJECT_ITEM* itemData = GetItemIdData( m_root );
            if( itemData )
                rootName = itemData->GetFileName().BeforeLast( '.' );

            if( fullFileName != rootName )
            {
                char        line[128]; // small because we just need a few bytes from the start of a line
                FILE*       fp;

                fullFileName = aName;
                fp = wxFopen( fullFileName, wxT( "rt" ) );

                if( fp == NULL )
                    return false;

                addFile = false;

                // check the first 100 lines for the "Sheet 1" string
                for( int i = 0; i<100; ++i )
                {
                    if( !fgets( line, sizeof(line), fp ) )
                        break;

                    if( !strncmp( line, "Sheet 1 ", 8 ) )
                    {
                        addFile = true;
                        break;
                    }
                }

                fclose( fp );

                if( !addFile )
                    return false; // it is a non-top-level schematic
            }
        }

        for( int i = TREE_PROJECT; i < TREE_MAX; i++ )
        {
            wxString ext = GetFileExt( (TreeFileType) i );

            if( ext == wxT( "" ) )
                continue;

            reg.Compile( wxString::FromAscii( "^.*\\" ) + ext +
                         wxString::FromAscii( "$" ), wxRE_ICASE );

            if( reg.Matches( aName ) )
            {
                type = (TreeFileType) i;
                break;
            }
        }
    }

    // also check to see if it is already there.
    wxTreeItemIdValue   cookie;
    wxTreeItemId        kid = m_TreeProject->GetFirstChild( aRoot, cookie );

    while( kid.IsOk() )
    {
        TREEPROJECT_ITEM* itemData = GetItemIdData( kid );

        if( itemData )
        {
            if( itemData->GetFileName() == aName )
            {
                return true;    // well, we would have added it, but it is already here!
            }
        }

        kid = m_TreeProject->GetNextChild( aRoot, cookie );
    }

    // Append the item (only appending the filename not the full path):
    wxString            file = wxFileNameFromPath( aName );
    cellule = m_TreeProject->AppendItem( aRoot, file );
    TREEPROJECT_ITEM*   data = new TREEPROJECT_ITEM( type, aName, m_TreeProject );

    m_TreeProject->SetItemData( cellule, data );
    data->SetState( 0 );

    // Mark root files (files which have the same aName as the project)
    wxFileName  project( m_Parent->GetProjectFileName() );
    wxFileName  currfile( file );

    if( currfile.GetName().CmpNoCase( project.GetName() ) == 0 )
        data->SetRootFile( true );
    else
        data->SetRootFile( false );

    // This section adds dirs and files found in the subdirs
    // in this case AddFile is recursive, but for the first level only.
    if( TREE_DIRECTORY == type && aRecurse )
    {
        wxDir   dir( aName );

        if( dir.IsOpened() )    // protected dirs will not open properly.
        {
            wxString        dir_filename;

            data->SetPopulated( true );

            if( dir.GetFirst( &dir_filename ) )
            {
                do    // Add name in tree, but do not recurse
                {
                    wxString path = aName + wxFileName::GetPathSeparator() + dir_filename;
                    AddItemToTreeProject( path, cellule, false );
                } while( dir.GetNext( &dir_filename ) );
            }
        }

        // Sort filenames by alphabetic order
        m_TreeProject->SortChildren( cellule );
    }

    return true;
}


void TREE_PROJECT_FRAME::ReCreateTreePrj()
{
    wxTreeItemId    rootcellule;
    bool            prjOpened = false;
    wxString        pro_dir = m_Parent->GetProjectFileName();

    if( !m_TreeProject )
        m_TreeProject = new TREEPROJECTFILES( this );
    else
        m_TreeProject->DeleteAllItems();

    if( !pro_dir )  // This is empty from TREE_PROJECT_FRAME constructor
        return;

    wxFileName fn = pro_dir;

    if( !fn.IsOk() )
    {
        fn.Clear();
        fn.SetPath( ::wxGetCwd() );
        fn.SetName( NAMELESS_PROJECT );
        fn.SetExt( ProjectFileExtension );
    }

    prjOpened = fn.FileExists();

    // root tree:
    m_root = rootcellule = m_TreeProject->AddRoot( fn.GetFullName(),
                                                   TREE_PROJECT - 1,
                                                   TREE_PROJECT - 1 );

    m_TreeProject->SetItemBold( rootcellule, true );

    m_TreeProject->SetItemData( rootcellule,
                                new TREEPROJECT_ITEM( TREE_PROJECT,
                                                      fn.GetFullPath(),
                                                      m_TreeProject ) );

    // Now adding all current files if available
    if( prjOpened )
    {
        wxString    pro_dir = wxPathOnly( m_Parent->GetProjectFileName() );
        wxDir       dir( pro_dir );

        if( dir.IsOpened() )    // protected dirs will not open, see "man opendir()"
        {
            wxString    filename;
            bool        cont = dir.GetFirst( &filename );

            while( cont )
            {
                if( filename != fn.GetFullName() )
                {
                    wxString name = dir.GetName() + wxFileName::GetPathSeparator() + filename;
                    AddItemToTreeProject( name, m_root );
                }

                cont = dir.GetNext( &filename );
            }
        }
    }
    else
    {
        m_TreeProject->AppendItem( m_root, wxT( "Empty project" ) );
    }

    m_TreeProject->Expand( rootcellule );

    // Sort filenames by alphabetic order
    m_TreeProject->SortChildren( m_root );
}


void TREE_PROJECT_FRAME::OnRight( wxTreeEvent& Event )
{
    int                 tree_id;
    TREEPROJECT_ITEM*   tree_data;
    wxString            fullFileName;
    wxTreeItemId        curr_item = Event.GetItem();

    // Ensure item is selected (Under Windows right click does not select the item)
    m_TreeProject->SelectItem( curr_item );

    tree_data = GetSelectedData();

    if( !tree_data )
        return;

    tree_id = tree_data->GetType();
    fullFileName = tree_data->GetFileName();

    wxMenu popupMenu;

    switch( tree_id )
    {
        case TREE_PROJECT:
            AddMenuItem( &popupMenu, ID_PROJECT_NEWDIR,
                         _( "New D&irectory" ),
                         _( "Create a New Directory" ),
                         KiBitmap( directory_xpm ) );
            break;

        case TREE_DIRECTORY:
            AddMenuItem( &popupMenu, ID_PROJECT_NEWDIR,
                         _( "New D&irectory" ),
                         _( "Create a New Directory" ),
                         KiBitmap( directory_xpm ) );
            AddMenuItem( &popupMenu,  ID_PROJECT_DELETE,
                         _( "&Delete Directory" ),
                         _( "Delete the Directory and its content" ),
                         KiBitmap( delete_xpm ) );
            break;

        default:
            AddMenuItem( &popupMenu, ID_PROJECT_TXTEDIT,
                         _( "&Edit in a text editor" ),
                         _( "Open the file in a Text Editor" ),
                         KiBitmap( icon_txt_xpm ) );
            AddMenuItem( &popupMenu, ID_PROJECT_RENAME,
                         _( "&Rename file" ),
                         _( "Rename file" ),
                         KiBitmap( right_xpm ) );
            AddMenuItem( &popupMenu,  ID_PROJECT_DELETE,
                         _( "&Delete File" ),
                         _( "Delete the Directory and its content" ),
                         KiBitmap( delete_xpm ) );
            break;
    }

    PopupMenu( &popupMenu );
}


void TREE_PROJECT_FRAME::OnOpenSelectedFileWithTextEditor( wxCommandEvent& event )
{
    TREEPROJECT_ITEM* tree_data = GetSelectedData();

    if( !tree_data )
        return;

    if( tree_data->GetType() == TREE_DIRECTORY )
        return;

    wxString    fullFileName = tree_data->GetFileName();
    AddDelimiterString( fullFileName );
    wxString    editorname = Pgm().GetEditorName();

    if( !editorname.IsEmpty() )
        ExecuteFile( this, editorname, fullFileName );
}


void TREE_PROJECT_FRAME::OnDeleteFile( wxCommandEvent& )
{
    TREEPROJECT_ITEM* tree_data = GetSelectedData();

    if( !tree_data )
        return;

    tree_data->Delete();
}


void TREE_PROJECT_FRAME::OnRenameFile( wxCommandEvent& )
{
    wxTreeItemId        curr_item   = m_TreeProject->GetSelection();
    TREEPROJECT_ITEM*   tree_data   = GetSelectedData();

    if( !tree_data )
        return;

    wxString buffer = m_TreeProject->GetItemText( curr_item );
    wxString msg = wxString::Format(
                    _( "Change filename: '%s'" ),
                    GetChars( tree_data->GetFileName() ) );

    wxTextEntryDialog   dlg( this, msg, _( "Change filename" ), buffer );

    if( dlg.ShowModal() != wxID_OK )
        return; // canceled by user

    buffer = dlg.GetValue();
    buffer.Trim( true );
    buffer.Trim( false );

    if( buffer.IsEmpty() )
        return; // empty file name not allowed

    if( tree_data->Rename( buffer, true ) )
        m_TreeProject->SetItemText( curr_item, buffer );
}


void TREE_PROJECT_FRAME::OnSelect( wxTreeEvent& Event )
{
    TREEPROJECT_ITEM*   tree_data = GetSelectedData();

    if( !tree_data )
        return;

    tree_data->Activate( this );
}


void TREE_PROJECT_FRAME::OnExpand( wxTreeEvent& Event )
{
    wxTreeItemId        itemId      = Event.GetItem();
    TREEPROJECT_ITEM*   tree_data   = GetItemIdData( itemId );

    if( !tree_data )
        return;

    if( tree_data->GetType() != TREE_DIRECTORY )
        return;

    // explore list of non populated subdirs, and populate them
    wxTreeItemIdValue   cookie;
    wxTreeItemId        kid = m_TreeProject->GetFirstChild( itemId, cookie );

    bool subdir_populated = false;

    for( ; kid.IsOk(); kid = m_TreeProject->GetNextChild( itemId, cookie ) )
    {
        TREEPROJECT_ITEM* itemData = GetItemIdData( kid );

        if( !itemData || itemData->GetType() != TREE_DIRECTORY )
            continue;

        if( itemData->IsPopulated() )
            continue;

        wxString    fileName = itemData->GetFileName();
        wxDir       dir( fileName );

        if( dir.IsOpened() )
        {
            wxString    dir_filename;

            if( dir.GetFirst( &dir_filename ) )
            {
                do    // Add name to tree item, but do not recurse in subdirs:
                {
                    wxString name = fileName + wxFileName::GetPathSeparator() + dir_filename;
                    AddItemToTreeProject( name, kid, false );
                } while( dir.GetNext( &dir_filename ) );
            }

            itemData->SetPopulated( true );       // set state to populated
            subdir_populated = true;
        }

        // Sort filenames by alphabetic order
        m_TreeProject->SortChildren( kid );
    }

    if( subdir_populated )
    {
    #ifndef __WINDOWS__
        FileWatcherReset();
    #endif
    }
}


TREEPROJECT_ITEM* TREE_PROJECT_FRAME::GetSelectedData()
{
    return dynamic_cast<TREEPROJECT_ITEM*>( m_TreeProject->GetItemData
                                            ( m_TreeProject->GetSelection() ) );
}


TREEPROJECT_ITEM* TREE_PROJECT_FRAME::GetItemIdData( wxTreeItemId aId )
{
    return dynamic_cast<TREEPROJECT_ITEM*>( m_TreeProject->GetItemData( aId ) );
}


wxTreeItemId TREE_PROJECT_FRAME::findSubdirTreeItem( const wxString& aSubDir )
{
    wxString prj_dir = wxPathOnly( m_Parent->GetProjectFileName() );

    // If the subdir is the current working directory, return m_root
    // in main list:
    if( prj_dir == aSubDir )
        return m_root;

    // The subdir is in the main tree or in a subdir: Locate it
    wxTreeItemIdValue  cookie;
    wxTreeItemId       root_id = m_root;
    std::stack < wxTreeItemId > subdirs_id;

    wxTreeItemId kid = m_TreeProject->GetFirstChild( root_id, cookie );

    while( 1 )
    {
        if( ! kid.IsOk() )
        {
            if( subdirs_id.empty() )    // all items were explored
            {
                root_id = kid;          // Not found: return an invalid wxTreeItemId
                break;
            }
            else
            {
                root_id = subdirs_id.top();
                subdirs_id.pop();
                kid = m_TreeProject->GetFirstChild( root_id, cookie );
                if( ! kid.IsOk() )
                    continue;
            }
        }

        TREEPROJECT_ITEM* itemData = GetItemIdData( kid );

        if( itemData && ( itemData->GetType() == TREE_DIRECTORY ) )
        {
            if( itemData->GetFileName() == aSubDir )    // Found!
            {
                root_id = kid;
                break;
            }

            // kid is a subdir, push in list to explore it later
            if( itemData->IsPopulated() )
                subdirs_id.push( kid );
        }
        kid = m_TreeProject->GetNextChild( root_id, cookie );
    }

    return root_id;
}


void TREE_PROJECT_FRAME::OnFileSystemEvent( wxFileSystemWatcherEvent& event )
{
    wxFileName pathModified = event.GetPath();
    wxString subdir = pathModified.GetPath();
    wxString fn = pathModified.GetFullPath();

    switch( event.GetChangeType() )
    {
    case wxFSW_EVENT_DELETE:
        break;

    case wxFSW_EVENT_CREATE:
        break;

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
        AddItemToTreeProject( pathModified.GetFullPath(), root_id, false );
        break;

    case wxFSW_EVENT_DELETE:
        while( kid.IsOk() )
        {
            TREEPROJECT_ITEM* itemData = GetItemIdData( kid );

            if( itemData  &&  itemData->GetFileName() == fn )
            {
                m_TreeProject->Delete( kid );
                return;
            }
            kid = m_TreeProject->GetNextChild( root_id, cookie );
        }
        break;

    case wxFSW_EVENT_RENAME :
        {
            wxFileName  newpath = event.GetNewPath();
            wxString    newfn = newpath.GetFullPath();

            while( kid.IsOk() )
            {
                TREEPROJECT_ITEM* itemData = GetItemIdData( kid );

                if( itemData  &&  itemData->GetFileName() == fn )
                {
                    m_TreeProject->Delete( kid );
                    break;
                }

                kid = m_TreeProject->GetNextChild( root_id, cookie );
            }

            AddItemToTreeProject( newfn, root_id, false );
        }
        break;
    }

    // Sort filenames by alphabetic order
    m_TreeProject->SortChildren( root_id );
}


void TREE_PROJECT_FRAME::FileWatcherReset()
{
    // Prepare file watcher:
    delete m_watcher;
    m_watcher = new wxFileSystemWatcher();
    m_watcher->SetOwner( this );

    // Add directories which should be monitored.
    // under windows, we add the curr dir and all subdirs
    // under unix, we add only the curr dir and the populated subdirs
    // see  http://docs.wxwidgets.org/trunk/classwx_file_system_watcher.htm
    // under unix, the file watcher needs more work to be efficient
    // moreover, under wxWidgets 2.9.4, AddTree does not work properly.

    // We can see wxString under a debugger, not a wxFileName
    wxString prj_dir = wxPathOnly( m_Parent->GetProjectFileName() );
    wxFileName fn;
    fn.AssignDir( prj_dir );
    fn.DontFollowLink();

#ifdef __WINDOWS__
    m_watcher->AddTree( fn );
#else
    m_watcher->Add( fn );

    // Add subdirs
    wxTreeItemIdValue  cookie;
    wxTreeItemId       root_id = m_root;

    std::stack < wxTreeItemId > subdirs_id;

    wxTreeItemId kid = m_TreeProject->GetFirstChild( root_id, cookie );

    while( 1 )
    {
        if( !kid.IsOk() )
        {
            if( subdirs_id.empty() )    // all items were explored
                break;
            else
            {
                root_id = subdirs_id.top();
                subdirs_id.pop();
                kid = m_TreeProject->GetFirstChild( root_id, cookie );
                if( !kid.IsOk() )
                    continue;
            }
        }

        TREEPROJECT_ITEM* itemData = GetItemIdData( kid );

        if( itemData && itemData->GetType() == TREE_DIRECTORY )
        {
            // we can see wxString under a debugger, not a wxFileName
            wxString path = itemData->GetFileName();

            DBG(printf( "%s: add '%s'\n", __func__, TO_UTF8( path ) );)

            if( wxFileName::IsDirReadable( path ) )     // linux whines about watching protected dir
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
    printf( "%s: watched paths:\n", __func__ );
    for( unsigned ii = 0; ii < paths.GetCount(); ii++ )
        printf( " %s\n", TO_UTF8( paths[ii] ) );
#endif
}

void KICAD_MANAGER_FRAME::OnChangeWatchedPaths(wxCommandEvent& aEvent )
{
    m_LeftWin->FileWatcherReset();
}

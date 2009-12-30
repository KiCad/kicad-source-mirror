/**
 * @file treeprj_frame.cpp
 * @brief TODO
 */


#ifdef KICAD_PYTHON
  #include <pyhandler.h>
#endif

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "appl_wxstruct.h"
#include "bitmaps.h"
#include "macros.h"

#include "kicad.h"

#include "wx/image.h"
#include "wx/imaglist.h"
#include "wx/treectrl.h"
#include "wx/regex.h"
#include "wx/dir.h"


// Comment this if you do no want to load subdirs files in the tree project
// UnComment this to load subdirs files in the tree project
#define ADD_FILES_IN_SUBDIRS

// list of files extensions listed in the tree project window
// *.sch files are always allowed, do not add here
// Add extensions in a compatible regex format to see others files types
const wxChar* s_AllowedExtensionsToList[] =
{
    wxT( "^.*\\.pro$" ),
    wxT( "^.*\\.pdf$" ),
    wxT( "^[^$].*\\.brd$" ),
    wxT( "^.*\\.net$" ),
    wxT( "^.*\\.txt$" ),
    wxT( "^.*\\.pho$" ),        // Gerber file
    wxT( "^.*\\.gbr$" ),        // Gerber file
    wxT( "^.*\\.gb[alops]$" ),  // Gerber back (or bottom) layer file
    wxT( "^.*\\.gt[alops]$" ),  // Gerber front (or top) layer file
    wxT( "^.*\\.g[0-9]{1,2}$" ),     // Gerber inner layer file
    wxT( "^.*\\.odt$" ),
    wxT( "^.*\\.sxw$" ),
    wxT( "^.*\\.htm$" ),
    wxT( "^.*\\.html$" ),
    wxT( "^.*\\.rpt$" ),
    wxT( "^.*\\.csv$" ),
    NULL                    // end of list
};


/* TODO: Check if these file extension and wildcard definitions are used
 *       in any of the other Kicad programs and move them into the common
 *       library as required. */

/* File extension definitions. */
const wxString PythonFileExtension( wxT( "py" ) );
const wxString TextFileExtension( wxT( "txt" ) );

/* File wildcard definitions. */
const wxString PythonFileWildcard( wxT( "Python files (*.py)|*.py" ) );
const wxString TextFileWildcard( wxT( "Text files (*.txt)|*.txt" ) );


/**
 * @brief TODO
 */
/******************************************************************/
WinEDA_PrjFrame::WinEDA_PrjFrame( WinEDA_MainFrame* parent ) :
    wxSashLayoutWindow( parent,
                        ID_LEFT_FRAME,
                        wxDefaultPosition,
                        wxDefaultSize,
                        wxNO_BORDER | wxSW_3D )
/******************************************************************/
{
    m_Parent = parent;
    m_TreeProject = NULL;
    wxMenuItem* item;
    m_PopupMenu = NULL;

    /*
     * Filtering is now inverted: the filters are actually used to _enable_ support
     * for a given file type.
     */

    // NOTE: sch filter must be first because of a test in AddFile() below
    m_Filters.push_back( wxT( "^.*\\.sch$" ) );
    for( int ii = 0; s_AllowedExtensionsToList[ii] != NULL; ii++ )
    {
        m_Filters.push_back( s_AllowedExtensionsToList[ii] );
    }

    m_Filters.push_back( wxT( "^no kicad files found" ) );


#ifdef KICAD_PYTHON
    m_Filters.push_back( wxT( "^.*\\.py$" ) );

    PyHandler::GetInstance()->DeclareEvent( wxT( "kicad::RunScript" ) );
    PyHandler::GetInstance()->DeclareEvent( wxT( "kicad::EditScript" ) );
    PyHandler::GetInstance()->DeclareEvent( wxT( "kicad::TreeContextMenu" ) );
    PyHandler::GetInstance()->DeclareEvent( wxT( "kicad::TreeAddFile" ) );
    PyHandler::GetInstance()->DeclareEvent( wxT( "kicad::NewFile" ) );
    PyHandler::GetInstance()->DeclareEvent( wxT( "kicad::NewDirectory" ) );
    PyHandler::GetInstance()->DeclareEvent( wxT( "kicad::DeleteFile" ) );
    PyHandler::GetInstance()->DeclareEvent( wxT( "kicad::RenameFile" ) );
    PyHandler::GetInstance()->DeclareEvent( wxT( "kicad::MoveFile" ) );
#endif /* KICAD_PYTHON */


    for( int i = 0; i < TREE_MAX; i++ )
        m_ContextMenus.push_back( new wxMenu() );

    wxMenu* menu = m_ContextMenus[TREE_PY];

    // Python script context menu
#ifdef KICAD_PYTHON
    item = new wxMenuItem( menu, ID_PROJECT_RUNPY,
                           _( "&Run" ),
                           _( "Run the Python Script" ) );
    item->SetBitmap( icon_python_small_xpm );
    menu->Append( item );
#endif /* KICAD_PYTHON */


    // ID_PROJECT_TXTEDIT
    item = new wxMenuItem( menu,
                           ID_PROJECT_TXTEDIT,
                           _( "&Edit in a text editor" ),
                           _( "&Open the file in a Text Editor" ) );
    item->SetBitmap( icon_txt_xpm );
    menu->Append( item );


    // New files context menu:
    wxMenu* menus[2];
    menus[0] = m_ContextMenus[TREE_DIRECTORY];
    menus[1] = m_ContextMenus[TREE_PROJECT];

    for( int i = 0; i < 2; i++ )
    {
        menu = menus[i];

        // ID_PROJECT_NEWDIR
        item = new wxMenuItem( menu,
                               ID_PROJECT_NEWDIR,
                               _( "New D&irectory" ),
                               _( "Create a New Directory" ) );
        item->SetBitmap( directory_xpm );
        menu->Append( item );


        // ID_PROJECT_NEWPY
#ifdef KICAD_PYTHON
        item = new wxMenuItem( menu,
                               ID_PROJECT_NEWPY,
                               _( "New P&ython Script" ),
                               _( "Create a New Python Script" ) );
        item->SetBitmap( new_python_xpm );
        menu->Append( item );
#endif /* KICAD_PYTHON */


        // ID_PROJECT_NEWTXT
        item = new wxMenuItem( menu,
                               ID_PROJECT_NEWTXT,
                               _( "New &Text File" ),
                               _( "Create a New Txt File" ) );
        item->SetBitmap( new_txt_xpm );
        menu->Append( item );


        // ID_PROJECT_NEWFILE
        item = new wxMenuItem( menu,
                               ID_PROJECT_NEWFILE,
                               _( "New &File" ),
                               _( "Create a New File" ) );
        item->SetBitmap( new_xpm );
        menu->Append( item );
    }


    // Put the Rename and Delete file menu commands:
    for( int i = TREE_PROJECT + 1; i < TREE_MAX; i++ )
    {
        menu = m_ContextMenus[i];

        // ID_PROJECT_RENAME
        item = new wxMenuItem( menu,
                               ID_PROJECT_RENAME,
                               TREE_DIRECTORY != i ? _( "&Rename file" ) :
                               _( "&Rename directory" ),
                               TREE_DIRECTORY != i ? _( "Rename file" ) :
                               _( "&Rename directory" ) );
        item->SetBitmap( right_xpm );
        menu->Append( item );


        if( TREE_DIRECTORY != i )
        {
            // ID_PROJECT_TXTEDIT
            item = new wxMenuItem( menu,
                                   ID_PROJECT_TXTEDIT,
                                   _( "&Edit in a text editor" ),
                                   _( "Open the file in a Text Editor" ) );
            item->SetBitmap( icon_txt_xpm );
            menu->Append( item );
        }

        // ID_PROJECT_DELETE
        item = new wxMenuItem( menu,
                               ID_PROJECT_DELETE,
                               TREE_DIRECTORY != i ? _( "&Delete File" ) :
                               _( "&Delete Directory" ),
                               TREE_DIRECTORY != i ? _( "Delete the File" ) :
                               _( "&Delete the Directory and its content" ) );
        item->SetBitmap( delete_xpm );
        menu->Append( item );
    }

    ReCreateTreePrj();
}

WinEDA_PrjFrame::~WinEDA_PrjFrame()
{
    size_t  i;
    wxMenu* menu;

    for( i = 0; i < m_ContextMenus.size(); i++ )
    {
        menu = m_ContextMenus[i];
        delete menu;
    }

    if( m_PopupMenu )
        delete m_PopupMenu;
}


/*****************************************************************************/
BEGIN_EVENT_TABLE( WinEDA_PrjFrame, wxSashLayoutWindow )
/*****************************************************************************/

    EVT_TREE_BEGIN_LABEL_EDIT( ID_PROJECT_TREE, WinEDA_PrjFrame::OnRenameAsk )
    EVT_TREE_END_LABEL_EDIT( ID_PROJECT_TREE, WinEDA_PrjFrame::OnRename )
    EVT_TREE_ITEM_ACTIVATED( ID_PROJECT_TREE, WinEDA_PrjFrame::OnSelect )
    EVT_TREE_ITEM_RIGHT_CLICK( ID_PROJECT_TREE, WinEDA_PrjFrame::OnRight )
    EVT_TREE_BEGIN_DRAG( ID_PROJECT_TREE, WinEDA_PrjFrame::OnDragStart )
    EVT_TREE_END_DRAG( ID_PROJECT_TREE, WinEDA_PrjFrame::OnDragEnd )
    EVT_MENU( ID_PROJECT_TXTEDIT, WinEDA_PrjFrame::OnTxtEdit )
    EVT_MENU( ID_PROJECT_NEWFILE, WinEDA_PrjFrame::OnNewFile )
    EVT_MENU( ID_PROJECT_NEWDIR, WinEDA_PrjFrame::OnNewDirectory )
    EVT_MENU( ID_PROJECT_NEWPY, WinEDA_PrjFrame::OnNewPyFile )
    EVT_MENU( ID_PROJECT_NEWTXT, WinEDA_PrjFrame::OnNewTxtFile )
    EVT_MENU( ID_PROJECT_DELETE, WinEDA_PrjFrame::OnDeleteFile )
    EVT_MENU( ID_PROJECT_RENAME, WinEDA_PrjFrame::OnRenameFile )


#ifdef KICAD_PYTHON
    EVT_MENU( ID_PROJECT_RUNPY, WinEDA_PrjFrame::OnRunPy )
#endif /* KICAD_PYTHON */


/*****************************************************************************/
END_EVENT_TABLE()
/*****************************************************************************/


/**
 * @brief Allowing drag & drop of file other than the currently opened project
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnDragStart( wxTreeEvent& event )
/*****************************************************************************/
{
    /* Ensure item is selected
     *  (Under Windows start drag does not activate the item) */
    wxTreeItemId     curr_item = event.GetItem();

    m_TreeProject->SelectItem( curr_item );
    TreePrjItemData* data = GetSelectedData();
    if( data->GetFileName() == m_Parent->m_ProjectFileName.GetFullPath() )
        return;

    wxTreeItemId id = m_TreeProject->GetSelection();

    wxImage      img =
        m_TreeProject->GetImageList()->GetBitmap( data->GetType() - 1 ).ConvertToImage();
    m_DragCursor = wxCursor( img );
    m_Parent->wxWindow::SetCursor( (wxCursor &)m_DragCursor );
    event.Allow();
}


/*****************************************************************************/
void WinEDA_PrjFrame::OnDragEnd( wxTreeEvent& event )
/*****************************************************************************/
{
    m_Parent->SetCursor( wxNullCursor );

    wxTreeItemId     moved = m_TreeProject->GetSelection();
    TreePrjItemData* source_data = GetSelectedData();
    wxTreeItemId     dest = event.GetItem();

    if( !dest.IsOk() )
        return;                // Cancelled ...

    TreePrjItemData* destData =
        dynamic_cast<TreePrjItemData*>( m_TreeProject->GetItemData( dest ) );

    if( !destData )
        return;

    // the item can be a member of the selected directory; get the directory itself
    if( TREE_DIRECTORY != destData->GetType()
       && !m_TreeProject->ItemHasChildren( dest ) )
    {
        // the item is a member of the selected directory; get the directory itself
        dest = m_TreeProject->GetItemParent( dest );
        if( !dest.IsOk() )
            return;                // no parent ?

        // Select the right destData:
        destData =
            dynamic_cast<TreePrjItemData*>( m_TreeProject->GetItemData( dest ) );

        if( !destData )
            return;
    }

    source_data->Move( destData );

#if 0
    /* Sort filenames by alphabetic order */
    m_TreeProject->SortChildren( dest );
#endif
}


/*****************************************************************************/
void WinEDA_PrjFrame::ClearFilters()
/*****************************************************************************/
{
    m_Filters.clear();
}


/*****************************************************************************/
void WinEDA_PrjFrame::RemoveFilter( const wxString& filter )
/*****************************************************************************/
{
    for( unsigned int i = 0; i < m_Filters.size(); i++ )
    {
        if( filter == m_Filters[i] )
        {
            m_Filters.erase( m_Filters.begin() + i );
            return;
        }
    }
}


#ifdef KICAD_PYTHON


/**
 * @brief Return the data corresponding to the file, or NULL
 */
/*****************************************************************************/
TreePrjItemData* WinEDA_PrjFrame::FindItemData( const boost::python::str& name )
/*****************************************************************************/
{
    // (Interative tree parsing)
    std::vector< wxTreeItemId >  roots1, roots2;
    std::vector< wxTreeItemId >* root, * reserve;
    wxString filename = PyHandler::MakeStr( name );

    root    = &roots1;
    reserve = &roots2;

    root->push_back( m_TreeProject->GetRootItem() );

    // if we look for the root, return it ...
    TreePrjItemData* data = dynamic_cast< TreePrjItemData*>(
        m_TreeProject->GetItemData( root->at( 0 ) ) );

    if( data->GetFileName() == filename )
        return data;

    // Then find in its child
    while( root->size() )
    {
        // look in all roots
        for( unsigned int i = 0; i < root->size(); i++ )
        {
            wxTreeItemId id = root->at( i );

            // for each root check any child:
            void*        cookie = NULL;
            wxTreeItemId child  = m_TreeProject->GetFirstChild( id, cookie );

            while( child.IsOk() )
            {
                TreePrjItemData* data = dynamic_cast< TreePrjItemData*>(
                    m_TreeProject->GetItemData( child ) );

                if( data )
                {
                    if( data->GetFileName() == filename )
                        return data;
                    if( m_TreeProject->ItemHasChildren( child ) )
                        reserve->push_back( child );
                }
                child = m_TreeProject->GetNextSibling( child );
            }
        }

        // Swap the roots
        root->clear();
        std::vector< wxTreeItemId >* tmp;
        tmp     = root;
        root    = reserve;
        reserve = tmp;
    }

    return NULL;
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::RemoveFilterPy( const boost::python::str& filter )
/*****************************************************************************/
{
    RemoveFilter( PyHandler::MakeStr( filter ) );
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::AddFilter( const boost::python::str& filter )
/*****************************************************************************/
{
    wxRegEx  reg;
    wxString text = PyHandler::MakeStr( filter );

    if( !reg.Compile( text ) )
        return;
    m_Filters.push_back( text );
}


#endif /* KICAD_PYTHON */


/**
 * @brief TODO
 */
/*****************************************************************************/
const std::vector<wxString>& WinEDA_PrjFrame::GetFilters()
/*****************************************************************************/
{
    return m_Filters;
}


/**
 * @brief TODO
 */
/*****************************************************************************/
wxMenu* WinEDA_PrjFrame::GetContextMenu( int type )
/*****************************************************************************/
{
    return m_ContextMenus[type];
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnNewDirectory( wxCommandEvent& event )
/*****************************************************************************/
{
    NewFile( TREE_DIRECTORY );
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnNewFile( wxCommandEvent& event )
/*****************************************************************************/
{
    NewFile( TREE_UNKNOWN );
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnNewPyFile( wxCommandEvent& event )
/*****************************************************************************/
{
    NewFile( TREE_PY );
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnNewTxtFile( wxCommandEvent& event )
/*****************************************************************************/
{
    NewFile( TREE_TXT );
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::NewFile( TreeFileType type )
/*****************************************************************************/
{
    wxString         mask = GetFileExt( type );
    wxString         wildcard = GetFileWildcard( type );

    // Get the directory:
    wxString         dir;
    wxString         title;

    TreePrjItemData* treeData;

    title = ( TREE_DIRECTORY != type ) ? _( "Create New File" ) :
        _( "Create New Directory" );

    treeData = GetSelectedData();
    if( !treeData )
        return;

    dir = wxGetCwd() + wxFileName().GetPathSeparator() + treeData->GetDir();

    // Ask for the new file name
    wxFileDialog dlg( this, title, dir, _( "noname." ) + mask,
                      wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    TreeFileType rootType = treeData->GetType();
    wxTreeItemId root;

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

    NewFile( dlg.GetPath(), type, root );
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::NewFile( const wxString& name,
                               TreeFileType    type,
                               wxTreeItemId&   root )
/*****************************************************************************/
{
    if( TREE_DIRECTORY != type )
    {
        wxFile( name, wxFile::write );

#ifdef KICAD_PYTHON
        PyHandler::GetInstance()->TriggerEvent( wxT( "kicad::NewFile" ),
                                                PyHandler::Convert( name ) );
#endif /* KICAD_PYTHON */
    }
    else
    {
        wxMkdir( name );

#ifdef KICAD_PYTHON
        PyHandler::GetInstance()->TriggerEvent( wxT( "kicad::NewDirectory" ),
                                                PyHandler::Convert( name ) );
#endif /* KICAD_PYTHON */
    }

    AddFile( name, root );
}


/**
 * @brief TODO
 */
/*****************************************************************************/
wxString WinEDA_PrjFrame::GetFileExt( TreeFileType type )
/*****************************************************************************/
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

    case TREE_PCB:
        ext = BoardFileExtension;
        break;

    case TREE_PY:
        ext = PythonFileExtension;
        break;

    case TREE_GERBER:
        ext = GerberFileExtension;
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
    default:                       /* Eliminates unnecessary GCC warning. */
        break;
    }

    return ext;
}

/*
 * Return the wxFileDialog wildcard string for the selected file type.
 */
wxString WinEDA_PrjFrame::GetFileWildcard( TreeFileType type )
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

    case TREE_PCB:
        ext = BoardFileWildcard;
        break;

    case TREE_PY:
        ext = PythonFileWildcard;
        break;

    case TREE_GERBER:
        ext = GerberFileWildcard;
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
    default:                       /* Eliminates unnecessary GCC warning. */
        break;
    }

    return ext;
}


/**
 * @brief  Add filename "name" to the tree \n
 *         if name is a directory, add the sub directory file names
 * @return TODO
 */
/*****************************************************************************/
bool WinEDA_PrjFrame::AddFile( const wxString& name, wxTreeItemId& root )
/*****************************************************************************/
{
    wxTreeItemId cellule;

    // Check the file type
    TreeFileType type = TREE_UNKNOWN;

    if( wxDirExists( name ) )
    {
        type = TREE_DIRECTORY;
    }
    else
    {
        // Filter
        wxRegEx reg;

        bool    isSchematic = false;
        bool    addFile     = false;
        for( unsigned i = 0; i < m_Filters.size(); i++ )
        {
            reg.Compile( m_Filters[i], wxRE_ICASE );
            if( reg.Matches( name ) )
            {
                addFile = true;
                if( i==0 )
                    isSchematic = true;
                break;
            }
        }

        if( !addFile )
            return false;

        // only show the schematic if it is a top level schematic.  eeschema
        // cannot open a schematic and display it properly unless it starts
        // at the top of the hierarchy.  The schematic is top level only if
        // there is a line in the header saying:
        // "Sheet 1 "
        if( isSchematic )
        {
            char     line[128]; // small because we just need a few bytes from the start of a line
            FILE*    fp;

            wxString FullFileName = name;

            fp = wxFopen( FullFileName, wxT( "rt" ) );
            if( fp == NULL )
            {
                return false;
            }

            addFile = false;

            // check the first 100 lines for the "Sheet 1" string
            for( int i = 0;  i<100;  ++i )
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
                return false;     // it is a non-top-level schematic
        }

        for( int i = TREE_PROJECT; i < TREE_MAX; i++ )
        {
            wxString ext = GetFileExt( (TreeFileType) i );

            if( ext == wxT( "" ) )
                continue;

            reg.Compile( wxString::FromAscii( "^.*\\" ) + ext +
                         wxString::FromAscii( "$" ), wxRE_ICASE );

            if( reg.Matches( name ) )
            {
                type = (TreeFileType) i;
                break;
            }
        }
    }

    //also check to see if it is already there.
    wxTreeItemIdValue cookie;
    wxTreeItemId      kid = m_TreeProject->GetFirstChild( root, cookie );
    while( kid.IsOk() )
    {
        TreePrjItemData* itemData = (TreePrjItemData*)
                                    m_TreeProject->GetItemData( kid );
        if( itemData )
        {
            if( itemData->m_FileName == name )
            {
                return true; //well, we would have added it, but it is already here!
            }
        }
        kid = m_TreeProject->GetNextChild( root, cookie );
    }

    // Append the item (only appending the filename not the full path):
    wxString         file = wxFileNameFromPath( name );
    cellule = m_TreeProject->AppendItem( root, file );
    TreePrjItemData* data = new TreePrjItemData( type, name, m_TreeProject );

    m_TreeProject->SetItemData( cellule, data );
    data->SetState( 0 );

    /* Mark root files (files which have the same name as the project) */
    wxFileName project( m_Parent->m_ProjectFileName );
    wxFileName currfile( file );

    if( currfile.GetName().CmpNoCase( project.GetName() ) == 0 )
        data->m_IsRootFile = true;
    else
        data->m_IsRootFile = false;


#ifdef KICAD_PYTHON
    PyHandler::GetInstance()->TriggerEvent( wxT( "kicad::TreeAddFile" ),
                                            PyHandler::Convert( name ) );
#endif /* KICAD_YTHON */


    // When enabled This section adds dirs and files found in the subdirs
    // in this case AddFile is recursive.
#ifdef ADD_FILES_IN_SUBDIRS
    if( TREE_DIRECTORY == type )
    {
        const wxString sep = wxFileName().GetPathSeparator();
        wxDir          dir( name );
        wxString       dir_filename;

        if( dir.GetFirst( &dir_filename ) )
        {
            do
            {
                AddFile( name + sep + dir_filename, cellule );
            } while( dir.GetNext( &dir_filename ) );
        }

        /* Sort filenames by alphabetic order */
        m_TreeProject->SortChildren( cellule );
    }
#endif /* ADD_FILES_IN_SUBDIRS */


    return true;
}


/**
 * @brief  Create or modify the tree showing project file names
 * @return TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::ReCreateTreePrj()
/*****************************************************************************/
{
    wxTreeItemId rootcellule;
    wxFileName   fn;
    bool         prjOpened = false;

    if( !m_TreeProject )
        m_TreeProject = new WinEDA_TreePrj( this );
    else
        m_TreeProject->DeleteAllItems();

    if( !m_Parent->m_ProjectFileName.IsOk() )
    {
        fn.Clear();
        fn.SetPath( ::wxGetCwd() );
        fn.SetName( wxT( "noname" ) );
        fn.SetExt( ProjectFileExtension );
    }
    else
        fn = m_Parent->m_ProjectFileName;

    prjOpened = fn.FileExists();

    // root tree:
    m_root = rootcellule = m_TreeProject->AddRoot( fn.GetFullName(),
                                                   TREE_PROJECT - 1,
                                                   TREE_PROJECT - 1 );

    m_TreeProject->SetItemBold( rootcellule, TRUE );

    m_TreeProject->SetItemData( rootcellule,
                                new TreePrjItemData( TREE_PROJECT,
                                                     wxEmptyString,
                                                     m_TreeProject ) );

    fn.SetExt( SchematicFileExtension );

    // Add at least a .sch / .brd if not existing:
    if( !fn.FileExists() )
        AddFile( fn.GetFullName(), m_root );

    fn.SetExt( BoardFileExtension );

    if( !fn.FileExists( ) )
        AddFile( fn.GetFullName(), m_root );

    fn.SetExt( ProjectFileExtension );

    // Now adding all current files if available
    if( prjOpened )
    {
        wxString filename;
        wxDir    dir( wxGetCwd() );
        bool     cont = dir.GetFirst( &filename );

        while( cont )
        {
            if( filename != fn.GetFullName() )
                AddFile( dir.GetName() + wxFileName::GetPathSeparator() +
                         filename, m_root );

            cont = dir.GetNext( &filename );
        }
    }

    m_TreeProject->Expand( rootcellule );

    /* Sort filenames by alphabetic order */
    m_TreeProject->SortChildren( m_root );

    m_Parent->m_ProjectFileName = fn;
}


/**
 * @brief  Opens *popup* the context menu
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnRight( wxTreeEvent& Event )
/*****************************************************************************/
{
    int tree_id;
    TreePrjItemData* tree_data;
    wxString         FullFileName;
    wxTreeItemId     curr_item = Event.GetItem();

    /* Ensure item is selected (Under Windows right click does not select the item) */
    m_TreeProject->SelectItem( curr_item );

    // Delete and recreate the context menu
    delete ( m_PopupMenu );
    m_PopupMenu = new wxMenu();

    // Get the current filename:
    tree_data = GetSelectedData();
    if( !tree_data )
        return;

    tree_id = tree_data->GetType();
    FullFileName = tree_data->GetFileName();

    // copy menu contents in order of the next array:
    wxMenu* menus[] =
    {
        GetContextMenu( tree_id ),
        const_cast<wxMenu*>( tree_data->GetMenu() )
    };

    for( unsigned int j = 0;  j < sizeof(menus) / sizeof(wxMenu*);  j++ )
    {
        wxMenu* menu = menus[j];
        if( !menu )
            continue;

        wxMenuItemList list = menu->GetMenuItems();

        for( unsigned int i = 0; i < list.GetCount(); i++ )
        {
            // Grrrr! wxMenu does not have any copy constructor !! (do it by hand)
            wxMenuItem* src   = list[i];
            wxString    label = src->GetItemLabelText();

            // for obscure reasons, the & is translated into _ ... so replace it
            label.Replace( wxT( "_" ), wxT( "&" ), true );
            wxMenuItem* item = new wxMenuItem( m_PopupMenu, src->GetId(),
                                               label,
                                               src->GetHelp(), src->GetKind() );

            item->SetBitmap( src->GetBitmap() );
            m_PopupMenu->Append( item );
        }
    }

    // At last, call python to let python add menu items "on the fly"


  #ifdef KICAD_PYTHON
    PyHandler::GetInstance()->TriggerEvent( wxT( "kicad::TreeContextMenu" ),
                                            PyHandler::Convert( FullFileName ) );
  #endif /* KICAD_PYTHON */


    if( m_PopupMenu )
        PopupMenu( m_PopupMenu );
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnTxtEdit( wxCommandEvent& event )
/*****************************************************************************/
{
    TreePrjItemData* tree_data = GetSelectedData();

    if( !tree_data )
        return;

    wxString FullFileName = tree_data->GetFileName();
    AddDelimiterString( FullFileName );
    wxString editorname = wxGetApp().GetEditorName();

    if( !editorname.IsEmpty() )
    {
  #ifdef KICAD_PYTHON
        PyHandler::GetInstance()->TriggerEvent( wxT( "kicad::EditScript" ),
                                                PyHandler::Convert( FullFileName ) );
  #endif

        ExecuteFile( this, editorname, FullFileName );
    }
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnDeleteFile( wxCommandEvent& )
/*****************************************************************************/
{
    TreePrjItemData* tree_data = GetSelectedData();

    if( !tree_data )
        return;
    tree_data->Delete();
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnRenameFile( wxCommandEvent& )
/*****************************************************************************/
{
    wxTreeItemId     curr_item = m_TreeProject->GetSelection();
    TreePrjItemData* tree_data = GetSelectedData();

    if( !tree_data )
        return;

    wxString buffer = m_TreeProject->GetItemText( curr_item );
    wxString msg    = _( "Change filename: " ) + tree_data->m_FileName;

    if( Get_Message( msg, _( "Change filename" ), buffer, this ) != 0 )
        return; //Abort command

    if( tree_data->Rename( buffer, true ) )
        m_TreeProject->SetItemText( curr_item, buffer );
}


#ifdef KICAD_PYTHON

/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnRunPy( wxCommandEvent& event )
/*****************************************************************************/
{
    TreePrjItemData* tree_data = GetSelectedData();

    if( !tree_data )
        return;

    wxString FullFileName = tree_data->GetFileName();
    PyHandler::GetInstance()->TriggerEvent( wxT( "kicad::RunScript" ),
                                            PyHandler::Convert( FullFileName ) );
    PyHandler::GetInstance()->RunScript( FullFileName );
}


/**
 * @brief Add a state to the image list
 */
/*****************************************************************************/
int WinEDA_PrjFrame::AddStatePy( boost::python::object& bitmap )
/*****************************************************************************/
{
    wxBitmap* image;
    bool      success = wxPyConvertSwigPtr( bitmap.ptr(),
                                            (void**) &image, _T( "wxBitmap" ) );

    if( !success )
        return -1;

    wxImageList* list = m_TreeProject->GetImageList();
    int          ret  = list->GetImageCount() / ( TREE_MAX - 2 );

    for( int i = 0; i < TREE_MAX - 1; i++ )
    {
        wxBitmap   composed( list->GetBitmap( i ) );

        wxMemoryDC dc;
        dc.SelectObject( composed );
        dc.DrawBitmap( *image, 0, 0, true );
        list->Add( composed );
    }

    return ret;
}


#endif /* KICAD_PYTHON */


/**
 * @brief Prevent the main project to be renamed
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnRenameAsk( wxTreeEvent& event )
/*****************************************************************************/
{
    TreePrjItemData* tree_data = GetSelectedData();

    if( !tree_data )
        return;
    if( m_Parent->m_ProjectFileName.GetFullPath() == tree_data->GetFileName() )
        event.Veto();
}


/**
 * @brief Rename a tree item on demand of the context menu
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnRename( wxTreeEvent& event )
/*****************************************************************************/
{
    TreePrjItemData* tree_data = GetSelectedData();

    if( !tree_data )
        return;

    tree_data->OnRename( event );
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::OnSelect( wxTreeEvent& Event )
/*****************************************************************************/
{
    wxString         FullFileName;

    TreePrjItemData* tree_data = GetSelectedData();

    if( !tree_data )
        return;
    tree_data->Activate( this );
}

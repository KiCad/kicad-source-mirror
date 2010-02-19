/*****************************************************************************/

/**
 * @file kicad.cpp
 * @brief Main kicad library manager file
 */
/*****************************************************************************/


#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "bitmaps.h"
#include "colors.h"


#include "kicad.h"
#include "macros.h"

#ifdef KICAD_PYTHON
 #include <pyhandler.h>
#endif


/* Import functions */
char* GetFileName( char* FullPathName );

#ifdef KICAD_PYTHON
using namespace boost::python;


/*****************************************************************************/

// Global functions:
/*****************************************************************************/
static WinEDA_MainFrame& GetMainFrame()
{
    return *( wxGetApp().m_MainFrame );
}


static void WinEDAPrint( str msg )
{
    GetMainFrame().PrintMsg( PyHandler::MakeStr( msg ) + wxT( "\n" ) );
}


static void WinEDAClear()
{
    GetMainFrame().ClearMsg();
}


static object GetTypeExt( enum TreeFileType type )
{
    return PyHandler::Convert( TREE_PROJECT_FRAME::GetFileExt( type ) );
}


/*****************************************************************************/

// WinEDA_MainFrame Special binding functions:
// (one line functions are simple wrappers)
/*****************************************************************************/
object WinEDA_MainFrame::GetPrjName() const
{
    return PyHandler::Convert( m_PrjFileName );
}


object WinEDA_MainFrame::ToWx()
{
    return object( handle<>( borrowed( wxPyMake_wxObject( this, false ) ) ) );
}


TREE_PROJECT_FRAME* WinEDA_MainFrame::GetTree() const
{
    return m_LeftWin;
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_MainFrame::AddFastLaunchPy( object& button )
/*****************************************************************************/
{
    wxBitmapButton* btn;

    bool success = wxPyConvertSwigPtr( button.ptr(),
                                       (void**) &btn, _T( "wxBitmapButton" ) );

    if( !success )
        return;

    Py_INCREF( button.ptr() );
    btn->Reparent( m_CommandWin );
    m_CommandWin->AddFastLaunch( btn );
}


/*****************************************************************************/

// TREE_PROJECT_FRAME Special binding functions:
// (one line functions are simple wrappers)
/*****************************************************************************/

// TODO To WxWidgets ?
object TREE_PROJECT_FRAME::ToWx()
{
    return object( handle<>( borrowed( wxPyMake_wxObject( this, false ) ) ) );
}

// TODO Get ?
object TREE_PROJECT_FRAME::GetFtExPy( enum TreeFileType type ) const
{
    return PyHandler::Convert( GetFileExt( type ) );
}

// Get python menu
object TREE_PROJECT_FRAME::GetMenuPy( enum TreeFileType type )
{
    return object( handle<>( borrowed( wxPyMake_wxObject( GetContextMenu( (int) type ), false ) ) ) );
}

// Get tree control
object TREE_PROJECT_FRAME::GetTreeCtrl()
{
    return object( handle<>( borrowed( wxPyMake_wxObject( m_TreeProject, false ) ) ) );
}

// Get current menu
object TREE_PROJECT_FRAME::GetCurrentMenu()
{
    return object( handle<>( borrowed( wxPyMake_wxObject( m_PopupMenu, false ) ) ) );
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void TREE_PROJECT_FRAME::NewFilePy( const str&        name,
                                 enum TreeFileType type,
                                 object&           id )
/*****************************************************************************/
{
    wxTreeItemId root;

    if( !wxPyConvertSwigPtr( id.ptr(), (void**) &root, _T( "wxTreeItemId" ) ) )
        return;
    NewFile( PyHandler::MakeStr( name ), type, root );
}


/**
 * @brief Add a file to the tree under root, or m_root if conversion is wrong
 */
/*****************************************************************************/
void TREE_PROJECT_FRAME::AddFilePy( const str& file, object& root )
/*****************************************************************************/
{
    wxTreeItemId* theroot = &m_root;

    if( !wxPyConvertSwigPtr( root.ptr(), (void**) &root, _T( "wxTreeItemId" ) ) )
    {
        theroot = &m_root;
    }

    AddFile( PyHandler::MakeStr( file ), *theroot );
}


/**
 * @brief convert wxTreeItem into TreePrjItemData
 */
/*****************************************************************************/
TreePrjItemData* TREE_PROJECT_FRAME::GetItemData( const object& item )
/*****************************************************************************/
{
    wxTreeItemId* id = NULL;

    if( !wxPyConvertSwigPtr( item.ptr(), (void**) &id, _T( "wxTreeItemId" ) ) )
    {
        return NULL;
    }

    return dynamic_cast<TreePrjItemData*>( m_TreeProject->GetItemData( *id ) );
}


/*****************************************************************************/

// TreePrjItemData Special binding functions
// (one line functions are simple wrappers)
/*****************************************************************************/

// Python rename
bool TreePrjItemData::RenamePy( const str& newname, bool check )
{
    return Rename( PyHandler::MakeStr( newname ), check );
}

// Get python directory
object TreePrjItemData::GetDirPy() const
{
    return PyHandler::Convert( GetDir() );
}

// Get python filename
object TreePrjItemData::GetFileNamePy() const
{
    return PyHandler::Convert( GetFileName() );
}

// Get python menu
object TreePrjItemData::GetMenuPy()
{
    return object( handle<>( borrowed( wxPyMake_wxObject( &m_fileMenu, false ) ) ) );
}


/**
 * @brief KiCad python module init, \n
 *        This function is called from PyHandler to init the kicad module
 */
/*****************************************************************************/
static void py_kicad_init()
/*****************************************************************************/
{
    def( "GetMainFrame", &GetMainFrame,
         return_value_policy< reference_existing_object >() );
    def( "GetTypeExtension", &GetTypeExt );

    class_<TreePrjItemData>( "PrjItem" )

    // Internal data:
    .def( "GetFileName", &TreePrjItemData::GetFileNamePy )
    .def( "GetDir", &TreePrjItemData::GetDirPy )
    .def( "GetType", &TreePrjItemData::GetType )
    .def( "GetId", &TreePrjItemData::GetIdPy )
    .def( "GetMenu", &TreePrjItemData::GetMenuPy )

    // Item control
    .def( "SetState", &TreePrjItemData::SetState )
    .def( "Rename", &TreePrjItemData::RenamePy )
    .def( "Move", &TreePrjItemData::Move )
    .def( "Delete", &TreePrjItemData::Delete )
    .def( "Activate", &TreePrjItemData::Activate )
    ;

    enum_<TreeFileType>( "FileType" )
    .value( "PROJECT", TREE_PROJECT )
    .value( "SCHEMA", TREE_SCHEMA )
    .value( "BOARD", TREE_PCB )
    .value( "PYSCRIPT", TREE_PY )
    .value( "GERBER", TREE_GERBER )
    .value( "PDF", TREE_PDF )
    .value( "TXT", TREE_TXT )
    .value( "NETLIST", TREE_NET )
    .value( "UNKNOWN", TREE_UNKNOWN )
    .value( "DIRECTORY", TREE_DIRECTORY )
    .value( "MAX", TREE_MAX );


    class_<TREE_PROJECT_FRAME>( "TreeWindow" )

    // wx Interface
    .def( "ToWx", &TREE_PROJECT_FRAME::ToWx )

    // common features
    .def( "GetContextMenu", &TREE_PROJECT_FRAME::GetMenuPy )
    .def( "GetFileExtension", &TREE_PROJECT_FRAME::GetFtExPy )

    // file filters control
    .def( "AddFilter", &TREE_PROJECT_FRAME::AddFilter )
    .def( "ClearFilters", &TREE_PROJECT_FRAME::ClearFilters )
    .def( "RemoveFilter", &TREE_PROJECT_FRAME::RemoveFilterPy )
    .def( "GetFilters", &TREE_PROJECT_FRAME::GetFilters,
          return_value_policy < copy_const_reference >() )
    .def( "GetCurrentMenu", &TREE_PROJECT_FRAME::GetCurrentMenu )


    /** Project tree control **/

    // AddState
    .def( "AddState",
          &TREE_PROJECT_FRAME::AddStatePy )

    // GetTreeCtrl
    .def( "GetTreeCtrl",
          &TREE_PROJECT_FRAME::GetTreeCtrl )

    // GetItemData
    .def( "GetItemData",
         &TREE_PROJECT_FRAME::GetItemData,
         return_value_policy < reference_existing_object >() )

    // FindItemData
    .def( "FindItemData",
          &TREE_PROJECT_FRAME::FindItemData,
          return_value_policy < reference_existing_object >() )

    // NewFile
    .def( "NewFile",
          &TREE_PROJECT_FRAME::NewFilePy )

    // AddFile
    .def( "AddFile",
          &TREE_PROJECT_FRAME::AddFilePy )

    ; /* ENDOF class_<TREE_PROJECT_FRAME>( "TreeWindow" ) */


    class_<WinEDA_MainFrame>( "MainFrame" )

    // Wx interface
    .def( "ToWx", &WinEDA_MainFrame::ToWx )

    // Common controls
    .def( "AddFastLaunch", &WinEDA_MainFrame::AddFastLaunchPy )
    .def( "Refresh", &WinEDA_MainFrame::OnRefreshPy )
    .def( "GetProjectName", &WinEDA_MainFrame::GetPrjName )
    .def( "GetProjectWindow", &WinEDA_MainFrame::GetTree,
          return_value_policy< reference_existing_object >() );
}


/**
 * @brief Common python module init
 */
/*****************************************************************************/
static void py_common_init()
/*****************************************************************************/
{
    def( "Print", &WinEDAPrint );
    def( "Clear", &WinEDAClear );
}


#endif


/*****************************************************************************/

/**
 * @file kicad.cpp
 * @brief Main kicad library manager file
 */
/*****************************************************************************/


#ifdef __GNUG__
#pragma implementation
#endif

#define MAIN

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "bitmaps.h"
#include "colors.h"

#ifdef USE_SPLASH_IMAGE
  #define SPLASH_IMAGE logo_kicad.png
  #include "wx/splash.h"
  #include "wx/mediactrl.h"
#endif

#include "kicad.h"
#include "macros.h"

#ifdef KICAD_PYTHON
 #include <pyhandler.h>
#endif

/* Global definitions for Kicad */
wxString g_Main_Title = wxT( "KiCad" );
wxString g_SchematicRootFileName;
wxString g_BoardFileName;


/* Export functions */

/* Import functions */
char* GetFileName( char* FullPathName );
void  ShowLogo( char* FonteFileName );

/* Local functions */

/************************************/
/* Called to initialize the program */
/************************************/

// Create a new application object
IMPLEMENT_APP( WinEDA_App )


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
    return PyHandler::Convert( WinEDA_PrjFrame::GetFileExt( type ) );
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


WinEDA_PrjFrame* WinEDA_MainFrame::GetTree() const
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

// WinEDA_PrjFrame Special binding functions:
// (one line functions are simple wrappers)
/*****************************************************************************/

// TODO To WxWidgets ?
object WinEDA_PrjFrame::ToWx()
{
    return object( handle<>( borrowed( wxPyMake_wxObject( this, false ) ) ) );
}

// TODO Get ?
object WinEDA_PrjFrame::GetFtExPy( enum TreeFileType type ) const
{
    return PyHandler::Convert( GetFileExt( type ) );
}

// Get python menu
object WinEDA_PrjFrame::GetMenuPy( enum TreeFileType type )
{
    return object( handle<>( borrowed( wxPyMake_wxObject( GetContextMenu( (int) type ), false ) ) ) );
}

// Get tree control
object WinEDA_PrjFrame::GetTreeCtrl()
{
    return object( handle<>( borrowed( wxPyMake_wxObject( m_TreeProject, false ) ) ) );
}

// Get current menu
object WinEDA_PrjFrame::GetCurrentMenu()
{
    return object( handle<>( borrowed( wxPyMake_wxObject( m_PopupMenu, false ) ) ) );
}


/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_PrjFrame::NewFilePy( const str&        name,
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
void WinEDA_PrjFrame::AddFilePy( const str& file, object& root )
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
TreePrjItemData* WinEDA_PrjFrame::GetItemData( const object& item )
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


    class_<WinEDA_PrjFrame>( "TreeWindow" )

    // wx Interface
    .def( "ToWx", &WinEDA_PrjFrame::ToWx )

    // common features
    .def( "GetContextMenu", &WinEDA_PrjFrame::GetMenuPy )
    .def( "GetFileExtension", &WinEDA_PrjFrame::GetFtExPy )

    // file filters control
    .def( "AddFilter", &WinEDA_PrjFrame::AddFilter )
    .def( "ClearFilters", &WinEDA_PrjFrame::ClearFilters )
    .def( "RemoveFilter", &WinEDA_PrjFrame::RemoveFilterPy )
    .def( "GetFilters", &WinEDA_PrjFrame::GetFilters,
          return_value_policy < copy_const_reference >() )
    .def( "GetCurrentMenu", &WinEDA_PrjFrame::GetCurrentMenu )


    /** Project tree control **/

    // AddState
    .def( "AddState",
          &WinEDA_PrjFrame::AddStatePy )

    // GetTreeCtrl
    .def( "GetTreeCtrl",
          &WinEDA_PrjFrame::GetTreeCtrl )

    // GetItemData
    .def( "GetItemData",
         &WinEDA_PrjFrame::GetItemData,
         return_value_policy < reference_existing_object >() )

    // FindItemData
    .def( "FindItemData",
          &WinEDA_PrjFrame::FindItemData,
          return_value_policy < reference_existing_object >() )

    // NewFile
    .def( "NewFile",
          &WinEDA_PrjFrame::NewFilePy )

    // AddFile
    .def( "AddFile",
          &WinEDA_PrjFrame::AddFilePy )

    ; /* ENDOF class_<WinEDA_PrjFrame>( "TreeWindow" ) */


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


/*****************************************************************************/
bool WinEDA_App::OnInit()
/*****************************************************************************/
{
    WinEDA_MainFrame* frame;

    InitEDA_Appl( wxT( "KiCad" ) );

    /* init kicad */
    GetSettings();                  // read current setup

    /* Make nameless project translatable */
    wxString project_ext = _T( ".pro" );
    wxString nameless_project = _( "noname" ) + project_ext;

    frame = new WinEDA_MainFrame( NULL, wxT( "KiCad" ),
                                  wxPoint( 30, 20 ), wxSize( 600, 400 ) );

    if( argc > 1 )
        frame->m_PrjFileName = argv[1];
    else if( m_fileHistory.GetCount() )
    {
        frame->m_PrjFileName = m_fileHistory.GetHistoryFile( 0 );
        if( !wxFileName::FileExists( frame->m_PrjFileName ) )
            m_fileHistory.RemoveFileFromHistory( 0 );
    }

    if( !wxFileName::FileExists( frame->m_PrjFileName ) )
        frame->m_PrjFileName = nameless_project;

    wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
    Title += wxT( " " ) + frame->m_PrjFileName;
    frame->SetTitle( Title );
    frame->ReCreateMenuBar();
    frame->RecreateBaseHToolbar();

    frame->m_LeftWin->ReCreateTreePrj();
    SetTopWindow( frame );

    /* Splash screen logo */
#ifdef USE_SPLASH_IMAGE
    wxBitmap bmp;
    if( bmp.LoadFile( m_BinDir + _T( "logokicad.png" ), wxBITMAP_TYPE_PNG ) )
    {
        wxSplashScreen* splash = new wxSplashScreen( splash_screen,
                                                     wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_TIMEOUT,
                                                     3000,
                                                     frame,
                                                     wxID_ANY,
                                                     wxDefaultPosition,
                                                     wxDefaultSize,
                                                     wxSIMPLE_BORDER | wxSTAY_ON_TOP );
    }
#endif /* USE_SPLASH_IMAGE */

    frame->Show( TRUE );
    frame->Raise();


    if( wxFileExists( frame->m_PrjFileName ) )
    {
        frame->Load_Prj_Config();
    }

#ifdef KICAD_PYTHON
    PyHandler::GetInstance()->AddToModule( wxT( "kicad" ), &py_kicad_init );
    PyHandler::GetInstance()->AddToModule( wxT( "common" ), &py_common_init );
#endif

    return TRUE;
}

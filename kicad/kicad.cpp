        /********************************/
        /* kicad.cpp - module principal */
        /********************************/

#ifdef __GNUG__
#pragma implementation
#endif

#define MAIN
#define eda_global

#include "fctsys.h"

#include <wx/image.h>

//#define SPLASH_OK

#ifdef SPLASH_OK
#include <wx/splash.h>
#endif
#include <wx/button.h>

#include "wxstruct.h"
#include "common.h"
#include "bitmaps.h"
#include "kicad.h"
#include "macros.h"

#ifdef KICAD_PYTHON
#include <pyhandler.h>
#endif

/* Routines exportees */

/* fonctions importees */
char *GetFileName(char *FullPathName);
void ShowLogo(char * FonteFileName);

/* Routines locales */

    /************************************/
    /* Called to initialize the program */
    /************************************/

// Create a new application object
IMPLEMENT_APP(WinEDA_App)

#ifdef KICAD_PYTHON
using namespace boost::python;

// Global functions:
static WinEDA_MainFrame& GetMainFrame()         { return *( wxGetApp().m_MainFrame );                               }
static void WinEDAPrint( str msg )                  { GetMainFrame().PrintMsg( PyHandler::MakeStr( msg ) + wxT("\n") ); }
static void WinEDAClear()                           { GetMainFrame().ClearMsg();                                        }
static object GetTypeExt( enum TreeFileType type )  { return PyHandler::Convert( WinEDA_PrjFrame::GetFileExt( type ) ); }

// WinEDA_MainFrame Special binding functions:
// (one line functions are simple wrappers)

object WinEDA_MainFrame::GetPrjName() const         { return PyHandler::Convert( m_PrjFileName );                                   }
object WinEDA_MainFrame::ToWx()                     { return object( handle<>( borrowed( wxPyMake_wxObject( this, false ) ) ) );    }
WinEDA_PrjFrame* WinEDA_MainFrame::GetTree() const  { return m_LeftWin;                                                             }

void WinEDA_MainFrame::AddFastLaunchPy( object & button )
{
    wxButton * btn;
    bool success = wxPyConvertSwigPtr( button.ptr(), (void**)&btn, _T("wxButton"));
    if ( !success ) return;

    Py_INCREF( button.ptr() );
    AddFastLaunch( btn );
}

// WinEDA_PrjFrame Special binding functions:
// (one line functions are simple wrappers)

object WinEDA_PrjFrame::ToWx()                                      { return object( handle<>( borrowed( wxPyMake_wxObject( this, false ) ) ) );                            }
object WinEDA_PrjFrame::GetFtExPy( enum TreeFileType type ) const   { return PyHandler::Convert( GetFileExt( type ) );                                                      }
object WinEDA_PrjFrame::GetMenuPy( enum TreeFileType type )         { return object( handle<>( borrowed( wxPyMake_wxObject( GetContextMenu( (int) type ), false ) ) ) );    }
object WinEDA_PrjFrame::GetTreeCtrl()                               { return object( handle<>( borrowed( wxPyMake_wxObject( m_TreeProject, false ) ) ) );                   }
object WinEDA_PrjFrame::GetCurrentMenu()                            { return object( handle<>( borrowed( wxPyMake_wxObject( m_PopupMenu, false ) ) ) );                     }

void WinEDA_PrjFrame::NewFilePy( const str & name, enum TreeFileType type, object & id )
{
    wxTreeItemId root;
    if (! wxPyConvertSwigPtr( id.ptr(), (void**)&root, _T("wxTreeItemId") ) ) return;
    NewFile( PyHandler::MakeStr( name ), type, root );
}

void WinEDA_PrjFrame::AddFilePy( const str & file, object & root )
/* Add a file to the tree under root, or m_root if conversion is wrong */
{
    wxTreeItemId * theroot = &m_root;
    if ( !wxPyConvertSwigPtr( root.ptr(), (void**)&root, _T("wxTreeItemId") ) )
    {
        theroot = &m_root;
    }
    AddFile( PyHandler::MakeStr( file ), *theroot );
}

TreePrjItemData * WinEDA_PrjFrame::GetItemData( const object & item )
/* convert wxTreeItem into TreePrjItemData */
{
    wxTreeItemId *id = NULL;
    if ( !wxPyConvertSwigPtr( item.ptr(), (void**)&id, _T("wxTreeItemId") ) ) return NULL;
    return dynamic_cast<TreePrjItemData *>( m_TreeProject->GetItemData( *id ) );
}

// TreePrjItemData Special binding functions
// (one line functions are simple wrappers)

bool TreePrjItemData::RenamePy( const str & newname, bool check )   { return Rename( PyHandler::MakeStr(newname), check );                          }
object TreePrjItemData::GetDirPy() const                            { return PyHandler::Convert( GetDir() );                                        }
object TreePrjItemData::GetFileNamePy() const                       { return PyHandler::Convert( GetFileName() );                                   }
object TreePrjItemData::GetMenuPy()                                 { return object( handle<>( borrowed( wxPyMake_wxObject( &m_fileMenu, false ) ) ) ); }

// kicad module init function
// ( this function is called from PyHandler to init the kicad module )

static void py_kicad_init()
{
    def( "GetMainFrame", &GetMainFrame, return_value_policy< reference_existing_object >() );
    def( "GetTypeExtension", &GetTypeExt );

    class_<TreePrjItemData>( "PrjItem" )
        // Internal data:
        .def( "GetFileName",    &TreePrjItemData::GetFileNamePy )
        .def( "GetDir",         &TreePrjItemData::GetDirPy )
        .def( "GetType",        &TreePrjItemData::GetType )
        .def( "GetId",          &TreePrjItemData::GetIdPy )
        .def( "GetMenu",        &TreePrjItemData::GetMenuPy )
        // Item control
        .def( "SetState",       &TreePrjItemData::SetState )
        .def( "Rename",         &TreePrjItemData::RenamePy )
        .def( "Move",           &TreePrjItemData::Move )
        .def( "Delete",         &TreePrjItemData::Delete )
        .def( "Activate",       &TreePrjItemData::Activate )
        ;

    enum_<TreeFileType>( "FileType" )
        .value( "PROJECT",      TREE_PROJECT )
        .value( "SCHEMA",       TREE_SCHEMA )
        .value( "BOARD",        TREE_PCB )
        .value( "PYSCRIPT",     TREE_PY )
        .value( "GERBER",       TREE_GERBER )
        .value( "PDF",          TREE_PDF )
        .value( "TXT",          TREE_TXT )
        .value( "NETLIST",      TREE_NET )
        .value( "UNKNOWN",      TREE_UNKNOWN )
        .value( "DIRECTORY",    TREE_DIRECTORY )
        .value( "MAX",          TREE_MAX );

    class_<WinEDA_PrjFrame>( "TreeWindow" )
        // wx Interface
        .def( "ToWx",               &WinEDA_PrjFrame::ToWx )
        // common features
        .def( "GetContextMenu",     &WinEDA_PrjFrame::GetMenuPy )
        .def( "GetFileExtension",   &WinEDA_PrjFrame::GetFtExPy )
        // file filters control
        .def( "AddFilter",          &WinEDA_PrjFrame::AddFilter )
        .def( "ClearFilters",       &WinEDA_PrjFrame::ClearFilters )
        .def( "RemoveFilter",       &WinEDA_PrjFrame::RemoveFilterPy )
        .def( "GetFilters",         &WinEDA_PrjFrame::GetFilters, return_value_policy < copy_const_reference >() )
        .def( "GetCurrentMenu",     &WinEDA_PrjFrame::GetCurrentMenu )
        // Project tree control
        .def( "AddState",           &WinEDA_PrjFrame::AddStatePy )
        .def( "GetTreeCtrl",        &WinEDA_PrjFrame::GetTreeCtrl )
        .def( "GetItemData",        &WinEDA_PrjFrame::GetItemData, return_value_policy < reference_existing_object >() )
        .def( "FindItemData",       &WinEDA_PrjFrame::FindItemData, return_value_policy < reference_existing_object >() )
        .def( "NewFile",            &WinEDA_PrjFrame::NewFilePy )
        .def( "AddFile",            &WinEDA_PrjFrame::AddFilePy )
        ;

    class_<WinEDA_MainFrame>( "MainFrame" )
        // Wx interface
        .def( "ToWx",               &WinEDA_MainFrame::ToWx )
        // Common controls
        .def( "AddFastLaunch",      &WinEDA_MainFrame::AddFastLaunchPy )
        .def( "Refresh",            &WinEDA_MainFrame::OnRefreshPy )
        .def( "GetProjectName",     &WinEDA_MainFrame::GetPrjName )
        .def( "GetProjectWindow",   &WinEDA_MainFrame::GetTree, return_value_policy< reference_existing_object >() );

}

// common module init function

static void py_common_init()
{
    def( "Print", &WinEDAPrint );
    def( "Clear", &WinEDAClear );
}

#endif


bool WinEDA_App::OnInit()
{
    EDA_Appl = this;
    InitEDA_Appl( wxT("kicad"));
    
    /* init kicad */
    GetSettings();                  // read current setup

    m_MainFrame = new WinEDA_MainFrame(this, NULL, wxT("KiCad"),
                 wxPoint(0,0), wxSize(600,400) );
    
    if(argc > 1 ) 
        m_MainFrame->m_PrjFileName = argv[1];
    else if ( m_EDA_Config )
    {
        m_MainFrame->m_PrjFileName = m_EDA_Config->Read(wxT("LastProject"),
                wxT("noname.pro") );
    }
    else 
        m_MainFrame->m_PrjFileName = wxT("noname.pro");

    wxString Title = g_Main_Title + wxT(" ") + GetBuildVersion();
    Title += wxT(" ") + m_MainFrame->m_PrjFileName;
    m_MainFrame->SetTitle(Title);
    m_MainFrame->ReCreateMenuBar();
    m_MainFrame->RecreateBaseHToolbar();

    m_MainFrame->m_LeftWin->ReCreateTreePrj();
    SetTopWindow(m_MainFrame);
    m_MainFrame->Show(TRUE);

    /* Preparation Affichage du logo */
#ifdef SPLASH_OK
    wxString logoname( wxString(m_BinDir) + wxT("logokicad.png") );
    wxBitmap image;
    if ( image.LoadFile( logoname, wxBITMAP_TYPE_PNG ) )
    {
        wxSplashScreen * logoscreen = new wxSplashScreen( image,
                wxSPLASH_CENTRE_ON_PARENT | wxSPLASH_TIMEOUT,
                500, m_MainFrame, -1,
                wxDefaultPosition, wxDefaultSize,
                wxSIMPLE_BORDER | wxSTAY_ON_TOP);
    }
#endif

    if( wxFileExists(m_MainFrame->m_PrjFileName) )
    {
        m_MainFrame->Load_Prj_Config();
    }

#ifdef KICAD_PYTHON
    PyHandler::GetInstance()->AddToModule( wxT("kicad"),  &py_kicad_init );
    PyHandler::GetInstance()->AddToModule( wxT("common"), &py_common_init );
#endif

    return TRUE;
}

// vim: tabstop=4 : noexpandtab :

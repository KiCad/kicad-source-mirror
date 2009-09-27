/*************************************************************************/
/* viewlib_frame.cpp - fonctions des classes du type WinEDA_ViewlibFrame */
/*************************************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "eeschema_id.h"
#include "class_drawpanel.h"
#include "bitmaps.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libviewfrm.h"
#include "class_library.h"


/*****************************/
/* class WinEDA_ViewlibFrame */
/*****************************/
BEGIN_EVENT_TABLE( WinEDA_ViewlibFrame, WinEDA_DrawFrame )
    /* Window events */
    EVT_CLOSE( WinEDA_ViewlibFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_ViewlibFrame::OnSize )
    EVT_ACTIVATE( WinEDA_DrawFrame::OnActivate )

    /* Sash drag events */
    EVT_SASH_DRAGGED( ID_LIBVIEW_LIBWINDOW, WinEDA_ViewlibFrame::OnSashDrag )
    EVT_SASH_DRAGGED( ID_LIBVIEW_CMPWINDOW, WinEDA_ViewlibFrame::OnSashDrag )


    /* Toolbar events */
    EVT_TOOL_RANGE( ID_LIBVIEW_NEXT, ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT,
                    WinEDA_ViewlibFrame::Process_Special_Functions )

    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, WinEDA_ViewlibFrame::OnZoom )
    EVT_TOOL( ID_LIBVIEW_CMP_EXPORT_TO_SCHEMATIC,
              WinEDA_ViewlibFrame::ExportToSchematicLibraryPart )
    EVT_KICAD_CHOICEBOX( ID_LIBVIEW_SELECT_PART_NUMBER,
                         WinEDA_ViewlibFrame::Process_Special_Functions )

    /* listbox events */
    EVT_LISTBOX( ID_LIBVIEW_LIB_LIST, WinEDA_ViewlibFrame::ClickOnLibList )
    EVT_LISTBOX( ID_LIBVIEW_CMP_LIST, WinEDA_ViewlibFrame::ClickOnCmpList )
END_EVENT_TABLE()


/*
 * This emulates the zoom menu entries found in the other Kicad applications.
 * The library viewer does not have any menus so add an accelerator table to
 * the main frame.
 */
static wxAcceleratorEntry accels[] =
{
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_F1, ID_ZOOM_IN ),
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_F2, ID_ZOOM_OUT ),
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_F3, ID_ZOOM_REDRAW ),
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_F4, ID_POPUP_ZOOM_CENTER ),
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_HOME, ID_ZOOM_PAGE )
};

#define ACCEL_TABLE_CNT ( sizeof( accels ) / sizeof( wxAcceleratorEntry ) )

#define EXTRA_BORDER_SIZE 2


WinEDA_ViewlibFrame::WinEDA_ViewlibFrame( wxWindow*    father,
                                          CMP_LIBRARY* Library,
                                          wxSemaphore* semaphore ) :
    WinEDA_DrawFrame( father, VIEWER_FRAME, _( "Library browser" ),
                      wxDefaultPosition, wxDefaultSize )
{
    wxAcceleratorTable table( ACCEL_TABLE_CNT, accels );

    m_FrameName = wxT( "ViewlibFrame" );
    m_ConfigPath =  wxT( "LibraryViewer" );

    // Give an icon
    SetIcon( wxIcon( library_browse_xpm ) );

    m_CmpList = NULL;
    m_LibList = NULL;
    m_LibListWindow = NULL;
    m_CmpListWindow = NULL;
    m_Semaphore     = semaphore;

    if( m_Semaphore )
        SetWindowStyle( GetWindowStyle() | wxSTAY_ON_TOP );

    SetBaseScreen( new SCH_SCREEN() );
    GetScreen()->m_Center = true;       // set to true to have the coordinates origine -0,0) centered on screen
    LoadSettings();
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    ReCreateHToolbar();
    ReCreateVToolbar();

    wxSize  size = GetClientSize();
    size.y -= m_MsgFrameHeight + 2;

    m_LibListSize.y = size.y;

    wxPoint win_pos( 0, 0 );
    if( Library == NULL )
    {
        // Creates the libraries window display
        m_LibListWindow =
            new wxSashLayoutWindow( this, ID_LIBVIEW_LIBWINDOW, win_pos,
                                    m_LibListSize, wxCLIP_CHILDREN | wxSW_3D,
                                    wxT( "LibWindow" ) );
        m_LibListWindow->SetOrientation( wxLAYOUT_VERTICAL );
        m_LibListWindow->SetAlignment( wxLAYOUT_LEFT );
        m_LibListWindow->SetSashVisible( wxSASH_RIGHT, TRUE );
        m_LibListWindow->SetExtraBorderSize( EXTRA_BORDER_SIZE );
        m_LibList =
            new wxListBox( m_LibListWindow, ID_LIBVIEW_LIB_LIST,
                           wxPoint( 0, 0 ),
                           m_LibListWindow->GetClientSize() -
                           wxSize( EXTRA_BORDER_SIZE * 2, 0 ),
                           0, NULL, wxLB_HSCROLL );
    }
    else
    {
        g_CurrentViewLibraryName = Library->GetName();
        m_LibListSize.x = 0;
    }

    // Creates the component window display
    m_CmpListSize.y = size.y;
    win_pos.x = m_LibListSize.x;
    win_pos.y = 0;
    m_CmpListWindow = new wxSashLayoutWindow( this, ID_LIBVIEW_CMPWINDOW,
                                              win_pos, m_CmpListSize,
                                              wxCLIP_CHILDREN | wxSW_3D,
                                              wxT( "CmpWindow" ) );
    m_CmpListWindow->SetOrientation( wxLAYOUT_VERTICAL );

    m_CmpListWindow->SetSashVisible( wxSASH_RIGHT, TRUE );
    m_CmpListWindow->SetExtraBorderSize( EXTRA_BORDER_SIZE );
    m_CmpList = new wxListBox( m_CmpListWindow, ID_LIBVIEW_CMP_LIST,
                               wxPoint( 0, 0 ),
                               m_CmpListWindow->GetClientSize() -
                               wxSize( EXTRA_BORDER_SIZE * 2, 0 ),
                               0, NULL, wxLB_HSCROLL );

    if( m_LibList )
        ReCreateListLib();
    DisplayLibInfos();
    if( DrawPanel )
        DrawPanel->SetAcceleratorTable( table );
    BestZoom();
    Show( TRUE );
}


WinEDA_ViewlibFrame::~WinEDA_ViewlibFrame()
{
    WinEDA_SchematicFrame* frame =
        (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();
    frame->m_ViewlibFrame = NULL;
}


/*****************************************************************/
void WinEDA_ViewlibFrame::OnCloseWindow( wxCloseEvent& Event )
/*****************************************************************/
{
    SaveSettings();
    if( m_Semaphore )
        m_Semaphore->Post();
    Destroy();
}


/****************************************************/
void WinEDA_ViewlibFrame::OnSashDrag( wxSashEvent& event )
/****************************************************/

/* Resize sub windows when dragging a sash window border
 */
{
    if( event.GetDragStatus() == wxSASH_STATUS_OUT_OF_RANGE )
        return;

    m_LibListSize.y = GetClientSize().y - m_MsgFrameHeight;
    m_CmpListSize.y = m_LibListSize.y;

    switch( event.GetId() )
    {
    case ID_LIBVIEW_LIBWINDOW:
        if( m_LibListWindow )
        {
            m_LibListSize.x = event.GetDragRect().width;
            m_LibListWindow->SetSize( m_LibListSize );
            m_CmpListWindow->SetPosition( wxPoint( m_LibListSize.x, 0 ) );
        }
        break;

    case ID_LIBVIEW_CMPWINDOW:
            m_CmpListSize.x = event.GetDragRect().width;
            m_CmpListWindow->SetSize( m_CmpListSize );
        break;
    }

    // Now, we must recalculate the position and size of subwindows
    wxSizeEvent SizeEv;
    OnSize( SizeEv );

    // Ensure the panel is always redrawn (sometimes some garbage remains):
    DrawPanel->Refresh();
}


/*****************************************************/
void WinEDA_ViewlibFrame::OnSize( wxSizeEvent& SizeEv )
/*****************************************************/
{
    wxSize clientsize;
    wxSize maintoolbar_size;
    wxSize Vtoolbar_size;

    clientsize    = GetClientSize();
    m_FrameSize   = clientsize;
    clientsize.y -= m_MsgFrameHeight;

    if( m_HToolBar )
    {
        maintoolbar_size = m_HToolBar->GetSize();
    }

    if( m_VToolBar )
    {
        Vtoolbar_size = m_VToolBar->GetSize();
        m_VToolBar->SetSize( clientsize.x - maintoolbar_size.y, 0, -1,
                             clientsize.y );
    }

    if( MsgPanel )
    {
        MsgPanel->SetSize( 0, clientsize.y, clientsize.x, m_MsgFrameHeight );
    }

    if( DrawPanel )
    {
        DrawPanel->SetSize( m_LibListSize.x + m_CmpListSize.x, 0,
                            clientsize.x - Vtoolbar_size.x -
                            m_LibListSize.x - m_CmpListSize.x,
                            clientsize.y );
    }

    if( m_LibList && m_LibListWindow )
    {
        m_LibListSize.y = clientsize.y - 2;
        m_LibListWindow->SetSize( m_LibListSize );
        m_LibList->SetSize( m_LibListWindow->GetClientSize() -
                            wxSize( EXTRA_BORDER_SIZE * 2, 0 ) );
    }

    if( m_CmpList && m_CmpListWindow )
    {
        m_CmpListSize.y = clientsize.y - 2;
        m_CmpListWindow->SetSize( m_CmpListSize );
        m_CmpListWindow->SetPosition( wxPoint( m_LibListSize.x, 0 ) );
        m_CmpList->SetSize( m_CmpListWindow->GetClientSize() -
                            wxSize( EXTRA_BORDER_SIZE * 2, 0 ) );
    }

    SizeEv.Skip();
}


/***********************************/
int WinEDA_ViewlibFrame::BestZoom()
/***********************************/
{
    int    bestzoom, ii, jj;
    wxSize size, itemsize;
    LIB_COMPONENT* component;
    CMP_LIBRARY* lib;

    GetScreen()->m_Curseur.x = 0;
    GetScreen()->m_Curseur.y = 0;
    bestzoom = 16;

    lib = CMP_LIBRARY::FindLibrary( g_CurrentViewLibraryName );

    if( lib == NULL )
        return bestzoom;

    component = lib->FindComponent( g_CurrentViewComponentName );

    if( component == NULL )
    {
        bestzoom = 16;
        GetScreen()->m_Curseur.x = 0;
        GetScreen()->m_Curseur.y = 0;
        return bestzoom;
    }

    EDA_Rect BoundaryBox = component->GetBoundaryBox( g_ViewUnit,
                                                      g_ViewConvert );
    itemsize = BoundaryBox.GetSize();
    size     = DrawPanel->GetClientSize();
    size    -= wxSize( 25, 25 ); // reserve a 25 mils margin.
    ii       = wxRound( double( itemsize.x ) / double( size.x ) );
    jj       = wxRound( double( itemsize.y ) / double( size.y ) );
    bestzoom = MAX( ii, jj ) + 1;

    GetScreen()->m_Curseur = BoundaryBox.Centre();

    return bestzoom * GetScreen()->m_ZoomScalar;
}


/**
 * Function ReCreateListLib
 *
 * Creates or recreates the list of current loaded libraries.
 * This list is sorted, with the library cache always at end of the list
 */
void WinEDA_ViewlibFrame::ReCreateListLib()
{
    wxArrayString  libNamesList;
    bool           found = false;

    if( m_LibList == NULL )
        return;

    m_LibList->Clear();
    libNamesList = CMP_LIBRARY::GetLibraryNames();
    m_LibList->Append( libNamesList );

    // Search for a previous selection:
    for ( unsigned ii = 0; ii < m_LibList->GetCount(); ii++ )
    {
        if( g_CurrentViewLibraryName.Cmp( m_LibList->GetString( ii ) ) == 0 )
        {
            m_LibList->SetSelection( ii, TRUE );
            found = TRUE;
            break;
        }
    }

    /* If not found, clear current library selection because it can be
     * deleted after a config change.
     */
    if( !found )
    {
        g_CurrentViewLibraryName.Empty();
        g_CurrentViewComponentName.Empty();
    }

    ReCreateListCmp();
    ReCreateHToolbar();
    DisplayLibInfos();
    DrawPanel->Refresh();
}


void WinEDA_ViewlibFrame::ReCreateListCmp()
{
    if( m_CmpList == NULL )
        return;

    CMP_LIBRARY* Library = CMP_LIBRARY::FindLibrary( g_CurrentViewLibraryName );
    wxArrayString  nameList;

    m_CmpList->Clear();
    g_CurrentViewComponentName.Empty();
    g_ViewConvert = 1;               /* Select normal/"de morgan" shape */
    g_ViewUnit    = 1;               /* Select unit to display for multiple
                                      * parts per package */
    if( Library )
        Library->GetEntryNames( nameList );
    m_CmpList->Append( nameList );
}


void WinEDA_ViewlibFrame::ClickOnLibList( wxCommandEvent& event )
{
    int ii = m_LibList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_LibList->GetString( ii );
    if( g_CurrentViewLibraryName == name )
        return;
    g_CurrentViewLibraryName = name;
    ReCreateListCmp();
    DrawPanel->Refresh();
    DisplayLibInfos();
    ReCreateHToolbar();
}


void WinEDA_ViewlibFrame::ClickOnCmpList( wxCommandEvent& event )
{
    int ii = m_CmpList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_CmpList->GetString( ii );
    g_CurrentViewComponentName = name;
    DisplayLibInfos();
    g_ViewUnit    = 1;
    g_ViewConvert = 1;
    Zoom_Automatique( FALSE );
    ReCreateHToolbar();
    DrawPanel->Refresh();
}



/*
 * Export the current component to schematic and close the library browser
 */
void WinEDA_ViewlibFrame::ExportToSchematicLibraryPart( wxCommandEvent& event )
{
    int ii = m_CmpList->GetSelection();

    if( ii >= 0 )
        g_CurrentViewComponentName = m_CmpList->GetString( ii );
    else
        g_CurrentViewComponentName.Empty();
    Close( TRUE );
}


#define LIBLIST_WIDTH_KEY wxT("Liblist_width")
#define CMPLIST_WIDTH_KEY wxT("Cmplist_width")
/**
 * Load library viewer frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get loaded.
 */
void WinEDA_ViewlibFrame::LoadSettings( )
{
    wxConfig* cfg ;

    WinEDA_DrawFrame::LoadSettings();

    wxConfigPathChanger cpc( wxGetApp().m_EDA_Config, m_ConfigPath );
    cfg = wxGetApp().m_EDA_Config;

    m_LibListSize.x = 150; // default width of libs list
    m_CmpListSize.x = 150; // default width of component list

    cfg->Read( LIBLIST_WIDTH_KEY, &m_LibListSize.x );
    cfg->Read( CMPLIST_WIDTH_KEY, &m_CmpListSize.x );

    // set parameters to a resonnable value
    if ( m_LibListSize.x > m_FrameSize.x/2 )
        m_LibListSize.x = m_FrameSize.x/2;

    if ( m_CmpListSize.x > m_FrameSize.x/2 )
        m_CmpListSize.x = m_FrameSize.x/2;
}


/**
 * Save library viewer frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get saved.
 */
void WinEDA_ViewlibFrame::SaveSettings()
{
    wxConfig* cfg;

    WinEDA_DrawFrame::SaveSettings();

    wxConfigPathChanger cpc( wxGetApp().m_EDA_Config, m_ConfigPath );
    cfg = wxGetApp().m_EDA_Config;

    if ( m_LibListSize.x )
        cfg->Write( LIBLIST_WIDTH_KEY, m_LibListSize.x );
    cfg->Write( CMPLIST_WIDTH_KEY, m_CmpListSize.x );
}

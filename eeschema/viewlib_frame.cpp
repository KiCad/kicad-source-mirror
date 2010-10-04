/***********************/
/*  viewlib_frame.cpp  */
/***********************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "eeschema_id.h"
#include "class_drawpanel.h"
#include "bitmaps.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "viewlib_frame.h"
#include "class_library.h"
#include "hotkeys.h"


/**
 * Save previous component library viewer state.
 */
wxString WinEDA_ViewlibFrame::m_libraryName;
wxString WinEDA_ViewlibFrame::m_entryName;
int WinEDA_ViewlibFrame::m_unit = 1;
int WinEDA_ViewlibFrame::m_convert = 1;
wxSize WinEDA_ViewlibFrame::m_clientSize = wxSize( -1, -1 );


/*****************************/
/* class WinEDA_ViewlibFrame */
/*****************************/
BEGIN_EVENT_TABLE( WinEDA_ViewlibFrame, WinEDA_DrawFrame )
    /* Window events */
    EVT_CLOSE( WinEDA_ViewlibFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_ViewlibFrame::OnSize )
    EVT_ACTIVATE( WinEDA_ViewlibFrame::OnActivate )

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

    EVT_MENU( ID_SET_RELATIVE_OFFSET, WinEDA_ViewlibFrame::OnSetRelativeOffset )
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
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_HOME, ID_ZOOM_PAGE ),
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_SPACE, ID_SET_RELATIVE_OFFSET )
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

    m_HotkeysZoomAndGridList = s_Viewlib_Hokeys_Descr;
    m_CmpList = NULL;
    m_LibList = NULL;
    m_LibListWindow = NULL;
    m_CmpListWindow = NULL;
    m_Semaphore     = semaphore;

    if( m_Semaphore )
        SetWindowStyle( GetWindowStyle() | wxSTAY_ON_TOP );

    SetBaseScreen( new SCH_SCREEN() );
    GetScreen()->m_Center = true;      // Center coordinate origins on screen.
    LoadSettings();

    // Initialize grid id to a default value if not found in config or bad:
    if( ( m_LastGridSizeId <= 0 ) ||
        ( m_LastGridSizeId < ( ID_POPUP_GRID_USER - ID_POPUP_GRID_LEVEL_1000 ) ) )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateHToolbar();
    ReCreateVToolbar();

    wxSize  size = GetClientSize();
    size.y -= m_MsgFrameHeight + 2;

    m_LibListSize.y = -1;

    wxPoint win_pos( 0, 0 );

    if( Library == NULL )
    {
        // Creates the libraries window display
        m_LibListWindow =
            new wxSashLayoutWindow( this, ID_LIBVIEW_LIBWINDOW, win_pos,
                                    wxDefaultSize, wxCLIP_CHILDREN | wxSW_3D,
                                    wxT( "LibWindow" ) );
        m_LibListWindow->SetOrientation( wxLAYOUT_VERTICAL );
        m_LibListWindow->SetAlignment( wxLAYOUT_LEFT );
        m_LibListWindow->SetSashVisible( wxSASH_RIGHT, TRUE );
        m_LibListWindow->SetExtraBorderSize( EXTRA_BORDER_SIZE );
        m_LibList = new wxListBox( m_LibListWindow, ID_LIBVIEW_LIB_LIST,
                                   wxPoint( 0, 0 ), wxDefaultSize,
                                   0, NULL, wxLB_HSCROLL );
    }
    else
    {
        m_libraryName = Library->GetName();
        m_entryName.Clear();
        m_unit = 1;
        m_convert = 1;
        m_LibListSize.x = 0;
    }

    // Creates the component window display
    m_CmpListSize.y = size.y;
    win_pos.x = m_LibListSize.x;
    m_CmpListWindow = new wxSashLayoutWindow( this, ID_LIBVIEW_CMPWINDOW,
                                              win_pos, wxDefaultSize,
                                              wxCLIP_CHILDREN | wxSW_3D,
                                              wxT( "CmpWindow" ) );
    m_CmpListWindow->SetOrientation( wxLAYOUT_VERTICAL );

    m_CmpListWindow->SetSashVisible( wxSASH_RIGHT, TRUE );
    m_CmpListWindow->SetExtraBorderSize( EXTRA_BORDER_SIZE );
    m_CmpList = new wxListBox( m_CmpListWindow, ID_LIBVIEW_CMP_LIST,
                               wxPoint( 0, 0 ), wxDefaultSize,
                               0, NULL, wxLB_HSCROLL );

    if( m_LibList )
        ReCreateListLib();

    DisplayLibInfos();

    if( DrawPanel )
        DrawPanel->SetAcceleratorTable( table );

#ifdef USE_WX_GRAPHICS_CONTEXT
    GetScreen()->SetZoom( BestZoom() );
#else
    Zoom_Automatique( false );
#endif

    Show( TRUE );

    m_auimgr.SetManagedWindow( this );

    wxAuiPaneInfo horiz;
    horiz.Gripper( false );
    horiz.DockFixed( true );
    horiz.Movable( false );
    horiz.Floatable( false );
    horiz.CloseButton( false );
    horiz.CaptionVisible( false );

    wxAuiPaneInfo vert( horiz );

    vert.TopDockable( false ).BottomDockable( false );
    horiz.LeftDockable( false ).RightDockable( false );

    // Manage main toolbal
    m_auimgr.AddPane( m_HToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT ("m_HToolBar" ) ).Top().Row( 0 ) );

    wxSize minsize(60,-1);
    // Manage the left window (list of libraries)
    if( m_LibListWindow )
        m_auimgr.AddPane( m_LibListWindow,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_LibList" ) ).
                      Left().Row( 0 ).MinSize(minsize) );

    // Manage the list of components)
    m_auimgr.AddPane( m_CmpListWindow,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_CmpList" ) ).
                      Left().Row( 1 ).MinSize(minsize) );

    // Manage the draw panel
    m_auimgr.AddPane( DrawPanel,
                      wxAuiPaneInfo( vert ).Name( wxT( "DrawFrame" ) ).Centre() );

    // Manage the message panel
    m_auimgr.AddPane( MsgPanel,
                      wxAuiPaneInfo( horiz ).Name( wxT( "MsgPanel" ) ).Bottom() );

    /* Now the minimum windows are fixed, set library list
    and component list of the previous values from last viewlib use
    */
    if( m_LibListWindow )
    {
        wxAuiPaneInfo& pane = m_auimgr.GetPane(m_LibListWindow);
        pane.MinSize( wxSize(m_LibListSize.x, -1));
    }
    wxAuiPaneInfo& pane = m_auimgr.GetPane(m_CmpListWindow);
    pane.MinSize(wxSize(m_CmpListSize.x, -1));

    m_auimgr.Update();
}


WinEDA_ViewlibFrame::~WinEDA_ViewlibFrame()
{
    WinEDA_SchematicFrame* frame =
        (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();
    frame->m_ViewlibFrame = NULL;
}


void WinEDA_ViewlibFrame::OnCloseWindow( wxCloseEvent& Event )
{
    SaveSettings();
    if( m_Semaphore )
        m_Semaphore->Post();
    Destroy();
}


/*
 * Resize sub windows when dragging a sash window border
 */
void WinEDA_ViewlibFrame::OnSashDrag( wxSashEvent& event )
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
            wxAuiPaneInfo& pane = m_auimgr.GetPane(m_LibListWindow);
            m_LibListSize.x = event.GetDragRect().width;
            pane.MinSize(m_LibListSize);
            m_auimgr.Update();
        }
        break;

    case ID_LIBVIEW_CMPWINDOW:
    {
            wxAuiPaneInfo& pane = m_auimgr.GetPane(m_CmpListWindow);
            m_CmpListSize.x = event.GetDragRect().width;
            pane.MinSize(m_CmpListSize);
            m_auimgr.Update();
    }
        break;
    }
}


void WinEDA_ViewlibFrame::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}


void WinEDA_ViewlibFrame::OnSetRelativeOffset( wxCommandEvent& event )
{
    GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
    UpdateStatusBar();
}


int WinEDA_ViewlibFrame::BestZoom()
{
    int    bestzoom, ii, jj;
    wxSize size;
    LIB_COMPONENT* component;
    CMP_LIBRARY* lib;

    GetScreen()->m_Curseur.x = 0;
    GetScreen()->m_Curseur.y = 0;
    bestzoom = 16;

    lib = CMP_LIBRARY::FindLibrary( m_libraryName );

    if( lib == NULL )
        return bestzoom;

    component = lib->FindComponent( m_entryName );

    if( component == NULL )
        return bestzoom;

    /*
     * This fixes a bug where the client size of the drawing area is not
     * correctly reported until after the window is shown.  This is most
     * likely due to the unmanaged windows ( vertical tool bars and message
     * panel ) that are drawn in the main window which wxWidgets knows
     * nothing about.  When the library editor is reopened with a component
     * already loading, the zoom will be calculated correctly.
     */
    if( !IsShownOnScreen() )
    {
        if( m_clientSize != wxSize( -1, -1 ) )
            size = m_clientSize;
        else
            size = DrawPanel->GetClientSize();
    }
    else
    {
        if( m_clientSize == wxSize( -1, -1 ) )
            m_clientSize = DrawPanel->GetClientSize();
        size = m_clientSize;
    }

    EDA_Rect BoundaryBox = component->GetBoundaryBox( m_unit, m_convert );

    // Reserve a 25 mils margin around component bounding box.
    size -= wxSize( 25, 25 );
    ii   = wxRound( ( (double) BoundaryBox.GetWidth() / double( size.x ) ) *
                    (double) GetScreen()->m_ZoomScalar );
    jj   = wxRound( ( (double) BoundaryBox.GetHeight() / (double) size.y ) *
                    (double) GetScreen()->m_ZoomScalar );
    bestzoom = MAX( ii, jj ) + 1;

    GetScreen()->m_Curseur = BoundaryBox.Centre();

    return bestzoom;
}


/**
 * Function ReCreateListLib
 *
 * Creates or recreates the list of current loaded libraries.
 * This list is sorted, with the library cache always at end of the list
 */
void WinEDA_ViewlibFrame::ReCreateListLib()
{
    if( m_LibList == NULL )
        return;

    m_LibList->Clear();
    m_LibList->Append( CMP_LIBRARY::GetLibraryNames() );

    // Search for a previous selection:
    int index =  m_LibList->FindString( m_libraryName );

    if( index != wxNOT_FOUND )
    {
        m_LibList->SetSelection( index, true );
    }
    else
    {
        /* If not found, clear current library selection because it can be
         * deleted after a config change. */
        m_libraryName = wxEmptyString;
        m_entryName = wxEmptyString;
        m_unit = 1;
        m_convert = 1;
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

    m_CmpList->Clear();

    CMP_LIBRARY* Library = CMP_LIBRARY::FindLibrary( m_libraryName );

    if( Library == NULL )
    {
        m_libraryName = wxEmptyString;
        m_entryName = wxEmptyString;
        m_convert = 1;
        m_unit    = 1;
        return;
    }

    wxArrayString  nameList;
    Library->GetEntryNames( nameList );
    m_CmpList->Append( nameList );

    int index = m_CmpList->FindString( m_entryName );

    if( index == wxNOT_FOUND )
    {
        m_entryName = wxEmptyString;
        m_convert = 1;
        m_unit    = 1;
    }
    else
    {
        m_CmpList->SetSelection( index, true );
    }
}


void WinEDA_ViewlibFrame::ClickOnLibList( wxCommandEvent& event )
{
    int ii = m_LibList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_LibList->GetString( ii );
    if( m_libraryName == name )
        return;
    m_libraryName = name;
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

    if( m_entryName.CmpNoCase( name ) != 0 )
    {
        m_entryName = name;
        DisplayLibInfos();
        m_unit    = 1;
        m_convert = 1;
        Zoom_Automatique( false );
        ReCreateHToolbar();
        DrawPanel->Refresh();
    }
}



/*
 * Export the current component to schematic and close the library browser
 */
void WinEDA_ViewlibFrame::ExportToSchematicLibraryPart( wxCommandEvent& event )
{
    int ii = m_CmpList->GetSelection();

    if( ii >= 0 )
        m_entryName = m_CmpList->GetString( ii );
    else
        m_entryName.Empty();
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

    // Set parameters to a reasonable value.
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

/** Called on activate the frame.
 * Reload the libraries lists that can be changed by the schematic editor or the library editor
 */
void WinEDA_ViewlibFrame::OnActivate( wxActivateEvent& event )
{
    WinEDA_DrawFrame::OnActivate( event );

    if( m_LibList )
        ReCreateListLib();
    DisplayLibInfos();
}

/**********************/
/** displayframe.cpp **/
/**********************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "macros.h"

#include "cvpcb.h"
#include "bitmaps.h"
#include "protos.h"
#include "cvstruct.h"
#include "class_DisplayFootprintsFrame.h"
#include "cvpcb_id.h"

/*
 * NOTE: There is something in 3d_viewer.h that causes a compiler error in
 *       <boost/foreach.hpp> in Linux so move it after cvpcb.h where it is
 *       included to prevent the error from occurring.
 */
#include "3d_viewer.h"



BEGIN_EVENT_TABLE( DISPLAY_FOOTPRINTS_FRAME, WinEDA_BasePcbFrame )
    EVT_CLOSE( DISPLAY_FOOTPRINTS_FRAME::OnCloseWindow )
    EVT_SIZE( DISPLAY_FOOTPRINTS_FRAME::OnSize )
    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, DISPLAY_FOOTPRINTS_FRAME::OnZoom )
    EVT_TOOL( ID_OPTIONS_SETUP, DISPLAY_FOOTPRINTS_FRAME::InstallOptionsDisplay )
    EVT_TOOL( ID_CVPCB_SHOW3D_FRAME, DISPLAY_FOOTPRINTS_FRAME::Show3D_Frame )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_GRID, DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar)
    EVT_TOOL( ID_TB_OPTIONS_SHOW_POLAR_COORD, DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar)
    EVT_TOOL( ID_TB_OPTIONS_SELECT_UNIT_INCH, DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar)
    EVT_TOOL( ID_TB_OPTIONS_SELECT_UNIT_MM, DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar)
    EVT_TOOL( ID_TB_OPTIONS_SELECT_CURSOR, DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar)
    EVT_TOOL( ID_TB_OPTIONS_SHOW_PADS_SKETCH, DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar)
    EVT_TOOL( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH, DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar)
    EVT_TOOL( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH, DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar)
END_EVENT_TABLE()


/***************************************************************************/
/* DISPLAY_FOOTPRINTS_FRAME: the frame to display the current focused footprint */
/***************************************************************************/

DISPLAY_FOOTPRINTS_FRAME::DISPLAY_FOOTPRINTS_FRAME( WinEDA_CvpcbFrame* father,
                                          const wxString& title,
                                          const wxPoint& pos,
                                          const wxSize& size, long style ) :
    WinEDA_BasePcbFrame( father, CVPCB_DISPLAY_FRAME, title, pos,
                         size, style )
{
    m_FrameName = wxT( "CmpFrame" );
    m_Draw_Axis = true;         // TRUE to draw axis.

    // Give an icon
#ifdef __WINDOWS__
    SetIcon( wxICON( a_icon_cvpcb ) );
#else
    SetIcon( wxICON( icon_cvpcb ) );
#endif
    SetTitle( title );

    SetBoard( new BOARD( NULL, this ) );
    SetBaseScreen( new PCB_SCREEN() );

    LoadSettings();
    // Initialize grid id to a default value if not found in config or bad:
    if( (m_LastGridSizeId <= 0) ||
        (m_LastGridSizeId > (ID_POPUP_GRID_USER - ID_POPUP_GRID_LEVEL_1000)) )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_500 - ID_POPUP_GRID_LEVEL_1000;
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );

    // Initialize some display options
    DisplayOpt.DisplayPadIsol = false;      // Pad clearance has no meaning here
    DisplayOpt.ShowTrackClearanceMode = 0;  /* tracks and vias clearance has
                                             * no meaning here. */


    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

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

    m_auimgr.AddPane( m_HToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top().
                     Row( 0 ) );

    if( m_VToolBar )    // Currently, no vertical right toolbar.
        m_auimgr.AddPane( m_VToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right() );

    m_auimgr.AddPane( DrawPanel,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    m_auimgr.AddPane( MsgPanel,
                      wxAuiPaneInfo( horiz ).Name( wxT( "MsgPanel" ) ).Bottom() );

    m_auimgr.AddPane( m_OptionsToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_OptionsToolBar" ) ).Left() );

    m_auimgr.Update();

    Show( TRUE );
}


DISPLAY_FOOTPRINTS_FRAME::~DISPLAY_FOOTPRINTS_FRAME()
{
    delete GetBoard();

    delete GetBaseScreen();
    SetBaseScreen( 0 );

    ( (WinEDA_CvpcbFrame*) wxGetApp().GetTopWindow() )->DrawFrame = NULL;
}

/* Called when the frame is closed
 *  Save current settings (frame position and size
 */
void DISPLAY_FOOTPRINTS_FRAME::OnCloseWindow( wxCloseEvent& event )
{
    wxPoint pos;
    wxSize  size;

    size = GetSize();
    pos  = GetPosition();

    SaveSettings();
    Destroy();
}


void DISPLAY_FOOTPRINTS_FRAME::ReCreateVToolbar()
{
    // Currently, no vertical right toolbar.
    // So do nothing
}


void DISPLAY_FOOTPRINTS_FRAME::ReCreateOptToolbar()
{
    if( m_OptionsToolBar )
        return;

    // Create options tool bar.
    m_OptionsToolBar = new WinEDA_Toolbar( TOOLBAR_OPTION, this,
                                           ID_OPT_TOOLBAR, FALSE );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString,
                                wxBitmap( grid_xpm ),
                               _( "Hide grid" ), wxITEM_CHECK );
    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID,IsGridVisible() );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               wxBitmap( polar_coord_xpm ),
                               _( "Display Polar Coord ON" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               wxBitmap( unit_inch_xpm ),
                               _( "Units in inches" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               wxBitmap( unit_mm_xpm ),
                               _( "Units in millimeters" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               wxBitmap( cursor_shape_xpm ),
                               _( "Change Cursor Shape" ), wxITEM_CHECK  );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               wxBitmap( pad_sketch_xpm ),
                               _( "Show Pads Sketch" ), wxITEM_CHECK  );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH,
                               wxEmptyString,
                               wxBitmap( text_sketch_xpm ),
                               _( "Show Texts Sketch" ), wxITEM_CHECK  );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH,
                               wxEmptyString,
                               wxBitmap( show_mod_edge_xpm ),
                               _( "Show Edges Sketch" ), wxITEM_CHECK  );

    m_OptionsToolBar->Realize();

    SetToolbars();
}


void DISPLAY_FOOTPRINTS_FRAME::ReCreateHToolbar()
{
    if( m_HToolBar != NULL )
        return;

    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );

    m_HToolBar->AddTool( ID_OPTIONS_SETUP, wxEmptyString,
                         wxBitmap( display_options_xpm ),
                         _( "Display Options" ) );

    m_HToolBar->AddSeparator();

    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                         wxBitmap( zoom_in_xpm ),
                         _( "Zoom in (F1)" ) );

    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                         wxBitmap( zoom_out_xpm ),
                         _( "Zoom out (F2)" ) );

    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                         wxBitmap( zoom_redraw_xpm ),
                         _( "Redraw view (F3)" ) );

    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                         wxBitmap( zoom_auto_xpm ),
                         _( "Zoom auto (Home)" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_SHOW3D_FRAME, wxEmptyString,
                         wxBitmap( show_3d_xpm ),
                         _( "3D Display" ) );

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_HToolBar->Realize();
}


void DISPLAY_FOOTPRINTS_FRAME::SetToolbars()
{
    if( m_OptionsToolBar )
    {
        m_OptionsToolBar->ToggleTool(
            ID_TB_OPTIONS_SELECT_UNIT_MM,
            g_UserUnit == MILLIMETRES );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH,
                                      g_UserUnit == INCHES );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                      DisplayOpt.DisplayPolarCood );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                            DisplayOpt.DisplayPolarCood ?
                                            _( "Display rectangular coordinates" ) :
                                            _( "Display polar coordinates" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID,
                                      IsGridVisible( ) );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_GRID,
                                            IsGridVisible( ) ?
                                            _( "Hide grid" ) :
                                            _( "Show grid" ) );


        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_CURSOR,
                                      m_CursorShape );


        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                      !m_DisplayPadFill );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                            m_DisplayPadFill ?
                                            _( "Show pads in sketch mode" ) :
                                            _( "Show pads in filled mode" ) );

        wxString msgTextsFill[3] =
        {_( "Show texts in line mode" ),
         _( "Show texts in filled mode" ),
         _( "Show texts in sketch mode" )
        };
        unsigned idx = m_DisplayModText+1;
        if ( idx > 2 )
            idx = 0;
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH,
                                      m_DisplayModText == 0 );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH,
                                            msgTextsFill[idx] );

        wxString msgEdgesFill[3] =
        {_( "Show outlines in line mode" ),
         _( "Show outlines in filled mode" ),
         _( "Show outlines in sketch mode" )
        };
        idx = m_DisplayModEdge+1;
        if ( idx > 2 )
            idx = 0;
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH,
                                      m_DisplayModEdge == 0 );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH,
                                            msgEdgesFill[idx] );

        m_OptionsToolBar->Refresh();
    }
}


void DISPLAY_FOOTPRINTS_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


void DISPLAY_FOOTPRINTS_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
{
}


bool DISPLAY_FOOTPRINTS_FRAME::OnRightClick( const wxPoint& MousePos,
                                        wxMenu* PopMenu )
{
    return true;
}

void DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int        id = event.GetId();

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_GRID:
        SetGridVisibility( m_OptionsToolBar->GetToolState( id ) );
        DrawPanel->Refresh( );
        break;

    case ID_TB_OPTIONS_SELECT_UNIT_MM:
        g_UserUnit = MILLIMETRES;
        UpdateStatusBar();
        break;

    case ID_TB_OPTIONS_SELECT_UNIT_INCH:
        g_UserUnit = INCHES;
        UpdateStatusBar();
        break;

    case ID_TB_OPTIONS_SHOW_POLAR_COORD:
        Affiche_Message( wxEmptyString );
        DisplayOpt.DisplayPolarCood = m_OptionsToolBar->GetToolState( id );
        UpdateStatusBar();
        break;

    case ID_TB_OPTIONS_SELECT_CURSOR:
        m_CursorShape = m_OptionsToolBar->GetToolState( id );
        DrawPanel->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_PADS_SKETCH:
        m_DisplayPadFill = !m_OptionsToolBar->GetToolState( id );
        DrawPanel->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH:
        m_DisplayModText++;
        if( m_DisplayModText > 2 )
            m_DisplayModText = 0;
        DrawPanel->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH:
        m_DisplayModEdge++;
        if( m_DisplayModEdge > 2 )
            m_DisplayModEdge = 0;
        DrawPanel->Refresh( );
        break;

    default:
        DisplayError( this,
                      wxT( "DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar error" ) );
        break;
    }

    SetToolbars();
}

void DISPLAY_FOOTPRINTS_FRAME::GeneralControle( wxDC* aDC, wxPoint aPosition )
{
    wxRealPoint gridSize;
    int         flagcurseur = 0;
    wxPoint     oldpos;
    wxPoint     pos = aPosition;

    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    PutOnGrid( &pos );
    oldpos = GetScreen()->m_Curseur;
    gridSize = GetScreen()->GetGridSize();

    switch( g_KeyPressed )
    {
    case WXK_F1:
        cmd.SetId( ID_POPUP_ZOOM_IN );
        GetEventHandler()->ProcessEvent( cmd );
        flagcurseur = 2;
        pos = GetScreen()->m_Curseur;
        break;

    case WXK_F2:
        cmd.SetId( ID_POPUP_ZOOM_OUT );
        GetEventHandler()->ProcessEvent( cmd );
        flagcurseur = 2;
        pos = GetScreen()->m_Curseur;
        break;

    case WXK_F3:
        cmd.SetId( ID_ZOOM_REDRAW );
        GetEventHandler()->ProcessEvent( cmd );
        flagcurseur = 2;
        break;

    case WXK_F4:
        cmd.SetId( ID_POPUP_ZOOM_CENTER );
        GetEventHandler()->ProcessEvent( cmd );
        flagcurseur = 2;
        pos = GetScreen()->m_Curseur;
        break;

    case WXK_HOME:
        cmd.SetId( ID_ZOOM_PAGE );
        GetEventHandler()->ProcessEvent( cmd );
        flagcurseur = 2;
        pos = GetScreen()->m_Curseur;
        break;

    case ' ':
        GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
        break;

    case WXK_NUMPAD8:       /* cursor moved up */
    case WXK_UP:
        pos.y -= wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD2:       /* cursor moved down */
    case WXK_DOWN:
        pos.y += wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD4:       /*  cursor moved left */
    case WXK_LEFT:
        pos.x -= wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD6:      /*  cursor moved right */
    case WXK_RIGHT:
        pos.x += wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;
    }

    GetScreen()->m_Curseur = pos;

    if( GetScreen()->IsRefreshReq() )
    {
        flagcurseur = 2;
        Refresh();
    }

    if( oldpos != GetScreen()->m_Curseur )
    {
        if( flagcurseur != 2 )
        {
            pos = GetScreen()->m_Curseur;
            GetScreen()->m_Curseur = oldpos;
            DrawPanel->CursorOff( aDC );
            GetScreen()->m_Curseur = pos;
            DrawPanel->CursorOn( aDC );
        }

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, aDC, 0 );
        }
    }

    UpdateStatusBar();    /* Display new cursor coordinates */
}


/**
 * Display 3D frame of current footprint selection.
 */
void DISPLAY_FOOTPRINTS_FRAME::Show3D_Frame( wxCommandEvent& event )
{
    if( m_Draw3DFrame )
    {
        DisplayInfoMessage( this, _( "3D Frame already opened" ) );
        return;
    }

    m_Draw3DFrame = new WinEDA3D_DrawFrame( this, _( "3D Viewer" ),
                                            KICAD_DEFAULT_3D_DRAWFRAME_STYLE |
                                            wxFRAME_FLOAT_ON_PARENT );
    m_Draw3DFrame->Show( TRUE );
}

/* Virtual function needed by the PCB_SCREEN class derived from BASE_SCREEN
* this is a virtual pure function in BASE_SCREEN
* do nothing in cvpcb
* could be removed later
*/
void PCB_SCREEN::ClearUndoORRedoList(UNDO_REDO_CONTAINER&, int )
{
}

/**
 * Function IsGridVisible() , virtual
 * @return true if the grid must be shown
 */
bool DISPLAY_FOOTPRINTS_FRAME::IsGridVisible()
{
    return m_DrawGrid;
}

/**
 * Function SetGridVisibility() , virtual
 * It may be overloaded by derived classes
 * if you want to store/retrieve the grid visiblity in configuration.
 * @param aVisible = true if the grid must be shown
 */
void DISPLAY_FOOTPRINTS_FRAME::SetGridVisibility(bool aVisible)
{
    m_DrawGrid = aVisible;
}

/**
 * Function GetGridColor() , virtual
 * @return the color of the grid
 */
int DISPLAY_FOOTPRINTS_FRAME::GetGridColor()
{
    return DARKGRAY;
}


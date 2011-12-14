/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2007-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file class_DisplayFootprintsFrame.cpp
 */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "macros.h"
#include "bitmaps.h"

#include "class_board.h"

#include "cvpcb.h"
#include "cvpcb_mainframe.h"
#include "class_DisplayFootprintsFrame.h"
#include "cvpcb_id.h"

/*
 * NOTE: There is something in 3d_viewer.h that causes a compiler error in
 *       <boost/foreach.hpp> in Linux so move it after cvpcb.h where it is
 *       included to prevent the error from occurring.
 */
#include "3d_viewer.h"



BEGIN_EVENT_TABLE( DISPLAY_FOOTPRINTS_FRAME, PCB_BASE_FRAME )
    EVT_CLOSE( DISPLAY_FOOTPRINTS_FRAME::OnCloseWindow )
    EVT_SIZE( DISPLAY_FOOTPRINTS_FRAME::OnSize )
    EVT_TOOL( ID_OPTIONS_SETUP, DISPLAY_FOOTPRINTS_FRAME::InstallOptionsDisplay )
    EVT_TOOL( ID_CVPCB_SHOW3D_FRAME, DISPLAY_FOOTPRINTS_FRAME::Show3D_Frame )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH,
              DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar)
    EVT_TOOL( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH,
              DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar)

    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH,
                   DISPLAY_FOOTPRINTS_FRAME::OnUpdateTextDrawMode )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH,
                   DISPLAY_FOOTPRINTS_FRAME::OnUpdateLineDrawMode )
END_EVENT_TABLE()


/***************************************************************************/
/* DISPLAY_FOOTPRINTS_FRAME: the frame to display the current focused footprint */
/***************************************************************************/

DISPLAY_FOOTPRINTS_FRAME::DISPLAY_FOOTPRINTS_FRAME( CVPCB_MAINFRAME* father,
                                                    const wxString& title,
                                                    const wxPoint& pos,
                                                    const wxSize& size, long style ) :
    PCB_BASE_FRAME( father, CVPCB_DISPLAY_FRAME, title, pos, size, style )
{
    m_FrameName = wxT( "CmpFrame" );
    m_showAxis = true;         // true to draw axis.

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( icon_cvpcb_xpm ) );
    SetIcon( icon );

    SetBoard( new BOARD() );
    SetScreen( new PCB_SCREEN() );

    LoadSettings();

    // Initialize grid id to a default value if not found in config or bad:
    if( (m_LastGridSizeId <= 0) ||
        (m_LastGridSizeId > (ID_POPUP_GRID_USER - ID_POPUP_GRID_LEVEL_1000)) )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_500 - ID_POPUP_GRID_LEVEL_1000;
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );

    // Initialize some display options
    DisplayOpt.DisplayPadIsol = false;      // Pad clearance has no meaning here

    // Track and via clearance has no meaning here.
    DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    m_auimgr.SetManagedWindow( this );

    EDA_PANEINFO horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO vert;
    vert.VerticalToolbarPane();

    EDA_PANEINFO mesg;
    mesg.MessageToolbarPane();



    m_auimgr.AddPane( m_HToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top(). Row( 0 ) );

    if( m_drawToolBar )    // Currently, no vertical right toolbar.
        m_auimgr.AddPane( m_drawToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_drawToolBar" ) ).Right() );

    m_auimgr.AddPane( DrawPanel,
                      wxAuiPaneInfo().Name( wxT( "DisplayFrame" ) ).CentrePane() );

    m_auimgr.AddPane( m_messagePanel,
                      wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom().Layer(10) );

    m_auimgr.AddPane( m_optionsToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_optionsToolBar" ) ).Left() );

    m_auimgr.Update();

    Show( true );
}


DISPLAY_FOOTPRINTS_FRAME::~DISPLAY_FOOTPRINTS_FRAME()
{
    delete GetBoard();
    delete GetScreen();
    SetScreen( NULL );

    ( (CVPCB_MAINFRAME*) wxGetApp().GetTopWindow() )->m_DisplayFootprintFrame = NULL;
}

/* Called when the frame is closed
 *  Save current settings (frame position and size
 */
void DISPLAY_FOOTPRINTS_FRAME::OnCloseWindow( wxCloseEvent& event )
{
    if( m_Draw3DFrame )
        m_Draw3DFrame->Close( true );

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
    if( m_optionsToolBar )
        return;

    // Create options tool bar.
    m_optionsToolBar = new EDA_TOOLBAR( TOOLBAR_OPTION, this, ID_OPT_TOOLBAR, false );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, KiBitmap( grid_xpm ),
                               _( "Hide grid" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               KiBitmap( polar_coord_xpm ),
                               _( "Display polar coordinates" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiBitmap( unit_inch_xpm ),
                               _( "Units in inches" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiBitmap( unit_mm_xpm ),
                               _( "Units in millimeters" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiBitmap( cursor_shape_xpm ),
                               _( "Change cursor shape" ), wxITEM_CHECK  );

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               KiBitmap( pad_sketch_xpm ),
                               _( "Show pads in outline mode" ), wxITEM_CHECK  );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH, wxEmptyString,
                               KiBitmap( text_sketch_xpm ),
                               _( "Show texts in line mode" ), wxITEM_CHECK  );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH, wxEmptyString,
                               KiBitmap( show_mod_edge_xpm ),
                               _( "Show outlines in line mode" ), wxITEM_CHECK  );

    m_optionsToolBar->Realize();
}


void DISPLAY_FOOTPRINTS_FRAME::ReCreateHToolbar()
{
    if( m_HToolBar != NULL )
        return;

    m_HToolBar = new EDA_TOOLBAR( TOOLBAR_MAIN, this, ID_H_TOOLBAR, true );

    m_HToolBar->AddTool( ID_OPTIONS_SETUP, wxEmptyString, KiBitmap( display_options_xpm ),
                         _( "Display options" ) );

    m_HToolBar->AddSeparator();

    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ),
                         _( "Zoom in (F1)" ) );

    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ),
                         _( "Zoom out (F2)" ) );

    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiBitmap( zoom_redraw_xpm ),
                         _( "Redraw view (F3)" ) );

    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ),
                         _( "Zoom auto (Home)" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_SHOW3D_FRAME, wxEmptyString, KiBitmap( three_d_xpm ),
                         _( "3D Display" ) );

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_HToolBar->Realize();
}


void DISPLAY_FOOTPRINTS_FRAME::OnUpdateTextDrawMode( wxUpdateUIEvent& aEvent )
{
    wxString msgTextsFill[3] = { _( "Show texts in line mode" ),
                                 _( "Show texts in filled mode" ),
                                 _( "Show texts in sketch mode" ) };

    unsigned i = m_DisplayModText + 1;

    if ( i > 2 )
        i = 0;

    aEvent.Check( m_DisplayModText == 0 );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH, msgTextsFill[i] );

}


void DISPLAY_FOOTPRINTS_FRAME::OnUpdateLineDrawMode( wxUpdateUIEvent& aEvent )
{
    wxString msgEdgesFill[3] = { _( "Show outlines in line mode" ),
                                 _( "Show outlines in filled mode" ),
                                 _( "Show outlines in sketch mode" ) };

    int i = m_DisplayModEdge + 1;

    if ( i > 2 )
        i = 0;

    aEvent.Check( m_DisplayModEdge == 0 );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH, msgEdgesFill[i] );
}


void DISPLAY_FOOTPRINTS_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


void DISPLAY_FOOTPRINTS_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
{
}


bool DISPLAY_FOOTPRINTS_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    return true;
}


void DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int        id = event.GetId();

    switch( id )
    {
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

        DrawPanel->Refresh();
        break;

    default:
        DisplayError( this,
                      wxT( "DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar error" ) );
        break;
    }
}


void DISPLAY_FOOTPRINTS_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
{
    wxRealPoint gridSize;
    wxPoint     oldpos;
    PCB_SCREEN* screen = GetScreen();
    wxPoint     pos = aPosition;

    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    pos = screen->GetNearestGridPosition( pos );
    oldpos = screen->GetCrossHairPosition();
    gridSize = screen->GetGridSize();

    switch( aHotKey )
    {
    case WXK_F1:
        cmd.SetId( ID_POPUP_ZOOM_IN );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F2:
        cmd.SetId( ID_POPUP_ZOOM_OUT );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F3:
        cmd.SetId( ID_ZOOM_REDRAW );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F4:
        cmd.SetId( ID_POPUP_ZOOM_CENTER );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_HOME:
        cmd.SetId( ID_ZOOM_PAGE );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case ' ':
        screen->m_O_Curseur = screen->GetCrossHairPosition();
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

    screen->SetCrossHairPosition( pos );

    if( oldpos != screen->GetCrossHairPosition() )
    {
        pos = screen->GetCrossHairPosition();
        screen->SetCrossHairPosition( oldpos );
        DrawPanel->CrossHairOff( aDC );
        screen->SetCrossHairPosition( pos );
        DrawPanel->CrossHairOn( aDC );

        if( DrawPanel->IsMouseCaptured() )
        {
            DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, aPosition, 0 );
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
        // Raising the window does not show the window on Windows if iconized.
        // This should work on any platform.
        if( m_Draw3DFrame->IsIconized() )
             m_Draw3DFrame->Iconize( false );

        m_Draw3DFrame->Raise();

        // Raising the window does not set the focus on Linux.  This should work on any platform.
        if( wxWindow::FindFocus() != m_Draw3DFrame )
            m_Draw3DFrame->SetFocus();

        return;
    }

    m_Draw3DFrame = new EDA_3D_FRAME( this, _( "3D Viewer" ), KICAD_DEFAULT_3D_DRAWFRAME_STYLE );
    m_Draw3DFrame->Show( true );
}


/**
 * Virtual function needed by the PCB_SCREEN class derived from BASE_SCREEN
 * this is a virtual pure function in BASE_SCREEN
 * do nothing in Cvpcb
 * could be removed later
 */
void PCB_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER&, int )
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
 * if you want to store/retrieve the grid visibility in configuration.
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

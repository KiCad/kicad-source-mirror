/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2007-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pgm_base.h>
#include <common.h>
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>
#include <confirm.h>
#include <macros.h>
#include <bitmaps.h>
#include <msgpanel.h>
#include <wildcards_and_files_ext.h>
#include <fpid.h>
#include <fp_lib_table.h>
#include <pcbcommon.h>

#include <io_mgr.h>
#include <class_module.h>
#include <class_board.h>

#include <cvpcb_mainframe.h>
#include <class_DisplayFootprintsFrame.h>
#include <cvpcb_id.h>
#include <cvstruct.h>

#include <3d_viewer.h>


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


DISPLAY_FOOTPRINTS_FRAME::DISPLAY_FOOTPRINTS_FRAME( KIWAY* aKiway, CVPCB_MAINFRAME* aParent ) :
    PCB_BASE_FRAME( aKiway, aParent, FRAME_CVPCB_DISPLAY, _( "Footprint Viewer" ),
        wxDefaultPosition, wxDefaultSize,
        KICAD_DEFAULT_DRAWFRAME_STYLE, FOOTPRINTVIEWER_FRAME_NAME )
{
    m_showAxis = true;         // true to draw axis.

    // Give an icon
    wxIcon  icon;

    icon.CopyFromBitmap( KiBitmap( icon_cvpcb_xpm ) );
    SetIcon( icon );

    SetBoard( new BOARD() );
    SetScreen( new PCB_SCREEN( GetPageSizeIU() ) );

    LoadSettings( config() );

    // Initialize grid id to a default value if not found in config or bad:
    if( (m_LastGridSizeId <= 0) ||
        (m_LastGridSizeId > (ID_POPUP_GRID_USER - ID_POPUP_GRID_LEVEL_1000)) )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_500 - ID_POPUP_GRID_LEVEL_1000;

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );

    // Initialize some display options
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();
    displ_opts->m_DisplayPadIsol = false;      // Pad clearance has no meaning here

    // Track and via clearance has no meaning here.
    displ_opts->m_ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;

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

    m_auimgr.AddPane( m_mainToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_mainToolBar" ) ).Top(). Row( 0 ) );

    if( m_drawToolBar )    // Currently, no vertical right toolbar.
        m_auimgr.AddPane( m_drawToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_drawToolBar" ) ).Right() );

    m_auimgr.AddPane( m_canvas,
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
    delete GetScreen();
    SetScreen( NULL );      // Be sure there is no double deletion
}


void DISPLAY_FOOTPRINTS_FRAME::OnCloseWindow( wxCloseEvent& event )
{
    if( m_Draw3DFrame )
        m_Draw3DFrame->Close( true );

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
    m_optionsToolBar = new wxAuiToolBar( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                         wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

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
    if( m_mainToolBar != NULL )
        return;

    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    m_mainToolBar->AddTool( ID_OPTIONS_SETUP, wxEmptyString, KiBitmap( display_options_xpm ),
                            _( "Display options" ) );

    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ),
                            _( "Zoom in (F1)" ) );

    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ),
                            _( "Zoom out (F2)" ) );

    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiBitmap( zoom_redraw_xpm ),
                            _( "Redraw view (F3)" ) );

    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ),
                            _( "Zoom auto (Home)" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_SHOW3D_FRAME, wxEmptyString, KiBitmap( three_d_xpm ),
                            _( "3D Display (Alt+3)" ) );

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_mainToolBar->Realize();
}


void DISPLAY_FOOTPRINTS_FRAME::OnUpdateTextDrawMode( wxUpdateUIEvent& aEvent )
{
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();

    wxString msgTextsFill[2] = { _( "Show texts in filled mode" ),
                                 _( "Show texts in sketch mode" ) };

    unsigned i = displ_opts->m_DisplayModTextFill == SKETCH ? 0 : 1;

    aEvent.Check( displ_opts->m_DisplayModTextFill == SKETCH );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH, msgTextsFill[i] );

}


void DISPLAY_FOOTPRINTS_FRAME::OnUpdateLineDrawMode( wxUpdateUIEvent& aEvent )
{
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();

    wxString msgEdgesFill[2] = { _( "Show outlines in filled mode" ),
                                 _( "Show outlines in sketch mode" ) };

    int i = displ_opts->m_DisplayModEdgeFill == SKETCH ? 0 : 1;

    aEvent.Check( displ_opts->m_DisplayModEdgeFill == SKETCH );
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
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH:
        displ_opts->m_DisplayModTextFill = displ_opts->m_DisplayModTextFill == FILLED ? SKETCH : FILLED;
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH:
        displ_opts->m_DisplayModEdgeFill = displ_opts->m_DisplayModEdgeFill == FILLED ? SKETCH : FILLED;
        m_canvas->Refresh();
        break;

    default:
        DisplayError( this,
                      wxT( "DISPLAY_FOOTPRINTS_FRAME::OnSelectOptionToolbar error" ) );
        break;
    }
}


bool DISPLAY_FOOTPRINTS_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
{
    bool eventHandled = true;

    // Filter out the 'fake' mouse motion after a keyboard movement
    if( !aHotKey && m_movingCursorWithKeyboard )
    {
        m_movingCursorWithKeyboard = false;
        return false;
    }

    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    wxPoint pos = aPosition;
    wxPoint oldpos = GetCrossHairPosition();
    GeneralControlKeyMovement( aHotKey, &pos, true );

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
        GetScreen()->m_O_Curseur = GetCrossHairPosition();
        break;

    case GR_KB_ALT + '3':
        cmd.SetId( ID_CVPCB_SHOW3D_FRAME );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    default:
        eventHandled = false;
    }

    SetCrossHairPosition( pos );
    RefreshCrossHair( oldpos, aPosition, aDC );

    UpdateStatusBar();    /* Display new cursor coordinates */

    return eventHandled;
}


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

    m_Draw3DFrame = new EDA_3D_FRAME( &Kiway(), this, _( "3D Viewer" ), KICAD_DEFAULT_3D_DRAWFRAME_STYLE );
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


bool DISPLAY_FOOTPRINTS_FRAME::IsGridVisible() const
{
    return m_drawGrid;
}


void DISPLAY_FOOTPRINTS_FRAME::SetGridVisibility(bool aVisible)
{
    m_drawGrid = aVisible;
}


EDA_COLOR_T DISPLAY_FOOTPRINTS_FRAME::GetGridColor() const
{
    return DARKGRAY;
}


MODULE* DISPLAY_FOOTPRINTS_FRAME::Get_Module( const wxString& aFootprintName )
{
    MODULE* footprint = NULL;

    try
    {
        FPID fpid;

        if( fpid.Parse( aFootprintName ) >= 0 )
        {
            DisplayInfoMessage( this, wxString::Format( wxT( "Footprint ID <%s> is not valid." ),
                                                        GetChars( aFootprintName ) ) );
            return NULL;
        }

        std::string nickname = fpid.GetLibNickname();
        std::string fpname   = fpid.GetFootprintName();

        wxLogDebug( wxT( "Load footprint <%s> from library <%s>." ),
                    fpname.c_str(), nickname.c_str()  );

        footprint = Prj().PcbFootprintLibs()->FootprintLoad(
                FROM_UTF8( nickname.c_str() ), FROM_UTF8( fpname.c_str() ) );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.errorText );
        return NULL;
    }

    if( footprint )
    {
        footprint->SetParent( (EDA_ITEM*) GetBoard() );
        footprint->SetPosition( wxPoint( 0, 0 ) );
        return footprint;
    }

    wxString msg = wxString::Format( _( "Footprint '%s' not found" ), aFootprintName.GetData() );
    DisplayError( this, msg );
    return NULL;
}


void DISPLAY_FOOTPRINTS_FRAME::InitDisplay()
{
    wxString msg;

    CVPCB_MAINFRAME* parentframe = (CVPCB_MAINFRAME *) GetParent();

    wxString footprintName = parentframe->m_footprintListBox->GetSelectedFootprint();

    if( !footprintName.IsEmpty() )
    {
        msg.Printf( _( "Footprint: %s" ), GetChars( footprintName ) );

        SetTitle( msg );
        const FOOTPRINT_INFO* module_info = parentframe->m_footprints.GetModuleInfo( footprintName );

        const wxChar* libname;

        if( module_info )
            libname = GetChars( module_info->GetNickname() );
        else
            libname = GetChars( wxT( "???" ) );

        msg.Printf( _( "Lib: %s" ), libname );

        SetStatusText( msg, 0 );

        if( GetBoard()->m_Modules.GetCount() )
        {
            // there is only one module in the list
            GetBoard()->m_Modules.DeleteAll();
        }

        MODULE* module = Get_Module( footprintName );

        if( module )
            GetBoard()->m_Modules.PushBack( module );

        Zoom_Automatique( false );
    }
    else   // No footprint to display. Erase old footprint, if any
    {
        if( GetBoard()->m_Modules.GetCount() )
        {
            GetBoard()->m_Modules.DeleteAll();
            Zoom_Automatique( false );
            SetStatusText( wxEmptyString, 0 );
        }
    }

    // Display new cursor coordinates and zoom value:
    UpdateStatusBar();

    GetCanvas()->Refresh();

    if( m_Draw3DFrame )
        m_Draw3DFrame->NewDisplay();
}


void DISPLAY_FOOTPRINTS_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( !GetBoard() )
        return;

    m_canvas->DrawBackGround( DC );
    GetBoard()->Draw( m_canvas, DC, GR_COPY );

    MODULE* Module = GetBoard()->m_Modules;

    if ( Module )
    {
        MSG_PANEL_ITEMS items;
        Module->GetMsgPanelInfo( items );
        SetMsgPanel( items );
    }

    m_canvas->DrawCrossHair( DC );
}


/*
 * Redraw the BOARD items but not cursors, axis or grid.
 */
void BOARD::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                  GR_DRAWMODE aDrawMode, const wxPoint& aOffset )
{
    if( m_Modules )
    {
        m_Modules->Draw( aPanel, aDC, GR_COPY );
    }
}

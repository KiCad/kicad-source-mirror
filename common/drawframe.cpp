/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file drawframe.cpp
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <gr_basic.h>
#include <common.h>
#include <bitmaps.h>
#include <macros.h>
#include <id.h>
#include <class_drawpanel.h>
#include <class_base_screen.h>
#include <wxstruct.h>
#include <confirm.h>
#include <kicad_device_context.h>
#include <dialog_helpers.h>

#include <wx/fontdlg.h>


/**
 * Definition for enabling and disabling scroll bar setting trace output.  See the
 * wxWidgets documentation on useing the WXTRACE environment variable.
 */
static const wxString traceScrollSettings( wxT( "KicadScrollSettings" ) );


// Configuration entry names.
static const wxString CursorShapeEntryKeyword( wxT( "CursorShape" ) );
static const wxString ShowGridEntryKeyword( wxT( "ShowGrid" ) );
static const wxString GridColorEntryKeyword( wxT( "GridColor" ) );
static const wxString LastGridSizeId( wxT( "_LastGridSize" ) );


BEGIN_EVENT_TABLE( EDA_DRAW_FRAME, EDA_BASE_FRAME )
    EVT_MOUSEWHEEL( EDA_DRAW_FRAME::OnMouseEvent )
    EVT_MENU_OPEN( EDA_DRAW_FRAME::OnMenuOpen )
    EVT_ACTIVATE( EDA_DRAW_FRAME::OnActivate )
    EVT_MENU_RANGE( ID_ZOOM_IN, ID_ZOOM_REDRAW, EDA_DRAW_FRAME::OnZoom )
    EVT_MENU_RANGE( ID_POPUP_ZOOM_START_RANGE, ID_POPUP_ZOOM_END_RANGE,
                    EDA_DRAW_FRAME::OnZoom )
    EVT_MENU_RANGE( ID_POPUP_GRID_LEVEL_1000, ID_POPUP_GRID_USER,
                    EDA_DRAW_FRAME::OnSelectGrid )

    EVT_TOOL( ID_TB_OPTIONS_SHOW_GRID, EDA_DRAW_FRAME::OnToggleGridState )
    EVT_TOOL_RANGE( ID_TB_OPTIONS_SELECT_UNIT_MM, ID_TB_OPTIONS_SELECT_UNIT_INCH,
                    EDA_DRAW_FRAME::OnSelectUnits )
    EVT_TOOL( ID_TB_OPTIONS_SELECT_CURSOR, EDA_DRAW_FRAME::OnToggleCrossHairStyle )

    EVT_UPDATE_UI( wxID_UNDO, EDA_DRAW_FRAME::OnUpdateUndo )
    EVT_UPDATE_UI( wxID_REDO, EDA_DRAW_FRAME::OnUpdateRedo )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SHOW_GRID, EDA_DRAW_FRAME::OnUpdateGrid )
    EVT_UPDATE_UI( ID_TB_OPTIONS_SELECT_CURSOR, EDA_DRAW_FRAME::OnUpdateCrossHairStyle )
    EVT_UPDATE_UI_RANGE( ID_TB_OPTIONS_SELECT_UNIT_MM, ID_TB_OPTIONS_SELECT_UNIT_INCH,
                         EDA_DRAW_FRAME::OnUpdateUnits )
END_EVENT_TABLE()


EDA_DRAW_FRAME::EDA_DRAW_FRAME( wxWindow* father, int idtype, const wxString& title,
                                const wxPoint& pos, const wxSize& size, long style ) :
    EDA_BASE_FRAME( father, idtype, title, pos, size, style )
{
    wxSize minsize;

    m_drawToolBar         = NULL;
    m_optionsToolBar      = NULL;
    m_gridSelectBox       = NULL;
    m_zoomSelectBox       = NULL;
    m_HotkeysZoomAndGridList = NULL;

    m_canvas              = NULL;
    m_messagePanel        = NULL;
    m_currentScreen       = NULL;
    m_toolId              = ID_NO_TOOL_SELECTED;
    m_lastDrawToolId      = ID_NO_TOOL_SELECTED;
    m_showAxis            = false;      // true to draw axis.
    m_showBorderAndTitleBlock = false;  // true to display reference sheet.
    m_showGridAxis        = false;      // true to draw the grid axis
    m_cursorShape         = 0;
    m_LastGridSizeId      = 0;
    m_DrawGrid            = true;       // hide/Show grid. default = show
    m_GridColor           = DARKGRAY;   // Grid color
    m_snapToGrid          = true;

    // Internal units per inch: = 1000 for schema, = 10000 for PCB
    m_internalUnits       = EESCHEMA_INTERNAL_UNIT;
    minsize.x             = 470;
    minsize.y             = 350 + m_MsgFrameHeight;

    SetSizeHints( minsize.x, minsize.y, -1, -1, -1, -1 );

    // Make sure window has a sane minimum size.
    if( ( size.x < minsize.x ) || ( size.y < minsize.y ) )
        SetSize( 0, 0, minsize.x, minsize.y );

    // Pane sizes for status bar.
    // @todo these should be sized based on typical text content, like
    // "dx -10.123 -10.123 dy -10.123 -10.123" using the system font which is
    // in play on a particular platform, and should not be constants.
    // Please do not reduce these constant values, and please use dynamic
    // system font specific sizing in the future.
    #define ZOOM_DISPLAY_SIZE       60
    #define COORD_DISPLAY_SIZE      165
    #define DELTA_DISPLAY_SIZE      190
    #define UNITS_DISPLAY_SIZE      65
    #define FUNCTION_DISPLAY_SIZE   110
    static const int dims[6] = { -1, ZOOM_DISPLAY_SIZE,
        COORD_DISPLAY_SIZE, DELTA_DISPLAY_SIZE,
        UNITS_DISPLAY_SIZE, FUNCTION_DISPLAY_SIZE };

    CreateStatusBar( 6 );
    SetStatusWidths( 6, dims );

    // Create child subwindows.
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y );
    m_FramePos.x   = m_FramePos.y = 0;
    m_FrameSize.y -= m_MsgFrameHeight;

    m_canvas = new EDA_DRAW_PANEL( this, -1, wxPoint( 0, 0 ), m_FrameSize );
    m_messagePanel  = new EDA_MSG_PANEL( this, -1, wxPoint( 0, m_FrameSize.y ),
                                         wxSize( m_FrameSize.x, m_MsgFrameHeight ) );

    m_messagePanel->SetBackgroundColour( wxColour( ColorRefs[LIGHTGRAY].m_Red,
                                                   ColorRefs[LIGHTGRAY].m_Green,
                                                   ColorRefs[LIGHTGRAY].m_Blue ) );
}


EDA_DRAW_FRAME::~EDA_DRAW_FRAME()
{
    SAFE_DELETE( m_currentScreen );

    m_auimgr.UnInit();
}


void EDA_DRAW_FRAME::unitsChangeRefresh()
{
    UpdateStatusBar();

    EDA_ITEM* item = GetScreen()->GetCurItem();

    if( item )
        item->DisplayInfo( this );
}


void EDA_DRAW_FRAME::EraseMsgBox()
{
    if( m_messagePanel )
        m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::OnActivate( wxActivateEvent& event )
{
    m_FrameIsActive = event.GetActive();

    if( m_canvas )
        m_canvas->SetCanStartBlock( -1 );

    event.Skip();   // required under wxMAC
}


void EDA_DRAW_FRAME::OnMenuOpen( wxMenuEvent& event )
{
    if( m_canvas )
        m_canvas->SetCanStartBlock( -1 );

    event.Skip();
}


void EDA_DRAW_FRAME::OnToggleGridState( wxCommandEvent& aEvent )
{
    SetGridVisibility( !IsGridVisible() );
    m_canvas->Refresh();
}


void EDA_DRAW_FRAME::OnSelectUnits( wxCommandEvent& aEvent )
{
    if( aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_MM && g_UserUnit != MILLIMETRES )
    {
        g_UserUnit = MILLIMETRES;
        unitsChangeRefresh();
    }
    else if( aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_INCH && g_UserUnit != INCHES )
    {
        g_UserUnit = INCHES;
        unitsChangeRefresh();
    }
}


void EDA_DRAW_FRAME::OnToggleCrossHairStyle( wxCommandEvent& aEvent )
{
    INSTALL_UNBUFFERED_DC( dc, m_canvas );
    m_canvas->CrossHairOff( &dc );
    m_cursorShape = !m_cursorShape;
    m_canvas->CrossHairOn( &dc );
}


void EDA_DRAW_FRAME::OnUpdateUndo( wxUpdateUIEvent& aEvent )
{
    if( GetScreen() )
        aEvent.Enable( GetScreen()->GetUndoCommandCount() > 0 );
}


void EDA_DRAW_FRAME::OnUpdateRedo( wxUpdateUIEvent& aEvent )
{
    if( GetScreen() )
        aEvent.Enable( GetScreen()->GetRedoCommandCount() > 0 );
}


void EDA_DRAW_FRAME::OnUpdateUnits( wxUpdateUIEvent& aEvent )
{
    bool enable;

    enable = ( ((aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_MM) && (g_UserUnit == MILLIMETRES))
            || ((aEvent.GetId() == ID_TB_OPTIONS_SELECT_UNIT_INCH) && (g_UserUnit == INCHES)) );

    aEvent.Check( enable );
    DisplayUnitsMsg();
}


void EDA_DRAW_FRAME::OnUpdateGrid( wxUpdateUIEvent& aEvent )
{
    wxString tool_tip = IsGridVisible() ? _( "Hide grid" ) : _( "Show grid" );

    aEvent.Check( IsGridVisible() );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_GRID, tool_tip );
}


void EDA_DRAW_FRAME::OnUpdateCrossHairStyle( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_cursorShape );
}


void EDA_DRAW_FRAME::ReCreateAuxiliaryToolbar()
{
}


void EDA_DRAW_FRAME::ReCreateMenuBar()
{
}


void EDA_DRAW_FRAME::OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition, EDA_ITEM* aItem )
{
}


void EDA_DRAW_FRAME::ToolOnRightClick( wxCommandEvent& event )
{
}


void EDA_DRAW_FRAME::PrintPage( wxDC* aDC,int aPrintMask, bool aPrintMirrorMode, void* aData )
{
    wxMessageBox( wxT("EDA_DRAW_FRAME::PrintPage() error") );
}


void EDA_DRAW_FRAME::OnSelectGrid( wxCommandEvent& event )
{
    int* clientData;
    int  id = ID_POPUP_GRID_LEVEL_100;

    if( event.GetEventType() == wxEVT_COMMAND_COMBOBOX_SELECTED )
    {
        if( m_gridSelectBox == NULL )
            return;

        /*
         * Don't use wxCommandEvent::GetClientData() here.  It always
         * returns NULL in GTK.  This solution is not as elegant but
         * it works.
         */
        int index = m_gridSelectBox->GetSelection();
        wxASSERT( index != wxNOT_FOUND );
        clientData = (int*) m_gridSelectBox->wxItemContainer::GetClientData( index );

        if( clientData != NULL )
            id = *clientData;
    }
    else
    {
        id = event.GetId();

        /* Update the grid select combobox if the grid size was changed
         * by menu event.
         */
        if( m_gridSelectBox != NULL )
        {
            for( size_t i = 0; i < m_gridSelectBox->GetCount(); i++ )
            {
                clientData = (int*) m_gridSelectBox->wxItemContainer::GetClientData( i );

                if( clientData && id == *clientData )
                {
                    m_gridSelectBox->SetSelection( i );
                    break;
                }
            }
        }
    }

    BASE_SCREEN* screen = GetScreen();

    if( screen->GetGridId() == id )
        return;

    /*
     * This allows for saving non-sequential command ID offsets used that
     * may be used in the grid size combobox.  Do not use the selection
     * index returned by GetSelection().
     */
    m_LastGridSizeId = id - ID_POPUP_GRID_LEVEL_1000;
    screen->SetGrid( id );
    screen->SetCrossHairPosition( screen->RefPos( true ) );
    Refresh();
}


void EDA_DRAW_FRAME::OnSelectZoom( wxCommandEvent& event )
{
    if( m_zoomSelectBox == NULL )
        return;                        // Should not happen!

    int id = m_zoomSelectBox->GetCurrentSelection();

    if( id < 0 || !( id < (int)m_zoomSelectBox->GetCount() ) )
        return;

    if( id == 0 )                      // Auto zoom (Fit in Page)
    {
        Zoom_Automatique( true );
    }
    else
    {
        id--;
        int selectedZoom = GetScreen()->m_ZoomList[id];

        if( GetScreen()->GetZoom() == selectedZoom )
            return;

        GetScreen()->SetZoom( selectedZoom );
        RedrawScreen( GetScreen()->GetScrollCenterPosition(), false );
    }
}


double EDA_DRAW_FRAME::GetZoom( void )
{
    return GetScreen()->GetZoom();
}


void EDA_DRAW_FRAME::OnMouseEvent( wxMouseEvent& event )
{
    event.Skip();
}


void EDA_DRAW_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
{
}


void EDA_DRAW_FRAME::DisplayToolMsg( const wxString& msg )
{
    SetStatusText( msg, 5 );
}


void EDA_DRAW_FRAME::DisplayUnitsMsg()
{
    wxString msg;

    switch( g_UserUnit )
    {
    case INCHES:
        msg = _( "Inches" );
        break;

    case MILLIMETRES:
        msg += _( "mm" );
        break;

    default:
        msg += _( "Units" );
        break;
    }

    SetStatusText( msg, 4 );
}



void EDA_DRAW_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    m_FrameSize = GetClientSize( );

    SizeEv.Skip();
}


void EDA_DRAW_FRAME::SetToolID( int aId, int aCursor, const wxString& aToolMsg )
{
    // Keep default cursor in toolbars
    SetCursor( wxNullCursor );

    // Change m_canvas cursor if requested.
    if( m_canvas && aCursor >= 0 )
        m_canvas->SetCurrentCursor( aCursor );

    DisplayToolMsg( aToolMsg );

    if( aId < 0 )
        return;

    wxCHECK2_MSG( aId >= ID_NO_TOOL_SELECTED, aId = ID_NO_TOOL_SELECTED,
                  wxString::Format( wxT( "Current tool ID cannot be set to %d." ), aId ) );

    m_toolId = aId;
}


void EDA_DRAW_FRAME::OnGrid( int grid_type )
{
}


wxPoint EDA_DRAW_FRAME::GetGridPosition( const wxPoint& aPosition )
{
    wxPoint pos = aPosition;

    if( m_currentScreen != NULL && m_snapToGrid )
        pos = m_currentScreen->GetNearestGridPosition( aPosition );

    return pos;
}


int EDA_DRAW_FRAME::ReturnBlockCommand( int key )
{
    return 0;
}


void EDA_DRAW_FRAME::InitBlockPasteInfos()
{
    GetScreen()->m_BlockLocate.ClearItemsList();
    m_canvas->SetMouseCaptureCallback( NULL );
}


void EDA_DRAW_FRAME::HandleBlockPlace( wxDC* DC )
{
}


bool EDA_DRAW_FRAME::HandleBlockEnd( wxDC* DC )
{
    return false;
}


void EDA_DRAW_FRAME::AdjustScrollBars( const wxPoint& aCenterPosition )
{
    int     unitsX, unitsY, posX, posY;
    wxSize  clientSize, logicalClientSize, virtualSize;
    BASE_SCREEN* screen = GetScreen();
    bool noRefresh = true;

    if( screen == NULL || m_canvas == NULL )
        return;

    double scalar = screen->GetScalingFactor();

    wxLogTrace( traceScrollSettings, wxT( "Center Position = ( %d, %d ), scalar = %0.5f." ),
                aCenterPosition.x, aCenterPosition.y, scalar );

    // Calculate the portion of the drawing that can be displayed in the
    // client area at the current zoom level.
    clientSize = m_canvas->GetClientSize();

    // The logical size of the client window.
    logicalClientSize.x = wxRound( (double) clientSize.x / scalar );
    logicalClientSize.y = wxRound( (double) clientSize.y / scalar );

    // A corner of the drawing in internal units.
    wxSize corner = GetPageSizeIU();

    // The drawing rectangle logical units
    wxRect drawingRect( wxPoint( 0, 0 ),  corner );

    wxLogTrace( traceScrollSettings, wxT( "Logical drawing rect = ( %d, %d, %d, %d )." ),
                drawingRect.x, drawingRect.y, drawingRect.width, drawingRect.height );
    wxLogTrace( traceScrollSettings, wxT( "   left %d, right %d, top %d, bottome %d" ),
                drawingRect.GetLeft(), drawingRect.GetRight(),
                drawingRect.GetTop(), drawingRect.GetBottom() );

    // The size of the client rectangle in logical units.
    int x = wxRound( (double) aCenterPosition.x - ( (double) logicalClientSize.x / 2.0 ) );
    int y = wxRound( (double) aCenterPosition.y - ( (double) logicalClientSize.y / 2.0 ) );

    // If drawn around the center, adjust the client rectangle accordingly.
    if( screen->m_Center )
    {
        x += wxRound( (double) drawingRect.width / 2.0 );
        y += wxRound( (double) drawingRect.height / 2.0 );
    }

    wxRect logicalClientRect( wxPoint( x, y ), logicalClientSize );

    wxLogTrace( traceScrollSettings, wxT( "Logical client rect = ( %d, %d, %d, %d )." ),
                logicalClientRect.x, logicalClientRect.y,
                logicalClientRect.width, logicalClientRect.height );
    wxLogTrace( traceScrollSettings, wxT( "   left %d, right %d, top %d, bottome %d" ),
                logicalClientRect.GetLeft(), logicalClientRect.GetRight(),
                logicalClientRect.GetTop(), logicalClientRect.GetBottom() );

    if( drawingRect == logicalClientRect )
    {
        virtualSize = drawingRect.GetSize();
    }
    else if( drawingRect.Contains( logicalClientRect ) )
    {
        virtualSize = drawingRect.GetSize();
    }
    else
    {
        int drawingCenterX = drawingRect.x + ( drawingRect.width / 2 );
        int clientCenterX = logicalClientRect.x + ( logicalClientRect.width / 2 );
        int drawingCenterY = drawingRect.y + ( drawingRect.height / 2 );
        int clientCenterY = logicalClientRect.y + ( logicalClientRect.height / 2 );

        if( logicalClientRect.width > drawingRect.width )
        {
            if( drawingCenterX > clientCenterX )
                virtualSize.x = ( drawingCenterX - logicalClientRect.GetLeft() ) * 2;
            else if( drawingCenterX < clientCenterX )
                virtualSize.x = ( logicalClientRect.GetRight() - drawingCenterX ) * 2;
            else
                virtualSize.x = logicalClientRect.width;
        }
        else if( logicalClientRect.width < drawingRect.width )
        {
            if( drawingCenterX > clientCenterX )
                virtualSize.x = drawingRect.width +
                                ( (drawingRect.GetLeft() - logicalClientRect.GetLeft() ) * 2 );
            else if( drawingCenterX < clientCenterX )
                virtualSize.x = drawingRect.width +
                                ( (logicalClientRect.GetRight() - drawingRect.GetRight() ) * 2 );
            else
                virtualSize.x = drawingRect.width;
        }
        else
        {
            virtualSize.x = drawingRect.width;
        }

        if( logicalClientRect.height > drawingRect.height )
        {
            if( drawingCenterY > clientCenterY )
                virtualSize.y = ( drawingCenterY - logicalClientRect.GetTop() ) * 2;
            else if( drawingCenterY < clientCenterY )
                virtualSize.y = ( logicalClientRect.GetBottom() - drawingCenterY ) * 2;
            else
                virtualSize.y = logicalClientRect.height;
        }
        else if( logicalClientRect.height < drawingRect.height )
        {
            if( drawingCenterY > clientCenterY )
                virtualSize.y = drawingRect.height +
                                ( ( drawingRect.GetTop() - logicalClientRect.GetTop() ) * 2 );
            else if( drawingCenterY < clientCenterY )
                virtualSize.y = drawingRect.height +
                                ( ( logicalClientRect.GetBottom() - drawingRect.GetBottom() ) * 2 );
            else
                virtualSize.y = drawingRect.height;
        }
        else
        {
            virtualSize.y = drawingRect.height;
        }
    }

    if( screen->m_Center )
    {
        screen->m_DrawOrg.x = -( wxRound( (double) virtualSize.x / 2.0 ) );
        screen->m_DrawOrg.y = -( wxRound( (double) virtualSize.y / 2.0 ) );
    }
    else
    {
        screen->m_DrawOrg.x = -( wxRound( (double) (virtualSize.x - drawingRect.width) / 2.0 ) );
        screen->m_DrawOrg.y = -( wxRound( (double) (virtualSize.y - drawingRect.height) / 2.0 ) );
    }

    /* Always set scrollbar pixels per unit to 1 unless you want the zoom
     * around cursor to jump around.  This reported problem occurs when the
     * zoom point is not on a pixel per unit increment.  If you set the
     * pixels per unit to 10, you have potential for the zoom point to
     * jump around +/-5 pixels from the nearest grid point.
     */
    screen->m_ScrollPixelsPerUnitX = screen->m_ScrollPixelsPerUnitY = 1;

    // Calculate the number of scroll bar units for the given zoom level in device units.
    unitsX = wxRound( (double) virtualSize.x * scalar );
    unitsY = wxRound( (double) virtualSize.y * scalar );

    // Calculate the scroll bar position in logical units to place the center position at
    // the center of client rectangle.
    screen->SetScrollCenterPosition( aCenterPosition );
    posX = aCenterPosition.x - wxRound( (double) logicalClientRect.width / 2.0 ) -
           screen->m_DrawOrg.x;
    posY = aCenterPosition.y - wxRound( (double) logicalClientRect.height / 2.0 ) -
           screen->m_DrawOrg.y;

    // Convert scroll bar position to device units.
    posX = wxRound( (double) posX * scalar );
    posY = wxRound( (double) posY * scalar );

    if( posX < 0 )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar X position %d" ), posX );
        posX = 0;
    }

    if( posX > unitsX )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar X position %d" ), posX );
        posX = unitsX;
    }

    if( posY < 0 )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar Y position %d" ), posY );
        posY = 0;
    }

    if( posY > unitsY )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar Y position %d" ), posY );
        posY = unitsY;
    }

    screen->m_ScrollbarPos = wxPoint( posX, posY );
    screen->m_ScrollbarNumber = wxSize( unitsX, unitsY );

    wxLogTrace( traceScrollSettings,
                wxT( "Drawing = (%d, %d), Client = (%d, %d), Offset = (%d, %d), \
SetScrollbars(%d, %d, %d, %d, %d, %d)" ),
                virtualSize.x, virtualSize.y, logicalClientSize.x, logicalClientSize.y,
                screen->m_DrawOrg.x, screen->m_DrawOrg.y,
                screen->m_ScrollPixelsPerUnitX, screen->m_ScrollPixelsPerUnitY,
                screen->m_ScrollbarNumber.x, screen->m_ScrollbarNumber.y,
                screen->m_ScrollbarPos.x, screen->m_ScrollbarPos.y );

    m_canvas->SetScrollbars( screen->m_ScrollPixelsPerUnitX,
                             screen->m_ScrollPixelsPerUnitY,
                             screen->m_ScrollbarNumber.x,
                             screen->m_ScrollbarNumber.y,
                             screen->m_ScrollbarPos.x,
                             screen->m_ScrollbarPos.y, noRefresh );
}


void EDA_DRAW_FRAME::SetLanguage( wxCommandEvent& event )
{
    EDA_BASE_FRAME::SetLanguage( event );
}


/**
 * Round to the nearest precision.
 *
 * Try to approximate a coordinate using a given precision to prevent
 * rounding errors when converting from inches to mm.
 *
 * ie round the unit value to 0 if unit is 1 or 2, or 8 or 9
 */
double RoundTo0( double x, double precision )
{
    assert( precision != 0 );

    long long ix = wxRound( x * precision );

    if ( x < 0.0 )
        NEGATE( ix );

    int remainder = ix % 10;   // remainder is in precision mm

    if ( remainder <= 2 )
        ix -= remainder;       // truncate to the near number
    else if (remainder >= 8 )
        ix += 10 - remainder;  // round to near number

    if ( x < 0 )
        NEGATE( ix );

    return (double) ix / precision;
}


void EDA_DRAW_FRAME::UpdateStatusBar()
{
    wxString        Line;
    int             dx, dy;
    BASE_SCREEN*    screen = GetScreen();

    if( !screen )
        return;

    // Display Zoom level: zoom = zoom_coeff/ZoomScalar
    Line.Printf( wxT( "Z %g" ), screen->GetZoom() );

    SetStatusText( Line, 1 );

    // Display absolute coordinates:
    double dXpos = To_User_Unit( g_UserUnit, screen->GetCrossHairPosition().x, m_internalUnits );
    double dYpos = To_User_Unit( g_UserUnit, screen->GetCrossHairPosition().y, m_internalUnits );

    /*
     * Converting from inches to mm can give some coordinates due to
     * float point precision rounding errors, like 1.999 or 2.001 so
     * round to the nearest drawing precision required by the application.
     */
    if ( g_UserUnit == MILLIMETRES )
    {
        dXpos = RoundTo0( dXpos, (double)( m_internalUnits / 10 ) );
        dYpos = RoundTo0( dYpos, (double)( m_internalUnits / 10 ) );
    }

    // The following sadly is an if Eeschema/if Pcbnew
    wxString absformatter;
    wxString locformatter;
    switch( g_UserUnit )
    {
    case INCHES:
        if( m_internalUnits == EESCHEMA_INTERNAL_UNIT )
        {
            absformatter = wxT( "X %.3f  Y %.3f" );
            locformatter = wxT( "dx %.3f  dy %.3f" );
        }
        else
        {
            absformatter = wxT( "X %.4f  Y %.4f" );
            locformatter = wxT( "dx %.4f  dy %.4f" );
        }
        break;

    case MILLIMETRES:
        if( m_internalUnits == EESCHEMA_INTERNAL_UNIT )
        {
            absformatter = wxT( "X %.2f  Y %.2f" );
            locformatter = wxT( "dx %.2f  dy %.2f" );
        }
        else
        {
            absformatter = wxT( "X %.3f  Y %.3f" );
            locformatter = wxT( "dx %.3f  dy %.3f" );
        }
        break;

    case UNSCALED_UNITS:
        absformatter = wxT( "X %f  Y %f" );
        locformatter = wxT( "dx %f  dy %f" );
        break;
    }

    Line.Printf( absformatter, dXpos, dYpos );
    SetStatusText( Line, 2 );

    // Display relative coordinates:
    dx = screen->GetCrossHairPosition().x - screen->m_O_Curseur.x;
    dy = screen->GetCrossHairPosition().y - screen->m_O_Curseur.y;
    dXpos = To_User_Unit( g_UserUnit, dx, m_internalUnits );
    dYpos = To_User_Unit( g_UserUnit, dy, m_internalUnits );

    if( g_UserUnit == MILLIMETRES )
    {
        dXpos = RoundTo0( dXpos, (double) ( m_internalUnits / 10 ) );
        dYpos = RoundTo0( dYpos, (double) ( m_internalUnits / 10 ) );
    }

    // We already decided the formatter above
    Line.Printf( locformatter, dXpos, dYpos );
    SetStatusText( Line, 3 );
}


void EDA_DRAW_FRAME::LoadSettings()
{
    wxASSERT( wxGetApp().GetSettings() != NULL );

    wxConfig* cfg = wxGetApp().GetSettings();

    EDA_BASE_FRAME::LoadSettings();
    cfg->Read( m_FrameName + CursorShapeEntryKeyword, &m_cursorShape, ( long )0 );
    bool btmp;

    if ( cfg->Read( m_FrameName + ShowGridEntryKeyword, &btmp ) )
        SetGridVisibility( btmp );

    int itmp;

    if( cfg->Read( m_FrameName + GridColorEntryKeyword, &itmp ) )
        SetGridColor( itmp );

    cfg->Read( m_FrameName + LastGridSizeId, &m_LastGridSizeId, 0L );
}


void EDA_DRAW_FRAME::SaveSettings()
{
    wxASSERT( wxGetApp().GetSettings() != NULL );

    wxConfig* cfg = wxGetApp().GetSettings();

    EDA_BASE_FRAME::SaveSettings();
    cfg->Write( m_FrameName + CursorShapeEntryKeyword, m_cursorShape );
    cfg->Write( m_FrameName + ShowGridEntryKeyword, IsGridVisible() );
    cfg->Write( m_FrameName + GridColorEntryKeyword, GetGridColor() );
    cfg->Write( m_FrameName + LastGridSizeId, ( long ) m_LastGridSizeId );
}


void EDA_DRAW_FRAME::AppendMsgPanel( const wxString& textUpper,
                                     const wxString& textLower,
                                     int color, int pad )
{
    if( m_messagePanel == NULL )
        return;

    m_messagePanel->AppendMessage( textUpper, textLower, color, pad );
}


void EDA_DRAW_FRAME::ClearMsgPanel( void )
{
    if( m_messagePanel == NULL )
        return;

    m_messagePanel->EraseMsgBox();
}


wxString EDA_DRAW_FRAME::CoordinateToString( int aValue, bool aConvertToMils )
{
    return ::CoordinateToString( aValue, m_internalUnits, aConvertToMils );
}

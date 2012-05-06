/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
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
#include <base_units.h>
#include <vector2d.h>

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
    minsize.x             = 470;
    minsize.y             = 350 + m_MsgFrameHeight;

    // Pane sizes for status bar.
    // @todo these should be sized based on typical text content, like
    // "dx -10.123 -10.123 dy -10.123 -10.123" using the system font which is
    // in play on a particular platform, and should not be constants.
    // Please do not reduce these constant values, and please use dynamic
    // system font specific sizing in the future.
    #define ZOOM_DISPLAY_SIZE       60
    #define COORD_DISPLAY_SIZE      165
    #define DELTA_DISPLAY_SIZE      245
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
        double selectedZoom = GetScreen()->m_ZoomList[id];

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


void EDA_DRAW_FRAME::SetLanguage( wxCommandEvent& event )
{
    EDA_BASE_FRAME::SetLanguage( event );
}


void EDA_DRAW_FRAME::UpdateStatusBar()
{
    wxString        Line;
    BASE_SCREEN*    screen = GetScreen();

    if( !screen )
        return;

    // Display Zoom level: zoom = zoom_coeff/ZoomScalar
    Line.Printf( wxT( "Z %g" ), screen->GetZoom() );

    SetStatusText( Line, 1 );

    // Absolute and relative cursor positions are handled by overloading this function and
    // handling the internal to user units conversion at the appropriate level.

    // refresh units display
    DisplayUnitsMsg();
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
    return ::CoordinateToString( aValue, aConvertToMils );
}

wxString EDA_DRAW_FRAME::LengthDoubleToString( double aValue, bool aConvertToMils )
{
    return ::LengthDoubleToString( aValue, aConvertToMils );
}


bool EDA_DRAW_FRAME::HandleBlockBegin( wxDC* aDC, int aKey, const wxPoint& aPosition )
{
    BLOCK_SELECTOR* Block = &GetScreen()->m_BlockLocate;

    if( ( Block->GetCommand() != BLOCK_IDLE ) || ( Block->GetState() != STATE_NO_BLOCK ) )
        return false;

    Block->SetCommand( (BLOCK_COMMAND_T) ReturnBlockCommand( aKey ) );

    if( Block->GetCommand() == 0 )
        return false;

    switch( Block->GetCommand() )
    {
    case BLOCK_IDLE:
        break;

    case BLOCK_MOVE:                /* Move */
    case BLOCK_DRAG:                /* Drag */
    case BLOCK_COPY:                /* Copy */
    case BLOCK_DELETE:              /* Delete */
    case BLOCK_SAVE:                /* Save */
    case BLOCK_ROTATE:              /* Rotate 90 deg */
    case BLOCK_FLIP:                /* Flip */
    case BLOCK_ZOOM:                /* Window Zoom */
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:            /* mirror */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        Block->InitData( m_canvas, aPosition );
        break;

    case BLOCK_PASTE:
        Block->InitData( m_canvas, aPosition );
        Block->SetLastCursorPosition( wxPoint( 0, 0 ) );
        InitBlockPasteInfos();

        if( Block->GetCount() == 0 )      /* No data to paste */
        {
            DisplayError( this, wxT( "No Block to paste" ), 20 );
            GetScreen()->m_BlockLocate.SetCommand( BLOCK_IDLE );
            m_canvas->SetMouseCaptureCallback( NULL );
            return true;
        }

        if( !m_canvas->IsMouseCaptured() )
        {
            Block->ClearItemsList();
            DisplayError( this,
                          wxT( "EDA_DRAW_FRAME::HandleBlockBegin() Err: m_mouseCaptureCallback NULL" ) );
            return true;
        }

        Block->SetState( STATE_BLOCK_MOVE );
        m_canvas->CallMouseCapture( aDC, aPosition, false );
        break;

    default:
    {
        wxString msg;
        msg << wxT( "EDA_DRAW_FRAME::HandleBlockBegin() error: Unknown command " ) <<
        Block->GetCommand();
        DisplayError( this, msg );
    }
    break;
    }

    Block->SetMessageBlock( this );
    return true;
}

#define SAFETY_MARGIN   100

// see comment in classpcb.cpp near line 66
static const double MAX_AXIS = 1518500251 - SAFETY_MARGIN;

#define VIRT_MIN    (-MAX_AXIS/2.0)     ///< min X or Y coordinate in virtual space
#define VIRT_MAX    (MAX_AXIS/2.0)      ///< max X or Y coordinate in virtual space


void EDA_DRAW_FRAME::AdjustScrollBars( const wxPoint& aCenterPositionIU )
{
    BASE_SCREEN* screen = GetScreen();

    if( screen == NULL || m_canvas == NULL )
        return;

    // There are no safety limits on these calculations, so in NANOMETRES build it
    // still blows up.  This is incomplete work.

    double scale = screen->GetScalingFactor();

    wxLogTrace( traceScrollSettings, wxT( "Center Position = ( %d, %d ), scale = %.16g" ),
                aCenterPositionIU.x, aCenterPositionIU.y, scale );

    // Calculate the portion of the drawing that can be displayed in the
    // client area at the current zoom level.

    // visible viewport in device units ~ pixels
    wxSize  clientSizeDU = m_canvas->GetClientSize();

    // Size of the client window in IU
    DSIZE   clientSizeIU( clientSizeDU.x / scale, clientSizeDU.y / scale );

    // Full drawing or "page" rectangle in internal units
    DBOX    pageRectIU( 0, 0, GetPageSizeIU().x, GetPageSizeIU().y );

    // The upper left corner of the client rectangle in internal units.
    double xIU = aCenterPositionIU.x - clientSizeIU.x / 2.0;
    double yIU = aCenterPositionIU.y - clientSizeIU.y / 2.0;

    // If drawn around the center, adjust the client rectangle accordingly.
    if( screen->m_Center )
    {
        // half page offset.
        xIU += pageRectIU.width  / 2.0;
        yIU += pageRectIU.height / 2.0;
    }

    DBOX    clientRectIU( xIU, yIU, clientSizeIU.x, clientSizeIU.y );
    wxPoint centerPositionIU;

#if 1 || defined( USE_PCBNEW_NANOMETRES )
    // put "int" limits on the clientRect
    if( clientRectIU.GetLeft() < VIRT_MIN )
        clientRectIU.MoveLeftTo( VIRT_MIN );
    if( clientRectIU.GetTop() < VIRT_MIN )
        clientRectIU.MoveTopTo( VIRT_MIN );
    if( clientRectIU.GetRight() > VIRT_MAX )
        clientRectIU.MoveRightTo( VIRT_MAX );
    if( clientRectIU.GetBottom() > VIRT_MAX )
        clientRectIU.MoveBottomTo( VIRT_MAX );
#endif

    centerPositionIU.x = KiROUND( clientRectIU.x + clientRectIU.width/2 );
    centerPositionIU.y = KiROUND( clientRectIU.y + clientRectIU.height/2 );

    DSIZE   virtualSizeIU;

    if( pageRectIU.GetLeft() < clientRectIU.GetLeft() && pageRectIU.GetRight() > clientRectIU.GetRight() )
    {
        virtualSizeIU.x = pageRectIU.GetSize().x;
    }
    else
    {
        double pageCenterX    = pageRectIU.x   + ( pageRectIU.width / 2 );
        double clientCenterX  = clientRectIU.x + ( clientRectIU.width / 2 );

        if( clientRectIU.width > pageRectIU.width )
        {
            if( pageCenterX > clientCenterX )
                virtualSizeIU.x = ( pageCenterX - clientRectIU.GetLeft() ) * 2;
            else if( pageCenterX < clientCenterX )
                virtualSizeIU.x = ( clientRectIU.GetRight() - pageCenterX ) * 2;
            else
                virtualSizeIU.x = clientRectIU.width;
        }
        else
        {
            if( pageCenterX > clientCenterX )
                virtualSizeIU.x = pageRectIU.width + ( (pageRectIU.GetLeft() - clientRectIU.GetLeft() ) * 2 );
            else if( pageCenterX < clientCenterX )
                virtualSizeIU.x = pageRectIU.width + ( (clientRectIU.GetRight() - pageRectIU.GetRight() ) * 2 );
            else
                virtualSizeIU.x = pageRectIU.width;
        }
    }

    if( pageRectIU.GetTop() < clientRectIU.GetTop() && pageRectIU.GetBottom() > clientRectIU.GetBottom() )
    {
        virtualSizeIU.y = pageRectIU.GetSize().y;
    }
    else
    {
        double pageCenterY   = pageRectIU.y   + ( pageRectIU.height / 2 );
        double clientCenterY = clientRectIU.y + ( clientRectIU.height / 2 );

        if( clientRectIU.height > pageRectIU.height )
        {
            if( pageCenterY > clientCenterY )
                virtualSizeIU.y = ( pageCenterY - clientRectIU.GetTop() ) * 2;
            else if( pageCenterY < clientCenterY )
                virtualSizeIU.y = ( clientRectIU.GetBottom() - pageCenterY ) * 2;
            else
                virtualSizeIU.y = clientRectIU.height;
        }
        else
        {
            if( pageCenterY > clientCenterY )
                virtualSizeIU.y = pageRectIU.height +
                                ( ( pageRectIU.GetTop() - clientRectIU.GetTop() ) * 2 );
            else if( pageCenterY < clientCenterY )
                virtualSizeIU.y = pageRectIU.height +
                                ( ( clientRectIU.GetBottom() - pageRectIU.GetBottom() ) * 2 );
            else
                virtualSizeIU.y = pageRectIU.height;
        }
    }

#if 1 || defined( USE_PCBNEW_NANOMETRES )
    // put "int" limits on the virtualSizeIU
    virtualSizeIU.x = std::min( virtualSizeIU.x, MAX_AXIS );
    virtualSizeIU.y = std::min( virtualSizeIU.y, MAX_AXIS );
#endif

    if( screen->m_Center )
    {
        screen->m_DrawOrg.x = -KiROUND( virtualSizeIU.x / 2.0 );
        screen->m_DrawOrg.y = -KiROUND( virtualSizeIU.y / 2.0 );
    }
    else
    {
        screen->m_DrawOrg.x = -KiROUND( ( virtualSizeIU.x - pageRectIU.width )  / 2.0 );
        screen->m_DrawOrg.y = -KiROUND( ( virtualSizeIU.y - pageRectIU.height ) / 2.0 );
    }

    /* Always set scrollbar pixels per unit to 1 unless you want the zoom
     * around cursor to jump around.  This reported problem occurs when the
     * zoom point is not on a pixel per unit increment.  If you set the
     * pixels per unit to 10, you have potential for the zoom point to
     * jump around +/-5 pixels from the nearest grid point.
     */
    screen->m_ScrollPixelsPerUnitX = screen->m_ScrollPixelsPerUnitY = 1;

    // Number of scroll bar units for the given zoom level in device units.
    double unitsX = virtualSizeIU.x * scale;
    double unitsY = virtualSizeIU.y * scale;

    // Calculate the scroll bar position in internal units to place the
    // center position at the center of client rectangle.
    screen->SetScrollCenterPosition( centerPositionIU );

    double posX = centerPositionIU.x - clientRectIU.width /2.0 - screen->m_DrawOrg.x;
    double posY = centerPositionIU.y - clientRectIU.height/2.0 - screen->m_DrawOrg.y;

    // Convert scroll bar position to device units.
    posX = KiROUND( posX * scale );
    posY = KiROUND( posY * scale );

    if( posX < 0 )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar X position %.16g" ), posX );
        posX = 0;
    }

    if( posX > unitsX )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar X position %.16g" ), posX );
        posX = unitsX;
    }

    if( posY < 0 )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar Y position %.16g" ), posY );
        posY = 0;
    }

    if( posY > unitsY )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar Y position %.16g" ), posY );
        posY = unitsY;
    }

    screen->m_ScrollbarPos    = wxPoint( KiROUND( posX ),  KiROUND( posY ) );
    screen->m_ScrollbarNumber = wxSize( KiROUND( unitsX ), KiROUND( unitsY ) );

    wxLogTrace( traceScrollSettings,
                wxT( "Drawing = (%.16g, %.16g), Client = (%.16g, %.16g), Offset = (%d, %d), SetScrollbars(%d, %d, %d, %d, %d, %d)" ),
                virtualSizeIU.x, virtualSizeIU.y, clientSizeIU.x, clientSizeIU.y,
                screen->m_DrawOrg.x, screen->m_DrawOrg.y,
                screen->m_ScrollPixelsPerUnitX, screen->m_ScrollPixelsPerUnitY,
                screen->m_ScrollbarNumber.x, screen->m_ScrollbarNumber.y,
                screen->m_ScrollbarPos.x, screen->m_ScrollbarPos.y );

    bool            noRefresh = true;

    m_canvas->SetScrollbars( screen->m_ScrollPixelsPerUnitX,
                             screen->m_ScrollPixelsPerUnitY,
                             screen->m_ScrollbarNumber.x,
                             screen->m_ScrollbarNumber.y,
                             screen->m_ScrollbarPos.x,
                             screen->m_ScrollbarPos.y, noRefresh );
}

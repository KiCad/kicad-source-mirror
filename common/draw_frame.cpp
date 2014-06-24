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
#include <pgm_base.h>
#include <kiface_i.h>
#include <gr_basic.h>
#include <common.h>
#include <bitmaps.h>
#include <macros.h>
#include <id.h>
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>
#include <class_base_screen.h>
#include <msgpanel.h>
#include <draw_frame.h>
#include <confirm.h>
#include <kicad_device_context.h>
#include <dialog_helpers.h>
#include <base_units.h>
#include <math/box2.h>

#include <wx/fontdlg.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>

/**
 * Definition for enabling and disabling scroll bar setting trace output.  See the
 * wxWidgets documentation on useing the WXTRACE environment variable.
 */
static const wxString traceScrollSettings( wxT( "KicadScrollSettings" ) );


// Configuration entry names.
static const wxString CursorShapeEntryKeyword( wxT( "CursorShape" ) );
static const wxString ShowGridEntryKeyword( wxT( "ShowGrid" ) );
static const wxString GridColorEntryKeyword( wxT( "GridColor" ) );
static const wxString LastGridSizeIdKeyword( wxT( "_LastGridSize" ) );


BEGIN_EVENT_TABLE( EDA_DRAW_FRAME, KIWAY_PLAYER )
    EVT_MOUSEWHEEL( EDA_DRAW_FRAME::OnMouseEvent )
    EVT_MENU_OPEN( EDA_DRAW_FRAME::OnMenuOpen )
    EVT_ACTIVATE( EDA_DRAW_FRAME::OnActivate )
    EVT_MENU_RANGE( ID_ZOOM_IN, ID_ZOOM_REDRAW, EDA_DRAW_FRAME::OnZoom )
    EVT_MENU_RANGE( ID_OFFCENTER_ZOOM_IN, ID_OFFCENTER_ZOOM_OUT, EDA_DRAW_FRAME::OnZoom )
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


EDA_DRAW_FRAME::EDA_DRAW_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                FRAME_T aFrameType,
                                const wxString& aTitle,
                                const wxPoint& aPos, const wxSize& aSize,
                                long aStyle, const wxString & aFrameName ) :
    KIWAY_PLAYER( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName )
{
    m_drawToolBar         = NULL;
    m_optionsToolBar      = NULL;
    m_gridSelectBox       = NULL;
    m_zoomSelectBox       = NULL;
    m_HotkeysZoomAndGridList = NULL;

    m_canvas              = NULL;
    m_galCanvas           = NULL;
    m_galCanvasActive     = false;
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
    m_MsgFrameHeight      = EDA_MSG_PANEL::GetRequiredHeight();

    m_auimgr.SetFlags(wxAUI_MGR_DEFAULT|wxAUI_MGR_LIVE_RESIZE);

    CreateStatusBar( 6 );

    // set the size of the status bar subwindows:

    wxWindow* stsbar = GetStatusBar();

    int dims[] = {

        // remainder of status bar on far left is set to a default or whatever is left over.
        -1,

        // When using GetTextSize() remember the width of character '1' is not the same
        // as the width of '0' unless the font is fixed width, and it usually won't be.

        // zoom:
        GetTextSize( wxT( "Z 762000" ), stsbar ).x + 10,

        // cursor coords
        GetTextSize( wxT( "X 0234.567890  Y 0234.567890" ), stsbar ).x + 10,

        // delta distances
        GetTextSize( wxT( "dx 0234.567890  dx 0234.567890  d 0234.567890" ), stsbar ).x + 10,

        // units display, Inches is bigger than mm
        GetTextSize( _( "Inches" ), stsbar ).x + 10,

        // Size for the panel used as "Current tool in play": will take longest string from
        // void PCB_EDIT_FRAME::OnSelectTool( wxCommandEvent& aEvent ) in pcbnew/edit.cpp
        GetTextSize( wxT( "Add layer alignment target" ), stsbar ).x + 10,
    };

    SetStatusWidths( DIM( dims ), dims );

    // Create child subwindows.
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y );
    m_FramePos.x   = m_FramePos.y = 0;
    m_FrameSize.y -= m_MsgFrameHeight;

    m_canvas = new EDA_DRAW_PANEL( this, -1, wxPoint( 0, 0 ), m_FrameSize );
    m_messagePanel  = new EDA_MSG_PANEL( this, -1, wxPoint( 0, m_FrameSize.y ),
                                         wxSize( m_FrameSize.x, m_MsgFrameHeight ) );

    m_messagePanel->SetBackgroundColour( MakeColour( LIGHTGRAY ) );
}


EDA_DRAW_FRAME::~EDA_DRAW_FRAME()
{
    delete m_currentScreen;
    m_currentScreen = NULL;

    m_auimgr.UnInit();
}


void EDA_DRAW_FRAME::unitsChangeRefresh()
{
    UpdateStatusBar();

    EDA_ITEM* item = GetScreen()->GetCurItem();

    if( item )
        SetMsgPanel( item );
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

/* function SkipNextLeftButtonReleaseEvent
 * after calling this function, if the left mouse button
 * is down, the next left mouse button release event will be ignored.
 * It is is usefull for instance when closing a dialog on a mouse click,
 * to skip the next mouse left button release event
 * by the parent window, because the mouse button
 * clicked on the dialog is often released in the parent frame,
 * and therefore creates a left button released mouse event
 * which can be unwanted in some cases
 */
void EDA_DRAW_FRAME::SkipNextLeftButtonReleaseEvent()
{
   m_canvas->SetIgnoreLeftButtonReleaseEvent( true );
}

void EDA_DRAW_FRAME::OnToggleGridState( wxCommandEvent& aEvent )
{
    SetGridVisibility( !IsGridVisible() );
    if( IsGalCanvasActive() )
    {
        GetGalCanvas()->GetGAL()->SetGridVisibility( IsGridVisible() );
        GetGalCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    }

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


void EDA_DRAW_FRAME::PrintPage( wxDC* aDC, LSET aPrintMask, bool aPrintMirrorMode, void* aData )
{
    wxMessageBox( wxT("EDA_DRAW_FRAME::PrintPage() error") );
}


void EDA_DRAW_FRAME::OnSelectGrid( wxCommandEvent& event )
{
    int* clientData;
    int  eventId = ID_POPUP_GRID_LEVEL_100;

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
            eventId = *clientData;
    }
    else
    {
        eventId = event.GetId();

        /* Update the grid select combobox if the grid size was changed
         * by menu event.
         */
        if( m_gridSelectBox != NULL )
        {
            for( size_t i = 0; i < m_gridSelectBox->GetCount(); i++ )
            {
                clientData = (int*) m_gridSelectBox->wxItemContainer::GetClientData( i );

                if( clientData && eventId == *clientData )
                {
                    m_gridSelectBox->SetSelection( i );
                    break;
                }
            }
        }
    }

    // Be sure m_LastGridSizeId is up to date.
    m_LastGridSizeId = eventId - ID_POPUP_GRID_LEVEL_1000;

    BASE_SCREEN* screen = GetScreen();

    if( screen->GetGridId() == eventId )
        return;

    /*
     * This allows for saving non-sequential command ID offsets used that
     * may be used in the grid size combobox.  Do not use the selection
     * index returned by GetSelection().
     */
    screen->SetGrid( eventId );
    SetCrossHairPosition( RefPos( true ) );

    if( IsGalCanvasActive() )
    {
        GetGalCanvas()->GetGAL()->SetGridSize( VECTOR2D( screen->GetGrid().m_Size.x,
                                                         screen->GetGrid().m_Size.y ) );
        GetGalCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    }

    m_canvas->Refresh();
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

        if( IsGalCanvasActive() )
        {
            // Apply computed view settings to GAL
            KIGFX::VIEW* view = GetGalCanvas()->GetView();
            KIGFX::GAL* gal = GetGalCanvas()->GetGAL();

            double zoomFactor = gal->GetWorldScale() / gal->GetZoomFactor();
            double zoom = 1.0 / ( zoomFactor * GetZoom() );

            view->SetScale( zoom );
            GetGalCanvas()->Refresh();
        }
        else
            RedrawScreen( GetScrollCenterPosition(), false );
    }
}


double EDA_DRAW_FRAME::GetZoom()
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
        msg = _( "mm" );
        break;

    default:
        msg = _( "Units" );
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


wxPoint EDA_DRAW_FRAME::GetGridPosition( const wxPoint& aPosition ) const
{
    wxPoint pos = aPosition;

    if( m_currentScreen != NULL && m_snapToGrid )
        pos = GetNearestGridPosition( aPosition );

    return pos;
}


void EDA_DRAW_FRAME::SetNextGrid()
{
    if( m_gridSelectBox )
    {
        m_gridSelectBox->SetSelection( ( m_gridSelectBox->GetSelection() + 1 ) %
                                       m_gridSelectBox->GetCount() );

        wxCommandEvent cmd( wxEVT_COMMAND_COMBOBOX_SELECTED );
        //        cmd.SetEventObject( this );
        OnSelectGrid( cmd );
    }
}


void EDA_DRAW_FRAME::SetPrevGrid()
{
    if( m_gridSelectBox )
    {
        int cnt = m_gridSelectBox->GetSelection();

        if( --cnt < 0 )
            cnt = m_gridSelectBox->GetCount() - 1;

        m_gridSelectBox->SetSelection( cnt );

        wxCommandEvent cmd( wxEVT_COMMAND_COMBOBOX_SELECTED );
        //        cmd.SetEventObject( this );
        OnSelectGrid( cmd );
    }
}


int EDA_DRAW_FRAME::BlockCommand( int key )
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


void EDA_DRAW_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    aCfg->Read( m_FrameName + CursorShapeEntryKeyword, &m_cursorShape, ( long )0 );

    bool btmp;
    if( aCfg->Read( m_FrameName + ShowGridEntryKeyword, &btmp ) )
        SetGridVisibility( btmp );

    int itmp;
    if( aCfg->Read( m_FrameName + GridColorEntryKeyword, &itmp ) )
        SetGridColor( ColorFromInt( itmp ) );

    aCfg->Read( m_FrameName + LastGridSizeIdKeyword, &m_LastGridSizeId, 0L );

    // m_LastGridSizeId is an offset, expected to be >= 0
    if( m_LastGridSizeId < 0 )
        m_LastGridSizeId = 0;
}


void EDA_DRAW_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    aCfg->Write( m_FrameName + CursorShapeEntryKeyword, m_cursorShape );
    aCfg->Write( m_FrameName + ShowGridEntryKeyword, IsGridVisible() );
    aCfg->Write( m_FrameName + GridColorEntryKeyword, ( long ) GetGridColor() );
    aCfg->Write( m_FrameName + LastGridSizeIdKeyword, ( long ) m_LastGridSizeId );
}


void EDA_DRAW_FRAME::AppendMsgPanel( const wxString& textUpper,
                                     const wxString& textLower,
                                     EDA_COLOR_T color, int pad )
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


void EDA_DRAW_FRAME::SetMsgPanel( const MSG_PANEL_ITEMS& aList )
{
    if( m_messagePanel == NULL )
        return;

    ClearMsgPanel();

    for( unsigned i = 0;  i < aList.size();  i++ )
        m_messagePanel->AppendMessage( aList[i] );
}


void EDA_DRAW_FRAME::SetMsgPanel( EDA_ITEM* aItem )
{
    wxCHECK_RET( aItem != NULL, wxT( "Invalid EDA_ITEM pointer.  Bad programmer." ) );

    MSG_PANEL_ITEMS items;
    aItem->GetMsgPanelInfo( items );
    SetMsgPanel( items );
}


wxString EDA_DRAW_FRAME::CoordinateToString( int aValue, bool aConvertToMils ) const
{
    return ::CoordinateToString( aValue, aConvertToMils );
}

wxString EDA_DRAW_FRAME::LengthDoubleToString( double aValue, bool aConvertToMils ) const
{
    return ::LengthDoubleToString( aValue, aConvertToMils );
}


bool EDA_DRAW_FRAME::HandleBlockBegin( wxDC* aDC, int aKey, const wxPoint& aPosition )
{
    BLOCK_SELECTOR* Block = &GetScreen()->m_BlockLocate;

    if( ( Block->GetCommand() != BLOCK_IDLE ) || ( Block->GetState() != STATE_NO_BLOCK ) )
        return false;

    Block->SetCommand( (BLOCK_COMMAND_T) BlockCommand( aKey ) );

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


// See comment in classpcb.cpp near line 66
//static const double MAX_AXIS = 1518500251;

// However I am not seeing a problem with this size yet:
static const double MAX_AXIS = INT_MAX - 100;

#define VIRT_MIN    (-MAX_AXIS/2.0)     ///< min X or Y coordinate in virtual space
#define VIRT_MAX    (MAX_AXIS/2.0)      ///< max X or Y coordinate in virtual space


void EDA_DRAW_FRAME::AdjustScrollBars( const wxPoint& aCenterPositionIU )
{
    BASE_SCREEN* screen = GetScreen();

    if( !screen || !m_canvas )
        return;

    double scale = screen->GetScalingFactor();

    wxLogTrace( traceScrollSettings, wxT( "Center Position = ( %d, %d ), scale = %.10g" ),
                aCenterPositionIU.x, aCenterPositionIU.y, scale );

    // Calculate the portion of the drawing that can be displayed in the
    // client area at the current zoom level.

    // visible viewport in device units ~ pixels
    wxSize  clientSizeDU = m_canvas->GetClientSize();

    // Size of the client window in IU
    DSIZE   clientSizeIU( clientSizeDU.x / scale, clientSizeDU.y / scale );

    // Full drawing or "page" rectangle in internal units
    DBOX    pageRectIU( wxPoint( 0, 0 ), wxSize( GetPageSizeIU().x, GetPageSizeIU().y ) );

    // The upper left corner of the client rectangle in internal units.
    double xIU = aCenterPositionIU.x - clientSizeIU.x / 2.0;
    double yIU = aCenterPositionIU.y - clientSizeIU.y / 2.0;

    // If drawn around the center, adjust the client rectangle accordingly.
    if( screen->m_Center )
    {
        // half page offset.
        xIU += pageRectIU.GetWidth()  / 2.0;
        yIU += pageRectIU.GetHeight() / 2.0;
    }

    DBOX    clientRectIU( wxPoint( xIU, yIU ), wxSize( clientSizeIU.x, clientSizeIU.y ) );
    wxPoint centerPositionIU;

    // put "int" limits on the clientRect
    if( clientRectIU.GetLeft() < VIRT_MIN )
        clientRectIU.MoveLeftTo( VIRT_MIN );
    if( clientRectIU.GetTop() < VIRT_MIN )
        clientRectIU.MoveTopTo( VIRT_MIN );
    if( clientRectIU.GetRight() > VIRT_MAX )
        clientRectIU.MoveRightTo( VIRT_MAX );
    if( clientRectIU.GetBottom() > VIRT_MAX )
        clientRectIU.MoveBottomTo( VIRT_MAX );

    centerPositionIU.x = KiROUND( clientRectIU.GetX() + clientRectIU.GetWidth() / 2 );
    centerPositionIU.y = KiROUND( clientRectIU.GetY() + clientRectIU.GetHeight() / 2 );

    if( screen->m_Center )
    {
        centerPositionIU.x -= KiROUND( pageRectIU.GetWidth() / 2.0 );
        centerPositionIU.y -= KiROUND( pageRectIU.GetHeight() / 2.0 );
    }

    DSIZE   virtualSizeIU;

    if( pageRectIU.GetLeft() < clientRectIU.GetLeft() && pageRectIU.GetRight() > clientRectIU.GetRight() )
    {
        virtualSizeIU.x = pageRectIU.GetSize().x;
    }
    else
    {
        double pageCenterX    = pageRectIU.GetX()   + ( pageRectIU.GetWidth() / 2 );
        double clientCenterX  = clientRectIU.GetX() + ( clientRectIU.GetWidth() / 2 );

        if( clientRectIU.GetWidth() > pageRectIU.GetWidth() )
        {
            if( pageCenterX > clientCenterX )
                virtualSizeIU.x = ( pageCenterX - clientRectIU.GetLeft() ) * 2;
            else if( pageCenterX < clientCenterX )
                virtualSizeIU.x = ( clientRectIU.GetRight() - pageCenterX ) * 2;
            else
                virtualSizeIU.x = clientRectIU.GetWidth();
        }
        else
        {
            if( pageCenterX > clientCenterX )
                virtualSizeIU.x = pageRectIU.GetWidth() + ( (pageRectIU.GetLeft() - clientRectIU.GetLeft() ) * 2 );
            else if( pageCenterX < clientCenterX )
                virtualSizeIU.x = pageRectIU.GetWidth() + ( (clientRectIU.GetRight() - pageRectIU.GetRight() ) * 2 );
            else
                virtualSizeIU.x = pageRectIU.GetWidth();
        }
    }

    if( pageRectIU.GetTop() < clientRectIU.GetTop() && pageRectIU.GetBottom() > clientRectIU.GetBottom() )
    {
        virtualSizeIU.y = pageRectIU.GetSize().y;
    }
    else
    {
        double pageCenterY   = pageRectIU.GetY()   + ( pageRectIU.GetHeight() / 2 );
        double clientCenterY = clientRectIU.GetY() + ( clientRectIU.GetHeight() / 2 );

        if( clientRectIU.GetHeight() > pageRectIU.GetHeight() )
        {
            if( pageCenterY > clientCenterY )
                virtualSizeIU.y = ( pageCenterY - clientRectIU.GetTop() ) * 2;
            else if( pageCenterY < clientCenterY )
                virtualSizeIU.y = ( clientRectIU.GetBottom() - pageCenterY ) * 2;
            else
                virtualSizeIU.y = clientRectIU.GetHeight();
        }
        else
        {
            if( pageCenterY > clientCenterY )
                virtualSizeIU.y = pageRectIU.GetHeight() +
                                ( ( pageRectIU.GetTop() - clientRectIU.GetTop() ) * 2 );
            else if( pageCenterY < clientCenterY )
                virtualSizeIU.y = pageRectIU.GetHeight() +
                                ( ( clientRectIU.GetBottom() - pageRectIU.GetBottom() ) * 2 );
            else
                virtualSizeIU.y = pageRectIU.GetHeight();
        }
    }

    // put "int" limits on the virtualSizeIU
    virtualSizeIU.x = std::min( virtualSizeIU.x, MAX_AXIS );
    virtualSizeIU.y = std::min( virtualSizeIU.y, MAX_AXIS );

    if( screen->m_Center )
    {
        screen->m_DrawOrg.x = -KiROUND( virtualSizeIU.x / 2.0 );
        screen->m_DrawOrg.y = -KiROUND( virtualSizeIU.y / 2.0 );
    }
    else
    {
        screen->m_DrawOrg.x = -KiROUND( ( virtualSizeIU.x - pageRectIU.GetWidth() )  / 2.0 );
        screen->m_DrawOrg.y = -KiROUND( ( virtualSizeIU.y - pageRectIU.GetHeight() ) / 2.0 );
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
    SetScrollCenterPosition( centerPositionIU );

    double posX = centerPositionIU.x - clientRectIU.GetWidth()  / 2.0 - screen->m_DrawOrg.x;
    double posY = centerPositionIU.y - clientRectIU.GetHeight() / 2.0 - screen->m_DrawOrg.y;

    // Convert scroll bar position to device units.
    posX = KiROUND( posX * scale );
    posY = KiROUND( posY * scale );

    if( posX < 0 )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar X position %.10g" ), posX );
        posX = 0;
    }

    if( posX > unitsX )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar X position %.10g" ), posX );
        posX = unitsX;
    }

    if( posY < 0 )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar Y position %.10g" ), posY );
        posY = 0;
    }

    if( posY > unitsY )
    {
        wxLogTrace( traceScrollSettings, wxT( "Required scroll bar Y position %.10g" ), posY );
        posY = unitsY;
    }

    screen->m_ScrollbarPos    = wxPoint( KiROUND( posX ),  KiROUND( posY ) );
    screen->m_ScrollbarNumber = wxSize( KiROUND( unitsX ), KiROUND( unitsY ) );

    wxLogTrace( traceScrollSettings,
                wxT( "Drawing = (%.10g, %.10g), Client = (%.10g, %.10g), Offset = (%d, %d), SetScrollbars(%d, %d, %d, %d, %d, %d)" ),
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


void EDA_DRAW_FRAME::UseGalCanvas( bool aEnable )
{
    KIGFX::VIEW* view = GetGalCanvas()->GetView();
    KIGFX::GAL* gal = GetGalCanvas()->GetGAL();

    double zoomFactor = gal->GetWorldScale() / gal->GetZoomFactor();

    // Display the same view after canvas switching
    if( aEnable )
    {
        BASE_SCREEN* screen = GetScreen();

        // Switch to GAL rendering
        if( !IsGalCanvasActive() )
        {
            // Set up viewport
            double zoom = 1.0 / ( zoomFactor * m_canvas->GetZoom() );
            view->SetScale( zoom );
            view->SetCenter( VECTOR2D( m_canvas->GetScreenCenterLogicalPosition() ) );
        }

        // Set up grid settings
        gal->SetGridVisibility( IsGridVisible() );
        gal->SetGridSize( VECTOR2D( screen->GetGridSize().x, screen->GetGridSize().y ) );
        gal->SetGridOrigin( VECTOR2D( GetGridOrigin() ) );
    }
    else
    {
        // Switch to standard rendering
        if( IsGalCanvasActive() )
        {
            // Change view settings only if GAL was active previously
            double zoom = 1.0 / ( zoomFactor * view->GetScale() );
            m_canvas->SetZoom( zoom );

            VECTOR2D center = view->GetCenter();
            RedrawScreen( wxPoint( center.x, center.y ), false );
        }
    }

    m_canvas->SetEvtHandlerEnabled( !aEnable );
    GetGalCanvas()->SetEvtHandlerEnabled( aEnable );

    // Switch panes
    m_auimgr.GetPane( wxT( "DrawFrame" ) ).Show( !aEnable );
    m_auimgr.GetPane( wxT( "DrawFrameGal" ) ).Show( aEnable );
    m_auimgr.Update();

    SetGalCanvasActive( aEnable );

    if( aEnable )
        GetGalCanvas()->SetFocus();
}

//-----< BASE_SCREEN API moved here >--------------------------------------------

wxPoint EDA_DRAW_FRAME::GetCrossHairPosition( bool aInvertY ) const
{
    // subject to change, borrow from old BASE_SCREEN for now.
    if( IsGalCanvasActive() )
    {
        VECTOR2I cursor = GetGalCanvas()->GetViewControls()->GetCursorPosition();

        return wxPoint( cursor.x, cursor.y );
    }
    else
    {
        BASE_SCREEN* screen = GetScreen();  // virtual call
        return screen->getCrossHairPosition( aInvertY );
    }
}


void EDA_DRAW_FRAME::SetCrossHairPosition( const wxPoint& aPosition, bool aSnapToGrid )
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    screen->setCrossHairPosition( aPosition, GetGridOrigin(), aSnapToGrid );
}


wxPoint EDA_DRAW_FRAME::GetCursorPosition( bool aOnGrid, wxRealPoint* aGridSize ) const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->getCursorPosition( aOnGrid, GetGridOrigin(), aGridSize );
}


wxPoint EDA_DRAW_FRAME::GetNearestGridPosition( const wxPoint& aPosition, wxRealPoint* aGridSize ) const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->getNearestGridPosition( aPosition, GetGridOrigin(), aGridSize );
}


wxPoint EDA_DRAW_FRAME::GetCrossHairScreenPosition() const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->getCrossHairScreenPosition();
}


void EDA_DRAW_FRAME::SetMousePosition( const wxPoint& aPosition )
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    screen->setMousePosition( aPosition );
}


wxPoint EDA_DRAW_FRAME::RefPos( bool useMouse ) const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->refPos( useMouse );
}


const wxPoint& EDA_DRAW_FRAME::GetScrollCenterPosition() const
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    return screen->getScrollCenterPosition();
}


void EDA_DRAW_FRAME::SetScrollCenterPosition( const wxPoint& aPoint )
{
    BASE_SCREEN* screen = GetScreen();  // virtual call
    screen->setScrollCenterPosition( aPoint );
}

//-----</BASE_SCREEN API moved here >--------------------------------------------

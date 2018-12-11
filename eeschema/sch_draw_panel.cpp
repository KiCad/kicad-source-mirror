/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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


#include <view/wx_view_controls.h>
#include <worksheet_viewitem.h>

#include <gal/graphics_abstraction_layer.h>
#include <sch_draw_panel.h>
#include <sch_view.h>
#include <sch_painter.h>
#include <sch_edit_frame.h>
#include <preview_items/selection_area.h>

#include <functional>

#include <sch_sheet.h>
#include <pgm_base.h>

using namespace std::placeholders;


// Events used by EDA_DRAW_PANEL
// GAL TODO: some (most?) of these need to be implemented...
BEGIN_EVENT_TABLE( SCH_DRAW_PANEL, wxScrolledCanvas )
//    EVT_LEAVE_WINDOW( EDA_DRAW_PANEL::OnMouseLeaving )
//    EVT_ENTER_WINDOW( EDA_DRAW_PANEL::OnMouseEntering )
//    EVT_MOUSEWHEEL( EDA_DRAW_PANEL::OnMouseWheel )
#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
//    EVT_MAGNIFY( EDA_DRAW_PANEL::OnMagnify )
#endif
//    EVT_MOUSE_EVENTS( EDA_DRAW_PANEL::OnMouseEvent )
      EVT_CHAR( SCH_DRAW_PANEL::OnKeyEvent )
      EVT_CHAR_HOOK( SCH_DRAW_PANEL::OnCharHook )
      EVT_PAINT( SCH_DRAW_PANEL::onPaint )
//    EVT_ERASE_BACKGROUND( EDA_DRAW_PANEL::OnEraseBackground )
//    EVT_SCROLLWIN( EDA_DRAW_PANEL::OnScroll )
//    EVT_ACTIVATE( EDA_DRAW_PANEL::OnActivate )
//    EVT_MENU_RANGE( ID_PAN_UP, ID_PAN_RIGHT, EDA_DRAW_PANEL::OnPan )
END_EVENT_TABLE()

SCH_DRAW_PANEL::SCH_DRAW_PANEL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                const wxPoint& aPosition, const wxSize& aSize,
                                KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
    EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aOptions, aGalType ),
    m_parent( aParentWindow )
{
    m_defaultCursor = m_currentCursor = wxCURSOR_ARROW;
    m_showCrossHair = true;
    m_view = new KIGFX::SCH_VIEW( true, dynamic_cast<SCH_BASE_FRAME*>( aParentWindow ) );
    m_view->SetGAL( m_gal );

    m_gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    m_painter.reset( new KIGFX::SCH_PAINTER( m_gal ) );

    m_view->SetPainter( m_painter.get() );
    m_view->SetScaleLimits( 50.0, 0.05 );    // This fixes the zoom in and zoom out limits
    m_view->SetMirror( false, false );

    setDefaultLayerOrder();
    setDefaultLayerDeps();

    view()->UpdateAllLayersOrder();

    // View controls is the first in the event handler chain, so the Tool Framework operates
    // on updated viewport data.
    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );

    const wxEventType events[] =
    {
        wxEVT_LEFT_UP, wxEVT_LEFT_DOWN, wxEVT_LEFT_DCLICK,
        wxEVT_RIGHT_UP, wxEVT_RIGHT_DOWN, wxEVT_RIGHT_DCLICK,
        wxEVT_MIDDLE_UP, wxEVT_MIDDLE_DOWN, wxEVT_MIDDLE_DCLICK,
        wxEVT_MOTION, wxEVT_MOUSEWHEEL,
    };

    for( auto e : events )
    {
        Connect( e, wxMouseEventHandler( SCH_DRAW_PANEL::OnMouseEvent ), NULL, this );
    }

    Connect( wxEVT_CHAR, wxKeyEventHandler( SCH_DRAW_PANEL::OnKeyEvent ), NULL, this );
    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( SCH_DRAW_PANEL::OnCharHook ), NULL, this );

    Pgm().CommonSettings()->Read( ENBL_MOUSEWHEEL_PAN_KEY, &m_enableMousewheelPan, false );
    Pgm().CommonSettings()->Read( ENBL_ZOOM_NO_CENTER_KEY, &m_enableZoomNoCenter, false );
    Pgm().CommonSettings()->Read( ENBL_AUTO_PAN_KEY, &m_enableAutoPan, true );

    m_canStartBlock = -1;       // Command block can start if >= 0
    m_abortRequest = false;
    m_ignoreMouseEvents = false;
    // Be sure a mouse release button event will be ignored when creating the canvas
    // if the mouse click was not made inside the canvas (can happen sometimes, when
    // launching a editor from a double click made in another frame)
    m_ignoreNextLeftButtonRelease = true;

    m_mouseCaptureCallback = NULL;
    m_endMouseCaptureCallback = NULL;

    m_enableBlockCommands = false;
    m_minDragEventCount = 0;

    m_cursorLevel = 0;
    m_PrintIsMirrored = false;

    m_doubleClickInterval = 250;

    m_viewControls->SetSnapping( true );

    SetEvtHandlerEnabled( true );
    SetFocus();
    Show( true );
    Raise();
    StartDrawing();
}


SCH_DRAW_PANEL::~SCH_DRAW_PANEL()
{
}


void SCH_DRAW_PANEL::DisplayComponent( const LIB_PART* aComponent )
{
    view()->Clear();
    view()->DisplayComponent( const_cast<LIB_PART*>(aComponent) );

}


void SCH_DRAW_PANEL::DisplaySheet( const SCH_SHEET* aSheet )
{
    view()->Clear();
    view()->DisplaySheet( const_cast<SCH_SHEET*>(aSheet) );
}


void SCH_DRAW_PANEL::DisplaySheet( const SCH_SCREEN *aScreen )
{
    view()->Clear();

    if( aScreen )
        view()->DisplaySheet( const_cast<SCH_SCREEN*>( aScreen ) );
}


void SCH_DRAW_PANEL::OnShow()
{
    //m_view->RecacheAllItems();
}


void SCH_DRAW_PANEL::setDefaultLayerOrder()
{
    for( LAYER_NUM i = 0; (unsigned) i < sizeof( SCH_LAYER_ORDER ) / sizeof( LAYER_NUM ); ++i )
    {
        LAYER_NUM layer = SCH_LAYER_ORDER[i];
        wxASSERT( layer < KIGFX::VIEW::VIEW_MAX_LAYERS );

        m_view->SetLayerOrder( layer, i );
    }
}


bool SCH_DRAW_PANEL::SwitchBackend( GAL_TYPE aGalType )
{
    VECTOR2D grid_size = m_gal->GetGridSize();
    bool rv = EDA_DRAW_PANEL_GAL::SwitchBackend( aGalType );
    setDefaultLayerDeps();
    m_gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    // Keep grid size and grid visibility:
    m_gal->SetGridSize( grid_size );
    SCH_BASE_FRAME* frame = dynamic_cast<SCH_BASE_FRAME*>( GetParent() );

    if( frame )
        m_gal->SetGridVisibility( frame->IsGridVisible() );

    Refresh();

    return rv;
}


void SCH_DRAW_PANEL::SetEnableMousewheelPan( bool aEnable )
{
    m_enableMousewheelPan = aEnable;

    if( GetParent()->IsGalCanvasActive() )
        GetParent()->GetGalCanvas()->GetViewControls()->EnableMousewheelPan( aEnable );
}


void SCH_DRAW_PANEL::SetEnableAutoPan( bool aEnable )
{
    EDA_DRAW_PANEL::SetEnableAutoPan( aEnable );

    if( GetParent()->IsGalCanvasActive() )
        GetParent()->GetGalCanvas()->GetViewControls()->EnableAutoPan( aEnable );
}


void SCH_DRAW_PANEL::SetAutoPanRequest( bool aEnable )
{
    wxCHECK( GetParent()->IsGalCanvasActive(), /*void*/ );
    GetParent()->GetGalCanvas()->GetViewControls()->SetAutoPan( aEnable );
}


void SCH_DRAW_PANEL::SetEnableZoomNoCenter( bool aEnable )
{
    m_enableZoomNoCenter = aEnable;

    if( GetParent()->IsGalCanvasActive() )
        GetParent()->GetGalCanvas()->GetViewControls()->EnableCursorWarping( !aEnable );
}


void SCH_DRAW_PANEL::setDefaultLayerDeps()
{
    // caching makes no sense for Cairo and other software renderers
    auto target = m_backend == GAL_TYPE_OPENGL ? KIGFX::TARGET_CACHED : KIGFX::TARGET_NONCACHED;

    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; i++ )
        m_view->SetLayerTarget( i, target );

    // Bitmaps are draw on a non cached GAL layer:
    m_view->SetLayerTarget( LAYER_DRAW_BITMAPS , KIGFX::TARGET_NONCACHED );

    // Some draw layers need specific settings
    m_view->SetLayerTarget( LAYER_GP_OVERLAY , KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_GP_OVERLAY ) ;

    m_view->SetLayerTarget( LAYER_SELECT_OVERLAY , KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_SELECT_OVERLAY ) ;

    m_view->SetLayerTarget( LAYER_WORKSHEET , KIGFX::TARGET_NONCACHED );
    m_view->SetLayerDisplayOnly( LAYER_WORKSHEET ) ;
}


KIGFX::SCH_VIEW* SCH_DRAW_PANEL::view() const
{
    return static_cast<KIGFX::SCH_VIEW*>( m_view );
}

BASE_SCREEN* SCH_DRAW_PANEL::GetScreen()
{
    return GetParent()->GetScreen();
}

EDA_DRAW_FRAME* SCH_DRAW_PANEL::GetParent() const
{
    return static_cast<EDA_DRAW_FRAME*>(m_parent); // static_cast<SCH_EDIT_FRAME*> (m_parent);
}


void SCH_DRAW_PANEL::OnMouseEvent( wxMouseEvent& event )
{
    int          localbutt = 0;
    BASE_SCREEN* screen = GetScreen();
    auto controls = GetViewControls();
    auto vmp = VECTOR2I( controls->GetMousePosition() );
    wxPoint mousePos ( vmp.x, vmp.y );

    event.Skip();

    if( !screen )
        return;

    /* Adjust value to filter mouse displacement before consider the drag
     * mouse is really a drag command, not just a movement while click
     */
#define MIN_DRAG_COUNT_FOR_START_BLOCK_COMMAND 5

    if( event.Leaving() )
        m_canStartBlock = -1;

    if( !IsMouseCaptured() )          // No mouse capture in progress.
        SetAutoPanRequest( false );

    if( GetParent()->IsActive() )
        SetFocus();
    else
        return;

    if( !event.IsButton() && !event.Moving() && !event.Dragging() )
        return;

    if( event.RightUp() )
    {
        OnRightClick( event );
        return;
    }

    if( m_ignoreMouseEvents )
        return;

    if( event.LeftDown() )
        localbutt = GR_M_LEFT_DOWN;

    if( event.ButtonDClick( 1 ) )
        localbutt = GR_M_LEFT_DOWN | GR_M_DCLICK;

    if( event.MiddleDown() )
        localbutt = GR_M_MIDDLE_DOWN;

    // Compute the cursor position in drawing (logical) units.
    //GetParent()->SetMousePosition( event.GetLogicalPosition( DC ) );

    int kbstat = 0;

    if( event.ShiftDown() )
        kbstat |= GR_KB_SHIFT;

    if( event.ControlDown() )
        kbstat |= GR_KB_CTRL;

    if( event.AltDown() )
        kbstat |= GR_KB_ALT;

    // Calling Double Click and Click functions :
    if( localbutt == (int) ( GR_M_LEFT_DOWN | GR_M_DCLICK ) )
    {
        GetParent()->OnLeftDClick( nullptr, mousePos );

        // inhibit a response to the mouse left button release,
        // because we have a double click, and we do not want a new
        // OnLeftClick command at end of this Double Click
        m_ignoreNextLeftButtonRelease = true;
    }
    else if( event.LeftUp() )
    {
        // A block command is in progress: a left up is the end of block
        // or this is the end of a double click, already seen
        // Note also m_ignoreNextLeftButtonRelease can be set by
        // the call to OnLeftClick(), so do not change it after calling OnLeftClick
        bool ignoreEvt = m_ignoreNextLeftButtonRelease;
        m_ignoreNextLeftButtonRelease = false;

        if( screen->m_BlockLocate.GetState() == STATE_NO_BLOCK && !ignoreEvt )
            GetParent()->OnLeftClick( nullptr, mousePos );

    }
    else if( !event.LeftIsDown() )
    {
        /* be sure there is a response to a left button release command
         * even when a LeftUp event is not seen.  This happens when a
         * double click opens a dialog box, and the release mouse button
         * is made when the dialog box is opened.
         */
        m_ignoreNextLeftButtonRelease = false;
    }

    if( event.ButtonDown( wxMOUSE_BTN_MIDDLE ) )
    {
        m_PanStartCenter = GetParent()->GetScrollCenterPosition();
        m_PanStartEventPosition = event.GetPosition();

          CrossHairOff( );
          SetCurrentCursor( wxCURSOR_SIZING );
    }

    if( event.ButtonUp( wxMOUSE_BTN_MIDDLE ) )
    {
         CrossHairOn();
         SetDefaultCursor();
    }

    if( event.MiddleIsDown() )
    {
        // already managed by EDA_DRAW_PANEL_GAL mouse event handler.
        return;
    }

    // Calling the general function on mouse changes (and pseudo key commands)
    GetParent()->GeneralControl( nullptr, mousePos );

    /*******************************/
    /* Control of block commands : */
    /*******************************/

    // Command block can't start if mouse is dragging a new panel
    static SCH_DRAW_PANEL* lastPanel;
    if( lastPanel != this )
    {
        m_minDragEventCount = 0;
        m_canStartBlock   = -1;
    }

    /* A new command block can start after a release buttons
     * and if the drag is enough
     * This is to avoid a false start block when a dialog box is dismissed,
     * or when changing panels in hierarchy navigation
     * or when clicking while and moving mouse
     */
    if( !event.LeftIsDown() && !event.MiddleIsDown() )
    {
        m_minDragEventCount = 0;
        m_canStartBlock   = 0;

        /* Remember the last cursor position when a drag mouse starts
         * this is the last position ** before ** clicking a button
         * this is useful to start a block command from the point where the
         * mouse was clicked first
         * (a filter creates a delay for the real block command start, and
         * we must remember this point)
         */
        m_CursorStartPos = GetParent()->GetCrossHairPosition();
    }

    if( m_enableBlockCommands && !(localbutt & GR_M_DCLICK) )
    {
        if( !screen->IsBlockActive() )
        {
            screen->m_BlockLocate.SetOrigin( m_CursorStartPos );
        }

        if( event.LeftDown() )
        {
            if( screen->m_BlockLocate.GetState() == STATE_BLOCK_MOVE )
            {
                SetAutoPanRequest( false );
                GetParent()->HandleBlockPlace( nullptr );
                m_ignoreNextLeftButtonRelease = true;
            }
        }
        else if( ( m_canStartBlock >= 0 ) && event.LeftIsDown() && !IsMouseCaptured() )
        {
            // Mouse is dragging: if no block in progress,  start a block command.
            if( screen->m_BlockLocate.GetState() == STATE_NO_BLOCK )
            {
                //  Start a block command
                int cmd_type = kbstat;

                // A block command is started if the drag is enough.  A small
                // drag is ignored (it is certainly a little mouse move when
                // clicking) not really a drag mouse
                if( m_minDragEventCount < MIN_DRAG_COUNT_FOR_START_BLOCK_COMMAND )
                    m_minDragEventCount++;
                else
                {
                    auto cmd = (GetParent()->GetToolId() == ID_ZOOM_SELECTION) ? BLOCK_ZOOM : 0;

                    DBG(printf("start block\n");)

                    if( !GetParent()->HandleBlockBegin( nullptr, cmd_type, m_CursorStartPos, cmd ) )
                    {
                        // should not occur: error
                        GetParent()->DisplayToolMsg(
                            wxT( "EDA_DRAW_PANEL::OnMouseEvent() Block Error" ) );
                    }
                    else
                    {
                        SetAutoPanRequest( true );
                        SetCursor( wxCURSOR_SIZING );
                    }
                }
            }
        }

        if( event.ButtonUp( wxMOUSE_BTN_LEFT ) )
        {
            /* Release the mouse button: end of block.
             * The command can finish (DELETE) or have a next command (MOVE,
             * COPY).  However the block command is canceled if the block
             * size is small because a block command filtering is already
             * made, this case happens, but only when the on grid cursor has
             * not moved.
             */
            #define BLOCK_MINSIZE_LIMIT 1
            bool BlockIsSmall =
                ( std::abs( screen->m_BlockLocate.GetWidth() ) < BLOCK_MINSIZE_LIMIT )
                && ( std::abs( screen->m_BlockLocate.GetHeight() ) < BLOCK_MINSIZE_LIMIT );

            if( (screen->m_BlockLocate.GetState() != STATE_NO_BLOCK) && BlockIsSmall )
            {
                if( m_endMouseCaptureCallback )
                {
                    m_endMouseCaptureCallback( this, nullptr );
                    SetAutoPanRequest( false );
                }

                //SetCursor( (wxStockCursor) m_currentCursor );
           }
            else if( screen->m_BlockLocate.GetState() == STATE_BLOCK_END )
            {
                SetAutoPanRequest( false );
                GetParent()->HandleBlockEnd( nullptr );
                //SetCursor( (wxStockCursor) m_currentCursor );
                if( screen->m_BlockLocate.GetState() == STATE_BLOCK_MOVE )
                {
                    SetAutoPanRequest( true );
                    SetCursor( wxCURSOR_HAND );
                }
           }
        }
    }

    // End of block command on a double click
    // To avoid an unwanted block move command if the mouse is moved while double clicking
    if( localbutt == (int) ( GR_M_LEFT_DOWN | GR_M_DCLICK ) )
    {
        if( !screen->IsBlockActive() && IsMouseCaptured() )
        {
            m_endMouseCaptureCallback( this, nullptr );
        }
    }

    lastPanel = this;

}


bool SCH_DRAW_PANEL::OnRightClick( wxMouseEvent& event )
{
    auto controls = GetViewControls();
    auto vmp = controls->GetMousePosition();
    wxPoint mouseWorldPos ( (int) vmp.x, (int) vmp.y );

    wxMenu  MasterMenu;

    if( !GetParent()->OnRightClick( mouseWorldPos, &MasterMenu ) )
        return false;

    GetParent()->AddMenuZoomAndGrid( &MasterMenu );

    m_ignoreMouseEvents = true;
    PopupMenu( &MasterMenu, event.GetPosition() );
    m_ignoreMouseEvents = false;

    return true;
}

void SCH_DRAW_PANEL::CallMouseCapture( wxDC* aDC, const wxPoint& aPosition, bool aErase )
{
    wxCHECK_RET( m_mouseCaptureCallback != NULL, wxT( "Mouse capture callback not set." ) );

    m_mouseCaptureCallback( this, aDC, aPosition, aErase );
}


void SCH_DRAW_PANEL::CallEndMouseCapture( wxDC* aDC )
{
    // CallEndMouseCapture is sometimes called with m_endMouseCaptureCallback == NULL
    // for instance after an ABORT in block paste.
    if( m_endMouseCaptureCallback )
        m_endMouseCaptureCallback( this, aDC );
}


void SCH_DRAW_PANEL::EndMouseCapture( int id, int cursor, const wxString& title,
                                      bool aCallEndFunc )
{
    if( m_mouseCaptureCallback && m_endMouseCaptureCallback && aCallEndFunc )
    {
        m_endMouseCaptureCallback( this, nullptr );
    }

    m_mouseCaptureCallback = NULL;
    m_endMouseCaptureCallback = NULL;
    SetAutoPanRequest( false );

    if( id != -1 && cursor != -1 )
    {
        //wxASSERT( cursor > wxCURSOR_NONE && cursor < wxCURSOR_MAX );
        GetParent()->SetToolID( id, cursor, title );
    }
}

void SCH_DRAW_PANEL::CrossHairOff( wxDC* DC )
{
    m_viewControls->ShowCursor( false );
}


void SCH_DRAW_PANEL::CrossHairOn( wxDC* DC )
{
    m_viewControls->ShowCursor( true );
}


void SCH_DRAW_PANEL::MoveCursorToCrossHair()
{
    GetViewControls()->WarpCursor( GetParent()->GetCrossHairPosition(), true );
}


void SCH_DRAW_PANEL::Refresh( bool aEraseBackground, const wxRect* aRect )
{
    EDA_DRAW_PANEL_GAL::Refresh( aEraseBackground, aRect );
}


void SCH_DRAW_PANEL::OnCharHook( wxKeyEvent& event )
{
    event.Skip();
}


void SCH_DRAW_PANEL::OnKeyEvent( wxKeyEvent& event )
{
    int localkey;

    localkey = event.GetKeyCode();

    switch( localkey )
    {
    default:
        break;

    case WXK_ESCAPE:
        m_abortRequest = true;

        if( IsMouseCaptured() )
            EndMouseCapture();
        else
            EndMouseCapture( ID_NO_TOOL_SELECTED, 0 /*m_defaultCursor*/, wxEmptyString );
        break;
    }

    /* Normalize keys code to easily handle keys from Ctrl+A to Ctrl+Z
     * They have an ascii code from 1 to 27 remapped
     * to GR_KB_CTRL + 'A' to GR_KB_CTRL + 'Z'
     */
    if( event.ControlDown() && localkey >= WXK_CONTROL_A && localkey <= WXK_CONTROL_Z )
        localkey += 'A' - 1;

    /* Disallow shift for keys that have two keycodes on them (e.g. number and
     * punctuation keys) leaving only the "letter keys" of A-Z.
     * Then, you can have, e.g. Ctrl-5 and Ctrl-% (GB layout)
     * and Ctrl-( and Ctrl-5 (FR layout).
     * Otherwise, you'd have to have to say Ctrl-Shift-5 on a FR layout
     */
    bool keyIsLetter = ( localkey >= 'A' && localkey <= 'Z' ) ||
                       ( localkey >= 'a' && localkey <= 'z' );

    if( event.ShiftDown() && ( keyIsLetter || localkey > 256 ) )
        localkey |= GR_KB_SHIFT;

    if( event.ControlDown() )
        localkey |= GR_KB_CTRL;

    if( event.AltDown() )
        localkey |= GR_KB_ALT;


    // Some key commands use the current mouse position: refresh it.
    //pos = wxGetMousePosition() - GetScreenPosition();

    // Compute the cursor position in drawing units.  Also known as logical units to wxDC.
    //pos = wxPoint( DC.DeviceToLogicalX( pos.x ), DC.DeviceToLogicalY( pos.y ) );

    auto p =  GetViewControls()->GetCursorPosition( false );

    wxPoint pos ((int)p.x, (int)p.y);

    GetParent()->SetMousePosition( pos );

    if( !GetParent()->GeneralControl( nullptr, pos, localkey ) )
        event.Skip();
}


void SCH_DRAW_PANEL::onPaint( wxPaintEvent& aEvent )
{
    if( !m_gal->IsInitialized() )
        // The first wxPaintEvent can be fired at startup before the GAL engine is fully initialized
        // (depending on platforms). Do nothing in this case
        return;

    if( m_painter )
        static_cast<KIGFX::SCH_PAINTER*>(m_painter.get())->GetSettings()->ImportLegacyColors( nullptr );

    EDA_DRAW_PANEL_GAL::onPaint( aEvent );
}

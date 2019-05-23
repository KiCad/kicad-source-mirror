/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <functional>
#include <sch_sheet.h>
#include <pgm_base.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>

using namespace std::placeholders;


// Events used by EDA_DRAW_PANEL
BEGIN_EVENT_TABLE( SCH_DRAW_PANEL, wxScrolledCanvas )
      EVT_CHAR( SCH_DRAW_PANEL::OnKeyEvent )
      EVT_CHAR_HOOK( SCH_DRAW_PANEL::OnCharHook )
      EVT_PAINT( SCH_DRAW_PANEL::onPaint )
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

    // Early initialization of the canvas background color,
    // before any OnPaint event is fired for the canvas using a wrong bg color
    auto settings = m_painter->GetSettings();
    m_gal->SetClearColor( settings->GetBackgroundColor() );

    setDefaultLayerOrder();
    setDefaultLayerDeps();

    view()->UpdateAllLayersOrder();

    // View controls is the first in the event handler chain, so the Tool Framework operates
    // on updated viewport data.
    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );

    Connect( wxEVT_CHAR, wxKeyEventHandler( SCH_DRAW_PANEL::OnKeyEvent ), NULL, this );
    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( SCH_DRAW_PANEL::OnCharHook ), NULL, this );

    Pgm().CommonSettings()->Read( ENBL_MOUSEWHEEL_PAN_KEY, &m_enableMousewheelPan, false );
    Pgm().CommonSettings()->Read( ENBL_ZOOM_NO_CENTER_KEY, &m_enableZoomNoCenter, false );
    Pgm().CommonSettings()->Read( ENBL_AUTO_PAN_KEY, &m_enableAutoPan, true );

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


void SCH_DRAW_PANEL::DisplaySheet( const SCH_SCREEN *aScreen )
{
    view()->Clear();

    if( aScreen )
        view()->DisplaySheet( const_cast<SCH_SCREEN*>( aScreen ) );
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
    SCH_BASE_FRAME* frame = (SCH_BASE_FRAME*) m_parent;
    int             localkey = event.GetKeyCode();
    bool            keyWasHandled = false;

    if( localkey == WXK_ESCAPE )
    {
        m_abortRequest = true;

        if( frame->IsModal() )
            frame->DismissModal( wxID_CANCEL );
        else
            GetParent()->GetToolManager()->RunAction( ACTIONS::cancelInteractive, true );

        keyWasHandled = true;   // The key is captured: the key event will be not skipped
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

    // a Key event has to be skipped only if it is not handled:
    if( !GetParent()->GeneralControl( nullptr, pos, localkey ) && !keyWasHandled )
        event.Skip();
}


void SCH_DRAW_PANEL::onPaint( wxPaintEvent& aEvent )
{
    if( !m_gal->IsInitialized() || !m_gal->IsVisible() )
        // The first wxPaintEvent can be fired at startup before the GAL engine is fully initialized
        // (depending on platforms). Do nothing in this case
        return;

    if( m_painter )
        static_cast<KIGFX::SCH_PAINTER*>(m_painter.get())->GetSettings()->ImportLegacyColors( nullptr );

    EDA_DRAW_PANEL_GAL::onPaint( aEvent );
}

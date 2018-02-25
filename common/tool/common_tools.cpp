/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
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

#include <tool/actions.h>
#include <draw_frame.h>
#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <base_screen.h>
#include <hotkeys.h>

#include <tool/common_tools.h>


static TOOL_ACTION ACT_toggleCursor( "common.Control.toggleCursor",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_TOGGLE_CURSOR ),
        _( "Toggle Always Show Cursor" ),
        _( "Toogle display of the cursor, even when not in an interactive tool" ) );


COMMON_TOOLS::COMMON_TOOLS() :
    TOOL_INTERACTIVE( "common.Control" ), m_frame( NULL )
{
}


COMMON_TOOLS::~COMMON_TOOLS()
{
}


void COMMON_TOOLS::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<EDA_DRAW_FRAME>();
}


int COMMON_TOOLS::ZoomInOut( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = m_frame->GetGalCanvas()->GetView();
    KIGFX::VIEW_CONTROLS* ctls = getViewControls();
    double zoomScale = 1.0;

    if( aEvent.IsAction( &ACTIONS::zoomIn ) )
        zoomScale = 1.3;
    else if( aEvent.IsAction( &ACTIONS::zoomOut ) )
        zoomScale = 1/1.3;

    view->SetScale( view->GetScale() * zoomScale, getViewControls()->GetCursorPosition() );

    if( ctls->IsCursorWarpingEnabled() )
        ctls->CenterOnCursor();

    return 0;
}


int COMMON_TOOLS::ZoomInOutCenter( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();
    double zoomScale = 1.0;

    if( aEvent.IsAction( &ACTIONS::zoomInCenter ) )
        zoomScale = 1.3;
    else if( aEvent.IsAction( &ACTIONS::zoomOutCenter ) )
        zoomScale = 1/1.3;

    view->SetScale( view->GetScale() * zoomScale );

    return 0;
}


int COMMON_TOOLS::ZoomCenter( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* ctls = getViewControls();

    ctls->CenterOnCursor();

    return 0;
}


int COMMON_TOOLS::ZoomFitScreen( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();
    EDA_DRAW_PANEL_GAL* galCanvas = m_frame->GetGalCanvas();
    EDA_ITEM* model = getModel<EDA_ITEM>();
    EDA_BASE_FRAME* frame = getEditFrame<EDA_BASE_FRAME>();

    BOX2I bBox = model->ViewBBox();
    VECTOR2D scrollbarSize = VECTOR2D( galCanvas->GetSize() - galCanvas->GetClientSize() );
    VECTOR2D screenSize = view->ToWorld( galCanvas->GetClientSize(), false );

    if( bBox.GetWidth() == 0 || bBox.GetHeight() == 0 )
    {
        bBox = galCanvas->GetDefaultViewBBox();
    }

    VECTOR2D vsize = bBox.GetSize();
    double scale = view->GetScale() / std::max( fabs( vsize.x / screenSize.x ),
                                                fabs( vsize.y / screenSize.y ) );

    // Reserve a 10% margin around component bounding box.
    double margin_scale_factor = 1.1;

    // Leave 20% for library editors & viewers
    if( frame->IsType( FRAME_PCB_MODULE_VIEWER ) || frame->IsType( FRAME_PCB_MODULE_VIEWER_MODAL )
            || frame->IsType( FRAME_SCH_VIEWER ) || frame->IsType( FRAME_SCH_VIEWER_MODAL )
            || frame->IsType( FRAME_SCH_LIB_EDITOR ) || frame->IsType( FRAME_PCB_MODULE_EDITOR ) )
    {
        margin_scale_factor = 1.2;
    }

    view->SetScale( scale / margin_scale_factor );
    view->SetCenter( bBox.Centre() );

    // Take scrollbars into account
    VECTOR2D worldScrollbarSize = view->ToWorld( scrollbarSize, false );
    view->SetCenter( view->GetCenter() + worldScrollbarSize / 2.0 );

    return 0;
}


int COMMON_TOOLS::ZoomPreset( const TOOL_EVENT& aEvent )
{
    unsigned int idx = aEvent.Parameter<intptr_t>();
    std::vector<double>& zoomList = m_frame->GetScreen()->m_ZoomList;
    KIGFX::VIEW* view = m_frame->GetGalCanvas()->GetView();
    KIGFX::GAL* gal = m_frame->GetGalCanvas()->GetGAL();

    m_frame->SetPresetZoom( idx );

    if( idx == 0 )      // Zoom Auto
    {
        return ZoomFitScreen( aEvent );
    }
    else if( idx >= zoomList.size() )
    {
        assert( false );
        return 0;
    }

    double selectedZoom = zoomList[idx];
    double zoomFactor = gal->GetWorldScale() / gal->GetZoomFactor();
    view->SetScale( 1.0 / ( zoomFactor * selectedZoom ) );

    return 0;
}


// Grid control
int COMMON_TOOLS::GridNext( const TOOL_EVENT& aEvent )
{
    m_frame->SetNextGrid();
    updateGrid();

    return 0;
}


int COMMON_TOOLS::GridPrev( const TOOL_EVENT& aEvent )
{
    m_frame->SetPrevGrid();
    updateGrid();

    return 0;
}


int COMMON_TOOLS::GridPreset( const TOOL_EVENT& aEvent )
{
    long idx = aEvent.Parameter<intptr_t>();

    m_frame->SetPresetGrid( idx );
    updateGrid();

    return 0;
}


int COMMON_TOOLS::ToggleCursor( const TOOL_EVENT& aEvent )
{
    auto& galOpts = m_frame->GetGalDisplayOptions();

    galOpts.m_forceDisplayCursor = !galOpts.m_forceDisplayCursor;
    galOpts.NotifyChanged();

    return 0;
}


void COMMON_TOOLS::setTransitions()
{
    Go( &COMMON_TOOLS::ZoomInOut,          ACTIONS::zoomIn.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOut,          ACTIONS::zoomOut.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOutCenter,    ACTIONS::zoomInCenter.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOutCenter,    ACTIONS::zoomOutCenter.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomCenter,         ACTIONS::zoomCenter.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomFitScreen,      ACTIONS::zoomFitScreen.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomPreset,         ACTIONS::zoomPreset.MakeEvent() );

    Go( &COMMON_TOOLS::GridNext,           ACTIONS::gridNext.MakeEvent() );
    Go( &COMMON_TOOLS::GridPrev,           ACTIONS::gridPrev.MakeEvent() );
    Go( &COMMON_TOOLS::GridPreset,         ACTIONS::gridPreset.MakeEvent() );

    Go( &COMMON_TOOLS::ToggleCursor,       ACT_toggleCursor.MakeEvent() );
}


void COMMON_TOOLS::updateGrid()
{
    BASE_SCREEN* screen = m_frame->GetScreen();
    getView()->GetGAL()->SetGridSize( VECTOR2D( screen->GetGridSize() ) );
    getView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
}

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
    bool direction = aEvent.IsAction( &ACTIONS::zoomIn );
    return doZoomInOut( direction, true );
}


int COMMON_TOOLS::ZoomInOutCenter( const TOOL_EVENT& aEvent )
{
    bool direction = aEvent.IsAction( &ACTIONS::zoomInCenter );
    return doZoomInOut( direction, false );
}


int COMMON_TOOLS::doZoomInOut( bool aDirection, bool aCenterOnCursor )
{
    double zoom = m_frame->GetGalCanvas()->GetLegacyZoom();

    // Step must be AT LEAST 1.3
    if( aDirection )
        zoom /= 1.3;
    else
        zoom *= 1.3;

    // Now look for the next closest menu step
    std::vector<double>& zoomList = m_frame->GetScreen()->m_ZoomList;
    int idx;

    if( aDirection )
    {
        for( idx = zoomList.size() - 1; idx >= 0; --idx )
        {
            if( zoomList[idx] <= zoom )
                break;
        }

        if( idx < 0 )
            idx = 0;        // if we ran off the end then peg to the end
    }
    else
    {
        for( idx = 0; idx < (int)zoomList.size(); ++idx )
        {
            if( zoomList[idx] >= zoom )
                break;
        }

        if( idx >= (int)zoomList.size() )
            idx = zoomList.size() - 1;        // if we ran off the end then peg to the end
    }

    return doZoomToPreset( idx + 1, aCenterOnCursor );
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
    return doZoomToPreset( idx, false );
}


// Note: idx == 0 is Auto; idx == 1 is first entry in zoomList
int COMMON_TOOLS::doZoomToPreset( int idx, bool aCenterOnCursor )
{
    std::vector<double>& zoomList = m_frame->GetScreen()->m_ZoomList;
    KIGFX::VIEW* view = m_frame->GetGalCanvas()->GetView();
    KIGFX::GAL* gal = m_frame->GetGalCanvas()->GetGAL();

    m_frame->SetPresetZoom( idx );

    if( idx == 0 )      // Zoom Auto
    {
        TOOL_EVENT dummy;
        return ZoomFitScreen( dummy );
    }
    else
        idx--;

    double selectedZoom = zoomList[idx];
    double zoomFactor = gal->GetWorldScale() / gal->GetZoomFactor();
    double scale = 1.0 / ( zoomFactor * selectedZoom );

    if( aCenterOnCursor )
    {
        view->SetScale( scale, getViewControls()->GetCursorPosition() );

        if( getViewControls()->IsCursorWarpingEnabled() )
            getViewControls()->CenterOnCursor();
    }
    else
        view->SetScale( scale );

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

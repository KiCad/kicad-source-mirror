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
#include <class_base_screen.h>

#include <tool/common_tools.h>

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
        zoomScale = 0.7;

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
        zoomScale = 0.7;

    view->SetScale( view->GetScale() * zoomScale );

    return 0;
}


int COMMON_TOOLS::ZoomCenter( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* ctls = getViewControls();

    if( ctls->IsCursorWarpingEnabled() )
        ctls->CenterOnCursor();
    else
        getView()->SetCenter( getViewControls()->GetCursorPosition() );

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


void COMMON_TOOLS::SetTransitions()
{
    Go( &COMMON_TOOLS::ZoomInOut,          ACTIONS::zoomIn.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOut,          ACTIONS::zoomOut.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOutCenter,    ACTIONS::zoomInCenter.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOutCenter,    ACTIONS::zoomOutCenter.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomCenter,         ACTIONS::zoomCenter.MakeEvent() );

    Go( &COMMON_TOOLS::GridNext,           ACTIONS::gridNext.MakeEvent() );
    Go( &COMMON_TOOLS::GridPrev,           ACTIONS::gridPrev.MakeEvent() );
    Go( &COMMON_TOOLS::GridPreset,         ACTIONS::gridPreset.MakeEvent() );
}


void COMMON_TOOLS::updateGrid()
{
    BASE_SCREEN* screen = m_frame->GetScreen();
    getView()->GetGAL()->SetGridSize( VECTOR2D( screen->GetGridSize() ) );
    getView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
}

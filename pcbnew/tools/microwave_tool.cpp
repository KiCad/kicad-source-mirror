/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2019 Kicad Developers, see change_log.txt for contributors.
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

#include "microwave_tool.h"
#include <gal/graphics_abstraction_layer.h>
#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <board_commit.h>
#include <confirm.h>
#include <preview_items/two_point_geom_manager.h>
#include <preview_items/centreline_rect_item.h>
#include <pcbnew_id.h>
#include <bitmaps.h>
#include <class_board_item.h>
#include <class_module.h>
#include <microwave/microwave_inductor.h>
#include "pcb_actions.h"
#include "selection_tool.h"


MICROWAVE_TOOL::MICROWAVE_TOOL() :
        PCB_TOOL_BASE( "pcbnew.MicrowaveTool" )
{
}


MICROWAVE_TOOL::~MICROWAVE_TOOL()
{}


void MICROWAVE_TOOL::Reset( RESET_REASON aReason )
{
}


int MICROWAVE_TOOL::addMicrowaveFootprint( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME>();

    struct MICROWAVE_PLACER : public INTERACTIVE_PLACER_BASE
    {
        MICROWAVE_PLACER( PCB_EDIT_FRAME* aFrame, int aType ) :
                m_frame( aFrame ),
                m_itemType( aType )
        { };

        std::unique_ptr<BOARD_ITEM> CreateItem() override
        {
            switch( m_itemType )
            {
            case MWAVE_TOOL_SIMPLE_ID::GAP:
                return std::unique_ptr<MODULE>( m_frame->Create_MuWaveComponent( 0 ) );
            case MWAVE_TOOL_SIMPLE_ID::STUB:
                return std::unique_ptr<MODULE>( m_frame->Create_MuWaveComponent( 1 ) );
            case MWAVE_TOOL_SIMPLE_ID::STUB_ARC:
                return std::unique_ptr<MODULE>( m_frame->Create_MuWaveComponent( 2 ) );
            case MWAVE_TOOL_SIMPLE_ID::FUNCTION_SHAPE:
                return std::unique_ptr<MODULE>( m_frame->Create_MuWavePolygonShape() );
            default:
                return std::unique_ptr<MODULE>();
            };
        }

    private:
        PCB_EDIT_FRAME* m_frame;
        int             m_itemType;
    };

    MICROWAVE_PLACER placer( frame, aEvent.Parameter<intptr_t>() );

    doInteractiveItemPlacement( aEvent.GetCommandStr().get(), &placer,
                                _( "Place microwave feature" ),
                                IPO_REPEAT | IPO_ROTATE | IPO_FLIP );

    return 0;
}


void MICROWAVE_TOOL::createInductorBetween( const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    auto& frame = *getEditFrame<PCB_EDIT_FRAME>();

    MWAVE::INDUCTOR_PATTERN pattern;

    pattern.m_Width = board()->GetDesignSettings().GetCurrentTrackWidth();

    pattern.m_Start = { aStart.x, aStart.y };
    pattern.m_End = { aEnd.x, aEnd.y };

    wxString errorMessage;

    auto inductorModule = std::unique_ptr<MODULE>( CreateMicrowaveInductor( pattern, &frame,
                                                                            errorMessage ) );

    // on any error, report if we can
    if ( !inductorModule || !errorMessage.IsEmpty() )
    {
        if ( !errorMessage.IsEmpty() )
            DisplayError( &frame, errorMessage );
    }
    else
    {
        // at this point, we can save the module
        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, inductorModule.get() );

        BOARD_COMMIT commit( this );
        commit.Add( inductorModule.release() );
        commit.Push( _("Add microwave inductor" ) );
    }
}


static const COLOR4D inductorAreaFill( 0.3, 0.3, 0.5, 0.3 );
static const COLOR4D inductorAreaStroke( 0.4, 1.0, 1.0, 1.0 );
static const double  inductorAreaStrokeWidth = 1.0;

///> Aspect of the preview rectangle - this is hardcoded in the
///> microwave backend for now
static const double  inductorAreaAspect = 0.5;


int MICROWAVE_TOOL::drawMicrowaveInductor( const TOOL_EVENT& aEvent )
{
    using namespace KIGFX::PREVIEW;

    KIGFX::VIEW& view = *getView();
    KIGFX::VIEW_CONTROLS& controls = *getViewControls();
    auto& frame = *getEditFrame<PCB_EDIT_FRAME>();

    std::string tool = aEvent.GetCommandStr().get();
    frame.PushTool( tool );
    Activate();

    TWO_POINT_GEOMETRY_MANAGER tpGeomMgr;

    CENTRELINE_RECT_ITEM previewRect( tpGeomMgr, inductorAreaAspect );

    previewRect.SetFillColor( inductorAreaFill );
    previewRect.SetStrokeColor( inductorAreaStroke );
    previewRect.SetLineWidth( inductorAreaStrokeWidth );

    bool originSet = false;

    controls.ShowCursor( true );
    controls.SetSnapping( true );
    controls.CaptureCursor( false );
    controls.SetAutoPan( false );

    view.Add( &previewRect );

    while( auto evt = Wait() )
    {
        frame.GetCanvas()->SetCurrentCursor( wxCURSOR_PENCIL );
        VECTOR2I cursorPos = controls.GetCursorPosition();

        auto cleanup = [&] () {
            originSet = false;
            controls.CaptureCursor( false );
            controls.SetAutoPan( false );
            view.SetVisible( &previewRect, false );
            view.Update( &previewRect, KIGFX::GEOMETRY );
        };

        if( evt->IsCancelInteractive() )
        {
            if( originSet )
                cleanup();
            else
            {
                frame.PopTool( tool );
                break;
            }
        }

        else if( evt->IsActivate() )
        {
            if( originSet )
                cleanup();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                frame.PopTool( tool );
                break;
            }
        }

        // A click or drag starts
        else if( !originSet && ( evt->IsClick( BUT_LEFT ) || evt->IsDrag( BUT_LEFT ) ) )
        {
            tpGeomMgr.SetOrigin( cursorPos );
            tpGeomMgr.SetEnd( cursorPos );

            originSet = true;
            controls.CaptureCursor( true );
            controls.SetAutoPan( true );
        }

        // another click after origin set is the end
        // left up is also the end, as you'll only get that after a drag
        else if( originSet && ( evt->IsClick( BUT_LEFT ) || evt->IsMouseUp( BUT_LEFT ) ) )
        {
            // second click, we're done:
            // delegate to the point-to-point inductor creator function
            createInductorBetween( tpGeomMgr.GetOrigin(), tpGeomMgr.GetEnd() );

            // start again if needed
            originSet = false;
            controls.CaptureCursor( false );
            controls.SetAutoPan( false );

            view.SetVisible( &previewRect, false );
            view.Update( &previewRect, KIGFX::GEOMETRY );
        }

        // any move or drag once the origin was set updates
        // the end point
        else if( originSet && ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            tpGeomMgr.SetAngleSnap( evt->Modifier( MD_CTRL ) );
            tpGeomMgr.SetEnd( cursorPos );

            view.SetVisible( &previewRect, true );
            view.Update( &previewRect, KIGFX::GEOMETRY );
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }

        else
            evt->SetPassEvent();
    }

    controls.CaptureCursor( false );
    controls.SetAutoPan( false );
    view.Remove( &previewRect );
    return 0;
}


void MICROWAVE_TOOL::setTransitions()
{
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint, PCB_ACTIONS::microwaveCreateGap.MakeEvent() );
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint, PCB_ACTIONS::microwaveCreateStub.MakeEvent() );
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint, PCB_ACTIONS::microwaveCreateStubArc.MakeEvent() );
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint, PCB_ACTIONS::microwaveCreateFunctionShape.MakeEvent() );

    Go( &MICROWAVE_TOOL::drawMicrowaveInductor, PCB_ACTIONS::microwaveCreateLine.MakeEvent() );
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <board_commit.h>
#include <board_item.h>
#include <class_draw_panel_gal.h>
#include <footprint.h>
#include <confirm.h>
#include <gal/graphics_abstraction_layer.h>
#include <microwave/microwave_tool.h>
#include <preview_items/two_point_geom_manager.h>
#include <preview_items/centreline_rect_item.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <view/view_controls.h>
#include <view/view.h>


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
    struct MICROWAVE_PLACER : public INTERACTIVE_PLACER_BASE
    {
        MICROWAVE_PLACER( MICROWAVE_TOOL* aTool, MICROWAVE_FOOTPRINT_SHAPE aType ) :
                m_tool( aTool ),
                m_itemType( aType )
        { };

        virtual ~MICROWAVE_PLACER()
        {
        }

        std::unique_ptr<BOARD_ITEM> CreateItem() override
        {
            switch( m_itemType )
            {
            case MICROWAVE_FOOTPRINT_SHAPE::GAP:
            case MICROWAVE_FOOTPRINT_SHAPE::STUB:
            case MICROWAVE_FOOTPRINT_SHAPE::STUB_ARC:
                return std::unique_ptr<FOOTPRINT>( m_tool->createFootprint( m_itemType ) );

            case MICROWAVE_FOOTPRINT_SHAPE::FUNCTION_SHAPE:
                return std::unique_ptr<FOOTPRINT>( m_tool->createPolygonShape() );

            default:
                return std::unique_ptr<FOOTPRINT>();
            };
        }

    private:
        MICROWAVE_TOOL*           m_tool;
        MICROWAVE_FOOTPRINT_SHAPE m_itemType;
    };

    MICROWAVE_PLACER placer( this, aEvent.Parameter<MICROWAVE_FOOTPRINT_SHAPE>() );

    doInteractiveItemPlacement( aEvent, &placer, _( "Place microwave feature" ),
                                IPO_REPEAT | IPO_ROTATE | IPO_FLIP );

    return 0;
}


static const COLOR4D inductorAreaFill( 0.3, 0.3, 0.5, 0.3 );
static const COLOR4D inductorAreaStroke( 0.4, 1.0, 1.0, 1.0 );
static const double  inductorAreaStrokeWidth = 1.0;

///< Aspect of the preview rectangle - this is hardcoded in the
///< microwave backend for now
static const double  inductorAreaAspect = 0.5;


int MICROWAVE_TOOL::drawMicrowaveInductor( const TOOL_EVENT& aEvent )
{
    using namespace KIGFX::PREVIEW;

    KIGFX::VIEW&          view = *getView();
    KIGFX::VIEW_CONTROLS& controls = *getViewControls();
    PCB_EDIT_FRAME&       frame = *getEditFrame<PCB_EDIT_FRAME>();

    frame.PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                frame.GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls.ShowCursor( true );
    controls.CaptureCursor( false );
    controls.SetAutoPan( false );
    // Set initial cursor
    setCursor();

    bool                       originSet = false;
    TWO_POINT_GEOMETRY_MANAGER tpGeomMgr;
    CENTRELINE_RECT_ITEM       previewRect( tpGeomMgr, inductorAreaAspect );

    previewRect.SetFillColor( inductorAreaFill );
    previewRect.SetStrokeColor( inductorAreaStroke );
    previewRect.SetLineWidth( inductorAreaStrokeWidth );
    view.Add( &previewRect );

    while( auto evt = Wait() )
    {
        setCursor();
        VECTOR2I cursorPos = controls.GetCursorPosition();

        auto cleanup =
                [&] ()
                {
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
                frame.PopTool( aEvent );
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
                frame.PopTool( aEvent );
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
            tpGeomMgr.SetAngleSnap( GetAngleSnapMode() );
            tpGeomMgr.SetEnd( cursorPos );

            view.SetVisible( &previewRect, true );
            view.Update( &previewRect, KIGFX::GEOMETRY );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( selection() );
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    view.Remove( &previewRect );

    frame.GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    controls.CaptureCursor( false );
    controls.SetAutoPan( false );
    return 0;
}



void MICROWAVE_TOOL::setTransitions()
{
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint, PCB_ACTIONS::microwaveCreateGap.MakeEvent() );
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint, PCB_ACTIONS::microwaveCreateStub.MakeEvent() );
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint, PCB_ACTIONS::microwaveCreateStubArc.MakeEvent() );
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint,
        PCB_ACTIONS::microwaveCreateFunctionShape.MakeEvent() );

    Go( &MICROWAVE_TOOL::drawMicrowaveInductor, PCB_ACTIONS::microwaveCreateLine.MakeEvent() );
}

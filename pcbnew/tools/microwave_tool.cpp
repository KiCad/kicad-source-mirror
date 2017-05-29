/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Kicad Developers, see change_log.txt for contributors.
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

// For frame ToolID values
#include <pcbnew_id.h>

// For action icons
#include <bitmaps.h>

#include <class_board_item.h>
#include <class_module.h>

#include <microwave/microwave_inductor.h>

#include "pcb_actions.h"
#include "selection_tool.h"
#include "tool_event_utils.h"


///> Type of items that are "simple" - just get placed on
///> the board directly, without a graphical interactive setup stage
enum MWAVE_TOOL_SIMPLE_ID
{
    GAP,
    STUB,
    STUB_ARC,
    FUNCTION_SHAPE,
};

TOOL_ACTION PCB_ACTIONS::microwaveCreateGap(
        "pcbnew.MicrowaveTool.createGap",
        AS_GLOBAL, 0,
        _( "Add Gap" ), _( "Create gap of specified length for microwave applications" ),
        mw_add_gap_xpm, AF_ACTIVATE, (void*) MWAVE_TOOL_SIMPLE_ID::GAP );

TOOL_ACTION PCB_ACTIONS::microwaveCreateStub(
        "pcbnew.MicrowaveTool.createStub",
        AS_GLOBAL, 0,
        _( "Add Stub" ), _( "Create stub of specified length for microwave applications" ),
        mw_add_stub_xpm, AF_ACTIVATE, (void*) MWAVE_TOOL_SIMPLE_ID::STUB );

TOOL_ACTION PCB_ACTIONS::microwaveCreateStubArc(
        "pcbnew.MicrowaveTool.createStubArc",
        AS_GLOBAL, 0,
        _( "Add Arc Stub" ), _( "Create stub (arc) of specified length for microwave applications" ),
        mw_add_stub_arc_xpm, AF_ACTIVATE, (void*) MWAVE_TOOL_SIMPLE_ID::STUB_ARC );

TOOL_ACTION PCB_ACTIONS::microwaveCreateFunctionShape(
        "pcbnew.MicrowaveTool.createFunctionShape",
        AS_GLOBAL, 0,
        _( "Add Polynomial Shape" ), _( "Create polynomial shape for microwave applications" ),
        mw_add_gap_xpm, AF_ACTIVATE, (void*) MWAVE_TOOL_SIMPLE_ID::FUNCTION_SHAPE );

TOOL_ACTION PCB_ACTIONS::microwaveCreateLine(
        "pcbnew.MicrowaveTool.createLine",
        AS_GLOBAL, 0,
        _( "Add Microwave Line" ), _( "Create line of specified length for microwave applications" ),
        mw_add_line_xpm, AF_ACTIVATE );


MICROWAVE_TOOL::MICROWAVE_TOOL() :
        PCB_TOOL( "pcbnew.MicrowaveTool" ),
        m_menu( *this )
{
}


MICROWAVE_TOOL::~MICROWAVE_TOOL()
{}


void MICROWAVE_TOOL::Reset( RESET_REASON aReason )
{
}


bool MICROWAVE_TOOL::Init()
{
    auto activeToolFunctor = [ this ] ( const SELECTION& aSel ) {
        return true;
    };

    auto& ctxMenu = m_menu.GetMenu();

    // cancel current goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolFunctor, 1000 );
    ctxMenu.AddSeparator( activeToolFunctor, 1000 );

    m_menu.AddStandardSubMenus( *getEditFrame<PCB_BASE_FRAME>() );

    return true;
}


struct MICROWAVE_TOOL_INFO
{
    using MOD_CREATOR = std::function<std::unique_ptr<MODULE>()>;

    wxString  name;
    int       toolId;
    MOD_CREATOR creatorFunc;
};


MICROWAVE_TOOL_INFO getMicrowaveItemCreator( PCB_EDIT_FRAME& frame, int aParam )
{
    MICROWAVE_TOOL_INFO info;

    switch( aParam )
    {
    case MWAVE_TOOL_SIMPLE_ID::GAP:
        info.name =  _( "Add Gap" );
        info.toolId = ID_PCB_MUWAVE_TOOL_GAP_CMD;
        info.creatorFunc = [&frame] () {
            return std::unique_ptr<MODULE>( frame.Create_MuWaveComponent( 0 ) );
        };
        break;

    case MWAVE_TOOL_SIMPLE_ID::STUB:
        info.name =  _( "Add Stub" );
        info.toolId = ID_PCB_MUWAVE_TOOL_STUB_CMD;
        info.creatorFunc = [&frame] () {
            return std::unique_ptr<MODULE>( frame.Create_MuWaveComponent( 1 ) );
        };
        break;

    case MWAVE_TOOL_SIMPLE_ID::STUB_ARC:
        info.name =  _( "Add Stub (Arc)" );
        info.toolId = ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD;
        info.creatorFunc = [&frame] () {
            return std::unique_ptr<MODULE>( frame.Create_MuWaveComponent( 2 ) );
        };
        break;

    case MWAVE_TOOL_SIMPLE_ID::FUNCTION_SHAPE:
        info.name =  _( "Add Polynomial Shape" );
        info.toolId = ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD;
        info.creatorFunc = [&frame] () {
            return std::unique_ptr<MODULE>( frame.Create_MuWavePolygonShape() );
        };
        break;

    default:
        // Avoid uninitilized value:
        info.toolId = 0;
        // info.name is already set to empty string
        break;
    };

    return info;
}


int MICROWAVE_TOOL::addMicrowaveFootprint( const TOOL_EVENT& aEvent )
{
    auto& frame = *getEditFrame<PCB_EDIT_FRAME>();

    const int param = aEvent.Parameter<intptr_t>();

    MICROWAVE_TOOL_INFO info = getMicrowaveItemCreator( frame, param );

    // failed to find suitable item info - shouldn't be possible
    // if all the id's are handled
    if( !info.name )
    {
        wxASSERT_MSG( 0, "Failed to find suitable microwave tool info" );
        return 0;
    }

    frame.SetToolID( info.toolId, wxCURSOR_PENCIL, info.name );

    ITEM_CREATOR moduleCreator = [this, &info] ( const TOOL_EVENT& aAddingEvent )
    {
        auto module = info.creatorFunc();

        // Module has been added in the legacy backend,
        // so we have to remove it before committing the change
        // @todo LEGACY
        if( module )
        {
            board()->Remove( module.get() );
        }

        return module;
    };

    doInteractiveItemPlacement( moduleCreator,  _( "Place microwave feature" ) );

    frame.SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

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

    auto inductorModule = std::unique_ptr<MODULE>(
            CreateMicrowaveInductor( pattern, &frame, errorMessage )
    );

    if( inductorModule )
    {
        // legacy mode tools add to the board
        // so remove it and add it back with the commit object
        // this has to happen, even if we don't end up storing the module
        // @todo LEGACY
        board()->Remove( inductorModule.get() );
    }

    // on any error, report if we can
    if ( !inductorModule || !errorMessage.IsEmpty() )
    {
        if ( !errorMessage.IsEmpty() )
        {
            DisplayError( &frame, errorMessage );
        }
    }
    else
    {
        // at this point, we can save the module
        frame.SetCurItem( inductorModule.get() );

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

    frame.SetToolID( ID_PCB_MUWAVE_TOOL_SELF_CMD, wxCURSOR_PENCIL, _( "Add Microwave Inductor" ) );

    Activate();

    TWO_POINT_GEOMETRY_MANAGER tpGeomMgr;

    CENTRELINE_RECT_ITEM previewRect( tpGeomMgr, inductorAreaAspect );

    previewRect.SetFillColor( inductorAreaFill );
    previewRect.SetStrokeColor( inductorAreaStroke );
    previewRect.SetLineWidth( inductorAreaStrokeWidth );

    bool originSet = false;

    controls.ShowCursor( true );
    controls.SetSnapping( true );

    view.Add( &previewRect );

    while( auto evt = Wait() )
    {
        VECTOR2I cursorPos = controls.GetCursorPosition();

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            // overriding action, or we're cancelling without
            // an in-progress preview area
            if ( evt->IsActivate() || !originSet )
            {
                break;
            }

            // had an in-progress area, so start again but don't
            // cancel the tool
            originSet = false;
            view.SetVisible( &previewRect, false );
            view.Update( &previewRect, KIGFX::GEOMETRY );
        }

        // A click or drag starts
        else if( !originSet &&
                ( evt->IsClick( BUT_LEFT ) || evt->IsDrag( BUT_LEFT ) ) )
        {
            tpGeomMgr.SetOrigin( cursorPos );
            tpGeomMgr.SetEnd( cursorPos );

            originSet = true;

            controls.CaptureCursor( true );
            controls.SetAutoPan( true );
        }

        // another click after origin set is the end
        // left up is also the end, as you'll only get that after a drag
        else if( originSet &&
                ( evt->IsClick( BUT_LEFT ) || evt->IsMouseUp( BUT_LEFT ) ) )
        {
            // second click, we're done:
            // delegate to the point-to-point inductor creator function
            createInductorBetween( tpGeomMgr.GetOrigin(), tpGeomMgr.GetEnd() );

            // start again if needed
            originSet = false;

            view.SetVisible( &previewRect, false );
            view.Update( &previewRect, KIGFX::GEOMETRY );
        }

        // any move or drag once the origin was set updates
        // the end point
        else if( originSet &&
                ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            tpGeomMgr.SetAngleSnap( evt->Modifier( MD_CTRL ) );
            tpGeomMgr.SetEnd( cursorPos );

            view.SetVisible( &previewRect, true );
            view.Update( &previewRect, KIGFX::GEOMETRY );
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu();
        }
    }

    view.Remove( &previewRect );

    frame.SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


void MICROWAVE_TOOL::SetTransitions()
{
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint, PCB_ACTIONS::microwaveCreateGap.MakeEvent() );
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint, PCB_ACTIONS::microwaveCreateStub.MakeEvent() );
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint, PCB_ACTIONS::microwaveCreateStubArc.MakeEvent() );
    Go( &MICROWAVE_TOOL::addMicrowaveFootprint, PCB_ACTIONS::microwaveCreateFunctionShape.MakeEvent() );

    Go( &MICROWAVE_TOOL::drawMicrowaveInductor, PCB_ACTIONS::microwaveCreateLine.MakeEvent() );
}

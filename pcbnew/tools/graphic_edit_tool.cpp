/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tools/graphic_edit_tool.h>

#include <board_commit.h>
#include <board.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/painter.h>
#include <geometry/circle.h>
#include <geometry/seg.h>
#include <pcb_shape.h>
#include <preview_items/construction_geom.h>
#include <render_settings.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/graphic_extend.h>
#include <tools/graphic_trim.h>
#include <tools/hover_picker.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_selection_tool.h>

#include <algorithm>
#include <cmath>
#include <limits>

/// How far aPointer is from the shape's outline.  A fill does not count as a hit, so a shape
/// drawn over another does not swallow it.
static double outlineProximity( const BOARD_ITEM& aItem, const VECTOR2I& aPointer )
{
    const PCB_SHAPE* shape = GraphicEditShape( &aItem );

    if( !shape )
        return std::numeric_limits<double>::infinity();

    switch( shape->GetShape() )
    {
    case SHAPE_T::SEGMENT: return SEG( shape->GetStart(), shape->GetEnd() ).Distance( aPointer );

    case SHAPE_T::CIRCLE:
        return std::abs( shape->GetCenter().Distance( aPointer ) - (double) shape->GetRadius() );

    case SHAPE_T::ARC:
    {
        SHAPE_ARC arc = GraphicEditArc( *shape );

        return arc.NearestPoint( aPointer ).Distance( aPointer );
    }

    case SHAPE_T::RECTANGLE:
    {
        std::vector<VECTOR2I> corners = shape->GetRectCorners();
        double                nearest = std::numeric_limits<double>::infinity();

        for( size_t i = 0; i < corners.size(); i++ )
            nearest = std::min( nearest, (double) SEG( corners[i], corners[( i + 1 ) % corners.size()] )
                                                         .Distance( aPointer ) );

        return nearest;
    }

    default: return std::numeric_limits<double>::infinity();
    }
}


static void applyGeometry( PCB_SHAPE& aShape, const GRAPHIC_EDIT_GEOMETRY& aGeometry )
{
    if( aShape.GetShape() != aGeometry.m_Shape )
    {
        // A shape that has been opened up has no inside left to fill.
        aShape.SetShape( aGeometry.m_Shape );
        aShape.SetFillMode( FILL_T::NO_FILL );
    }

    if( aGeometry.m_Shape == SHAPE_T::ARC )
    {
        aShape.SetArcGeometry( aGeometry.m_Start, aGeometry.m_Mid, aGeometry.m_End );
    }
    else
    {
        aShape.SetStart( aGeometry.m_Start );
        aShape.SetEnd( aGeometry.m_End );
    }
}


static KIGFX::CONSTRUCTION_GEOM::DRAWABLE drawable( const GRAPHIC_EDIT_GEOMETRY& aGeometry )
{
    if( aGeometry.m_Shape == SHAPE_T::ARC )
        return SHAPE_ARC( aGeometry.m_Start, aGeometry.m_Mid, aGeometry.m_End, 0 );

    // A circle carries its centre in m_Start and a point on the rim in m_End.
    if( aGeometry.m_Shape == SHAPE_T::CIRCLE )
        return CIRCLE( aGeometry.m_Start, aGeometry.m_Start.Distance( aGeometry.m_End ) );

    return SEG( aGeometry.m_Start, aGeometry.m_End );
}


/// aNoResult carries wording only the operation can supply.
static wxString refusalMessage( GRAPHIC_EDIT_REFUSAL aRefusal, const wxString& aNoResult )
{
    switch( aRefusal )
    {
    case GRAPHIC_EDIT_REFUSAL::AMBIGUOUS: return _( "More than one shape meets this one here." );
    case GRAPHIC_EDIT_REFUSAL::DEGENERATE:
        return _( "That shape is too small or too near a full circle to work with." );
    case GRAPHIC_EDIT_REFUSAL::LOCKED_SOURCE: return _( "That shape is locked." );
    default: return aNoResult;
    }
}


GRAPHIC_EDIT_TOOL::GRAPHIC_EDIT_TOOL() :
        PCB_TOOL_BASE( "pcbnew.GraphicEdit" )
{
}


bool GRAPHIC_EDIT_TOOL::Init()
{
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    // Menu entries live in EDIT_TOOL's Shape Modification submenu.

    return true;
}


int GRAPHIC_EDIT_TOOL::Extend( const TOOL_EVENT& aEvent )
{
    return runInteractive( aEvent,
                           { .m_NoResult = _( "There is nothing to reach in that direction." ),
                             .m_CommitDescription = _( "Extend Line or Arc" ),
                             .m_Accepts = IsGraphicExtendSource,
                             .m_PreviewLayer = LAYER_AUX_ITEMS,
                             .m_QueryBounds = GRAPHIC_EXTEND_PLANNER::QueryBounds,
                             .m_Plan = GRAPHIC_EXTEND_PLANNER::Plan } );
}


int GRAPHIC_EDIT_TOOL::Trim( const TOOL_EVENT& aEvent )
{
    return runInteractive( aEvent,
                           { .m_NoResult = _( "Nothing crosses the shape here." ),
                             .m_CommitDescription = _( "Trim Shape" ),
                             .m_Accepts = IsGraphicTrimSource,
                             .m_SuppressedSnaps = { SNAP_CANDIDATE_SUBTYPE::INTERSECTION,
                                                    SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT },
                             .m_PreviewLayer = LAYER_DRC_ERROR,
                             .m_QueryBounds = nullptr,
                             .m_Plan = GRAPHIC_TRIM_PLANNER::Plan } );
}


int GRAPHIC_EDIT_TOOL::runInteractive( const TOOL_EVENT& aEvent, const OPERATION& aOperation )
{
    PCB_PICKER_TOOL*         picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();
    KIGFX::CONSTRUCTION_GEOM preview;
    HOVER_PICKER             hover( m_toolMgr );
    bool                     done = false;

    // Nothing is picked up front, so a leftover selection would only be a distraction.
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    // Only bounds the extension ray.  A snapshot is enough if the board grows.
    const BOX2I worldBounds = board()->GetBoundingBox();

    // A locked shape is still hovered.  The planner refuses it by name, which tells the user
    // more than nothing lighting up.
    auto hoveredSource = [&]( const VECTOR2I& aPointer ) -> PCB_SHAPE*
    {
        auto accepts = [&]( BOARD_ITEM& aItem )
        {
            const PCB_SHAPE* shape = GraphicEditShape( &aItem );

            return shape && aOperation.m_Accepts( *shape );
        };

        return static_cast<PCB_SHAPE*>( hover.Pick( aPointer, accepts, outlineProximity ) );
    };

    auto collectBoundaries = [&]( const BOX2I& aQueryBounds, const PCB_SHAPE& aSource )
    {
        std::set<const BOARD_ITEM*>    seen;
        std::vector<const BOARD_ITEM*> boundaries;
        const PCB_LAYER_ID             layer = aSource.GetLayer();

        // Selectable() is asked for visibility only, because footprint graphics are boundaries
        // the board editor will not select.  It is also the costliest test, so it goes last.
        view()->Query( aQueryBounds,
                       [&]( KIGFX::VIEW_ITEM* aViewItem )
                       {
                           if( !aViewItem->IsBOARD_ITEM() )
                               return true;

                           BOARD_ITEM* item = static_cast<BOARD_ITEM*>( aViewItem );

                           if( item != &aSource && item->GetLayer() == layer && GraphicEditShape( item )
                               && view()->IsVisible( item ) && m_selectionTool->Selectable( item, true )
                               && seen.insert( item ).second )
                           {
                               boundaries.push_back( item );
                           }

                           return true;
                       } );

        return boundaries;
    };

    auto resetPreview = [&]()
    {
        hover.ClearBrightening();
        preview.ClearDrawables();
        view()->Update( &preview );
    };

    // Returns the source it planned against, so the click handler can commit to the same one.
    auto updatePreview = [&]( const VECTOR2I& aPointer, GRAPHIC_EDIT_RESULT& aResult ) -> PCB_SHAPE*
    {
        PCB_SHAPE* source = hoveredSource( aPointer );

        preview.ClearDrawables();

        if( !source )
        {
            aResult = {};
            hover.ClearBrightening();
            view()->Update( &preview );
            return nullptr;
        }

        BOX2I queryBounds = source->GetBoundingBox();

        if( aOperation.m_QueryBounds )
        {
            BOX2I rayBounds = worldBounds;

            rayBounds.Merge( queryBounds );
            queryBounds = aOperation.m_QueryBounds( *source, aPointer, rayBounds );
        }

        if( !queryBounds.IsValid() )
        {
            aResult = {};
            hover.BrightenOnly( { source } );
            view()->Update( &preview );
            return source;
        }

        std::vector<const BOARD_ITEM*> candidates = collectBoundaries( queryBounds, *source );

        aResult = aOperation.m_Plan( *source, aPointer, candidates );

        // The shape under the pointer is worth marking whether or not it can be edited here.
        std::vector<BOARD_ITEM*> lit{ source };

        if( aResult )
        {
            const std::vector<GRAPHIC_EDIT_GEOMETRY>& shown =
                    aResult.m_Preview.empty() ? aResult.m_Geometry : aResult.m_Preview;

            for( const GRAPHIC_EDIT_GEOMETRY& geometry : shown )
                preview.AddDrawable( drawable( geometry ), false, 4 );

            for( const BOARD_ITEM* boundary : aResult.m_Boundaries )
                lit.push_back( const_cast<BOARD_ITEM*>( boundary ) );
        }

        hover.BrightenOnly( lit );
        view()->Update( &preview );
        return source;
    };

    KIGFX::RENDER_SETTINGS* settings = view()->GetPainter()->GetSettings();
    KIGFX::COLOR4D          previewColor = settings->GetLayerColor( aOperation.m_PreviewLayer );

    // The borrowed colour is not always a good choice.  Compare with the background.
    if( view()->GetGAL()->GetClearColor().Distance( previewColor ) < 0.5 )
        previewColor.Invert();

    preview.SetColor( previewColor );

    Activate();

    // The selection tool arms its disambiguation on button-down unless a tool owns the stack.
    // Without this the committing click also selects, which cancels the picker.
    frame()->PushTool( aEvent );

    view()->Add( &preview );
    picker->SetCursor( KICURSOR::BULLSEYE );

    // Snapping on is what makes the picker honour the modifiers that turn it off again, so
    // <shift> and <ctrl> give a finer aim among crowded items.
    picker->SetSnapping( true );

    // Nothing here follows an extension, so the lines the snap system draws are just noise.
    picker->SetConstructionGeometry( false );
    picker->SetSuppressedSnaps( aOperation.m_SuppressedSnaps );
    picker->ClearHandlers();
    picker->SetMotionHandler(
            [&]( const VECTOR2D& aPointer )
            {
                GRAPHIC_EDIT_RESULT result;

                updatePreview( aPointer, result );
            } );
    picker->SetClickHandler(
            [&]( const VECTOR2D& aPointer )
            {
                GRAPHIC_EDIT_RESULT result;
                PCB_SHAPE*          source = updatePreview( aPointer, result );

                if( !source )
                    return true;

                if( !result )
                {
                    frame()->ShowInfoBarError( refusalMessage( result.m_Refusal, aOperation.m_NoResult ) );
                    return true;
                }

                // The commit may free the source, so let go of it first.
                hover.ClearBrightening();
                preview.ClearDrawables();
                view()->Update( &preview );

                BOARD_COMMIT commit( this );

                if( result.m_Geometry.empty() )
                {
                    commit.Remove( source );
                }
                else
                {
                    std::vector<PCB_SHAPE*> pieces;

                    // Duplicate off the untouched source, before its own geometry is replaced.
                    for( size_t i = 1; i < result.m_Geometry.size(); i++ )
                    {
                        PCB_SHAPE* piece = static_cast<PCB_SHAPE*>( source->Duplicate( true, &commit ) );

                        piece->ClearSelected();
                        pieces.push_back( piece );
                    }

                    commit.Modify( source );
                    applyGeometry( *source, result.m_Geometry.front() );

                    for( size_t i = 0; i < pieces.size(); i++ )
                    {
                        applyGeometry( *pieces[i], result.m_Geometry[i + 1] );
                        commit.Add( pieces[i] );
                    }
                }

                commit.Push( aOperation.m_CommitDescription );

                updatePreview( aPointer, result );
                return true;
            } );
    picker->SetCancelHandler( resetPreview );
    picker->SetFinalizeHandler(
            [&]( const int& )
            {
                resetPreview();
                done = true;
            } );
    m_toolMgr->RunAction( ACTIONS::pickerSubTool );

    while( !done )
    {
        TOOL_EVENT* event = Wait();

        if( !event )
            break;

        // Nothing is cached across events.  Model changes and undo need no handling.  The next
        // motion queries the board as it stands.
        event->SetPassEvent();
    }

    picker->ClearHandlers();
    resetPreview();
    view()->Remove( &preview );
    frame()->PopTool( aEvent );
    return 0;
}


void GRAPHIC_EDIT_TOOL::setTransitions()
{
    Go( &GRAPHIC_EDIT_TOOL::Extend, PCB_ACTIONS::extendGraphic.MakeEvent() );
    Go( &GRAPHIC_EDIT_TOOL::Trim, PCB_ACTIONS::trimGraphic.MakeEvent() );
}

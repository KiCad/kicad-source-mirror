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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "block_drag_capability.h"

#include <algorithm>
#include <functional>
#include <unordered_set>

#include <board.h>
#include <footprint.h>
#include <pcb_base_frame.h>
#include <pcbnew_settings.h>
#include <pcb_generator.h>
#include <generators/pcb_tuning_pattern.h>
#include <generators/pcb_via_stack.h>
#include <generators/pcb_via_stitch.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <constraints/constraint_builder.h>
#include <constraints/pcb_constraint.h>


bool BlockDragAnchorEditingEnabled( const PCB_BASE_FRAME* aFrame )
{
    PCBNEW_SETTINGS* cfg = aFrame ? aFrame->GetPcbNewSettings() : nullptr;

    return cfg && cfg->m_AreaAnchorPointSelection;
}


bool BlockDragIsConstraintMember( const BOARD_ITEM& aItem )
{
    const BOARD* board = aItem.GetBoard();

    if( !board )
        return false;

    for( const PCB_CONSTRAINT* constraint : CollectResolvedConstraints( board ) )
    {
        for( const CONSTRAINT_MEMBER& member : constraint->GetMembers() )
        {
            if( member.m_item == aItem.m_Uuid )
                return true;
        }
    }

    return false;
}


BLOCK_DRAG_CAPABILITY BlockDragCapability( const BOARD_ITEM& aItem )
{
    const BOARD* board = aItem.GetBoard();
    bool         footprintHolder = board && board->IsFootprintHolder();

    switch( aItem.Type() )
    {
    case PCB_TRACE_T:
    case PCB_ARC_T:
    case PCB_VIA_T:
    case PCB_FOOTPRINT_T: return footprintHolder ? BLOCK_DRAG_CAPABILITY::UNSUPPORTED : BLOCK_DRAG_CAPABILITY::ROUTER;

    case PCB_PAD_T: return footprintHolder ? BLOCK_DRAG_CAPABILITY::RIGID : BLOCK_DRAG_CAPABILITY::ROUTER;

    case PCB_ZONE_T:
        // A zone inside a footprint belongs to that footprint. Reshaping it from the
        // board editor would edit the footprint from outside its own editor.
        if( !footprintHolder && aItem.GetParentFootprint() )
            return BLOCK_DRAG_CAPABILITY::RIGID;

        return BLOCK_DRAG_CAPABILITY::DEFORMABLE;

    case PCB_SHAPE_T:
    {
        // Same reason as a zone inside a footprint.
        if( !footprintHolder && aItem.GetParentFootprint() )
            return BLOCK_DRAG_CAPABILITY::RIGID;

        switch( static_cast<const PCB_SHAPE&>( aItem ).GetShape() )
        {
        case SHAPE_T::SEGMENT:
        case SHAPE_T::POLY:
        case SHAPE_T::RECTANGLE:
        case SHAPE_T::ELLIPSE:
        case SHAPE_T::ELLIPSE_ARC: return BLOCK_DRAG_CAPABILITY::DEFORMABLE;

        case SHAPE_T::ARC:
        case SHAPE_T::CIRCLE:
        case SHAPE_T::BEZIER: return BLOCK_DRAG_CAPABILITY::RIGID;

        default: return BLOCK_DRAG_CAPABILITY::UNSUPPORTED;
        }
    }

    case PCB_GENERATOR_T:
    {
        // Safe to carry only when Move() takes every member along. A via stitch is an
        // outline like a zone's. Its vias are re-placed by the regeneration that follows
        // the drag, so it can be reshaped.
        wxString genType = static_cast<const PCB_GENERATOR&>( aItem ).GetGeneratorType();

        if( genType == PCB_VIA_STITCH::GENERATOR_TYPE )
            return BLOCK_DRAG_CAPABILITY::DEFORMABLE;

        if( genType == PCB_TUNING_PATTERN::GENERATOR_TYPE || genType == PCB_VIA_STACK::GENERATOR_TYPE )
            return BLOCK_DRAG_CAPABILITY::RIGID;

        return BLOCK_DRAG_CAPABILITY::UNSUPPORTED;
    }

    case PCB_TABLECELL_T:
        // A cell's rectangle belongs to the table, which lays its cells out. Carrying the cell
        // on its own takes it out of the table, and the move tool refuses a lone cell for the
        // same reason, so say so instead
        return BLOCK_DRAG_CAPABILITY::UNSUPPORTED;

    case PCB_TEXT_T:
    case PCB_FIELD_T:
    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_REFERENCE_IMAGE_T:
    case PCB_BARCODE_T:
    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_LEADER_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_TARGET_T:
    case PCB_POINT_T:
    case PCB_GRID_ITEM_T:
    case PCB_DRILL_CHART_T:
    case PCB_DRILL_MAP_T: return BLOCK_DRAG_CAPABILITY::RIGID;

    default: return BLOCK_DRAG_CAPABILITY::UNSUPPORTED;
    }
}


std::vector<BOARD_ITEM*> BlockDragExpandSelection( const std::vector<BOARD_ITEM*>& aSelection )
{
    std::vector<BOARD_ITEM*>              expanded;
    std::unordered_set<const BOARD_ITEM*> seen;

    std::function<void( BOARD_ITEM* )> add = [&]( BOARD_ITEM* aItem )
    {
        if( !aItem || !seen.insert( aItem ).second )
            return;

        // A DRC marker is not board content, so there is nothing here to edit.
        if( aItem->Type() == PCB_MARKER_T )
            return;

        if( aItem->Type() == PCB_GROUP_T )
        {
            for( BOARD_ITEM* member : static_cast<PCB_GROUP*>( aItem )->GetBoardItems() )
            {
                add( member );
            }

            return;
        }

        expanded.push_back( aItem );
    };

    for( BOARD_ITEM* item : aSelection )
        add( item );

    // A footprint moves its own children, so a child selected alongside its parent would
    // take the displacement once from the parent and once from itself
    std::erase_if( expanded,
                   [&]( const BOARD_ITEM* aItem )
                   {
                       FOOTPRINT* parent = aItem->GetParentFootprint();

                       return parent && seen.count( parent );
                   } );

    return expanded;
}

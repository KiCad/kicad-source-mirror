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

#include "block_drag_pick.h"

#include <board_item.h>
#include <geometry/shape_line_chain.h>
#include <pcb_generator.h>
#include <pcb_shape.h>
#include <tool/edit_points.h>
#include <tools/block_drag_capability.h>


bool BlockDragHandlePickable( const EDIT_POINTS& aCollection, const EDIT_POINT& aPoint )
{
    const EDA_ITEM* parent = aCollection.GetParent();

    if( !parent || !parent->IsBOARD_ITEM() )
        return false;

    const BOARD_ITEM& item = static_cast<const BOARD_ITEM&>( *parent );

    if( BlockDragCapability( item ) != BLOCK_DRAG_CAPABILITY::DEFORMABLE )
        return false;

    // A block drag displaces the handles it picked and then asks the behavior to rebuild the
    // item once per handle. A handle that rebuilds the whole shape reads the shape as the
    // other handles have already left it, so picking both compounds
    if( aPoint.DrivesWholeShape() )
        return false;

    if( dynamic_cast<const EDIT_LINE*>( &aPoint ) )
    {
        if( item.Type() == PCB_ZONE_T || dynamic_cast<const PCB_GENERATOR_POLY*>( &item ) )
            return true;

        return item.Type() == PCB_SHAPE_T && static_cast<const PCB_SHAPE&>( item ).GetShape() == SHAPE_T::POLY;
    }

    return true;
}


void BlockDragApplyPick( std::set<PCB_HANDLE_KEY>& aPicked, const PCB_HANDLE_KEY& aKey, bool aAccumulate )
{
    if( !aAccumulate )
    {
        aPicked.clear();
        aPicked.insert( aKey );
        return;
    }

    if( !aPicked.erase( aKey ) )
        aPicked.insert( aKey );
}


SHAPE_LINE_CHAIN BlockDragPickRect( const VECTOR2I& aFirst, const VECTOR2I& aSecond )
{
    return SHAPE_LINE_CHAIN( { aFirst, VECTOR2I( aSecond.x, aFirst.y ), aSecond, VECTOR2I( aFirst.x, aSecond.y ) },
                             true );
}


void BlockDragLassoPick( const std::vector<EDIT_POINTS*>& aCollections, const SHAPE_LINE_CHAIN& aLasso,
                         bool aAccumulate, std::set<PCB_HANDLE_KEY>& aPicked )
{
    if( !aAccumulate )
        aPicked.clear();

    if( aLasso.PointCount() < 3 )
        return;

    aLasso.GenerateBBoxCache();

    for( EDIT_POINTS* collection : aCollections )
    {
        for( unsigned ii = 0; ii < collection->PointsSize(); ++ii )
        {
            const EDIT_POINT& point = collection->Point( ii );

            if( !aLasso.PointInside( point.GetPosition(), 0, true ) && !aLasso.PointOnEdge( point.GetPosition() ) )
                continue;

            if( !BlockDragHandlePickable( *collection, point ) )
                continue;

            if( std::optional<PCB_HANDLE_KEY> key = PCB_HANDLE_KEY::Make( *collection, &point ) )
                aPicked.insert( *key );
        }
    }
}

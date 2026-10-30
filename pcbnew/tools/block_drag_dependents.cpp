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

#include "block_drag_dependents.h"

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <constraints/board_constraint_adapter.h>

#include <optional>
#include <set>


namespace
{

/// How a shape follows a moved joint, or nothing when it cannot follow. An arc
/// travels whole, since moving one end alone would leave its centre behind.
std::optional<BLOCK_DRAG_FOLLOW> followKind( const PCB_SHAPE& aShape )
{
    switch( aShape.GetShape() )
    {
    case SHAPE_T::SEGMENT: return BLOCK_DRAG_FOLLOW::STRETCH;
    case SHAPE_T::ARC: return BLOCK_DRAG_FOLLOW::WHOLE;
    default: return std::nullopt;
    }
}

} // namespace


BLOCK_DRAG_DEPENDENT_RESULT
BlockDragFindDependents( const std::vector<PCB_SHAPE*>&          aCandidates,
                         const std::vector<BLOCK_DRAG_ENDPOINT>& aMovedEndpoints, bool aOverrideLocks,
                         std::vector<BLOCK_DRAG_ENDPOINT>& aDependents )
{
    aDependents.clear();

    const int baseTol = pcbIUScale.mmToIU( 0.001 );
    int64_t   tolSq = (int64_t) baseTol * baseTol;

    auto withinTol = [&]( const VECTOR2I& aFirst, const VECTOR2I& aSecond )
    {
        return aFirst.SquaredDistance( aSecond ) <= tolSq;
    };

    // Grows as each carried shape hands its far end on.
    std::vector<BLOCK_DRAG_ENDPOINT> seeds( aMovedEndpoints.begin(), aMovedEndpoints.end() );

    // Already moving, so their ends cannot be matched again.
    std::set<const PCB_SHAPE*> carried;

    auto isMoved = [&]( const PCB_SHAPE* aShape, bool aIsStart )
    {
        for( const BLOCK_DRAG_ENDPOINT& moved : seeds )
        {
            if( moved.shape == aShape && moved.isStart == aIsStart )
                return true;
        }

        return false;
    };

    auto alreadyAccepted = [&]( const PCB_SHAPE* aShape, bool aIsStart )
    {
        for( const BLOCK_DRAG_ENDPOINT& dep : aDependents )
        {
            if( dep.shape == aShape && dep.isStart == aIsStart )
                return true;
        }

        return false;
    };

    // Indexed, because a carried shape appends its far end to the seeds as this runs.
    for( size_t si = 0; si < seeds.size(); ++si )
    {
        const BLOCK_DRAG_ENDPOINT        moved = seeds[si]; // by value, the vector may reallocate
        std::vector<BLOCK_DRAG_ENDPOINT> cluster;
        int                              tol = baseTol;

        // The board outline joins Edge.Cuts ends within its chaining epsilon, so match that
        if( BOARD* board = moved.shape->GetBoard(); board && moved.shape->GetLayer() == Edge_Cuts )
            tol = std::max( tol, board->GetOutlinesChainingEpsilon() );

        tolSq = (int64_t) tol * tol;

        for( PCB_SHAPE* candidate : aCandidates )
        {
            if( !candidate )
                continue;

            std::optional<BLOCK_DRAG_FOLLOW> follow = followKind( *candidate );

            if( !follow || carried.count( candidate ) )
                continue;

            if( candidate->GetLayer() != moved.shape->GetLayer() )
                continue;

            for( bool isStart : { true, false } )
            {
                VECTOR2I pos = isStart ? candidate->GetStart() : candidate->GetEnd();

                // Distance first, so the scan over moved endpoints only runs for the
                // handful of candidates that sit on one.
                if( !withinTol( pos, moved.position ) )
                    continue;

                if( !isMoved( candidate, isStart ) )
                    cluster.push_back( { candidate, isStart, pos, *follow } );
            }
        }

        // Every end meeting at this joint has to sit on every other one, or there is
        // no single shape to follow.
        bool ambiguous = false;

        for( size_t ii = 0; ii < cluster.size() && !ambiguous; ++ii )
        {
            for( size_t jj = ii + 1; jj < cluster.size(); ++jj )
            {
                if( !withinTol( cluster[ii].position, cluster[jj].position ) )
                {
                    ambiguous = true;
                    break;
                }
            }
        }

        if( ambiguous )
            continue;

        for( const BLOCK_DRAG_ENDPOINT& member : cluster )
        {
            if( alreadyAccepted( member.shape, member.isStart ) )
                continue;

            aDependents.push_back( member );

            if( member.follow != BLOCK_DRAG_FOLLOW::WHOLE )
                continue;

            // Mark it carried before seeding, or its far end could match it again.
            carried.insert( member.shape );

            bool     farIsStart = !member.isStart;
            VECTOR2I farPos = farIsStart ? member.shape->GetStart() : member.shape->GetEnd();

            seeds.push_back( { member.shape, farIsStart, farPos, BLOCK_DRAG_FOLLOW::WHOLE } );
        }
    }

    if( !aOverrideLocks )
    {
        for( const BLOCK_DRAG_ENDPOINT& dep : aDependents )
        {
            // The same lock test the selected items get, which also covers a locked parent
            if( ConstraintItemIsLocked( dep.shape ) )
            {
                aDependents.clear();
                return BLOCK_DRAG_DEPENDENT_RESULT::LOCKED;
            }
        }
    }

    return BLOCK_DRAG_DEPENDENT_RESULT::OK;
}

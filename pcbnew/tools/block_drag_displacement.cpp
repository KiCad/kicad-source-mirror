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

#include "block_drag_displacement.h"

#include <tool/edit_points.h>


static EDIT_POINT* resolveInCollections( const std::vector<EDIT_POINTS*>& aCollections, const PCB_HANDLE_KEY& aKey,
                                         EDIT_POINTS** aOwner = nullptr )
{
    for( EDIT_POINTS* points : aCollections )
    {
        if( EDIT_POINT* point = aKey.Resolve( *points ) )
        {
            if( aOwner )
                *aOwner = points;

            return point;
        }
    }

    return nullptr;
}


std::map<PCB_HANDLE_KEY, VECTOR2I> BlockDragMoveOrigins( const std::vector<EDIT_POINTS*>& aCollections,
                                                         const std::set<PCB_HANDLE_KEY>&  aPicked )
{
    std::map<PCB_HANDLE_KEY, VECTOR2I> origins;

    for( const PCB_HANDLE_KEY& key : aPicked )
    {
        EDIT_POINTS* owner = nullptr;
        EDIT_POINT*  point = resolveInCollections( aCollections, key, &owner );

        if( !point )
            return {};

        EDIT_LINE* line = dynamic_cast<EDIT_LINE*>( point );

        if( !line )
        {
            origins.emplace( key, point->GetPosition() );
            continue;
        }

        // The edge, which each frame drives through its own drag rule, plus its two
        // corners, which the frame places first so that rule starts from known points.
        origins.emplace( key, line->GetPosition() );

        for( EDIT_POINT* endpoint : { &line->GetOrigin(), &line->GetEnd() } )
        {
            std::optional<PCB_HANDLE_KEY> endpointKey = PCB_HANDLE_KEY::Make( *owner, endpoint );

            if( !endpointKey )
                return {};

            origins.emplace( *endpointKey, endpoint->GetPosition() );
        }
    }

    return origins;
}


bool BlockDragApplyDisplacement( const std::vector<EDIT_POINTS*>&          aCollections,
                                 const std::map<PCB_HANDLE_KEY, VECTOR2I>& aOrigins, const VECTOR2I& aDelta,
                                 const GRID_HELPER& aGrid )
{
    // Corners first, measured from where they started, so an edge begins every frame
    // from the same geometry and a long drag cannot drift.
    for( const auto& [key, origin] : aOrigins )
    {
        if( key.IsLine() )
            continue;

        EDIT_POINT* point = resolveInCollections( aCollections, key );

        if( !point )
            return false;

        point->SetPosition( origin + aDelta );
    }

    // Now let each picked edge follow its corners, which is what holds a zone edge
    // square to its neighbours. The corners already sit exactly where they belong, and
    // setting the edge's own position would nudge them by the rounding in a midpoint.
    for( const auto& [key, origin] : aOrigins )
    {
        if( !key.IsLine() )
            continue;

        EDIT_POINT* point = resolveInCollections( aCollections, key );

        if( !point )
            return false;

        point->ApplyRelation( aGrid );
    }

    return true;
}

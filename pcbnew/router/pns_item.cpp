/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <class_board.h>
#include <class_board_item.h>
#include <class_pad.h>
#include <class_track.h>    // For ::VIA
#include <pad_shapes.h>

#include "pns_item.h"
#include "pns_line.h"

typedef VECTOR2I::extended_type ecoord;

namespace PNS {

static int holeRadius( BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_PAD_T )
    {
        const D_PAD* pad = static_cast<const D_PAD*>( aItem );

        if( pad->GetDrillSize().x && pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
            return pad->GetDrillSize().x / 2;
        else
            return 0;
    }
    else if( aItem->Type() == PCB_VIA_T )
    {
        return static_cast<const ::VIA*>( aItem )->GetDrillValue() / 2;
    }

    return 0;
}


bool ITEM::CollideHoles( const ITEM* aOther, bool aNeedMTV, VECTOR2I& aMTV ) const
{
    BOARD_ITEM* a = Parent();
    BOARD_ITEM* b = aOther->Parent();

    if( !a || !b )
        return false;

    // Holes with identical locations are allowable
    if( a->GetPosition() == b->GetPosition() )
        return false;

    int radius_a = holeRadius( a );
    int radius_b = holeRadius( b );

    // Do both objects have holes?
    if( radius_a > 0 && radius_b > 0 )
    {
        int holeToHoleMin = a->GetBoard()->GetDesignSettings().m_HoleToHoleMin;

        ecoord min_dist = holeToHoleMin + radius_a + radius_b;
        ecoord min_dist_sq = min_dist * min_dist;

        const VECTOR2I delta = b->GetPosition() - a->GetPosition();

        ecoord dist_sq = delta.SquaredEuclideanNorm();

        if( dist_sq < min_dist_sq )
        {
            if( aNeedMTV )
                aMTV = delta.Resize( min_dist - sqrt( dist_sq ) + 3 );  // fixme: apparent rounding error

            return true;
        }
    }

    return false;
}


bool ITEM::collideSimple( const ITEM* aOther, int aClearance, bool aNeedMTV, VECTOR2I& aMTV,
                          bool aDifferentNetsOnly ) const
{
    // hole-to-hole is a mechanical constraint (broken drill bits) not an electrical one, so
    // it must be checked before checking aDifferentNetsOnly
    if( CollideHoles( aOther, aNeedMTV, aMTV ) )
        return true;

    // same nets? no collision!
    if( aDifferentNetsOnly && m_net == aOther->m_net && m_net >= 0 && aOther->m_net >= 0 )
        return false;

    // check if we are not on completely different layers first
    if( !m_layers.Overlaps( aOther->m_layers ) )
        return false;

    return Shape()->Collide( aOther->Shape(), aClearance, aMTV );
}


bool ITEM::Collide( const ITEM* aOther, int aClearance, bool aNeedMTV, VECTOR2I& aMTV,
                    bool aDifferentNetsOnly ) const
{
    if( collideSimple( aOther, aClearance, aNeedMTV, aMTV, aDifferentNetsOnly ) )
        return true;

    // special case for "head" line with a via attached at the end.
    if( aOther->m_kind == LINE_T )
    {
        const LINE* line = static_cast<const LINE*>( aOther );
        int clearance = aClearance - line->Width() / 2;

        if( line->EndsWithVia() )
            return collideSimple( &line->Via(), clearance, aNeedMTV, aMTV, aDifferentNetsOnly );
    }

    return false;
}


std::string ITEM::KindStr() const
{
    switch( m_kind )
    {
    case LINE_T:    return "line";
    case SEGMENT_T: return "segment";
    case VIA_T:     return "via";
    case JOINT_T:   return "joint";
    case SOLID_T:   return "solid";
    default:        return "unknown";
    }
}


ITEM::~ITEM()
{
}

}

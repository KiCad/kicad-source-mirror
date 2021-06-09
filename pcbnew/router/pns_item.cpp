/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pns_node.h"
#include "pns_item.h"
#include "pns_line.h"
#include "pns_router.h"

typedef VECTOR2I::extended_type ecoord;

namespace PNS {

bool ITEM::collideSimple( const ITEM* aOther, const NODE* aNode, bool aDifferentNetsOnly ) const
{
    const ROUTER_IFACE* iface = ROUTER::GetInstance()->GetInterface();
    const SHAPE*        shapeA = Shape();
    const SHAPE*        holeA = Hole();
    int                 lineWidthA = 0;
    const SHAPE*        shapeB = aOther->Shape();
    const SHAPE*        holeB = aOther->Hole();
    int                 lineWidthB = 0;

    // Sadly collision routines ignore SHAPE_POLY_LINE widths so we have to pass them in as part
    // of the clearance value.
    if( m_kind == LINE_T )
        lineWidthA = static_cast<const LINE*>( this )->Width() / 2;

    if( aOther->m_kind == LINE_T )
        lineWidthB = static_cast<const LINE*>( aOther )->Width() / 2;

    // same nets? no collision!
    if( aDifferentNetsOnly && m_net == aOther->m_net && m_net >= 0 && aOther->m_net >= 0 )
        return false;

    // check if we are not on completely different layers first
    if( !m_layers.Overlaps( aOther->m_layers ) )
        return false;

    if( holeA || holeB )
    {
        int holeClearance = aNode->GetHoleClearance( this, aOther );

        if( holeA && holeA->Collide( shapeB, holeClearance + lineWidthB ) )
        {
            Mark( Marker() | MK_HOLE );
            return true;
        }

        if( holeB && holeB->Collide( shapeA, holeClearance + lineWidthA ) )
        {
            aOther->Mark( aOther->Marker() | MK_HOLE );
            return true;
        }

        if( holeA && holeB )
        {
            int holeToHoleClearance = aNode->GetHoleToHoleClearance( this, aOther );

            if( holeA->Collide( holeB, holeToHoleClearance ) )
            {
                Mark( Marker() | MK_HOLE );
                aOther->Mark( aOther->Marker() | MK_HOLE );
                return true;
            }
        }
    }

    if( !aOther->Layers().IsMultilayer() && !iface->IsFlashedOnLayer( this, aOther->Layer()) )
        return false;

    if( !Layers().IsMultilayer() && !iface->IsFlashedOnLayer( aOther, Layer()) )
        return false;

    int clearance = aNode->GetClearance( this, aOther );
    return shapeA->Collide( shapeB, clearance + lineWidthA + lineWidthB );
}


bool ITEM::Collide( const ITEM* aOther, const NODE* aNode, bool aDifferentNetsOnly ) const
{
    if( collideSimple( aOther, aNode, aDifferentNetsOnly ) )
        return true;

    // Special cases for "head" lines with vias attached at the end.  Note that this does not
    // support head-line-via to head-line-via collisions, but you can't route two independent
    // tracks at once so it shouldn't come up.

    if( m_kind == LINE_T )
    {
        const LINE* line = static_cast<const LINE*>( this );

        if( line->EndsWithVia() && line->Via().collideSimple( aOther, aNode, aDifferentNetsOnly ) )
            return true;
    }

    if( aOther->m_kind == LINE_T )
    {
        const LINE* line = static_cast<const LINE*>( aOther );

        if( line->EndsWithVia() && line->Via().collideSimple( this, aNode, aDifferentNetsOnly ) )
            return true;
    }

    return false;
}


std::string ITEM::KindStr() const
{
    switch( m_kind )
    {
    case ARC_T:       return "arc";
    case LINE_T:      return "line";
    case SEGMENT_T:   return "segment";
    case VIA_T:       return "via";
    case JOINT_T:     return "joint";
    case SOLID_T:     return "solid";
    case DIFF_PAIR_T: return "diff-pair";
    default:          return "unknown";
    }
}


ITEM::~ITEM()
{
}

}

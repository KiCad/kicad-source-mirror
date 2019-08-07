/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

typedef VECTOR2I::extended_type ecoord;

namespace PNS {

bool ITEM::collideSimple( const ITEM* aOther, int aClearance, bool aNeedMTV, VECTOR2I* aMTV,
                          const NODE* aParentNode, bool aDifferentNetsOnly ) const
{
    // same nets? no collision!
    if( aDifferentNetsOnly && m_net == aOther->m_net && m_net >= 0 && aOther->m_net >= 0 )
        return false;

    // check if we are not on completely different layers first
    if( !m_layers.Overlaps( aOther->m_layers ) )
        return false;

    if( aNeedMTV )
        return Shape()->Collide( aOther->Shape(), aClearance, *aMTV );
    else
        return Shape()->Collide( aOther->Shape(), aClearance );
}


bool ITEM::Collide( const ITEM* aOther, int aClearance, bool aNeedMTV, VECTOR2I* aMTV,
                    const NODE* aParentNode, bool aDifferentNetsOnly ) const
{
    if( collideSimple( aOther, aClearance, aNeedMTV, aMTV, aParentNode, aDifferentNetsOnly ) )
        return true;

    // special case for "head" line with a via attached at the end.
    if( aOther->m_kind == LINE_T )
    {
        const LINE* line = static_cast<const LINE*>( aOther );
        int clearance = aClearance - line->Width() / 2;

        if( line->EndsWithVia() )
        {
            return collideSimple( &line->Via(), clearance, aNeedMTV, aMTV, aParentNode,
                                  aDifferentNetsOnly );
        }
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

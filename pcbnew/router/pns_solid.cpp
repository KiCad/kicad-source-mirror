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

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_compound.h>

#include "pns_router.h"
#include "pns_solid.h"
#include "pns_utils.h"

namespace PNS {

static const SHAPE_LINE_CHAIN buildHullForPrimitiveShape( const SHAPE* aShape, int aClearance, int aWalkaroundThickness )
{
    int cl = aClearance + ( aWalkaroundThickness + 1 )/ 2;

    switch( aShape->Type() )
    {
    case SH_RECT:
    {
        const SHAPE_RECT* rect = static_cast<const SHAPE_RECT*>( aShape );
        return OctagonalHull( rect->GetPosition(), rect->GetSize(), cl + 1, 0.2 * cl );
    }

    case SH_CIRCLE:
    {
        const SHAPE_CIRCLE* circle = static_cast<const SHAPE_CIRCLE*>( aShape );
        int r = circle->GetRadius();
        return OctagonalHull( circle->GetCenter() - VECTOR2I( r, r ), VECTOR2I( 2 * r, 2 * r ),
                              cl + 1, 0.52 * ( r + cl ) );
    }

    case SH_SEGMENT:
    {
        const SHAPE_SEGMENT* seg = static_cast<const SHAPE_SEGMENT*>( aShape );
        return SegmentHull( *seg, aClearance, aWalkaroundThickness );
    }

    case SH_SIMPLE:
    {
        const SHAPE_SIMPLE* convex = static_cast<const SHAPE_SIMPLE*>( aShape );

        return ConvexHull( *convex, cl );
    }
    default:
    {
        wxLogError("Unsupported hull shape: %d", aShape->Type() );
        break;
    }
    }

    return SHAPE_LINE_CHAIN();
}


const SHAPE_LINE_CHAIN SOLID::Hull( int aClearance, int aWalkaroundThickness, int aLayer ) const
{
    SHAPE* shape = m_shape;

    if( !ROUTER::GetInstance()->GetInterface()->IsOnLayer( this, aLayer ) )
    {
        /// The alternate shape is defined for THT pads.  If we don't have an alternate shape
        /// then the solid shape does not exist on this layer
        if( !m_alternateShape )
            return SHAPE_LINE_CHAIN();

        shape = m_alternateShape;
    }

    if( shape->Type() == SH_COMPOUND )
    {
        auto cmpnd = static_cast<SHAPE_COMPOUND*>( shape );
        if ( cmpnd->Shapes().size() == 1 )
        {
            return buildHullForPrimitiveShape( cmpnd->Shapes()[0], aClearance, aWalkaroundThickness );
        }
        else
        {
            // fixme - shouldn't happen but one day we should move TransformShapeWithClearanceToPolygon()
            // to the Geometry Library
            return SHAPE_LINE_CHAIN();
        }
    }
    else
    {
        return buildHullForPrimitiveShape( shape, aClearance, aWalkaroundThickness );
    }
        
    return SHAPE_LINE_CHAIN();
}


ITEM* SOLID::Clone() const
{
    ITEM* solid = new SOLID( *this );
    return solid;
}

void SOLID::SetPos( const VECTOR2I& aCenter )
{
    auto delta = aCenter - m_pos;

    if( m_shape )
        m_shape->Move( delta );

    m_pos = aCenter;
}


}

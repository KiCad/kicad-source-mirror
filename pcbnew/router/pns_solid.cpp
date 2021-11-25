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

#include <wx/log.h>

#include "pns_router.h"
#include "pns_solid.h"
#include "pns_utils.h"

namespace PNS {

static const SHAPE_LINE_CHAIN buildHullForPrimitiveShape( const SHAPE* aShape, int aClearance,
                                                          int aWalkaroundThickness )
{
    int cl = aClearance + ( aWalkaroundThickness + 1 )/ 2;

    switch( aShape->Type() )
    {
    case SH_RECT:
    {
        const SHAPE_RECT* rect = static_cast<const SHAPE_RECT*>( aShape );
        return OctagonalHull( rect->GetPosition(),
                              rect->GetSize(),
                              cl + 1,
                              0 );
    }

    case SH_CIRCLE:
    {
        const SHAPE_CIRCLE* circle = static_cast<const SHAPE_CIRCLE*>( aShape );
        int r = circle->GetRadius();
        return OctagonalHull( circle->GetCenter() - VECTOR2I( r, r ),
                              VECTOR2I( 2 * r, 2 * r ),
                              cl + 1,
                              2.0 * ( 1.0 - M_SQRT1_2 ) * ( r + cl ) );
    }

    case SH_SEGMENT:
    {
        const SHAPE_SEGMENT* seg = static_cast<const SHAPE_SEGMENT*>( aShape );
        return SegmentHull( *seg, aClearance, aWalkaroundThickness );
    }

    case SH_ARC:
    {
        const SHAPE_ARC* arc = static_cast<const SHAPE_ARC*>( aShape );
        return ArcHull( *arc, aClearance, aWalkaroundThickness );
    }

    case SH_SIMPLE:
    {
        const SHAPE_SIMPLE* convex = static_cast<const SHAPE_SIMPLE*>( aShape );

        return ConvexHull( *convex, cl );
    }
    default:
    {
        wxFAIL_MSG( wxString::Format( "Unsupported hull shape: %d (%s).",
                                      aShape->Type(),
                                      SHAPE_TYPE_asString( aShape->Type() ) ) );
        break;
    }
    }

    return SHAPE_LINE_CHAIN();
}


const SHAPE_LINE_CHAIN SOLID::Hull( int aClearance, int aWalkaroundThickness, int aLayer ) const
{
    if( !ROUTER::GetInstance()->GetInterface()->IsFlashedOnLayer( this, aLayer ) )
        return HoleHull( aClearance, aWalkaroundThickness, aLayer );

    if( !m_shape )
        return SHAPE_LINE_CHAIN();

    if( m_shape->Type() == SH_COMPOUND )
    {
        SHAPE_COMPOUND* cmpnd = static_cast<SHAPE_COMPOUND*>( m_shape );

        if ( cmpnd->Shapes().size() == 1 )
        {
            return buildHullForPrimitiveShape( cmpnd->Shapes()[0], aClearance,
                                               aWalkaroundThickness );
        }
        else
        {
            // fixme - shouldn't happen but one day we should move
            // TransformShapeWithClearanceToPolygon() to the Geometry Library
            return SHAPE_LINE_CHAIN();
        }
    }
    else
    {
        return buildHullForPrimitiveShape( m_shape, aClearance, aWalkaroundThickness );
    }
}


const SHAPE_LINE_CHAIN SOLID::HoleHull( int aClearance, int aWalkaroundThickness, int aLayer ) const
{
    if( !m_hole )
        return SHAPE_LINE_CHAIN();

    if( m_hole->Type() == SH_COMPOUND )
    {
        SHAPE_COMPOUND* cmpnd = static_cast<SHAPE_COMPOUND*>( m_hole );

        if ( cmpnd->Shapes().size() == 1 )
        {
            return buildHullForPrimitiveShape( cmpnd->Shapes()[0], aClearance,
                                               aWalkaroundThickness );
        }
        else
        {
            // fixme - shouldn't happen but one day we should move
            // TransformShapeWithClearanceToPolygon() to the Geometry Library
            return SHAPE_LINE_CHAIN();
        }
    }
    else
    {
        return buildHullForPrimitiveShape( m_hole, aClearance, aWalkaroundThickness );
    }
}


ITEM* SOLID::Clone() const
{
    ITEM* solid = new SOLID( *this );
    return solid;
}

void SOLID::SetPos( const VECTOR2I& aCenter )
{
    VECTOR2I delta = aCenter - m_pos;

    if( m_shape )
        m_shape->Move( delta );

    if( m_hole )
        m_hole->Move( delta );

    m_pos = aCenter;
}


}

/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <geometry/shape_circle.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_poly_set.h>

#include <wx/log.h>

#include "pns_router.h"
#include "pns_solid.h"
#include "pns_utils.h"

namespace PNS {


const SHAPE_LINE_CHAIN SOLID::Hull( int aClearance, int aWalkaroundThickness, int aLayer ) const
{
    if( !m_shape )
        return SHAPE_LINE_CHAIN();

    if( m_shape->Type() == SH_COMPOUND )
    {
        SHAPE_COMPOUND* cmpnd = static_cast<SHAPE_COMPOUND*>( m_shape );

        if ( cmpnd->Shapes().size() == 1 )
        {
            return BuildHullForPrimitiveShape( cmpnd->Shapes()[0], aClearance,
                                               aWalkaroundThickness );
        }
        else
        {
            SHAPE_POLY_SET hullSet;

            for( SHAPE* shape : cmpnd->Shapes() )
            {
                hullSet.AddOutline( BuildHullForPrimitiveShape( shape, aClearance,
                                                                aWalkaroundThickness ) );
            }

            hullSet.Simplify();
            return hullSet.Outline( 0 );
        }
    }
    else
    {
        return BuildHullForPrimitiveShape( m_shape, aClearance, aWalkaroundThickness );
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


VECTOR2I SOLID::Anchor( int aN ) const
{
    return m_anchorPoints.empty() ? m_pos : m_anchorPoints[aN];
}

 int SOLID::AnchorCount() const
{
    return m_anchorPoints.empty() ? 1 : m_anchorPoints.size();
}


}

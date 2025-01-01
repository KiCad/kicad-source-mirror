/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include "pns_item.h"
#include "pns_hole.h"
#include "pns_node.h"
#include "pns_utils.h"

#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_poly_set.h>

namespace PNS
{

HOLE::~HOLE()
{
    delete m_holeShape;
}


HOLE* HOLE::Clone() const
{
    HOLE* h = new HOLE( m_holeShape->Clone() );

    h->SetLayers( Layers() );
    h->SetOwner( nullptr );

    h->m_rank = m_rank;
    h->m_marker = m_marker;
    h->m_parent = m_parent;
    h->m_isVirtual = m_isVirtual;

    return h;
}


const SHAPE_LINE_CHAIN HOLE::Hull( int aClearance, int aWalkaroundThickness, int aLayer ) const
{
    if( !m_holeShape )
        return SHAPE_LINE_CHAIN();

    if( m_holeShape->Type() == SH_CIRCLE )
    {
        auto cir = static_cast<SHAPE_CIRCLE*>( m_holeShape );
        int  cl = ( aClearance + aWalkaroundThickness / 2 );
        int  width = cir->GetRadius() * 2;

        // Chamfer = width * ( 1 - sqrt(2)/2 ) for equilateral octagon
        return OctagonalHull( cir->GetCenter() - VECTOR2I( width / 2, width / 2 ),
                              VECTOR2I( width, width ), cl,
                              ( 2 * cl + width ) * ( 1.0 - M_SQRT1_2 ) );
    }
    else if( m_holeShape->Type() == SH_COMPOUND )
    {
        SHAPE_COMPOUND* cmpnd = static_cast<SHAPE_COMPOUND*>( m_holeShape );

        if( cmpnd->Shapes().size() == 1 )
        {
            return BuildHullForPrimitiveShape( cmpnd->Shapes()[0], aClearance,
                                               aWalkaroundThickness );
        }
        else
        {
            SHAPE_POLY_SET hullSet;

            for( SHAPE* shape : cmpnd->Shapes() )
            {
                hullSet.AddOutline(
                        BuildHullForPrimitiveShape( shape, aClearance, aWalkaroundThickness ) );
            }

            hullSet.Simplify();
            return hullSet.Outline( 0 );
        }
    }
    else
    {
        return BuildHullForPrimitiveShape( m_holeShape, aClearance, aWalkaroundThickness );
    }
}


int HOLE::Radius() const
{
    assert( m_holeShape->Type() == SH_CIRCLE );

    return static_cast<const SHAPE_CIRCLE*>( m_holeShape )->GetRadius();
}


void HOLE::SetCenter( const VECTOR2I& aCenter )
{
    assert( m_holeShape->Type() == SH_CIRCLE );
    static_cast<SHAPE_CIRCLE*>( m_holeShape )->SetCenter( aCenter );
}


void HOLE::SetRadius( int aRadius )
{
    assert( m_holeShape->Type() == SH_CIRCLE );
    static_cast<SHAPE_CIRCLE*>( m_holeShape )->SetRadius( aRadius );
}


void HOLE::Move( const VECTOR2I& delta )
{
    m_holeShape->Move( delta );
}


HOLE* HOLE::MakeCircularHole( const VECTOR2I& pos, int radius, PNS_LAYER_RANGE aLayers )
{
    SHAPE_CIRCLE* circle = new SHAPE_CIRCLE( pos, radius );
    HOLE*         hole = new HOLE( circle );
    hole->SetLayers( aLayers );
    return hole;
}

}; // namespace PNS

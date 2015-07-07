/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#include "pns_via.h"
#include "pns_node.h"
#include "pns_utils.h"
#include "pns_router.h"

#include <geometry/shape_rect.h>

bool PNS_VIA::PushoutForce( PNS_NODE* aNode, const VECTOR2I& aDirection, VECTOR2I& aForce,
                            bool aSolidsOnly, int aMaxIterations )
{
    int iter = 0;
    PNS_VIA mv( *this );
    VECTOR2I force, totalForce, force2;

    while( iter < aMaxIterations )
    {
        PNS_NODE::OPT_OBSTACLE obs = aNode->CheckColliding( &mv,
                aSolidsOnly ? PNS_ITEM::SOLID : PNS_ITEM::ANY );

        if( !obs )
            break;

        int clearance = aNode->GetClearance( obs->m_item, &mv );

        if( iter > aMaxIterations / 2 )
        {
            VECTOR2I l = aDirection.Resize( m_diameter / 2 );
            totalForce += l;
            mv.SetPos( mv.Pos() + l );
        }

        bool col = CollideShapes( obs->m_item->Shape(), mv.Shape(), clearance, true, force2 );

        if( col ) {
            totalForce += force2;
            mv.SetPos( mv.Pos() + force2 );
        }

        iter++;
    }

    if( iter == aMaxIterations )
        return false;

    aForce = totalForce;

    return true;
}


const SHAPE_LINE_CHAIN PNS_VIA::Hull( int aClearance, int aWalkaroundThickness ) const
{
    int cl = ( aClearance + aWalkaroundThickness / 2 );

    return OctagonalHull( m_pos -
            VECTOR2I( m_diameter / 2, m_diameter / 2 ), VECTOR2I( m_diameter, m_diameter ),
            cl + 1, ( 2 * cl + m_diameter ) * 0.26 );
}


PNS_VIA* PNS_VIA::Clone ( ) const
{
    PNS_VIA* v = new PNS_VIA();

    v->SetNet( Net() );
    v->SetLayers( Layers() );
    v->m_pos = m_pos;
    v->m_diameter = m_diameter;
    v->m_drill = m_drill;
    v->m_owner = NULL;
    v->m_shape = SHAPE_CIRCLE( m_pos, m_diameter / 2 );
    v->m_rank = m_rank;
    v->m_marker = m_marker;
    v->m_viaType = m_viaType;

    return v;
}


OPT_BOX2I PNS_VIA::ChangedArea( const PNS_VIA* aOther ) const
{
    if ( aOther->Pos() != Pos() )
    {
        BOX2I tmp = Shape()->BBox();
        tmp.Merge( aOther->Shape()->BBox() );
        return tmp;
    }

    return OPT_BOX2I();
}

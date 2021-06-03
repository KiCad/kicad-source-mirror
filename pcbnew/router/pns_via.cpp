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

#include "pns_via.h"
#include "pns_node.h"
#include "pns_utils.h"
#include "pns_router.h"

#include <geometry/shape_rect.h>
#include <math/box2.h>

namespace PNS {

bool VIA::PushoutForce( NODE* aNode, const VECTOR2I& aDirection, VECTOR2I& aForce,
                            bool aSolidsOnly, int aMaxIterations )
{
    int iter = 0;
    VIA mv( *this );
    VECTOR2I force, totalForce;

    while( iter < aMaxIterations )
    {
        NODE::OPT_OBSTACLE obs = aNode->CheckColliding( &mv, aSolidsOnly ? ITEM::SOLID_T
                                                                         : ITEM::ANY_T );

        if( !obs )
            break;

        int clearance = aNode->GetClearance( obs->m_item, &mv );

        if( iter > aMaxIterations / 2 )
        {
            VECTOR2I l = aDirection.Resize( m_diameter / 2 );
            totalForce += l;
            mv.SetPos( mv.Pos() + l );
        }

        if( obs->m_item->Shape()->Collide( mv.Shape(), clearance, &force ) )
        {
            totalForce += force;
            mv.SetPos( mv.Pos() + force );
        }

        iter++;
    }

    if( iter == aMaxIterations )
        return false;

    aForce = totalForce;

    return true;
}


const SHAPE_LINE_CHAIN VIA::Hull( int aClearance, int aWalkaroundThickness, int aLayer ) const
{
    int cl = ( aClearance + aWalkaroundThickness / 2 );
    int width = m_diameter;

    if( !ROUTER::GetInstance()->GetInterface()->IsFlashedOnLayer( this, aLayer ) )
        width = m_drill;

    return OctagonalHull( m_pos -
        VECTOR2I( width / 2, width / 2 ), VECTOR2I( width, width ),
        cl + 1, ( 2 * cl + width ) * 0.26 );
}


VIA* VIA::Clone() const
{
    VIA* v = new VIA();

    v->SetNet( Net() );
    v->SetLayers( Layers() );
    v->m_pos = m_pos;
    v->m_diameter = m_diameter;
    v->m_drill = m_drill;
    v->m_shape = SHAPE_CIRCLE( m_pos, m_diameter / 2 );
    v->m_rank = m_rank;
    v->m_marker = m_marker;
    v->m_viaType = m_viaType;
    v->m_parent = m_parent;
    v->m_isFree = m_isFree;
    v->m_isVirtual = m_isVirtual;

    return v;
}


OPT_BOX2I VIA::ChangedArea( const VIA* aOther ) const
{
    if ( aOther->Pos() != Pos() )
    {
        BOX2I tmp = Shape()->BBox();
        tmp.Merge( aOther->Shape()->BBox() );
        return tmp;
    }

    return OPT_BOX2I();
}

const VIA_HANDLE VIA::MakeHandle() const
{
    VIA_HANDLE h;
    h.pos = Pos();
    h.layers = Layers();
    h.net = Net();
    h.valid = true;
    return h;
}

}

/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "pns_debug_decorator.h"

#include <geometry/shape_rect.h>
#include <math/box2.h>

namespace PNS {

bool VIA::PushoutForce( NODE* aNode, const ITEM* aOther, VECTOR2I& aForce )
{
    int      clearance = aNode->GetClearance( this, aOther );
    VECTOR2I elementForces[4], force;
    size_t   nf = 0;

    aOther->Shape()->Collide( Shape(), clearance, &elementForces[nf++] );

    for( size_t i = 0; i < nf; i++ )
    {
        if( elementForces[i].SquaredEuclideanNorm() > force.SquaredEuclideanNorm() )
            force = elementForces[i];
    }

    aForce = force;

    return ( force != VECTOR2I( 0, 0 ) );
}

bool VIA::PushoutForce( NODE* aNode, const VECTOR2I& aDirection, VECTOR2I& aForce,
                        int aCollisionMask, int aMaxIterations )
{
    int      iter = 0;
    VIA      mv( *this );
    VECTOR2I totalForce;

    auto dbg = ROUTER::GetInstance()->GetInterface()->GetDebugDecorator();
    PNS_DBG( dbg, AddPoint, Pos(), YELLOW, 100000, wxString::Format( "via-force-init-pos, iter %d", aMaxIterations ) );

    while( iter < aMaxIterations )
    {
        NODE::OPT_OBSTACLE obs = aNode->CheckColliding( &mv, aCollisionMask );

        if( !obs )
            break;

        VECTOR2I force;
        bool     collFound = mv.PushoutForce( aNode, obs->m_item, force );

        if( !collFound )
        {
            if( obs )
            {
                // might happen (although rarely) that we see a collision, but the MTV
                // is zero... Assume force propagation has failed in such case.
                return false;
            }
            PNS_DBG( dbg, Message, wxString::Format( "no-coll %d", iter ) );
            break;
        }

        const int threshold = Diameter() / 4; // another stupid heuristic.
        const int forceMag = force.EuclideanNorm();

        // We've been through a lot of iterations already and our pushout force is still too big?
        // Perhaps the barycentric force goes in the wrong direction, let's try to move along
        // the 'lead' vector instead (usually backwards to the cursor)
        if( iter > aMaxIterations / 2 && forceMag > threshold )
        {
            VECTOR2I l = aDirection.Resize( threshold );
            totalForce += l;

            SHAPE_LINE_CHAIN ff;
            ff.Append( mv.Pos() );
            ff.Append( mv.Pos() + l );

            mv.SetPos( mv.Pos() + l );

            PNS_DBG( dbg, AddShape, &ff, YELLOW, 100000, "via-force-lead" );
        }
        else if( collFound ) // push along the minmum translation vector
        {
            // Limit the force magnitude to, say, 25% of the via diameter
            // This adds a few iterations for large areas (e.g. keepouts)
            // But makes the algorithm more predictable and less 'jumpy'
            if( forceMag > threshold )
            {
                force.Resize( threshold );
            }

            totalForce += force;

            SHAPE_LINE_CHAIN ff;
            ff.Append( mv.Pos() );
            ff.Append( mv.Pos() + force );

            mv.SetPos( mv.Pos() + force );

            PNS_DBG( dbg, AddShape, &ff, WHITE, 100000, "via-force-coll" );
        }

        iter++;
    }

    if( iter == aMaxIterations )
        return false;

    PNS_DBG( dbg, AddPoint, ( Pos() + totalForce ), WHITE, 1000000, "via-force-new" );

    aForce = totalForce;

    return true;
}


const SHAPE_LINE_CHAIN VIA::Hull( int aClearance, int aWalkaroundThickness, int aLayer ) const
{
    int cl = ( aClearance + aWalkaroundThickness / 2 );
    int width = m_diameter;

    if( m_hole && !ROUTER::GetInstance()->GetInterface()->IsFlashedOnLayer( this, aLayer ) )
        width = m_hole->Radius() * 2;

    // Chamfer = width * ( 1 - sqrt(2)/2 ) for equilateral octagon
    return OctagonalHull( m_pos - VECTOR2I( width / 2, width / 2 ),
                          VECTOR2I( width, width ),
                          cl, ( 2 * cl + width ) * ( 1.0 - M_SQRT1_2 ) );
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
    v->SetHole( HOLE::MakeCircularHole( m_pos, m_drill / 2 ) );
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
    if( aOther->Pos() != Pos() )
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


const std::string VIA::Format( ) const
{
    std::stringstream ss;
    ss << ITEM::Format() << " drill " << m_drill << " ";
    ss << m_shape.Format( false );
    return ss.str();
}

}

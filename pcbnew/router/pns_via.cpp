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

#include "pns_via.h"
#include "pns_node.h"
#include "pns_utils.h"
#include "pns_router.h"
#include "pns_debug_decorator.h"

#include <geometry/shape_rect.h>
#include <math/box2.h>

namespace PNS {

int VIA::EffectiveLayer( int aLayer ) const
{
    switch( m_stackMode )
    {
    default:
    case STACK_MODE::NORMAL:
        return ALL_LAYERS;

    case STACK_MODE::FRONT_INNER_BACK:
        if( aLayer == m_layers.Start() || aLayer == m_layers.End() )
            return aLayer;

        if( m_layers.Start() + 1 < m_layers.End() )
            return m_layers.Start() + 1;

        return m_layers.Start();

    case STACK_MODE::CUSTOM:
        return m_layers.Overlaps( aLayer ) ? aLayer : m_layers.Start();
    }
}


std::vector<int> VIA::UniqueShapeLayers() const
{
    switch( m_stackMode )
    {
    default:
    case STACK_MODE::NORMAL:
        return { ALL_LAYERS };

    case STACK_MODE::FRONT_INNER_BACK:
        return { ALL_LAYERS, INNER_LAYERS, m_layers.End() };

    case STACK_MODE::CUSTOM:
        std::vector<int> ret;

        for( int l = m_layers.Start(); l <= m_layers.End(); l++ )
            ret.push_back( l );

        return ret;
    }
}


bool VIA::ConnectsLayer( int aLayer ) const
{
    if( m_unconnectedLayerMode == PADSTACK::UNCONNECTED_LAYER_MODE::START_END_ONLY )
        return aLayer == m_layers.Start() || aLayer == m_layers.End();

    return m_layers.Overlaps( aLayer );
}


void VIA::SetStackMode( STACK_MODE aStackMode )
{
    m_stackMode = aStackMode;

    wxASSERT_MSG( m_stackMode != STACK_MODE::FRONT_INNER_BACK || m_layers.Start() == 0,
                  wxT( "Cannot use FRONT_INNER_BACK with blind/buried vias!" ) );

    // In theory, it might be good to do some housekeeping on m_diameters and m_shapes here,
    // but it's not yet clear if the stack mode needs to be changed after initial creation.
}


bool VIA::PadstackMatches( const VIA& aOther ) const
{
    std::vector<int> myLayers = UniqueShapeLayers();
    std::vector<int> otherLayers = aOther.UniqueShapeLayers();

    if( !std::equal( myLayers.begin(), myLayers.end(), otherLayers.begin() ) )
        return false;

    for( int i : myLayers )
    {
        if( Diameter( i ) != aOther.Diameter( i ) )
            return false;
    }

    return true;
}


bool VIA::PushoutForce( NODE* aNode, const ITEM* aOther, VECTOR2I& aForce )
{
    int      clearance = aNode->GetClearance( this, aOther, false );
    VECTOR2I elementForce;

    for( int layer : RelevantShapeLayers( aOther ) )
    {
        aOther->Shape( layer )->Collide( Shape( layer ), clearance, &elementForce );

        if( elementForce.SquaredEuclideanNorm() > aForce.SquaredEuclideanNorm() )
            aForce = elementForce;
    }

    return ( aForce != VECTOR2I( 0, 0 ) );
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
        COLLISION_SEARCH_OPTIONS opt;
        opt.m_limitCount = 1;
        opt.m_kindMask = aCollisionMask;
        opt.m_useClearanceEpsilon = false;

        NODE::OPT_OBSTACLE obs = aNode->CheckColliding( &mv, opt );

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

        // TODO(JE) padstacks -- what is the correct logic here?
        const int threshold = Diameter( EffectiveLayer( 0 ) ) / 4; // another stupid heuristic.
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
    wxASSERT_MSG( aLayer >= 0 || m_stackMode == STACK_MODE::NORMAL,
                  wxT( "Warning: VIA::Hull called with invalid layer but viastack is complex" ) );

    int cl = ( aClearance + aWalkaroundThickness / 2 );
    int width = Diameter( aLayer );

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

    v->m_uid = m_uid; // fixme: oop
    v->m_parent = m_parent;
    v->m_sourceItem = m_sourceItem;

    v->SetNet( Net() );
    v->SetLayers( Layers() );
    v->m_movable = m_movable;
    v->m_pos = m_pos;
    v->m_stackMode = m_stackMode;
    v->m_diameters = m_diameters;
    v->m_drill = m_drill;

    for( const auto& [layer, shape] : m_shapes )
        v->m_shapes[layer] = SHAPE_CIRCLE( m_pos, shape.GetRadius() );

    v->SetHole( HOLE::MakeCircularHole( m_pos, m_drill / 2, m_layers ) );
    v->m_rank = m_rank;
    v->m_marker = m_marker;
    v->m_routable = m_routable;
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
        BOX2I tmp;

        for( int layer : UniqueShapeLayers() )
            tmp.Merge( Shape( layer )->BBox() );

        for( int layer : aOther->UniqueShapeLayers() )
            tmp.Merge( aOther->Shape( layer )->BBox() );

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
    // TODO(JE) padstacks
    ss << Shape( 0 )->Format( false );
    return ss.str();
}

}

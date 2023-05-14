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

#include "pns_node.h"
#include "pns_item.h"
#include "pns_line.h"
#include "pns_router.h"

#include <geometry/shape_compound.h>
#include <geometry/shape_poly_set.h>

typedef VECTOR2I::extended_type ecoord;

namespace PNS {

static void dumpObstacles( const PNS::NODE::OBSTACLES &obstacles )
{
    printf( "&&&& %lu obstacles: \n", obstacles.size() );

    for( const auto& obs : obstacles )
    {
        printf( "%p [%s] - %p [%s], clearance %d\n",
                obs.m_head, obs.m_head->KindStr().c_str(),
                obs.m_item, obs.m_item->KindStr().c_str(),
                obs.m_clearance );
    }
}


bool ITEM::collideSimple( const ITEM* aHead, const NODE* aNode,
                          COLLISION_SEARCH_CONTEXT* aCtx ) const
{
    // Note: if 'this' is a pad or a via then its hole is a separate PNS::ITEM in the node's
    // index and we don't need to deal with holeI here.  The same is *not* true of the routing
    // "head", so we do need to handle holeH.

    const SHAPE* shapeI = Shape();
    int          lineWidthI = 0;

    const SHAPE* shapeH = aHead->Shape();
    const HOLE*  holeH = aHead->Hole();
    int          lineWidthH = 0;
    int          clearanceEpsilon = aNode->GetRuleResolver()->ClearanceEpsilon();
    bool         collisionsFound = false;

    //printf( "******************** CollideSimple %lu\n", aCtx->obstacles.size() );

    //printf( "h %p n %p t %p ctx %p\n", aHead, aNode, this, aCtx );

    if( this == aHead || this == holeH )  // we cannot be self-colliding
        return false;

    // Special cases for "head" lines with vias attached at the end.  Note that this does not
    // support head-line-via to head-line-via collisions, but you can't route two independent
    // tracks at once so it shouldn't come up.

    if( const auto line = dyn_cast<const LINE*>( this ) )
    {
        if( line->EndsWithVia() )
            collisionsFound |= line->Via().collideSimple( aHead, aNode, aCtx );
    }

    if( const auto line = dyn_cast<const LINE*>( aHead ) )
    {
        if( line->EndsWithVia() )
            collisionsFound |= line->Via().collideSimple( this, aNode, aCtx );
    }

    // And a special case for the "head" via's hole.

    if( holeH )
        collisionsFound |= collideSimple( holeH, aNode, aCtx );

    // Sadly collision routines ignore SHAPE_POLY_LINE widths so we have to pass them in as part
    // of the clearance value.
    if( m_kind == LINE_T )
        lineWidthI = static_cast<const LINE*>( this )->Width() / 2;

    if( aHead->m_kind == LINE_T )
        lineWidthH = static_cast<const LINE*>( aHead )->Width() / 2;

    // check if we are not on completely different layers first
    if( !m_layers.Overlaps( aHead->m_layers ) )
        return false;

    // fixme: this f***ing singleton must go...
    ROUTER*       router = ROUTER::GetInstance();
    ROUTER_IFACE* iface = router ? router->GetInterface() : nullptr;
    bool          differentNetsOnly = aCtx && aCtx->options.m_differentNetsOnly;
    int           clearance;

    // Hole-to-hole collisions don't have anything to do with nets
    if( Kind() == HOLE_T && aHead->Kind() == HOLE_T )
        differentNetsOnly = false;

    if( differentNetsOnly && Net() == aHead->Net() && aHead->Net() >= 0 )
    {
        // same nets? no clearance!
        clearance = -1;
    }
    else if( differentNetsOnly && ( IsFreePad() || aHead->IsFreePad() ) )
    {
        // a pad associated with a "free" pin (NIC) doesn't have a net until it has been used
        clearance = -1;
    }
    else if( aNode->GetRuleResolver()->IsKeepout( this, aHead ) )
    {
        clearance = 0;    // keepouts are exact boundary; no clearance
    }
    else if( iface && !iface->IsFlashedOnLayer( this, aHead->Layers() ) )
    {
        clearance = -1;
    }
    else if( iface && !iface->IsFlashedOnLayer( aHead, Layers() ) )
    {
        clearance = -1;
    }
    else if( aCtx && aCtx->options.m_overrideClearance >= 0 )
    {
        clearance = aCtx->options.m_overrideClearance;
    }
    else
    {
        clearance = aNode->GetClearance( this, aHead,
                                         aCtx ? aCtx->options.m_useClearanceEpsilon : false );
    }

    if( clearance >= 0 )
    {
        // Note: we can't do castellation or net-tie processing in GetClearance() because they
        // depend on *where* the collision is.

        bool checkCastellation = ( m_parent && m_parent->GetLayer() == Edge_Cuts );
        bool checkNetTie = ( m_parent && aNode->GetRuleResolver()->IsInNetTie( this ) );

        if( checkCastellation || checkNetTie )
        {
            // Slow method
            int      actual;
            VECTOR2I pos;

            if( shapeH->Collide( shapeI, clearance + lineWidthH + lineWidthI - clearanceEpsilon,
                                 &actual, &pos ) )
            {
                if( checkCastellation && aNode->QueryEdgeExclusions( pos ) )
                    return false;

                if( checkNetTie && aNode->GetRuleResolver()->IsNetTieExclusion( aHead, pos, this ) )
                    return false;

                if( aCtx )
                {
                    collisionsFound = true;
                    OBSTACLE obs;
                    obs.m_head = const_cast<ITEM*>( aHead );
                    obs.m_item = const_cast<ITEM*>( this );
                    obs.m_clearance = clearance;
                    aCtx->obstacles.insert( obs );
                }
                else
                {
                    return true;
                }
            }
        }
        else
        {
            // Fast method
            if( shapeH->Collide( shapeI, clearance + lineWidthH + lineWidthI - clearanceEpsilon ) )
            {
                if( aCtx )
                {
                    collisionsFound = true;
                    OBSTACLE obs;
                    obs.m_head = const_cast<ITEM*>( aHead );
                    obs.m_item = const_cast<ITEM*>( this );
                    obs.m_clearance = clearance;
                    aCtx->obstacles.insert( obs );
                }
                else
                {
                    return true;
                }
            }
        }
    }

    return collisionsFound;
}


bool ITEM::Collide( const ITEM* aOther, const NODE* aNode, COLLISION_SEARCH_CONTEXT *aCtx ) const
{
    if( collideSimple( aOther, aNode, aCtx ) )
        return true;

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
    case HOLE_T:      return "hole";

    default:          return "unknown";
    }
}


ITEM::~ITEM()
{
}


const std::string ITEM::Format() const
{
    std::stringstream ss;
    ss << KindStr() << " ";
    ss << "net " << Net() << " ";
    ss << "layers " << m_layers.Start() << " " << m_layers.End();
    return ss.str();
}

}

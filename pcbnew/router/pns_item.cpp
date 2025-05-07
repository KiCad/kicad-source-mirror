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
    printf( "&&&& %zu obstacles: \n", obstacles.size() );

    for( const auto& obs : obstacles )
    {
        printf( "%p [%s] - %p [%s], clearance %d\n",
                obs.m_head, obs.m_head->KindStr().c_str(),
                obs.m_item, obs.m_item->KindStr().c_str(),
                obs.m_clearance );
    }
}


// prune self-collisions, i.e. a via/pad annular ring with its own hole
static bool shouldWeConsiderHoleCollisions( const ITEM* aItem, const ITEM* aHead )
{
    const HOLE* holeI = aItem->OfKind( ITEM::HOLE_T ) ? static_cast<const HOLE*>( aItem ) : nullptr;
    const HOLE* holeH = aHead->OfKind( ITEM::HOLE_T ) ? static_cast<const HOLE*>( aHead ) : nullptr;

    if( holeI && holeH ) // hole-to-hole case
    {
        const ITEM* parentI = holeI->ParentPadVia();
        const ITEM* parentH = holeH->ParentPadVia();

        if( !parentH || !parentI )
            return true;

        const VIA* parentViaI = dyn_cast<const VIA*>( parentI );
        const VIA* parentViaH = dyn_cast<const VIA*>( parentH );

        // Note to self: the if() below is an ugly heuristic to determine if we aren't trying
        // to check for collisions of the hole of the via with another (although identical)
        // copy of it. Such case occurs when checking a LINE against a NODE where this LINE
        // has been already added. LINE has no notion of ownership of it's via (it's just a
        // copy) and before hole-to-hole clearance support has been introduced it didn't matter
        // becasue we didn't consider collisions of the objects belonging to the same net anyway
        // Now that hole clearance check doesn't care about the nets assigned to the parent
        // vias/solids, I'll probably have to refactor the LINE class to manage ownership of
        // its (optional) VIA. For the moment, we just treat via holes that are geometrically
        // identical and belonging to the same net as non-colliding.

        if( parentViaI && parentViaH && parentViaI->Pos() == parentViaH->Pos()
            && parentViaI->PadstackMatches( *parentViaH )
            && parentViaI->Net() == parentViaH->Net()
            && parentViaI->Drill() == parentViaH->Drill() )
            return false;

        return parentI != parentH;
    }

    if( holeI )
        return holeI->ParentPadVia() != aHead;
    else if( holeH )
        return holeH->ParentPadVia() != aItem;
    else
        return true;
}


std::set<int> ITEM::RelevantShapeLayers( const ITEM* aOther ) const
{
    std::vector<int> myLayers = UniqueShapeLayers();
    std::vector<int> otherLayers = aOther->UniqueShapeLayers();

    if( !HasUniqueShapeLayers() && !aOther->HasUniqueShapeLayers() )
        return { -1 };

    // TODO(JE) at this point we should also mask off the layers of each item.
    // In the case that one item is a via and the other is a track, we don't want to test
    // more than once even if the via has multiple unique layers

    std::set<int> relevantLayers;

    std::set_union( myLayers.begin(), myLayers.end(), otherLayers.begin(), otherLayers.end(),
                    std::inserter( relevantLayers, relevantLayers.begin() ) );

    return relevantLayers;
}


bool ITEM::collideSimple( const ITEM* aHead, const NODE* aNode, int aLayer,
                          COLLISION_SEARCH_CONTEXT* aCtx ) const
{
    // Note: if 'this' is a pad or a via then its hole is a separate PNS::ITEM in the node's
    // index and we don't need to deal with holeI here.  The same is *not* true of the routing
    // "head", so we do need to handle holeH.
    int          lineWidthI = 0;

    //const SHAPE* shapeH = aHead->Shape();
    const HOLE*  holeH = aHead->Hole();
    const HOLE*  holeI = Hole();

    int          lineWidthH = 0;
    bool         collisionsFound = false;

    if( this == aHead )  // we cannot be self-colliding
        return false;

    if ( !shouldWeConsiderHoleCollisions( this, aHead ) )
        return false;

    // Special cases for "head" lines with vias attached at the end.  Note that this does not
    // support head-line-via to head-line-via collisions, but you can't route two independent
    // tracks at once so it shouldn't come up.

    if( const auto line = dyn_cast<const LINE*>( this ) )
    {
        if( line->EndsWithVia() )
            collisionsFound |= line->Via().collideSimple( aHead, aNode, aLayer, aCtx );
    }

    if( const auto line = dyn_cast<const LINE*>( aHead ) )
    {
        if( line->EndsWithVia() )
            collisionsFound |= line->Via().collideSimple( this, aNode, aLayer, aCtx );
    }

    // And a special case for the "head" via's hole.
    if( aHead->HasHole() && shouldWeConsiderHoleCollisions( this, holeH ) )
    {
        // Skip net check when doing hole-to-hole collisions.
        if( Kind() == HOLE_T || Net() != holeH->Net() )
            collisionsFound |= collideSimple( holeH, aNode, aLayer, aCtx );
    }

    if( HasHole() && shouldWeConsiderHoleCollisions( holeI, aHead ) )
    {
        collisionsFound |= holeI->collideSimple( aHead, aNode, aLayer, aCtx );
    }

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
    bool          differentNetsOnly = true;
    bool          enforce = false;
    int           clearance;

    if( aCtx )
        differentNetsOnly = aCtx->options.m_differentNetsOnly;

    // Hole-to-hole collisions don't have anything to do with nets
    if( Kind() == HOLE_T && aHead->Kind() == HOLE_T )
        differentNetsOnly = false;

    if( differentNetsOnly && Net() == aHead->Net() && aHead->Net() )
    {
        // same nets? no clearance!
        clearance = -1;
    }
    else if( differentNetsOnly && ( IsFreePad() || aHead->IsFreePad() ) )
    {
        // a pad associated with a "free" pin (NIC) doesn't have a net until it has been used
        clearance = -1;
    }
    else if( aNode->GetRuleResolver()->IsKeepout( this, aHead, &enforce )
             || aNode->GetRuleResolver()->IsKeepout( aHead, this, &enforce ) )
    {
        if( enforce )
            clearance = 0;    // keepouts are exact boundary; no clearance
        else
            clearance = -1;
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
        clearance = aNode->GetClearance( this, aHead, aCtx ? aCtx->options.m_useClearanceEpsilon
                                                           : false );
    }

    if( clearance >= 0 )
    {
        // Note: we can't do castellation or net-tie processing in GetClearance() because they
        // depend on *where* the collision is.

        bool checkCastellation = ( m_parent && m_parent->GetLayer() == Edge_Cuts )
                                   || aNode->GetRuleResolver()->IsNonPlatedSlot( this );

        bool checkNetTie = aNode->GetRuleResolver()->IsInNetTie( this );

        const SHAPE* shapeI = Shape( aLayer );
        const SHAPE* shapeH = aHead->Shape( aLayer );

        if( checkCastellation || checkNetTie )
        {
            // Slow method
            int      actual;
            VECTOR2I pos;

            // The extra "1" here is to account for the fact that the hulls are built to exactly
            // the clearance distance, so we need to allow for no collision when exactly at the
            // clearance distance.
            if( shapeH->Collide( shapeI, clearance + lineWidthH + lineWidthI - 1, &actual, &pos ) )
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
                    obs.m_distFirst = 0;
                    obs.m_maxFanoutWidth = 0;
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
            // The extra "1" here is to account for the fact that the hulls are built to exactly
            // the clearance distance, so we need to allow for no collision when exactly at the
            // clearance distance.
            if( shapeH->Collide( shapeI, clearance + lineWidthH + lineWidthI - 1 ) )
            {
                if( aCtx )
                {
                    collisionsFound = true;
                    OBSTACLE obs;
                    obs.m_head = const_cast<ITEM*>( aHead );
                    obs.m_item = const_cast<ITEM*>( this );
                    obs.m_clearance = clearance;
                    obs.m_distFirst = 0;
                    obs.m_maxFanoutWidth = 0;
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


bool ITEM::Collide( const ITEM* aOther, const NODE* aNode, int aLayer,
                    COLLISION_SEARCH_CONTEXT *aCtx ) const
{
    if( collideSimple( aOther, aNode, aLayer, aCtx ) )
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
    ROUTER*       router = ROUTER::GetInstance();
    ROUTER_IFACE* iface = router ? router->GetInterface() : nullptr;

    std::stringstream ss;
    ss << KindStr() << " ";

    if( iface )
        ss << "net " << iface->GetNetName( Net() ) << " ";

    ss << "layers " << m_layers.Start() << " " << m_layers.End();
    return ss.str();
}


const NODE* ITEM::OwningNode() const
{
    if( ParentPadVia() )
        return static_cast<const NODE*>( ParentPadVia()->Owner() );
    else
        return static_cast<const NODE*>( Owner() );
}

LINKED_ITEM::UNIQ_ID LINKED_ITEM::genNextUid()
{
    static UNIQ_ID uidCount = 0; // fixme: make atomic
    return uidCount++;
}

} // namespace PNS

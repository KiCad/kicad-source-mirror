/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2019 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <vector>
#include <cassert>
#include <utility>

#include <math/vector2d.h>

#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>

#include "pns_arc.h"
#include "pns_item.h"
#include "pns_line.h"
#include "pns_node.h"
#include "pns_via.h"
#include "pns_solid.h"
#include "pns_joint.h"
#include "pns_index.h"
#include "pns_debug_decorator.h"
#include "pns_router.h"


namespace PNS {

#ifdef DEBUG
static std::unordered_set<NODE*> allocNodes;
#endif

NODE::NODE()
{
    m_depth = 0;
    m_root = this;
    m_parent = NULL;
    m_maxClearance = 800000;    // fixme: depends on how thick traces are.
    m_ruleResolver = NULL;
    m_index = new INDEX;

#ifdef DEBUG
    allocNodes.insert( this );
#endif
}


NODE::~NODE()
{
    if( !m_children.empty() )
    {
        wxLogTrace( "PNS", "attempting to free a node that has kids." );
        assert( false );
    }

#ifdef DEBUG
    if( allocNodes.find( this ) == allocNodes.end() )
    {
        wxLogTrace( "PNS", "attempting to free an already-free'd node." );
        assert( false );
    }

    allocNodes.erase( this );
#endif

    m_joints.clear();

    for( ITEM* item : *m_index )
    {
        if( item->BelongsTo( this ) )
            delete item;
    }

    releaseGarbage();
    unlinkParent();

    delete m_index;
}

int NODE::GetClearance( const ITEM* aA, const ITEM* aB ) const
{
   if( !m_ruleResolver )
        return 100000;

   return m_ruleResolver->Clearance( aA, aB );
}


int NODE::GetHoleClearance( const ITEM* aA, const ITEM* aB ) const
   {
      if( !m_ruleResolver )
           return 0;

      return m_ruleResolver->HoleClearance( aA, aB );
   }


int NODE::GetHoleToHoleClearance( const ITEM* aA, const ITEM* aB ) const
{
   if( !m_ruleResolver )
        return 0;

   return m_ruleResolver->HoleToHoleClearance( aA, aB );
}


NODE* NODE::Branch()
{
    NODE* child = new NODE;

    m_children.insert( child );

    child->m_depth = m_depth + 1;
    child->m_parent = this;
    child->m_ruleResolver = m_ruleResolver;
    child->m_root = isRoot() ? this : m_root;
    child->m_maxClearance = m_maxClearance;

    // Immediate offspring of the root branch needs not copy anything. For the rest, deep-copy
    // joints, overridden item maps and pointers to stored items.
    if( !isRoot() )
    {
        JOINT_MAP::iterator j;

        for( ITEM* item : *m_index )
            child->m_index->Add( item );

        child->m_joints = m_joints;
        child->m_override = m_override;
    }

#if 0
    wxLogTrace( "PNS", "%d items, %d joints, %d overrides",
                child->m_index->Size(),
                (int) child->m_joints.size(),
                (int) child->m_override.size() );
#endif

    return child;
}


void NODE::unlinkParent()
{
    if( isRoot() )
        return;

    m_parent->m_children.erase( this );
}


OBSTACLE_VISITOR::OBSTACLE_VISITOR( const ITEM* aItem ) :
    m_item( aItem ),
    m_node( NULL ),
    m_override( NULL ),
    m_extraClearance( 0 )
{
}


void OBSTACLE_VISITOR::SetWorld( const NODE* aNode, const NODE* aOverride )
{
    m_node = aNode;
    m_override = aOverride;
}


bool OBSTACLE_VISITOR::visit( ITEM* aCandidate )
{
    // check if there is a more recent branch with a newer (possibly modified) version of this
    // item.
    if( m_override && m_override->Overrides( aCandidate ) )
        return true;

    return false;
}


// function object that visits potential obstacles and performs the actual collision refining
struct NODE::DEFAULT_OBSTACLE_VISITOR : public OBSTACLE_VISITOR
{
    OBSTACLES& m_tab;

    int        m_kindMask;          ///<  (solids, vias, segments, etc...)
    int        m_limitCount;
    int        m_matchCount;
    int        m_extraClearance;
    bool       m_differentNetsOnly;

    DEFAULT_OBSTACLE_VISITOR( NODE::OBSTACLES& aTab, const ITEM* aItem, int aKindMask,
                              bool aDifferentNetsOnly ) :
        OBSTACLE_VISITOR( aItem ),
        m_tab( aTab ),
        m_kindMask( aKindMask ),
        m_limitCount( -1 ),
        m_matchCount( 0 ),
        m_extraClearance( 0 ),
        m_differentNetsOnly( aDifferentNetsOnly )
    {
        if( aItem && aItem->Kind() == ITEM::LINE_T )
        {
             m_extraClearance += static_cast<const LINE*>( aItem )->Width() / 2;
        }
    }

    virtual ~DEFAULT_OBSTACLE_VISITOR()
    {
    }

    void SetCountLimit( int aLimit )
    {
        m_limitCount = aLimit;
    }

    bool operator()( ITEM* aCandidate ) override
    {
        if( !aCandidate->OfKind( m_kindMask ) )
            return true;

        if( visit( aCandidate ) )
            return true;

        if( !aCandidate->Collide( m_item, m_node, m_differentNetsOnly ) )
            return true;

        OBSTACLE obs;

        obs.m_item = aCandidate;
        obs.m_head = m_item;
        m_tab.push_back( obs );

        m_matchCount++;

        if( m_limitCount > 0 && m_matchCount >= m_limitCount )
            return false;

        return true;
    };
};


int NODE::QueryColliding( const ITEM* aItem, NODE::OBSTACLES& aObstacles, int aKindMask,
                          int aLimitCount, bool aDifferentNetsOnly )
{
    DEFAULT_OBSTACLE_VISITOR visitor( aObstacles, aItem, aKindMask, aDifferentNetsOnly );

#ifdef DEBUG
    assert( allocNodes.find( this ) != allocNodes.end() );
#endif

    visitor.SetCountLimit( aLimitCount );
    visitor.SetWorld( this, NULL );

    // first, look for colliding items in the local index
    m_index->Query( aItem, m_maxClearance, visitor );

    // if we haven't found enough items, look in the root branch as well.
    if( !isRoot() && ( visitor.m_matchCount < aLimitCount || aLimitCount < 0 ) )
    {
        visitor.SetWorld( m_root, this );
        m_root->m_index->Query( aItem, m_maxClearance, visitor );
    }

    return aObstacles.size();
}


NODE::OPT_OBSTACLE NODE::NearestObstacle( const LINE* aLine, int aKindMask,
                                          const std::set<ITEM*>* aRestrictedSet )
{
    OBSTACLES obstacleList;
    obstacleList.reserve( 100 );

    for( int i = 0; i < aLine->CLine().SegmentCount(); i++ )
    {
        const SEGMENT s( *aLine, aLine->CLine().CSegment( i ) );
        QueryColliding( &s, obstacleList, aKindMask );
    }

    if( aLine->EndsWithVia() )
        QueryColliding( &aLine->Via(), obstacleList, aKindMask );

    if( obstacleList.empty() )
        return OPT_OBSTACLE();

    OBSTACLE nearest;
    nearest.m_item = NULL;
    nearest.m_distFirst = INT_MAX;

    auto updateNearest =
            [&]( VECTOR2I pt, ITEM* obstacle, const SHAPE_LINE_CHAIN& hull, bool isHole )
            {
                int dist = aLine->CLine().PathLength( pt );

                if( dist < nearest.m_distFirst )
                {
                    nearest.m_distFirst = dist;
                    nearest.m_ipFirst = pt;
                    nearest.m_item = obstacle;
                    nearest.m_hull = hull;

                    obstacle->Mark( isHole ? obstacle->Marker() | MK_HOLE
                                           : obstacle->Marker() & ~MK_HOLE );
                }
            };

    SHAPE_LINE_CHAIN obstacleHull;
    DEBUG_DECORATOR* debugDecorator = ROUTER::GetInstance()->GetInterface()->GetDebugDecorator();
    std::vector<SHAPE_LINE_CHAIN::INTERSECTION> intersectingPts;
    int layer = aLine->Layer();

    for( const OBSTACLE& obstacle : obstacleList )
    {
        if( aRestrictedSet && aRestrictedSet->find( obstacle.m_item ) == aRestrictedSet->end() )
            continue;

        int clearance = GetClearance( obstacle.m_item, aLine ) + aLine->Width() / 2;
        obstacleHull = obstacle.m_item->Hull( clearance + PNS_HULL_MARGIN, 0, layer );
        debugDecorator->AddLine( obstacleHull, 2 );

        intersectingPts.clear();
        obstacleHull.Intersect( aLine->CLine(), intersectingPts );

        for( const SHAPE_LINE_CHAIN::INTERSECTION& ip : intersectingPts )
            updateNearest( ip.p, obstacle.m_item, obstacleHull, false );

        if( aLine->EndsWithVia() )
        {
            const VIA& via = aLine->Via();
            // Don't use via.Drill(); it doesn't include the plating thickness
            int viaHoleRadius = static_cast<const SHAPE_CIRCLE*>( via.Hole() )->GetRadius();

            int viaClearance = GetClearance( obstacle.m_item, &via ) + via.Diameter() / 2;
            int holeClearance = GetHoleClearance( obstacle.m_item, &via ) + viaHoleRadius;

            if( holeClearance > viaClearance )
                viaClearance = holeClearance;

            obstacleHull = obstacle.m_item->Hull( viaClearance + PNS_HULL_MARGIN, 0, layer );
            debugDecorator->AddLine( obstacleHull, 3 );

            intersectingPts.clear();
            obstacleHull.Intersect( aLine->CLine(), intersectingPts );

            for( const SHAPE_LINE_CHAIN::INTERSECTION& ip : intersectingPts )
                updateNearest( ip.p, obstacle.m_item, obstacleHull, false );
        }

        if( obstacle.m_item->Hole() )
        {
            clearance = GetHoleClearance( obstacle.m_item, aLine ) + aLine->Width() / 2;
            obstacleHull = obstacle.m_item->HoleHull( clearance + PNS_HULL_MARGIN, 0, layer );
            debugDecorator->AddLine( obstacleHull, 4 );

            intersectingPts.clear();
            obstacleHull.Intersect( aLine->CLine(), intersectingPts );

            for( const SHAPE_LINE_CHAIN::INTERSECTION& ip : intersectingPts )
                updateNearest( ip.p, obstacle.m_item, obstacleHull, true );

            if( aLine->EndsWithVia() )
            {
                const VIA& via = aLine->Via();
                // Don't use via.Drill(); it doesn't include the plating thickness
                int viaHoleRadius = static_cast<const SHAPE_CIRCLE*>( via.Hole() )->GetRadius();

                int viaClearance = GetClearance( obstacle.m_item, &via ) + via.Diameter() / 2;
                int holeClearance = GetHoleClearance( obstacle.m_item, &via ) + viaHoleRadius;
                int holeToHole = GetHoleToHoleClearance( obstacle.m_item, &via ) + viaHoleRadius;

                if( holeClearance > viaClearance )
                    viaClearance = holeClearance;

                if( holeToHole > viaClearance )
                    viaClearance = holeToHole;

                obstacleHull = obstacle.m_item->Hull( viaClearance + PNS_HULL_MARGIN, 0, layer );
                debugDecorator->AddLine( obstacleHull, 5 );

                intersectingPts.clear();
                obstacleHull.Intersect( aLine->CLine(), intersectingPts );

                for( const SHAPE_LINE_CHAIN::INTERSECTION& ip : intersectingPts )
                    updateNearest( ip.p, obstacle.m_item, obstacleHull, true );
            }
        }
    }

    if( nearest.m_distFirst == INT_MAX )
        nearest.m_item = obstacleList[0].m_item;

    return nearest;
}


NODE::OPT_OBSTACLE NODE::CheckColliding( const ITEM_SET& aSet, int aKindMask )
{
    for( const ITEM* item : aSet.CItems() )
    {
        OPT_OBSTACLE obs = CheckColliding( item, aKindMask );

        if( obs )
            return  obs;
    }

    return OPT_OBSTACLE();
}


NODE::OPT_OBSTACLE NODE::CheckColliding( const ITEM* aItemA, int aKindMask )
{
    OBSTACLES obs;

    obs.reserve( 100 );

    if( aItemA->Kind() == ITEM::LINE_T )
    {
        int n = 0;
        const LINE* line = static_cast<const LINE*>( aItemA );
        const SHAPE_LINE_CHAIN& l = line->CLine();

        for( int i = 0; i < l.SegmentCount(); i++ )
        {
            const SEGMENT s( *line, l.CSegment( i ) );
            n += QueryColliding( &s, obs, aKindMask, 1 );

            if( n )
                return OPT_OBSTACLE( obs[0] );
        }

        if( line->EndsWithVia() )
        {
            n += QueryColliding( &line->Via(), obs, aKindMask, 1 );

            if( n )
                return OPT_OBSTACLE( obs[0] );
        }
    }
    else if( QueryColliding( aItemA, obs, aKindMask, 1 ) > 0 )
    {
        return OPT_OBSTACLE( obs[0] );
    }

    return OPT_OBSTACLE();
}


struct HIT_VISITOR : public OBSTACLE_VISITOR
{
    ITEM_SET& m_items;
    const VECTOR2I& m_point;

    HIT_VISITOR( ITEM_SET& aTab, const VECTOR2I& aPoint ) :
        OBSTACLE_VISITOR( NULL ),
        m_items( aTab ),
        m_point( aPoint )
    {}

    virtual ~HIT_VISITOR()
    {
    }

    bool operator()( ITEM* aItem ) override
    {
        SHAPE_CIRCLE cp( m_point, 0 );

        int cl = 0;

        if( aItem->Shape()->Collide( &cp, cl ) )
            m_items.Add( aItem );

        return true;
    }
};


const ITEM_SET NODE::HitTest( const VECTOR2I& aPoint ) const
{
    ITEM_SET items;

    // fixme: we treat a point as an infinitely small circle - this is inefficient.
    SHAPE_CIRCLE s( aPoint, 0 );
    HIT_VISITOR visitor( items, aPoint );
    visitor.SetWorld( this, NULL );

    m_index->Query( &s, m_maxClearance, visitor );

    if( !isRoot() )    // fixme: could be made cleaner
    {
        ITEM_SET items_root;
        visitor.SetWorld( m_root, NULL );
        HIT_VISITOR  visitor_root( items_root, aPoint );
        m_root->m_index->Query( &s, m_maxClearance, visitor_root );

        for( ITEM* item : items_root.Items() )
        {
            if( !Overrides( item ) )
                items.Add( item );
        }
    }

    return items;
}


void NODE::addSolid( SOLID* aSolid )
{
    if( aSolid->IsRoutable() )
        linkJoint( aSolid->Pos(), aSolid->Layers(), aSolid->Net(), aSolid );

    m_index->Add( aSolid );
}


void NODE::Add( std::unique_ptr< SOLID > aSolid )
{
    aSolid->SetOwner( this );
    addSolid( aSolid.release() );
}


void NODE::addVia( VIA* aVia )
{
    linkJoint( aVia->Pos(), aVia->Layers(), aVia->Net(), aVia );

    m_index->Add( aVia );
}


void NODE::Add( std::unique_ptr< VIA > aVia )
{
    aVia->SetOwner( this );
    addVia( aVia.release() );
}


void NODE::Add( LINE& aLine, bool aAllowRedundant )
{
    assert( !aLine.IsLinked() );

    SHAPE_LINE_CHAIN& l = aLine.Line();

    for( size_t i = 0; i < l.ArcCount(); i++ )
    {
        auto s = l.Arc( i );
        ARC* rarc;

        if( !aAllowRedundant && ( rarc = findRedundantArc( s.GetP0(), s.GetP1(), aLine.Layers(),
                                                           aLine.Net() ) ) )
        {
            aLine.Link( rarc );
        }
        else
        {
            auto newarc = std::make_unique< ARC >( aLine, s );
            aLine.Link( newarc.get() );
            Add( std::move( newarc ), true );
        }
    }

    for( int i = 0; i < l.SegmentCount(); i++ )
    {
        if( l.isArc( i ) )
            continue;

        SEG s = l.CSegment( i );

        if( s.A != s.B )
        {
            SEGMENT* rseg;

            if( !aAllowRedundant && ( rseg = findRedundantSegment( s.A, s.B, aLine.Layers(),
                                                                   aLine.Net() ) ) )
            {
                // another line could be referencing this segment too :(
                aLine.Link( rseg );
            }
            else
            {
                std::unique_ptr<SEGMENT> newseg = std::make_unique<SEGMENT>( aLine, s );
                aLine.Link( newseg.get() );
                Add( std::move( newseg ), true );
            }
        }
    }
}


void NODE::addSegment( SEGMENT* aSeg )
{
    linkJoint( aSeg->Seg().A, aSeg->Layers(), aSeg->Net(), aSeg );
    linkJoint( aSeg->Seg().B, aSeg->Layers(), aSeg->Net(), aSeg );

    m_index->Add( aSeg );
}


bool NODE::Add( std::unique_ptr< SEGMENT > aSegment, bool aAllowRedundant )
{
    if( aSegment->Seg().A == aSegment->Seg().B )
    {
        wxLogTrace( "PNS", "attempting to add a segment with same end coordinates, ignoring." );
        return false;
    }

    if( !aAllowRedundant && findRedundantSegment( aSegment.get() ) )
        return false;

    aSegment->SetOwner( this );
    addSegment( aSegment.release() );

    return true;
}


void NODE::addArc( ARC* aArc )
{
    linkJoint( aArc->Anchor( 0 ), aArc->Layers(), aArc->Net(), aArc );
    linkJoint( aArc->Anchor( 1 ), aArc->Layers(), aArc->Net(), aArc );

    m_index->Add( aArc );
}


void NODE::Add( std::unique_ptr< ARC > aArc )
{
    aArc->SetOwner( this );
    addArc( aArc.release() );
}


void NODE::Add( std::unique_ptr< ITEM > aItem, bool aAllowRedundant )
{
    switch( aItem->Kind() )
    {
    case ITEM::SOLID_T:   Add( ItemCast<SOLID>( std::move( aItem ) ) );                    break;
    case ITEM::SEGMENT_T: Add( ItemCast<SEGMENT>( std::move( aItem ) ), aAllowRedundant ); break;
    case ITEM::VIA_T:     Add( ItemCast<VIA>( std::move( aItem ) ) );                      break;

    case ITEM::ARC_T:
        //todo(snh): Add redundant search
        Add( ItemCast<ARC>( std::move( aItem ) ) );
        break;

    case ITEM::LINE_T:
    default:
        assert( false );
    }
}


void NODE::doRemove( ITEM* aItem )
{
    // case 1: removing an item that is stored in the root node from any branch:
    // mark it as overridden, but do not remove
    if( aItem->BelongsTo( m_root ) && !isRoot() )
        m_override.insert( aItem );

    // case 2: the item belongs to this branch or a parent, non-root branch,
    // or the root itself and we are the root: remove from the index
    else if( !aItem->BelongsTo( m_root ) || isRoot() )
        m_index->Remove( aItem );

    // the item belongs to this particular branch: un-reference it
    if( aItem->BelongsTo( this ) )
    {
        aItem->SetOwner( NULL );
        m_root->m_garbageItems.insert( aItem );
    }
}


void NODE::removeSegmentIndex( SEGMENT* aSeg )
{
    unlinkJoint( aSeg->Seg().A, aSeg->Layers(), aSeg->Net(), aSeg );
    unlinkJoint( aSeg->Seg().B, aSeg->Layers(), aSeg->Net(), aSeg );
}


void NODE::removeArcIndex( ARC* aArc )
{
    unlinkJoint( aArc->Anchor( 0 ), aArc->Layers(), aArc->Net(), aArc );
    unlinkJoint( aArc->Anchor( 1 ), aArc->Layers(), aArc->Net(), aArc );
}


void NODE::rebuildJoint( JOINT* aJoint, ITEM* aItem )
{
    // We have to split a single joint (associated with a via or a pad, binding together multiple
    // layers) into multiple independent joints. As I'm a lazy bastard, I simply delete the
    // via/solid and all its links and re-insert them.

    JOINT::LINKED_ITEMS links( aJoint->LinkList() );
    JOINT::HASH_TAG tag;
    int net = aItem->Net();

    tag.net = net;
    tag.pos = aJoint->Pos();

    bool split;

    do
    {
        split = false;
        auto range = m_joints.equal_range( tag );

        if( range.first == m_joints.end() )
            break;

        // find and remove all joints containing the via to be removed

        for( auto f = range.first; f != range.second; ++f )
        {
            if( aItem->LayersOverlap( &f->second ) )
            {
                m_joints.erase( f );
                split = true;
                break;
            }
        }
    } while( split );

    // and re-link them, using the former via's link list
    for( ITEM* link : links )
    {
        if( link != aItem )
            linkJoint( tag.pos, link->Layers(), net, link );
    }
}


void NODE::removeViaIndex( VIA* aVia )
{
    JOINT* jt = FindJoint( aVia->Pos(), aVia->Layers().Start(), aVia->Net() );
    assert( jt );
    rebuildJoint( jt, aVia );
}


void NODE::removeSolidIndex( SOLID* aSolid )
{
    if( !aSolid->IsRoutable() )
        return;

    // fixme: redundant code
    JOINT* jt = FindJoint( aSolid->Pos(), aSolid->Layers().Start(), aSolid->Net() );
    assert( jt );
    rebuildJoint( jt, aSolid );
}


void NODE::Replace( ITEM* aOldItem, std::unique_ptr< ITEM > aNewItem )
{
    Remove( aOldItem );
    Add( std::move( aNewItem ) );
}


void NODE::Replace( LINE& aOldLine, LINE& aNewLine )
{
    Remove( aOldLine );
    Add( aNewLine );
}


void NODE::Remove( SOLID* aSolid )
{
    removeSolidIndex( aSolid );
    doRemove( aSolid );
}


void NODE::Remove( VIA* aVia )
{
    removeViaIndex( aVia );
    doRemove( aVia );
}


void NODE::Remove( SEGMENT* aSegment )
{
    removeSegmentIndex( aSegment );
    doRemove( aSegment );
}


void NODE::Remove( ARC* aArc )
{
    removeArcIndex( aArc );
    doRemove( aArc );
}


void NODE::Remove( ITEM* aItem )
{
    switch( aItem->Kind() )
    {
    case ITEM::ARC_T:
        Remove( static_cast<ARC*>( aItem ) );
        break;

    case ITEM::SOLID_T:
        Remove( static_cast<SOLID*>( aItem ) );
        break;

    case ITEM::SEGMENT_T:
        Remove( static_cast<SEGMENT*>( aItem ) );
        break;

    case ITEM::LINE_T:
    {
        LINE* l = static_cast<LINE*>( aItem );

        for ( LINKED_ITEM* s : l->Links() )
            Remove( s );

        break;
    }

    case ITEM::VIA_T:
        Remove( static_cast<VIA*>( aItem ) );
        break;

    default:
        break;
    }
}


void NODE::Remove( LINE& aLine )
{
    // LINE does not have a separate remover, as LINEs are never truly a member of the tree
    std::vector<LINKED_ITEM*>& segRefs = aLine.Links();

    for( LINKED_ITEM* li : segRefs )
    {
        if( li->OfKind( ITEM::SEGMENT_T ) )
            Remove( static_cast<SEGMENT*>( li ) );
        else if( li->OfKind( ITEM::ARC_T ) )
            Remove( static_cast<ARC*>( li ) );
    }

    aLine.SetOwner( nullptr );
    aLine.ClearLinks();
}


void NODE::followLine( LINKED_ITEM* aCurrent, bool aScanDirection, int& aPos, int aLimit,
                       VECTOR2I* aCorners, LINKED_ITEM** aSegments, bool* aArcReversed,
                       bool& aGuardHit, bool aStopAtLockedJoints )
{
    bool prevReversed = false;

    const VECTOR2I guard = aCurrent->Anchor( aScanDirection );

    for( int count = 0 ; ; ++count )
    {
        const VECTOR2I p = aCurrent->Anchor( aScanDirection ^ prevReversed );
        const JOINT* jt = FindJoint( p, aCurrent );

        assert( jt );

        aCorners[aPos] = jt->Pos();
        aSegments[aPos] = aCurrent;
        aArcReversed[aPos] = false;

        if( aCurrent->Kind() == ITEM::ARC_T )
        {
            if( ( aScanDirection && jt->Pos() == aCurrent->Anchor( 0 ) ) ||
                ( !aScanDirection && jt->Pos() == aCurrent->Anchor( 1 ) ) )
                aArcReversed[aPos] = true;
        }

        aPos += ( aScanDirection ? 1 : -1 );

        if( count && guard == p )
        {
            aSegments[aPos] = NULL;
            aGuardHit = true;
            break;
        }

        bool locked = aStopAtLockedJoints ? jt->IsLocked() : false;

        if( locked || !jt->IsLineCorner() || aPos < 0 || aPos == aLimit )
            break;

        aCurrent = jt->NextSegment( aCurrent );

        prevReversed = ( jt->Pos() == aCurrent->Anchor( aScanDirection ) );
    }
}


const LINE NODE::AssembleLine( LINKED_ITEM* aSeg, int* aOriginSegmentIndex,
                               bool aStopAtLockedJoints )
{
    const int MaxVerts = 1024 * 16;

    VECTOR2I corners[MaxVerts + 1];
    LINKED_ITEM* segs[MaxVerts + 1];
    bool arcReversed[MaxVerts + 1];

    LINE pl;
    bool guardHit = false;

    int i_start = MaxVerts / 2, i_end = i_start + 1;

    pl.SetWidth( aSeg->Width() );
    pl.SetLayers( aSeg->Layers() );
    pl.SetNet( aSeg->Net() );
    pl.SetOwner( this );

    followLine( aSeg, false, i_start, MaxVerts, corners, segs, arcReversed,
                guardHit, aStopAtLockedJoints );

    if( !guardHit )
    {
        followLine( aSeg, true, i_end, MaxVerts, corners, segs, arcReversed, guardHit,
                    aStopAtLockedJoints );
    }

    int n = 0;

    LINKED_ITEM* prev_seg = NULL;
    bool originSet = false;

    for( int i = i_start + 1; i < i_end; i++ )
    {
        const VECTOR2I& p  = corners[i];
        LINKED_ITEM*    li = segs[i];

        if( !li || li->Kind() != ITEM::ARC_T )
            pl.Line().Append( p );

        if( li && prev_seg != li )
        {
            if( li->Kind() == ITEM::ARC_T )
            {
                const ARC*       arc = static_cast<const ARC*>( li );
                const SHAPE_ARC* sa  = static_cast<const SHAPE_ARC*>( arc->Shape() );
                pl.Line().Append( arcReversed[i] ? sa->Reversed() : *sa );
            }

            pl.Link( li );

            // latter condition to avoid loops
            if( li == aSeg && aOriginSegmentIndex && !originSet )
            {
                *aOriginSegmentIndex = n;
                originSet = true;
            }

            n++;
        }

        prev_seg = li;
    }

    // Remove duplicate verts, but do NOT remove colinear segments here!
    pl.Line().Simplify( false );

    assert( pl.SegmentCount() != 0 );

    return pl;
}


void NODE::FindLineEnds( const LINE& aLine, JOINT& aA, JOINT& aB )
{
    aA = *FindJoint( aLine.CPoint( 0 ), &aLine );
    aB = *FindJoint( aLine.CPoint( -1 ), &aLine );
}


int NODE::FindLinesBetweenJoints( const JOINT& aA, const JOINT& aB, std::vector<LINE>& aLines )
{
    for( ITEM* item : aA.LinkList() )
    {
        if( item->Kind() == ITEM::SEGMENT_T || item->Kind() == ITEM::ARC_T )
        {
            LINKED_ITEM* li = static_cast<LINKED_ITEM*>( item );
            LINE line = AssembleLine( li );

            if( !line.Layers().Overlaps( aB.Layers() ) )
                continue;

            JOINT j_start, j_end;

            FindLineEnds( line, j_start, j_end );

            int id_start = line.CLine().Find( aA.Pos() );
            int id_end   = line.CLine().Find( aB.Pos() );

            if( id_end < id_start )
                std::swap( id_end, id_start );

            if( id_start >= 0 && id_end >= 0 )
            {
                line.ClipVertexRange( id_start, id_end );
                aLines.push_back( line );
            }
        }
    }

    return 0;
}


JOINT* NODE::FindJoint( const VECTOR2I& aPos, int aLayer, int aNet )
{
    JOINT::HASH_TAG tag;

    tag.net = aNet;
    tag.pos = aPos;

    JOINT_MAP::iterator f = m_joints.find( tag ), end = m_joints.end();

    if( f == end && !isRoot() )
    {
        end = m_root->m_joints.end();
        f = m_root->m_joints.find( tag );    // m_root->FindJoint(aPos, aLayer, aNet);
    }

    if( f == end )
        return NULL;

    while( f != end )
    {
        if( f->second.Layers().Overlaps( aLayer ) )
            return &f->second;

        ++f;
    }

    return NULL;
}


void NODE::LockJoint( const VECTOR2I& aPos, const ITEM* aItem, bool aLock )
{
    JOINT& jt = touchJoint( aPos, aItem->Layers(), aItem->Net() );
    jt.Lock( aLock );
}


JOINT& NODE::touchJoint( const VECTOR2I& aPos, const LAYER_RANGE& aLayers, int aNet )
{
    JOINT::HASH_TAG tag;

    tag.pos = aPos;
    tag.net = aNet;

    // try to find the joint in this node.
    JOINT_MAP::iterator f = m_joints.find( tag );

    std::pair<JOINT_MAP::iterator, JOINT_MAP::iterator> range;

    // not found and we are not root? find in the root and copy results here.
    if( f == m_joints.end() && !isRoot() )
    {
        range = m_root->m_joints.equal_range( tag );

        for( f = range.first; f != range.second; ++f )
            m_joints.insert( *f );
    }

    // now insert and combine overlapping joints
    JOINT jt( aPos, aLayers, aNet );

    bool merged;

    do
    {
        merged  = false;
        range   = m_joints.equal_range( tag );

        if( range.first == m_joints.end() )
            break;

        for( f = range.first; f != range.second; ++f )
        {
            if( aLayers.Overlaps( f->second.Layers() ) )
            {
                jt.Merge( f->second );
                m_joints.erase( f );
                merged = true;
                break;
            }
        }
    }
    while( merged );

    return m_joints.insert( TagJointPair( tag, jt ) )->second;
}


void JOINT::Dump() const
{
    wxLogTrace( "PNS", "joint layers %d-%d, net %d, pos %s, links: %d",
                m_layers.Start(),
                m_layers.End(),
                m_tag.net,
                m_tag.pos.Format().c_str(),
                LinkCount() );
}


void NODE::linkJoint( const VECTOR2I& aPos, const LAYER_RANGE& aLayers, int aNet, ITEM* aWhere )
{
    JOINT& jt = touchJoint( aPos, aLayers, aNet );

    jt.Link( aWhere );
}


void NODE::unlinkJoint( const VECTOR2I& aPos, const LAYER_RANGE& aLayers, int aNet, ITEM* aWhere )
{
    // fixme: remove dangling joints
    JOINT& jt = touchJoint( aPos, aLayers, aNet );

    jt.Unlink( aWhere );
}


void NODE::Dump( bool aLong )
{
#if 0
    std::unordered_set<SEGMENT*> all_segs;
    SHAPE_INDEX_LIST<ITEM*>::iterator i;

    for( i = m_items.begin(); i != m_items.end(); i++ )
    {
        if( (*i)->GetKind() == ITEM::SEGMENT_T )
            all_segs.insert( static_cast<SEGMENT*>( *i ) );
    }

    if( !isRoot() )
    {
        for( i = m_root->m_items.begin(); i != m_root->m_items.end(); i++ )
        {
            if( (*i)->GetKind() == ITEM::SEGMENT_T && !overrides( *i ) )
                all_segs.insert( static_cast<SEGMENT*>(*i) );
        }
    }

    JOINT_MAP::iterator j;

    if( aLong )
    {
        for( j = m_joints.begin(); j != m_joints.end(); ++j )
        {
            wxLogTrace( "PNS", "joint : %s, links : %d\n",
                        j->second.GetPos().Format().c_str(), j->second.LinkCount() );
            JOINT::LINKED_ITEMS::const_iterator k;

            for( k = j->second.GetLinkList().begin(); k != j->second.GetLinkList().end(); ++k )
            {
                const ITEM* m_item = *k;

                switch( m_item->GetKind() )
                {
                case ITEM::SEGMENT_T:
                    {
                        const SEGMENT* seg = static_cast<const SEGMENT*>( m_item );
                        wxLogTrace( "PNS", " -> seg %s %s\n", seg->GetSeg().A.Format().c_str(),
                                seg->GetSeg().B.Format().c_str() );
                        break;
                    }

                default:
                    break;
                }
            }
        }
    }

    int lines_count = 0;

    while( !all_segs.empty() )
    {
        SEGMENT* s = *all_segs.begin();
        LINE* l = AssembleLine( s );

        LINE::LinkedSegments* seg_refs = l->GetLinkedSegments();

        if( aLong )
            wxLogTrace( "PNS", "Line: %s, net %d ", l->GetLine().Format().c_str(), l->GetNet() );

        for( std::vector<SEGMENT*>::iterator j = seg_refs->begin(); j != seg_refs->end(); ++j )
        {
            wxLogTrace( "PNS", "%s ", (*j)->GetSeg().A.Format().c_str() );

            if( j + 1 == seg_refs->end() )
                wxLogTrace( "PNS", "%s\n", (*j)->GetSeg().B.Format().c_str() );

            all_segs.erase( *j );
        }

        lines_count++;
    }

    wxLogTrace( "PNS", "Local joints: %d, lines : %d \n", m_joints.size(), lines_count );
#endif
}


void NODE::GetUpdatedItems( ITEM_VECTOR& aRemoved, ITEM_VECTOR& aAdded )
{
    if( isRoot() )
        return;

    if( m_override.size() )
        aRemoved.reserve( m_override.size() );

    if( m_index->Size() )
        aAdded.reserve( m_index->Size() );

    for( ITEM* item : m_override )
        aRemoved.push_back( item );

    for( INDEX::ITEM_SET::iterator i = m_index->begin(); i != m_index->end(); ++i )
        aAdded.push_back( *i );
}


void NODE::releaseChildren()
{
    // copy the kids as the NODE destructor erases the item from the parent node.
    std::set<NODE*> kids = m_children;

    for( NODE* node : kids )
    {
        node->releaseChildren();
        delete node;
    }
}


void NODE::releaseGarbage()
{
    if( !isRoot() )
        return;

    for( ITEM* item : m_garbageItems )
    {
        if( !item->BelongsTo( this ) )
            delete item;
    }

    m_garbageItems.clear();
}


void NODE::Commit( NODE* aNode )
{
    if( aNode->isRoot() )
        return;

    for( ITEM* item : aNode->m_override )
        Remove( item );

    for( ITEM* item : *aNode->m_index )
    {
        item->SetRank( -1 );
        item->Unmark();
        Add( std::unique_ptr<ITEM>( item ) );
    }

    releaseChildren();
    releaseGarbage();
}


void NODE::KillChildren()
{
    releaseChildren();
}


void NODE::AllItemsInNet( int aNet, std::set<ITEM*>& aItems, int aKindMask)
{
    INDEX::NET_ITEMS_LIST* l_cur = m_index->GetItemsForNet( aNet );

    if( l_cur )
    {
        for( ITEM* item : *l_cur )
        {
            if( item->OfKind( aKindMask ) && item->IsRoutable() )
                aItems.insert( item );
        }
    }

    if( !isRoot() )
    {
        INDEX::NET_ITEMS_LIST* l_root = m_root->m_index->GetItemsForNet( aNet );

        if( l_root )
        {
            for( ITEM* item : *l_root )
            {
                if( !Overrides( item ) && item->OfKind( aKindMask ) && item->IsRoutable() )
                    aItems.insert( item );
            }
        }
    }
}


void NODE::ClearRanks( int aMarkerMask )
{
    for( ITEM* item : *m_index )
    {
        item->SetRank( -1 );
        item->Mark( item->Marker() & ~aMarkerMask );
    }
}


void NODE::RemoveByMarker( int aMarker )
{
    std::list<ITEM*> garbage;

    for( ITEM* item : *m_index )
    {
        if( item->Marker() & aMarker )
            garbage.push_back( item );
    }

    for( ITEM* item : garbage )
        Remove( item );
}


SEGMENT* NODE::findRedundantSegment( const VECTOR2I& A, const VECTOR2I& B, const LAYER_RANGE& lr,
                                     int aNet )
{
    JOINT* jtStart = FindJoint( A, lr.Start(), aNet );

    if( !jtStart )
        return nullptr;

    for( ITEM* item : jtStart->LinkList() )
    {
        if( item->OfKind( ITEM::SEGMENT_T ) )
        {
            SEGMENT* seg2 = (SEGMENT*)item;

            const VECTOR2I a2( seg2->Seg().A );
            const VECTOR2I b2( seg2->Seg().B );

            if( seg2->Layers().Start() == lr.Start()
                    && ( ( A == a2 && B == b2 ) || ( A == b2 && B == a2 ) ) )
            {
                return seg2;
            }
        }
    }

    return nullptr;
}


SEGMENT* NODE::findRedundantSegment( SEGMENT* aSeg )
{
    return findRedundantSegment( aSeg->Seg().A, aSeg->Seg().B, aSeg->Layers(), aSeg->Net() );
}


ARC* NODE::findRedundantArc( const VECTOR2I& A, const VECTOR2I& B, const LAYER_RANGE& lr,
                             int aNet )
{
    JOINT* jtStart = FindJoint( A, lr.Start(), aNet );

    if( !jtStart )
        return nullptr;

    for( ITEM* item : jtStart->LinkList() )
    {
        if( item->OfKind( ITEM::ARC_T ) )
        {
            ARC* seg2 = static_cast<ARC*>( item );

            const VECTOR2I a2( seg2->Anchor( 0 ) );
            const VECTOR2I b2( seg2->Anchor( 1 ) );

            if( seg2->Layers().Start() == lr.Start()
                    && ( ( A == a2 && B == b2 ) || ( A == b2 && B == a2 ) ) )
            {
                return seg2;
            }
        }
    }

    return nullptr;
}


ARC* NODE::findRedundantArc( ARC* aArc )
{
    return findRedundantArc( aArc->Anchor( 0 ), aArc->Anchor( 1 ), aArc->Layers(), aArc->Net() );
}


int NODE::QueryJoints( const BOX2I& aBox, std::vector<JOINT*>& aJoints, LAYER_RANGE aLayerMask,
                       int aKindMask )
{
    int n = 0;

    aJoints.clear();

    for( JOINT_MAP::value_type& j : m_joints )
    {
        if( !j.second.Layers().Overlaps( aLayerMask ) )
            continue;

        if( aBox.Contains( j.second.Pos() ) && j.second.LinkCount( aKindMask ) )
        {
            aJoints.push_back( &j.second );
            n++;
        }
    }

    if( isRoot() )
        return n;

    for( JOINT_MAP::value_type& j : m_root->m_joints )
    {
        if( !Overrides( &j.second ) && j.second.Layers().Overlaps( aLayerMask ) )
        {
            if( aBox.Contains( j.second.Pos() ) && j.second.LinkCount( aKindMask ) )
            {
                aJoints.push_back( &j.second );
                n++;
            }
        }
    }

    return n;
}


ITEM *NODE::FindItemByParent( const BOARD_ITEM* aParent )
{
    if( aParent->IsConnected() )
    {
        const BOARD_CONNECTED_ITEM* cItem = static_cast<const BOARD_CONNECTED_ITEM*>( aParent );

        INDEX::NET_ITEMS_LIST* l_cur = m_index->GetItemsForNet( cItem->GetNetCode() );

        if( l_cur )
        {
            for( ITEM* item : *l_cur )
            {
                if( item->Parent() == aParent )
                    return item;
            }
        }
    }

    return NULL;
}

}

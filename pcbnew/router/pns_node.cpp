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

#include <vector>
#include <cassert>

#include <math/vector2d.h>

#include <geometry/seg.h>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_index.h>

#include "trace.h"
#include "pns_item.h"
#include "pns_line.h"
#include "pns_node.h"
#include "pns_via.h"
#include "pns_solid.h"
#include "pns_joint.h"
#include "pns_index.h"
#include "pns_router.h"

using boost::unordered_set;
using boost::unordered_map;

#ifdef DEBUG
static boost::unordered_set<PNS_NODE*> allocNodes;
#endif

PNS_NODE::PNS_NODE()
{
    TRACE( 0, "PNS_NODE::create %p", this );
    m_depth = 0;
    m_root = this;
    m_parent = NULL;
    m_maxClearance = 800000;    // fixme: depends on how thick traces are.
    m_index = new PNS_INDEX;

#ifdef DEBUG
    allocNodes.insert( this );
#endif
}


PNS_NODE::~PNS_NODE()
{
    TRACE( 0, "PNS_NODE::delete %p", this );
    
    if( !m_children.empty() )
    {
        TRACEn( 0, "attempting to free a node that has kids.\n" );
        assert( false );
    }

#ifdef DEBUG
    if( allocNodes.find( this ) == allocNodes.end() )
    {
        TRACEn( 0, "attempting to free an already-free'd node.\n" );
        assert( false );
    }

    allocNodes.erase( this );
#endif

    for( PNS_INDEX::ITEM_SET::iterator i = m_index->begin(); i != m_index->end(); ++i )
    {
        if( (*i)->BelongsTo( this ) )
            delete *i;
    }

    unlinkParent();
    delete m_index;
}


int PNS_NODE::GetClearance( const PNS_ITEM* aA, const PNS_ITEM* aB ) const
{
   return (*m_clearanceFunctor)( aA, aB );
}


PNS_NODE* PNS_NODE::Branch()
{
    PNS_NODE* child = new PNS_NODE;

    TRACE( 0, "PNS_NODE::branch %p (parent %p)", child % this );
    
    m_children.push_back( child );

    child->m_depth = m_depth + 1;
    child->m_parent = this;
    child->m_clearanceFunctor = m_clearanceFunctor;
    child->m_root = isRoot() ? this : m_root;

    // immmediate offspring of the root branch needs not copy anything.
    // For the rest, deep-copy joints, overridden item map and pointers
    // to stored items.
    if( !isRoot() )
    {
        JOINT_MAP::iterator j;

        for( PNS_INDEX::ITEM_SET::iterator i = m_index->begin(); i != m_index->end(); ++i )
            child->m_index->Add( *i );

        child->m_joints = m_joints;
        child->m_override = m_override;
    }

    TRACE( 2, "%d items, %d joints, %d overrides",
            child->m_index->Size() % child->m_joints.size() % child->m_override.size() );

    return child;
}


void PNS_NODE::unlinkParent()
{
    if( isRoot() )
        return;

    for( std::vector<PNS_NODE*>::iterator i = m_parent->m_children.begin();
         i != m_parent->m_children.end(); ++i )
    {
        if( *i == this )
        {
            m_parent->m_children.erase( i );
            return;
        }
    }
}


// function object that visits potential obstacles and performs
// the actual collision refining
struct PNS_NODE::OBSTACLE_VISITOR
{
    ///> node we are searching in (either root or a branch)
    PNS_NODE* m_node;

    ///> node that overrides root entries
    PNS_NODE* m_override;

    ///> list of encountered obstacles
    OBSTACLES& m_tab;

    ///> the item we are looking for collisions with
    const PNS_ITEM* m_item;

    ///> acccepted kinds of colliding items (solids, vias, segments, etc...)
    int m_kindMask;

    ///> max number of hits
    int m_limitCount;

    ///> number of items found so far
    int m_matchCount;

	///> additional clearance 
	int m_extraClearance;

    OBSTACLE_VISITOR( PNS_NODE::OBSTACLES& aTab, const PNS_ITEM* aItem, int aKindMask ) :
        m_tab( aTab ),
        m_item( aItem ),
        m_kindMask( aKindMask ),
        m_limitCount( -1 ),
        m_matchCount( 0 ),
		m_extraClearance( 0 )
    {
	   if( aItem->Kind() == PNS_ITEM::LINE )
            m_extraClearance += static_cast<const PNS_LINE*>( aItem )->Width() / 2;
	}

    void SetCountLimit( int aLimit )
    {
        m_limitCount = aLimit;
    }

    void SetWorld( PNS_NODE* aNode, PNS_NODE* aOverride = NULL )
    {
        m_node = aNode;
        m_override = aOverride;
    }

    bool operator()( PNS_ITEM* aItem )
    {
        if( !aItem->OfKind( m_kindMask ) )
            return true;

        // check if there is a more recent branch with a newer
        // (possibily modified) version of this item.
        if( m_override && m_override->overrides( aItem ) )
            return true;

        int clearance = m_extraClearance + m_node->GetClearance( aItem, m_item );

        if( aItem->Kind() == PNS_ITEM::LINE )
            clearance += static_cast<PNS_LINE *>(aItem)->Width() / 2;

        if( !aItem->Collide( m_item, clearance ) )
            return true;

        PNS_OBSTACLE obs;

        obs.m_item = aItem;
        m_tab.push_back( obs );

        m_matchCount++;

        if( m_limitCount > 0 && m_matchCount >= m_limitCount )
            return false;

        return true;
    };
};


int PNS_NODE::QueryColliding( const PNS_ITEM* aItem,
        PNS_NODE::OBSTACLES& aObstacles, int aKindMask, int aLimitCount )
{
    OBSTACLE_VISITOR visitor( aObstacles, aItem, aKindMask );

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


PNS_NODE::OPT_OBSTACLE PNS_NODE::NearestObstacle( const PNS_LINE* aItem, int aKindMask )
{
    OBSTACLES obs_list;
    bool found_isects = false;

    const SHAPE_LINE_CHAIN& line = aItem->CLine();

    obs_list.reserve( 100 );

    int n = 0;

    for( int i = 0; i < line.SegmentCount(); i++ )
    {
        const PNS_SEGMENT s( *aItem, line.CSegment( i ) );
        n += QueryColliding( &s, obs_list, aKindMask );
    }

    if( aItem->EndsWithVia() )
        n += QueryColliding( &aItem->Via(), obs_list, aKindMask );

    // if(! QueryColliding ( aItem, obs_list, aKindMask ))
    if( !n )
        return OPT_OBSTACLE();

    PNS_LINE& aLine = (PNS_LINE&) *aItem;

    PNS_OBSTACLE nearest;
    nearest.m_item = NULL;
    nearest.m_distFirst = INT_MAX;

    BOOST_FOREACH( PNS_OBSTACLE obs, obs_list )
    {
        VECTOR2I ip_first, ip_last;
        int dist_max = INT_MIN;

        std::vector<SHAPE_LINE_CHAIN::INTERSECTION> isect_list;

        int clearance = GetClearance( obs.m_item, &aLine );

        SHAPE_LINE_CHAIN hull = obs.m_item->Hull( clearance, aItem->Width() );

        if( aLine.EndsWithVia() )
        {
            int clearance = GetClearance( obs.m_item, &aLine.Via() );

            SHAPE_LINE_CHAIN viaHull = aLine.Via().Hull( clearance, aItem->Width() );

            viaHull.Intersect( hull, isect_list );

            BOOST_FOREACH( SHAPE_LINE_CHAIN::INTERSECTION isect, isect_list )
            {
                int dist = aLine.CLine().Length() +
                           ( isect.p - aLine.Via().Pos() ).EuclideanNorm();

                if( dist < nearest.m_distFirst )
                {
                    found_isects = true;
                    nearest.m_distFirst = dist;
                    nearest.m_ipFirst = isect.p;
                    nearest.m_item = obs.m_item;
                    nearest.m_hull = hull;
                }

                if( dist > dist_max )
                {
                    dist_max = dist;
                    ip_last = isect.p;
                }
            }
        }

        isect_list.clear();

        hull.Intersect( aLine.CLine(), isect_list );

        BOOST_FOREACH( SHAPE_LINE_CHAIN::INTERSECTION isect, isect_list )
        {
            int dist = aLine.CLine().PathLength( isect.p );

            if( dist < nearest.m_distFirst )
            {
                found_isects = true;
                nearest.m_distFirst = dist;
                nearest.m_ipFirst = isect.p;
                nearest.m_item = obs.m_item;
                nearest.m_hull = hull;
            }

            if( dist > dist_max )
            {
                dist_max = dist;
                ip_last = isect.p;
            }
        }

        nearest.m_ipLast = ip_last;
        nearest.m_distLast = dist_max;
    }

    if( !found_isects )
        nearest.m_item = obs_list[0].m_item;

    return nearest;
}


PNS_NODE::OPT_OBSTACLE PNS_NODE::CheckColliding( const PNS_ITEMSET& aSet, int aKindMask )
{
    BOOST_FOREACH( const PNS_ITEM* item, aSet.CItems() )
    {
        OPT_OBSTACLE obs = CheckColliding( item, aKindMask );

        if( obs )
            return  obs;
    }

    return OPT_OBSTACLE();
}


PNS_NODE::OPT_OBSTACLE PNS_NODE::CheckColliding( const PNS_ITEM* aItemA, int aKindMask )
{
    OBSTACLES obs;

    obs.reserve( 100 );

    if( aItemA->Kind() == PNS_ITEM::LINE )
    {
        int n = 0;
        const PNS_LINE* line = static_cast<const PNS_LINE*>( aItemA );
        const SHAPE_LINE_CHAIN& l = line->CLine();

        for( int i = 0; i < l.SegmentCount(); i++ )
        {
            const PNS_SEGMENT s( *line, l.CSegment( i ) );
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
        return OPT_OBSTACLE( obs[0] );

    return OPT_OBSTACLE();
}


bool PNS_NODE::CheckColliding( const PNS_ITEM* aItemA, const PNS_ITEM* aItemB, int aKindMask )
{
    assert( aItemB );
    int clearance = GetClearance( aItemA, aItemB );

	// fixme: refactor
    if( aItemA->Kind() == PNS_ITEM::LINE )
        clearance += static_cast<const PNS_LINE*>( aItemA )->Width() / 2;
    if( aItemB->Kind() == PNS_ITEM::LINE )
        clearance += static_cast<const PNS_LINE*>( aItemB )->Width() / 2;

    return aItemA->Collide( aItemB, clearance );
}


struct HIT_VISITOR
{
    PNS_ITEMSET& m_items;
    const VECTOR2I& m_point;
    const PNS_NODE* m_world;

    HIT_VISITOR( PNS_ITEMSET& aTab, const VECTOR2I& aPoint, const PNS_NODE* aWorld ) :
        m_items( aTab ), m_point( aPoint ), m_world( aWorld )
    {}

    bool operator()( PNS_ITEM* aItem )
    {
        SHAPE_CIRCLE cp( m_point, 0 );

        int cl = 0;

        if( aItem->Shape()->Collide( &cp, cl ) )
            m_items.Add( aItem );

        return true;
    }
};


const PNS_ITEMSET PNS_NODE::HitTest( const VECTOR2I& aPoint ) const
{
    PNS_ITEMSET items;

    // fixme: we treat a point as an infinitely small circle - this is inefficient.
    SHAPE_CIRCLE s( aPoint, 0 );
    HIT_VISITOR visitor( items, aPoint, this );

    m_index->Query( &s, m_maxClearance, visitor );

    if( !isRoot() )    // fixme: could be made cleaner
    {
        PNS_ITEMSET items_root;
        HIT_VISITOR  visitor_root( items_root, aPoint, m_root );
        m_root->m_index->Query( &s, m_maxClearance, visitor_root );

        BOOST_FOREACH( PNS_ITEM* item, items_root.Items() )
        {
            if( !overrides( item ) )
                items.Add( item );
        }
    }

    return items;
}


void PNS_NODE::addSolid( PNS_SOLID* aSolid )
{
    linkJoint( aSolid->Pos(), aSolid->Layers(), aSolid->Net(), aSolid );
    m_index->Add( aSolid );
}


void PNS_NODE::addVia( PNS_VIA* aVia )
{
    linkJoint( aVia->Pos(), aVia->Layers(), aVia->Net(), aVia );
    m_index->Add( aVia );
}


void PNS_NODE::addLine( PNS_LINE* aLine, bool aAllowRedundant )
{
    SHAPE_LINE_CHAIN& l = aLine->Line();

    for( int i = 0; i < l.SegmentCount(); i++ )
    {
        SEG s = l.CSegment( i );

        if( s.A != s.B )
        {
            PNS_SEGMENT* pseg = new PNS_SEGMENT( *aLine, s );
            PNS_SEGMENT* psegR = NULL;

            if ( !aAllowRedundant )
                psegR =  findRedundantSegment( pseg );

            if( psegR )
                aLine->LinkSegment( psegR );
            else
            {
                pseg->SetOwner( this );

                linkJoint( s.A, pseg->Layers(), aLine->Net(), pseg );
                linkJoint( s.B, pseg->Layers(), aLine->Net(), pseg );

                aLine->LinkSegment( pseg );

                m_index->Add( pseg );
            }
        } 
    }
}


void PNS_NODE::addSegment( PNS_SEGMENT* aSeg, bool aAllowRedundant )
{
    if( aSeg->Seg().A == aSeg->Seg().B )
    {
        TRACEn( 0, "attempting to add a segment with same end coordinates, ignoring." )
        return;
    }

   if( !aAllowRedundant && findRedundantSegment ( aSeg ) )
        return;
    
    aSeg->SetOwner( this );

    linkJoint( aSeg->Seg().A, aSeg->Layers(), aSeg->Net(), aSeg );
    linkJoint( aSeg->Seg().B, aSeg->Layers(), aSeg->Net(), aSeg );

    m_index->Add( aSeg );
}


void PNS_NODE::Add( PNS_ITEM* aItem, bool aAllowRedundant )
{
    aItem->SetOwner( this );

    switch( aItem->Kind() )
    {
    case PNS_ITEM::SOLID:
        addSolid( static_cast<PNS_SOLID*>( aItem ) );
        break;

    case PNS_ITEM::SEGMENT:
        addSegment( static_cast<PNS_SEGMENT*>( aItem ), aAllowRedundant );
        break;

    case PNS_ITEM::LINE:
        addLine( static_cast<PNS_LINE*>( aItem ), aAllowRedundant );
        break;

    case PNS_ITEM::VIA:
        addVia( static_cast<PNS_VIA*>( aItem ) );
        break;

    default:
        assert( false );
    }
}


void PNS_NODE::doRemove( PNS_ITEM* aItem )
{
 //   assert(m_root->m_index->Contains(aItem) || m_index->Contains(aItem));

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
        aItem->SetOwner( NULL );
}


void PNS_NODE::removeSegment( PNS_SEGMENT* aSeg )
{
    unlinkJoint( aSeg->Seg().A, aSeg->Layers(), aSeg->Net(), aSeg );
    unlinkJoint( aSeg->Seg().B, aSeg->Layers(), aSeg->Net(), aSeg );

    doRemove( aSeg );
}


void PNS_NODE::removeLine( PNS_LINE* aLine )
{
    std::vector<PNS_SEGMENT*>* segRefs = aLine->LinkedSegments();

    if(! aLine->SegmentCount() )
        return;

    assert (segRefs != NULL);
    assert (aLine->Owner());
    
    if ( (int) segRefs->size() != aLine->SegmentCount() )
    {
        //printf("******weird deletion: segrefs %d segcount %d hasloops %d\n", segRefs->size(), aLine->SegmentCount(), aLine->HasLoops());
    }

    BOOST_FOREACH( PNS_SEGMENT* seg, *segRefs )
    {
        removeSegment( seg );
    }

    aLine->SetOwner( NULL );
    aLine->ClearSegmentLinks();
}


void PNS_NODE::removeVia( PNS_VIA* aVia )
{
    // We have to split a single joint (associated with a via, binding together multiple layers)
    // into multiple independent joints. As I'm a lazy bastard, I simply delete the via and all its links and re-insert them.

    PNS_JOINT::HASH_TAG tag;

    VECTOR2I p( aVia->Pos() );
    PNS_LAYERSET vLayers( aVia->Layers() );
    int net = aVia->Net();

    PNS_JOINT* jt = FindJoint( p, vLayers.Start(), net );
    PNS_JOINT::LINKED_ITEMS links( jt->LinkList() );

    tag.net = net;
    tag.pos = p;

    bool split; 
    do
    {
        split = false;
        std::pair<JOINT_MAP::iterator, JOINT_MAP::iterator> range = m_joints.equal_range( tag );

        if( range.first == m_joints.end() )
            break;

        // find and remove all joints containing the via to be removed

        for( JOINT_MAP::iterator f = range.first; f != range.second; ++f )
        {
            if( aVia->LayersOverlap ( &f->second ) )
            {
                m_joints.erase( f );
                split = true;
                break;
            }
        }
    } while( split );
    
    // and re-link them, using the former via's link list
    BOOST_FOREACH(PNS_ITEM* item, links)
    {
        if( item != aVia )
            linkJoint ( p, item->Layers(), net, item );
    }

    doRemove( aVia );  
}


void PNS_NODE::Replace( PNS_ITEM* aOldItem, PNS_ITEM* aNewItem )
{
    Remove( aOldItem );
    Add( aNewItem );
}


void PNS_NODE::Remove( PNS_ITEM* aItem )
{
    switch( aItem->Kind() )
    {
    case PNS_ITEM::SOLID:
        // fixme: this fucks up the joints, but it's only used for marking colliding obstacles for the moment, so we don't care.
        doRemove ( aItem );
        break;

    case PNS_ITEM::SEGMENT:
        removeSegment( static_cast<PNS_SEGMENT*>( aItem ) );
        break;

    case PNS_ITEM::LINE:
        removeLine( static_cast<PNS_LINE*>( aItem ) );
        break;

    case PNS_ITEM::VIA:
        removeVia( static_cast<PNS_VIA*>( aItem ) );
        break;

    default:
        break;
    }
}


void PNS_NODE::followLine( PNS_SEGMENT* aCurrent, bool aScanDirection, int& aPos,
        int aLimit, VECTOR2I* aCorners, PNS_SEGMENT** aSegments, bool& aGuardHit )
{
    bool prevReversed = false;

    const VECTOR2I guard = aScanDirection ? aCurrent->Seg().B : aCurrent->Seg().A;

    for( int count = 0 ; ; ++count )
    {
        const VECTOR2I p =
            ( aScanDirection ^ prevReversed ) ? aCurrent->Seg().B : aCurrent->Seg().A;
        const PNS_JOINT* jt = FindJoint( p, aCurrent );

        assert( jt );

        aCorners[aPos] = jt->Pos();

        if( count && guard == p )
        {
            aSegments[aPos] = NULL;
            aGuardHit = true;
            break;
        }

        aSegments[aPos] = aCurrent;
        
        aPos += ( aScanDirection ? 1 : -1 );

        if( !jt->IsLineCorner() || aPos < 0 || aPos == aLimit )
            break;

        aCurrent = jt->NextSegment( aCurrent );

        prevReversed =
            ( jt->Pos() == (aScanDirection ? aCurrent->Seg().B : aCurrent->Seg().A ) );
    }
}


PNS_LINE* PNS_NODE::AssembleLine( PNS_SEGMENT* aSeg, int* aOriginSegmentIndex)
{
    const int MaxVerts = 1024;

    VECTOR2I corners[MaxVerts + 1];
    PNS_SEGMENT* segs[MaxVerts + 1];

    PNS_LINE* pl = new PNS_LINE;
    bool guardHit = false;

    int i_start = MaxVerts / 2, i_end = i_start + 1;

    pl->SetWidth( aSeg->Width() );
    pl->SetLayers( aSeg->Layers() );
    pl->SetNet( aSeg->Net() );
    pl->SetOwner( this );

    followLine( aSeg, false, i_start, MaxVerts, corners, segs, guardHit );
    
    if( !guardHit )
        followLine( aSeg, true, i_end, MaxVerts, corners, segs, guardHit );
    
    int n = 0;

    PNS_SEGMENT* prev_seg = NULL;

    for( int i = i_start + 1; i < i_end; i++ )
    {
        const VECTOR2I& p = corners[i];

        pl->Line().Append( p );
            
        if( segs[i] && prev_seg != segs[i] )
        {
            pl->LinkSegment( segs[i] );

            if( segs[i] == aSeg && aOriginSegmentIndex )
                *aOriginSegmentIndex = n;

            n++;
        }

        prev_seg = segs[i];
    }

    assert( pl->SegmentCount() != 0 );
    assert( pl->SegmentCount() == (int) pl->LinkedSegments()->size() );
    
    return pl;
}


void PNS_NODE::FindLineEnds( PNS_LINE* aLine, PNS_JOINT& aA, PNS_JOINT& aB )
{
    aA = *FindJoint( aLine->CPoint( 0 ), aLine );
    aB = *FindJoint( aLine->CPoint( -1 ), aLine );
}


void PNS_NODE::MapConnectivity ( PNS_JOINT* aStart, std::vector<PNS_JOINT*>& aFoundJoints )
{
    std::deque<PNS_JOINT*> searchQueue;
    std::set<PNS_JOINT*> processed;

    searchQueue.push_back( aStart );
    processed.insert( aStart );

    while( !searchQueue.empty() )
    {
        PNS_JOINT* current = searchQueue.front();
        searchQueue.pop_front();

        BOOST_FOREACH( PNS_ITEM* item, current->LinkList() )
        {
            if ( item->OfKind( PNS_ITEM::SEGMENT ) )
            {
                PNS_SEGMENT* seg = static_cast<PNS_SEGMENT *>( item );
                PNS_JOINT* a = FindJoint( seg->Seg().A, seg );
                PNS_JOINT* b = FindJoint( seg->Seg().B, seg );
                PNS_JOINT* next = ( *a == *current ) ? b : a;

                if( processed.find( next ) == processed.end() )
                {
                    processed.insert( next );
                    searchQueue.push_back( next );
                }
            } 
        }
    }

    BOOST_FOREACH(PNS_JOINT* jt, processed)
        aFoundJoints.push_back( jt );
}


PNS_ITEM* PNS_NODE::NearestUnconnectedItem( PNS_JOINT* aStart, int* aAnchor, int aKindMask )
{
    std::set<PNS_ITEM*> disconnected;
    std::vector<PNS_JOINT*> joints;

    AllItemsInNet( aStart->Net(), disconnected );
    MapConnectivity ( aStart, joints );

    BOOST_FOREACH( PNS_JOINT *jt, joints )
    {
        BOOST_FOREACH( PNS_ITEM* link, jt->LinkList() )
        {
            if( disconnected.find( link ) != disconnected.end() )
                disconnected.erase( link );
        }    
    }

    int best_dist = INT_MAX;
    PNS_ITEM* best = NULL;

    BOOST_FOREACH( PNS_ITEM* item, disconnected )
    {
        if( item->OfKind( aKindMask ) )
        {
            for(int i = 0; i < item->AnchorCount(); i++) 
            {
                VECTOR2I p = item->Anchor( i );
                int d = ( p - aStart->Pos() ).EuclideanNorm();

                if( d < best_dist )
                {
                    best_dist = d;
                    best = item;

                    if( aAnchor )
                        *aAnchor = i;
                }
            }
        }
    }

    return best;
}


int PNS_NODE::FindLinesBetweenJoints( PNS_JOINT& aA, PNS_JOINT& aB, std::vector<PNS_LINE*>& aLines )
{
    BOOST_FOREACH( PNS_ITEM* item, aA.LinkList() )
    {
        if( item->Kind() == PNS_ITEM::SEGMENT )
        {
            PNS_SEGMENT* seg = static_cast<PNS_SEGMENT*>( item );
            PNS_LINE* line = AssembleLine( seg );

            PNS_JOINT j_start, j_end;
            
            FindLineEnds( line, j_start, j_end );

            int id_start = line->CLine().Find( aA.Pos() );
            int id_end   = line->CLine().Find( aB.Pos() );

            if( id_end < id_start )
                std::swap( id_end, id_start );

            if( id_start >= 0 && id_end >= 0 )
            {
                line->ClipVertexRange ( id_start, id_end );
                aLines.push_back( line );
            }
            else
                delete line;
        }
    }

    return 0;
}


PNS_JOINT* PNS_NODE::FindJoint( const VECTOR2I& aPos, int aLayer, int aNet )
{
    PNS_JOINT::HASH_TAG tag;

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


PNS_JOINT& PNS_NODE::touchJoint( const VECTOR2I& aPos, const PNS_LAYERSET& aLayers, int aNet )
{
    PNS_JOINT::HASH_TAG tag;

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
    PNS_JOINT jt( aPos, aLayers, aNet );

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


void PNS_JOINT::Dump() const
{
    printf( "joint layers %d-%d, net %d, pos %s, links: %d\n", m_layers.Start(),
            m_layers.End(), m_tag.net, m_tag.pos.Format().c_str(), LinkCount() );
}


void PNS_NODE::linkJoint( const VECTOR2I& aPos, const PNS_LAYERSET& aLayers,
                          int aNet, PNS_ITEM* aWhere )
{
    PNS_JOINT& jt = touchJoint( aPos, aLayers, aNet );

    jt.Link( aWhere );
}


void PNS_NODE::unlinkJoint( const VECTOR2I& aPos, const PNS_LAYERSET& aLayers,
                            int aNet, PNS_ITEM* aWhere )
{
    // fixme: remove dangling joints
    PNS_JOINT& jt = touchJoint( aPos, aLayers, aNet );

    jt.Unlink( aWhere );
}


void PNS_NODE::Dump( bool aLong )
{
#if 0
    boost::unordered_set<PNS_SEGMENT*> all_segs;
    SHAPE_INDEX_LIST<PNS_ITEM*>::iterator i;

    for( i = m_items.begin(); i != m_items.end(); i++ )
    {
        if( (*i)->GetKind() == PNS_ITEM::SEGMENT )
            all_segs.insert( static_cast<PNS_SEGMENT*>( *i ) );
    }

    if( !isRoot() )
	{
        for( i = m_root->m_items.begin(); i != m_root->m_items.end(); i++ )
        {
            if( (*i)->GetKind() == PNS_ITEM::SEGMENT && !overrides( *i ) )
                all_segs.insert( static_cast<PNS_SEGMENT*>(*i) );
        }
	}

    JOINT_MAP::iterator j;

    if( aLong )
        for( j = m_joints.begin(); j != m_joints.end(); ++j )
        {
            printf( "joint : %s, links : %d\n",
                    j->second.GetPos().Format().c_str(), j->second.LinkCount() );
            PNS_JOINT::LINKED_ITEMS::const_iterator k;

            for( k = j->second.GetLinkList().begin(); k != j->second.GetLinkList().end(); ++k )
            {
                const PNS_ITEM* m_item = *k;

                switch( m_item->GetKind() )
                {
                case PNS_ITEM::SEGMENT:
                    {
                        const PNS_SEGMENT* seg = static_cast<const PNS_SEGMENT*>( m_item );
                        printf( " -> seg %s %s\n", seg->GetSeg().A.Format().c_str(),
                                seg->GetSeg().B.Format().c_str() );
                        break;
                    }

                default:
                    break;
                }
            }
        }


    int lines_count = 0;

    while( !all_segs.empty() )
    {
        PNS_SEGMENT* s = *all_segs.begin();
        PNS_LINE* l = AssembleLine( s );

        PNS_LINE::LinkedSegments* seg_refs = l->GetLinkedSegments();

        if( aLong )
            printf( "Line: %s, net %d ", l->GetLine().Format().c_str(), l->GetNet() );

        for( std::vector<PNS_SEGMENT*>::iterator j = seg_refs->begin(); j != seg_refs->end(); ++j )
        {
            printf( "%s ", (*j)->GetSeg().A.Format().c_str() );

            if( j + 1 == seg_refs->end() )
                printf( "%s\n", (*j)->GetSeg().B.Format().c_str() );

            all_segs.erase( *j );
        }

        lines_count++;
    }

    printf( "Local joints: %d, lines : %d \n", m_joints.size(), lines_count );
#endif
}


void PNS_NODE::GetUpdatedItems( ITEM_VECTOR& aRemoved, ITEM_VECTOR& aAdded )
{
    aRemoved.reserve( m_override.size() );
    aAdded.reserve( m_index->Size() );

    if( isRoot() )
        return;

    BOOST_FOREACH( PNS_ITEM* item, m_override )
    aRemoved.push_back( item );

    for( PNS_INDEX::ITEM_SET::iterator i = m_index->begin(); i != m_index->end(); ++i )
        aAdded.push_back( *i );
}


void PNS_NODE::releaseChildren()
{
    // copy the kids as the PNS_NODE destructor erases the item from the parent node.
    std::vector<PNS_NODE*> kids = m_children;

    BOOST_FOREACH( PNS_NODE * node, kids )
    {
        node->releaseChildren();
        delete node;
    }
}


void PNS_NODE::Commit( PNS_NODE* aNode )
{
    if( aNode->isRoot() )
        return;

    BOOST_FOREACH( PNS_ITEM * item, aNode->m_override )
    Remove( item );

    for( PNS_INDEX::ITEM_SET::iterator i = aNode->m_index->begin();
         i != aNode->m_index->end(); ++i )
    {
        (*i)->SetRank( -1 );
        (*i)->Unmark();
        Add( *i );
    }

    releaseChildren();
}


void PNS_NODE::KillChildren()
{
    assert ( isRoot() );
    releaseChildren();
}


void PNS_NODE::AllItemsInNet( int aNet, std::set<PNS_ITEM*>& aItems )
{
    PNS_INDEX::NET_ITEMS_LIST* l_cur = m_index->GetItemsForNet( aNet );

    if( l_cur )
    {
        BOOST_FOREACH( PNS_ITEM*item, *l_cur )
            aItems.insert( item );
    }
    
    if( !isRoot() )
    {
        PNS_INDEX::NET_ITEMS_LIST* l_root = m_root->m_index->GetItemsForNet( aNet );

        if( l_root )
            for( PNS_INDEX::NET_ITEMS_LIST::iterator i = l_root->begin(); i!= l_root->end(); ++i )
                if( !overrides( *i ) )
                    aItems.insert( *i );
    }
}


void PNS_NODE::ClearRanks()
{
        for( PNS_INDEX::ITEM_SET::iterator i = m_index->begin(); i != m_index->end(); ++i )
        {
            (*i)->SetRank( -1 );
            (*i)->Mark( 0 );
        }
}


int PNS_NODE::FindByMarker( int aMarker, PNS_ITEMSET& aItems )
{
    for( PNS_INDEX::ITEM_SET::iterator i = m_index->begin(); i != m_index->end(); ++i )
    {
        if( (*i)->Marker() & aMarker )
            aItems.Add( *i );
    }

    return 0;
}


int PNS_NODE::RemoveByMarker( int aMarker )
{
    for( PNS_INDEX::ITEM_SET::iterator i = m_index->begin(); i != m_index->end(); ++i )
    {
        if ( (*i)->Marker() & aMarker )
        {
            Remove( *i );
        }
    }

    return 0;
}


PNS_SEGMENT* PNS_NODE::findRedundantSegment ( PNS_SEGMENT *aSeg )
{
    PNS_JOINT* jtStart = FindJoint ( aSeg->Seg().A, aSeg );

    if( !jtStart )
        return NULL;

    BOOST_FOREACH( PNS_ITEM* item, jtStart->LinkList() )
    {
        if( item->OfKind( PNS_ITEM::SEGMENT ) )
        {
            PNS_SEGMENT* seg2 = (PNS_SEGMENT*) item;
            
            const VECTOR2I a1( aSeg->Seg().A );
            const VECTOR2I b1( aSeg->Seg().B );
            
            const VECTOR2I a2( seg2->Seg().A );
            const VECTOR2I b2( seg2->Seg().B );
            
            if( seg2->Layers().Start() == aSeg->Layers().Start() && 
                ( ( a1 == a2 && b1 == b2 ) || ( a1 == b2 && a2 == b1 ) ) )
                    return seg2;
        }
    }
    
    return NULL;
}

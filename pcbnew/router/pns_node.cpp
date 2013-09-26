/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
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

using namespace std;

using boost::unordered_set;
using boost::unordered_map;

static boost::unordered_set<PNS_NODE*> allocNodes;

PNS_NODE::PNS_NODE()
{
    // printf("MakeNode [%p, total = %d]\n", this, allocNodes.size());
    m_root = this;
    m_parent = NULL;
    m_maxClearance = 800000;    // fixme: depends on how thick traces are.
    m_index = new PNS_INDEX;
    allocNodes.insert( this );
}


PNS_NODE::~PNS_NODE()
{
    if( !m_children.empty() )
    {
        TRACEn( 0, "attempting to free a node that has kids.\n" );
        assert( false );
    }

    if( allocNodes.find( this ) == allocNodes.end() )
    {
        TRACEn( 0, "attempting to free an already-free'd node.\n" );
        assert( false );
    }

    allocNodes.erase( this );

    for( PNS_INDEX::ItemSet::iterator i = m_index->begin(); 
            i != m_index->end(); ++i )
        if( (*i)->BelongsTo( this ) )
            delete *i;

    unlinkParent();
    delete m_index;
}


int PNS_NODE::GetClearance( const PNS_ITEM* a, const PNS_ITEM* b ) const
{
    int clearance = (*m_clearanceFunctor)( a, b );

    if( a->OfKind( PNS_ITEM::SEGMENT ) )
        clearance += static_cast<const PNS_SEGMENT*>(a)->GetWidth() / 2;

    if( a->OfKind( PNS_ITEM::LINE ) )
        clearance += static_cast<const PNS_LINE*>(a)->GetWidth() / 2;

    if( b->OfKind( PNS_ITEM::SEGMENT ) )
        clearance += static_cast<const PNS_SEGMENT*>(b)->GetWidth() / 2;

    if( b->OfKind( PNS_ITEM::LINE ) )
        clearance += static_cast<const PNS_LINE*>(b)->GetWidth() / 2;

    return clearance;
}


PNS_NODE* PNS_NODE::Branch()
{
    PNS_NODE* child = new PNS_NODE;

    m_children.push_back( child );

    child->m_parent = this;
    child->m_clearanceFunctor = m_clearanceFunctor;
    child->m_root = isRoot() ? this : m_root;

    // immmediate offspring of the root branch needs not copy anything. 
    // For the rest, deep-copy joints, overridden item map and pointers 
    // to stored items.
    if( !isRoot() )
    {
        JointMap::iterator j;

        for( PNS_INDEX::ItemSet::iterator i = m_index->begin(); 
                i != m_index->end(); ++i )
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

    for( vector<PNS_NODE*>::iterator i = m_parent->m_children.begin();
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
struct PNS_NODE::obstacleVisitor
{
    ///> node we are searching in (either root or a branch)
    PNS_NODE* m_node;

    ///> node that overrides root entries
    PNS_NODE* m_override;

    ///> list of encountered obstacles
    Obstacles& m_tab;

    ///> the item we are looking for collisions with
    const PNS_ITEM* m_item;

    ///> acccepted kinds of colliding items (solids, vias, segments, etc...)
    int m_kindMask;

    ///> max number of hits
    int m_limitCount;

    ///> number of items found so far
    int m_matchCount;

    obstacleVisitor( PNS_NODE::Obstacles& aTab, const PNS_ITEM* aItem,
            int aKindMask ) :
        m_tab( aTab ),
        m_item( aItem ),
        m_kindMask( aKindMask ),
        m_limitCount( -1 ),
        m_matchCount( 0 )
    {};

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

        int clearance = m_node->GetClearance( aItem, m_item );

        if( !aItem->Collide( m_item, clearance ) )
            return true;

        PNS_OBSTACLE obs;

        obs.item = aItem;
        m_tab.push_back( obs );

        m_matchCount++;

        if( m_limitCount > 0 && m_matchCount >= m_limitCount )
            return false;

        return true;
    };
};


int PNS_NODE::QueryColliding( const PNS_ITEM* aItem,
        PNS_NODE::Obstacles& aObstacles, int aKindMask, int aLimitCount )
{
    obstacleVisitor visitor( aObstacles, aItem, aKindMask );

    assert( allocNodes.find( this ) != allocNodes.end() );

    visitor.SetCountLimit( aLimitCount );
    visitor.SetWorld( this, NULL );

    // first, look for colliding items ourselves
    m_index->Query( aItem, m_maxClearance, visitor );

    // if we haven't found enough items, look in the root branch as well.
    if( !isRoot() && ( visitor.m_matchCount < aLimitCount || aLimitCount < 0) )
    {
        visitor.SetWorld( m_root, this );
        m_root->m_index->Query( aItem, m_maxClearance, visitor );
    }

    return aObstacles.size();
}


PNS_NODE::OptObstacle PNS_NODE::NearestObstacle( const PNS_LINE* aItem, int aKindMask )
{
    Obstacles obs_list;
    bool found_isects = false;

    const SHAPE_LINE_CHAIN& line = aItem->GetCLine();

    obs_list.reserve( 100 );

    int n = 0;

    for( int i = 0; i < line.SegmentCount(); i++ )
    {
        const PNS_SEGMENT s( *aItem, line.CSegment( i ) );
        n += QueryColliding( &s, obs_list, aKindMask );
    }

    if( aItem->EndsWithVia() )
        n += QueryColliding( &aItem->GetVia(), obs_list, aKindMask );

    // if(! QueryColliding ( aItem, obs_list, aKindMask ))
    if( !n )
        return OptObstacle();

    PNS_LINE& aLine = (PNS_LINE&) *aItem;

    PNS_OBSTACLE nearest;
    nearest.item = NULL;
    nearest.dist_first = INT_MAX;

    BOOST_FOREACH( PNS_OBSTACLE obs, obs_list )
    {
        VECTOR2I ip_first, ip_last;
        int dist_max = INT_MIN;

        vector<SHAPE_LINE_CHAIN::Intersection> isect_list;

        int clearance = GetClearance( obs.item, &aLine );

        SHAPE_LINE_CHAIN hull = obs.item->Hull( clearance );

        if( aLine.EndsWithVia() )
        {
            int clearance = GetClearance( obs.item, &aLine.GetVia() );

            SHAPE_LINE_CHAIN viaHull = aLine.GetVia().Hull( clearance );

            viaHull.Intersect( hull, isect_list );

            BOOST_FOREACH( SHAPE_LINE_CHAIN::Intersection isect, isect_list )
            {
                int dist = aLine.GetCLine().Length() +
                           ( isect.p - aLine.GetVia().GetPos() ).EuclideanNorm();

                if( dist < nearest.dist_first )
                {
                    found_isects = true;
                    nearest.dist_first = dist;
                    nearest.ip_first    = isect.p;
                    nearest.item    = obs.item;
                    nearest.hull    = hull;
                }

                if( dist > dist_max )
                {
                    dist_max = dist;
                    ip_last = isect.p;
                }
            }
        }

        isect_list.clear();

        hull.Intersect( aLine.GetCLine(), isect_list );

        BOOST_FOREACH( SHAPE_LINE_CHAIN::Intersection isect, isect_list )
        {
            int dist = aLine.GetCLine().PathLength( isect.p );

            if( dist < nearest.dist_first )
            {
                found_isects = true;
                nearest.dist_first = dist;
                nearest.ip_first = isect.p;
                nearest.item = obs.item;
                nearest.hull = hull;
            }

            if( dist > dist_max )
            {
                dist_max = dist;
                ip_last = isect.p;
            }
        }

        nearest.ip_last = ip_last;
        nearest.dist_last = dist_max;
    }

    return found_isects ? nearest : OptObstacle();
}


PNS_NODE::OptObstacle PNS_NODE::CheckColliding( const PNS_ITEM* aItemA, int aKindMask )
{
    Obstacles obs;

    obs.reserve( 100 );

    if( aItemA->GetKind() == PNS_ITEM::LINE )
    {
        int n = 0;
        const PNS_LINE* line = static_cast<const PNS_LINE*>(aItemA);
        const SHAPE_LINE_CHAIN& l = line->GetCLine();

        for( int i = 0; i < l.SegmentCount(); i++ )
        {
            const PNS_SEGMENT s( *line, l.CSegment( i ) );
            n += QueryColliding( &s, obs, aKindMask, 1 );

            if( n )
                return OptObstacle( obs[0] );
        }

        if( line->EndsWithVia() )
        {
            n += QueryColliding( &line->GetVia(), obs, aKindMask, 1 );

            if( n )
                return OptObstacle( obs[0] );
        }
    }
    else if( QueryColliding( aItemA, obs, aKindMask, 1 ) > 0 )
        return OptObstacle( obs[0] );

    return OptObstacle();
}


bool PNS_NODE::CheckColliding( const PNS_ITEM* aItemA, const PNS_ITEM* aItemB, int aKindMask )
{
    Obstacles dummy;

    assert( aItemB );
    // return QueryColliding(aItemA, dummy, aKindMask, 1) > 0;

    return aItemA->Collide( aItemB, GetClearance( aItemA, aItemB ) );
}


struct hitVisitor
{
    PNS_ITEMSET& m_items;
    const VECTOR2I& m_point;
    PNS_NODE* m_world;

    hitVisitor( PNS_ITEMSET& aTab, const VECTOR2I& aPoint, PNS_NODE* aWorld ) :
        m_items( aTab ), m_point( aPoint ), m_world( aWorld ) {};

    bool operator()( PNS_ITEM* aItem )
    {
        SHAPE_CIRCLE cp( m_point, 0 );

        int cl = 0;

        if( aItem->GetKind() == PNS_ITEM::SEGMENT )
            cl += static_cast<PNS_SEGMENT*>(aItem)->GetWidth() / 2;

        if( aItem->GetShape()->Collide( &cp, cl ) )
            m_items.Add( aItem );

        return true;
    }
};


const PNS_ITEMSET PNS_NODE::HitTest( const VECTOR2I& aPoint )
{
    PNS_ITEMSET items;
    // fixme: we treat a point as an infinitely small circle - this is inefficient. 
    SHAPE_CIRCLE s( aPoint, 0 ); 
    hitVisitor visitor( items, aPoint, this );

    m_index->Query( &s, m_maxClearance, visitor );

    if( !isRoot() )    // fixme: could be made cleaner
    {
        PNS_ITEMSET items_root;
        hitVisitor  visitor_root( items_root, aPoint, m_root );
        m_root->m_index->Query( &s, m_maxClearance, visitor_root );

        BOOST_FOREACH( PNS_ITEM * item, items_root.Items() )
        {
            if( !overrides( item ) )
                items.Add( item );
        }
    }

    return items;
}


void PNS_NODE::addSolid( PNS_SOLID* aSolid )
{
    linkJoint( aSolid->GetCenter(), aSolid->GetLayers(), aSolid->GetNet(), aSolid );
    m_index->Add( aSolid );
}


void PNS_NODE::addVia( PNS_VIA* aVia )
{
    linkJoint( aVia->GetPos(), aVia->GetLayers(), aVia->GetNet(), aVia );
    m_index->Add( aVia );
}


void PNS_NODE::addLine( PNS_LINE* aLine )
{
    const SHAPE_LINE_CHAIN& l = aLine->GetLine();

    for( int i = 0; i < l.SegmentCount(); i++ )
    {
        SEG s = l.CSegment( i );

        if( s.a != s.b )
        {
            PNS_SEGMENT* pseg = new PNS_SEGMENT( *aLine, s );

            pseg->SetOwner( this );

            linkJoint( s.a, pseg->GetLayers(), aLine->GetNet(), pseg );
            linkJoint( s.b, pseg->GetLayers(), aLine->GetNet(), pseg );

            aLine->LinkSegment( pseg );

            m_index->Add( pseg );
        }
    }
}


void PNS_NODE::addSegment( PNS_SEGMENT* aSeg )
{
    if( aSeg->GetSeg().a == aSeg->GetSeg().b )
    {
        TRACEn( 0, "attempting to add a segment with same end coordinates, ignoring." )
        return;
    }

    aSeg->SetOwner( this );

    linkJoint( aSeg->GetSeg().a, aSeg->GetLayers(), aSeg->GetNet(), aSeg );
    linkJoint( aSeg->GetSeg().b, aSeg->GetLayers(), aSeg->GetNet(), aSeg );

    m_index->Add( aSeg );
}


void PNS_NODE::Add( PNS_ITEM* aItem )
{
    aItem->SetOwner( this );

    switch( aItem->GetKind() )
    {
    case PNS_ITEM::SOLID:
        addSolid( static_cast<PNS_SOLID*>( aItem ) );
        break;

    case PNS_ITEM::SEGMENT:
        addSegment( static_cast<PNS_SEGMENT*>( aItem ) );
        break;

    case PNS_ITEM::LINE:
        addLine( static_cast<PNS_LINE*> (aItem) );
        break;

    case PNS_ITEM::VIA:
        addVia( static_cast<PNS_VIA*>(aItem) );
        break;

    default:
        assert( false );
    }
}


void PNS_NODE::doRemove( PNS_ITEM* aItem )
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
        aItem->SetOwner( NULL );
}


void PNS_NODE::removeSegment( PNS_SEGMENT* aSeg )
{
    unlinkJoint( aSeg->GetSeg().a, aSeg->GetLayers(), aSeg->GetNet(), aSeg );
    unlinkJoint( aSeg->GetSeg().b, aSeg->GetLayers(), aSeg->GetNet(), aSeg );

    doRemove( aSeg );
}


void PNS_NODE::removeLine( PNS_LINE* aLine )
{
    vector<PNS_SEGMENT*>* segRefs = aLine->GetLinkedSegments();

    if( !segRefs )
        return;

    assert( aLine->GetOwner() );

    BOOST_FOREACH( PNS_SEGMENT* seg, *segRefs )
    {
        removeSegment( seg );
    }

    aLine->SetOwner( NULL );
}


void PNS_NODE::removeVia( PNS_VIA* aVia )
{
    unlinkJoint( aVia->GetPos(), aVia->GetLayers(), aVia->GetNet(), aVia );

    doRemove( aVia );
}


void PNS_NODE::Replace( PNS_ITEM* aOldItem, PNS_ITEM* aNewItem )
{
    Remove( aOldItem );
    Add( aNewItem );
}


void PNS_NODE::Remove( PNS_ITEM* aItem )
{
    switch( aItem->GetKind() )
    {
    case PNS_ITEM::SOLID:
        assert( false );
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


void PNS_NODE::followLine( PNS_SEGMENT* current, bool scanDirection, int& pos,
        int limit, VECTOR2I* corners, PNS_SEGMENT** segments )
{
    bool prevReversed = false;

    for( ; ; )
    {
        const VECTOR2I p =
            (scanDirection ^ prevReversed) ? current->GetSeg().b : current->GetSeg().a;
        const OptJoint jt = FindJoint( p, current->GetLayer(), current->GetNet() );

        assert( jt );
        assert( pos > 0 && pos < limit );

        corners[pos] = jt->GetPos();
        segments[pos] = current;

        pos += (scanDirection ? 1 : -1);

        if( !jt->IsLineCorner() )
            break;

        current = jt->NextSegment( current );
        prevReversed =
            ( jt->GetPos() == (scanDirection ? current->GetSeg().b : current->GetSeg().a ) );
    }
}


PNS_LINE* PNS_NODE::AssembleLine( PNS_SEGMENT* aSeg, const OptJoint& a, const OptJoint& b )
{
    const int MaxVerts = 1024;

    VECTOR2I corners[MaxVerts + 1];
    PNS_SEGMENT* segs[MaxVerts + 1];

    PNS_LINE* pl = new PNS_LINE;
    int i_start = MaxVerts / 2, i_end = i_start + 1;

    pl->SetWidth( aSeg->GetWidth() );
    pl->SetLayers( aSeg->GetLayers() );
    pl->SetNet( aSeg->GetNet() );
    pl->SetOwner( this );

    // pl->LinkSegment(aSeg);

    followLine( aSeg, false, i_start, MaxVerts, corners, segs );
    followLine( aSeg, true, i_end, MaxVerts, corners, segs );


    int clip_start = -1, clip_end = -1;

    for( int i = i_start + 1; i < i_end; i++ )
    {
        const VECTOR2I& p = corners[i];

        if( a && ( p == a->GetPos() || p == b->GetPos() ) )
        {
            clip_start = std::min( clip_start, i );
            clip_end = std::max( clip_end, i );
        }

        pl->GetLine().Append( p );

        if( segs[i - 1] != segs[i] )
            pl->LinkSegment( segs[i] );
    }

    return pl;
}


void PNS_NODE::FindLineEnds( PNS_LINE* aLine, PNS_JOINT& a, PNS_JOINT& b )
{
    a = *FindJoint( aLine->GetCLine().CPoint( 0 ), aLine->GetLayers().Start(), aLine->GetNet() );
    b = *FindJoint( aLine->GetCLine().CPoint( -1 ), aLine->GetLayers().Start(), aLine->GetNet() );
}


int PNS_NODE::FindLinesBetweenJoints( PNS_JOINT& a, PNS_JOINT& b, vector<PNS_LINE*>& aLines )
{
    BOOST_FOREACH( PNS_ITEM * item, a.GetLinkList() )
    {
        if( item->GetKind() == PNS_ITEM::SEGMENT )
        {
            PNS_SEGMENT* seg = static_cast<PNS_SEGMENT*>(item);
            PNS_LINE* line = AssembleLine( seg );

            PNS_JOINT j_start, j_end;
            FindLineEnds( line, j_start, j_end );

            if( (j_start == a && j_end == b )|| (j_end == a && j_start == b) )
                aLines.push_back( line );
            else
                delete line;
        }
    }

    return 0;
}


const PNS_NODE::OptJoint PNS_NODE::FindJoint( const VECTOR2I& aPos, int aLayer, int aNet )
{
    PNS_JOINT::HashTag tag;

    tag.net = aNet;
    tag.pos = aPos;

    JointMap::iterator f = m_joints.find( tag ), end = m_joints.end();

    if( f == end && !isRoot() )
    {
        end = m_root->m_joints.end();
        f = m_root->m_joints.find( tag );    // m_root->FindJoint(aPos, aLayer, aNet);
    }

    if( f == end )
        return OptJoint();

    while( f != end )
    {
        if( f->second.GetLayers().Overlaps( aLayer ) )
            return f->second;

        ++f;
    }

    return OptJoint();
}


PNS_JOINT& PNS_NODE::touchJoint( const VECTOR2I& aPos, const PNS_LAYERSET& aLayers, int aNet )
{
    PNS_JOINT::HashTag tag;

    tag.pos = aPos;
    tag.net = aNet;

    // try to find the joint in this node.
    JointMap::iterator f = m_joints.find( tag );

    pair<JointMap::iterator, JointMap::iterator> range;

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
            if( aLayers.Overlaps( f->second.GetLayers() ) )
            {
                jt.Merge( f->second );
                m_joints.erase( f );
                merged = true;
                break;
            }
        }
    } while( merged );

    return m_joints.insert( TagJointPair( tag, jt ) )->second;
}


void PNS_JOINT::Dump() const
{
    printf( "joint layers %d-%d, net %d, pos %s, links: %d\n", m_layers.Start(),
            m_layers.End(), m_tag.net, m_tag.pos.Format().c_str(), LinkCount() );
}


void PNS_NODE::linkJoint( const VECTOR2I& aPos,
        const PNS_LAYERSET& aLayers,
        int aNet,
        PNS_ITEM* aWhere )
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
            all_segs.insert( static_cast<PNS_SEGMENT*>(*i) );
    }

    if( !isRoot() )
        for( i = m_root->m_items.begin(); i != m_root->m_items.end(); i++ )
        {
            if( (*i)->GetKind() == PNS_ITEM::SEGMENT && !overrides( *i ) )
                all_segs.insert( static_cast<PNS_SEGMENT*>(*i) );
        }

    JointMap::iterator j;

    if( aLong )
        for( j = m_joints.begin(); j!=m_joints.end(); ++j )
        {
            printf( "joint : %s, links : %d\n",
                    j->second.GetPos().Format().c_str(), j->second.LinkCount() );
            PNS_JOINT::LinkedItems::const_iterator k;

            for( k = j->second.GetLinkList().begin(); k != j->second.GetLinkList().end(); ++k )
            {
                const PNS_ITEM* item = *k;

                switch( item->GetKind() )
                {
                case PNS_ITEM::SEGMENT:
                    {
                        const PNS_SEGMENT* seg = static_cast<const PNS_SEGMENT*>(item);
                        printf( " -> seg %s %s\n", seg->GetSeg().a.Format().c_str(),
                                seg->GetSeg().b.Format().c_str() );
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


        for( vector<PNS_SEGMENT*>::iterator j = seg_refs->begin(); j != seg_refs->end(); ++j )
        {
            printf( "%s ", (*j)->GetSeg().a.Format().c_str() );

            if( j + 1 == seg_refs->end() )
                printf( "%s\n", (*j)->GetSeg().b.Format().c_str() );

            all_segs.erase( *j );
        }

        lines_count++;
    }

    printf( "Local joints: %d, lines : %d \n", m_joints.size(), lines_count );
#endif
}


void PNS_NODE::GetUpdatedItems( ItemVector& aRemoved, ItemVector& aAdded )
{
    aRemoved.reserve( m_override.size() );
    aAdded.reserve( m_index->Size() );

    if( isRoot() )
        return;

    BOOST_FOREACH( PNS_ITEM * item, m_override )
    aRemoved.push_back( item );

    for( PNS_INDEX::ItemSet::iterator i = m_index->begin(); i!=m_index->end(); ++i )
        aAdded.push_back( *i );
}


void PNS_NODE::releaseChildren()
{
    // copy the kids as the PNS_NODE destructor erases the item from the parent node.
    vector<PNS_NODE*> kids = m_children;

    BOOST_FOREACH( PNS_NODE * node, kids ) {
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

    for( PNS_INDEX::ItemSet::iterator i = aNode->m_index->begin(); 
            i != aNode->m_index->end(); ++i )
        Add( *i );

    releaseChildren();
}


void PNS_NODE::KillChildren()
{
    assert( isRoot() );

    releaseChildren();
}


void PNS_NODE::AllItemsInNet( int aNet, std::list<PNS_ITEM*>& aItems )
{
    PNS_INDEX::NetItemsList* l_cur = m_index->GetItemsForNet( aNet );

    if( !l_cur )
        return;

    std::copy( aItems.begin(), l_cur->begin(), l_cur->end() );

    if( !isRoot() )
    {
        PNS_INDEX::NetItemsList* l_root = m_root->m_index->GetItemsForNet( aNet );

        for( PNS_INDEX::NetItemsList::iterator i = l_root->begin(); i!= l_root->end(); ++i )
            if( !overrides( *i ) )
                aItems.push_back( *i );


    }
}


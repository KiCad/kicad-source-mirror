/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2019 CERN
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

#include <vector>
#include <cassert>
#include <utility>

#include <math/vector2d.h>

#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>
#include <zone.h>

#include <wx/log.h>

#include "pns_arc.h"
#include "pns_item.h"
#include "pns_itemset.h"
#include "pns_line.h"
#include "pns_node.h"
#include "pns_via.h"
#include "pns_solid.h"
#include "pns_joint.h"
#include "pns_index.h"
#include "pns_debug_decorator.h"
#include "pns_router.h"
#include "pns_utils.h"


namespace PNS {

#ifdef DEBUG
static std::unordered_set<const NODE*> allocNodes;
#endif

NODE::NODE()
{
    m_depth = 0;
    m_root = this;
    m_parent = nullptr;
    m_maxClearance = 800000;    // fixme: depends on how thick traces are.
    m_ruleResolver = nullptr;
    m_index = new INDEX;

#ifdef DEBUG
    allocNodes.insert( this );
#endif
}


NODE::~NODE()
{
    if( !m_children.empty() )
    {
        wxLogTrace( wxT( "PNS" ), wxT( "attempting to free a node that has kids." ) );
        assert( false );
    }

#ifdef DEBUG
    if( allocNodes.find( this ) == allocNodes.end() )
    {
        wxLogTrace( wxT( "PNS" ), wxT( "attempting to free an already-free'd node." ) );
        assert( false );
    }

    allocNodes.erase( this );
#endif

    m_joints.clear();

    std::vector<const ITEM*> toDelete;

    toDelete.reserve( m_index->Size() );

    for( ITEM* item : *m_index )
    {
        if( item->BelongsTo( this ) )
        {
            if ( item->OfKind( ITEM::HOLE_T ) )
            {
                HOLE* hole = static_cast<HOLE*>( item );
                if( hole->ParentPadVia() )
                {
                    // If a hole is no longer owned by the same NODE as its parent then we're in a
                    // heap of trouble.
                    assert( hole->ParentPadVia()->BelongsTo( this ) );

                    // we will encounter its parent later, disguised as VIA or SOLID.
                    // don't bother reparenting the hole now, it's deleted anyway.
                }
                else
                {
                    // freestanding hole
                    toDelete.push_back(item);
                }
            }
            else {
                // other geometry with no holes
                toDelete.push_back(item);
            }
        }
    }

    if( m_ruleResolver )
    {
        m_ruleResolver->ClearCacheForItems( toDelete );
    }

    for( const ITEM* item : toDelete )
    {
        wxLogTrace( wxT( "PNS" ), wxT( "del item %p type %s" ), item, item->KindStr().c_str() );
        delete item;
    }

    releaseGarbage();
    unlinkParent();

    delete m_index;
}


int NODE::GetClearance( const ITEM* aA, const ITEM* aB, bool aUseClearanceEpsilon ) const
{
    if( !m_ruleResolver )
        return 100000;

    if( aA->IsVirtual() || aB->IsVirtual() )
        return 0;

    int cl = m_ruleResolver->Clearance( aA, aB, aUseClearanceEpsilon );

    return cl;
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
    wxLogTrace( wxT( "PNS" ), wxT( "%d items, %d joints, %d overrides" ),
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
    m_node( nullptr ),
    m_override( nullptr )
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
    COLLISION_SEARCH_CONTEXT* m_ctx;

    DEFAULT_OBSTACLE_VISITOR( COLLISION_SEARCH_CONTEXT* aCtx, const ITEM* aItem ) :
        OBSTACLE_VISITOR( aItem ),
        m_ctx( aCtx )
    {
    }

    virtual ~DEFAULT_OBSTACLE_VISITOR()
    {
    }

    bool operator()( ITEM* aCandidate ) override
    {
        if( !aCandidate->OfKind( m_ctx->options.m_kindMask ) )
            return true;

        // Collisions with self aren't a thing; don't spend time on them.
        if( m_item == aCandidate )
            return true;

        if( m_ctx->options.m_filter && !m_ctx->options.m_filter( aCandidate ) )
            return true;

        if( visit( aCandidate ) )
            return true;

        if( !aCandidate->Collide( m_item, m_node, m_layerContext.value_or( -1 ), m_ctx ) )
            return true;

        if( m_ctx->options.m_limitCount > 0 && m_ctx->obstacles.size() >= m_ctx->options.m_limitCount )
            return false;

        return true;
    };
};


int NODE::QueryColliding( const ITEM* aItem, NODE::OBSTACLES& aObstacles,
                          const COLLISION_SEARCH_OPTIONS& aOpts ) const
{
    COLLISION_SEARCH_CONTEXT ctx( aObstacles, aOpts );

    /// By default, virtual items cannot collide
    if( aItem->IsVirtual() )
        return 0;

    DEFAULT_OBSTACLE_VISITOR visitor( &ctx, aItem );

#ifdef DEBUG
    assert( allocNodes.find( this ) != allocNodes.end() );
#endif

    visitor.SetWorld( this, nullptr );

    // first, look for colliding items in the local index
    m_index->Query( aItem, m_maxClearance, visitor );

    // if we haven't found enough items, look in the root branch as well.
    if( !isRoot() && ( ctx.obstacles.size() < aOpts.m_limitCount || aOpts.m_limitCount < 0 ) )
    {
        visitor.SetWorld( m_root, this );
        m_root->m_index->Query( aItem, m_maxClearance, visitor );
    }

    return aObstacles.size();
}


NODE::OPT_OBSTACLE NODE::NearestObstacle( const LINE* aLine,
                                          const COLLISION_SEARCH_OPTIONS& aOpts )
{
    DIRECTION_45::CORNER_MODE cornerMode = ROUTER::GetInstance()->Settings().GetCornerMode();
    const int                 clearanceEpsilon = GetRuleResolver()->ClearanceEpsilon();
    OBSTACLES                 obstacleList;

    for( int i = 0; i < aLine->CLine().SegmentCount(); i++ )
    {
        // Note: Clearances between &s and other items are cached,
        // which means they'll be the same for all segments in the line.
        // Disabling the cache will lead to slowness.

        const SEGMENT s( *aLine, aLine->CLine().CSegment( i ) );
        QueryColliding( &s, obstacleList, aOpts );
    }

    if( aLine->EndsWithVia() )
        QueryColliding( &aLine->Via(), obstacleList, aOpts );

    if( obstacleList.empty() )
        return OPT_OBSTACLE();

    OBSTACLE nearest;
    nearest.m_head = nullptr;
    nearest.m_item = nullptr;
    nearest.m_distFirst = INT_MAX;
    nearest.m_maxFanoutWidth = 0;

    bool foundZeroDistance = false;

    auto updateNearest =
            [&]( const SHAPE_LINE_CHAIN::INTERSECTION& pt, const OBSTACLE& obstacle )
            {
                int dist = aLine->CLine().PathLength( pt.p, pt.index_their );

                if( dist < nearest.m_distFirst )
                {
                    nearest = obstacle;
                    nearest.m_distFirst = dist;
                    nearest.m_ipFirst = pt.p;

                    if( dist == 0 )
                        foundZeroDistance = true;
                }
            };

    SHAPE_LINE_CHAIN obstacleHull;
    DEBUG_DECORATOR* debugDecorator = ROUTER::GetInstance()->GetInterface()->GetDebugDecorator();
    std::vector<SHAPE_LINE_CHAIN::INTERSECTION> intersectingPts;
    int layer = aLine->Layer();

    RULE_RESOLVER* ruleResolver = GetRuleResolver();
    bool           simplifyHull = ( cornerMode == DIRECTION_45::MITERED_90
                                  || cornerMode == DIRECTION_45::ROUNDED_90 );

    for( const OBSTACLE& obstacle : obstacleList )
    {
        if( foundZeroDistance )
            break;

        int clearance = GetClearance( obstacle.m_item, aLine, aOpts.m_useClearanceEpsilon )
                            + aLine->Width() / 2;

        const SHAPE_LINE_CHAIN& cachedHull = ruleResolver->HullCache( obstacle.m_item, clearance,
                                                                      0, layer );

        if( simplifyHull )
        {
            BOX2I bbox = cachedHull.BBox();
            obstacleHull.Clear();
            obstacleHull.Append( bbox.GetLeft(),  bbox.GetTop()    );
            obstacleHull.Append( bbox.GetRight(), bbox.GetTop()    );
            obstacleHull.Append( bbox.GetRight(), bbox.GetBottom() );
            obstacleHull.Append( bbox.GetLeft(),  bbox.GetBottom() );
        }
        else
        {
            obstacleHull = cachedHull;
        }

        intersectingPts.clear();
        HullIntersection( obstacleHull, aLine->CLine(), intersectingPts );

        for( const auto& ip : intersectingPts )
        {
            if( ip.valid )
                updateNearest( ip, obstacle );
        }

        if( aLine->EndsWithVia() )
        {
            const VIA& via = aLine->Via();
            int viaClearance = GetClearance( obstacle.m_item, &via, aOpts.m_useClearanceEpsilon )
                               + via.Diameter( aLine->Layer() ) / 2;

            const SHAPE_LINE_CHAIN& viaCachedHull = ruleResolver->HullCache( obstacle.m_item,
                                                                             viaClearance, 0, layer );

            if( simplifyHull )
            {
                BOX2I bbox = viaCachedHull.BBox();
                obstacleHull.Clear();
                obstacleHull.Append( bbox.GetLeft(),  bbox.GetTop()    );
                obstacleHull.Append( bbox.GetRight(), bbox.GetTop()    );
                obstacleHull.Append( bbox.GetRight(), bbox.GetBottom() );
                obstacleHull.Append( bbox.GetLeft(),  bbox.GetBottom() );
            }
            else
            {
                obstacleHull = viaCachedHull;
            }

            intersectingPts.clear();
            HullIntersection( obstacleHull, aLine->CLine(), intersectingPts );

            for( const SHAPE_LINE_CHAIN::INTERSECTION& ip : intersectingPts )
                updateNearest( ip, obstacle );
        }
    }

    if( nearest.m_distFirst == INT_MAX )
        nearest = (*obstacleList.begin());

    return nearest;
}


NODE::OPT_OBSTACLE NODE::CheckColliding( const ITEM_SET& aSet, int aKindMask )
{
    for( const ITEM* item : aSet.CItems() )
    {
        OPT_OBSTACLE obs = CheckColliding( item, aKindMask );

        if( obs )
            return obs;
    }

    return OPT_OBSTACLE();
}


NODE::OPT_OBSTACLE NODE::CheckColliding( const ITEM* aItemA, int aKindMask )
{
    COLLISION_SEARCH_OPTIONS opts;

    opts.m_kindMask = aKindMask;
    opts.m_limitCount = 1;

    return CheckColliding( aItemA, opts );
}

NODE::OPT_OBSTACLE NODE::CheckColliding( const ITEM* aItemA, const COLLISION_SEARCH_OPTIONS& aOpts )
{
    OBSTACLES obs;

    if( aItemA->Kind() == ITEM::LINE_T )
    {
        int n = 0;
        const LINE* line = static_cast<const LINE*>( aItemA );
        const SHAPE_LINE_CHAIN& l = line->CLine();

        for( int i = 0; i < l.SegmentCount(); i++ )
        {
            // Note: Clearances between &s and other items are cached,
            // which means they'll be the same for all segments in the line.
            // Disabling the cache will lead to slowness.

            const SEGMENT s( *line, l.CSegment( i ) );
            n += QueryColliding( &s, obs, aOpts );

            if( n )
                return OPT_OBSTACLE( *obs.begin() );
        }

        if( line->EndsWithVia() )
        {
            n += QueryColliding( &line->Via(), obs, aOpts );

            if( n )
                return OPT_OBSTACLE( *obs.begin() );
        }
    }
    else if( QueryColliding( aItemA, obs, aOpts ) > 0 )
    {
        return OPT_OBSTACLE( *obs.begin() );
    }

    return OPT_OBSTACLE();
}


struct HIT_VISITOR : public OBSTACLE_VISITOR
{
    ITEM_SET& m_items;
    const VECTOR2I& m_point;

    HIT_VISITOR( ITEM_SET& aTab, const VECTOR2I& aPoint ) :
        OBSTACLE_VISITOR( nullptr ),
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

        // TODO(JE) padstacks -- this may not work
        if( aItem->Shape( -1 )->Collide( &cp, cl ) )
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
    visitor.SetWorld( this, nullptr );

    m_index->Query( &s, m_maxClearance, visitor );

    if( !isRoot() )    // fixme: could be made cleaner
    {
        ITEM_SET items_root;
        visitor.SetWorld( m_root, nullptr );
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
    if( aSolid->HasHole() )
    {
        assert( aSolid->Hole()->BelongsTo( aSolid ) );
        addHole( aSolid->Hole() );
    }

    if( aSolid->IsRoutable() )
        linkJoint( aSolid->Pos(), aSolid->Layers(), aSolid->Net(), aSolid );

    aSolid->SetOwner( this );
    m_index->Add( aSolid );
}


void NODE::Add( std::unique_ptr< SOLID > aSolid )
{
    aSolid->SetOwner( this );
    addSolid( aSolid.release() );
}


void NODE::addVia( VIA* aVia )
{
    if( aVia->HasHole() )
    {
        if( ! aVia->Hole()->BelongsTo( aVia ) )
        {
            assert( false );
        }
        addHole( aVia->Hole() );
    }

    linkJoint( aVia->Pos(), aVia->Layers(), aVia->Net(), aVia );
    aVia->SetOwner( this );

    m_index->Add( aVia );
}


void NODE::addHole( HOLE* aHole )
{
    // do we need holes in the connection graph?
    //linkJoint( aHole->Pos(), aHole->Layers(), aHole->Net(), aHole );

    aHole->SetOwner( this );
    m_index->Add( aHole );
}


void NODE::Add( std::unique_ptr< VIA > aVia )
{
    addVia( aVia.release() );
}


void NODE::add( ITEM* aItem, bool aAllowRedundant )
{
    switch( aItem->Kind() )
    {
    case ITEM::ARC_T:
        addArc( static_cast<ARC*>( aItem ) );
        break;
    case ITEM::SEGMENT_T:
        addSegment( static_cast<SEGMENT*>( aItem ) );
        break;
    case ITEM::VIA_T:
        addVia( static_cast<VIA*>( aItem ) );
        break;
    case ITEM::SOLID_T:
        addSolid( static_cast<SOLID*>( aItem ) );
        break;
    case ITEM::HOLE_T:
        // added by parent VIA_T or SOLID_T (pad)
        break;
    default:
        assert( false );
    }
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
        if( l.IsArcSegment( i ) )
            continue;

        SEG s = l.CSegment( i );

        if( s.A != s.B )
        {
            SEGMENT* rseg;

            if( !aAllowRedundant && ( rseg = findRedundantSegment( s.A, s.B, aLine.Layers(),
                                                                   aLine.Net() ) ) )
            {
                // another line could be referencing this segment too :(
                if( !aLine.ContainsLink( rseg ) )
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
    aSeg->SetOwner( this );

    linkJoint( aSeg->Seg().A, aSeg->Layers(), aSeg->Net(), aSeg );
    linkJoint( aSeg->Seg().B, aSeg->Layers(), aSeg->Net(), aSeg );

    m_index->Add( aSeg );
}


bool NODE::Add( std::unique_ptr< SEGMENT > aSegment, bool aAllowRedundant )
{
    if( aSegment->Seg().A == aSegment->Seg().B )
    {
        wxLogTrace( wxT( "PNS" ),
                    wxT( "attempting to add a segment with same end coordinates, ignoring." ) );
        return false;
    }

    if( !aAllowRedundant && findRedundantSegment( aSegment.get() ) )
        return false;

    addSegment( aSegment.release() );

    return true;
}


void NODE::addArc( ARC* aArc )
{
    aArc->SetOwner( this );

    linkJoint( aArc->Anchor( 0 ), aArc->Layers(), aArc->Net(), aArc );
    linkJoint( aArc->Anchor( 1 ), aArc->Layers(), aArc->Net(), aArc );

    m_index->Add( aArc );
}


bool NODE::Add( std::unique_ptr< ARC > aArc, bool aAllowRedundant )
{
    const SHAPE_ARC& arc = aArc->CArc();

    if( !aAllowRedundant && findRedundantArc( arc.GetP0(), arc.GetP1(), aArc->Layers(),
                                              aArc->Net() ) )
    {
        return false;
    }

    addArc( aArc.release() );
    return true;
}


void NODE::AddEdgeExclusion( std::unique_ptr<SHAPE> aShape )
{
    m_edgeExclusions.push_back( std::move( aShape ) );
}


bool NODE::QueryEdgeExclusions( const VECTOR2I& aPos ) const
{
    for( const std::unique_ptr<SHAPE>& edgeExclusion : m_edgeExclusions )
    {
        if( edgeExclusion->Collide( aPos ) )
            return true;
    }

    return false;
}


void NODE::doRemove( ITEM* aItem )
{
    bool holeRemoved = false; // fixme: better logic, I don't like this

    // case 1: removing an item that is stored in the root node from any branch:
    // mark it as overridden, but do not remove
    if( aItem->BelongsTo( m_root ) && !isRoot() )
    {
        m_override.insert( aItem );

        if( aItem->HasHole() )
            m_override.insert( aItem->Hole() );
    }

    // case 2: the item belongs to this branch or a parent, non-root branch,
    // or the root itself and we are the root: remove from the index
    else if( !aItem->BelongsTo( m_root ) || isRoot() )
    {
        m_index->Remove( aItem );

        if( aItem->HasHole() )
        {
            m_index->Remove( aItem->Hole() );
            holeRemoved = true;
        }
    }

    // the item belongs to this particular branch: un-reference it
    if( aItem->BelongsTo( this ) )
    {
        aItem->SetOwner( nullptr );
        m_root->m_garbageItems.insert( aItem );
        HOLE *hole = aItem->Hole();

        if( hole )
        {
            if( ! holeRemoved )
            {
                m_index->Remove( hole ); // hole is not directly owned by NODE but by the parent SOLID/VIA.
            }

            hole->SetOwner( aItem );
        }
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


void NODE::rebuildJoint( const JOINT* aJoint, const ITEM* aItem )
{
    if( !aJoint )
        return;

    // We have to split a single joint (associated with a via or a pad, binding together multiple
    // layers) into multiple independent joints. As I'm a lazy bastard, I simply delete the
    // via/solid and all its links and re-insert them.

    std::vector<ITEM*> links( aJoint->LinkList() );
    JOINT::HASH_TAG    tag;
    NET_HANDLE         net = aItem->Net();

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

    bool completelyErased = false;

    if( !isRoot() && m_joints.find( tag ) == m_joints.end() )
    {
        JOINT jtDummy( tag.pos, PNS_LAYER_RANGE(-1), tag.net );

        m_joints.insert( TagJointPair( tag, jtDummy ) );
        completelyErased = true;
    }


    // and re-link them, using the former via's link list
    for( ITEM* link : links )
    {
        if( link != aItem )
            linkJoint( tag.pos, link->Layers(), net, link );
        else if( !completelyErased )
            unlinkJoint( tag.pos, link->Layers(), net, link );
    }
}


void NODE::removeViaIndex( VIA* aVia )
{
    const JOINT* jt = FindJoint( aVia->Pos(), aVia->Layers().Start(), aVia->Net() );
    assert( jt );
    rebuildJoint( jt, aVia );
}


void NODE::removeSolidIndex( SOLID* aSolid )
{
    if( !aSolid->IsRoutable() )
        return;

    // fixme: redundant code
    const JOINT* jt = FindJoint( aSolid->Pos(), aSolid->Layers().Start(), aSolid->Net() );
    assert( jt );
    rebuildJoint( jt, aSolid );
}


void NODE::Replace( ITEM* aOldItem, std::unique_ptr< ITEM > aNewItem )
{
    Remove( aOldItem );
    add( aNewItem.release() );
}


void NODE::Replace( LINE& aOldLine, LINE& aNewLine, bool aAllowRedundantSegments )
{
    Remove( aOldLine );
    Add( aNewLine, aAllowRedundantSegments );
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

    if( !aVia->Owner() )
    {
        assert( aVia->Hole()->BelongsTo( aVia ) );
    }
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
    {
        SOLID* solid = static_cast<SOLID*>( aItem );

        if( solid->HasHole() )
        {
            Remove( solid->Hole() );
            solid->Hole()->SetOwner( solid );
        }

        Remove( static_cast<SOLID*>( aItem ) );
        break;
    }

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
    {
        VIA* via = static_cast<VIA*>( aItem );

        if( via->HasHole() )
        {
            Remove( via->Hole() );
            via->Hole()->SetOwner( via );
        }

        Remove( static_cast<VIA*>( aItem ) );
        break;
    }

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
        else if( li->OfKind( ITEM::VIA_T ) )
            Remove( static_cast<VIA*>( li ) );
    }

    aLine.SetOwner( nullptr );
    aLine.ClearLinks();
}


void NODE::followLine( LINKED_ITEM* aCurrent, bool aScanDirection, int& aPos, int aLimit,
                       VECTOR2I* aCorners, LINKED_ITEM** aSegments, bool* aArcReversed,
                       bool& aGuardHit, bool aStopAtLockedJoints, bool aFollowLockedSegments )
{
    bool prevReversed = false;

    const VECTOR2I guard = aCurrent->Anchor( aScanDirection );

    for( int count = 0 ; ; ++count )
    {
        const VECTOR2I p  = aCurrent->Anchor( aScanDirection ^ prevReversed );
        const JOINT*   jt = FindJoint( p, aCurrent );

        if( !jt )
            break;

        aCorners[aPos]     = jt->Pos();
        aSegments[aPos]    = aCurrent;
        aArcReversed[aPos] = false;

        if( aCurrent->Kind() == ITEM::ARC_T )
        {
            if( ( aScanDirection && jt->Pos() == aCurrent->Anchor( 0 ) )
                    || ( !aScanDirection && jt->Pos() == aCurrent->Anchor( 1 ) ) )
            {
                aArcReversed[aPos] = true;
            }
        }

        aPos += ( aScanDirection ? 1 : -1 );

        if( count && guard == p )
        {
            if( aPos >= 0 && aPos < aLimit )
                aSegments[aPos] = nullptr;

            aGuardHit = true;
            break;
        }

        bool locked = aStopAtLockedJoints ? jt->IsLocked() : false;

        if( locked || aPos < 0 || aPos == aLimit )
            break;

        aCurrent = jt->NextSegment( aCurrent, aFollowLockedSegments );

        if( !aCurrent )
            break;

        prevReversed = ( aCurrent && jt->Pos() == aCurrent->Anchor( aScanDirection ) );
    }
}


const LINE NODE::AssembleLine( LINKED_ITEM* aSeg, int* aOriginSegmentIndex, bool aStopAtLockedJoints,
                               bool aFollowLockedSegments, bool aAllowSegmentSizeMismatch )
{
    const int MaxVerts = 1024 * 16;

    std::array<VECTOR2I, MaxVerts + 1> corners;
    std::array<LINKED_ITEM*, MaxVerts + 1> segs;
    std::array<bool, MaxVerts + 1> arcReversed;

    LINE pl;
    bool guardHit = false;

    int i_start = MaxVerts / 2;
    int i_end   = i_start + 1;

    pl.SetWidth( aSeg->Width() );
    pl.SetLayers( aSeg->Layers() );
    pl.SetNet( aSeg->Net() );
    pl.SetParent( nullptr );
    pl.SetSourceItem( aSeg->GetSourceItem() );
    pl.SetOwner( this );

    followLine( aSeg, false, i_start, MaxVerts, corners.data(), segs.data(), arcReversed.data(),
                guardHit, aStopAtLockedJoints, aFollowLockedSegments );

    if( !guardHit )
    {
        followLine( aSeg, true, i_end, MaxVerts, corners.data(), segs.data(), arcReversed.data(),
                    guardHit, aStopAtLockedJoints, aFollowLockedSegments );
    }

    int n = 0;

    LINKED_ITEM* prev_seg = nullptr;
    bool originSet = false;

    SHAPE_LINE_CHAIN& line = pl.Line();

    for( int i = i_start + 1; i < i_end; i++ )
    {
        const VECTOR2I& p  = corners[i];
        LINKED_ITEM*    li = segs[i];

        if( !aAllowSegmentSizeMismatch && ( li && li->Width() != aSeg->Width() ) )
            continue;

        if( !li || li->Kind() != ITEM::ARC_T )
            line.Append( p );

        if( li && prev_seg != li )
        {
            if( li->Kind() == ITEM::ARC_T )
            {
                const ARC*       arc = static_cast<const ARC*>( li );
                const SHAPE_ARC* sa  = static_cast<const SHAPE_ARC*>( arc->Shape( -1 ) );

                int      nSegs     = line.PointCount();
                VECTOR2I last      = nSegs ? line.CLastPoint() : VECTOR2I();
                ssize_t lastShape = nSegs ? line.ArcIndex( static_cast<ssize_t>( nSegs ) - 1 ) : -1;

                line.Append( arcReversed[i] ? sa->Reversed() : *sa );
            }

            pl.Link( li );

            // latter condition to avoid loops
            if( li == aSeg && aOriginSegmentIndex && !originSet )
            {
                wxASSERT( n < line.SegmentCount() ||
                          ( n == line.SegmentCount() && li->Kind() == ITEM::SEGMENT_T ) );
                *aOriginSegmentIndex = line.PointCount() - 1;
                originSet = true;
            }
        }

        prev_seg = li;
    }

    // Remove duplicate verts, but do NOT remove colinear segments here!
    pl.Line().RemoveDuplicatePoints();

    // TODO: maintain actual segment index under simplification system
    if( aOriginSegmentIndex && *aOriginSegmentIndex >= pl.SegmentCount() )
        *aOriginSegmentIndex = pl.SegmentCount() - 1;

    wxASSERT_MSG( pl.SegmentCount() != 0, "assembled line should never be empty" );

    return pl;
}


void NODE::FindLineEnds( const LINE& aLine, JOINT& aA, JOINT& aB )
{
    aA = *FindJoint( aLine.CPoint( 0 ), &aLine );
    aB = *FindJoint( aLine.CLastPoint(), &aLine );
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


void NODE::FixupVirtualVias()
{
    const SEGMENT* locked_seg = nullptr;
    std::vector<VVIA*> vvias;

    for( auto& jointPair : m_joints )
    {
        JOINT joint = jointPair.second;

        if( joint.Layers().IsMultilayer() )
            continue;

        int                n_seg   = 0;
        int                n_solid = 0;
        int                n_vias  = 0;
        int                prev_w    = -1;
        bool               prev_mask = false;
        std::optional<int> prev_mask_margin;
        int                max_w           = -1;
        bool               is_width_change = false;
        bool               is_locked       = false;

        for( const ITEM* item : joint.LinkList() )
        {
            if( item->OfKind( ITEM::VIA_T ) )
            {
                n_vias++;
            }
            else if( item->OfKind( ITEM::SOLID_T ) )
            {
                n_solid++;
            }
            else if( const auto t = dyn_cast<const PNS::SEGMENT*>( item ) )
            {
                int                w    = t->Width();
                bool               mask = false;
                std::optional<int> mask_margin;

                if( PCB_TRACK* track = static_cast<PCB_TRACK*>( t->Parent() ) )
                {
                    mask = track->HasSolderMask();
                    mask_margin = track->GetLocalSolderMaskMargin();
                }

                if( prev_w < 0 )
                {
                    prev_w = w;
                    prev_mask = mask;
                    prev_mask_margin = mask_margin;
                }
                else if( w != prev_w || mask != prev_mask || mask_margin != prev_mask_margin )
                {
                    is_width_change = true;
                }

                max_w = std::max( w, max_w );
                prev_w = w;

                is_locked  = t->IsLocked();
                locked_seg = t;
            }
        }

        if( ( is_width_change || n_seg >= 3 || is_locked ) && n_solid == 0 && n_vias == 0 )
        {
            // fixme: the hull margin here is an ugly temporary workaround. The real fix
            // is to use octagons for via force propagation.
            vvias.push_back( new VVIA( joint.Pos(), joint.Layers().Start(),
                                       max_w + 2 * PNS_HULL_MARGIN, joint.Net() ) );
        }

        if( is_locked )
        {
            const VECTOR2I& secondPos = ( locked_seg->Seg().A == joint.Pos() ) ?
                                        locked_seg->Seg().B :
                                        locked_seg->Seg().A;

            vvias.push_back( new VVIA( secondPos, joint.Layers().Start(),
                                       max_w + 2 * PNS_HULL_MARGIN, joint.Net() ) );
        }
    }

    for( auto vvia : vvias )
    {
        Add( ItemCast<VIA>( std::move( std::unique_ptr<VVIA>( vvia ) ) ) );
    }
}


const JOINT* NODE::FindJoint( const VECTOR2I& aPos, int aLayer, NET_HANDLE aNet ) const
{
    JOINT::HASH_TAG tag;

    tag.net = aNet;
    tag.pos = aPos;

    JOINT_MAP::const_iterator f = m_joints.find( tag ), end = m_joints.end();

    if( f == end && !isRoot() )
    {
        end = m_root->m_joints.end();
        f = m_root->m_joints.find( tag );    // m_root->FindJoint(aPos, aLayer, aNet);
    }

    while( f != end )
    {
        if( f->second.Pos() == aPos && f->second.Net() == aNet && f->second.Layers().Overlaps( aLayer ) )
            return &f->second;

        f++;
    }

    return nullptr;
}


void NODE::LockJoint( const VECTOR2I& aPos, const ITEM* aItem, bool aLock )
{
    JOINT& jt = touchJoint( aPos, aItem->Layers(), aItem->Net() );
    jt.Lock( aLock );
}


JOINT& NODE::touchJoint( const VECTOR2I& aPos, const PNS_LAYER_RANGE& aLayers, NET_HANDLE aNet )
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
    } while( merged );

    return m_joints.insert( TagJointPair( tag, jt ) )->second;
}


void JOINT::Dump() const
{
    wxLogTrace( wxT( "PNS" ), wxT( "joint layers %d-%d, net %d, pos %s, links: %d" ),
                m_layers.Start(),
                m_layers.End(),
                m_tag.net,
                m_tag.pos.Format().c_str(),
                LinkCount() );
}


void NODE::linkJoint( const VECTOR2I& aPos, const PNS_LAYER_RANGE& aLayers, NET_HANDLE aNet,
                      ITEM* aWhere )
{
    JOINT& jt = touchJoint( aPos, aLayers, aNet );

    jt.Link( aWhere );
}


void NODE::unlinkJoint( const VECTOR2I& aPos, const PNS_LAYER_RANGE& aLayers, NET_HANDLE aNet,
                        ITEM* aWhere )
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
            wxLogTrace( wxT( "PNS" ), wxT( "joint : %s, links : %d\n" ),
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
                        wxLogTrace( wxT( "PNS" ), wxT( " -> seg %s %s\n" ),
                                    seg->GetSeg().A.Format().c_str(),
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
        {
            wxLogTrace( wxT( "PNS" ), wxT( "Line: %s, net %d " ),
                        l->GetLine().Format().c_str(), l->GetNet() );
        }

        for( std::vector<SEGMENT*>::iterator j = seg_refs->begin(); j != seg_refs->end(); ++j )
        {
            wxLogTrace( wxT( "PNS" ), wxT( "%s " ), (*j)->GetSeg().A.Format().c_str() );

            if( j + 1 == seg_refs->end() )
                wxLogTrace( wxT( "PNS" ), wxT( "%s\n" ), (*j)->GetSeg().B.Format().c_str() );

            all_segs.erase( *j );
        }

        lines_count++;
    }

    wxLogTrace( wxT( "PNS" ), wxT( "Local joints: %d, lines : %d \n" ),
                m_joints.size(), lines_count );
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

    for( ITEM* item : *m_index )
        aAdded.push_back( item );
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

    std::vector<const ITEM*> toDelete;
    toDelete.reserve( m_garbageItems.size() );

    for( ITEM* item : m_garbageItems )
    {
        if( !item->BelongsTo( this ) )
        {
            toDelete.push_back( item );
        }
    }

    if( m_ruleResolver )
    {
        m_ruleResolver->ClearCacheForItems( toDelete );
    }

    for( const ITEM* item : toDelete)
    {
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
        if( item->HasHole() )
        {
            item->Hole()->SetOwner( item );
        }

        item->SetRank( -1 );
        item->Unmark();
        add( item );
    }

    releaseChildren();
    releaseGarbage();
}


void NODE::KillChildren()
{
    releaseChildren();
}


void NODE::AllItemsInNet( NET_HANDLE aNet, std::set<ITEM*>& aItems, int aKindMask )
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
    std::vector<ITEM*> garbage;

    for( ITEM* item : *m_index )
    {
        if( item->Marker() & aMarker )
            garbage.emplace_back( item );
    }

    for( ITEM* item : garbage )
        Remove( item );
}


SEGMENT* NODE::findRedundantSegment( const VECTOR2I& A, const VECTOR2I& B, const PNS_LAYER_RANGE& lr,
                                     NET_HANDLE aNet )
{
    const JOINT* jtStart = FindJoint( A, lr.Start(), aNet );

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


ARC* NODE::findRedundantArc( const VECTOR2I& A, const VECTOR2I& B, const PNS_LAYER_RANGE& lr,
                             NET_HANDLE aNet )
{
    const JOINT* jtStart = FindJoint( A, lr.Start(), aNet );

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


int NODE::QueryJoints( const BOX2I& aBox, std::vector<JOINT*>& aJoints, PNS_LAYER_RANGE aLayerMask,
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
    if( aParent && aParent->IsConnected() )
    {
        const BOARD_CONNECTED_ITEM* cItem = static_cast<const BOARD_CONNECTED_ITEM*>( aParent );
        INDEX::NET_ITEMS_LIST*      l_cur = m_index->GetItemsForNet( cItem->GetNet() );

        if( l_cur )
        {
            for( ITEM* item : *l_cur )
            {
                if( item->Parent() == aParent )
                    return item;
            }
        }
    }

    return nullptr;
}


std::vector<ITEM*> NODE::FindItemsByParent( const BOARD_ITEM* aParent )
{
    std::vector<ITEM*> ret;

    for( ITEM* item : *m_index )
    {
        if( item->Parent() == aParent )
            ret.push_back( item );
    }

    return ret;
}


VIA* NODE::FindViaByHandle ( const VIA_HANDLE& handle ) const
{
    const JOINT* jt = FindJoint( handle.pos, handle.layers.Start(), handle.net );

    if( !jt )
        return nullptr;

    for( ITEM* item : jt->LinkList() )
    {
        if( item->OfKind( ITEM::VIA_T ) )
        {
            if( item->Net() == handle.net && item->Layers().Overlaps(handle.layers) )
                return static_cast<VIA*>( item );
        }
    }

    return nullptr;
}

}


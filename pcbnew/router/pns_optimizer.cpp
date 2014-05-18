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

#include <boost/foreach.hpp>

#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>

#include "pns_line.h"
#include "pns_node.h"
#include "pns_optimizer.h"
#include "pns_utils.h"
#include "pns_router.h"

/**
 *  Cost Estimator Methods
 */
int PNS_COST_ESTIMATOR::CornerCost( const SEG& aA, const SEG& aB )
{
    DIRECTION_45 dir_a( aA ), dir_b( aB );

    switch( dir_a.Angle( dir_b ) )
    {
    case DIRECTION_45::ANG_OBTUSE:
        return 1;

    case DIRECTION_45::ANG_STRAIGHT:
        return 0;

    case DIRECTION_45::ANG_ACUTE:
        return 50;

    case DIRECTION_45::ANG_RIGHT:
        return 30;

    case DIRECTION_45::ANG_HALF_FULL:
        return 60;

    default:
        return 100;
    }
}


int PNS_COST_ESTIMATOR::CornerCost( const SHAPE_LINE_CHAIN& aLine )
{
    int total = 0;

    for( int i = 0; i < aLine.SegmentCount() - 1; ++i )
        total += CornerCost( aLine.CSegment( i ), aLine.CSegment( i + 1 ) );

    return total;
}


int PNS_COST_ESTIMATOR::CornerCost( const PNS_LINE& aLine )
{
    return CornerCost( aLine.CLine() );
}


void PNS_COST_ESTIMATOR::Add( PNS_LINE& aLine )
{
    m_lengthCost += aLine.CLine().Length();
    m_cornerCost += CornerCost( aLine );
}


void PNS_COST_ESTIMATOR::Remove( PNS_LINE& aLine )
{
    m_lengthCost -= aLine.CLine().Length();
    m_cornerCost -= CornerCost( aLine );
}


void PNS_COST_ESTIMATOR::Replace( PNS_LINE& aOldLine, PNS_LINE& aNewLine )
{
    m_lengthCost -= aOldLine.CLine().Length();
    m_cornerCost -= CornerCost( aOldLine );
    m_lengthCost += aNewLine.CLine().Length();
    m_cornerCost += CornerCost( aNewLine );
}


bool PNS_COST_ESTIMATOR::IsBetter( PNS_COST_ESTIMATOR& aOther,
        double aLengthTollerance,
        double aCornerTollerance ) const
{
    if( aOther.m_cornerCost < m_cornerCost && aOther.m_lengthCost < m_lengthCost )
        return true;

    else if( aOther.m_cornerCost < m_cornerCost * aCornerTollerance &&
             aOther.m_lengthCost < m_lengthCost * aLengthTollerance )
        return true;

    return false;
}


/**
 *  Optimizer
 **/
PNS_OPTIMIZER::PNS_OPTIMIZER( PNS_NODE* aWorld ) :
    m_world( aWorld ), m_collisionKindMask( PNS_ITEM::ANY ), m_effortLevel( MERGE_SEGMENTS )
{
    // m_cache = new SHAPE_INDEX_LIST<PNS_ITEM*>();
}


PNS_OPTIMIZER::~PNS_OPTIMIZER()
{
    // delete m_cache;
}


struct PNS_OPTIMIZER::CACHE_VISITOR
{
    CACHE_VISITOR( const PNS_ITEM* aOurItem, PNS_NODE* aNode, int aMask ) :
        m_ourItem( aOurItem ),
        m_collidingItem( NULL ),
        m_node( aNode ),
        m_mask( aMask )
    {}

    bool operator()( PNS_ITEM* aOtherItem )
    {
        if( !m_mask & aOtherItem->Kind() )
            return true;

        int clearance = m_node->GetClearance( aOtherItem, m_ourItem );

        if( !aOtherItem->Collide( m_ourItem, clearance ) )
            return true;

        m_collidingItem = aOtherItem;
        return false;
    }

    const PNS_ITEM* m_ourItem;
    PNS_ITEM* m_collidingItem;
    PNS_NODE* m_node;
    int m_mask;
};


void PNS_OPTIMIZER::cacheAdd( PNS_ITEM* aItem, bool aIsStatic = false )
{
    if( m_cacheTags.find( aItem ) != m_cacheTags.end() )
        return;

    m_cache.Add( aItem );
    m_cacheTags[aItem].m_hits = 1;
    m_cacheTags[aItem].m_isStatic = aIsStatic;
}


void PNS_OPTIMIZER::removeCachedSegments( PNS_LINE* aLine, int aStartVertex, int aEndVertex )
{
    PNS_LINE::SEGMENT_REFS* segs = aLine->LinkedSegments();

    if( !segs )
        return;

    if( aEndVertex < 0 )
        aEndVertex += aLine->PointCount();

    for( int i = aStartVertex; i < aEndVertex - 1; i++ )
    {
        PNS_SEGMENT* s = (*segs)[i];
        m_cacheTags.erase( s );
        m_cache.Remove( s );
    }    // *cacheRemove( (*segs)[i] );
}


void PNS_OPTIMIZER::CacheRemove( PNS_ITEM* aItem )
{
    if( aItem->Kind() == PNS_ITEM::LINE )
        removeCachedSegments( static_cast<PNS_LINE*>( aItem ) );
}


void PNS_OPTIMIZER::CacheStaticItem( PNS_ITEM* aItem )
{
    cacheAdd( aItem, true );
}


void PNS_OPTIMIZER::ClearCache( bool aStaticOnly  )
{
    if( !aStaticOnly )
    {
        m_cacheTags.clear();
        m_cache.Clear();
        return;
    }

    for( CachedItemTags::iterator i = m_cacheTags.begin(); i!= m_cacheTags.end(); ++i )
    {
        if( i->second.m_isStatic )
        {
            m_cache.Remove( i->first );
            m_cacheTags.erase( i->first );
        }
    }
}


bool PNS_OPTIMIZER::checkColliding( PNS_ITEM* aItem, bool aUpdateCache )
{
    CACHE_VISITOR v( aItem, m_world, m_collisionKindMask );

    return m_world->CheckColliding( aItem );

    // something is wrong with the cache, need to investigate.
    m_cache.Query( aItem->Shape(), m_world->GetMaxClearance(), v, false );

    if( !v.m_collidingItem )
    {
        PNS_NODE::OPT_OBSTACLE obs = m_world->CheckColliding( aItem );

        if( obs )
        {
            if( aUpdateCache )
                cacheAdd( obs->m_item );

            return true;
        }
    }
    else
    {
        m_cacheTags[v.m_collidingItem].m_hits++;
        return true;
    }

    return false;
}


bool PNS_OPTIMIZER::checkColliding( PNS_LINE* aLine, const SHAPE_LINE_CHAIN& aOptPath )
{
    PNS_LINE tmp( *aLine, aOptPath );

    return checkColliding( &tmp );
}


bool PNS_OPTIMIZER::mergeObtuse( PNS_LINE* aLine )
{
    SHAPE_LINE_CHAIN& line = aLine->Line();

    int step = line.PointCount() - 3;
    int iter = 0;
    int segs_pre = line.SegmentCount();

    if( step < 0 )
        return false;

    SHAPE_LINE_CHAIN current_path( line );

    while( 1 )
    {
        iter++;
        int n_segs = current_path.SegmentCount();
        int max_step = n_segs - 2;

        if( step > max_step )
            step = max_step;

        if( step < 2 )
        {
            line = current_path;
            return current_path.SegmentCount() < segs_pre;
        }

        bool found_anything = false;
        int n = 0;

        while( n < n_segs - step )
        {
            const SEG s1 = current_path.CSegment( n );
            const SEG s2 = current_path.CSegment( n + step );
            SEG s1opt, s2opt;

            if( DIRECTION_45( s1 ).IsObtuse( DIRECTION_45( s2 ) ) )
            {
                VECTOR2I ip = *s1.IntersectLines( s2 );

                if( s1.Distance( ip ) <= 1 || s2.Distance( ip ) <= 1 )
                {
                    s1opt = SEG( s1.A, ip );
                    s2opt = SEG( ip, s2.B );
                }
                else
                {
                    s1opt = SEG( s1.A, ip );
                    s2opt = SEG( ip, s2.B );
                }


                if( DIRECTION_45( s1opt ).IsObtuse( DIRECTION_45( s2opt ) ) )
                {
                    SHAPE_LINE_CHAIN opt_path;
                    opt_path.Append( s1opt.A );
                    opt_path.Append( s1opt.B );
                    opt_path.Append( s2opt.B );

                    PNS_LINE opt_track( *aLine, opt_path );

                    if( !checkColliding( &opt_track ) )
                    {
                        current_path.Replace( s1.Index() + 1, s2.Index(), ip );
                        // removeCachedSegments(aLine, s1.Index(), s2.Index());
                        n_segs = current_path.SegmentCount();
                        found_anything = true;
                        break;
                    }
                }
            }

            n++;
        }

        if( !found_anything )
        {
            if( step <= 2 )
            {
                line = current_path;
                return line.SegmentCount() < segs_pre;
            }

            step--;
        }
    }

    return line.SegmentCount() < segs_pre;
}


bool PNS_OPTIMIZER::mergeFull( PNS_LINE* aLine )
{
    SHAPE_LINE_CHAIN& line = aLine->Line();
    int step = line.SegmentCount() - 1;

    int segs_pre = line.SegmentCount();

    line.Simplify();

    if( step < 0 )
        return false;

    SHAPE_LINE_CHAIN current_path( line );

    while( 1 )
    {
        int n_segs = current_path.SegmentCount();
        int max_step = n_segs - 2;

        if( step > max_step )
            step = max_step;

        if( step < 1 )
            break;

        bool found_anything = mergeStep( aLine, current_path, step );

        if( !found_anything )
            step--;
    }

    aLine->SetShape( current_path );

    return current_path.SegmentCount() < segs_pre;
}


bool PNS_OPTIMIZER::Optimize( PNS_LINE* aLine, PNS_LINE* aResult )//, int aStartVertex, int aEndVertex )
{
    if( !aResult )
        aResult = aLine;
    else
        *aResult = *aLine;

    m_keepPostures = false;

    bool rv = false;

    if( m_effortLevel & MERGE_SEGMENTS )
        rv |= mergeFull( aResult );

    if( m_effortLevel & MERGE_OBTUSE )
        rv |= mergeObtuse( aResult );

    if( m_effortLevel & SMART_PADS )
        rv |= runSmartPads( aResult );

    if( m_effortLevel & FANOUT_CLEANUP )
        rv |= fanoutCleanup( aResult );

    return rv;
}


bool PNS_OPTIMIZER::mergeStep( PNS_LINE* aLine, SHAPE_LINE_CHAIN& aCurrentPath, int step )
{
    int n = 0;
    int n_segs = aCurrentPath.SegmentCount();

    int cost_orig = PNS_COST_ESTIMATOR::CornerCost( aCurrentPath );

    if( aLine->SegmentCount() < 4 )
        return false;

    DIRECTION_45 orig_start( aLine->CSegment( 0 ) );
    DIRECTION_45 orig_end( aLine->CSegment( -1 ) );

    while( n < n_segs - step )
    {
        const SEG s1    = aCurrentPath.CSegment( n );
        const SEG s2    = aCurrentPath.CSegment( n + step );

        SHAPE_LINE_CHAIN path[2];
        SHAPE_LINE_CHAIN* picked = NULL;
        int cost[2];

        for( int i = 0; i < 2; i++ )
        {
            bool postureMatch = true;
            SHAPE_LINE_CHAIN bypass = DIRECTION_45().BuildInitialTrace( s1.A, s2.B, i );
            cost[i] = INT_MAX;


            if( n == 0 && orig_start != DIRECTION_45( bypass.CSegment( 0 ) ) )
                postureMatch = false;
            else if( n == n_segs - step && orig_end != DIRECTION_45( bypass.CSegment( -1 ) ) )
                postureMatch = false;

            if( (postureMatch || !m_keepPostures) && !checkColliding( aLine, bypass ) )
            {
                path[i] = aCurrentPath;
                path[i].Replace( s1.Index(), s2.Index(), bypass );
                path[i].Simplify();
                cost[i] = PNS_COST_ESTIMATOR::CornerCost( path[i] );
            }
        }

        if( cost[0] < cost_orig && cost[0] < cost[1] )
            picked = &path[0];
        else if( cost[1] < cost_orig )
            picked = &path[1];

        if( picked )
        {
            n_segs = aCurrentPath.SegmentCount();
            aCurrentPath = *picked;
            return true;
        }

        n++;
    }

    return false;
}


PNS_OPTIMIZER::BREAKOUT_LIST PNS_OPTIMIZER::circleBreakouts( int aWidth,
        const SHAPE* aShape, bool aPermitDiagonal ) const
{
    BREAKOUT_LIST breakouts;

    for( int angle = 0; angle < 360; angle += 45 )
    {
        const SHAPE_CIRCLE* cir = static_cast<const SHAPE_CIRCLE*>( aShape );
        SHAPE_LINE_CHAIN l;
        VECTOR2I p0 = cir->GetCenter();
        VECTOR2I v0( cir->GetRadius() * M_SQRT2, 0 );
        l.Append( p0 );
        l.Append( p0 + v0.Rotate( angle * M_PI / 180.0 ) );
        breakouts.push_back( l );
    }

    return breakouts;
}


PNS_OPTIMIZER::BREAKOUT_LIST PNS_OPTIMIZER::rectBreakouts( int aWidth,
        const SHAPE* aShape, bool aPermitDiagonal ) const
{
    const SHAPE_RECT* rect = static_cast<const SHAPE_RECT*>(aShape);
    VECTOR2I s = rect->GetSize(), c = rect->GetPosition() + VECTOR2I( s.x / 2, s.y / 2 );
    BREAKOUT_LIST breakouts;

    VECTOR2I d_offset;

    d_offset.x = ( s.x > s.y ) ? ( s.x - s.y ) / 2 : 0;
    d_offset.y = ( s.x < s.y ) ? ( s.y - s.x ) / 2 : 0;

    VECTOR2I d_vert  = VECTOR2I( 0, s.y / 2 + aWidth );
    VECTOR2I d_horiz = VECTOR2I( s.x / 2 + aWidth, 0 );

    breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_horiz ) );
    breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_horiz ) );
    breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_vert ) );
    breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_vert ) );

    if( aPermitDiagonal )
    {
        int l = aWidth + std::min( s.x, s.y ) / 2;
        VECTOR2I d_diag;

        if( s.x >= s.y )
        {
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_offset,
                                                   c + d_offset + VECTOR2I( l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_offset,
                                                   c + d_offset - VECTOR2I( -l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_offset,
                                                   c - d_offset + VECTOR2I( -l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_offset,
                                                   c - d_offset - VECTOR2I( l, l ) ) );
        }
        else
        {
            // fixme: this could be done more efficiently
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_offset,
                                                   c + d_offset + VECTOR2I( l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_offset,
                                                   c - d_offset - VECTOR2I( -l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_offset,
                                                   c + d_offset + VECTOR2I( -l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_offset,
                                                   c - d_offset - VECTOR2I( l, l ) ) );
        }
    }

    return breakouts;
}


PNS_OPTIMIZER::BREAKOUT_LIST PNS_OPTIMIZER::computeBreakouts( int aWidth,
        const PNS_ITEM* aItem, bool aPermitDiagonal ) const
{
    switch( aItem->Kind() )
    {
    case PNS_ITEM::VIA:
    {
        const PNS_VIA* via = static_cast<const PNS_VIA*>( aItem );
        return circleBreakouts( aWidth, via->Shape(), aPermitDiagonal );
    }

    case PNS_ITEM::SOLID:
    {
        const SHAPE* shape = aItem->Shape();

        switch( shape->Type() )
        {
        case SH_RECT:
            return rectBreakouts( aWidth, shape, aPermitDiagonal );

        case SH_SEGMENT:
        {
            const SHAPE_SEGMENT* seg = static_cast<const SHAPE_SEGMENT*> (shape);
            const SHAPE_RECT rect = ApproximateSegmentAsRect ( *seg );
            return rectBreakouts( aWidth, &rect, aPermitDiagonal );
        }

        case SH_CIRCLE:
            return circleBreakouts( aWidth, shape, aPermitDiagonal );

        default:
            break;
        }
    }

    default:
        break;
    }

    return BREAKOUT_LIST();
}


PNS_ITEM* PNS_OPTIMIZER::findPadOrVia( int aLayer, int aNet, const VECTOR2I& aP ) const
{
    PNS_JOINT* jt = m_world->FindJoint( aP, aLayer, aNet );

    if( !jt )
        return NULL;

    BOOST_FOREACH( PNS_ITEM* item, jt->LinkList() )
    {
        if( item->OfKind( PNS_ITEM::VIA | PNS_ITEM::SOLID ) )
            return item;
    }

    return NULL;
}


int PNS_OPTIMIZER::smartPadsSingle( PNS_LINE* aLine, PNS_ITEM* aPad, bool aEnd, int aEndVertex )
{
    int min_cost = INT_MAX; // PNS_COST_ESTIMATOR::CornerCost( line );
    int min_len = INT_MAX;
    DIRECTION_45 dir;

    const int ForbiddenAngles = DIRECTION_45::ANG_ACUTE | DIRECTION_45::ANG_RIGHT |
                                DIRECTION_45::ANG_HALF_FULL | DIRECTION_45::ANG_UNDEFINED;

    typedef std::pair<int, SHAPE_LINE_CHAIN> RtVariant;
    std::vector<RtVariant> variants;

    BREAKOUT_LIST breakouts = computeBreakouts( aLine->Width(), aPad, true );

    SHAPE_LINE_CHAIN line = ( aEnd ? aLine->CLine().Reverse() : aLine->CLine() );


    int p_end = std::min( aEndVertex, std::min( 3, line.PointCount() - 1 ) );

    for( int p = 1; p <= p_end; p++ )
    {
        BOOST_FOREACH( SHAPE_LINE_CHAIN & l, breakouts ) {

            for( int diag = 0; diag < 2; diag++ )
            {
                SHAPE_LINE_CHAIN v;
                SHAPE_LINE_CHAIN connect = dir.BuildInitialTrace( l.CPoint( -1 ),
                                                                  line.CPoint( p ), diag == 0 );

                DIRECTION_45 dir_bkout( l.CSegment( -1 ) );

                if(!connect.SegmentCount())
                    continue;
                
                int ang1 = dir_bkout.Angle( DIRECTION_45( connect.CSegment( 0 ) ) );
                int ang2 = 0;

                if( (ang1 | ang2) & ForbiddenAngles )
                    continue;

                if( l.Length() > line.Length() )
                    continue;

                v = l;

                v.Append( connect );

                for( int i = p + 1; i < line.PointCount(); i++ )
                    v.Append( line.CPoint( i ) );

                PNS_LINE tmp( *aLine, v );
                int cc = tmp.CountCorners( ForbiddenAngles );

                if( cc == 0 )
                {
                    RtVariant vp;
                    vp.first = p;
                    vp.second = aEnd ? v.Reverse() : v;
                    vp.second.Simplify();
                    variants.push_back( vp );
                }
            }
        }
    }

    SHAPE_LINE_CHAIN l_best;
    bool found = false;
    int p_best = -1;

    BOOST_FOREACH( RtVariant& vp, variants )
    {
        PNS_LINE tmp( *aLine, vp.second );
        int cost = PNS_COST_ESTIMATOR::CornerCost( vp.second );
        int len = vp.second.Length();

        if( !checkColliding( &tmp ) )
        {
            if( cost < min_cost || ( cost == min_cost && len < min_len ) )
            {
                l_best = vp.second;
                p_best = vp.first;
                found  = true;

                if( cost == min_cost )
                    min_len = std::min( len, min_len );

                min_cost = std::min( cost, min_cost );
            }
        }
    }

    if( found )
    {
        aLine->SetShape( l_best );
        return p_best;
    }

    return -1;
}

bool PNS_OPTIMIZER::runSmartPads( PNS_LINE* aLine )
{
    SHAPE_LINE_CHAIN& line = aLine->Line();
    
    if( line.PointCount() < 3 )
        return false;

    VECTOR2I p_start = line.CPoint( 0 ), p_end = line.CPoint( -1 );

    PNS_ITEM* startPad = findPadOrVia( aLine->Layer(), aLine->Net(), p_start );
    PNS_ITEM* endPad = findPadOrVia( aLine->Layer(), aLine->Net(), p_end );

    int vtx = -1;

    if( startPad )
        vtx = smartPadsSingle( aLine, startPad, false, 3 );

    if( endPad )
        smartPadsSingle( aLine, endPad, true,
                         vtx < 0 ? line.PointCount() - 1 : line.PointCount() - 1 - vtx );

    aLine->Line().Simplify();

    return true;
}


bool PNS_OPTIMIZER::Optimize( PNS_LINE* aLine, int aEffortLevel, PNS_NODE* aWorld )
{
    PNS_OPTIMIZER opt( aWorld );

    opt.SetEffortLevel( aEffortLevel );
    opt.SetCollisionMask( -1 );
    return opt.Optimize( aLine );
}


bool PNS_OPTIMIZER::fanoutCleanup( PNS_LINE* aLine )
{
    if( aLine->PointCount() < 3 )
        return false;

    VECTOR2I p_start = aLine->CPoint( 0 ), p_end = aLine->CPoint( -1 );

    PNS_ITEM* startPad = findPadOrVia( aLine->Layer(), aLine->Net(), p_start );
    PNS_ITEM* endPad = findPadOrVia( aLine->Layer(), aLine->Net(), p_end );

    int thr = aLine->Width() * 10; 
    int len = aLine->CLine().Length();

    if( !startPad )
        return false;

    bool startMatch = startPad->OfKind( PNS_ITEM::VIA | PNS_ITEM::SOLID );
    bool endMatch = false;   

    if(endPad)
    {
        endMatch = endPad->OfKind( PNS_ITEM::VIA | PNS_ITEM::SOLID );
    } else {
        endMatch = aLine->EndsWithVia();
    }
    
    if( startMatch && endMatch && len < thr )
    {
        for(int i = 0; i < 2; i++ )
        {
            SHAPE_LINE_CHAIN l2 = DIRECTION_45().BuildInitialTrace( p_start, p_end, i );
            PNS_ROUTER::GetInstance()->DisplayDebugLine( l2, 4, 10000 );
            PNS_LINE repl;
            repl = PNS_LINE( *aLine, l2 );

            if( !m_world->CheckColliding( &repl ) )
            {
                aLine->SetShape( repl.CLine() );
                return true;
            }
        }
    }

    return false;
}

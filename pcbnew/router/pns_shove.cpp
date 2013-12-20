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

#include <deque>
#include <cassert>

#include <wx/timer.h>

#include "trace.h"

#include "pns_line.h"
#include "pns_node.h"
#include "pns_walkaround.h"
#include "pns_shove.h"
#include "pns_optimizer.h"
#include "pns_via.h"
#include "pns_utils.h"

#include <profile.h>

PNS_SHOVE::PNS_SHOVE( PNS_NODE* aWorld )
{
    m_root = aWorld;
    m_iterLimit = 100;
};


PNS_SHOVE::~PNS_SHOVE()
{
}


struct range
{
    range()
    {
        min_v = max_v = -1;
    }

    void add( int x )
    {
        if( min_v < 0 ) min_v = x;

        if( max_v < 0 ) max_v = x;

        if( x < min_v )
            min_v = x;
        else if( x > max_v )
            max_v = x;
    }

    int start()
    {
        return min_v;
    }

    int end()
    {
        return max_v;
    }

    int min_v, max_v;
};

// fixme: this is damn f***ing inefficient. And fails much too often due to broken direction finding algorithm.
bool PNS_SHOVE::tryShove( PNS_NODE* aNode, PNS_LINE* aHead, PNS_LINE* aObstacle,
        PNS_SEGMENT& aObstacleSeg, PNS_LINE* aResult, bool aInvertWinding )
{
    const SHAPE_LINE_CHAIN& head = aHead->GetCLine();
    bool cw = false;
    int i;

    if( aHead->EndsWithVia() && !aHead->GetLayers().Overlaps( aObstacle->GetLayers() ) )
    {
        int clearance = aNode->GetClearance( aHead, aObstacle );
        SHAPE_LINE_CHAIN hull = aHead->GetVia().Hull( clearance - aObstacle->GetWidth() / 2 );

        // SHAPE_LINE_CHAIN path_pre, path_walk_cw, path_walk_ccw, path_post;

        SHAPE_LINE_CHAIN path_cw, path_ccw, * path;

        aObstacle->NewWalkaround( hull, path_cw, true );
        aObstacle->NewWalkaround( hull, path_ccw, false );

        path = path_ccw.Length() < path_cw.Length() ? &path_ccw : &path_cw;
        aResult->SetShape( *path );

        // PNSDisplayDebugLine (*path, 5);

        if( !aResult->Is45Degree() )
        {
            // printf("polyset non-45\npoly %s\nendpolyset\n", aResult->GetCLine().Format().c_str());
        }

        /*... special case for vias? */

        return !aNode->CheckColliding( aResult, aHead );
    }

    int ns = head.SegmentCount();

    if( aHead->EndsWithVia() )
        ns++;

    for( i = 0; i < head.SegmentCount(); i++ )
    {
        const PNS_SEGMENT hs( *aHead, head.CSegment( i ) );


        if( aNode->CheckColliding( &hs, aObstacle ) )
        {
            VECTOR2I v1 = hs.GetSeg().B - hs.GetSeg().A;
            VECTOR2I v2 = aObstacleSeg.GetSeg().B - aObstacleSeg.GetSeg().A;

            VECTOR2I::extended_type det = v1.Cross( v2 );

            if( det > 0 )
                cw = true;
            else
                cw = false;

            break;
        }
    }

    if( aInvertWinding )
    {
        if( cw )
            cw = false;
        else
            cw = true;
    }

    PNS_LINE shoved( *aObstacle );

    int clearance = aNode->GetClearance( aHead, aObstacle );

    range r;

    for( i = 0; i < ns; i++ )
    {
        SHAPE_LINE_CHAIN hull;

        if( i < head.SegmentCount() )
        {
            const PNS_SEGMENT hs( *aHead, head.CSegment( i ) );
            hull = hs.Hull( clearance, 0 );
        }
        else
            hull = aHead->GetVia().Hull( clearance - aObstacle->GetWidth() / 2 );

        SHAPE_LINE_CHAIN path_pre, path_walk, path_post, tmp;
        SHAPE_LINE_CHAIN path_pre2, path_walk2, path_post2;

        // shoved.NewWalkaround(hull, path_pre, path_walk, path_post, cw);
        shoved.NewWalkaround( hull, path_pre, path_walk, path_post, cw );

        /*if(path_pre != path_pre2 || path_post != path_post2 || path_walk != path_walk2 )
         *  {
         *   TRACE(5, "polyset orig\npoly %s\npoly %s\npoly %s\nendpolyset\n", path_pre.Format().c_str() % path_walk.Format().c_str() % path_post.Format().c_str());
         *   TRACE(5, "polyset err\npoly %s\npoly %s\npoly %s\nendpolyset\n", path_pre2.Format().c_str() % path_walk2.Format().c_str() % path_post2.Format().c_str());
         *  }*/

        tmp = shoved.GetCLine();

        if( path_walk.SegmentCount() )
            r.add( i );

        path_pre.Append( path_walk );
        path_pre.Append( path_post );
        path_pre.Simplify();
        shoved.SetShape( path_pre );
// shoved.SetAffectedRange ( start, end );
        *aResult = shoved;

        if( !aResult->Is45Degree() )
        {
            // TRACE(5, "polyset non-45\npoly %s\npoly %s\npoly %s\nendpolyset\n", tmp.Format().c_str() % hull.Format().c_str() % aResult->GetCLine().Format().c_str());
        }
    }

    TRACE( 2, "CW %d affectedRange %d-%d [total %d]", (cw ? 1 : 0) % r.start() % r.end() % ns );

    return !aNode->CheckColliding( aResult, aHead );
}


PNS_SHOVE::ShoveStatus PNS_SHOVE::shoveSingleLine( PNS_NODE* aNode, PNS_LINE* aCurrent,
        PNS_LINE* aObstacle, PNS_SEGMENT& aObstacleSeg, PNS_LINE* aResult )
{
    bool rv = tryShove( aNode, aCurrent, aObstacle, aObstacleSeg, aResult, false );

    if( !rv )
        rv = tryShove( aNode, aCurrent, aObstacle, aObstacleSeg, aResult, true );

    if( !rv )
    {
        TRACEn( 2, "Shove failed" );
        return SH_INCOMPLETE;
    }

    aResult->GetLine().Simplify();

    const SHAPE_LINE_CHAIN& sh_shoved = aResult->GetCLine();
    const SHAPE_LINE_CHAIN& sh_orig = aObstacle->GetCLine();

    if( sh_shoved.SegmentCount() > 1 && sh_shoved.CPoint( 0 ) == sh_orig.CPoint( 0 )
        && sh_shoved.CPoint( -1 ) == sh_orig.CPoint( -1 ) )
        return SH_OK;
    else if( !sh_shoved.SegmentCount() )
        return SH_NULL;
    else
        return SH_INCOMPLETE;
}


bool PNS_SHOVE::reduceSpringback( PNS_LINE* aHead )
{
    bool rv = false;

    while( !m_nodeStack.empty() )
    {
        SpringbackTag st_stack = m_nodeStack.back();
        bool tail_ok = true;

        if( !st_stack.node->CheckColliding( aHead ) && tail_ok )
        {
            rv = true;
            delete st_stack.node;
            m_nodeStack.pop_back();
        }
        else
            break;
    }

    return rv;
}


bool PNS_SHOVE::pushSpringback( PNS_NODE* aNode, PNS_LINE* aHead, const PNS_COST_ESTIMATOR& aCost )
{
    BOX2I headBB = aHead->GetCLine().BBox();
    SpringbackTag st;

    st.node = aNode;
    st.cost = aCost;
    st.length = std::max( headBB.GetWidth(), headBB.GetHeight() );;
    m_nodeStack.push_back( st );
    return true;
}


const PNS_COST_ESTIMATOR PNS_SHOVE::TotalCost() const
{
    if( m_nodeStack.empty() )
        return PNS_COST_ESTIMATOR();
    else
        return m_nodeStack.back().cost;
}


PNS_SHOVE::ShoveStatus PNS_SHOVE::ShoveLines( PNS_LINE* aCurrentHead )
{
    std::stack<PNS_LINE*> lineStack;
    PNS_NODE* node, * parent;
    PNS_VIA* headVia = NULL;
    bool    fail    = false;
    int     iter    = 0;

    PNS_LINE* head = aCurrentHead->Clone();

    reduceSpringback( aCurrentHead );

    parent = m_nodeStack.empty() ? m_root : m_nodeStack.back().node;
    node = parent->Branch();

    lineStack.push( head );

    // node->Add(tail);
    node->Add( head );

    if( head->EndsWithVia() )
    {
        headVia = head->GetVia().Clone();
        node->Add( headVia );
    }

    PNS_OPTIMIZER optimizer( node );

    optimizer.SetEffortLevel( PNS_OPTIMIZER::MERGE_SEGMENTS | PNS_OPTIMIZER::SMART_PADS );
    optimizer.SetCollisionMask( -1 );
    PNS_NODE::OptObstacle nearest;

    optimizer.CacheStaticItem( head );

    if( headVia )
        optimizer.CacheStaticItem( headVia );

    TRACE( 1, "ShoveStart [root: %d jts, node: %d jts]", m_root->JointCount() %
            node->JointCount() );

    // PNS_ITEM *lastWalkSolid = NULL;
    prof_counter totalRealTime;

    wxLongLong t_start = wxGetLocalTimeMillis();

    while( !lineStack.empty() )
    {
        wxLongLong t_cur = wxGetLocalTimeMillis();

        if( (t_cur - t_start).ToLong() > ShoveTimeLimit )
        {
            fail = true;
            break;
        }

        iter++;

        if( iter > m_iterLimit )
        {
            fail = true;
            break;
        }

        PNS_LINE* currentLine = lineStack.top();

        prof_start( &totalRealTime );
        nearest = node->NearestObstacle( currentLine, PNS_ITEM::ANY );
        prof_end( &totalRealTime );

        TRACE( 2, "t-nearestObstacle %lld us", totalRealTime.usecs() );

        if( !nearest )
        {
            if( lineStack.size() > 1 )
            {
                PNS_LINE* original = lineStack.top();
                PNS_LINE optimized;
                int r_start, r_end;

                original->GetAffectedRange( r_start, r_end );

                TRACE( 1, "Iter %d optimize-line [range %d-%d, total %d]",
                        iter % r_start % r_end % original->GetCLine().PointCount() );
                // lastWalkSolid = NULL;
                prof_start( &totalRealTime );

                if( optimizer.Optimize( original, &optimized ) )
                {
                    node->Remove( original );
                    optimizer.CacheRemove( original );
                    node->Add( &optimized );

                    if( original->BelongsTo( node ) )
                        delete original;
                }

                prof_end( &totalRealTime );

                TRACE( 2, "t-optimizeObstacle %lld us", totalRealTime.usecs() );
            }

            lineStack.pop();
        }
        else
        {
            switch( nearest->item->GetKind() )
            {
            case PNS_ITEM::SEGMENT:
                {
                    TRACE( 1, "Iter %d shove-line", iter );

                    PNS_SEGMENT* pseg = static_cast<PNS_SEGMENT*>(nearest->item);
                    PNS_LINE* collidingLine = node->AssembleLine( pseg );
                    PNS_LINE* shovedLine = collidingLine->CloneProperties();

                    prof_start( &totalRealTime );
                    ShoveStatus st = shoveSingleLine( node, currentLine, collidingLine,
                            *pseg, shovedLine );
                    prof_end( &totalRealTime );

                    TRACE( 2, "t-shoveSingle %lld us", totalRealTime.usecs() );

                    if( st == SH_OK )
                    {
                        node->Replace( collidingLine, shovedLine );

                        if( collidingLine->BelongsTo( node ) )
                            delete collidingLine;

                        optimizer.CacheRemove( collidingLine );
                        lineStack.push( shovedLine );
                    }
                    else
                        fail = true;

                    // lastWalkSolid = NULL;

                    break;
                }    // case SEGMENT

            case PNS_ITEM::SOLID:
            case PNS_ITEM::VIA:
                {
                    TRACE( 1, "Iter %d walkaround-solid [%p]", iter % nearest->item );

                    if( lineStack.size() == 1 )
                    {
                        fail = true;
                        break;
                    }

/*                    if(lastWalkSolid == nearest->item)
 *                   {
 *                       fail = true;
 *                       break;
 *                   }*/

                    PNS_WALKAROUND walkaround( node );
                    PNS_LINE* walkaroundLine = currentLine->CloneProperties();

                    walkaround.SetSolidsOnly( true );
                    walkaround.SetSingleDirection( true );

                    prof_start( &totalRealTime );
                    walkaround.Route( *currentLine, *walkaroundLine, false );
                    prof_end( &totalRealTime );

                    TRACE( 2, "t-walkSolid %lld us", totalRealTime.usecs() );


                    node->Replace( currentLine, walkaroundLine );

                    if( currentLine->BelongsTo( node ) )
                        delete currentLine;

                    optimizer.CacheRemove( currentLine );
                    lineStack.top() = walkaroundLine;

                    // lastWalkSolid = nearest->item;
                    break;
                }

            default:
                break;
            }    // switch

            if( fail )
                break;
        }
    }

    node->Remove( head );
    delete head;

    if( headVia )
    {
        node->Remove( headVia );
        delete headVia;
    }

    TRACE( 1, "Shove status : %s after %d iterations", (fail ? "FAILED" : "OK") % iter );

    if( !fail )
    {
        pushSpringback( node, aCurrentHead, PNS_COST_ESTIMATOR() );
        return SH_OK;
    }
    else
    {
        delete node;
        return SH_INCOMPLETE;
    }
}

/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
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

#include "pns_multi_dragger.h"
#include "pns_router.h"
#include "pns_debug_decorator.h"
#include "pns_walkaround.h"
#include "pns_shove.h"

namespace PNS
{

MULTI_DRAGGER::MULTI_DRAGGER( ROUTER* aRouter ) : DRAG_ALGO( aRouter )
{
    m_world = nullptr;
    m_lastNode = nullptr;
}


MULTI_DRAGGER::~MULTI_DRAGGER()
{
}

// here we initialize everything that's needed for multidrag. this means:
bool MULTI_DRAGGER::Start( const VECTOR2I& aP, ITEM_SET& aPrimitives )
{
    m_lastNode = nullptr;
    m_dragStatus = false;
    m_dragStartPoint = aP;

    // check if the initial ("leader") primitive set is empty...
    if( aPrimitives.Empty() )
        return false;

    m_mdragLines.clear();

    // find all LINEs to be dragged. Indicate the LINE that contains the point (aP)
    // as the "primary line", the multidrag algo will place all other lines in such way
    // that the cursor position lies on the primary line.
    for( ITEM* pitem : aPrimitives.Items() )
    {
        LINKED_ITEM* litem = static_cast<LINKED_ITEM*>( pitem );
        bool         redundant = false;
        for( auto& l : m_mdragLines )
        {
            if( l.originalLine.ContainsLink( litem ) )
            {
                l.originalLeaders.push_back( litem );
                redundant = true;
                break;
            }
        }

        // we can possibly have multiple SEGMENTs in aPrimitives that belong to the same line.
        // We reject these.
        if( !redundant )
        {
            MDRAG_LINE l;
            l.originalLine = m_world->AssembleLine( litem );
            l.originalLeaders.push_back( litem );
            l.isDraggable = true;
            m_mdragLines.push_back( std::move( l ) );
        }
    }

    int n = 0;

    bool anyStrictCornersFound = false;
    bool anyStrictMidSegsFound = false;

    for( auto& l : m_mdragLines )
    {
        const int       thr = l.originalLine.Width() / 2;

        const VECTOR2I& origFirst = l.originalLine.CLine().CPoint( 0 );
        const int       distFirst = ( origFirst - aP ).EuclideanNorm();

        const VECTOR2I& origLast = l.originalLine.CLine().CLastPoint();
        const int       distLast = ( origLast - aP ).EuclideanNorm();

        l.cornerDistance = std::min( distFirst, distLast );

        bool takeFirst = false;
        auto ilast = aPrimitives.FindVertex( origLast );
        auto ifirst = aPrimitives.FindVertex( origFirst );

        if( ilast && ifirst )
            takeFirst = distFirst < distLast;
        else if( ilast )
            takeFirst = false;
        else if( ifirst )
            takeFirst = true;

        if( ifirst || ilast )
        {
            if( takeFirst )
            {
                l.cornerIsLast = false;
                l.leaderSegIndex = 0;
                l.cornerDistance = distFirst;
                l.isCorner = true;

                if( distFirst <= thr )
                {
                    l.isStrict = true;
                    l.cornerDistance = 0;
                }
            }
            else
            {
                l.cornerIsLast = true;
                l.leaderSegIndex = l.originalLine.SegmentCount() - 1;
                l.cornerDistance = distLast;
                l.isCorner = true;

                if( distLast <= thr )
                {
                    l.isStrict = true;
                    l.cornerDistance = 0;
                }
            }
        }

        const auto& links = l.originalLine.Links();

            for( int lidx = 0; lidx < (int) links.size(); lidx++ )
            {
                if( auto lseg = dyn_cast<SEGMENT*>( links[lidx] ) )
                {

                    if( !aPrimitives.Contains( lseg ) )
                        continue;

                        int d = lseg->Seg().Distance( aP );

                        l.midSeg = lseg->Seg();
                        l.isMidSeg = true;
                        l.leaderSegIndex = lidx;
                        l.leaderSegDistance = d + thr;

                        if( d < thr && !l.isStrict )
                        {
                            l.isCorner = false;
                            l.isStrict = true;
                            l.leaderSegDistance = 0;
                        }
                }
            }

         if( l.isStrict )
        {
            anyStrictCornersFound |= l.isCorner;
            anyStrictMidSegsFound |= !l.isCorner;
        }
    }

    if( anyStrictCornersFound )
        m_dragMode = DM_CORNER;
    else if (anyStrictMidSegsFound )
        m_dragMode = DM_SEGMENT;
    else
    {
        int minLeadSegDist = std::numeric_limits<int>::max();
        int minCornerDist = std::numeric_limits<int>::max();
        MDRAG_LINE *bestSeg = nullptr;
        MDRAG_LINE *bestCorner = nullptr;

        for( auto& l : m_mdragLines )
        {
            if( l.cornerDistance < minCornerDist )
            {
                minCornerDist = l.cornerDistance;
                bestCorner = &l;
            }
            if( l.leaderSegDistance < minLeadSegDist )
            {
                minLeadSegDist = l.leaderSegDistance;
                bestSeg = &l;
            }
        }

        if( bestCorner && bestSeg )
        {
            if( minCornerDist < minLeadSegDist )
            {
                m_dragMode = DM_CORNER;
                bestCorner->isPrimaryLine = true;
            }
            else
            {
                m_dragMode = DM_SEGMENT;
                bestSeg->isPrimaryLine = true;
            }
        }
        else if ( bestCorner )
        {
            m_dragMode = DM_CORNER;
            bestCorner->isPrimaryLine = true;
        }
        else if ( bestSeg )
        {
            m_dragMode = DM_SEGMENT;
            bestSeg->isPrimaryLine = true;
        }
        else return false; // can it really happen?
    }

    if( m_dragMode == DM_CORNER )
    {
        for( auto& l : m_mdragLines )
        {
            // make sure the corner to drag is the last one
            if ( !l.cornerIsLast )
            {
                l.originalLine.Reverse();
                l.cornerIsLast = true;
            }
            // and if it's connected (non-trivial fanout), disregard it

            const JOINT* jt = m_world->FindJoint( l.originalLine.CLastPoint(), &l.originalLine );

            assert (jt != nullptr);

            if( !jt->IsTrivialEndpoint() )
            {
                m_dragMode = DM_SEGMENT; // fallback to segment mode if non-trivial endpoints found
            }
        }
    }

    for( auto& l : m_mdragLines )
    {
        if( (anyStrictCornersFound || anyStrictMidSegsFound) && l.isStrict )
        {
            l.isPrimaryLine = true;
            break;
        }
    }

    m_origDraggedItems = aPrimitives;

    if( Settings().Mode() == RM_Shove )
    {
        m_preShoveNode = m_world->Branch();

        for( auto& l : m_mdragLines )
        {
            m_preShoveNode->Remove( l.originalLine );
        }

        m_shove.reset( new SHOVE( m_preShoveNode, Router() ) );
        m_shove->SetLogger( Logger() );
        m_shove->SetDebugDecorator( Dbg() );
        m_shove->SetDefaultShovePolicy( SHOVE::SHP_SHOVE | SHOVE::SHP_DONT_LOCK_ENDPOINTS );
    }

    return true;
}


void MULTI_DRAGGER::SetMode( PNS::DRAG_MODE aMode )
{
}


PNS::DRAG_MODE MULTI_DRAGGER::Mode() const
{
    return DM_CORNER;
}

bool clipToOtherLine( NODE* aNode, const LINE& aRef, LINE& aClipped )
{
    std::set<OBSTACLE> obstacles;
    COLLISION_SEARCH_CONTEXT ctx( obstacles );

    constexpr int clipLengthThreshold = 100;

    //DEBUG_DECORATOR* dbg = ROUTER::GetInstance()->GetInterface()->GetDebugDecorator();

    LINE l( aClipped );
    SHAPE_LINE_CHAIN tightest;

    bool didClip = false;
    int curL = l.CLine().Length();
    int step = curL / 2 - 1;

    while( step > clipLengthThreshold )
    {
        SHAPE_LINE_CHAIN sl_tmp( aClipped.CLine() );
        VECTOR2I pclip = sl_tmp.PointAlong( curL );
        int idx = sl_tmp.Split( pclip );
        sl_tmp = sl_tmp.Slice(0, idx);

        l.SetShape( sl_tmp );

        //PNS_DBG( dbg, 3int, pclip, WHITE, 500000, wxT(""));

        if( l.Collide( &aRef, aNode, l.Layer(), &ctx ) )
        {
            didClip = true;
            curL -= step;
            step /= 2;
        }
        else
        {
            tightest = std::move( sl_tmp );

            if( didClip )
            {
                curL += step;
                step /= 2;
            }
            else
            {
                break;
            }
        }
    }

    aClipped.SetShape( tightest );

    return didClip;
}




const std::vector<NET_HANDLE> MULTI_DRAGGER::CurrentNets() const
{
    std::set<NET_HANDLE> uniqueNets;
    for( auto &l : m_mdragLines )
    {
        NET_HANDLE net = l.draggedLine.Net();
        if( net )
            uniqueNets.insert( net );
    }

    return std::vector<NET_HANDLE>( uniqueNets.begin(), uniqueNets.end() );
}

// this is what ultimately gets called when the user clicks/releases the mouse button
// during drag.
bool MULTI_DRAGGER::FixRoute( bool aForceCommit )
{
    NODE* node = CurrentNode();

    if( node )
    {
	// last drag status is OK?
        if( !m_dragStatus && !Settings().AllowDRCViolations() )
            return false;

	// commit the current world state
        Router()->CommitRouting( node );
        return true;
    }

    return false;
}

bool MULTI_DRAGGER::tryWalkaround( NODE* aNode, LINE& aOrig, LINE& aWalk )
{
    WALKAROUND walkaround( aNode, Router() );
    bool       ok = false;
    walkaround.SetSolidsOnly( false );
    walkaround.SetDebugDecorator( Dbg() );
    walkaround.SetLogger( Logger() );
    walkaround.SetIterationLimit( Settings().WalkaroundIterationLimit() );
    walkaround.SetLengthLimit( true, 3.0 );
    walkaround.SetAllowedPolicies( { WALKAROUND::WP_SHORTEST } );

    aWalk = aOrig;

    WALKAROUND::RESULT wr = walkaround.Route( aWalk );

    if( wr.status[ WALKAROUND::WP_SHORTEST ] == WALKAROUND::ST_DONE )
    {
        aWalk = wr.lines[ WALKAROUND::WP_SHORTEST ];
        return true;
    }

    return false;
}

int MULTI_DRAGGER::findNewLeaderSegment( const MULTI_DRAGGER::MDRAG_LINE& aLine ) const
{
    const SEG origLeader = aLine.preDragLine.CSegment( aLine.leaderSegIndex );
    const DIRECTION_45 origLeaderDir( origLeader );

    for ( int i = 0; i < aLine.draggedLine.SegmentCount(); i++ )
    {
        const SEG& curSeg = aLine.draggedLine.CSegment(i);
        const DIRECTION_45 curDir( curSeg );

        auto ip = curSeg.IntersectLines( m_guide );
        PNS_DBG(Dbg(), Message, wxString::Format("s %d ip=%d c=%s o=%s", i, ip?1:0, curDir.Format(), origLeaderDir.Format() ));
        if( ip && curSeg.Contains( *ip ) )
        {
            if( curDir == origLeaderDir || curDir == origLeaderDir.Opposite() )
                return i;
        }
    }

    return -1;
}

void MULTI_DRAGGER::restoreLeaderSegments( std::vector<MDRAG_LINE>& aCompletedLines )
{
     m_leaderSegments.clear();

    for( auto& l : aCompletedLines )
    {
        if( l.dragOK )
        {
            if( m_dragMode == DM_CORNER )
            {
                if( l.draggedLine.LinkCount() > 0 )
                {
                    m_leaderSegments.push_back(
                        static_cast<PNS::ITEM*>( l.draggedLine.GetLink( -1 ) ) );
                }
            }
            else
            {
                int newLeaderIdx = findNewLeaderSegment( l );
                if( newLeaderIdx >= 0 && newLeaderIdx < l.draggedLine.LinkCount() )
                {
                    m_leaderSegments.push_back(
                        static_cast<PNS::ITEM*>( l.draggedLine.GetLink( newLeaderIdx ) ) );
                }
            }
        }
    }
}

bool MULTI_DRAGGER::multidragWalkaround( std::vector<MDRAG_LINE>& aCompletedLines )
{
    // fixme: rewrite using shared_ptr...
    if( m_lastNode )
    {
        delete m_lastNode;
        m_lastNode = nullptr;
    }

    auto compareDragStartDist = []( const MDRAG_LINE& a, const MDRAG_LINE& b ) -> int
    {
        return a.dragDist < b.dragDist;
    };

    std::sort( aCompletedLines.begin(), aCompletedLines.end(), compareDragStartDist );


    NODE* preWalkNode = m_world->Branch();

    for( auto& l : aCompletedLines )
    {
        PNS_DBG( Dbg(), AddItem, &l.originalLine, BLUE, 100000, wxString::Format("prewalk-remove lc=%d", l.originalLine.LinkCount() ) );
        preWalkNode->Remove( l.originalLine );
    }

    bool fail = false;

    NODE* tmpNodes[2];
    int totalLength[2];

    for( int attempt = 0; attempt < 2; attempt++ )
    {
        NODE *node = tmpNodes[attempt] = preWalkNode->Branch();
        totalLength[attempt] = 0;
        fail = false;

        for( int lidx = 0; lidx < aCompletedLines.size(); lidx++ )
        {
            MDRAG_LINE& l = aCompletedLines[attempt ? aCompletedLines.size() - 1 - lidx : lidx];

            LINE walk( l.draggedLine );
            auto result = tryWalkaround( node, l.draggedLine, walk );

            PNS_DBG( Dbg(), AddItem, &l.draggedLine, YELLOW, 100000, wxString::Format("dragged lidx=%d attempt=%d dd=%d isPrimary=%d", lidx, attempt, l.dragDist, l.isPrimaryLine?1:0) );
            PNS_DBG( Dbg(), AddItem, &walk, BLUE, 100000, wxString::Format("walk    lidx=%d attempt=%d", lidx, attempt) );


            if( result )
            {
                node->Add( walk );
                totalLength[attempt] += walk.CLine().Length() - l.draggedLine.CLine().Length();
                l.draggedLine = std::move( walk );
            }
            else
            {
                delete node;
                tmpNodes[attempt] = nullptr;
                fail = true;
                break;
            }
        }
    }

    if( fail )
        return false;


    bool rv = false;

    if( tmpNodes[0] && tmpNodes[1] )
    {
        if ( totalLength[0] < totalLength[1] )
        {
            delete tmpNodes[1];
            m_lastNode = tmpNodes[0];
            rv = true;
        }
        else
        {
            delete tmpNodes[0];
            m_lastNode = tmpNodes[1];
            rv = true;
        }
    }
    else if ( tmpNodes[0] )
    {
        m_lastNode = tmpNodes[0];
        rv = true;
    }
    else if ( tmpNodes[1] )
    {
        m_lastNode = tmpNodes[1];
        rv = true;
    }

    restoreLeaderSegments( aCompletedLines );

    return rv;
}


bool MULTI_DRAGGER::multidragMarkObstacles( std::vector<MDRAG_LINE>& aCompletedLines )
{

// fixme: rewrite using shared_ptr...
    if( m_lastNode )
    {
        delete m_lastNode;
        m_lastNode = nullptr;
    }

    // m_lastNode contains the temporary (post-modification) state. Think of it as
    // of an efficient undo buffer. We don't change the PCB directly, but a branch of it
    // created below. We can then commit its state (applying the modifications to the host board
    // by calling ROUTING::CommitRouting(m_lastNode) or simply discard it.
    m_lastNode = m_world->Branch();


    int nclipped = 0;
    for( int l1 = 0; l1 < aCompletedLines.size(); l1++ )
    {
        for( int l2 = l1 + 1; l2 < aCompletedLines.size(); l2++ )
        {
            const auto& l1l = aCompletedLines[l1].draggedLine;
            auto l2l        = aCompletedLines[l2].draggedLine;

            if( clipToOtherLine( m_lastNode, l1l, l2l ) )
            {
                aCompletedLines[l2].draggedLine = l2l;
                nclipped++;
            }
        }
    }

    for ( auto&l : aCompletedLines )
    {
        m_lastNode->Remove( l.originalLine );
        m_lastNode->Add( l.draggedLine );
    }

    restoreLeaderSegments( aCompletedLines );

    return true;
}

bool MULTI_DRAGGER::multidragShove( std::vector<MDRAG_LINE>& aCompletedLines )
{
    if( m_lastNode )
    {
        delete m_lastNode;
        m_lastNode = nullptr;
    }

    if( !m_shove )
        return false;

    auto compareDragStartDist = []( const MDRAG_LINE& a, const MDRAG_LINE& b ) -> int
    {
        return a.dragDist < b.dragDist;
    };

    std::sort( aCompletedLines.begin(), aCompletedLines.end(), compareDragStartDist );

    auto iface = Router()->GetInterface();

    for( auto& l : m_mdragLines )
    {
        PNS_DBG( Dbg(), Message, wxString::Format ( wxT("net %-30s: isCorner %d isStrict %d c-Dist %-10d l-dist %-10d leadIndex %-2d CisLast %d dragDist %-10d"),
            iface->GetNetName( l.draggedLine.Net() ),
            (int) l.isCorner?1:0,
            (int) l.isStrict?1:0,
            (int) l.cornerDistance,
            (int) l.leaderSegDistance,
            (int) l.leaderSegIndex,
            (int) l.cornerIsLast?1:0,
            (int) l.dragDist ) );
    }


    m_shove->SetDefaultShovePolicy( SHOVE::SHP_SHOVE );
    m_shove->ClearHeads();

    for( auto& l : aCompletedLines )
    {
        PNS_DBG( Dbg(), AddItem, &l.draggedLine, GREEN, 0, "dragged-line" );
        m_shove->AddHeads( l.draggedLine, SHOVE::SHP_SHOVE | SHOVE::SHP_DONT_OPTIMIZE );
    }

    auto status = m_shove->Run();

    m_lastNode = m_shove->CurrentNode()->Branch();

    if( status == SHOVE::SH_OK )
    {
        for( int i = 0; i < aCompletedLines.size(); i++ )
        {
            MDRAG_LINE&l = aCompletedLines[i];
            if( m_shove->HeadsModified( i ) )
                l.draggedLine = m_shove->GetModifiedHead( i );

            // this should not be linked (assert in rt-test)
            l.draggedLine.ClearLinks();

            m_lastNode->Add( l.draggedLine );
        }
    }
    else
    {
        return false;
    }

    restoreLeaderSegments( aCompletedLines );

    return true;
}

// this is called every time the user moves the mouse while dragging a set of multiple tracks
bool MULTI_DRAGGER::Drag( const VECTOR2I& aP )
{
    std::optional<LINE> primaryPreDrag, primaryDragged;



    SEG lastPreDrag;
    DIRECTION_45 primaryDir;
    VECTOR2I perp;

    DIRECTION_45 primaryLastSegDir;
    std::vector<MDRAG_LINE> completed;

    auto tryPosture = [&] ( int aVariant ) -> bool
    {
        MDRAG_LINE* primaryLine = nullptr;

        for( auto &l : m_mdragLines )
        {
            l.dragOK = false;
            l.preDragLine = l.originalLine;
                //PNS_DBG( Dbg(), AddItem, &l.originalLine, GREEN, 300000, "par" );
            if( l.isPrimaryLine )
            {

                //PNS_DBG( Dbg(), AddItem, &l.originalLine, BLUE, 300000, wxT("mdrag-prim"));

            // create a copy of the primary line (pre-drag and post-drag).
            // the pre-drag version is necessary for NODE::Remove() to be able to
                // find out the segments before modification by the multidrag algorithm
                primaryDragged = l.originalLine;
                primaryDragged->ClearLinks();
                primaryPreDrag = l.originalLine;
                primaryLine = &l;

            }
        }

        if( aVariant == 1 && (primaryPreDrag->PointCount() > 2) )
        {
                primaryPreDrag->Line().Remove( -1 );
                primaryDragged->Line().Remove( -1 );

                for( auto&l : m_mdragLines )
                {
                    l.preDragLine.Line().Remove(-1);
                }
        }

        completed.clear();

        int snapThreshold = Settings().SmoothDraggedSegments() ? primaryDragged->Width() / 4 : 0;

        if( m_dragMode == DM_CORNER )
        {
            // first, drag only the primary line
            PNS_DBG( Dbg(), AddPoint, primaryDragged->CLastPoint(), YELLOW, 600000, wxT("mdrag-sec"));

	        lastPreDrag =  primaryPreDrag->CSegment( -1 );
            primaryDir = DIRECTION_45( lastPreDrag );

            primaryDragged->SetSnapThreshhold( snapThreshold );
            primaryDragged->DragCorner( aP, primaryDragged->PointCount() - 1, false );

         
            if( primaryDragged->SegmentCount() > 0 )
            {
                SEG lastPrimDrag = primaryDragged->CSegment( -1 );

                if ( aVariant == 2 )
                    lastPrimDrag = lastPreDrag;

                auto lastSeg = primaryDragged->CSegment( -1 );
                if( DIRECTION_45( lastSeg ) != primaryDir )
                {
                    if( lastSeg.Length() < primaryDragged->Width() )
                    {
                        lastPrimDrag = lastPreDrag;
                    }
                }
            
                perp = (lastPrimDrag.B - lastPrimDrag.A).Perpendicular();
                primaryLastSegDir = DIRECTION_45( lastPrimDrag );

                
                PNS_DBG( Dbg(), AddItem, &(*primaryDragged), LIGHTGRAY, 100000, "prim" );
                PNS_DBG( Dbg(), AddShape, SEG(lastPrimDrag.B, lastPrimDrag.B + perp), LIGHTGRAY, 100000, wxString::Format("prim-perp-seg") );
            } else {
                return false;
            }



//            PNS_DBG( Dbg(), AddShape, &ll, LIGHTBLUE, 200000, "par" );

        }
        else
        {

            SHAPE_LINE_CHAIN ll2( { lastPreDrag.A, lastPreDrag.B } );
            PNS_DBG( Dbg(), AddShape, &ll2, LIGHTYELLOW, 300000, "par" );
            lastPreDrag =  primaryDragged->CSegment( primaryLine->leaderSegIndex );
            primaryDragged->SetSnapThreshhold( snapThreshold );
            primaryDragged->DragSegment( aP, primaryLine->leaderSegIndex );
            perp = (primaryLine->midSeg.B - primaryLine->midSeg.A).Perpendicular();
            m_guide = SEG( aP, aP + perp );
        }


        m_leaderSegments = m_origDraggedItems.CItems();
        m_draggedItems.Clear();

        // now drag all other lines
        for( auto& l : m_mdragLines )
        {
            //PNS_DBG( Dbg(), AddPoint, l.originalLine.CPoint( l.cornerIndex ), WHITE, 1000000, wxT("l-end"));
            if( l.isDraggable )
            {
                l.dragOK = false;
                //PNS_DBG( Dbg(), AddItem, &l.originalLine, GREEN, 100000, wxT("mdrag-sec"));

                // reject nulls
                if( l.preDragLine.SegmentCount() >= 1 )
                {

                    //PNS_DBG( Dbg(), AddPoint, l.preDragLine.CPoint( l.cornerIndex ), YELLOW, 600000, wxT("mdrag-sec"));

                    // check the direction of the last segment of the line against the direction of
                    // the last segment of the primary line (both before dragging) and perform drag
                    // only when the directions are the same. The algorithm here is quite trival and
                    // otherwise would produce really awkward results. There's of course a TON of
                    // room for improvement here :-)

                    if( m_dragMode == DM_CORNER )
                    {
                        DIRECTION_45 parallelDir( l.preDragLine.CSegment( -1 ) );

                        auto leadAngle = primaryDir.Angle( parallelDir );

                        if( leadAngle == DIRECTION_45::ANG_OBTUSE
                            || leadAngle == DIRECTION_45::ANG_STRAIGHT )
                        {
                            // compute the distance between the primary line and the last point of
                            // the currently processed line
                            int dist = lastPreDrag.LineDistance( l.preDragLine.CLastPoint(), true );

                            // now project it on the perpendicular line we computed before
                            auto projected = aP + perp.Resize( dist );


                            LINE parallelDragged( l.preDragLine );

                            PNS_DBG( Dbg(), AddPoint, projected, LIGHTGRAY, 100000, "dragged-c" );
                            PNS_DBG( Dbg(), AddPoint, parallelDragged.CLastPoint(), LIGHTGRAY, 100000, wxString::Format("orig-c cil %d", l.cornerIsLast?1:0) );

                            parallelDragged.ClearLinks();
                            //m_lastNode->Remove( parallelDragged );
                            // drag the non-primary line's end trying to place it at the projected point
                            parallelDragged.DragCorner( projected, parallelDragged.PointCount() - 1,
                                                        false, primaryLastSegDir );

                            PNS_DBG( Dbg(), AddPoint, projected, LIGHTYELLOW, 600000,
                                   wxT( "l-end" ) );

                            l.dragOK = true;

                            if( !l.isPrimaryLine )
                            {
                                l.draggedLine = parallelDragged;
                                completed.push_back( l );
                                m_draggedItems.Add( parallelDragged );
                            }
                        }
                    }
                    else if ( m_dragMode == DM_SEGMENT )
                    {
                        SEG sdrag = l.midSeg;
                        DIRECTION_45 refDir( lastPreDrag );
                        DIRECTION_45 curDir( sdrag );
                        auto ang = refDir.Angle( curDir );

                        if( ang & ( DIRECTION_45::ANG_HALF_FULL | DIRECTION_45::ANG_STRAIGHT ) )
                        {
                            int dist = lastPreDrag.LineDistance(
                                    l.preDragLine.CPoint( l.leaderSegIndex ), true );
                            auto projected = aP + perp.Resize( dist );

                            SEG      sperp( aP, aP + perp.Resize( 10000000 ) );
                            VECTOR2I startProj = sperp.LineProject( m_dragStartPoint );

                            SHAPE_LINE_CHAIN ll( { sperp.A, sperp.B } );


                            PNS_DBG( Dbg(), AddShape, &ll, LIGHTBLUE, 100000, "par" );
                            SHAPE_LINE_CHAIN ll2( { sdrag.A, sdrag.B } );
                            PNS_DBG( Dbg(), AddShape, &ll2, LIGHTBLUE, 100000, "sdrag" );
                            VECTOR2I v = projected - startProj;
                            l.dragDist = v.EuclideanNorm() * sign( v.Dot( perp ) );
                            l.dragOK = true;

                            if( !l.isPrimaryLine )
                            {
                                l.draggedLine = l.preDragLine;
                                l.draggedLine.ClearLinks();
                                l.draggedLine.SetSnapThreshhold( snapThreshold );
                                l.draggedLine.DragSegment( projected, l.leaderSegIndex, false );
                                completed.push_back( l );
                                PNS_DBG( Dbg(), AddItem, &l.draggedLine, LIGHTBLUE, 100000,
                                         "dragged" );
                            }


                            PNS_DBG( Dbg(), AddPoint, startProj, LIGHTBLUE, 400000,
                                     wxT( "startProj" ) );
                            PNS_DBG( Dbg(), AddPoint, projected, LIGHTRED, 400000,
                                     wxString::Format( "pro dd=%d", l.dragDist ) );
                        }
                    }
                }
            }

            if (l.isPrimaryLine)
            {
                l.draggedLine = *primaryDragged;
                l.dragOK = true;
                completed.push_back( l );
            }
        }

        if( m_dragMode == DM_SEGMENT )
            return true;
        else
        {
            for ( const auto &l: completed )
            {
                if( !l.dragOK && aVariant < 2 )
                    return false;

                if( l.isPrimaryLine )
                    continue;

                DIRECTION_45 lastDir ( l.draggedLine.CSegment(-1) );

                if( lastDir != primaryLastSegDir )
                    return false;
            }
        }

        return true;
    };

    bool res = false;

    for( int variant = 0; variant < 3; variant++ )
    {
        res = tryPosture( 0 );
        if( res )
            break;
    }

    switch( Settings().Mode() )
    {
        case RM_Walkaround:
            m_dragStatus = multidragWalkaround ( completed );
            break;

        case RM_Shove:
            m_dragStatus = multidragShove ( completed );
            break;

        case RM_MarkObstacles:
            m_dragStatus = multidragMarkObstacles( completed );
            break;



        default:
        break;
    }

    return m_dragStatus;
}


NODE* MULTI_DRAGGER::CurrentNode() const
{
    return m_lastNode ? m_lastNode : m_world;
}


const ITEM_SET MULTI_DRAGGER::Traces()
{
    return m_draggedItems;
}


int MULTI_DRAGGER::CurrentLayer() const
{
    // fixme: should we care?
    return 0;
}


} // namespace PNS

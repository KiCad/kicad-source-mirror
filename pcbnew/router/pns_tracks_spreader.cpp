/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <deque>

#include "pns_router.h"
#include "pns_debug_decorator.h"
#include "pns_utils.h"
#include "pns_tracks_spreader.h"
#include "pns_walkaround.h"
#include "pns_optimizer.h"
#include "pns_utils.h"

#undef SPREAD_TRACKS_EXTRA_VERBOSE

namespace PNS
{

TRACKS_SPREADER::TRACKS_SPREADER( ROUTER* aRouter ) :
        PLACEMENT_ALGO( aRouter )
{
    m_startItem = nullptr;
    m_currentNode = nullptr;
}


TRACKS_SPREADER::~TRACKS_SPREADER()
{
}


bool TRACKS_SPREADER::Start( const VECTOR2I& aP, ITEM* aStartItem )
{
    if( m_auxItems.Empty() )
        return false;

    if( !aStartItem->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
        return false;

    m_startItem = static_cast<LINKED_ITEM*>( aStartItem );
    m_startAnchor = aP;
    m_currentNode = nullptr;

    if( !prepareSpreadableLines() )
        return false;

    return true;
}


bool approxSegOverlaps( const SEG& aA, const SEG& aB, int aThreshold, int& aOverlapDist )
{
    if( aA.A == aA.B ) // single point corner case
    {
        if( aB.A == aA.A || aB.B == aA.A )
            return true;

        if( aB.Contains( aA.A, aThreshold ) )
        {
            aOverlapDist = aB.Distance( aA.A );
            return true;
        }
    }

    if( !aB.ApproxCollinear( aA, aThreshold ) )
        return false;

    if( aB.Contains( aA.A, aThreshold ) )
    {
        aOverlapDist = aB.Distance( aA.A );
        return true;
    }

    if( aB.Contains( aA.B, aThreshold ) )
    {
        aOverlapDist = aB.Distance( aA.B );
        return true;
    }

    if( aA.Contains( aB.A, aThreshold ) )
    {
        aOverlapDist = aA.Distance( aB.A );
        return true;
    }

    if( aA.Contains( aB.B, aThreshold ) )
    {
        aOverlapDist = aA.Distance( aB.B );
        return true;
    }

    return 0;
}


bool TRACKS_SPREADER::Move( const VECTOR2I& aP, ITEM* aEndItem )
{
    auto rnearest = m_rootLine.CLine().NearestPoint( aP );

    PNS_DBG( Dbg(), AddPoint, m_startAnchor, GREEN, 300000, wxT( "move=start" ) );
    PNS_DBG( Dbg(), AddPoint, rnearest, GREEN, 300000, wxT( "move=end" ) );

    if( m_currentNode )
    {
        delete m_currentNode;
        m_currentNode = m_preSpreadNode;
    }

    NODE* spreadNode = m_preSpreadNode->Branch();

    auto clippedHead = breakAndReassemble( spreadNode, &m_rootLine, m_startAnchor, rnearest );
    m_spreadableItems.clear();

    if( clippedHead )
    {
#ifdef SPREAD_TRACKS_EXTRA_VERBOSE
        PNS_DBG( Dbg(), AddItem, &( *clippedHead ), GREEN, 10000,
                 wxString::Format( "clip-head %d", (int) clippedHead->LinkCount() ) );
#endif

        for( auto& lnk : clippedHead->Links() )
        {
            m_spreadableItems.insert( lnk );
#ifdef SPREAD_TRACKS_EXTRA_VERBOSE
            PNS_DBG( Dbg(), AddItem, static_cast<const ITEM*>( lnk ), GREEN, 10000,
                     wxString::Format( "cliplinkd %d", 0 ) );
#endif
        }
    }

    for( SPREADABLE_LINE& l : m_lines )
    {
        LINE lrem( l.spreadableLine );
#ifdef SPREAD_TRACKS_EXTRA_VERBOSE
        PNS_DBG( Dbg(), AddItem, &l.spreadableLine, RED, 10000,
                 wxString::Format( "rem-line l %d", (int) l.spreadableLine.LinkCount() ) );
#endif
        spreadNode->Remove( lrem );
    }

    std::unordered_set<ITEM*> processedItems;

    auto spreadClearanceFunc = [&]( const ITEM* aA, const ITEM* aB, CLEARANCE_ASPECT aAspect ) -> int
    {
        auto lnkA = static_cast<const LINKED_ITEM*>( aA );
        auto lnkB = static_cast<const LINKED_ITEM*>( aB );

        if( !alg::contains( processedItems, aA ) && !alg::contains( processedItems, aB ) )
            return 0;

        if( aA->Net() == aB->Net() )
            return -1;

        bool Asp = m_spreadableItems.find( lnkA ) != m_spreadableItems.end();
        bool Bsp = m_spreadableItems.find( lnkB ) != m_spreadableItems.end();

        return Asp || Bsp ? Router()->Settings().GetSpreadTracksDistance() : -1;
    };

    for( LINKED_ITEM* lnk : clippedHead->Links() )
        processedItems.insert( lnk );

    for( SPREADABLE_LINE& entry : m_lines )
    {
        WALKAROUND walkaround( spreadNode, Router() );
        walkaround.SetAllowedPolicies( { WALKAROUND::WP_CW, WALKAROUND::WP_CCW }, true );
        walkaround.SetClearanceOverride( spreadClearanceFunc );
        walkaround.SetDebugDecorator( Dbg() );
        walkaround.SetUseClustering( false );
        walkaround.SetLengthLimit( false, 0.0 );

        PNS_DBG( Dbg(), BeginGroup, wxT( "grp-walk-details" ), 0 );
        auto ret = walkaround.Route( entry.spreadableLine );
        PNS_DBGN( Dbg(), EndGroup );

        bool success = false;

        for( auto policy : { WALKAROUND::WP_CW, WALKAROUND::WP_CCW } )
        {
            if( ret.status[policy] == WALKAROUND::ST_DONE )
            {
                spreadNode->Add( ret.lines[policy] );
                entry.forwardLine = ret.lines[policy];


                for( auto lnk : ret.lines[policy].Links() )
                    processedItems.insert( lnk );
#ifdef SPREAD_TRACKS_EXTRA_VERBOSE
                PNS_DBG( Dbg(), AddItem, &ret.lines[policy], GREEN, 10000,
                         wxString::Format( "walk-line l %d pitems %d", ret.lines[policy].LinkCount(),
                                           (int) processedItems.size() ) );
#endif
                for( auto lnk : ret.lines[policy].Links() )
                {
                    bool spreadable = true;

#ifdef SPREAD_TRACKS_EXTRA_VERBOSE
                    PNS_DBG( Dbg(), BeginGroup, "spreadable-propagate", 0 );
                    PNS_DBG( Dbg(), AddItem, lnk, WHITE, 100000, wxString::Format( "lnk" ) );
#endif
                    for( int i = 0; i < entry.spreadableLine.SegmentCount(); i++ )
                    {
                        const SEG& s = entry.spreadableLine.CSegment( i );

                        SEGMENT* ps = dynamic_cast<SEGMENT*>( lnk );
                        if( ps )
                        {
                            int overlapDist = 0;
                            if( ps->Seg() == s || s.ApproxOverlaps( ps->Seg() ) )
                            {
                                SEG c1, c2;
                                if( !commonParallelProjection( s, ps->Seg(), c1, c2 ) )
                                    continue;

                                overlapDist = c1.Length();

#ifdef SPREAD_TRACKS_EXTRA_VERBOSE
                                PNS_DBG( Dbg(), AddShape, s, RED, 100000,
                                         wxString::Format( "ns-dist %d", overlapDist ) );
#endif

                                if( overlapDist > 100 ) // fixme: make configurable
                                    spreadable = false;

                                break;
                            }
                        }
                    }
#ifdef SPREAD_TRACKS_EXTRA_VERBOSE
                    PNS_DBGN( Dbg(), EndGroup );
#endif

                    if( spreadable )
                    {
                        m_spreadableItems.insert( lnk );
#ifdef SPREAD_TRACKS_EXTRA_VERBOSE
                        PNS_DBG( Dbg(), AddItem, lnk, WHITE, 100000, wxString::Format( "spreadable" ) );
#endif
                    }
                }
                success = true;
                break;
            }
        }
        if( !success )
        {
            PNS_DBG( Dbg(), AddItem, &entry.originLine, WHITE, 100000, wxString::Format( "failing-line" ) );
            break;
        }
    }

    for( auto& ent : m_lines )
    {
        spreadNode->Remove( ent.forwardLine );
        ent.forwardLine.Line().Simplify2();
    }

    OPT_BOX2I totalAffectedArea;

    for( SPREADABLE_LINE& entry : m_lines | std::views::reverse )
    {
        WALKAROUND walkaround( spreadNode, Router() );
        walkaround.SetAllowedPolicies( { WALKAROUND::WP_CW, WALKAROUND::WP_CCW }, true );
        walkaround.SetDebugDecorator( Dbg() );
        walkaround.SetUseClustering( false );
        walkaround.SetLengthLimit( false, 0.0 );

        PNS_DBG( Dbg(), BeginGroup, wxT( "grp-back-details" ), 0 );
        auto ret = walkaround.Route( entry.forwardLine );
        PNS_DBGN( Dbg(), EndGroup );

        bool success = false;
        for( auto policy : { WALKAROUND::WP_CW, WALKAROUND::WP_CCW } )
        {
            if( ret.status[policy] == WALKAROUND::ST_DONE )
            {
                entry.backwardLine = ret.lines[policy];
                OPT_BOX2I affectedArea = entry.backwardLine.ChangedArea( &entry.spreadableLine );
                if( affectedArea.has_value() )
                {
                    if( !totalAffectedArea )
                        totalAffectedArea = affectedArea;
                    else
                        totalAffectedArea->Merge( affectedArea.value() );
                }
                entry.backwardLine.Line().Simplify2();
                spreadNode->Add( entry.backwardLine );

                success = true;
                break;
            }
        }
        if( !success )
        {
            PNS_DBG( Dbg(), AddItem, &entry.originLine, WHITE, 100000, wxString::Format( "failing-line-back" ) );
            break;
        }
    }

    // stage 3: optimize

    if( totalAffectedArea )
    {
        int delta = 4 * m_rootLine.Width(); // 4 is a perfectly fair choice for a fudge factor, isn't it?
        totalAffectedArea->Inflate( delta );
    }


    for( SPREADABLE_LINE& entry : m_lines | std::views::reverse )
    {
        OPTIMIZER        optimizer( spreadNode );
        std::vector<SEG> doNotOptimizeSegs = findNoOptSegments( clippedHead.value(), entry );

        int effort = OPTIMIZER::MERGE_SEGMENTS;

        effort |= OPTIMIZER::MERGE_COLINEAR;
        effort |= OPTIMIZER::REQUIRE_OBTUSE_ANGLES;

        optimizer.SetEffortLevel( effort );
        optimizer.SetPreserveSegments( doNotOptimizeSegs );

        if( totalAffectedArea )
            optimizer.SetRestrictArea( *totalAffectedArea );

        spreadNode->Remove( entry.backwardLine );
        optimizer.Optimize( &entry.backwardLine, &entry.optimizedLine );
        spreadNode->Add( entry.optimizedLine );
    }


    m_currentNode = spreadNode;

    return true;
}


bool TRACKS_SPREADER::FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish )
{
    if( !m_currentNode )
        return false;

    return true;
}


bool TRACKS_SPREADER::CommitPlacement()
{
    if( !m_currentNode )
        return false;

    Router()->CommitRouting( m_currentNode );


    return true;
}


bool TRACKS_SPREADER::AbortPlacement()
{
    Router()->GetWorld()->KillChildren();
    m_currentNode = nullptr;
    return true;
}


bool TRACKS_SPREADER::HasPlacedAnything() const
{
    return true;
}


NODE* TRACKS_SPREADER::CurrentNode( bool aLoopsRemoved ) const
{
    return m_currentNode ? m_currentNode : Router()->GetWorld();
}


const ITEM_SET TRACKS_SPREADER::Traces()
{
    return ITEM_SET();
}


const VECTOR2I& TRACKS_SPREADER::CurrentStart() const
{
    return m_startAnchor;
}


const VECTOR2I& TRACKS_SPREADER::CurrentEnd() const
{
    return m_endAnchor;
}


const std::vector<NET_HANDLE> TRACKS_SPREADER::CurrentNets() const
{
    return std::vector<NET_HANDLE>();
}


int TRACKS_SPREADER::CurrentLayer() const
{
    return -1;
}


bool TRACKS_SPREADER::prepareSpreadableLines()
{
    m_lines.clear();

    NODE* world = Router()->GetWorld();

    std::deque<LINKED_ITEM*> pending;

    for( ITEM* item : m_auxItems.CItems() )
    {
        if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
            pending.push_back( static_cast<LINKED_ITEM*>( item ) );
    }


    m_rootLine = world->AssembleLine( m_startItem );

    // prepare a list of unique LINEs composed of the segments/arcs selected by the users
    while( !pending.empty() )
    {
        LINKED_ITEM* item = pending.front();
        pending.pop_front();

        bool alreadyAssembled =
                std::any_of( m_lines.begin(), m_lines.end(),
                             [item, this]( const SPREADABLE_LINE& aLine )
                             {
                                 return aLine.originLine.ContainsLink( item ) || m_rootLine.ContainsLink( item );
                             } );

        if( alreadyAssembled )
            continue;

        SPREADABLE_LINE line;
        line.originLine = world->AssembleLine( item );
        m_lines.push_back( line );
    }

    int margin = m_rootLine.Width();

    for( const SPREADABLE_LINE& entry : m_lines )
        margin += entry.originLine.Width();

    margin += 0; //static_cast<int>( m_originLines.size() ) * Settings().GetSpreadTracksGap();


    for( SPREADABLE_LINE& entry : m_lines )
    {
        const SHAPE_LINE_CHAIN& shape = entry.originLine.CLine();

        int plNearest = -1;
        int plFarthest = -1;

        for( ITEM* item : m_auxItems.CItems() )
        {
            if( !item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
                continue;

            LINKED_ITEM* litem = static_cast<LINKED_ITEM*>( item );

            if( !entry.originLine.ContainsLink( litem ) )
                continue;

            for( int i = 0; i < litem->AnchorCount(); i++ )
            {
                auto nearest = shape.NearestPoint( litem->Anchor( i ), false );
                int  pl = shape.PathLength( nearest );

                if( pl < 0 )
                    continue;

                if( plNearest < 0 || pl < plNearest )
                    plNearest = pl;

                if( pl > plFarthest )
                    plFarthest = pl;
            }
        }


        //PNS_DBG( Dbg(), AddItem, &entry.originLine, WHITE, 100000,
        //      wxString::Format( wxT( "pl-nearest %d pl-farthest %d orig-sc %d orig-lc %d" ), (int) plNearest, (int) plFarthest, entry.originLine.SegmentCount(), entry.originLine.LinkCount() ) );

        if( plFarthest < 0 )
            continue;

        int lineLength = static_cast<int>( shape.Length() );

        VECTOR2I pNearest = shape.PointAlong( std::max( 0, plNearest - margin ) );
        VECTOR2I pFarthest = shape.PointAlong( std::min( lineLength, plFarthest + margin ) );

        SHAPE_LINE_CHAIN pre, mid, post;
        shape.Split( pNearest, pFarthest, pre, mid, post );

        entry.spreadableLine = LINE( entry.originLine, mid );
        entry.p_start = pNearest;
        entry.p_end = pFarthest;
    }

    PNS_DBG( Dbg(), BeginGroup, "spreadable-lines", 0 );

    for( const SPREADABLE_LINE& entry : m_lines )
        PNS_DBG( Dbg(), AddItem, &entry.originLine, GREEN, 10000, wxT( "origin-line" ) );

    PNS_DBGN( Dbg(), EndGroup );

    PNS_DBG( Dbg(), BeginGroup, "spreadable-spans", 0 );

    determineWalkOrder();

    m_preSpreadNode = world->Branch();

    for( size_t i = 0; i < m_lines.size(); i++ )
    {
        SPREADABLE_LINE& entry = m_lines[i];

        PNS_DBG( Dbg(), AddPoint, entry.p_start, CYAN, 200000, wxString::Format( wxT( "span-nearest-%d" ), (int) i ) );
        PNS_DBG( Dbg(), AddPoint, entry.p_end, LIGHTBLUE, 200000,
                 wxString::Format( wxT( "span-farthest-%d" ), (int) i ) );


        auto adjusted = breakAndReassemble( m_preSpreadNode, &entry.originLine, entry.p_start, entry.p_end );

        PNS_DBG( Dbg(), AddItem, &entry.originLine, YELLOW, 20000,
                 wxString::Format( wxT( "spreadable-line-%d sc %d lc %d adj %d" ), (int) i, 0, 0,
                                   adjusted.has_value() ? 1 : 0 ) );


        if( adjusted )
        {
            PNS_DBG( Dbg(), AddItem, &( *adjusted ), YELLOW, 20000, wxString::Format( wxT( "adj-line-%d" ), (int) i ) );

            entry.spreadableLine = *adjusted;
        }
    }


    PNS_DBGN( Dbg(), EndGroup );

    PNS_DBG( Dbg(), Message, wxString::Format( wxT( "total-spreadable %d" ), (int) m_spreadableItems.size() ) );


    return true;
}


void TRACKS_SPREADER::determineWalkOrder()
{
    std::sort(
            m_lines.begin(), m_lines.end(),
            [this]( const SPREADABLE_LINE& aA, const SPREADABLE_LINE& aB )
            {
                SEG::ecoord distA =
                        ( aA.originLine.CLine().NearestPoint( m_startAnchor ) - m_startAnchor ).SquaredEuclideanNorm();
                SEG::ecoord distB =
                        ( aB.originLine.CLine().NearestPoint( m_startAnchor ) - m_startAnchor ).SquaredEuclideanNorm();

                return distA < distB;
            } );
}


std::optional<LINE> TRACKS_SPREADER::breakAndReassemble( NODE* aNode, LINE* aOrigLine, const VECTOR2I& aP1,
                                                         const VECTOR2I& aP2 )
{
    LINE current( *aOrigLine );
    aNode->Remove( aOrigLine );
    SHAPE_LINE_CHAIN newShape( aOrigLine->CLine() );
    newShape.Split( aP1 );
    newShape.Split( aP2 );
    LINE newLine( *aOrigLine, newShape );
    aNode->Add( newLine );

    auto jtA = aNode->FindJoint( aP1, &newLine );
    auto jtB = aNode->FindJoint( aP2, &newLine );

    if( !jtA || !jtB )
        return std::nullopt;

    std::vector<LINE> adjustedLines;

    aNode->FindLinesBetweenJoints( *jtA, *jtB, adjustedLines );

    if( adjustedLines.empty() )
        return std::nullopt;

    return adjustedLines[0];
}


void TRACKS_SPREADER::SetSpreadableItems( const ITEM_SET& aAuxItems )
{
    m_auxItems = aAuxItems;
}


const std::vector<SEG> TRACKS_SPREADER::findNoOptSegments( const LINE&                             aClippedHead,
                                                           const TRACKS_SPREADER::SPREADABLE_LINE& aEntry ) const
{
    std::vector<SEG> segs;

    const SHAPE_LINE_CHAIN& headShape = aClippedHead.CLine();
    for( int s = 0; s < headShape.SegmentCount(); s++ )
    {
        const SEG& headSeg = headShape.CSegment( s );

        for( int s2 = 0; s2 < aEntry.forwardLine.SegmentCount(); s2++ )
        {
            const SEG& spreadSeg = aEntry.forwardLine.CSegment( s2 );
            SEG        c1, c2;

            if( commonParallelProjection( headSeg, spreadSeg, c1, c2 ) )
            {
                int dist = headSeg.Distance( spreadSeg );
                int spreadDist = Settings().GetSpreadTracksDistance();

                //int remainder = dist % spreadDist;
                //const int threshold = 100;

                //if( remainder > spreadDist - threshold || remainder < threshold )
                {
                    PNS_DBG( Dbg(), AddShape, spreadSeg, LIGHTBLUE, 200000,
                             wxString::Format( "com-par dist %d", dist ) );
                    segs.push_back( spreadSeg );
                }
            }
        }
    }

    return segs;
}
} // namespace PNS

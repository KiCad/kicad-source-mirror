/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
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

#include "pns_walkaround.h"
#include "pns_shove.h"
#include "pns_router.h"
#include "pns_diff_pair_placer.h"
#include "pns_solid.h"
#include "pns_topology.h"
#include "pns_debug_decorator.h"
#include "pns_arc.h"
#include "pns_utils.h"

namespace PNS {

DIFF_PAIR_PLACER::DIFF_PAIR_PLACER( ROUTER* aRouter ) :
    PLACEMENT_ALGO( aRouter )
{
    m_state = RT_START;
    m_chainedPlacement = false;
    m_initialDiagonal = false;
    m_startDiagonal = false;
    m_fitOk = false;
    m_netP = nullptr;
    m_netN = nullptr;
    m_iteration = 0;
    m_world = nullptr;
    m_shove = nullptr;
    m_currentNode = nullptr;
    m_lastNode = nullptr;
    m_lastFixNode = nullptr;
    m_placingVia = false;
    m_viaDiameter = 0;
    m_viaDrill = 0;
    m_currentWidth = 0;
    m_currentLayer = 0;
    m_startsOnVia = false;
    m_orthoMode = false;
    m_snapOnTarget = false;
    m_currentEndItem = nullptr;
    m_currentTraceOk = false;
    m_idle = true;
    m_hasFixedAnything = false;
}

DIFF_PAIR_PLACER::~DIFF_PAIR_PLACER()
{}


void DIFF_PAIR_PLACER::setWorld( NODE* aWorld )
{
    m_world = aWorld;
}


const VIA DIFF_PAIR_PLACER::makeVia( const VECTOR2I& aP, NET_HANDLE aNet )
{
    const PNS_LAYER_RANGE layers( m_sizes.GetLayerTop(), m_sizes.GetLayerBottom() );

    VIA v( aP, layers, m_sizes.ViaDiameter(), m_sizes.ViaDrill(), aNet, m_sizes.ViaType() );

    return v;
}


void DIFF_PAIR_PLACER::SetOrthoMode ( bool aOrthoMode )
{
    m_orthoMode = aOrthoMode;

    if( !m_idle )
        Move( m_currentEnd, nullptr );
}


bool DIFF_PAIR_PLACER::ToggleVia( bool aEnabled )
{
    m_placingVia = aEnabled;

    if( !m_idle )
        Move( m_currentEnd, nullptr );

    return true;
}


bool DIFF_PAIR_PLACER::rhMarkObstacles( const VECTOR2I& aP )
{
    if( !routeHead( aP ) )
        return false;

    bool collP = static_cast<bool>( m_currentNode->CheckColliding( &m_currentTrace.PLine() ) );
    bool collN = static_cast<bool>( m_currentNode->CheckColliding( &m_currentTrace.NLine() ) );

    m_fitOk = !( collP || collN ) ;

    return m_fitOk;
}


bool DIFF_PAIR_PLACER::propagateDpHeadForces ( const VECTOR2I& aP, VECTOR2I& aNewP )
{
    VIA virtHead = makeVia( aP, nullptr );

    if( m_placingVia )
    {
        virtHead.SetDiameter( 0, viaGap() + 2 * virtHead.Diameter( 0 ) );
    }
    else
    {
        virtHead.SetLayer( m_currentLayer );
        virtHead.SetDiameter( 0, m_sizes.DiffPairGap() + 2 * m_sizes.DiffPairWidth() );
    }

    bool solidsOnly = true;

    if( Settings().Mode() == RM_MarkObstacles )
    {
        aNewP = aP;
        return true;
    }
    else if( Settings().Mode() == RM_Walkaround )
    {
        solidsOnly = false;
    }

    // fixme: I'm too lazy to do it well. Circular approximaton will do for the moment.

    // Note: this code is lifted from VIA::PushoutForce and then optimized for this use case and to
    // check proper clearances to the diff pair line.  It can be removed if some specialized
    // pushout for traces / diff pairs is implemented.  Just calling VIA::PushoutForce does not work
    // as the via may have different resolved clearance to items than the diff pair should.
    int                   maxIter  = 40;
    int                   iter     = 0;
    bool                  collided = false;
    VECTOR2I              force, totalForce;
    std::set<const ITEM*> handled;

    while( iter < maxIter )
    {
        NODE::OPT_OBSTACLE obs = m_currentNode->CheckColliding( &virtHead, solidsOnly ?
                                                                           ITEM::SOLID_T :
                                                                           ITEM::ANY_T  );
        if( !obs || handled.count( obs->m_item ) )
            break;

        int clearance = m_currentNode->GetClearance( obs->m_item, &m_currentTrace.PLine(), false );
        VECTOR2I layerForce;
        collided = false;

        for( int viaLayer : virtHead.RelevantShapeLayers( obs->m_item ) )
        {
            collided |= obs->m_item->Shape( viaLayer )->Collide( virtHead.Shape( viaLayer ),
                                                                 clearance, &layerForce );

            if( layerForce.SquaredEuclideanNorm() > force.SquaredEuclideanNorm() )
                force = layerForce;
        }

        if( collided )
        {
            totalForce += force;
            virtHead.SetPos( virtHead.Pos() + force );
        }

        handled.insert( obs->m_item );

        iter++;
    }

    bool succeeded = ( !collided || iter != maxIter );

    if( succeeded )
    {
        aNewP = aP + force;
        return true;
    }

    return false;
}


bool DIFF_PAIR_PLACER::attemptWalk( NODE* aNode, DIFF_PAIR* aCurrent, DIFF_PAIR& aWalk,
                                    bool aPFirst, bool aWindCw, bool aSolidsOnly )
{
    WALKAROUND walkaround( aNode, Router() );
    WALKAROUND::STATUS wf1;

    walkaround.SetSolidsOnly( aSolidsOnly );
    walkaround.SetIterationLimit( Settings().WalkaroundIterationLimit() );
    walkaround.SetAllowedPolicies( { WALKAROUND::WP_SHORTEST } );

    SHOVE shove( aNode, Router() );
    LINE walkP, walkN;

    aWalk = *aCurrent;

    int iter = 0;

    DIFF_PAIR cur( *aCurrent );

    bool currentIsP = aPFirst;

    int mask = aSolidsOnly ? ITEM::SOLID_T : ITEM::ANY_T;

    do
    {
        LINE preWalk = ( currentIsP ? cur.PLine() : cur.NLine() );
        LINE preShove = ( currentIsP ? cur.NLine() : cur.PLine() );
        LINE postWalk;

        if( !aNode->CheckColliding ( &preWalk, mask ) )
        {
            currentIsP = !currentIsP;

            if( !aNode->CheckColliding( &preShove, mask ) )
                break;
            else
                continue;
        }

        auto wf1 = walkaround.Route( preWalk );

        if( wf1.status[ WALKAROUND::WP_SHORTEST ] != WALKAROUND::ST_DONE )
            return false;

        postWalk = wf1.lines[ WALKAROUND::WP_SHORTEST ];

        LINE postShove( preShove );

        shove.ForceClearance( true, cur.Gap() - 2 * PNS_HULL_MARGIN );

        bool sh1;

        sh1 = shove.ShoveObstacleLine( postWalk, preShove, postShove );

        if( !sh1 )
            return false;

        postWalk.Line().Simplify();
        postShove.Line().Simplify();

        cur.SetShape( postWalk.CLine(), postShove.CLine(), !currentIsP );

        currentIsP = !currentIsP;

        if( !aNode->CheckColliding( &postShove, mask ) )
            break;

        iter++;
    }
    while( iter < 3 );

    if( iter == 3 )
        return false;

    aWalk.SetShape( cur.CP(), cur.CN() );

    return true;
}


bool DIFF_PAIR_PLACER::tryWalkDp( NODE* aNode, DIFF_PAIR &aPair, bool aSolidsOnly )
{
    DIFF_PAIR best;
    double bestScore = 100000000000000.0;

    for( int attempt = 0; attempt <= 3; attempt++ )
    {
        DIFF_PAIR p;
        NODE *tmp = m_currentNode->Branch();

        bool pfirst = ( attempt & 1 ) ? true : false;
        bool wind_cw = ( attempt & 2 ) ? true : false;

        if( attemptWalk( tmp, &aPair, p, pfirst, wind_cw, aSolidsOnly ) )
        {
            double cl   = 1 + p.CoupledLength();
            double skew = p.Skew();

            double score = cl + fabs( skew ) * 3.0;

            if( score < bestScore )
            {
                bestScore = score;
                best = std::move( p );
            }
        }

        delete tmp;
    }

    if( bestScore > 0.0 )
    {
        OPTIMIZER optimizer( m_currentNode );

        aPair.SetShape( best );
        optimizer.Optimize( &aPair );

        return true;
    }

    return false;
}


bool DIFF_PAIR_PLACER::rhWalkOnly( const VECTOR2I& aP )
{
    if( !routeHead ( aP ) )
        return false;

    m_fitOk = tryWalkDp( m_currentNode, m_currentTrace, false );

    return m_fitOk;
}


bool DIFF_PAIR_PLACER::route( const VECTOR2I& aP )
{
    switch( Settings().Mode() )
    {
    case RM_MarkObstacles:
        return rhMarkObstacles( aP );
    case RM_Walkaround:
        return rhWalkOnly( aP );
    case RM_Shove:
        return rhShoveOnly( aP );
    default:
        break;
    }

    return false;
}


bool DIFF_PAIR_PLACER::rhShoveOnly( const VECTOR2I& aP )
{
    m_currentNode = m_shove->CurrentNode();

    bool ok = routeHead( aP );

    m_fitOk  = false;

    if( !ok )
        return false;

    if( !tryWalkDp( m_currentNode, m_currentTrace, true ) )
        return false;

    LINE pLine( m_currentTrace.PLine() );
    LINE nLine( m_currentTrace.NLine() );
    ITEM_SET head;

    m_shove->ClearHeads();
    m_shove->AddHeads( pLine );
    m_shove->AddHeads( nLine );

    SHOVE::SHOVE_STATUS status = m_shove->Run();

    m_currentNode = m_shove->CurrentNode();

    if( status == SHOVE::SH_OK )
    {
        m_currentNode = m_shove->CurrentNode();

        if( m_shove->HeadsModified( 0 ))
            pLine = m_shove->GetModifiedHead(0);

        if( m_shove->HeadsModified( 1 ))
            nLine = m_shove->GetModifiedHead(1);

        // Update m_currentTrace with the shoved shapes so FixRoute() commits correct geometry
        m_currentTrace.SetShape( pLine.CLine(), nLine.CLine() );

        if( !m_currentNode->CheckColliding( &pLine ) &&
            !m_currentNode->CheckColliding( &nLine ) )
        {
            m_fitOk = true;
        }
    }
    else
    {
        // bring back previous state
        m_currentTrace.SetShape( pLine.CLine(), nLine.CLine() );
    }


    return m_fitOk;
}


const ITEM_SET DIFF_PAIR_PLACER::Traces()
{
      ITEM_SET t;

      t.Add( &m_currentTrace.PLine() );
      t.Add( &m_currentTrace.NLine() );

      return t;
}


void DIFF_PAIR_PLACER::FlipPosture()
{
    m_startDiagonal = !m_startDiagonal;

    if( !m_idle )
        Move( m_currentEnd, nullptr );
}


NODE* DIFF_PAIR_PLACER::CurrentNode( bool aLoopsRemoved ) const
{
    if( m_lastNode )
        return m_lastNode;

    return m_currentNode;
}


bool DIFF_PAIR_PLACER::SetLayer( int aLayer )
{
    if( m_idle )
    {
        m_currentLayer = aLayer;
        return true;
    }
    else if( m_chainedPlacement || !m_prevPair )
    {
        return false;
    }
    else if( !m_prevPair->PrimP() || ( m_prevPair->PrimP()->OfKind( ITEM::VIA_T ) &&
                m_prevPair->PrimP()->Layers().Overlaps( aLayer ) ) )
    {
        m_currentLayer = aLayer;
        m_start = *m_prevPair;
        initPlacement();
        Move( m_currentEnd, nullptr );
        return true;
    }

    return false;
}


OPT_VECTOR2I getDanglingAnchor( NODE* aNode, ITEM* aItem )
{
    switch( aItem->Kind() )
    {
    case ITEM::LINE_T:
    {
        LINE* l = static_cast<LINE*>( aItem );

        if( !l->PointCount() )
            return OPT_VECTOR2I();
        else
            return l->CPoint( 0 );
    }
    case ITEM::VIA_T:
    case ITEM::SOLID_T:
        return aItem->Anchor( 0 );

    case ITEM::ARC_T:
    {
        ARC* a = static_cast<ARC*>( aItem );

        const JOINT* jA = aNode->FindJoint( aItem->Anchor( 0 ), aItem );
        const JOINT* jB = aNode->FindJoint( aItem->Anchor( 1 ), aItem );

        if( jA && jA->LinkCount() == 1 )
            return a->Arc().GetP0();
        else if( jB && jB->LinkCount() == 1 )
            return a->Arc().GetP1();
        else
            return OPT_VECTOR2I();
    }
    case ITEM::SEGMENT_T:
    {
        SEGMENT* s = static_cast<SEGMENT*>( aItem );

        const JOINT* jA = aNode->FindJoint( aItem->Anchor( 0 ), aItem );
        const JOINT* jB = aNode->FindJoint( aItem->Anchor( 1 ), aItem );

        if( jA && jA->LinkCount() == 1 )
            return s->Seg().A;
        else if( jB && jB->LinkCount() == 1 )
            return s->Seg().B;
        else
            return OPT_VECTOR2I();
    }

    default:
        return OPT_VECTOR2I();
    }
}



bool DIFF_PAIR_PLACER::FindDpPrimitivePair( NODE* aWorld, const VECTOR2I& aP, ITEM* aItem,
                                            DP_PRIMITIVE_PAIR& aPair, wxString* aErrorMsg )
{
    NET_HANDLE netP, netN;

    bool result = aWorld->GetRuleResolver()->DpNetPair( aItem, netP, netN );

    if( !result )
    {
        if( aErrorMsg )
        {
            *aErrorMsg = _( "Unable to find complementary differential pair "
                            "nets. Make sure the names of the nets belonging "
                            "to a differential pair end with either N/P or +/-." );
        }
        return false;
    }

    NET_HANDLE refNet = aItem->Net();
    NET_HANDLE coupledNet = ( refNet == netP ) ? netN : netP;

    OPT_VECTOR2I refAnchor = getDanglingAnchor( aWorld, aItem );
    ITEM* primRef = aItem;

    if( !refAnchor )
    {
        if( aErrorMsg )
        {
            *aErrorMsg = _( "Can't find a suitable starting point.  If starting "
                            "from an existing differential pair make sure you are "
                            "at the end." );
        }

        return false;
    }

    std::set<ITEM*> coupledItems;

    aWorld->AllItemsInNet( coupledNet, coupledItems );
    double bestDist = std::numeric_limits<double>::max();
    bool found = false;

    for( ITEM* item : coupledItems )
    {
        if( item->Kind() == aItem->Kind() )
        {
            OPT_VECTOR2I anchor = getDanglingAnchor( aWorld, item );

            if( !anchor )
                continue;

            double dist = ( *anchor - *refAnchor ).EuclideanNorm();

            bool shapeMatches = true;

            if( item->OfKind( ITEM::SOLID_T | ITEM::VIA_T ) && item->Layers() != aItem->Layers() )
            {
                shapeMatches = false;
            }

            if( dist < bestDist && shapeMatches )
            {
                found = true;
                bestDist = dist;

                if( refNet != netP )
                {
                    aPair = DP_PRIMITIVE_PAIR ( item, primRef );
                    aPair.SetAnchors( *anchor, *refAnchor );
                }
                else
                {
                    aPair = DP_PRIMITIVE_PAIR( primRef, item );
                    aPair.SetAnchors( *refAnchor, *anchor );
                }
            }
        }
    }

    if( !found )
    {
        if( aErrorMsg )
        {
            *aErrorMsg = wxString::Format( _( "Can't find a suitable starting point "
                                              "for coupled net \"%s\"." ),
                                           aWorld->GetRuleResolver()->NetName( coupledNet ) );
        }

        return false;
    }

    return true;
}


int DIFF_PAIR_PLACER::viaGap() const
{
    return std::max( m_sizes.DiffPairViaGap(),
                     m_sizes.GetDiffPairHoleToHole() + m_sizes.ViaDrill() - m_sizes.ViaDiameter() );
}


int DIFF_PAIR_PLACER::gap() const
{
    return m_sizes.DiffPairGap() + m_sizes.DiffPairWidth();
}


bool DIFF_PAIR_PLACER::Start( const VECTOR2I& aP, ITEM* aStartItem )
{
    VECTOR2I p( aP );

    setWorld( Router()->GetWorld() );
    m_currentNode = m_world;

    wxString err_msg;

    if( !FindDpPrimitivePair( m_currentNode, aP, aStartItem, m_start, &err_msg ) )
    {
        Router()->SetFailureReason( err_msg );
        return false;
    }

    m_netP = m_start.PrimP()->Net();
    m_netN = m_start.PrimN()->Net();

    m_currentStart = p;
    m_currentEnd = p;
    m_placingVia = false;
    m_chainedPlacement = false;
    m_currentTraceOk = false;
    m_currentTrace = DIFF_PAIR();
    m_currentTrace.SetNets( m_netP, m_netN );
    m_lastFixNode = nullptr;

    initPlacement();

    return true;
}


void DIFF_PAIR_PLACER::initPlacement()
{
    m_idle = false;
    m_orthoMode = false;
    m_currentEndItem = nullptr;
    m_startDiagonal = m_initialDiagonal;

    NODE* world = Router()->GetWorld();

    world->KillChildren();
    NODE* rootNode = world->Branch();

    setWorld( rootNode );

    m_lastNode = nullptr;
    m_currentNode = rootNode;

    m_shove = std::make_unique<SHOVE>( m_currentNode, Router() );
}


bool DIFF_PAIR_PLACER::routeHead( const VECTOR2I& aP )
{
    m_fitOk = false;

    DP_GATEWAYS gwsEntry( gap() );
    DP_GATEWAYS gwsTarget( gap() );

    if( !m_prevPair )
        m_prevPair = m_start;

    gwsEntry.BuildFromPrimitivePair( *m_prevPair, m_startDiagonal );

    DP_PRIMITIVE_PAIR target;

    if( FindDpPrimitivePair( m_currentNode, aP, m_currentEndItem, target ) )
    {
        gwsTarget.BuildFromPrimitivePair( target, m_startDiagonal );
        m_snapOnTarget = true;
    }
    else
    {
        VECTOR2I fp;

        if( !propagateDpHeadForces( aP, fp ) )
            return false;

        VECTOR2I midp, dirV;
        m_prevPair->CursorOrientation( fp, midp, dirV );

        VECTOR2I fpProj = SEG( midp, midp + dirV ).LineProject( fp );

        // compute 'leader point' distance from the cursor (project cursor position
        // on the extension of the starting segment pair of the DP)
        int lead_dist = ( fpProj - fp ).EuclideanNorm();

        gwsTarget.SetFitVias( m_placingVia, m_sizes.ViaDiameter(), viaGap() );

        // far from the initial segment extension line -> allow a 45-degree obtuse turn
        if( lead_dist > ( m_sizes.DiffPairGap() + m_sizes.DiffPairWidth() ) / 2 )
        {
            gwsTarget.BuildForCursor( fp );
        }
        else
        {
            // close to the initial segment extension line -> keep straight part only, project
            // as close as possible to the cursor.
            gwsTarget.BuildForCursor( fpProj );
            gwsTarget.FilterByOrientation( DIRECTION_45::ANG_STRAIGHT | DIRECTION_45::ANG_HALF_FULL,
                                           DIRECTION_45( dirV ) );
        }

        m_snapOnTarget = false;
    }

    m_currentTrace.SetGap( gap() );
    m_currentTrace.SetLayer( m_currentLayer );

    bool result = gwsEntry.FitGateways( gwsEntry, gwsTarget, m_startDiagonal, m_currentTrace );

    if( result )
    {
        m_currentTraceOk = true;
        m_currentTrace.SetNets( m_netP, m_netN );
        m_currentTrace.SetWidth( m_sizes.DiffPairWidth() );
        m_currentTrace.SetGap( m_sizes.DiffPairGap() );

        if( m_placingVia )
        {
            m_currentTrace.AppendVias ( makeVia( m_currentTrace.CP().CLastPoint(), m_netP ),
                                        makeVia( m_currentTrace.CN().CLastPoint(), m_netN ) );
        }
        else
        {
            m_currentTrace.RemoveVias();
        }

        return true;
    }

    return m_currentTraceOk;
}


bool DIFF_PAIR_PLACER::Move( const VECTOR2I& aP , ITEM* aEndItem )
{
    m_currentEndItem = aEndItem;
    m_fitOk = false;

    delete m_lastNode;
    m_lastNode = nullptr;

    bool retval = route( aP );

    NODE* latestNode = m_currentNode;
    m_lastNode = latestNode->Branch();

    assert( m_lastNode != nullptr );
    m_currentEnd = aP;

    updateLeadingRatLine();

    return retval;
}


void DIFF_PAIR_PLACER::UpdateSizes( const SIZES_SETTINGS& aSizes )
{
    m_sizes = aSizes;

    if( !m_idle )
    {
        m_currentTrace.SetWidth( m_sizes.DiffPairWidth() );
        m_currentTrace.SetGap( m_sizes.DiffPairGap() );

        if( m_currentTrace.EndsWithVias() )
        {
            m_currentTrace.SetViaDiameter( m_sizes.ViaDiameter() );
            m_currentTrace.SetViaDrill( m_sizes.ViaDrill() );
        }
    }
}


bool DIFF_PAIR_PLACER::FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish )
{
    if( !m_fitOk && !Settings().AllowDRCViolations() )
        return false;

    if( m_currentTrace.CP().SegmentCount() < 1 || m_currentTrace.CN().SegmentCount() < 1 )
        return false;

    if( m_currentTrace.CP().SegmentCount() > 1 )
        m_initialDiagonal = !DIRECTION_45( m_currentTrace.CP().CSegment( -2 ) ).IsDiagonal();

    TOPOLOGY topo( m_lastNode );

    if( !m_snapOnTarget && !m_currentTrace.EndsWithVias() && !aForceFinish &&
        !Settings().GetFixAllSegments() )
    {
        SHAPE_LINE_CHAIN newP( m_currentTrace.CP() );
        SHAPE_LINE_CHAIN newN( m_currentTrace.CN() );

        if( newP.SegmentCount() > 1 && newN.SegmentCount() > 1 )
        {
            newP.Remove( -1, -1 );
            newN.Remove( -1, -1 );
        }

        m_currentTrace.SetShape( newP, newN );
    }

    if( m_currentTrace.EndsWithVias() )
    {
        m_lastNode->Add( Clone( m_currentTrace.PLine().Via() ) );
        m_lastNode->Add( Clone( m_currentTrace.NLine().Via() ) );
        m_chainedPlacement = false;
    }
    else
    {
        m_chainedPlacement = !m_snapOnTarget && !aForceFinish;
    }

    LINE lineP( m_currentTrace.PLine() );
    LINE lineN( m_currentTrace.NLine() );

    m_lastNode->Add( lineP );
    m_lastNode->Add( lineN );

    topo.SimplifyLine( &lineP );
    topo.SimplifyLine( &lineN );

    m_prevPair = m_currentTrace.EndingPrimitives();
    m_lastFixNode = m_lastNode;

    // avoid an use-after-free error (CommitPlacement calls NODE::Commit which will invalidate the shove heads state. Need to rethink the memory management).
    if( Settings().Mode() == RM_Shove )
        m_shove = std::make_unique<SHOVE>( m_world, Router() );

    CommitPlacement();
    m_placingVia = false;
    m_lastFixNode = nullptr;

    if( m_snapOnTarget || aForceFinish )
    {
        m_idle = true;
        return true;
    }
    else
    {
        initPlacement();
        return false;
    }
}


bool DIFF_PAIR_PLACER::AbortPlacement()
{
    m_world->KillChildren();
    m_lastNode = nullptr;
    return true;
}


bool DIFF_PAIR_PLACER::HasPlacedAnything() const
{
     return m_currentTrace.CP().SegmentCount() > 0 || m_currentTrace.CN().SegmentCount() > 0;
}


bool DIFF_PAIR_PLACER::CommitPlacement()
{
    if( m_lastFixNode )
        Router()->CommitRouting( m_lastFixNode );

    m_lastFixNode = nullptr;
    m_lastNode = nullptr;
    m_currentNode = nullptr;
    return true;
}


void DIFF_PAIR_PLACER::GetModifiedNets( std::vector<NET_HANDLE> &aNets ) const
{
    aNets.push_back( m_netP );
    aNets.push_back( m_netN );
}


void DIFF_PAIR_PLACER::updateLeadingRatLine()
{
    SHAPE_LINE_CHAIN ratLineN, ratLineP;
    TOPOLOGY topo( m_lastNode );

    if( topo.LeadingRatLine( &m_currentTrace.PLine(), ratLineP ) )
        m_router->GetInterface()->DisplayRatline( ratLineP, m_netP );

    if( topo.LeadingRatLine ( &m_currentTrace.NLine(), ratLineN ) )
        m_router->GetInterface()->DisplayRatline( ratLineN, m_netN );
}


const std::vector<NET_HANDLE> DIFF_PAIR_PLACER::CurrentNets() const
{
    std::vector<NET_HANDLE> rv;
    rv.push_back( m_netP );
    rv.push_back( m_netN );
    return rv;
}

}

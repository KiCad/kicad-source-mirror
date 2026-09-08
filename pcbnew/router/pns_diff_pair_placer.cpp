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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <core/typeinfo.h>

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


static void drawGateways( PNS::DEBUG_DECORATOR *dbg, const wxString& groupName, PNS::DP_PRIMITIVE_PAIR& prims, PNS::DP_GATEWAYS& gws, VECTOR2D offset, VECTOR2D step );


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
{
    if( m_target )
        m_target->Unlink();
    if( m_prevPair )
        m_prevPair->Unlink();
}


void DIFF_PAIR_PLACER::setWorld( NODE* aWorld )
{
    m_world = aWorld;
}


const VIA DIFF_PAIR_PLACER::makeVia( const VECTOR2I& aP, NET_HANDLE aNet )
{
    const PNS_LAYER_RANGE layers = Router()->GetInterface()->GetViaLayerRange( m_sizes );

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


    COLLISION_SEARCH_OPTIONS ctxOpts;
    ctxOpts.m_filter = [&]( const PNS::ITEM* aTestItem, const PNS::ITEM* aRefItem ) -> bool
    {
        if( aTestItem->Net() == m_currentTrace.NetP() || aTestItem->Net() == m_currentTrace.NetN() )
        {
            return false;
        }
        return true;
    };

    bool collP = ( m_currentNode->CheckColliding( &m_currentTrace.PLine(), ctxOpts ) ).has_value();
    bool collN = ( m_currentNode->CheckColliding( &m_currentTrace.NLine(), ctxOpts ) ).has_value();

    m_fitOk = !( collP || collN );

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

    COLLISION_SEARCH_OPTIONS ctxOpts;
    ctxOpts.m_filter = [&]( const PNS::ITEM* aTestItem, const PNS::ITEM* aRefItem ) -> bool
    {
        if( aTestItem->Net() == m_currentTrace.NetP() || aTestItem->Net() == m_currentTrace.NetN() )
        {
            return false;
        }
        return true;
    };

    ctxOpts.m_kindMask = solidsOnly ? ITEM::SOLID_T : ITEM::ANY_T;
    while( iter < maxIter )
    {
        NODE::OPT_OBSTACLE obs = m_currentNode->CheckColliding( &virtHead, ctxOpts );
        if( !obs || handled.count( obs->m_item ) )
            break;

        int      clearance = m_currentNode->GetClearance( obs->m_item, &m_currentTrace.PLine(), false );
        VECTOR2I layerForce;
        collided = false;

        for( int viaLayer : virtHead.RelevantShapeLayers( obs->m_item ) )
        {
            collided |= obs->m_item->Shape( viaLayer )->Collide( virtHead.Shape( viaLayer ), clearance, &layerForce );

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
    COLLISION_SEARCH_OPTIONS opts;

    auto excludeHeadDp = [aCurrent]( const ITEM* aTestItem, const ITEM* aRefItem ) -> bool
    {
        if( aTestItem->Net() == aCurrent->NetN() || aTestItem->Net() == aCurrent->NetP() )
        {
            return false;
        }
        return true;
    };

    walkaround.SetSolidsOnly( aSolidsOnly );
    walkaround.SetIterationLimit( Settings().WalkaroundIterationLimit() );
    walkaround.SetAllowedPolicies( { WALKAROUND::WP_SHORTEST } );
    walkaround.SetCollisionFilter( excludeHeadDp );
    walkaround.SetDebugDecorator( Dbg() );


    SHOVE shove( aNode, Router() );
    LINE walkP, walkN;

    aWalk = *aCurrent;

    int iter = 0;

    DIFF_PAIR cur( *aCurrent );

    bool currentIsP = aPFirst;

    int mask = aSolidsOnly ? ITEM::SOLID_T : ITEM::ANY_T;

    opts.m_kindMask = mask;
    opts.m_filter = excludeHeadDp;

    do
    {
        LINE preWalk = ( currentIsP ? cur.PLine() : cur.NLine() );
        LINE preShove = ( currentIsP ? cur.NLine() : cur.PLine() );
        LINE postWalk;

        if( !aNode->CheckColliding ( &preWalk, opts ) )
        {
            currentIsP = !currentIsP;

            if( !aNode->CheckColliding( &preShove, opts ) )
                break;
            else
                continue;
        }

        PNS_DBG( Dbg(), AddItem, &preWalk, GREEN, 100000, wxString::Format("preWalk") );

        WALKAROUND::RESULT wf1 = walkaround.Route( preWalk );

        if( wf1.status[ WALKAROUND::WP_SHORTEST ] != WALKAROUND::ST_DONE )
            return false;

        postWalk = wf1.lines[ WALKAROUND::WP_SHORTEST ];

        PNS_DBG( Dbg(), AddItem, &postWalk, BLUE, 100000, wxString::Format("postWalk") );

        LINE postShove( preShove );

        shove.ForceClearance( true, cur.Dimensions().Gap() - 2 * PNS_HULL_MARGIN );

        bool sh1;

        sh1 = shove.ShoveObstacleLine( postWalk, preShove, postShove );

        if( !sh1 )
            return false;

        postWalk.Line().Simplify();
        postShove.Line().Simplify();

        cur.SetShape( postWalk.CLine(), postShove.CLine(), !currentIsP );

        currentIsP = !currentIsP;

        if( !aNode->CheckColliding( &postShove, opts ) )
            break;

        iter++;
    }
    while( iter < 2 );

    if( iter == 2 )
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

    auto collisionFilter = [&]( const PNS::ITEM* aTestItem, const PNS::ITEM* aRefItem ) -> bool
    {
        if( aTestItem->Net() == m_currentTrace.NetP() || aTestItem->Net() == m_currentTrace.NetN() )
        {
            return false;
        }
        return true;
    };

    m_shove->SetCollisionFilter( collisionFilter );
    
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
    if( aLoopsRemoved && m_lastNode )
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
    case ITEM::SOLID_T: return aItem->Anchor( 0 );

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

    default: return OPT_VECTOR2I();
    }
}


bool DIFF_PAIR_PLACER::findDpEndingPrimitives( NODE* aWorld, const VECTOR2I& aP, ITEM* aStartItem,
                                               DP_PRIMITIVE_PAIR& aPair, wxString* aErrorMsg )
{
    NET_HANDLE netP, netN;

    (void) aWorld->GetRuleResolver()->DpNetPair( aStartItem, netP, netN );

    NET_HANDLE refNet = aStartItem->Net();
    NET_HANDLE coupledNet = ( refNet == netP ) ? netN : netP;

    OPT_VECTOR2I refAnchor = getDanglingAnchor( aWorld, aStartItem );
    ITEM*        primRef = aStartItem;

    double distThreshold = 10000000;

    if( auto seg = dyn_cast<SEGMENT*>( aStartItem ) )
    {
        distThreshold = seg->Width() / 2;
    }
    else if( auto arc = dyn_cast<ARC*>( aStartItem ) )
    {
        distThreshold = arc->Width() / 2;
    }

    if( !refAnchor || ( refAnchor->Distance( aP ) > distThreshold ) )
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
    bool   found = false;

    for( ITEM* item : coupledItems )
    {
        if( item->Kind() == aStartItem->Kind() )
        {
            OPT_VECTOR2I anchor = getDanglingAnchor( aWorld, item );

            if( !anchor )
                continue;

            double dist = ( *anchor - *refAnchor ).EuclideanNorm();

            bool shapeMatches = true;

            if( item->OfKind( ITEM::SOLID_T | ITEM::VIA_T ) && item->Layers() != aStartItem->Layers() )
            {
                shapeMatches = false;
            }

            if( dist < bestDist && shapeMatches )
            {
                found = true;
                bestDist = dist;

                if( refNet != netP )
                {
                    aPair = DP_PRIMITIVE_PAIR( item, primRef );
                    aPair.SetIsMidtrace( false );
                    aPair.SetAnchors( *anchor, *refAnchor );
                }
                else
                {
                    aPair = DP_PRIMITIVE_PAIR( primRef, item );
                    aPair.SetIsMidtrace( false );
                    aPair.SetAnchors( *refAnchor, *anchor );
                }
            }
        }
    }

    return found;
}


bool DIFF_PAIR_PLACER::findDpMidtraceIntersection( NODE* aWorld, const VECTOR2I& aP, ITEM* aStartItem,
                                                   DP_PRIMITIVE_PAIR& aPair, wxString* aErrorMsg )
{
    PNS::TOPOLOGY  topo( aWorld );
    PNS::DIFF_PAIR originPair;
    auto           startSeg = dyn_cast<PNS::SEGMENT*>( aStartItem );

    if( !startSeg )
    {
        return false;
    }

    if( aStartItem && aStartItem->OfKind( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T ) )
    {
        if( !topo.AssembleDiffPair( aStartItem, originPair ) )
            return false;

        auto ppair = originPair.BuildMidpairIntersection( startSeg, aP );


        if( ppair )
        {
            ppair->SetIsMidtrace( true );

            aPair = *ppair;
            return true;
        }
    }

    return false;
}


bool DIFF_PAIR_PLACER::FindDpPrimitivePair( NODE* aWorld, const VECTOR2I& aP, ITEM* aItem, DP_PRIMITIVE_PAIR& aPair,
                                            wxString* aErrorMsg )
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

    bool found = findDpEndingPrimitives( aWorld, aP, aItem, aPair, aErrorMsg );

    PNS_DBG( Dbg(), Message,
             wxString::Format( "EP=%d target-p [%d,%d] target-n [%d,%d]", found ? 1 : 0, m_target->AnchorP().x,
                               m_target->AnchorP().y, m_target->AnchorN().x, m_target->AnchorN().y, aP.x, aP.y ) );

    if( !found )
    {
        found = findDpMidtraceIntersection( aWorld, aP, aItem, aPair, aErrorMsg );

        PNS_DBG( Dbg(), Message,
                 wxString::Format( "MT=%d target-p [%d,%d] target-n [%d,%d]", found ? 1 : 0, m_target->AnchorP().x,
                                   m_target->AnchorP().y, m_target->AnchorN().x, m_target->AnchorN().y, aP.x, aP.y ) );
    }


    if( !found )
    {
        if( aErrorMsg )
        {
            *aErrorMsg = wxString::Format( _( "Can't find a suitable starting point for the diff pair" ) );
        }

        return false;
    }

    return true;
}


int DIFF_PAIR_PLACER::viaGap() const
{
    return m_sizes.EffectiveDiffPairViaGap();
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
    m_hasFixedAnything = false;
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

    PNS_DBG( Dbg(), Message, wxString::Format("Start-is-mid %d wd %d rd %d", m_start.IsMidtrace()?1:0, (int) world->Depth(), (int) rootNode->Depth() ) );

    if( m_start.IsMidtrace() )
    {
        SplitAdjacentSegments( rootNode, m_start.PrimP(), m_start.AnchorP() );
        SplitAdjacentSegments( rootNode, m_start.PrimN(), m_start.AnchorN() );
    }

    setWorld( rootNode );

    m_lastNode = nullptr;
    m_currentNode = rootNode;

    m_shove = std::make_unique<SHOVE>( m_currentNode, Router() );
}


static void drawSingleGateway( DEBUG_DECORATOR* dbg, DP_GATEWAY gw, wxString grpName )
{
    PNS_DBG( dbg, BeginGroup, grpName, 0 );

    SHAPE_CIRCLE gwp( gw.AnchorP(), 30000 );
    PNS_DBG( dbg, AddShape, &gwp, LIGHTRED, 20000, "gw-entry-p" );

    if( gw.EntryP().SegmentCount() )
    {
        PNS_DBG( dbg, AddShape, &gw.EntryP(), LIGHTRED, 20000, "gwp" );
    }

    SHAPE_CIRCLE gwn( gw.AnchorN(), 30000 );
    PNS_DBG( dbg, AddShape, &gwn, LIGHTBLUE, 30000, "gw-entry-n" );

    if( gw.EntryN().SegmentCount() )
    {
        PNS_DBG( dbg, AddShape, &gw.EntryN(), LIGHTBLUE, 10000, "gwn" );
    }

    SHAPE_LINE_CHAIN link( { gw.AnchorN(), gw.AnchorP() } );
    PNS_DBG( dbg, AddShape, &link, LIGHTGRAY, 10000, "link" );

    PNS_DBGN( dbg, EndGroup );
}


bool DIFF_PAIR_PLACER::routeHead( const VECTOR2I& aP )
{
    std::optional<int> minClearance = 0;

    RULE_RESOLVER* ruleResolver = Router()->GetInterface()->GetRuleResolver();
    DP_DIMENSIONS  dims( m_sizes.DiffPairWidth(), m_sizes.DiffPairGap(), viaGap(), m_sizes.ViaDiameter(), 0 );
    DP_GATEWAYS    gwsEntry;
    DP_GATEWAYS    gwsTarget;

    m_fitOk = false;

    auto updateMinClearance = [&minClearance, ruleResolver]( const DP_PRIMITIVE_PAIR& aTarget )
    {
        if( aTarget.PrimN() && aTarget.PrimP() )
        {
            int clearance = ruleResolver->Clearance( aTarget.PrimP(), aTarget.PrimN() );
            if( minClearance )
                minClearance = std::max( clearance, minClearance.value() );
            else
                minClearance = clearance;
        }
    };

    m_target.reset();

    if( !m_prevPair )
        m_prevPair = m_start;


    if( m_prevPair )
        updateMinClearance( *m_prevPair );

    DP_DIMENSIONS dims2( m_sizes.DiffPairWidth(), m_sizes.DiffPairGap(), viaGap(), m_sizes.ViaDiameter(),
                         minClearance.value() );
    gwsEntry.SetDimensions( dims2 );
    gwsEntry.BuildFromPrimitivePair( *m_prevPair, m_startDiagonal );

    if( m_prevPair )
        drawGateways( Dbg(), wxT( "entry-gateways" ), *m_prevPair, gwsEntry, VECTOR2D( 0, 0 ), VECTOR2D( 0, 2000000 ) );

    DP_PRIMITIVE_PAIR target;
    VECTOR2I          midpoint;
    bool              snapVias = false;
    bool              foundTarget = false;

    if( FindDpPrimitivePair( m_currentNode, aP, m_currentEndItem, target ) )
    {
        if( m_placingVia && ( target.DirP() == target.DirN() ) )
        {
            midpoint = ( target.AnchorN() + target.AnchorP() ) / 2;
            PNS_DBG( Dbg(), AddPoint, midpoint, YELLOW, 100000, wxT( "midpoint" ) );
            snapVias = true;
        }
        else
        {
            updateMinClearance( target );
            DP_DIMENSIONS dimsTarget( m_sizes.DiffPairWidth(), m_sizes.DiffPairGap(), viaGap(), m_sizes.ViaDiameter(),
                                      minClearance.value() );

            gwsTarget.SetDimensions( dimsTarget );
            gwsTarget.BuildFromPrimitivePair( target, m_startDiagonal );
            m_snapOnTarget = true;
            m_target = target;

            PNS_DBG( Dbg(), Message,
                     wxString::Format( "target-p [%d,%d] target-n [%d,%d], cursor [%d,%d]", m_target->AnchorP().x,
                                       m_target->AnchorP().y, m_target->AnchorN().x, m_target->AnchorN().y, aP.x,
                                       aP.y ) );

            PNS_DBG( Dbg(), AddPoint, aP, YELLOW, 100000, wxT( "targer-cursor" ) );
            PNS_DBG( Dbg(), AddPoint, m_target->AnchorP(), RED, 100000, wxT( "anchor+" ) );
            PNS_DBG( Dbg(), AddPoint, m_target->AnchorN(), BLUE, 100000, wxT( "anchor-" ) );
            foundTarget = true;

            drawGateways( Dbg(), wxT( "target-gateways" ), target, gwsTarget, VECTOR2D( 0, 0 ),
                          VECTOR2D( 0, 2000000 ) );
        }
    }


    if( !foundTarget )
    {
        VECTOR2I fp;

        if( !propagateDpHeadForces( snapVias ? midpoint : aP, fp ) )
            return false;

        VECTOR2I midp, dirV;
        m_prevPair->CursorOrientation( fp, midp, dirV );

        VECTOR2I fpProj = SEG( midp, midp + dirV ).LineProject( fp );

        // compute 'leader point' distance from the cursor (project cursor position
        // on the extension of the starting segment pair of the DP)
        int lead_dist = ( fpProj - fp ).EuclideanNorm();

        gwsTarget.SetFitVias( m_placingVia );
        DP_DIMENSIONS dimsTarget( m_sizes.DiffPairWidth(), m_sizes.DiffPairGap(), viaGap(), m_sizes.ViaDiameter(),
                                  minClearance.value() );
        gwsTarget.SetDimensions( dimsTarget );

        int snapThreshold = ( m_sizes.DiffPairGap() + m_sizes.DiffPairWidth() ) / 2;

        PNS_DBG( Dbg(), Message,
                 wxString::Format( "leadDist %d snapVias %d thr %d dirv %s", lead_dist, snapVias ? 1 : 0, snapThreshold,
                                   DIRECTION_45( dirV ).Format() ) );

        // far from the initial segment extension line -> allow a 45-degree obtuse turn
        if( !snapVias && lead_dist > snapThreshold )
        {
            gwsTarget.BuildForCursor( fp );
        }
        else
        {
            // close to the initial segment extension line -> keep straight part only, project
            // as close as possible to the cursor.
            int mask = DIRECTION_45( dirV.Perpendicular() ).Mask() | DIRECTION_45( dirV ).Opposite().Mask();
            gwsTarget.BuildForCursor( snapVias ? fp : fpProj, mask );
            drawGateways( Dbg(), wxT( "target-aligned-gateways" ), target, gwsTarget, VECTOR2D( 0, 0 ),
                          VECTOR2D( 0, 200000 ) );
        }

        m_snapOnTarget = false;
    }

    m_currentTrace.SetDimensions( dims );
    m_currentTrace.SetLayer( m_currentLayer );

    DP_GAP_CONSTRAINT tmpGapC;
    tmpGapC.SetOpt( dims.Gap() );
    tmpGapC.SetMin( dims.Gap() - DP_DEFAULT_GAP_EPSILON );
    tmpGapC.SetMax( dims.Gap() + DP_DEFAULT_GAP_EPSILON );

    dims.SetGapConstraint( tmpGapC );
    dims.SetMinClearance( minClearance.value() );

    gwsEntry.SetDimensions( dims );
    gwsTarget.SetDimensions( dims );

    auto fits = gwsEntry.FitGateways( gwsEntry, gwsTarget, m_placingVia );

    const DP_GATEWAYS::FIT_RESULT* bestFits[2] = { nullptr, nullptr };
    const DP_GATEWAYS::FIT_RESULT* bestestFit = nullptr;

    for( bool rejectNonObtuseAngles : { true, false } )
    {
        bestestFit = nullptr;
        bestFits[0] = bestFits[1] = nullptr;

        int   bestScore[2] = { -100, -100 }; // cater for negative score adjustments
        float bestCpr[2] = { 0.0f, 0.0f };

        for( const auto& f : fits )
        {
            constexpr int angleMask = DIRECTION_45::ANG_OBTUSE | DIRECTION_45::ANG_STRAIGHT;

            PNS_DBG( Dbg(), BeginGroup,
                     wxString::Format( wxT( "fit: bestCpr0=%.3f bestCpr1=%.3f diag=%d cpr=%.2f ar=%.2f score=%d" ),
                                       bestCpr[0], bestCpr[1], f.diagonal ? 1 : 0, f.coupledRatio, f.aspectRatio,
                                       f.score ),
                     0 );
            drawSingleGateway( Dbg(), f.entry, wxString::Format( "entry=%s", f.entry.GetName() ) );
            drawSingleGateway( Dbg(), f.target, wxString::Format( "target=%s", f.target.GetName() ) );

            DIFF_PAIR dp( m_sizes.DiffPairGap() );
            dp.SetDimensions( dims );
            dp.SetShape( f.p, f.n );

            if( rejectNonObtuseAngles )
            {
                DIRECTION_45 startDirP = m_start.DirP();
                DIRECTION_45 startDirN = m_start.DirN();
                auto         angP = startDirP.Angle( dp.DirP( false ) );
                auto         angN = startDirN.Angle( dp.DirN( false ) );

                if( !( angP & angleMask ) || !( angN & angleMask ) )
                {
                    PNS_DBG( Dbg(), Message,
                             wxString::Format( " reject dp %s dn %s sd %s %s", dp.DirP( false ).Format(),
                                               dp.DirN( false ).Format(), startDirP.Format(), startDirN.Format() ) );
                    PNS_DBGN( Dbg(), EndGroup );

                    continue;
                }
            }
            PNS_DBGN( Dbg(), EndGroup );

            int index = f.diagonal ? 1 : 0;
            int score = f.score;

            if( score > bestScore[index] || f.coupledRatio > bestCpr[index] * 2.0 )
            {
                bestFits[index] = &f;
                bestScore[index] = score;
                bestCpr[index] = f.coupledRatio;
            }
            else if( score == bestScore[index] )
            {
                if( f.coupledRatio > bestCpr[index] )
                {
                    bestCpr[index] = f.coupledRatio;
                    bestFits[index] = &f;
                }
            }
        }

        if( bestFits[0] || bestFits[1] )
            break;
    }

    for( int index = 0; index < 2; index++ )
    {
        const DP_GATEWAYS::FIT_RESULT* f = bestFits[index];

        if( !f )
            continue;


        PNS_DBG( Dbg(), BeginGroup,
                 wxString::Format( wxT( "best: diag=%d cpr=%.2f ar=%.2f score=%d cl=%d" ), f->diagonal ? 1 : 0,
                                   f->coupledRatio, f->aspectRatio, f->score, minClearance.value() ),
                 0 );

        drawSingleGateway( Dbg(), f->entry, wxString::Format( "entry=%s", f->entry.GetName() ) );
        drawSingleGateway( Dbg(), f->target, wxString::Format( "target=%s", f->target.GetName() ) );

        PNS_DBG( Dbg(), AddShape, &f->p, RED, 20000, wxT( "l+" ) );
        PNS_DBG( Dbg(), AddShape, &f->n, BLUE, 20000, wxT( "l-" ) );

        PNS_DBGN( Dbg(), EndGroup );
    }

    if( m_startDiagonal && bestFits[1] )
        bestestFit = bestFits[1];
    else if( !m_startDiagonal && bestFits[0] )
        bestestFit = bestFits[0];
    else if( bestFits[1] )
        bestestFit = bestFits[1];
    else if( bestFits[0] )
        bestestFit = bestFits[0];

    if( bestestFit )
    {
        m_currentTraceOk = true;
        m_currentTrace.SetShape( bestestFit->p, bestestFit->n );
        m_currentTrace.SetNets( m_netP, m_netN );
        m_currentTrace.SetDimensions( dims );

        if( m_placingVia )
        {
            m_currentTrace.AppendVias( makeVia( m_currentTrace.CP().CLastPoint(), m_netP ),
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


bool DIFF_PAIR_PLACER::Move( const VECTOR2I& aP, ITEM* aEndItem )
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

    PNS_DBG( Dbg(), Message,
             wxString::Format( "target %d, p-sc %d n-sc %d", m_target ? 1 : 0,
                               m_currentTrace.PLine().SegmentCount() && m_currentTrace.NLine().SegmentCount() ) );


    if( m_target )
    {
        if( m_currentTrace.PLine().SegmentCount() && m_currentTrace.NLine().SegmentCount() )
        {
            if( m_target->PrimN()->Net() == m_currentTrace.NLine().Net() )
                SplitAdjacentSegments( m_lastNode, m_target->PrimN(), m_currentTrace.NLine().CLastPoint() );
            if( m_target->PrimP()->Net() == m_currentTrace.PLine().Net() )
                SplitAdjacentSegments( m_lastNode, m_target->PrimP(), m_currentTrace.PLine().CLastPoint() );

            if( Settings().RemoveLoops() )
            {
                removeLoops( m_lastNode, m_currentTrace.PLine() );
                removeLoops( m_lastNode, m_currentTrace.NLine() );
            }
        }
    }


    PNS_DBG( Dbg(), AddPoint, m_start.AnchorP(), RED, 100000, wxT( "start-p" ) );
    PNS_DBG( Dbg(), AddPoint, m_start.AnchorN(), BLUE, 100000, wxT( "start-n" ) );

    updateLeadingRatLine();

    return retval;
}


static void drawGateways( PNS::DEBUG_DECORATOR* dbg, const wxString& groupName, PNS::DP_PRIMITIVE_PAIR& prims,
                          PNS::DP_GATEWAYS& gws, VECTOR2D offset, VECTOR2D step )
{
    PNS_DBG( dbg, BeginGroup, groupName, 0 );
    for( auto gw : gws.Gateways() )
    {
        PNS_DBG( dbg, BeginGroup, wxString::Format( wxT( "gw-%s" ), gw.GetName() ), 0 );

        SHAPE_CIRCLE gwp( gw.AnchorP(), 30000 );
        PNS_DBG( dbg, AddShape, &gwp, LIGHTRED, 20000, "gwp" );

        if( gw.EntryP().SegmentCount() )
        {
            PNS_DBG( dbg, AddShape, &gw.EntryP(), LIGHTRED, 20000, "gwp" );
        }

        SHAPE_CIRCLE gwn( gw.AnchorN(), 30000 );
        PNS_DBG( dbg, AddShape, &gwn, LIGHTBLUE, 30000, "gwn" );

        if( gw.EntryN().SegmentCount() )
        {
            PNS_DBG( dbg, AddShape, &gw.EntryN(), LIGHTBLUE, 10000, "gwn" );
        }

        SHAPE_LINE_CHAIN link( { gw.AnchorN(), gw.AnchorP() } );
        PNS_DBG( dbg, AddShape, &link, LIGHTGRAY, 10000, "link" );

        auto midpoint = ( gw.AnchorN() + gw.AnchorP() ) / 2;

        if( gw.HasPrimaryDirection() )
        {
            for( int dir = 0; dir < 8; dir++ )
            {
                DIRECTION_45 dirV( (DIRECTION_45::Directions) dir );
                if( dirV.Mask() & gw.PrimaryDirectionMask() )
                {
                    auto             dv = dirV.ToVector().Resize( 1000000 );
                    SHAPE_LINE_CHAIN ds( { midpoint, midpoint + dv } );
                    PNS_DBG( dbg, AddShape, &ds, LIGHTYELLOW, 10000, wxString::Format( "pdir-%s", dirV.Format() ) );
                }
            }
        }

        PNS_DBGN( dbg, EndGroup );
    }
    PNS_DBGN( dbg, EndGroup );
}


void DIFF_PAIR_PLACER::UpdateSizes( const SIZES_SETTINGS& aSizes )
{
    int prevDiffPairWidth = m_sizes.DiffPairWidth();

    m_sizes = aSizes;

    if( !m_idle )
    {
        // When continuing from an existing track in connected-track-width mode, preserve the
        // inherited diff pair width rather than reverting to the netclass default. This matches
        // the guard in LINE_PLACER::UpdateSizes() for single tracks.
        if( !m_sizes.TrackWidthIsExplicit() && m_hasFixedAnything )
            m_sizes.SetDiffPairWidth( prevDiffPairWidth );

        DP_DIMENSIONS dims( m_sizes.DiffPairWidth(), m_sizes.DiffPairGap(), viaGap(), m_sizes.ViaDiameter(), 0 );
        m_currentTrace.SetDimensions( dims );

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

    LINE &lineP = m_currentTrace.PLine();
    LINE &lineN = m_currentTrace.NLine();

    m_lastNode->Add( lineP );
    m_lastNode->Add( lineN );

    //topo.SimplifyLine( &lineP );
    //topo.SimplifyLine( &lineN );

    m_currentTrace.SetLines( lineP, lineN );

    m_prevPair = m_currentTrace.EndingPrimitives();

    PNS_DBG( Dbg(), Message, wxString::Format("Fix-RT pp-p %p pp-n %p snapon=%d ff=%d", m_prevPair?m_prevPair->PrimP():0, m_prevPair?m_prevPair->PrimN():0, m_snapOnTarget?1:0, aForceFinish?1:0 ) );

    m_lastFixNode = m_lastNode;

    // avoid an use-after-free error (CommitPlacement calls NODE::Commit which will invalidate the shove heads state. Need to rethink the memory management).
    if( Settings().Mode() == RM_Shove )
        m_shove = std::make_unique<SHOVE>( m_world, Router() );

    CommitPlacement();

    m_currentTrace.Clear();
    m_currentTrace.ClearLinks();

    m_placingVia = false;
    m_lastFixNode = nullptr;

    if( m_snapOnTarget || aForceFinish )
    {
        m_idle = true;
        if( m_prevPair )
            m_prevPair->Unlink();
        
        m_target->Unlink();
        return true;
    }
    else
    {
        m_hasFixedAnything = true;
        m_start = *m_prevPair;

        PNS_DBG( Dbg(), Message, wxString::Format("Fix-RT2 pp-p %p pp-n %p snapon=%d ff=%d", m_prevPair?m_prevPair->PrimP():0, m_prevPair?m_prevPair->PrimN():0, m_snapOnTarget?1:0, aForceFinish?1:0 ) );

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
    m_target->Unlink();
    m_start = DP_PRIMITIVE_PAIR();

    if( m_lastFixNode )
        Router()->CommitRouting( m_lastFixNode );
    else if( m_prevPair )
        m_prevPair->Unlink();

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

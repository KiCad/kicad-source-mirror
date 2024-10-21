/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2023 KiCad Developers.
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


#include <advanced_config.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_track.h>

#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_rtree.h>

#include <geometry/shape_segment.h>

#include <connectivity/connectivity_data.h>
#include <connectivity/from_to_cache.h>

#include <view/view_overlay.h>

/*
    Differential pair gap/coupling test.
    Errors generated:
    - DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE
    - DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG
    - DRCE_TOO_MANY_VIAS
    Todo:
    - arc support.
    - improve recognition of coupled segments (now anything that's parallel is considered
      coupled, causing DRC errors on meanders)
*/

namespace test {

class DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING () :
        m_board( nullptr )
    {
    }

    virtual ~DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return wxT( "diff_pair_coupling" );
    };

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests differential pair coupling" );
    }

private:
    BOARD* m_board;
};

};


static bool commonParallelProjection( SEG p, SEG n, SEG &pClip, SEG& nClip )
{
    SEG n_proj_p( p.LineProject( n.A ), p.LineProject( n.B ) );

    int64_t t_a = 0;
    int64_t t_b = p.TCoef( p.B );

    int64_t tproj_a = p.TCoef( n_proj_p.A );
    int64_t tproj_b = p.TCoef( n_proj_p.B );

    if( t_b < t_a )
        std::swap( t_b, t_a );

    if( tproj_b < tproj_a )
        std::swap( tproj_b, tproj_a );

    if( t_b <= tproj_a )
        return false;

    if( t_a >= tproj_b )
        return false;

    int64_t t[4] = { 0, p.TCoef( p.B ), p.TCoef( n_proj_p.A ), p.TCoef( n_proj_p.B ) };
    std::vector<int64_t> tv( t, t + 4 );
    std::sort( tv.begin(), tv.end() ); // fixme: awful and disgusting way of finding 2 midpoints

    int64_t pLenSq = p.SquaredLength();

    VECTOR2I dp = p.B - p.A;
    pClip.A.x = p.A.x + rescale( (int64_t)dp.x, tv[1], pLenSq );
    pClip.A.y = p.A.y + rescale( (int64_t)dp.y, tv[1], pLenSq );

    pClip.B.x = p.A.x + rescale( (int64_t)dp.x, tv[2], pLenSq );
    pClip.B.y = p.A.y + rescale( (int64_t)dp.y, tv[2], pLenSq );

    nClip.A = n.LineProject( pClip.A );
    nClip.B = n.LineProject( pClip.B );

    return true;
}


static bool commonParallelProjection( const PCB_ARC& p, const PCB_ARC& n, SHAPE_ARC &pClip, SHAPE_ARC& nClip )
{
    VECTOR2I p_center = p.GetCenter();
    VECTOR2I n_center = n.GetCenter();
    double p_radius = p.GetRadius();
    double n_radius = n.GetRadius();

    VECTOR2I p_start( p.GetStart() );
    VECTOR2I p_end( p.GetEnd() );

    if( !p.IsCCW() )
        std::swap( p_start, p_end );

    VECTOR2I n_start( n.GetStart() );
    VECTOR2I n_end( n.GetEnd() );

    if( !n.IsCCW() )
        std::swap( n_start, n_end );

    SHAPE_ARC p_arc( p_start, p.GetMid(), p_end, 0 );
    SHAPE_ARC n_arc( n_start, n.GetMid(), n_end, 0 );

    EDA_ANGLE p_start_angle = p_arc.GetStartAngle();

    // Rotate the arcs to a common 0 starting angle
    p_arc.Rotate( -p_start_angle, p_center );
    n_arc.Rotate( -p_start_angle, n_center );

    EDA_ANGLE p_end_angle = p_arc.GetEndAngle();
    EDA_ANGLE n_start_angle = n_arc.GetStartAngle();
    EDA_ANGLE n_end_angle = n_arc.GetEndAngle();


    EDA_ANGLE clip_total_angle;
    EDA_ANGLE clip_start_angle;

    if( n_start_angle > p_end_angle )
    {
        // n is fully outside of p
        if( n_end_angle > p_end_angle )
            return false;

        // n starts before angle 0 and ends in the middle of p
        clip_total_angle = n_end_angle + p_start_angle;
        clip_start_angle = p_start_angle;
    }
    else
    {
        clip_start_angle = n_start_angle + p_start_angle;

        // n is fully inside of p
        if( n_end_angle < p_end_angle )
            clip_total_angle = n_end_angle - n_start_angle;
        else // n starts after 0 and ends after p
            clip_total_angle = p_end_angle - n_start_angle;
    }

    // One arc starts approximately where the other ends
    if( clip_total_angle <= EDA_ANGLE( ADVANCED_CFG::GetCfg().m_MinParallelAngle ) )
        return false;

    VECTOR2I n_start_pt = n_center + VECTOR2I( KiROUND( n_radius ), 0 );
    VECTOR2I p_start_pt = p_center + VECTOR2I( KiROUND( p_radius ), 0 );

    RotatePoint( n_start_pt, n_center, clip_start_angle );
    RotatePoint( p_start_pt, p_center, clip_start_angle );

    pClip = SHAPE_ARC( p_center, p_start_pt, clip_total_angle );
    nClip = SHAPE_ARC( n_center, n_start_pt, clip_total_angle );

    return true;
}


struct DIFF_PAIR_KEY
{
    bool operator<( const DIFF_PAIR_KEY& b ) const
    {
        if( netP < b.netP )
        {
            return true;
        }
        else if( netP > b.netP )
        {
            return false;
        }
        else // netP == b.netP
        {
            if( netN < b.netN )
                return true;
            else if( netN > b.netN )
                return false;
            else if( gapRuleName.IsEmpty() )
                return gapRuleName < b.gapRuleName;
            else
                return uncoupledRuleName < b.uncoupledRuleName;
        }
    }

    int       netP, netN;
    wxString  gapRuleName;
    wxString  uncoupledRuleName;
    std::optional<MINOPTMAX<int>> gapConstraint;
    DRC_RULE* gapRule;
    std::optional<MINOPTMAX<int>> uncoupledConstraint;
    DRC_RULE* uncoupledRule;
};

struct DIFF_PAIR_COUPLED_SEGMENTS
{
    SEG          coupledN;
    SEG          coupledP;
    bool         isArc;
    SHAPE_ARC    coupledArcN;
    SHAPE_ARC    coupledArcP;
    PCB_TRACK*   parentN;
    PCB_TRACK*   parentP;
    int          computedGap;
    PCB_LAYER_ID layer;
    bool         couplingFailMin;
    bool         couplingFailMax;

    DIFF_PAIR_COUPLED_SEGMENTS() :
        isArc( false ),
        parentN( nullptr ),
        parentP( nullptr ),
        computedGap( 0 ),
        layer( UNDEFINED_LAYER ),
        couplingFailMin( false ),
        couplingFailMax( false )
    {}
};


struct DIFF_PAIR_ITEMS
{
    std::set<BOARD_CONNECTED_ITEM*> itemsP, itemsN;
    std::vector<DIFF_PAIR_COUPLED_SEGMENTS> coupled;
    int totalCoupled;
    int totalLengthN;
    int totalLengthP;
};


static void extractDiffPairCoupledItems( DIFF_PAIR_ITEMS& aDp )
{
    for( BOARD_CONNECTED_ITEM* itemP : aDp.itemsP )
    {
        PCB_TRACK* sp = dyn_cast<PCB_TRACK*>( itemP );
        std::optional<DIFF_PAIR_COUPLED_SEGMENTS> bestCoupled;
        int bestGap = std::numeric_limits<int>::max();

        if( !sp )
            continue;

        for ( BOARD_CONNECTED_ITEM* itemN : aDp.itemsN )
        {
            PCB_TRACK* sn = dyn_cast<PCB_TRACK*> ( itemN );

            if( !sn )
                continue;

            if( ( sn->GetLayerSet() & sp->GetLayerSet() ).none() )
                continue;

            SEG ssp ( sp->GetStart(), sp->GetEnd() );
            SEG ssn ( sn->GetStart(), sn->GetEnd() );

            // Segments that are ~ 1 IU in length per side are approximately parallel (tolerance is 1 IU)
            // with everything and their parallel projection is < 1 IU, leading to bad distance calculations
            if( ssp.SquaredLength() > 2 && ssn.SquaredLength() > 2 && ssp.ApproxParallel(ssn) )
            {
                DIFF_PAIR_COUPLED_SEGMENTS cpair;
                bool coupled = commonParallelProjection( ssp, ssn, cpair.coupledP, cpair.coupledN );

                if( coupled )
                {
                    cpair.parentP = sp;
                    cpair.parentN = sn;
                    cpair.layer = sp->GetLayer();
                    cpair.computedGap = (cpair.coupledP.A - cpair.coupledN.A).EuclideanNorm();
                    cpair.computedGap -= ( sp->GetWidth() + sn->GetWidth() ) / 2;

                    if( cpair.computedGap < bestGap )
                    {
                        bestGap = cpair.computedGap;
                        bestCoupled = cpair;
                    }
                }

            }
        }

        if( bestCoupled )
        {
            auto excludeSelf = [&]( BOARD_ITEM* aItem )
            {
                if( aItem == bestCoupled->parentN || aItem == bestCoupled->parentP )
                    return false;

                if( aItem->Type() == PCB_TRACE_T || aItem->Type() == PCB_VIA_T
                    || aItem->Type() == PCB_ARC_T )
                {
                    BOARD_CONNECTED_ITEM* bci = static_cast<BOARD_CONNECTED_ITEM*>( aItem );

                    if( bci->GetNetCode() == bestCoupled->parentN->GetNetCode()
                        || bci->GetNetCode() == bestCoupled->parentP->GetNetCode() )
                    {
                        return false;
                    }
                }

                return true;
            };

            SHAPE_SEGMENT checkSegStart( bestCoupled->coupledP.A, bestCoupled->coupledN.A );
            SHAPE_SEGMENT checkSegEnd( bestCoupled->coupledP.B, bestCoupled->coupledN.B );
            DRC_RTREE*    tree = bestCoupled->parentP->GetBoard()->m_CopperItemRTreeCache.get();

            // check if there's anything in between the segments suspected to be coupled. If
            // there's nothing, assume they are really coupled.

            if( !tree->CheckColliding( &checkSegStart, sp->GetLayer(), 0, excludeSelf )
                  && !tree->CheckColliding( &checkSegEnd, sp->GetLayer(), 0, excludeSelf ) )
            {
                aDp.coupled.push_back( *bestCoupled );
            }
        }
    }

    for( BOARD_CONNECTED_ITEM* itemP : aDp.itemsP )
    {
        PCB_ARC* sp = dyn_cast<PCB_ARC*>( itemP );
        std::optional<DIFF_PAIR_COUPLED_SEGMENTS> bestCoupled;
        int bestGap = std::numeric_limits<int>::max();

        if( !sp )
            continue;

        for ( BOARD_CONNECTED_ITEM* itemN : aDp.itemsN )
        {
            PCB_ARC* sn = dyn_cast<PCB_ARC*> ( itemN );

            if( !sn )
                continue;

            if( ( sn->GetLayerSet() & sp->GetLayerSet() ).none() )
                continue;

            // Segments that are ~ 1 IU in length per side are approximately parallel (tolerance is 1 IU)
            // with everything and their parallel projection is < 1 IU, leading to bad distance calculations
            if( sp->GetLength() > 2 && sn->GetLength() > 2 && ( sp->GetCenter() - sn->GetCenter() ).SquaredEuclideanNorm() < 4 )
            {
                DIFF_PAIR_COUPLED_SEGMENTS cpair;
                cpair.isArc = true;
                bool coupled = commonParallelProjection( *sp, *sn, cpair.coupledArcP, cpair.coupledArcN );

                if( coupled )
                {
                    cpair.parentP = sp;
                    cpair.parentN = sn;
                    cpair.layer = sp->GetLayer();
                    cpair.computedGap = KiROUND( std::abs( cpair.coupledArcP.GetRadius()
                                                 - cpair.coupledArcN.GetRadius() ) );
                    cpair.computedGap -= ( sp->GetWidth() + sn->GetWidth() ) / 2;

                    if( cpair.computedGap < bestGap )
                    {
                        bestGap = cpair.computedGap;
                        bestCoupled = cpair;
                    }
                }

            }
        }

        if( bestCoupled )
        {
            auto excludeSelf =
                    [&] ( BOARD_ITEM *aItem )
                    {
                        if( aItem == bestCoupled->parentN || aItem == bestCoupled->parentP )
                        {
                            return false;
                        }

                        if( aItem->Type() == PCB_TRACE_T || aItem->Type() == PCB_VIA_T || aItem->Type() == PCB_ARC_T )
                        {
                            auto bci = static_cast<BOARD_CONNECTED_ITEM*>( aItem );

                            if( bci->GetNetCode() == bestCoupled->parentN->GetNetCode()
                            ||  bci->GetNetCode() == bestCoupled->parentP->GetNetCode() )
                                return false;
                        }

                        return true;
                    };

            SHAPE_SEGMENT checkSegStart( bestCoupled->coupledP.A, bestCoupled->coupledN.A );
            SHAPE_SEGMENT checkSegEnd( bestCoupled->coupledP.B, bestCoupled->coupledN.B );
            DRC_RTREE*    tree = bestCoupled->parentP->GetBoard()->m_CopperItemRTreeCache.get();

            // check if there's anything in between the segments suspected to be coupled. If
            // there's nothing, assume they are really coupled.

            if( !tree->CheckColliding( &checkSegStart, sp->GetLayer(), 0, excludeSelf )
                  && !tree->CheckColliding( &checkSegEnd, sp->GetLayer(), 0, excludeSelf ) )
            {
                aDp.coupled.push_back( *bestCoupled );
            }
        }
    }
}



bool test::DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING::Run()
{
    m_board = m_drcEngine->GetBoard();

    int epsilon = m_board->GetDesignSettings().GetDRCEpsilon();

    std::map<DIFF_PAIR_KEY, DIFF_PAIR_ITEMS> dpRuleMatches;

    auto evaluateDpConstraints =
            [&]( BOARD_ITEM *item ) -> bool
            {
                DIFF_PAIR_KEY         key;
                BOARD_CONNECTED_ITEM* citem = static_cast<BOARD_CONNECTED_ITEM*>( item );
                NETINFO_ITEM*         refNet = citem->GetNet();

                if( refNet && DRC_ENGINE::IsNetADiffPair( m_board, refNet, key.netP, key.netN ) )
                {
                    drc_dbg( 10, wxT( "eval dp %p\n" ), item );

                    const DRC_CONSTRAINT_T constraintsToCheck[] = {
                            DIFF_PAIR_GAP_CONSTRAINT,
                            MAX_UNCOUPLED_CONSTRAINT
                    };

                    for( int i = 0; i < 2; i++ )
                    {
                        DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( constraintsToCheck[ i ],
                                                                            item, nullptr,
                                                                            item->GetLayer() );

                        if( constraint.IsNull() || constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
                            continue;

                        drc_dbg( 10, wxT( "cns %d item %p\n" ), constraintsToCheck[i], item );

                        DRC_RULE* parentRule = constraint.GetParentRule();
                        wxString ruleName = parentRule ? parentRule->m_Name : constraint.GetName();

                        switch( constraintsToCheck[i] )
                        {
                        case DIFF_PAIR_GAP_CONSTRAINT:
                            key.gapConstraint = constraint.GetValue();
                            key.gapRule = parentRule;
                            key.gapRuleName = ruleName;
                            break;

                        case MAX_UNCOUPLED_CONSTRAINT:
                            key.uncoupledConstraint = constraint.GetValue();
                            key.uncoupledRule = parentRule;
                            key.uncoupledRuleName = ruleName;
                            break;

                        default:
                            break;
                        }

                        if( refNet->GetNetCode() == key.netN )
                            dpRuleMatches[key].itemsN.insert( citem );
                        else
                            dpRuleMatches[key].itemsP.insert( citem );
                    }
                }

                return true;
            };

    m_board->GetConnectivity()->GetFromToCache()->Rebuild( m_board );

    forEachGeometryItem( { PCB_TRACE_T, PCB_VIA_T, PCB_ARC_T }, LSET::AllCuMask(),
                         evaluateDpConstraints );

    drc_dbg( 10, wxT( "dp rule matches %d\n" ), (int) dpRuleMatches.size() );

    reportAux( wxT( "DPs evaluated:" ) );

    for( auto& [ key, itemSet ] : dpRuleMatches )
    {
        NETINFO_ITEM *niP = m_board->GetNetInfo().GetNetItem( key.netP );
        NETINFO_ITEM *niN = m_board->GetNetInfo().GetNetItem( key.netN );

        assert( niP );
        assert( niN );

        wxString nameP = niP->GetNetname();
        wxString nameN = niN->GetNetname();

        reportAux( wxString::Format( wxT( "Rule '%s', DP: (+) %s - (-) %s" ),
                                     key.gapRuleName, nameP, nameN ) );

        extractDiffPairCoupledItems( itemSet );

        itemSet.totalCoupled = 0;
        itemSet.totalLengthN = 0;
        itemSet.totalLengthP = 0;

        drc_dbg(10, wxT( "       coupled prims : %d\n" ), (int) itemSet.coupled.size() );

        for( BOARD_CONNECTED_ITEM* item : itemSet.itemsN )
        {
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
                itemSet.totalLengthN += track->GetLength();
        }

        for( BOARD_CONNECTED_ITEM* item : itemSet.itemsP )
        {
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
                itemSet.totalLengthP += track->GetLength();
        }

        for( DIFF_PAIR_COUPLED_SEGMENTS& dp : itemSet.coupled )
        {
            int length = dp.coupledN.Length();

            wxCHECK2( dp.parentN && dp.parentP, continue );

            std::shared_ptr<KIGFX::VIEW_OVERLAY> overlay = m_drcEngine->GetDebugOverlay();

            if( overlay )
            {
                overlay->SetIsFill(false);
                overlay->SetIsStroke(true);
                overlay->SetStrokeColor( RED );
                overlay->SetLineWidth( 100000 );
                overlay->Line( dp.coupledP );
                overlay->SetStrokeColor( BLUE );
                overlay->Line( dp.coupledN );
            }

            drc_dbg( 10, wxT( "               len %d gap %d l %d\n" ),
                     length,
                     dp.computedGap,
                     dp.parentP->GetLayer() );

            if( key.gapConstraint )
            {
                if( key.gapConstraint->HasMin()
                    && key.gapConstraint->Min() >= 0
                    && ( dp.computedGap < key.gapConstraint->Min() - epsilon ) )
                {
                    dp.couplingFailMin = true;
                }

                if( key.gapConstraint->HasMax()
                    && key.gapConstraint->Max() >= 0
                    && ( dp.computedGap > key.gapConstraint->Max() + epsilon ) )
                {
                    dp.couplingFailMax = true;
                }
            }

            if( !dp.couplingFailMin && !dp.couplingFailMax )
                itemSet.totalCoupled += length;
        }

        int totalLen = std::max( itemSet.totalLengthN, itemSet.totalLengthP );
        reportAux( wxString::Format( wxT( "   - coupled length: %s, total length: %s" ),
                                     MessageTextFromValue( itemSet.totalCoupled ),
                                     MessageTextFromValue( totalLen ) ) );

        int totalUncoupled = totalLen - itemSet.totalCoupled;

        bool uncoupledViolation = false;

        if( key.uncoupledConstraint && ( !itemSet.itemsP.empty() || !itemSet.itemsN.empty() ) )
        {
            const MINOPTMAX<int>& val = *key.uncoupledConstraint;

            if( val.HasMax() && val.Max() >= 0 && totalUncoupled > val.Max() )
            {
                auto     drce = DRC_ITEM::Create( DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG );
                wxString msg = formatMsg( _( "(%s maximum uncoupled length %s; actual %s)" ),
                                          key.uncoupledRuleName,
                                          val.Max(),
                                          totalUncoupled );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );

                BOARD_CONNECTED_ITEM* item = nullptr;
                auto                  p_it = itemSet.itemsP.begin();
                auto                  n_it = itemSet.itemsN.begin();

                if( p_it != itemSet.itemsP.end() )
                {
                    item = *p_it;
                    drce->AddItem( *p_it );
                    p_it++;
                }

                if( n_it != itemSet.itemsN.end() )
                {
                    item = *n_it;
                    drce->AddItem( *n_it );
                    n_it++;
                }

                while( p_it != itemSet.itemsP.end() )
                    drce->AddItem( *p_it++ );

                while( n_it != itemSet.itemsN.end() )
                    drce->AddItem( *n_it++ );

                uncoupledViolation = true;

                drce->SetViolatingRule( key.uncoupledRule );

                reportViolation( drce, item->GetPosition(), item->GetLayer() );
            }
        }

        if( key.gapConstraint && ( uncoupledViolation || !key.uncoupledConstraint ) )
        {
            for( DIFF_PAIR_COUPLED_SEGMENTS& dp : itemSet.coupled )
            {
                wxCHECK2( dp.parentP && dp.parentN, continue );

                if( ( dp.couplingFailMin || dp.couplingFailMax ) )
                {
                    // We have a candidate violation, now we need to re-query for a constraint
                    // given the actual items, because there may be a location-based rule in play.
                    DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( DIFF_PAIR_GAP_CONSTRAINT,
                                                                        dp.parentP, dp.parentN,
                                                                        dp.parentP->GetLayer() );
                    MINOPTMAX<int> val = constraint.GetValue();

                    if( !val.HasMin() || val.Min() < 0 || dp.computedGap >= val.Min() )
                        dp.couplingFailMin = false;

                    if( !val.HasMax() || val.Max() < 0 || dp.computedGap <= val.Max() )
                        dp.couplingFailMax = false;

                    if( !dp.couplingFailMin && !dp.couplingFailMax )
                        continue;

                    auto     drcItem = DRC_ITEM::Create( DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE );
                    wxString msg;

                    if( dp.couplingFailMin )
                    {
                        msg = formatMsg( _( "(%s minimum gap %s; actual %s)" ),
                                         key.gapRuleName,
                                         val.Min(),
                                         dp.computedGap );
                    }
                    else if( dp.couplingFailMax )
                    {
                        msg = formatMsg( _( "(%s maximum gap %s; actual %s)" ),
                                         key.gapRuleName,
                                         val.Max(),
                                         dp.computedGap );
                    }

                    drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + msg );

                    BOARD_CONNECTED_ITEM* item = nullptr;

                    if( dp.parentP )
                    {
                        item = dp.parentP;
                        drcItem->AddItem( dp.parentP );
                    }

                    if( dp.parentN )
                    {
                        item = dp.parentN;
                        drcItem->AddItem( dp.parentN );
                    }

                    drcItem->SetViolatingRule( key.gapRule );

                    reportViolation( drcItem, item->GetPosition(), item->GetLayer() );
                }
            }
        }
    }

    reportRuleStatistics();

    return true;
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING> dummy;
}

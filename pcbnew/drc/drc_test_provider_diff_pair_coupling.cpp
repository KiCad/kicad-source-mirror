/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2022 KiCad Developers.
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


#include <common.h>
#include <board.h>
#include <pcb_track.h>

#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_length_report.h>
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
            else
                return parentRule < b.parentRule;
        }
    }

    int       netP, netN;
    DRC_RULE* parentRule;
};

struct DIFF_PAIR_COUPLED_SEGMENTS
{
    SEG          coupledN;
    SEG          coupledP;
    PCB_TRACK*   parentN;
    PCB_TRACK*   parentP;
    int          computedGap;
    PCB_LAYER_ID layer;
    bool         couplingOK;

    DIFF_PAIR_COUPLED_SEGMENTS() :
        parentN( nullptr ),
        parentP( nullptr ),
        computedGap( 0 ),
        layer( UNDEFINED_LAYER ),
        couplingOK( false )
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

                    int gap = (cpair.coupledP.A - cpair.coupledN.A).EuclideanNorm();
                    if( gap < bestGap )
                    {
                        bestGap = gap;
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

                        if( aItem->Type() == PCB_TRACE_T || aItem->Type() == PCB_VIA_T )
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



    std::map<DIFF_PAIR_KEY, DIFF_PAIR_ITEMS> dpRuleMatches;

    auto evaluateDpConstraints =
            [&]( BOARD_ITEM *item ) -> bool
            {
                DIFF_PAIR_KEY key;
                BOARD_CONNECTED_ITEM* citem = static_cast<BOARD_CONNECTED_ITEM*>( item );
                NETINFO_ITEM* refNet = citem->GetNet();

                if( refNet && DRC_ENGINE::IsNetADiffPair( m_board, refNet, key.netP, key.netN ) )
                {
                    drc_dbg( 10, wxT( "eval dp %p\n" ), item );

                    const DRC_CONSTRAINT_T constraintsToCheck[] = {
                            DIFF_PAIR_GAP_CONSTRAINT,
                            DIFF_PAIR_MAX_UNCOUPLED_CONSTRAINT
                    };

                    for( int i = 0; i < 2; i++ )
                    {
                        auto constraint = m_drcEngine->EvalRules( constraintsToCheck[ i ], item,
                                                                  nullptr, item->GetLayer() );

                        if( constraint.IsNull() || constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
                            continue;

                        drc_dbg( 10, wxT( "cns %d item %p\n" ), constraintsToCheck[i], item );

                        key.parentRule = constraint.GetParentRule();

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
                                     key.parentRule->m_Name,
                                     nameP,
                                     nameN ) );

        extractDiffPairCoupledItems( itemSet );

        itemSet.totalCoupled = 0;
        itemSet.totalLengthN = 0;
        itemSet.totalLengthP = 0;

        drc_dbg(10, wxT( "       coupled prims : %d\n" ), (int) itemSet.coupled.size() );

        std::optional<DRC_CONSTRAINT> gapConstraint
                            = key.parentRule->FindConstraint( DIFF_PAIR_GAP_CONSTRAINT );

        std::optional<DRC_CONSTRAINT> maxUncoupledConstraint
                            = key.parentRule->FindConstraint( DIFF_PAIR_MAX_UNCOUPLED_CONSTRAINT );

        for( BOARD_CONNECTED_ITEM* item : itemSet.itemsN )
        {
            // fixme: include vias
            if( PCB_TRACK* track = dyn_cast<PCB_TRACK*>( item ) )
                itemSet.totalLengthN += track->GetLength();
        }

        for( BOARD_CONNECTED_ITEM* item : itemSet.itemsP )
        {
            // fixme: include vias
            if( PCB_TRACK* track = dyn_cast<PCB_TRACK*>( item ) )
                itemSet.totalLengthP += track->GetLength();
        }

        for( DIFF_PAIR_COUPLED_SEGMENTS& dp : itemSet.coupled )
        {
            int length = dp.coupledN.Length();
            int gap = dp.coupledN.Distance( dp.coupledP );

            gap -= dp.parentN->GetWidth() / 2;
            gap -= dp.parentP->GetWidth() / 2;

            dp.computedGap = gap;

            auto overlay = m_drcEngine->GetDebugOverlay();

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
                     gap,
                     dp.parentP->GetLayer() );

            if( gapConstraint )
            {
                auto val = gapConstraint->GetValue();
                bool insideRange = true;

                if( val.HasMin() && gap < val.Min() )
                    insideRange = false;

                if( val.HasMax() && gap > val.Max() )
                    insideRange = false;

                dp.couplingOK = insideRange;
            }
            else
            {
                dp.couplingOK = true;
            }

            if( dp.couplingOK )
                itemSet.totalCoupled += length;
        }

        int totalLen = std::max( itemSet.totalLengthN, itemSet.totalLengthP );
        reportAux( wxString::Format( wxT( "   - coupled length: %s, total length: %s" ),
                                     MessageTextFromValue( userUnits(), itemSet.totalCoupled ),
                                     MessageTextFromValue( userUnits(), totalLen ) ) );

        int totalUncoupled = totalLen - itemSet.totalCoupled;

        bool uncoupledViolation = false;

        if( maxUncoupledConstraint && ( !itemSet.itemsP.empty() || ! itemSet.itemsN.empty() ) )
        {
            const MINOPTMAX<int>& val = maxUncoupledConstraint->GetValue();

            if ( val.HasMax() && totalUncoupled > val.Max() )
            {
                auto     drce = DRC_ITEM::Create( DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG );
                wxString msg;

                msg = wxString::Format( _( "(%s maximum uncoupled length: %s; actual: %s)" ),
                                          maxUncoupledConstraint->GetParentRule()->m_Name,
                                          MessageTextFromValue( userUnits(), val.Max() ),
                                          MessageTextFromValue( userUnits(), totalUncoupled ) );

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

                drce->SetViolatingRule( maxUncoupledConstraint->GetParentRule() );

                reportViolation( drce, item->GetPosition(), item->GetLayer() );
            }
        }

        if ( gapConstraint && ( uncoupledViolation || !maxUncoupledConstraint ) )
        {
            for( DIFF_PAIR_COUPLED_SEGMENTS& dp : itemSet.coupled )
            {
                if( !dp.couplingOK && ( dp.parentP || dp.parentN ) )
                {
                    MINOPTMAX<int> val = gapConstraint->GetValue();
                    auto           drcItem = DRC_ITEM::Create( DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE );
                    wxString       msg;

                    msg = drcItem->GetErrorText() + wxT( " (" ) +
                            gapConstraint->GetParentRule()->m_Name + wxS( " " );

                    if( val.HasMin() )
                        msg += wxString::Format( _( "minimum gap: %s; " ),
                                                   MessageTextFromValue( userUnits(), val.Min() ) );

                    if( val.HasMax() )
                        msg += wxString::Format( _( "maximum gap: %s; " ),
                                                   MessageTextFromValue( userUnits(), val.Max() ) );

                    msg += wxString::Format( _( "actual: %s)" ),
                                             MessageTextFromValue( userUnits(), dp.computedGap ) );

                    drcItem->SetErrorMessage( msg );

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

                    drcItem->SetViolatingRule( gapConstraint->GetParentRule() );

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

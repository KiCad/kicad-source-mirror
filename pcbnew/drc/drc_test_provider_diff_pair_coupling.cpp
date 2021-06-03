/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers.
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
#include <track.h>

#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_length_report.h>
#include <drc/drc_rtree.h>

#include <geometry/shape_segment.h>

#include <connectivity/connectivity_data.h>
#include <connectivity/from_to_cache.h>


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
        return "diff_pair_coupling";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests differential pair coupling";
    }

    virtual int GetNumPhases() const override
    {
        return 1;
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

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
    TRACK*       parentN;
    TRACK*       parentP;
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

static void extractDiffPairCoupledItems( DIFF_PAIR_ITEMS& aDp, DRC_RTREE& aTree )
{
    for( BOARD_CONNECTED_ITEM* itemP : aDp.itemsP )
    {
        TRACK* sp = dyn_cast<TRACK*>( itemP );
        OPT<DIFF_PAIR_COUPLED_SEGMENTS> bestCoupled;
        int bestGap = std::numeric_limits<int>::max();

        if(!sp)
            continue;

        for ( BOARD_CONNECTED_ITEM* itemN : aDp.itemsN )
        {
            auto sn = dyn_cast<TRACK*> ( itemN );

            if(!sn)
                continue;

            if( ( sn->GetLayerSet() & sp->GetLayerSet() ).none() )
                continue;

            SEG ssp ( sp->GetStart(), sp->GetEnd() );
            SEG ssn ( sn->GetStart(), sn->GetEnd() );

            // Segments that are == 1 IU in length are approximately parallel with everything and their
            // parallel projection is < 1 IU, leading to bad distance calculations
            if( ssp.SquaredLength() > 1 && ssn.SquaredLength() > 1 && ssp.ApproxParallel(ssn) )
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

            // check if there's anyting in between the segments suspected to be coupled. If
            // there's nothing, assume they are really coupled.

            if( !aTree.CheckColliding( &checkSegStart, sp->GetLayer(), 0, excludeSelf )
                  && !aTree.CheckColliding( &checkSegEnd, sp->GetLayer(), 0, excludeSelf ) )
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
                    drc_dbg( 10, "eval dp %p\n", item );

                    const DRC_CONSTRAINT_T constraintsToCheck[] = {
                            DIFF_PAIR_GAP_CONSTRAINT,
                            DIFF_PAIR_MAX_UNCOUPLED_CONSTRAINT
                    };

                    for( int i = 0; i < 2; i++ )
                    {
                        auto constraint = m_drcEngine->EvalRules( constraintsToCheck[ i ], item,
                                                                  nullptr, item->GetLayer() );

                        if( constraint.IsNull() )
                            continue;

                        drc_dbg( 10, "cns %d item %p\n", constraintsToCheck[i], item );

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

    forEachGeometryItem( { PCB_TRACE_T, PCB_VIA_T, PCB_ARC_T },
                    LSET::AllCuMask(), evaluateDpConstraints );

    drc_dbg( 10, "dp rule matches %d\n", (int) dpRuleMatches.size() );


    DRC_RTREE copperTree;

    auto addToTree =
            [&copperTree]( BOARD_ITEM *item ) -> bool
            {
                for( PCB_LAYER_ID layer : item->GetLayerSet().Seq() )
                {
                    if( IsCopperLayer( layer ) )
                        copperTree.Insert( item, layer );
                }

                return true;
            };

    forEachGeometryItem( { PCB_TRACE_T, PCB_VIA_T, PCB_PAD_T, PCB_ZONE_T, PCB_ARC_T },
                         LSET::AllCuMask(), addToTree );


    reportAux( wxString::Format( _("DPs evaluated:") ) );

    for( auto& it : dpRuleMatches )
    {
        NETINFO_ITEM *niP = m_board->GetNetInfo().GetNetItem( it.first.netP );
        NETINFO_ITEM *niN = m_board->GetNetInfo().GetNetItem( it.first.netN );

        assert( niP );
        assert( niN );

        wxString nameP = niP->GetNetname();
        wxString nameN = niN->GetNetname();

        reportAux( wxString::Format( "Rule '%s', DP: (+) %s - (-) %s",
                                     it.first.parentRule->m_Name, nameP, nameN ) );

        extractDiffPairCoupledItems( it.second, copperTree );

        it.second.totalCoupled = 0;
        it.second.totalLengthN = 0;
        it.second.totalLengthP = 0;

        drc_dbg(10, "       coupled prims : %d\n", (int) it.second.coupled.size() );

        OPT<DRC_CONSTRAINT> gapConstraint =
                it.first.parentRule->FindConstraint( DIFF_PAIR_GAP_CONSTRAINT );
        OPT<DRC_CONSTRAINT> maxUncoupledConstraint =
                it.first.parentRule->FindConstraint( DIFF_PAIR_MAX_UNCOUPLED_CONSTRAINT );

        for( auto& item : it.second.itemsN )
        {
            // fixme: include vias
            if( auto track = dyn_cast<TRACK*>( item ) )
                it.second.totalLengthN += track->GetLength();
        }

        for( auto& item : it.second.itemsP )
        {
            // fixme: include vias
            if( auto track = dyn_cast<TRACK*>( item ) )
                it.second.totalLengthP += track->GetLength();
        }

        for( auto& cpair : it.second.coupled )
        {
            int length = cpair.coupledN.Length();
            int gap = cpair.coupledN.Distance( cpair.coupledP );

            gap -= cpair.parentN->GetWidth() / 2;
            gap -= cpair.parentP->GetWidth() / 2;

            cpair.computedGap = gap;

            auto overlay = m_drcEngine->GetDebugOverlay();

            if( overlay )
            {
                overlay->SetIsFill(false);
                overlay->SetIsStroke(true);
                overlay->SetStrokeColor( RED );
                overlay->SetLineWidth( 100000 );
                overlay->Line( cpair.coupledP );
                overlay->SetStrokeColor( BLUE );
                overlay->Line( cpair.coupledN );
            }

            drc_dbg( 10, "               len %d gap %d l %d\n", length, gap,
                     cpair.parentP->GetLayer() );

            if( gapConstraint )
            {
                auto val = gapConstraint->GetValue();
                bool insideRange = true;

                if( val.HasMin() && gap < val.Min() )
                    insideRange = false;

                if( val.HasMax() && gap > val.Max() )
                    insideRange = false;

//                if(val.HasMin() && val.HasMax() )
  //                  drc_dbg(10, "Vmin %d vmax %d\n", val.Min(), val.Max() );

                cpair.couplingOK = insideRange;
            }
            else
            {
                cpair.couplingOK = true;
            }

            if( cpair.couplingOK )
                it.second.totalCoupled += length;
        }

        int totalLen = std::max( it.second.totalLengthN, it.second.totalLengthP );
        reportAux( wxString::Format( "   - coupled length: %s, total length: %s",

                                     MessageTextFromValue( userUnits(), it.second.totalCoupled ),
                                     MessageTextFromValue( userUnits(), totalLen ) ) );

        int totalUncoupled = totalLen - it.second.totalCoupled;

        bool uncoupledViolation = false;

        if( maxUncoupledConstraint )
        {
            auto val = maxUncoupledConstraint->GetValue();

            if ( val.HasMax() && totalUncoupled > val.Max() )
            {
                auto drce = DRC_ITEM::Create( DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG );

                m_msg = wxString::Format( _( "(%s maximum uncoupled length: %s; actual: %s)" ),
                                          maxUncoupledConstraint->GetParentRule()->m_Name,
                                          MessageTextFromValue( userUnits(), val.Max() ),
                                          MessageTextFromValue( userUnits(), totalUncoupled ) );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );

                for( BOARD_CONNECTED_ITEM* offendingTrack : it.second.itemsP )
                    drce->AddItem( offendingTrack );

                for( BOARD_CONNECTED_ITEM* offendingTrack : it.second.itemsN )
                    drce->AddItem( offendingTrack );

                uncoupledViolation = true;

                drce->SetViolatingRule( maxUncoupledConstraint->GetParentRule() );

                reportViolation( drce, ( *it.second.itemsP.begin() )->GetPosition() );
            }
        }

        if ( gapConstraint && ( uncoupledViolation || !maxUncoupledConstraint ) )
        {
            for( auto& cpair : it.second.coupled )
            {
                if( !cpair.couplingOK )
                {
                    auto val = gapConstraint->GetValue();
                    auto drcItem = DRC_ITEM::Create( DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE );

                    m_msg = drcItem->GetErrorText() + " (" +
                            gapConstraint->GetParentRule()->m_Name + " ";

                    if( val.HasMin() )
                        m_msg += wxString::Format( _( "minimum gap: %s; " ),
                                                   MessageTextFromValue( userUnits(), val.Min() ) );

                    if( val.HasMax() )
                        m_msg += wxString::Format( _( "maximum gap: %s; " ),
                                                   MessageTextFromValue( userUnits(), val.Max() ) );

                    m_msg += wxString::Format( _( "actual: %s)" ),
                        MessageTextFromValue( userUnits(), cpair.computedGap ) );

                    drcItem->SetErrorMessage( m_msg );

                    drcItem->AddItem( cpair.parentP );
                    drcItem->AddItem( cpair.parentN );

                    drcItem->SetViolatingRule( gapConstraint->GetParentRule() );

                    reportViolation( drcItem, cpair.parentP->GetPosition() );
                }
            }
        }
    }

    reportRuleStatistics();

    return true;
}


std::set<DRC_CONSTRAINT_T> test::DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING::GetConstraintTypes() const
{
    return { DIFF_PAIR_GAP_CONSTRAINT, DIFF_PAIR_MAX_UNCOUPLED_CONSTRAINT };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING> dummy;
}

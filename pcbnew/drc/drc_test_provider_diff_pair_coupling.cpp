/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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
#include <pad.h>
#include <pcb_generator.h>
#include <drc/drc_item.h>
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
*/

namespace test {

class DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING () :
        m_board( nullptr )
    {}

    virtual ~DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "diff_pair_coupling" ); };

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
    bool p_is_ccw = p.IsCCW();
    bool n_is_ccw = n.IsCCW();

    // Quick check to ensure arcs are of similar size and close enough to be considered coupled
    double radiusDiffRatio = std::abs(p_radius - n_radius) / std::max(p_radius, n_radius);
    double centerDistance = (p_center - n_center).EuclideanNorm();

    if (radiusDiffRatio > 0.5 || centerDistance > std::max(p_radius, n_radius) * 0.5)
        return false;

    VECTOR2I p_start( p.GetStart() );
    VECTOR2I p_end( p.GetEnd() );

    if( !p_is_ccw )
        std::swap( p_start, p_end );

    VECTOR2I n_start( n.GetStart() );
    VECTOR2I n_end( n.GetEnd() );

    if( !n_is_ccw )
        std::swap( n_start, n_end );

    SHAPE_ARC p_arc( p_start, p.GetMid(), p_end, 0 );
    SHAPE_ARC n_arc( n_start, n.GetMid(), n_end, 0 );

    EDA_ANGLE p_start_angle = p_arc.GetStartAngle();

    // Rotate the arcs to a common 0 starting angle
    p_arc.Rotate( p_start_angle, p_center );
    n_arc.Rotate( p_start_angle, n_center );

    EDA_ANGLE p_end_angle = p_arc.GetEndAngle();
    EDA_ANGLE n_start_angle = n_arc.GetStartAngle();
    EDA_ANGLE n_end_angle = n_arc.GetEndAngle();

    // Determine overlap region
    EDA_ANGLE clip_start_angle, clip_end_angle;

    // No overlap when n starts after p ends or n ends before p starts
    if( n_start_angle >= p_end_angle || n_end_angle <= EDA_ANGLE(0) )
        return false;

    // Calculate the start and end angles of the overlap
    clip_start_angle = std::max( EDA_ANGLE(0), n_start_angle );
    clip_end_angle = std::min( p_end_angle, n_end_angle );

    // Calculate the total angle of the overlap
    EDA_ANGLE clip_total_angle = clip_end_angle - clip_start_angle;

    // Now we reset the angles.  However, note that the convention here for adding angles
    // is OPPOSITE the Rotate convention above.  So this undoes the rotation.
    clip_start_angle += p_start_angle;
    clip_end_angle += p_start_angle;
    clip_start_angle.Normalize();
    clip_end_angle.Normalize();

    // One arc starts approximately where the other ends or overlap is too small
    if( clip_total_angle <= EDA_ANGLE( ADVANCED_CFG::GetCfg().m_MinParallelAngle ) )
        return false;

    // For CCW arcs, we start at clip_start_angle and sweep through clip_total_angle
    // For CW arcs, we start at clip_end_angle and sweep through -clip_total_angle
    VECTOR2I p_clip_point, n_clip_point;

    if( p_is_ccw )
    {
        p_clip_point = p_center + KiROUND( p_radius * clip_start_angle.Cos(), p_radius * clip_start_angle.Sin() );
        pClip = SHAPE_ARC( p_center, p_clip_point, clip_total_angle );
    }
    else
    {
        p_clip_point = p_center + KiROUND( p_radius * clip_end_angle.Cos(), p_radius * clip_end_angle.Sin() );
        pClip = SHAPE_ARC( p_center, p_clip_point, -clip_total_angle );
    }

    if( n_is_ccw )
    {
        n_clip_point = n_center + KiROUND( n_radius * clip_start_angle.Cos(), n_radius * clip_start_angle.Sin() );
        nClip = SHAPE_ARC( n_center, n_clip_point, clip_total_angle );
    }
    else
    {
        n_clip_point = n_center + KiROUND( n_radius * clip_end_angle.Cos(), n_radius * clip_end_angle.Sin() );
        nClip = SHAPE_ARC( n_center, n_clip_point, -clip_total_angle );
    }

    // Ensure the resulting arcs are not degenerate
    if( pClip.GetLength() < 1.0 || nClip.GetLength() < 1.0 )
        return false;

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
    int64_t      computedGap;
    VECTOR2I     nearestN;
    VECTOR2I     nearestP;
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
    auto isInTuningPattern =
            []( BOARD_ITEM* track )
            {
                if( EDA_GROUP* parent = track->GetParentGroup() )
                {
                    if( PCB_GENERATOR* generator = dynamic_cast<PCB_GENERATOR*>( parent ) )
                    {
                        if( generator->GetGeneratorType() == wxS( "tuning_pattern" ) )
                            return true;
                    }
                }

                return false;
            };

    for( BOARD_CONNECTED_ITEM* itemP : aDp.itemsP )
    {
        if( !itemP || itemP->Type() != PCB_TRACE_T || isInTuningPattern( itemP ) )
            continue;

        PCB_TRACK* sp = static_cast<PCB_TRACK*>( itemP );
        std::vector<std::optional<DIFF_PAIR_COUPLED_SEGMENTS>> coupled_vec;

        for( BOARD_CONNECTED_ITEM* itemN : aDp.itemsN )
        {
            if( !itemN || itemN->Type() != PCB_TRACE_T || isInTuningPattern( itemN ) )
                continue;

            PCB_TRACK* sn = static_cast<PCB_TRACK*>( itemN );

            if( ( sn->GetLayerSet() & sp->GetLayerSet() ).none() )
                continue;

            SEG ssp( sp->GetStart(), sp->GetEnd() );
            SEG ssn( sn->GetStart(), sn->GetEnd() );

            // Segments that are ~ 1 IU in length per side are approximately parallel (tolerance is 1 IU)
            // with everything and their parallel projection is < 1 IU, leading to bad distance calculations
            if( ssp.SquaredLength() > 2 && ssn.SquaredLength() > 2 && !ssp.Intersect( ssn, false, true ) )
            {
                DIFF_PAIR_COUPLED_SEGMENTS cpair;
                bool coupled = commonParallelProjection( ssp, ssn, cpair.coupledP, cpair.coupledN );

                if( coupled )
                {
                    cpair.parentP = sp;
                    cpair.parentN = sn;
                    cpair.layer = sp->GetLayer();
                    cpair.coupledP.NearestPoints( cpair.coupledN, cpair.nearestP, cpair.nearestN,
                                                  cpair.computedGap );
                    cpair.computedGap = std::sqrt( cpair.computedGap );  // NearestPoints returns squared distance
                    cpair.computedGap -= ( sp->GetWidth() + sn->GetWidth() ) / 2;
                    coupled_vec.push_back( cpair );
                }

            }
        }

        for( const std::optional<DIFF_PAIR_COUPLED_SEGMENTS>& coupled : coupled_vec )
        {
            auto excludeSelf =
                    [&]( BOARD_ITEM* aItem )
                    {
                        if( aItem == coupled->parentN || aItem == coupled->parentP )
                            return false;

                        if( aItem->Type() == PCB_TRACE_T || aItem->Type() == PCB_VIA_T || aItem->Type() == PCB_ARC_T )
                        {
                            PCB_TRACK* bci = static_cast<PCB_TRACK*>( aItem );

                            // Directly connected items don't count
                            if( bci->HitTest( coupled->coupledN.A, 0 )
                                || bci->HitTest( coupled->coupledN.B, 0 )
                                || bci->HitTest( coupled->coupledP.A, 0 )
                                || bci->HitTest( coupled->coupledP.B, 0 ) )
                            {
                                return false;
                            }
                        }
                        else if( aItem->Type() == PCB_PAD_T )
                        {
                            PAD* pad = static_cast<PAD*>( aItem );

                            auto trackExitsPad = [&]( PCB_TRACK* track )
                            {
                                bool startIn = pad->HitTest( track->GetStart(), 0 );
                                bool endIn = pad->HitTest( track->GetEnd(), 0 );

                                return startIn ^ endIn;
                            };

                            if( trackExitsPad( static_cast<PCB_TRACK*>( coupled->parentP ) )
                                    || trackExitsPad( static_cast<PCB_TRACK*>( coupled->parentN ) ) )
                            {
                                return false;
                            }
                        }

                        return true;
                    };

            SHAPE_SEGMENT checkSeg( coupled->nearestN, coupled->nearestP );
            DRC_RTREE*    tree = coupled->parentP->GetBoard()->m_CopperItemRTreeCache.get();

            // check if there's anything in between the segments suspected to be coupled. If
            // there's nothing, assume they are really coupled.

            if( !tree->CheckColliding( &checkSeg, sp->GetLayer(), 0, excludeSelf ) )
                aDp.coupled.push_back( *coupled );
        }
    }

    for( BOARD_CONNECTED_ITEM* itemP : aDp.itemsP )
    {
        if( !itemP || itemP->Type() != PCB_ARC_T || isInTuningPattern( itemP ) )
            continue;

        PCB_ARC* sp = static_cast<PCB_ARC*>( itemP );
        std::vector<std::optional<DIFF_PAIR_COUPLED_SEGMENTS>> coupled_vec;

        for( BOARD_CONNECTED_ITEM* itemN : aDp.itemsN )
        {
            if( !itemN || itemN->Type() != PCB_ARC_T || isInTuningPattern( itemN ) )
                continue;

            PCB_ARC* sn = static_cast<PCB_ARC*>( itemN );

            if( ( sn->GetLayerSet() & sp->GetLayerSet() ).none() )
                continue;

            // Segments that are ~ 1 IU in length per side are approximately parallel (tolerance is 1 IU)
            // with everything and their parallel projection is < 1 IU, leading to bad distance calculations
            int64_t sqWidth = static_cast<int64_t>( sp->GetWidth() ) * sp->GetWidth();

            if( sp->GetLength() > 2
                    && sn->GetLength() > 2
                    && sp->GetCenter().SquaredDistance( sn->GetCenter() ) < sqWidth )
            {
                DIFF_PAIR_COUPLED_SEGMENTS cpair;
                cpair.isArc = true;
                bool coupled = commonParallelProjection( *sp, *sn, cpair.coupledArcP, cpair.coupledArcN );

                if( coupled )
                {
                    cpair.parentP = sp;
                    cpair.parentN = sn;
                    cpair.layer = sp->GetLayer();
                    cpair.coupledArcP.NearestPoints( cpair.coupledArcN, cpair.nearestP, cpair.nearestN,
                                                     cpair.computedGap );
                    cpair.computedGap = std::sqrt( cpair.computedGap );  // NearestPoints returns squared distance
                    cpair.computedGap -= ( sp->GetWidth() + sn->GetWidth() ) / 2;
                    coupled_vec.push_back( cpair );
                }
            }
        }

        for( const std::optional<DIFF_PAIR_COUPLED_SEGMENTS>& coupled : coupled_vec )
        {
            auto excludeSelf =
                    [&] ( BOARD_ITEM *aItem )
                    {
                        if( aItem == coupled->parentN || aItem == coupled->parentP )
                            return false;

                        if( aItem->Type() == PCB_TRACE_T || aItem->Type() == PCB_VIA_T || aItem->Type() == PCB_ARC_T )
                        {
                            BOARD_CONNECTED_ITEM* bci = static_cast<BOARD_CONNECTED_ITEM*>( aItem );

                            if( bci->GetNetCode() == coupled->parentN->GetNetCode()
                                    ||  bci->GetNetCode() == coupled->parentP->GetNetCode() )
                            {
                                return false;
                            }
                        }
                        else if( aItem->Type() == PCB_PAD_T )
                        {
                            PAD* pad = static_cast<PAD*>( aItem );

                            auto arcExitsPad = [&]( PCB_ARC* arc )
                            {
                                bool startIn = pad->HitTest( arc->GetStart(), 0 );
                                bool endIn = pad->HitTest( arc->GetEnd(), 0 );

                                return startIn ^ endIn;
                            };

                            if( arcExitsPad( static_cast<PCB_ARC*>( coupled->parentP ) )
                                    || arcExitsPad( static_cast<PCB_ARC*>( coupled->parentN ) ) )
                            {
                                return false;
                            }
                        }

                        return true;
                    };

            SHAPE_SEGMENT checkArcMid( coupled->coupledArcN.GetArcMid(), coupled->coupledArcP.GetArcMid() );
            DRC_RTREE*    tree = coupled->parentP->GetBoard()->m_CopperItemRTreeCache.get();

            // check if there's anything in between the segments suspected to be coupled. If
            // there's nothing, assume they are really coupled.

            if( !tree->CheckColliding( &checkArcMid, sp->GetLayer(), 0, excludeSelf ) )
                aDp.coupled.push_back( *coupled );
        }
    }
}



bool test::DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING::Run()
{
    m_board = m_drcEngine->GetBoard();

    int  epsilon = m_board->GetDesignSettings().GetDRCEpsilon();
    LSET boardCopperLayers = LSET::AllCuMask( m_board->GetCopperLayerCount() );

    std::map<DIFF_PAIR_KEY, DIFF_PAIR_ITEMS> dpRuleMatches;

    auto evaluateDpConstraints =
            [&]( BOARD_ITEM *item ) -> bool
            {
                DIFF_PAIR_KEY         key;
                BOARD_CONNECTED_ITEM* citem = static_cast<BOARD_CONNECTED_ITEM*>( item );
                NETINFO_ITEM*         refNet = citem->GetNet();

                if( refNet && DRC_ENGINE::IsNetADiffPair( m_board, refNet, key.netP, key.netN ) )
                {
                    // drc_dbg( 10, wxT( "eval dp %p\n" ), item );

                    for( DRC_CONSTRAINT_T constraintType : { DIFF_PAIR_GAP_CONSTRAINT,
                                                             MAX_UNCOUPLED_CONSTRAINT } )
                    {
                        DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( constraintType,
                                                                            item, nullptr,
                                                                            item->GetLayer() );

                        if( constraint.IsNull() || constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
                            continue;

                        // drc_dbg( 10, wxT( "cns %d item %p\n" ), (int) constraintType, item );

                        DRC_RULE* parentRule = constraint.GetParentRule();
                        wxString ruleName = parentRule ? parentRule->m_Name : constraint.GetName();

                        switch( constraintType )
                        {
                        case DIFF_PAIR_GAP_CONSTRAINT:
                            key.gapConstraint = constraint.GetValue();
                            key.gapRule = parentRule;
                            key.gapRuleName = std::move( ruleName );
                            break;

                        case MAX_UNCOUPLED_CONSTRAINT:
                            key.uncoupledConstraint = constraint.GetValue();
                            key.uncoupledRule = parentRule;
                            key.uncoupledRuleName = std::move( ruleName );
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

    forEachGeometryItem( { PCB_TRACE_T, PCB_VIA_T, PCB_ARC_T }, boardCopperLayers, evaluateDpConstraints );

    // drc_dbg( 10, wxT( "dp rule matches %d\n" ), (int) dpRuleMatches.size() );

    REPORT_AUX( wxT( "DPs evaluated:" ) );

    for( auto& [ key, itemSet ] : dpRuleMatches )
    {
        NETINFO_ITEM *niP = m_board->GetNetInfo().GetNetItem( key.netP );
        NETINFO_ITEM *niN = m_board->GetNetInfo().GetNetItem( key.netN );

        assert( niP );
        assert( niN );

        wxString nameP = niP->GetNetname();
        wxString nameN = niN->GetNetname();

        REPORT_AUX( wxString::Format( wxT( "Rule '%s', DP: (+) %s - (-) %s" ),
                                      key.gapRuleName, nameP, nameN ) );

        extractDiffPairCoupledItems( itemSet );

        itemSet.totalCoupled = 0;
        itemSet.totalLengthN = 0;
        itemSet.totalLengthP = 0;

        // drc_dbg( 10, wxT( "       coupled prims : %d\n" ), (int) itemSet.coupled.size() );

        std::set<BOARD_CONNECTED_ITEM*> allItems;

        for( BOARD_CONNECTED_ITEM* item : itemSet.itemsN )
        {
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
            {
                if( allItems.insert( item ).second)
                    itemSet.totalLengthN += track->GetLength();
            }
        }

        for( BOARD_CONNECTED_ITEM* item : itemSet.itemsP )
        {
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
            {
                if( allItems.insert( item ).second)
                    itemSet.totalLengthP += track->GetLength();
            }
        }

        for( DIFF_PAIR_COUPLED_SEGMENTS& dp : itemSet.coupled )
        {
            int length = dp.isArc ? dp.coupledArcN.GetLength() : dp.coupledN.Length();
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

            // drc_dbg( 10, wxT( "               len %d gap %ld l %d\n" ),
            //          length,
            //          (long int) dp.computedGap,
            //          (int) dp.parentP->GetLayer() );

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

        REPORT_AUX( wxString::Format( wxT( "   - coupled length: %s, total length: %s" ),
                                      MessageTextFromValue( itemSet.totalCoupled ),
                                      MessageTextFromValue( totalLen ) ) );

        int  totalUncoupled = totalLen - itemSet.totalCoupled;
        bool uncoupledViolation = false;

        if( key.uncoupledConstraint && ( !itemSet.itemsP.empty() || !itemSet.itemsN.empty() ) )
        {
            const MINOPTMAX<int>& val = *key.uncoupledConstraint;

            if( val.HasMax() && val.Max() >= 0 && totalUncoupled > val.Max() )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG );
                drce->SetErrorDetail( formatMsg( _( "(%s maximum uncoupled length %s; actual %s)" ),
                                                 key.uncoupledRuleName,
                                                 val.Max(),
                                                 totalUncoupled ) );

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

                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE );

                    if( dp.couplingFailMin )
                    {
                        drcItem->SetErrorDetail( formatMsg( _( "(%s minimum gap %s; actual %s)" ),
                                                            key.gapRuleName,
                                                            val.Min(),
                                                            (double) dp.computedGap ) );
                    }
                    else if( dp.couplingFailMax )
                    {
                        drcItem->SetErrorDetail( formatMsg( _( "(%s maximum gap %s; actual %s)" ),
                                                            key.gapRuleName,
                                                            val.Max(),
                                                            (double) dp.computedGap ) );
                    }

                    drcItem->SetViolatingRule( key.gapRule );

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

                    if( item )
                        reportViolation( drcItem, item->GetFocusPosition(), item->GetLayer() );
                }
            }
        }
    }

    return true;
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_DIFF_PAIR_COUPLING> dummy;
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <common.h>
#include <board.h>
#include <pcb_board_outline.h>
#include <pcb_track.h>
#include <geometry/shape_segment.h>
#include <geometry/seg.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_rtree.h>
#include <board_design_settings.h>

/*
    Silk to silk clearance test. Check all silkscreen features against each other.
    Errors generated:
    - DRCE_SILK_CLEARANCE

*/

class DRC_TEST_PROVIDER_SILK_CLEARANCE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_SILK_CLEARANCE ():
        m_board( nullptr ),
        m_largestClearance( 0 )
    {}

    virtual ~DRC_TEST_PROVIDER_SILK_CLEARANCE() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "silk_clearance" ); };

private:

    BOARD* m_board;
    int m_largestClearance;
};


bool DRC_TEST_PROVIDER_SILK_CLEARANCE::Run()
{
    const int progressDelta = 500;

    m_board = m_drcEngine->GetBoard();

    // If the soldermask min width is greater than 0 then we must use a healing algorithm to generate
    // a whole-board soldermask poly, and then test against that.  However, that can't deal well with
    // DRC exclusions (as any change anywhere on the board that affects the soldermask will null the
    // associated exclusions), so we only use that when soldermask min width is > 0.
    bool checkIndividualMaskItems = m_board->GetDesignSettings().m_SolderMaskMinWidth <= 0;

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_CLEARANCE )
            && m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_MASK_CLEARANCE) )
    {
        return true;    // continue with other tests
    }

    DRC_CONSTRAINT worstClearanceConstraint;
    m_largestClearance = 0;

    if( m_drcEngine->QueryWorstConstraint( SILK_CLEARANCE_CONSTRAINT, worstClearanceConstraint ) )
        m_largestClearance = worstClearanceConstraint.m_Value.Min();

    if( !reportPhase( _( "Checking silkscreen for overlapping items..." ) ) )
        return false;   // DRC cancelled

    DRC_RTREE silkTree;
    DRC_RTREE targetTree;
    int       ii = 0;
    int       items = 0;
    LSET      silkLayers = LSET( { F_SilkS, B_SilkS } );
    LSET      targetLayers = LSET::FrontMask() | LSET::BackMask() | LSET( { Edge_Cuts, Margin } );

    auto countItems =
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++items;
                return true;
            };

    auto addToSilkTree =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !reportProgress( ii++, items, progressDelta ) )
                    return false;

                for( PCB_LAYER_ID layer : { F_SilkS, B_SilkS } )
                {
                    if( item->IsOnLayer( layer ) )
                        silkTree.Insert( item, layer, 0, ATOMIC_TABLES );
                }

                return true;
            };

    auto addToTargetTree =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !reportProgress( ii++, items, progressDelta ) )
                    return false;

                for( PCB_LAYER_ID layer : LSET( item->GetLayerSet() & targetLayers ) )
                    targetTree.Insert( item, layer, 0, ATOMIC_TABLES );

                return true;
            };

    forEachGeometryItem( s_allBasicItems, silkLayers, countItems );
    forEachGeometryItem( s_allBasicItems, targetLayers, countItems );

    forEachGeometryItem( s_allBasicItems, silkLayers, addToSilkTree );
    forEachGeometryItem( s_allBasicItems, targetLayers, addToTargetTree );

    REPORT_AUX( wxString::Format( wxT( "Testing %d silkscreen features against %d board items." ),
                                  silkTree.size(),
                                  targetTree.size() ) );

    const std::vector<DRC_RTREE::LAYER_PAIR> layerPairs =
    {
        DRC_RTREE::LAYER_PAIR( F_SilkS, F_SilkS ),
        DRC_RTREE::LAYER_PAIR( F_SilkS, F_Mask ),
        DRC_RTREE::LAYER_PAIR( F_SilkS, F_Adhes ),
        DRC_RTREE::LAYER_PAIR( F_SilkS, F_Paste ),
        DRC_RTREE::LAYER_PAIR( F_SilkS, F_CrtYd ),
        DRC_RTREE::LAYER_PAIR( F_SilkS, F_Fab ),
        DRC_RTREE::LAYER_PAIR( F_SilkS, F_Cu ),
        DRC_RTREE::LAYER_PAIR( F_SilkS, Edge_Cuts ),
        DRC_RTREE::LAYER_PAIR( F_SilkS, Margin ),
        DRC_RTREE::LAYER_PAIR( B_SilkS, B_SilkS ),
        DRC_RTREE::LAYER_PAIR( B_SilkS, B_Mask ),
        DRC_RTREE::LAYER_PAIR( B_SilkS, B_Adhes ),
        DRC_RTREE::LAYER_PAIR( B_SilkS, B_Paste ),
        DRC_RTREE::LAYER_PAIR( B_SilkS, B_CrtYd ),
        DRC_RTREE::LAYER_PAIR( B_SilkS, B_Fab ),
        DRC_RTREE::LAYER_PAIR( B_SilkS, B_Cu ),
        DRC_RTREE::LAYER_PAIR( B_SilkS, Edge_Cuts ),
        DRC_RTREE::LAYER_PAIR( B_SilkS, Margin )
    };

    targetTree.QueryCollidingPairs( &silkTree, layerPairs,
            [&]( const DRC_RTREE::LAYER_PAIR& aLayers, DRC_RTREE::ITEM_WITH_SHAPE* aRefItemShape,
                 DRC_RTREE::ITEM_WITH_SHAPE* aTestItemShape, bool* aCollisionDetected ) -> bool
            {
                BOARD_ITEM*  refItem = aRefItemShape->parent;
                const SHAPE* refShape = aRefItemShape->shape;
                BOARD_ITEM*  testItem = aTestItemShape->parent;
                const SHAPE* testShape = aTestItemShape->shape;

                std::shared_ptr<SHAPE> hole;

                if( m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_CLEARANCE )
                        && m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_MASK_CLEARANCE ) )
                {
                    return false;
                }

                if( isInvisibleText( refItem ) || isInvisibleText( testItem ) )
                    return true;

                if( testItem->IsTented( aLayers.first ) )
                {
                    if( testItem->HasHole() )
                    {
                        hole = testItem->GetEffectiveHoleShape();
                        testShape = hole.get();
                    }
                    else
                    {
                        return true;
                    }
                }

                if( PCB_BOARD_OUTLINE* boardOutline = m_board->BoardOutline() )
                {
                    if( !testItem->GetBoundingBox().Intersects( boardOutline->GetOutline().BBoxFromCaches() ) )
                        return true;

                    if( !testShape->Collide( &boardOutline->GetOutline() ) )
                        return true;
                }

                int            errorCode = DRCE_SILK_CLEARANCE;
                DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( SILK_CLEARANCE_CONSTRAINT,
                                                                    refItem, testItem, aLayers.second );
                int            minClearance = -1;

                if( !constraint.IsNull() && constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
                    minClearance = constraint.GetValue().Min();

                if( aLayers.second == F_Mask || aLayers.second == B_Mask )
                {
                    if( checkIndividualMaskItems )
                        minClearance = std::max( minClearance, 0 );

                    errorCode = DRCE_SILK_MASK_CLEARANCE;
                }

                if( minClearance < 0 || m_drcEngine->IsErrorLimitExceeded( errorCode ) )
                    return true;

                int      actual;
                VECTOR2I pos;

                // Graphics are often compound shapes so ignore collisions between shapes in a
                // single footprint or on the board (both parent footprints will be nullptr).
                if( refItem->Type() == PCB_SHAPE_T && testItem->Type() == PCB_SHAPE_T
                         && refItem->GetParentFootprint() == testItem->GetParentFootprint() )
                {
                    return true;
                }

                // Collide (and generate violations) based on a well-defined order so that
                // exclusion checking against previously-generated violations will work.
                if( aLayers.first == aLayers.second )
                {
                    if( refItem->m_Uuid > testItem->m_Uuid )
                    {
                        std::swap( refItem, testItem );
                        std::swap( refShape, testShape );
                    }
                }

                if( refShape->Collide( testShape, minClearance, &actual, &pos ) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( errorCode );

                    if( minClearance > 0 )
                    {
                        drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                            constraint.GetParentRule()->m_Name,
                                                            minClearance,
                                                            actual ) );
                    }

                    drcItem->SetItems( refItem, testItem );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );
                    reportTwoShapeGeometry( drcItem, pos, refShape, testShape, aLayers.second, actual );
                    *aCollisionDetected = true;
                }

                return true;
            },
            m_largestClearance,
            [&]( int aCount, int aSize ) -> bool
            {
                return reportProgress( aCount, aSize, progressDelta );
            } );

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_SILK_CLEARANCE> dummy;
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers.
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

#include <geometry/seg.h>
#include <geometry/shape_segment.h>

#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>

#include <drc/drc_rtree.h>

/*
    Silk to pads clearance test. Check all pads against silkscreen (mask opening in the pad vs silkscreen)
    Errors generated:
    - DRCE_SILK_MASK_CLEARANCE
*/

class DRC_TEST_PROVIDER_SILK_TO_MASK : public ::DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_SILK_TO_MASK ():
            m_board( nullptr ),
            m_largestClearance( 0 )
    {
    }

    virtual ~DRC_TEST_PROVIDER_SILK_TO_MASK()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "silk_to_mask";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests for silkscreen being clipped by solder mask";
    }

    virtual int GetNumPhases() const override
    {
        return 1;
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

private:

    BOARD* m_board;
    int m_largestClearance;
};


bool DRC_TEST_PROVIDER_SILK_TO_MASK::Run()
{
    m_board = m_drcEngine->GetBoard();

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_MASK_CLEARANCE ) )
    {
        reportAux( "Silkscreen clipping violations ignored. Tests not run." );
        return true;    // continue with other tests
    }

    DRC_CONSTRAINT worstClearanceConstraint;
    m_largestClearance = 0;

    if( m_drcEngine->QueryWorstConstraint( SILK_CLEARANCE_CONSTRAINT, worstClearanceConstraint ) )
        m_largestClearance = worstClearanceConstraint.m_Value.Min();

    reportAux( "Worst clearance : %d nm", m_largestClearance );

    if( !reportPhase( _( "Checking silkscreen for potential soldermask clipping..." ) ) )
        return false;   // DRC cancelled

    DRC_RTREE maskTree, silkTree;

    auto addMaskToTree =
            [&maskTree]( BOARD_ITEM *item ) -> bool
            {
                for( PCB_LAYER_ID layer : { F_Mask, B_Mask } )
                {
                    if( item->IsOnLayer( layer ) )
                        maskTree.Insert( item, layer );
                }

                return true;
            };

    auto addSilkToTree =
            [&silkTree]( BOARD_ITEM *item ) -> bool
            {
                for( PCB_LAYER_ID layer : { F_SilkS, B_SilkS } )
                {
                    if( item->IsOnLayer( layer ) )
                        silkTree.Insert( item, layer );
                }

                return true;
            };

    auto checkClearance =
            [&]( const DRC_RTREE::LAYER_PAIR& aLayers, DRC_RTREE::ITEM_WITH_SHAPE* aRefItem,
                 DRC_RTREE::ITEM_WITH_SHAPE* aTestItem, bool* aCollisionDetected ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_MASK_CLEARANCE ) )
                    return false;

                if( isInvisibleText( aRefItem->parent ) )
                    return true;

                if( isInvisibleText( aTestItem->parent ) )
                    return true;

                auto constraint = m_drcEngine->EvalRules( SILK_CLEARANCE_CONSTRAINT,
                                                          aRefItem->parent, aTestItem->parent,
                                                          aLayers.first );

                int minClearance = constraint.GetValue().Min();

                if( minClearance < 0 )
                    return true;

                int      actual;
                VECTOR2I pos;

                if( aRefItem->shape->Collide( aTestItem->shape, minClearance, &actual, &pos ) )
                {
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SILK_MASK_CLEARANCE );

                    if( minClearance > 0 )
                    {
                        m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                                      constraint.GetName(),
                                      MessageTextFromValue( userUnits(), minClearance ),
                                      MessageTextFromValue( userUnits(), actual ) );

                        drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                    }

                    drce->SetItems( aRefItem->parent, aTestItem->parent );
                    drce->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drce, (wxPoint) pos );

                    *aCollisionDetected = true;
                }

                return true;
            };

    int numMask = forEachGeometryItem( s_allBasicItems, LSET( 2, F_Mask, B_Mask ), addMaskToTree );
    int numSilk = forEachGeometryItem( s_allBasicItems, LSET( 2, F_SilkS, B_SilkS ), addSilkToTree );

    reportAux( _("Testing %d mask apertures against %d silkscreen features."), numMask, numSilk );

    const std::vector<DRC_RTREE::LAYER_PAIR> layerPairs =
    {
        DRC_RTREE::LAYER_PAIR( F_SilkS, F_Mask ),
        DRC_RTREE::LAYER_PAIR( B_SilkS, B_Mask )
    };

    // This is the number of tests between 2 calls to the progress bar
    const int delta = 250;

    maskTree.QueryCollidingPairs( &silkTree, layerPairs, checkClearance, m_largestClearance,
                                  [&]( int aCount, int aSize ) -> bool
                                  {
                                      return reportProgress( aCount, aSize, delta );
                                  } );

    reportRuleStatistics();

    return true;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_SILK_TO_MASK::GetConstraintTypes() const
{
    return { SILK_CLEARANCE_CONSTRAINT };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_SILK_TO_MASK> dummy;
}

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
#include <class_board.h>
#include <class_drawsegment.h>
#include <class_pad.h>

#include <convert_basic_shapes_to_polygon.h>
#include <geometry/polygon_test_point_inside.h>

#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>

#include <drc_proto/drc_engine.h>
#include <drc_proto/drc_item.h>
#include <drc_proto/drc_rule.h>
#include <drc_proto/drc_test_provider_clearance_base.h>

/*
    Board edge clearance test. Checks all items for their mechanical clearances against the board edge.
    Errors generated:
    - DRCE_COPPER_EDGE_CLEARANCE

    TODO:
    - separate holes to edge check
    - tester only looks for edge crossings. it doesn't check if items are inside/outside the board area.
*/

namespace test {

class DRC_TEST_PROVIDER_EDGE_CLEARANCE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_TEST_PROVIDER_EDGE_CLEARANCE () :
        DRC_TEST_PROVIDER_CLEARANCE_BASE()
        {
        }

    virtual ~DRC_TEST_PROVIDER_EDGE_CLEARANCE() 
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override 
    {
        return "edge_clearance"; 
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests items vs board edge clearance";
    }

    virtual std::set<test::DRC_RULE_ID_T> GetMatchingRuleIds() const override;

private:
};

};


bool test::DRC_TEST_PROVIDER_EDGE_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();

    DRC_CONSTRAINT worstClearanceConstraint;
    m_largestClearance = 0;

    if( m_drcEngine->QueryWorstConstraint( test::DRC_RULE_ID_T::DRC_RULE_ID_EDGE_CLEARANCE, worstClearanceConstraint, DRCCQ_LARGEST_MINIMUM ) )
    {
        m_largestClearance = worstClearanceConstraint.m_Value.Min();
    }

    ReportAux( "Worst clearance : %d nm", m_largestClearance );
    ReportStage( ("Testing all items <> Board Edge clearance"), 0, 2 );
    
    std::vector<DRAWSEGMENT*> boardOutline;
    std::vector<BOARD_ITEM*> boardItems;

    auto queryBoardOutlineItems = [&] ( BOARD_ITEM *item ) -> int
    {
        boardOutline.push_back( dyn_cast<DRAWSEGMENT*>( item ) );
    };

    auto queryBoardGeometryItems = [&] ( BOARD_ITEM *item ) -> int
    {
        boardItems.push_back( item );
    };

    

    forEachGeometryItem( { PCB_LINE_T }, LSET( Edge_Cuts ), queryBoardOutlineItems );
    forEachGeometryItem( {}, LSET::AllTechMask() | LSET::AllCuMask(), queryBoardGeometryItems );


    drc_dbg(2,"outline: %d items, board: %d items\n", boardOutline.size(), boardItems.size() );

    for( auto outlineItem : boardOutline )
    {
        //printf("RefT %d\n", outlineItem->Type() );
        auto refShape = outlineItem->GetEffectiveShape();

        for( auto boardItem : boardItems )
        {
//            printf("BoardT %d\n", boardItem->Type() );
            
            auto shape = boardItem->GetEffectiveShape();

            test::DRC_RULE* rule = m_drcEngine->EvalRulesForItems( test::DRC_RULE_ID_T::DRC_RULE_ID_EDGE_CLEARANCE, outlineItem, boardItem );
            int minClearance = rule->GetConstraint().GetValue().Min();
            int actual;

            if( refShape->Collide( shape.get(), minClearance, &actual ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_COPPER_EDGE_CLEARANCE );
                wxString msg;

                msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                            rule->GetName(),
                            MessageTextFromValue( userUnits(), minClearance, true ),
                            MessageTextFromValue( userUnits(), actual, true ) );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( outlineItem, boardItem );
                drcItem->SetViolatingRule( rule );

                ReportWithMarker( drcItem, refShape->Centre() );
            }
        }
    }

    return true;
}


std::set<test::DRC_RULE_ID_T> test::DRC_TEST_PROVIDER_EDGE_CLEARANCE::GetMatchingRuleIds() const
{
    return { DRC_RULE_ID_T::DRC_RULE_ID_EDGE_CLEARANCE };
}


namespace detail
{
    static test::DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_EDGE_CLEARANCE> dummy;
}
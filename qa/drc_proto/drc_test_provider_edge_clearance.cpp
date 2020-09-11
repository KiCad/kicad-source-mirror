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
#include <class_drawsegment.h>
#include <geometry/polygon_test_point_inside.h>

#include <geometry/seg.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>

#include <pcbnew/drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc.h>
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

    virtual std::set<DRC_CONSTRAINT_TYPE_T> GetMatchingConstraintIds() const override;

private:
};

};


bool test::DRC_TEST_PROVIDER_EDGE_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();

    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_EDGE_CLEARANCE,
                                           worstClearanceConstraint, DRCCQ_LARGEST_MINIMUM ) )
    {
        m_largestClearance = worstClearanceConstraint.GetValue().Min();
    }
    else
    {
        ReportAux("No Clearance constraints found...");
        return false;
    }

    ReportAux( "Worst clearance : %d nm", m_largestClearance );
    ReportStage( ("Testing all items <> Board Edge clearance"), 0, 2 );
    
    std::vector<DRAWSEGMENT*> boardOutline;
    std::vector<BOARD_ITEM*> boardItems;

    auto queryBoardOutlineItems = [&] ( BOARD_ITEM *item ) -> bool
    {
        boardOutline.push_back( dyn_cast<DRAWSEGMENT*>( item ) );
        return true;
    };

    auto queryBoardGeometryItems = [&] ( BOARD_ITEM *item ) -> bool
    {
        boardItems.push_back( item );
        return true;
    };

    forEachGeometryItem( { PCB_LINE_T }, LSET( Edge_Cuts ), queryBoardOutlineItems );
    forEachGeometryItem( {}, LSET::AllTechMask() | LSET::AllCuMask(), queryBoardGeometryItems );

    wxString val;
    wxGetEnv( "WXTRACE", &val);

    drc_dbg(2,"outline: %d items, board: %d items\n", boardOutline.size(), boardItems.size() );

    bool stop = false;

    for( auto outlineItem : boardOutline )
    {
        auto refShape = outlineItem->GetEffectiveShape();

        for( auto boardItem : boardItems )
        {
            drc_dbg( 10, "RefT %d %p %s %d\n", outlineItem->Type(), outlineItem,
                     outlineItem->GetClass(), outlineItem->GetLayer() );
            drc_dbg( 10, "BoardT %d %p %s %d\n", boardItem->Type(), boardItem,
                     boardItem->GetClass(), boardItem->GetLayer() );

            auto shape = boardItem->GetEffectiveShape();

            DRC_CONSTRAINT constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_EDGE_CLEARANCE,
                                                                        outlineItem, boardItem );
            int minClearance = constraint.GetValue().Min();
            int actual;

            accountCheck( constraint );

            if( refShape->Collide( shape.get(), minClearance, &actual ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_COPPER_EDGE_CLEARANCE );
                wxString msg;

                msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                            constraint.GetParentRule()->m_Name,
                            MessageTextFromValue( userUnits(), minClearance, true ),
                            MessageTextFromValue( userUnits(), actual, true ) );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( outlineItem, boardItem );
                drcItem->SetViolatingRule( constraint.GetParentRule() );

                ReportWithMarker( drcItem, refShape->Centre() );

                if( isErrorLimitExceeded( DRCE_COPPER_EDGE_CLEARANCE ) )
                {
                    stop = true;
                    break;
                }
            }
        }

        if( stop )
            break;
    }

    reportRuleStatistics();

    return true;
}


std::set<DRC_CONSTRAINT_TYPE_T> test::DRC_TEST_PROVIDER_EDGE_CLEARANCE::GetMatchingConstraintIds() const
{
    return { DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_EDGE_CLEARANCE };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_EDGE_CLEARANCE> dummy;
}
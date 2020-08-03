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

    TODO: holes to edge check
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
        return "Tests copper item vs board edge clearance";
    }

    virtual std::set<test::DRC_RULE_ID_T> GetMatchingRuleIds() const override;

private:
};

};


bool test::DRC_TEST_PROVIDER_EDGE_CLEARANCE::Run()
{
    auto bds = m_drcEngine->GetDesignSettings();
    m_board = m_drcEngine->GetBoard();

    m_largestClearance = 0;

    for( auto rule : m_drcEngine->QueryRulesById( test::DRC_RULE_ID_T::DRC_RULE_ID_EDGE_CLEARANCE ) )
    {
        if( rule->GetConstraint().m_Value.HasMin() )
        {
            m_largestClearance = std::max( m_largestClearance, rule->GetConstraint().m_Value.Min() );
        }
    }

    ReportAux( "Worst clearance : %d nm", m_largestClearance );

    //m_largestClearance = 

    ReportStage( ("Testing all items <> Board Edge clearance"), 0, 2 );
    
    std::vector<DRAWSEGMENT*> boardOutline;
    std::vector<BOARD_ITEM*> boardItems;

    for( auto item : m_board->Drawings() )
    {
        if( auto dseg = dyn_cast<DRAWSEGMENT*>( item ) )
        {
            drc_dbg(10,"L %d\n", dseg->GetLayer() );
            if( dseg->GetLayer() == Edge_Cuts )
            {
                drc_dbg(10, "dseg ec %p\n", dseg);
                boardOutline.push_back( dseg );
            }
        }
    }

    for ( auto trk : m_board->Tracks() )
    {
        boardItems.push_back( trk );
    }

    for ( auto zone : m_board->Zones() )
    {
        boardItems.push_back( zone );
    }

    for ( auto zone : m_board->Zones() )
    {
        boardItems.push_back( zone );
    }

    for ( auto mod : m_board->Modules() )
    {
        for ( auto dwg : mod->GraphicalItems() )
            boardItems.push_back( dwg );
        for ( auto pad : mod->Pads() )
            boardItems.push_back( pad );
    }

    drc_dbg(2,"outline: %d items, board: %d items\n", boardOutline.size(), boardItems.size() );

    for( auto outlineItem : boardOutline )
    {
        auto refShape = outlineItem->GetEffectiveShape();

        for( auto boardItem : boardItems )
        {
            auto shape = boardItem->GetEffectiveShape();

            (void) shape;
            (void) refShape;
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
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
    Holes clearance test. Checks pad and via holes for their mechanical clearances.
    Generated errors:
    - DRCE_HOLE_CLEARANCE

    TODO: vias-in-smd-pads check
*/

namespace test {

class DRC_TEST_PROVIDER_HOLE_CLEARANCE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_TEST_PROVIDER_HOLE_CLEARANCE () :
        DRC_TEST_PROVIDER_CLEARANCE_BASE(),
        m_board( nullptr ),
        m_largestRadius( 0 )
        {
        }

    virtual ~DRC_TEST_PROVIDER_HOLE_CLEARANCE()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "hole_clearance";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests clearance of holes (via/pad drills)";
    }

    virtual std::set<test::DRC_CONSTRAINT_TYPE_T> GetMatchingConstraintIds() const override;

private:
    void addHole( const VECTOR2I& aLocation, int aRadius, BOARD_ITEM* aOwner );

    void buildHoleList();
    void testHoles2Holes();
    void testPads2Holes();

    bool doPadToPadHoleDrc(  D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd, int x_limit );

    struct DRILLED_HOLE
    {
        VECTOR2I     m_location;
        int         m_drillRadius = 0;
        BOARD_ITEM* m_owner = nullptr;
    };

    BOARD*                    m_board;
    std::vector<DRILLED_HOLE> m_holes;
    int                       m_largestRadius;

};

};


bool test::DRC_TEST_PROVIDER_HOLE_CLEARANCE::Run()
{
    auto bds = m_drcEngine->GetDesignSettings();
    m_board = m_drcEngine->GetBoard();

    m_largestClearance = 0;
    m_largestRadius = 0;

    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( test::DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE, worstClearanceConstraint, DRCCQ_LARGEST_MINIMUM ) )
    {
        m_largestClearance = worstClearanceConstraint.GetValue().Min();
    }
    else
    {
        ReportAux("No Clearance constraints found...");
        return false;
    }


    ReportAux( "Worst hole clearance : %d nm", m_largestClearance );

    buildHoleList();

    ReportStage( ("Testing hole<->pad clearances"), 0, 2 );
    testPads2Holes();
    ReportStage( ("Testing hole<->hole clearances"), 0, 2 );
    testHoles2Holes();

    reportRuleStatistics();

    return true;
}


void test::DRC_TEST_PROVIDER_HOLE_CLEARANCE::buildHoleList()
{
    bool                   success = true;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    m_holes.clear();

    for( auto module : m_board->Modules() )
    {
        for( auto pad : module->Pads() )
        {
            int holeSize = std::min( pad->GetDrillSize().x, pad->GetDrillSize().y );

            if( holeSize == 0 )
                continue;

            // fixme: support for non-round (i.e. slotted) holes
            if( pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                addHole( pad->GetPosition(), pad->GetDrillSize().x / 2, pad );
        }
    }

    for( auto track : m_board->Tracks() )
    {
        if ( track->Type() == PCB_VIA_T )
        {
            auto via = static_cast<VIA*>( track );
            addHole( via->GetPosition(), via->GetDrillValue() / 2, via );
        }
    }

    ReportAux( "Total drilled holes : %d", m_holes.size() );

}

void test::DRC_TEST_PROVIDER_HOLE_CLEARANCE::testPads2Holes()
{
    std::vector<D_PAD*>    sortedPads;

    m_board->GetSortedPadListByXthenYCoord( sortedPads );

    if( sortedPads.empty() )
        return;

    // find the max size of the pads (used to stop the pad-to-pad tests)
    int max_size = 0;

    for( D_PAD* pad : sortedPads )
    {
        // GetBoundingRadius() is the radius of the minimum sized circle fully containing the pad
        int radius = pad->GetBoundingRadius();

        if( radius > max_size )
            max_size = radius;
    }

    // Better to be fast than accurate; this keeps us from having to look up / calculate the
    // actual clearances
    max_size += m_largestClearance;

    // Upper limit of pad list (limit not included)
    D_PAD** listEnd = &sortedPads[0] + sortedPads.size();

    // Test the pads
    for( auto& pad : sortedPads )
    {
       int x_limit = pad->GetPosition().x + pad->GetBoundingRadius() + max_size;
        drc_dbg(10,"-> %p\n", pad);
       doPadToPadHoleDrc( pad, &pad, listEnd, x_limit );
    }
}


bool test::DRC_TEST_PROVIDER_HOLE_CLEARANCE::doPadToPadHoleDrc(  D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd,
                          int x_limit )
{
    const static LSET all_cu = LSET::AllCuMask();

    LSET layerMask = aRefPad->GetLayerSet() & all_cu;


    for( D_PAD** pad_list = aStart;  pad_list<aEnd;  ++pad_list )
    {
        D_PAD* pad = *pad_list;

        if( pad == aRefPad )
            continue;



  //      drc_dbg(10," chk against -> %p\n", pad);

        // We can stop the test when pad->GetPosition().x > x_limit
        // because the list is sorted by X values
        if( pad->GetPosition().x > x_limit )
            break;

//        drc_dbg(10," chk2 against -> %p ds %d %d\n", pad, pad->GetDrillSize().x, aRefPad->GetDrillSize().x );

        drc_dbg(1," chk1 against -> %p x0 %d x2 %d\n", pad, pad->GetDrillSize().x, aRefPad->GetDrillSize().x );

        // No problem if pads which are on copper layers are on different copper layers,
        // (pads can be only on a technical layer, to build complex pads)
        // but their hole (if any ) can create DRC error because they are on all
        // copper layers, so we test them
        if( ( pad->GetLayerSet() & layerMask ) != 0 &&
            ( pad->GetLayerSet() & all_cu ) != 0 &&
            ( aRefPad->GetLayerSet() & all_cu ) != 0 )
        {

            // if holes are in the same location and have the same size and shape,
            // this can be accepted
            if( pad->GetPosition() == aRefPad->GetPosition()
                && pad->GetDrillSize() == aRefPad->GetDrillSize()
                && pad->GetDrillShape() == aRefPad->GetDrillShape() )
            {
                if( aRefPad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                    continue;

                // for oval holes: must also have the same orientation
                if( pad->GetOrientation() == aRefPad->GetOrientation() )
                    continue;
            }

            drc_dbg(1," chk3 against -> %p x0 %d x2 %d\n", pad, pad->GetDrillSize().x, aRefPad->GetDrillSize().x );

            /* Here, we must test clearance between holes and pads
             * pad size and shape is adjusted to pad drill size and shape
             */
            if( pad->GetDrillSize().x )
            {
                // pad under testing has a hole, test this hole against pad reference

                auto constraint = m_drcEngine->EvalRulesForItems( test::DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE, aRefPad, pad );
                auto minClearance = constraint.GetValue().Min();
                int actual;

                drc_dbg(1,"check pad %p rule '%s' cl %d\n", pad, constraint.GetParentRule()->GetName(), minClearance );

                accountCheck( constraint.GetParentRule() );

                auto refPadShape = aRefPad->GetEffectiveShape();
                // fixme: pad stacks...
                if( refPadShape->Collide( pad->GetEffectiveHoleShape(), minClearance, &actual ) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );

                    wxString msg;

                    msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  "",
                                  MessageTextFromValue( userUnits(), minClearance, true ),
                                  MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( pad, aRefPad );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    ReportWithMarker( drcItem, pad->GetPosition() );
                    return false;
                }
            }

            if( aRefPad->GetDrillSize().x ) // pad reference has a hole
            {

                auto constraint = m_drcEngine->EvalRulesForItems( test::DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE, aRefPad, pad );
                auto minClearance = constraint.GetValue().Min();
                int actual;

                accountCheck( constraint.GetParentRule() );

                drc_dbg(1,"check pad %p rule '%s' cl %d\n", constraint.GetParentRule()->GetName(), minClearance );

                auto padShape = pad->GetEffectiveShape();
                if( padShape->Collide( aRefPad->GetEffectiveHoleShape(), minClearance, &actual ) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                    wxString msg;

                    msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  "",
                                  MessageTextFromValue( userUnits(), minClearance, true ),
                                  MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( aRefPad, pad );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    ReportWithMarker( drcItem, pad->GetPosition() );
                    return false;
                }
            }
        }
    }

    return true;
}


void test::DRC_TEST_PROVIDER_HOLE_CLEARANCE::addHole( const VECTOR2I& aLocation, int aRadius, BOARD_ITEM* aOwner )
{
    DRILLED_HOLE hole;

    hole.m_location = aLocation;
    hole.m_drillRadius = aRadius;
    hole.m_owner = aOwner;

    m_largestRadius = std::max( m_largestRadius, aRadius );

    m_holes.push_back( hole );
}


void test::DRC_TEST_PROVIDER_HOLE_CLEARANCE::testHoles2Holes()
{
    // No need to check if we're ignoring DRCE_DRILLED_HOLES_TOO_CLOSE; if we are then we
    // won't have collected any holes to test.

    // Sort holes by X for performance.  In the nested iteration we then need to look at
    // following holes only while they are within the refHole's neighborhood as defined by
    // the refHole radius + the minimum hole-to-hole clearance + the largest radius any of
    // the following holes can have.
    std::sort( m_holes.begin(), m_holes.end(),
               []( const DRILLED_HOLE& a, const DRILLED_HOLE& b )
               {
                   if( a.m_location.x == b.m_location.x )
                       return a.m_location.y < b.m_location.y;
                   else
                       return a.m_location.x < b.m_location.x;
               } );

    for( size_t ii = 0; ii < m_holes.size(); ++ii )
    {
        DRILLED_HOLE& refHole = m_holes[ ii ];
        int neighborhood = refHole.m_drillRadius + m_largestClearance + m_largestRadius;

        for( size_t jj = ii + 1; jj < m_holes.size(); ++jj )
        {
            DRILLED_HOLE& checkHole = m_holes[ jj ];

            if( refHole.m_location.x + neighborhood < checkHole.m_location.x )
                break;

            // Holes with identical locations are allowable
            if( checkHole.m_location == refHole.m_location )
                continue;

            int actual = ( checkHole.m_location - refHole.m_location ).EuclideanNorm();
            actual = std::max( 0, actual - checkHole.m_drillRadius - refHole.m_drillRadius );

            DRC_CONSTRAINT constraint = m_drcEngine->EvalRulesForItems( test::DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE, refHole.m_owner, checkHole.m_owner );
            int minClearance = constraint.GetValue().Min();

            accountCheck( constraint.GetParentRule() );

            if( actual < minClearance )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                wxString msg;

                msg.Printf( drcItem->GetErrorText() + _( " (clearance %s; actual %s)" ),
                            MessageTextFromValue( userUnits(), minClearance, true ),
                            MessageTextFromValue( userUnits(), actual, true ) );

                drcItem->SetViolatingRule( constraint.GetParentRule() );
                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( refHole.m_owner, checkHole.m_owner );

                ReportWithMarker( drcItem, refHole.m_location );

                if( isErrorLimitExceeded( DRCE_HOLE_CLEARANCE ) )
                    return;
            }
        }
    }
}


std::set<test::DRC_CONSTRAINT_TYPE_T> test::DRC_TEST_PROVIDER_HOLE_CLEARANCE::GetMatchingConstraintIds() const
{
    return { DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE };
}


namespace detail
{
    static test::DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_HOLE_CLEARANCE> dummy;
}

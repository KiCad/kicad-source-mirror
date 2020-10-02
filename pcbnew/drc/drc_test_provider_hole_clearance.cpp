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
#include <class_pad.h>
#include <class_track.h>
#include <geometry/shape_segment.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>

/*
    Holes clearance test. Checks pad and via holes for their mechanical clearances.
    Generated errors:
    - DRCE_HOLE_CLEARANCE

    TODO: vias-in-smd-pads check
*/

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

    virtual std::set<DRC_CONSTRAINT_TYPE_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;

private:
    void addHole( const VECTOR2I& aLocation, int aRadius, BOARD_ITEM* aOwner );

    void buildDrilledHoleList();
    void testHoles2Holes();
    void testPads2Holes();

    /**
     * Test clearance of a pad hole with the pad hole of other pads.
     * @param aSortedPadsList is the sorted by X pos of all pads
     * @param aRefPadIdx is the index of pad to test inside aSortedPadsList
     * @param aX_limit is the max X pos of others pads that need to be tested
     * To speed up the test, aSortedPadsList is a pad list sorted by X position,
     * and only pads after the pad to test are tested, so this function must be called
     * for each pad for the first in list to the last in list
     */
    bool doPadToPadHoleDrc( int aRefPadIdx, std::vector<D_PAD*>& aSortedPadsList, int aX_limit );

    struct DRILLED_HOLE
    {
        VECTOR2I    m_location;
        int         m_drillRadius = 0;
        BOARD_ITEM* m_owner = nullptr;
    };

    BOARD*                    m_board;
    std::vector<DRILLED_HOLE> m_drilledHoles;
    int                       m_largestRadius;

};


bool DRC_TEST_PROVIDER_HOLE_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();

    m_largestClearance = 0;
    m_largestRadius = 0;

    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE,
                                           worstClearanceConstraint, DRCCQ_LARGEST_MINIMUM ) )
    {
        m_largestClearance = worstClearanceConstraint.GetValue().Min();
        reportAux( "Worst hole clearance : %d nm", m_largestClearance );
    }
    else
    {
        reportAux( "No hole clearance constraints found..." );
        return false;
    }

    buildDrilledHoleList();

    if( !reportPhase( _( "Checking hole to pad clearances..." ) ) )
        return false;

    testPads2Holes();

    if( !reportPhase( _( "Checking hole to hole clearances..." ) ) )
        return false;

    testHoles2Holes();

    reportRuleStatistics();

    return true;
}


void DRC_TEST_PROVIDER_HOLE_CLEARANCE::buildDrilledHoleList()
{
    m_drilledHoles.clear();

    for( MODULE* module : m_board->Modules() )
    {
        for( D_PAD* pad : module->Pads() )
        {
            int holeSize = std::min( pad->GetDrillSize().x, pad->GetDrillSize().y );

            if( holeSize == 0 )
                continue;

            // Milled holes (slots) aren't required to meet the minimum hole-to-hole
            // distance, so we only have to collect the drilled holes.
            if( pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                addHole( pad->GetPosition(), pad->GetDrillSize().x / 2, pad );
        }
    }

    for( TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            VIA* via = static_cast<VIA*>( track );
            addHole( via->GetPosition(), via->GetDrillValue() / 2, via );
        }
    }

    reportAux( "Total drilled holes : %d", m_drilledHoles.size());
}

void DRC_TEST_PROVIDER_HOLE_CLEARANCE::testPads2Holes()
{
    const int delta = 25;  // This is the number of tests between 2 calls to the progress bar
    std::vector<D_PAD*> sortedPads;

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

    // Test the pads
    for( int idx = 0; idx < (int) sortedPads.size(); idx++ )
    {
        D_PAD* pad = sortedPads[idx];
        int x_limit = pad->GetPosition().x + pad->GetBoundingRadius() + max_size;

        drc_dbg( 10, "-> %p\n", pad );

        if( !reportProgress( idx, sortedPads.size(), delta ) )
            break;

        doPadToPadHoleDrc( idx, sortedPads, x_limit );
    }
}


bool DRC_TEST_PROVIDER_HOLE_CLEARANCE::doPadToPadHoleDrc( int aRefPadIdx,
                                                          std::vector<D_PAD*>& aSortedPadsList,
                                                          int aX_limit )
{
    const static LSET all_cu = LSET::AllCuMask();

    D_PAD* refPad = aSortedPadsList[aRefPadIdx];
    LSET layerMask = refPad->GetLayerSet() & all_cu;

    for( int idx = aRefPadIdx; idx < (int)aSortedPadsList.size();  ++idx )
    {
        D_PAD* pad = aSortedPadsList[idx];

        if( pad == refPad || pad->SameLogicalPadAs( refPad ) )
            continue;

//      drc_dbg(10," chk against -> %p\n", pad);

        // We can stop the test when pad->GetPosition().x > aX_limit because the list is
        // sorted by X positions, and other pads are too far.
        if( pad->GetPosition().x > aX_limit )
            break;

        drc_dbg( 10, " chk1 against -> %p x0 %d x2 %d\n",
                 pad, pad->GetDrillSize().x, refPad->GetDrillSize().x );

        // Since a hole pierces all layers we have to test pads which are on any copper layer.
        // Pads just on technical layers are not an issue.
        if( ( pad->GetLayerSet() & all_cu ) != 0 || ( refPad->GetLayerSet() & all_cu ) != 0 )
        {
            drc_dbg( 10, " chk3 against -> %p x0 %d x2 %d\n", pad, pad->GetDrillSize().x,
                     refPad->GetDrillSize().x );

            if( pad->GetDrillSize().x )     // test pad has a hole
            {
                auto constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE,
                                                                  refPad, pad );
                int  minClearance = constraint.GetValue().Min();
                int  actual;

                drc_dbg( 10, "check pad %p rule '%s' cl %d\n",
                         pad, constraint.GetParentRule()->m_Name, minClearance );

                accountCheck( constraint.GetParentRule() );

                const std::shared_ptr<SHAPE>&  refPadShape = refPad->GetEffectiveShape();

                // fixme: pad stacks...
                if( refPadShape->Collide( pad->GetEffectiveHoleShape(), minClearance, &actual ) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );

                    m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  constraint.GetName(),
                                  MessageTextFromValue( userUnits(), minClearance ),
                                  MessageTextFromValue( userUnits(), actual ) );

                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( pad, refPad );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, pad->GetPosition() );
                    return false;
                }
            }

            if( refPad->GetDrillSize().x )     // reference pad has a hole
            {
                auto constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE,
                                                                  refPad, pad );
                int  minClearance = constraint.GetValue().Min();
                int  actual;

                accountCheck( constraint.GetParentRule() );

                drc_dbg( 10,"check pad %p rule '%s' cl %d\n", refPad,
                         constraint.GetParentRule()->m_Name, minClearance );

                const std::shared_ptr<SHAPE>& padShape = pad->GetEffectiveShape();

                if( padShape->Collide( refPad->GetEffectiveHoleShape(), minClearance, &actual ) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );

                    m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  constraint.GetName(),
                                  MessageTextFromValue( userUnits(), minClearance ),
                                  MessageTextFromValue( userUnits(), actual ) );

                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( refPad, pad );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, pad->GetPosition() );
                    return false;
                }
            }
        }
    }

    return true;
}


void DRC_TEST_PROVIDER_HOLE_CLEARANCE::addHole( const VECTOR2I& aLocation, int aRadius,
                                                      BOARD_ITEM* aOwner )
{
    DRILLED_HOLE hole;

    hole.m_location = aLocation;
    hole.m_drillRadius = aRadius;
    hole.m_owner = aOwner;

    m_largestRadius = std::max( m_largestRadius, aRadius );

    m_drilledHoles.push_back( hole );
}


void DRC_TEST_PROVIDER_HOLE_CLEARANCE::testHoles2Holes()
{
    const int delta = 50;  // This is the number of tests between 2 calls to the progress bar

    // Sort holes by X for performance.  In the nested iteration we then need to look at
    // following holes only while they are within the refHole's neighborhood as defined by
    // the refHole radius + the minimum hole-to-hole clearance + the largest radius any of
    // the following holes can have.
    std::sort( m_drilledHoles.begin(), m_drilledHoles.end(),
               []( const DRILLED_HOLE& a, const DRILLED_HOLE& b )
               {
                   if( a.m_location.x == b.m_location.x )
                       return a.m_location.y < b.m_location.y;
                   else
                       return a.m_location.x < b.m_location.x;
               } );

    for( size_t ii = 0; ii < m_drilledHoles.size(); ++ii )
    {
        if( !reportProgress( ii, m_drilledHoles.size(), delta ) )
            break;

        if( m_drcEngine->IsErrorLimitExceeded( DRCE_DRILLED_HOLES_TOO_CLOSE ) )
            break;

        DRILLED_HOLE& refHole = m_drilledHoles[ ii ];
        int neighborhood = refHole.m_drillRadius + m_largestClearance + m_largestRadius;

        for( size_t jj = ii + 1; jj < m_drilledHoles.size(); ++jj )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_DRILLED_HOLES_TOO_CLOSE ) )
                break;

            DRILLED_HOLE& checkHole = m_drilledHoles[ jj ];

            if( refHole.m_location.x + neighborhood < checkHole.m_location.x )
                break;

            // Holes with identical locations are allowable
            if( checkHole.m_location == refHole.m_location )
                continue;

            int actual = ( checkHole.m_location - refHole.m_location ).EuclideanNorm();
            actual = std::max( 0, actual - checkHole.m_drillRadius - refHole.m_drillRadius );

            auto constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE,
                                                              refHole.m_owner, checkHole.m_owner );
            int  minClearance = constraint.GetValue().Min();

            accountCheck( constraint.GetParentRule() );

            if( actual < minClearance )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_DRILLED_HOLES_TOO_CLOSE );

                m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), minClearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( refHole.m_owner, checkHole.m_owner );
                drcItem->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drcItem, (wxPoint) refHole.m_location );
            }
        }
    }
}


int DRC_TEST_PROVIDER_HOLE_CLEARANCE::GetNumPhases() const
{
    return 2;
}


std::set<DRC_CONSTRAINT_TYPE_T> DRC_TEST_PROVIDER_HOLE_CLEARANCE::GetConstraintTypes() const
{
    return { DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_HOLE_CLEARANCE> dummy;
}

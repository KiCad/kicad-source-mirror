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
#include <footprint.h>
#include <pad.h>
#include <track.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>
#include "drc_rtree.h"

/*
    Holes clearance test. Checks pad and via holes for their mechanical clearances.
    Generated errors:
    - DRCE_DRILLED_HOLES_TOO_CLOSE
    - DRCE_DRILLED_HOLES_COLOCATED

    TODO: vias-in-smd-pads check
*/

class DRC_TEST_PROVIDER_HOLE_TO_HOLE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_TEST_PROVIDER_HOLE_TO_HOLE () :
        DRC_TEST_PROVIDER_CLEARANCE_BASE(),
        m_board( nullptr )
    {
    }

    virtual ~DRC_TEST_PROVIDER_HOLE_TO_HOLE()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "hole_to_hole_clearance";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests hole to hole spacing";
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;

private:
    bool testHoleAgainstHole( BOARD_ITEM* aItem, SHAPE_CIRCLE* aHole, BOARD_ITEM* aOther );

    BOARD*    m_board;
    DRC_RTREE m_holeTree;
};


static std::shared_ptr<SHAPE_CIRCLE> getDrilledHoleShape( BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_VIA_T )
    {
        VIA* via = static_cast<VIA*>( aItem );
        return std::make_shared<SHAPE_CIRCLE>( via->GetCenter(), via->GetDrillValue() / 2 );
    }
    else if( aItem->Type() == PCB_PAD_T )
    {
        PAD* pad = static_cast<PAD*>( aItem );
        return std::make_shared<SHAPE_CIRCLE>( pad->GetPosition(), pad->GetDrillSize().x / 2 );
    }

    return std::make_shared<SHAPE_CIRCLE>( VECTOR2I( 0, 0 ), 0 );
}


bool DRC_TEST_PROVIDER_HOLE_TO_HOLE::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_DRILLED_HOLES_TOO_CLOSE )
            && m_drcEngine->IsErrorLimitExceeded( DRCE_DRILLED_HOLES_COLOCATED ) )
    {
        reportAux( "Hole to hole violations ignored. Tests not run." );
        return true;        // continue with other tests
    }

    m_board = m_drcEngine->GetBoard();

    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( HOLE_TO_HOLE_CONSTRAINT, worstClearanceConstraint ) )
    {
        m_largestClearance = worstClearanceConstraint.GetValue().Min();
        reportAux( "Worst hole to hole : %d nm", m_largestClearance );
    }
    else
    {
        reportAux( "No hole to hole constraints found. Skipping check." );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking hole to hole clearances..." ) ) )
        return false;   // DRC cancelled

    // This is the number of tests between 2 calls to the progress bar
    const size_t delta = 50;
    size_t       count = 0;
    size_t       ii = 0;

    m_holeTree.clear();

    auto countItems =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( item->Type() == PCB_PAD_T )
                    ++count;
                else if( item->Type() == PCB_VIA_T )
                    ++count;

                return true;
            };

    auto addToHoleTree =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !reportProgress( ii++, count, delta ) )
                    return false;

                if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    // We only care about drilled (ie: round) holes
                    if( pad->GetDrillSize().x && pad->GetDrillSize().x == pad->GetDrillSize().y )
                        m_holeTree.Insert( item, F_Cu, m_largestClearance );
                }
                else if( item->Type() == PCB_VIA_T )
                {
                    VIA* via = static_cast<VIA*>( item );

                    // We only care about mechanically drilled (ie: non-laser) holes
                    if( via->GetViaType() == VIATYPE::THROUGH )
                        m_holeTree.Insert( item, F_Cu, m_largestClearance );
                }

                return true;
            };

    forEachGeometryItem( { PCB_PAD_T, PCB_VIA_T }, LSET::AllLayersMask(), countItems );

    count *= 2;  // One for adding to tree; one for checking

    forEachGeometryItem( { PCB_PAD_T, PCB_VIA_T }, LSET::AllLayersMask(), addToHoleTree );

    std::map< std::pair<BOARD_ITEM*, BOARD_ITEM*>, int> checkedPairs;

    for( TRACK* track : m_board->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        VIA* via = static_cast<VIA*>( track );

        if( !reportProgress( ii++, count, delta ) )
            return false;   // DRC cancelled

        // We only care about mechanically drilled (ie: non-laser) holes
        if( via->GetViaType() == VIATYPE::THROUGH )
        {
            std::shared_ptr<SHAPE_CIRCLE> holeShape = getDrilledHoleShape( via );

            m_holeTree.QueryColliding( via, F_Cu, F_Cu,
                    // Filter:
                    [&]( BOARD_ITEM* other ) -> bool
                    {
                        BOARD_ITEM* a = via;
                        BOARD_ITEM* b = other;

                        // store canonical order so we don't collide in both directions
                        // (a:b and b:a)
                        if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                            std::swap( a, b );

                        if( checkedPairs.count( { a, b } ) )
                        {
                            return false;
                        }
                        else
                        {
                            checkedPairs[ { a, b } ] = 1;
                            return true;
                        }
                    },
                    // Visitor:
                    [&]( BOARD_ITEM* other ) -> bool
                    {
                        return testHoleAgainstHole( via, holeShape.get(), other );
                    },
                    m_largestClearance );
        }
    }

    checkedPairs.clear();

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( !reportProgress( ii++, count, delta ) )
                return false;   // DRC cancelled

            // We only care about drilled (ie: round) holes
            if( pad->GetDrillSize().x && pad->GetDrillSize().x == pad->GetDrillSize().y )
            {
                std::shared_ptr<SHAPE_CIRCLE> holeShape = getDrilledHoleShape( pad );

                m_holeTree.QueryColliding( pad, F_Cu, F_Cu,
                        // Filter:
                        [&]( BOARD_ITEM* other ) -> bool
                        {
                            BOARD_ITEM* a = pad;
                            BOARD_ITEM* b = other;

                            // store canonical order so we don't collide in both directions
                            // (a:b and b:a)
                            if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                                std::swap( a, b );

                            if( checkedPairs.count( { a, b } ) )
                            {
                                return false;
                            }
                            else
                            {
                                checkedPairs[ { a, b } ] = 1;
                                return true;
                            }
                        },
                        // Visitor:
                        [&]( BOARD_ITEM* other ) -> bool
                        {
                            return testHoleAgainstHole( pad, holeShape.get(), other );
                        },
                        m_largestClearance );
            }
        }
    }

    reportRuleStatistics();

    return true;
}


bool DRC_TEST_PROVIDER_HOLE_TO_HOLE::testHoleAgainstHole( BOARD_ITEM* aItem, SHAPE_CIRCLE* aHole,
                                                          BOARD_ITEM* aOther )
{
    bool reportCoLocation = !m_drcEngine->IsErrorLimitExceeded( DRCE_DRILLED_HOLES_COLOCATED );
    bool reportHole2Hole = !m_drcEngine->IsErrorLimitExceeded( DRCE_DRILLED_HOLES_TOO_CLOSE );

    if( !reportCoLocation && !reportHole2Hole )
        return false;

    std::shared_ptr<SHAPE_CIRCLE> otherHole = getDrilledHoleShape( aOther );
    int                           epsilon = m_board->GetDesignSettings().GetDRCEpsilon();
    SEG::ecoord                   epsilon_sq = SEG::Square( epsilon );

    // Holes at same location generate a separate violation
    if( ( aHole->GetCenter() - otherHole->GetCenter() ).SquaredEuclideanNorm() < epsilon_sq )
    {
        if( reportCoLocation )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_DRILLED_HOLES_COLOCATED );
            drce->SetItems( aItem, aOther );
            reportViolation( drce, (wxPoint) aHole->GetCenter() );
        }
    }
    else if( reportHole2Hole )
    {
        int actual = ( aHole->GetCenter() - otherHole->GetCenter() ).EuclideanNorm();
        actual = std::max( 0, actual - aHole->GetRadius() - otherHole->GetRadius() );

        auto constraint = m_drcEngine->EvalRules( HOLE_TO_HOLE_CONSTRAINT, aItem, aOther,
                                                  UNDEFINED_LAYER /* holes pierce all layers */ );
        int  minClearance = constraint.GetValue().Min() - epsilon;

        if( minClearance >= 0 && actual < minClearance )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_DRILLED_HOLES_TOO_CLOSE );

            m_msg.Printf( _( "(%s min %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), minClearance ),
                          MessageTextFromValue( userUnits(), actual ) );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
            drce->SetItems( aItem, aOther );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, (wxPoint) aHole->GetCenter() );
        }
    }

    return true;
}


int DRC_TEST_PROVIDER_HOLE_TO_HOLE::GetNumPhases() const
{
    return 1;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_HOLE_TO_HOLE::GetConstraintTypes() const
{
    return { HOLE_TO_HOLE_CONSTRAINT };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_HOLE_TO_HOLE> dummy;
}

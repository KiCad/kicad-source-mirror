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
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include "drc_rtree.h"

/*
    Holes clearance test. Checks pad and via holes for their mechanical clearances.
    Generated errors:
    - DRCE_DRILLED_HOLES_TOO_CLOSE
    - DRCE_DRILLED_HOLES_COLOCATED
*/

class DRC_TEST_PROVIDER_HOLE_TO_HOLE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_HOLE_TO_HOLE () :
            DRC_TEST_PROVIDER(),
            m_board( nullptr ),
            m_largestHoleToHoleClearance( 0 )
    {}

    virtual ~DRC_TEST_PROVIDER_HOLE_TO_HOLE() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "hole_to_hole_clearance" ); };

private:
    bool testHoleAgainstHole( BOARD_ITEM* aItem, SHAPE_CIRCLE* aHole, BOARD_ITEM* aOther );

    BOARD*    m_board;
    DRC_RTREE m_holeTree;
    int       m_largestHoleToHoleClearance;
};


static std::shared_ptr<SHAPE_CIRCLE> getHoleShape( BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_VIA_T )
    {
        PCB_VIA* via = static_cast<PCB_VIA*>( aItem );
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
        REPORT_AUX( wxT( "Hole to hole violations ignored. Tests not run." ) );
        return true;        // continue with other tests
    }

    m_board = m_drcEngine->GetBoard();

    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( HOLE_TO_HOLE_CONSTRAINT, worstClearanceConstraint ) )
    {
        m_largestHoleToHoleClearance = worstClearanceConstraint.GetValue().Min();
    }
    else
    {
        REPORT_AUX( wxT( "No hole to hole constraints found. Skipping check." ) );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking hole to hole clearances..." ) ) )
        return false;   // DRC cancelled

    const size_t progressDelta = 200;
    size_t       count = 0;
    size_t       ii = 0;

    m_holeTree.clear();

    forEachGeometryItem( { PCB_PAD_T, PCB_VIA_T }, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++count;
                return true;
            } );

    count *= 2;  // One for adding to the rtree; one for checking

    forEachGeometryItem( { PCB_PAD_T, PCB_VIA_T }, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !reportProgress( ii++, count, progressDelta ) )
                    return false;

                if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    // Slots are generally milled _after_ drilling, so we ignore them.
                    if( pad->GetDrillSize().x && pad->GetDrillSize().x == pad->GetDrillSize().y )
                        m_holeTree.Insert( item, Edge_Cuts, m_largestHoleToHoleClearance );
                }
                else if( item->Type() == PCB_VIA_T )
                {
                    // Blind/buried/microvias will be drilled/burned _prior_ to lamination, so
                    // subsequently drilled holes need to avoid them.
                    m_holeTree.Insert( item, Edge_Cuts, m_largestHoleToHoleClearance );
                }

                return true;
            } );

    std::unordered_map<PTR_PTR_CACHE_KEY, int> checkedPairs;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( track );

        if( !reportProgress( ii++, count, progressDelta ) )
            return false;   // DRC cancelled

        // We only care about mechanically drilled (ie: non-laser) holes.  These include both
        // blind/buried via holes (drilled prior to lamination) and through-via and drilled pad
        // holes (which are generally drilled post laminataion).
        if( via->GetViaType() != VIATYPE::MICROVIA )
        {
            std::shared_ptr<SHAPE_CIRCLE> holeShape = getHoleShape( via );

            m_holeTree.QueryColliding( via, Edge_Cuts, Edge_Cuts,
                    // Filter:
                    [&]( BOARD_ITEM* other ) -> bool
                    {
                        BOARD_ITEM* a = via;
                        BOARD_ITEM* b = other;

                        // store canonical order so we don't collide in both directions
                        // (a:b and b:a)
                        if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                            std::swap( a, b );

                        if( checkedPairs.find( { a, b } ) != checkedPairs.end() )
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
                    m_largestHoleToHoleClearance );
        }
    }

    checkedPairs.clear();

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( !reportProgress( ii++, count, progressDelta ) )
                return false;   // DRC cancelled

            // We only care about drilled (ie: round) pad holes
            if( pad->HasDrilledHole() )
            {
                std::shared_ptr<SHAPE_CIRCLE> holeShape = getHoleShape( pad );

                m_holeTree.QueryColliding( pad, Edge_Cuts, Edge_Cuts,
                        // Filter:
                        [&]( BOARD_ITEM* other ) -> bool
                        {
                            BOARD_ITEM* a = pad;
                            BOARD_ITEM* b = other;

                            // store canonical order so we don't collide in both directions
                            // (a:b and b:a)
                            if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                                std::swap( a, b );

                            if( checkedPairs.find( { a, b } ) != checkedPairs.end() )
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
                        m_largestHoleToHoleClearance );
            }
        }

        if( m_drcEngine->IsCancelled() )
            return false;
    }

    return !m_drcEngine->IsCancelled();
}


bool DRC_TEST_PROVIDER_HOLE_TO_HOLE::testHoleAgainstHole( BOARD_ITEM* aItem, SHAPE_CIRCLE* aHole,
                                                          BOARD_ITEM* aOther )
{
    bool reportCoLocation = !m_drcEngine->IsErrorLimitExceeded( DRCE_DRILLED_HOLES_COLOCATED );
    bool reportHole2Hole = !m_drcEngine->IsErrorLimitExceeded( DRCE_DRILLED_HOLES_TOO_CLOSE );

    if( !reportCoLocation && !reportHole2Hole )
        return false;

    std::shared_ptr<SHAPE_CIRCLE> otherHole = getHoleShape( aOther );
    int                           epsilon = m_board->GetDesignSettings().GetDRCEpsilon();
    SEG::ecoord                   epsilon_sq = SEG::Square( epsilon );

    // Blind-buried vias are drilled prior to stackup; they're only an issue if they share layers
    if( aItem->Type() == PCB_VIA_T && aOther->Type() == PCB_VIA_T )
    {
        LSET viaHoleLayers = static_cast<PCB_VIA*>( aItem )->GetLayerSet() & LSET::AllCuMask();

        if( ( viaHoleLayers & static_cast<PCB_VIA*>( aOther )->GetLayerSet() ).none() )
            return false;
    }

    // Holes at same location generate a separate violation
    if( ( aHole->GetCenter() - otherHole->GetCenter() ).SquaredEuclideanNorm() < epsilon_sq )
    {
        if( reportCoLocation )
        {
            // Generate violations based on a well-defined order so that exclusion checking
            // against previously-generated violations will work.
            if( aItem->m_Uuid > aOther->m_Uuid )
                std::swap( aItem, aOther );

            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_DRILLED_HOLES_COLOCATED );
            drcItem->SetItems( aItem, aOther );
            reportTwoPointGeometry( drcItem, aHole->GetCenter(), aHole->GetCenter(), aHole->GetCenter(),
                                    UNDEFINED_LAYER );
        }
    }
    else if( reportHole2Hole )
    {
        int actual = ( aHole->GetCenter() - otherHole->GetCenter() ).EuclideanNorm();
        actual = std::max( 0, actual - aHole->GetRadius() - otherHole->GetRadius() );

        auto constraint = m_drcEngine->EvalRules( HOLE_TO_HOLE_CONSTRAINT, aItem, aOther,
                                                  UNDEFINED_LAYER /* holes pierce all layers */ );
        int  minClearance = std::max( 0, constraint.GetValue().Min() - epsilon );

        if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE
                && minClearance >= 0
                && actual < minClearance )
        {
            // Generate violations based on a well-defined order so that exclusion checking
            // against previously-generated violations will work.
            if( aItem->m_Uuid > aOther->m_Uuid )
                std::swap( aItem, aOther );

            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_DRILLED_HOLES_TOO_CLOSE );
            drcItem->SetErrorDetail( formatMsg( _( "(%s min %s; actual %s)" ),
                                                constraint.GetName(),
                                                minClearance,
                                                actual ) );
            drcItem->SetItems( aItem, aOther );
            drcItem->SetViolatingRule( constraint.GetParentRule() );
            reportTwoShapeGeometry( drcItem, aHole->GetCenter(), aHole, otherHole.get(), UNDEFINED_LAYER, actual );
        }
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_HOLE_TO_HOLE> dummy;
}

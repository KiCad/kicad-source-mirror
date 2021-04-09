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
#include <pcb_shape.h>
#include <pad.h>
#include <track.h>

#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>

#include <drc/drc_rtree.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>
#include <dimension.h>

/*
    Copper clearance test. Checks all copper items (pads, vias, tracks, drawings, zones) for their electrical clearance.
    Errors generated:
    - DRCE_CLEARANCE
    - DRCE_TRACKS_CROSSING
    - DRCE_ZONES_INTERSECT
    - DRCE_SHORTING_ITEMS
*/

class DRC_TEST_PROVIDER_COPPER_CLEARANCE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_TEST_PROVIDER_COPPER_CLEARANCE () :
            DRC_TEST_PROVIDER_CLEARANCE_BASE(),
            m_drcEpsilon( 0 )
    {
    }

    virtual ~DRC_TEST_PROVIDER_COPPER_CLEARANCE()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "clearance";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests copper item clearance";
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;

private:
    bool testTrackAgainstItem( TRACK* track, SHAPE* trackShape, PCB_LAYER_ID layer,
                               BOARD_ITEM* other );

    void testTrackClearances();

    bool testPadAgainstItem( PAD* pad, SHAPE* padShape, PCB_LAYER_ID layer, BOARD_ITEM* other );

    void testPadClearances();

    void testZones();

    void testItemAgainstZones( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer );

private:
    DRC_RTREE          m_copperTree;
    int                m_drcEpsilon;

    std::vector<ZONE*> m_zones;
};


bool DRC_TEST_PROVIDER_COPPER_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();
    DRC_CONSTRAINT worstConstraint;

    if( m_drcEngine->QueryWorstConstraint( CLEARANCE_CONSTRAINT, worstConstraint ) )
        m_largestClearance = worstConstraint.GetValue().Min();

    if( m_drcEngine->QueryWorstConstraint( HOLE_CLEARANCE_CONSTRAINT, worstConstraint ) )
        m_largestClearance = std::max( m_largestClearance, worstConstraint.GetValue().Min() );

    if( m_largestClearance <= 0 )
    {
        reportAux( "No Clearance constraints found. Tests not run." );
        return true;   // continue with other tests
    }

    m_drcEpsilon = m_board->GetDesignSettings().GetDRCEpsilon();

    m_zones.clear();

    for( ZONE* zone : m_board->Zones() )
    {
        if( !zone->GetIsRuleArea() )
        {
            m_zones.push_back( zone );
            m_largestClearance = std::max( m_largestClearance, zone->GetLocalClearance() );
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
            m_largestClearance = std::max( m_largestClearance, pad->GetLocalClearance() );

        for( ZONE* zone : footprint->Zones() )
        {
            if( !zone->GetIsRuleArea() )
            {
                m_zones.push_back( zone );
                m_largestClearance = std::max( m_largestClearance, zone->GetLocalClearance() );
            }
        }
    }

    reportAux( "Worst clearance : %d nm", m_largestClearance );

    // This is the number of tests between 2 calls to the progress bar
    size_t delta = 50;
    size_t count = 0;
    size_t ii = 0;

    m_copperTree.clear();

    auto countItems =
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++count;
                return true;
            };

    auto addToCopperTree =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !reportProgress( ii++, count, delta ) )
                    return false;

                m_copperTree.Insert( item, m_largestClearance );
                return true;
            };

    if( !reportPhase( _( "Gathering copper items..." ) ) )
        return false;   // DRC cancelled

    static const std::vector<KICAD_T> itemTypes = {
        PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_PAD_T, PCB_SHAPE_T, PCB_FP_SHAPE_T,
        PCB_TEXT_T, PCB_FP_TEXT_T, PCB_DIMENSION_T, PCB_DIM_ALIGNED_T, PCB_DIM_LEADER_T,
        PCB_DIM_CENTER_T,  PCB_DIM_ORTHOGONAL_T
    };

    forEachGeometryItem( itemTypes, LSET::AllCuMask(), countItems );
    forEachGeometryItem( itemTypes, LSET::AllCuMask(), addToCopperTree );

    reportAux( "Testing %d copper items and %d zones...", count, m_zones.size() );

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE ) )
    {
        if( !reportPhase( _( "Checking track & via clearances..." ) ) )
            return false;   // DRC cancelled

        testTrackClearances();
    }
    else if( !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE ) )
    {
        if( !reportPhase( _( "Checking hole clearances..." ) ) )
            return false;   // DRC cancelled

        testTrackClearances();
    }

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE ) )
    {
        if( !reportPhase( _( "Checking pad clearances..." ) ) )
            return false;   // DRC cancelled

        testPadClearances();
    }
    else if( !m_drcEngine->IsErrorLimitExceeded( DRCE_SHORTING_ITEMS )
            || !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE ) )
    {
        if( !reportPhase( _( "Checking pads..." ) ) )
            return false;   // DRC cancelled

        testPadClearances();
    }

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE ) )
    {
        if( !reportPhase( _( "Checking copper zone clearances..." ) ) )
            return false;   // DRC cancelled

        testZones();
    }
    else if( !m_drcEngine->IsErrorLimitExceeded( DRCE_ZONES_INTERSECT ) )
    {
        if( !reportPhase( _( "Checking zones..." ) ) )
            return false;   // DRC cancelled

        testZones();
    }

    reportRuleStatistics();

    return true;
}


bool DRC_TEST_PROVIDER_COPPER_CLEARANCE::testTrackAgainstItem( TRACK* track, SHAPE* trackShape,
                                                               PCB_LAYER_ID layer,
                                                               BOARD_ITEM* other )
{
    bool           testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
    bool           testHoles = !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE );
    DRC_CONSTRAINT constraint;
    int            clearance = -1;
    int            actual;
    VECTOR2I       pos;

    if( other->Type() == PCB_PAD_T )
    {
        PAD* pad = static_cast<PAD*>( other );

        if( pad->GetAttribute() == PAD_ATTRIB_NPTH && !pad->FlashLayer( layer ) )
            testClearance = false;
    }

    if( testClearance )
    {
        constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, track, other, layer );
        clearance = constraint.GetValue().Min();
    }

    if( clearance >= 0 )
    {
        // Special processing for track:track intersections
        if( track->Type() == PCB_TRACE_T && other->Type() == PCB_TRACE_T )
        {
            SEG trackSeg( track->GetStart(), track->GetEnd() );
            SEG otherSeg( track->GetStart(), track->GetEnd() );

            if( OPT_VECTOR2I intersection = trackSeg.Intersect( otherSeg ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TRACKS_CROSSING );
                drcItem->SetItems( track, other );
                drcItem->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drcItem, (wxPoint) intersection.get() );

                return m_drcEngine->GetReportAllTrackErrors();
            }
        }

        std::shared_ptr<SHAPE> otherShape = DRC_ENGINE::GetShape( other, layer );

        if( trackShape->Collide( otherShape.get(), clearance - m_drcEpsilon, &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );

            m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), clearance ),
                          MessageTextFromValue( userUnits(), actual ) );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
            drce->SetItems( track, other );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, (wxPoint) pos );

            if( !m_drcEngine->GetReportAllTrackErrors() )
                return false;
        }
    }

    if( testHoles && ( other->Type() == PCB_VIA_T || other->Type() == PCB_PAD_T ) )
    {
        std::unique_ptr<SHAPE_SEGMENT> holeShape;

        if( other->Type() == PCB_VIA_T )
        {
            VIA* via = static_cast<VIA*>( other );
            pos = via->GetPosition();

            if( via->GetLayerSet().Contains( layer ) )
                holeShape.reset( new SHAPE_SEGMENT( pos, pos, via->GetDrill() ) );
        }
        else if( other->Type() == PCB_PAD_T )
        {
            PAD* pad = static_cast<PAD*>( other );

            if( pad->GetDrillSize().x )
                holeShape.reset( new SHAPE_SEGMENT( *pad->GetEffectiveHoleShape() ) );
        }

        if( holeShape )
        {
            constraint = m_drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, other, track,
                                                 track->GetLayer() );
            clearance = constraint.GetValue().Min();

            if( clearance >= 0 && trackShape->Collide( holeShape.get(),
                                                       std::max( 0, clearance - m_drcEpsilon ),
                                                       &actual, &pos ) )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );

                m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), clearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                drce->SetItems( track, other );
                drce->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drce, (wxPoint) pos );

                if( !m_drcEngine->GetReportAllTrackErrors() )
                    return false;
            }
        }
    }

    return true;
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testItemAgainstZones( BOARD_ITEM* aItem,
                                                               PCB_LAYER_ID aLayer )
{
    for( ZONE* zone : m_zones )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE ) )
            break;

        if( !zone->GetLayerSet().test( aLayer ) )
            continue;

        if( zone->GetNetCode() && aItem->IsConnected() )
        {
            if( zone->GetNetCode() == static_cast<BOARD_CONNECTED_ITEM*>( aItem )->GetNetCode() )
                continue;
        }

        if( aItem->GetBoundingBox().Intersects( zone->GetCachedBoundingBox() ) )
        {
            auto constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, aItem, zone, aLayer );
            int clearance = constraint.GetValue().Min();

            if( clearance < 0 )
                continue;

            int                    actual;
            VECTOR2I               pos;
            DRC_RTREE*             zoneTree = m_board->m_CopperZoneRTrees[ zone ].get();
            EDA_RECT               itemBBox = aItem->GetBoundingBox();
            std::shared_ptr<SHAPE> itemShape = aItem->GetEffectiveShape( aLayer );

            if( aItem->Type() == PCB_PAD_T )
            {
                PAD* pad = static_cast<PAD*>( aItem );

                if( !pad->FlashLayer( aLayer ) )
                {
                    if( pad->GetDrillSize().x == 0 && pad->GetDrillSize().y == 0 )
                        continue;

                    const SHAPE_SEGMENT* hole = pad->GetEffectiveHoleShape();
                    int                  size = hole->GetWidth();

                    // Note: drill size represents finish size, which means the actual hole
                    // size is the plating thickness larger.
                    if( pad->GetAttribute() == PAD_ATTRIB_PTH )
                        size += m_board->GetDesignSettings().GetHolePlatingThickness();

                    itemShape = std::make_shared<SHAPE_SEGMENT>( hole->GetSeg(), size );
                }
            }

            if( zoneTree->QueryColliding( itemBBox, itemShape.get(), aLayer,
                                          clearance - m_drcEpsilon, &actual, &pos ) )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );

                m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), clearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                drce->SetItems( aItem, zone );
                drce->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drce, (wxPoint) pos );
            }
        }
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testTrackClearances()
{
    // This is the number of tests between 2 calls to the progress bar
    const int delta = 100;
    int       ii = 0;

    reportAux( "Testing %d tracks & vias...", m_board->Tracks().size() );

    std::map< std::pair<BOARD_ITEM*, BOARD_ITEM*>, int> checkedPairs;

    for( TRACK* track : m_board->Tracks() )
    {
        if( !reportProgress( ii++, m_board->Tracks().size(), delta ) )
            break;

        for( PCB_LAYER_ID layer : track->GetLayerSet().Seq() )
        {
            std::shared_ptr<SHAPE> trackShape = track->GetEffectiveShape( layer );

            m_copperTree.QueryColliding( track, layer, layer,
                    // Filter:
                    [&]( BOARD_ITEM* other ) -> bool
                    {
                        // It would really be better to know what particular nets a nettie
                        // should allow, but for now it is what it is.
                        if( DRC_ENGINE::IsNetTie( other ) )
                            return false;

                        auto otherCItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( other );

                        if( otherCItem && otherCItem->GetNetCode() == track->GetNetCode() )
                            return false;

                        BOARD_ITEM* a = track;
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
                        return testTrackAgainstItem( track, trackShape.get(), layer, other );
                    },
                    m_largestClearance );

            testItemAgainstZones( track, layer );
        }
    }
}


bool DRC_TEST_PROVIDER_COPPER_CLEARANCE::testPadAgainstItem( PAD* pad, SHAPE* padShape,
                                                             PCB_LAYER_ID layer,
                                                             BOARD_ITEM* other )
{
    bool testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
    bool testShorting = !m_drcEngine->IsErrorLimitExceeded( DRCE_SHORTING_ITEMS );
    bool testHoles = !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE );

    // Disable some tests *within* a single footprint
    if( other->GetParent() == pad->GetParent() )
    {
        FOOTPRINT* fp = static_cast<FOOTPRINT*>( pad->GetParent() );

        // Graphic items are allowed to act as net-ties within their own footprint
        if( fp->IsNetTie() && ( other->Type() == PCB_FP_SHAPE_T || other->Type() == PCB_PAD_T ) )
            testClearance = false;

        // CADSTAR implements thermal vias using pads insteads of vias
        if( fp->AllowThermalPads() && other->Type() == PCB_PAD_T )
            testHoles = false;
    }

    if( pad->GetAttribute() == PAD_ATTRIB_NPTH && !pad->FlashLayer( layer ) )
        testClearance = false;

    if( !IsCopperLayer( layer ) )
        testClearance = false;

    // Track clearances are tested in testTrackClearances()
    if( dynamic_cast<TRACK*>( other) )
        testClearance = false;

    if( !testClearance && !testShorting && !testHoles )
        return false;

    std::shared_ptr<SHAPE> otherShape = DRC_ENGINE::GetShape( other, layer );
    DRC_CONSTRAINT         constraint;
    int                    clearance;
    int                    actual;
    VECTOR2I               pos;

    if( other->Type() == PCB_PAD_T )
    {
        PAD* otherPad = static_cast<PAD*>( other );

        // If pads are equivalent (ie: from the same footprint with the same pad number)...
        if( pad->SameLogicalPadAs( otherPad ) )
        {
            // ...and have nets, then they must be the same net
            if( pad->GetNetCode() && otherPad->GetNetCode()
                    && pad->GetNetCode() != otherPad->GetNetCode()
                    && testShorting )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );

                m_msg.Printf( _( "(nets %s and %s)" ),
                              pad->GetNetname(),
                              otherPad->GetNetname() );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                drce->SetItems( pad, otherPad );

                reportViolation( drce, otherPad->GetPosition() );
            }

            return true;
        }

        if( testHoles && pad->FlashLayer( layer ) && otherPad->GetDrillSize().x )
        {
            constraint = m_drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, pad, otherPad, layer );
            clearance = constraint.GetValue().Min();

            if( clearance >= 0 && padShape->Collide( otherPad->GetEffectiveHoleShape(),
                                                     std::max( 0, clearance - m_drcEpsilon ),
                                                     &actual, &pos ) )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );

                m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), clearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                drce->SetItems( pad, other );
                drce->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drce, (wxPoint) pos );
            }
        }

        if( testHoles && otherPad->FlashLayer( layer ) && pad->GetDrillSize().x )
        {
            constraint = m_drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, pad, otherPad, layer );
            clearance = constraint.GetValue().Min();

            if( clearance >= 0 && otherShape->Collide( pad->GetEffectiveHoleShape(),
                                                       std::max( 0, clearance - m_drcEpsilon ),
                                                       &actual, &pos ) )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );

                m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), clearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                drce->SetItems( pad, other );
                drce->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drce, (wxPoint) pos );
            }
        }

        // Pads of the same (defined) net get a waiver on clearance tests
        if( pad->GetNetCode() && otherPad->GetNetCode() == pad->GetNetCode() )
            testClearance = false;

        if( otherPad->GetAttribute() == PAD_ATTRIB_NPTH && !otherPad->FlashLayer( layer ) )
            testClearance = false;
    }

    if( testClearance )
    {
        constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, pad, other, layer );
        clearance = constraint.GetValue().Min();

        if( clearance > 0 && padShape->Collide( otherShape.get(),
                                                std::max( 0, clearance - m_drcEpsilon ),
                                                &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );

            m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), clearance ),
                          MessageTextFromValue( userUnits(), actual ) );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
            drce->SetItems( pad, other );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, (wxPoint) pos );
        }
    }

    return true;
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testPadClearances( )
{
    const int delta = 50;  // This is the number of tests between 2 calls to the progress bar

    size_t count = 0;

    for( FOOTPRINT* footprint : m_board->Footprints() )
        count += footprint->Pads().size();

    reportAux( "Testing %d pads...", count );

    int ii = 0;
    std::map< std::pair<BOARD_ITEM*, BOARD_ITEM*>, int> checkedPairs;

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( !reportProgress( ii++, count, delta ) )
                break;

            for( PCB_LAYER_ID layer : pad->GetLayerSet().Seq() )
            {
                std::shared_ptr<SHAPE> padShape = DRC_ENGINE::GetShape( pad, layer );

                m_copperTree.QueryColliding( pad, layer, layer,
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
                        // Visitor
                        [&]( BOARD_ITEM* other ) -> bool
                        {
                            return testPadAgainstItem( pad, padShape.get(), layer, other );
                        },
                        m_largestClearance );

                testItemAgainstZones( pad, layer );
            }
        }
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testZones()
{
    const int delta = 50;  // This is the number of tests between 2 calls to the progress bar

    SHAPE_POLY_SET  buffer;
    SHAPE_POLY_SET* boardOutline = nullptr;

    if( m_board->GetBoardPolygonOutlines( buffer ) )
        boardOutline = &buffer;

    for( int layer_id = F_Cu; layer_id <= B_Cu; ++layer_id )
    {
        PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( layer_id );
        std::vector<SHAPE_POLY_SET> smoothed_polys;
        smoothed_polys.resize( m_zones.size() );

        // Skip over layers not used on the current board
        if( !m_board->IsLayerEnabled( layer ) )
            continue;

        for( size_t ii = 0; ii < m_zones.size(); ii++ )
        {
            if( m_zones[ii]->IsOnLayer( layer ) )
                m_zones[ii]->BuildSmoothedPoly( smoothed_polys[ii], layer, boardOutline );
        }

        // iterate through all areas
        for( size_t ia = 0; ia < m_zones.size(); ia++ )
        {
            if( !reportProgress( layer_id * m_zones.size() + ia, B_Cu * m_zones.size(), delta ) )
                break;

            ZONE* zoneRef = m_zones[ia];

            if( !zoneRef->IsOnLayer( layer ) )
                continue;

            // If we are testing a single zone, then iterate through all other zones
            // Otherwise, we have already tested the zone combination
            for( size_t ia2 = ia + 1; ia2 < m_zones.size(); ia2++ )
            {
                ZONE* zoneToTest = m_zones[ia2];

                if( zoneRef == zoneToTest )
                    continue;

                // test for same layer
                if( !zoneToTest->IsOnLayer( layer ) )
                    continue;

                // Test for same net
                if( zoneRef->GetNetCode() == zoneToTest->GetNetCode() && zoneRef->GetNetCode() >= 0 )
                    continue;

                // test for different priorities
                if( zoneRef->GetPriority() != zoneToTest->GetPriority() )
                    continue;

                // rule areas may overlap at will
                if( zoneRef->GetIsRuleArea() || zoneToTest->GetIsRuleArea() )
                    continue;

                // Examine a candidate zone: compare zoneToTest to zoneRef

                // Get clearance used in zone to zone test.
                auto constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, zoneRef, zoneToTest,
                                                          layer );
                int  zone2zoneClearance = constraint.GetValue().Min();

                // test for some corners of zoneRef inside zoneToTest
                for( auto iterator = smoothed_polys[ia].IterateWithHoles(); iterator; iterator++ )
                {
                    VECTOR2I currentVertex = *iterator;
                    wxPoint pt( currentVertex.x, currentVertex.y );

                    if( smoothed_polys[ia2].Contains( currentVertex ) )
                    {
                        std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_ZONES_INTERSECT );
                        drce->SetItems( zoneRef, zoneToTest );
                        drce->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drce, pt );
                    }
                }

                // test for some corners of zoneToTest inside zoneRef
                for( auto iterator = smoothed_polys[ia2].IterateWithHoles(); iterator; iterator++ )
                {
                    VECTOR2I currentVertex = *iterator;
                    wxPoint pt( currentVertex.x, currentVertex.y );

                    if( smoothed_polys[ia].Contains( currentVertex ) )
                    {
                        std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_ZONES_INTERSECT );
                        drce->SetItems( zoneToTest, zoneRef );
                        drce->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drce, pt );
                    }
                }

                // Iterate through all the segments of refSmoothedPoly
                std::map<wxPoint, int> conflictPoints;

                for( auto refIt = smoothed_polys[ia].IterateSegmentsWithHoles(); refIt; refIt++ )
                {
                    // Build ref segment
                    SEG refSegment = *refIt;

                    // Iterate through all the segments in smoothed_polys[ia2]
                    for( auto testIt = smoothed_polys[ia2].IterateSegmentsWithHoles(); testIt; testIt++ )
                    {
                        // Build test segment
                        SEG testSegment = *testIt;
                        wxPoint pt;

                        int ax1, ay1, ax2, ay2;
                        ax1 = refSegment.A.x;
                        ay1 = refSegment.A.y;
                        ax2 = refSegment.B.x;
                        ay2 = refSegment.B.y;

                        int bx1, by1, bx2, by2;
                        bx1 = testSegment.A.x;
                        by1 = testSegment.A.y;
                        bx2 = testSegment.B.x;
                        by2 = testSegment.B.y;

                        int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2,
                                                             0,
                                                             ax1, ay1, ax2, ay2,
                                                             0,
                                                             zone2zoneClearance,
                                                             &pt.x, &pt.y );

                        if( d < zone2zoneClearance )
                        {
                            if( conflictPoints.count( pt ) )
                                conflictPoints[ pt ] = std::min( conflictPoints[ pt ], d );
                            else
                                conflictPoints[ pt ] = d;
                        }
                    }
                }

                for( const std::pair<const wxPoint, int>& conflict : conflictPoints )
                {
                    int       actual = conflict.second;
                    std::shared_ptr<DRC_ITEM> drce;

                    if( actual <= 0 )
                    {
                        drce = DRC_ITEM::Create( DRCE_ZONES_INTERSECT );
                    }
                    else
                    {
                        drce = DRC_ITEM::Create( DRCE_CLEARANCE );

                        m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                                      constraint.GetName(),
                                      MessageTextFromValue( userUnits(), zone2zoneClearance ),
                                      MessageTextFromValue( userUnits(), conflict.second ) );

                        drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                    }

                    drce->SetItems( zoneRef, zoneToTest );
                    drce->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drce, conflict.first );
                }
            }
        }
    }
}


int DRC_TEST_PROVIDER_COPPER_CLEARANCE::GetNumPhases() const
{
    return 4;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_COPPER_CLEARANCE::GetConstraintTypes() const
{
    return { CLEARANCE_CONSTRAINT, HOLE_CLEARANCE_CONSTRAINT };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_COPPER_CLEARANCE> dummy;
}

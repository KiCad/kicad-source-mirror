/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2022 KiCad Developers.
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
#include <math_for_graphics.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>

#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_segment.h>

#include <drc/drc_engine.h>
#include <drc/drc_rtree.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>
#include <pcb_dimension.h>

/*
    Copper clearance test. Checks all copper items (pads, vias, tracks, drawings, zones) for their
    electrical clearance.

    Errors generated:
    - DRCE_CLEARANCE
    - DRCE_HOLE_CLEARANCE
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
        return wxT( "clearance" );
    };

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests copper item clearance" );
    }

private:
    bool testTrackAgainstItem( PCB_TRACK* track, SHAPE* trackShape, PCB_LAYER_ID layer,
                               BOARD_ITEM* other );

    void testTrackClearances();

    bool testPadAgainstItem( PAD* pad, SHAPE* padShape, PCB_LAYER_ID layer, BOARD_ITEM* other );

    void testPadClearances();

    void testZonesToZones();

    void testItemAgainstZone( BOARD_ITEM* aItem, ZONE* aZone, PCB_LAYER_ID aLayer );

private:
    int m_drcEpsilon;
};


bool DRC_TEST_PROVIDER_COPPER_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();

    if( m_board->m_DRCMaxClearance <= 0 )
    {
        reportAux( wxT( "No Clearance constraints found. Tests not run." ) );
        return true;   // continue with other tests
    }

    m_drcEpsilon = m_board->GetDesignSettings().GetDRCEpsilon();

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

        testZonesToZones();
    }
    else if( !m_drcEngine->IsErrorLimitExceeded( DRCE_ZONES_INTERSECT ) )
    {
        if( !reportPhase( _( "Checking zones..." ) ) )
            return false;   // DRC cancelled

        testZonesToZones();
    }

    reportRuleStatistics();

    return !m_drcEngine->IsCancelled();
}


bool DRC_TEST_PROVIDER_COPPER_CLEARANCE::testTrackAgainstItem( PCB_TRACK* track, SHAPE* trackShape,
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

        if( pad->GetAttribute() == PAD_ATTRIB::NPTH && !pad->FlashLayer( layer ) )
            testClearance = false;
    }

    if( testClearance )
    {
        constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, track, other, layer );
        clearance = constraint.GetValue().Min();
    }

    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
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

                reportViolation( drcItem, intersection.get(), layer );

                return m_drcEngine->GetReportAllTrackErrors();
            }
        }

        std::shared_ptr<SHAPE> otherShape = other->GetEffectiveShape( layer );

        if( trackShape->Collide( otherShape.get(), clearance - m_drcEpsilon, &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );
            wxString msg;

            msg.Printf( _( "(%s clearance %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), clearance ),
                          MessageTextFromValue( userUnits(), actual ) );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
            drce->SetItems( track, other );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, pos, layer );

            if( !m_drcEngine->GetReportAllTrackErrors() )
                return false;
        }
    }

    if( testHoles && other->HasHole() )
    {
        std::shared_ptr<SHAPE_SEGMENT> holeShape;

        if( other->Type() == PCB_VIA_T )
        {
            if( other->GetLayerSet().Contains( layer ) )
                holeShape = other->GetEffectiveHoleShape();
        }
        else
        {
            holeShape = other->GetEffectiveHoleShape();
        }

        if( holeShape )
        {
            constraint = m_drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, other, track, layer );
            clearance = constraint.GetValue().Min();

            if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
            {
                if( trackShape->Collide( holeShape.get(), std::max( 0, clearance - m_drcEpsilon ),
                                         &actual, &pos ) )
                {
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                    wxString msg;

                    msg.Printf( _( "(%s clearance %s; actual %s)" ),
                                  constraint.GetName(),
                                  MessageTextFromValue( userUnits(), clearance ),
                                  MessageTextFromValue( userUnits(), actual ) );

                    drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                    drce->SetItems( track, other );
                    drce->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drce, pos, layer );

                    if( !m_drcEngine->GetReportAllTrackErrors() )
                        return false;
                }
            }
        }
    }

    return !m_drcEngine->IsCancelled();
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testItemAgainstZone( BOARD_ITEM* aItem, ZONE* aZone,
                                                              PCB_LAYER_ID aLayer )
{
    if( !aZone->GetLayerSet().test( aLayer ) )
        return;

    if( aZone->GetNetCode() && aItem->IsConnected() )
    {
        if( aZone->GetNetCode() == static_cast<BOARD_CONNECTED_ITEM*>( aItem )->GetNetCode() )
            return;
    }

    if( !aItem->GetBoundingBox().Intersects( aZone->GetCachedBoundingBox() ) )
        return;

    bool testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
    bool testHoles = !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE );

    if( !testClearance && !testHoles )
        return;

    DRC_RTREE*       zoneTree = m_board->m_CopperZoneRTreeCache[ aZone ].get();
    EDA_RECT         itemBBox = aItem->GetBoundingBox();
    DRC_CONSTRAINT   constraint;
    DRC_CONSTRAINT_T constraintType = CLEARANCE_CONSTRAINT;
    int              clearance = -1;
    int              actual;
    VECTOR2I         pos;
    bool             unflashedPad = false;
    bool             platedHole = false;

    if( aItem->Type() == PCB_PAD_T )
    {
        unflashedPad = !static_cast<PAD*>( aItem )->FlashLayer( aLayer );

        if( unflashedPad && !aItem->HasHole() )
            return;

        platedHole = static_cast<PAD*>( aItem )->GetAttribute() == PAD_ATTRIB::PTH;
    }

    if( zoneTree && testClearance )
    {
        if( unflashedPad && !platedHole )
            constraintType = HOLE_CLEARANCE_CONSTRAINT;

        constraint = m_drcEngine->EvalRules( constraintType, aItem, aZone, aLayer );
        clearance = constraint.GetValue().Min();
    }

    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
    {
        std::shared_ptr<SHAPE> itemShape = aItem->GetEffectiveShape( aLayer );

        if( unflashedPad )
        {
            std::shared_ptr<SHAPE_SEGMENT> hole = aItem->GetEffectiveHoleShape();
            int                            size = hole->GetWidth();

            // Note: drill size represents finish size, which means the actual hole size is
            // 2x the plating thickness larger.
            if( platedHole )
                size += 2 * m_board->GetDesignSettings().GetHolePlatingThickness();

            itemShape = std::make_shared<SHAPE_SEGMENT>( hole->GetSeg(), size );
        }

        if( zoneTree && zoneTree->QueryColliding( itemBBox, itemShape.get(), aLayer,
                                                  std::max( 0, clearance - m_drcEpsilon ),
                                                  &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );
            wxString msg;

            msg.Printf( _( "(%s clearance %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), clearance ),
                          MessageTextFromValue( userUnits(), actual ) );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
            drce->SetItems( aItem, aZone );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, pos, aLayer );
        }
    }

    if( zoneTree && testHoles && aItem->HasHole() )
    {
        std::shared_ptr<SHAPE_SEGMENT> holeShape;

        if( aItem->Type() == PCB_VIA_T )
        {
            if( aItem->GetLayerSet().Contains( aLayer ) )
                holeShape = aItem->GetEffectiveHoleShape();
        }
        else
        {
            holeShape = aItem->GetEffectiveHoleShape();
        }

        if( holeShape )
        {
            constraint = m_drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, aItem, aZone, aLayer );
            clearance = constraint.GetValue().Min();

            if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
            {
                if( zoneTree->QueryColliding( itemBBox, holeShape.get(), aLayer,
                                              std::max( 0, clearance - m_drcEpsilon ),
                                              &actual, &pos ) )
                {
                    std::shared_ptr<DRC_ITEM>  drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                    wxString msg;

                    msg.Printf( _( "(%s clearance %s; actual %s)" ),
                                  constraint.GetName(),
                                  MessageTextFromValue( userUnits(), clearance ),
                                  MessageTextFromValue( userUnits(), actual ) );

                    drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                    drce->SetItems( aItem, aZone );
                    drce->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drce, pos, aLayer );
                }
            }
        }
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testTrackClearances()
{
    // This is the number of tests between 2 calls to the progress bar
    const int progressDelta = 100;
    int       ii = 0;

    reportAux( wxT( "Testing %d tracks & vias..." ), m_board->Tracks().size() );

    std::unordered_map<PTR_PTR_CACHE_KEY, int> checkedPairs;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( !reportProgress( ii++, m_board->Tracks().size(), progressDelta ) )
            break;

        for( PCB_LAYER_ID layer : track->GetLayerSet().Seq() )
        {
            std::shared_ptr<SHAPE> trackShape = track->GetEffectiveShape( layer );

            m_board->m_CopperItemRTreeCache->QueryColliding( track, layer, layer,
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
                    m_board->m_DRCMaxClearance );

            for( ZONE* zone : m_board->m_DRCCopperZones )
            {
                testItemAgainstZone( track, zone, layer );

                if( m_drcEngine->IsCancelled() )
                    break;
            }
        }
    }
}


bool DRC_TEST_PROVIDER_COPPER_CLEARANCE::testPadAgainstItem( PAD* pad, SHAPE* padShape,
                                                             PCB_LAYER_ID aLayer,
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

        // No hole testing within a footprint
        testHoles = false;
    }

    PAD*     otherPad = nullptr;
    PCB_VIA* otherVia = nullptr;

    if( other->Type() == PCB_PAD_T )
        otherPad = static_cast<PAD*>( other );

    if( other->Type() == PCB_VIA_T )
        otherVia = static_cast<PCB_VIA*>( other );

    if( !IsCopperLayer( aLayer ) )
        testClearance = false;

    // A NPTH has no cylinder, but it may still have pads on some layers
    if( pad->GetAttribute() == PAD_ATTRIB::NPTH && !pad->FlashLayer( aLayer ) )
        testClearance = false;

    if( otherPad && otherPad->GetAttribute() == PAD_ATTRIB::NPTH && !otherPad->FlashLayer( aLayer ) )
        testClearance = false;

    // Track clearances are tested in testTrackClearances()
    if( dynamic_cast<PCB_TRACK*>( other) )
        testClearance = false;

    int padNet = pad->GetNetCode();
    int otherPadNet = otherPad ? otherPad->GetNetCode() : 0;
    int otherViaNet = otherVia ? otherVia->GetNetCode() : 0;

    // Pads and vias of the same (defined) net get a waiver on clearance and hole tests
    if( ( otherPadNet && otherPadNet == padNet ) || ( otherViaNet && otherViaNet == padNet ) )
    {
        testClearance = false;
        testHoles = false;
    }

    if( !( pad->GetDrillSize().x > 0 )
            && !( otherPad && otherPad->GetDrillSize().x > 0 )
            && !( otherVia && otherVia->GetDrill() > 0 ) )
    {
        testHoles = false;
    }

    if( !testClearance && !testShorting && !testHoles )
        return false;

    std::shared_ptr<SHAPE> otherShape = other->GetEffectiveShape( aLayer );
    DRC_CONSTRAINT         constraint;
    int                    clearance;
    int                    actual;
    VECTOR2I               pos;

    if( otherPad && pad->SameLogicalPadAs( otherPad ) )
    {
        // If pads are equivalent (ie: from the same footprint with the same pad number)...
        // ... and have nets...
        // then they must be the same net
        if( pad->GetNetCode() && otherPad->GetNetCode()
                && pad->GetNetCode() != otherPad->GetNetCode()
                && testShorting )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );
            wxString msg;

            msg.Printf( _( "(nets %s and %s)" ),
                          pad->GetNetname(),
                          otherPad->GetNetname() );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
            drce->SetItems( pad, otherPad );

            reportViolation( drce, otherPad->GetPosition(), aLayer );
        }

        return !m_drcEngine->IsCancelled();
    }

    if( testClearance )
    {
        constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, pad, other, aLayer );
        clearance = constraint.GetValue().Min();

        if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
        {
            if( padShape->Collide( otherShape.get(), std::max( 0, clearance - m_drcEpsilon ),
                                   &actual, &pos ) )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );
                wxString msg;

                msg.Printf( _( "(%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), clearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                drce->SetItems( pad, other );
                drce->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drce, pos, aLayer );
                testHoles = false;  // No need for multiple violations
            }
        }
    }

    if( testHoles )
    {
        constraint = m_drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, pad, other, aLayer );
        clearance = constraint.GetValue().Min();

        if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
            testHoles = false;
    }

    if( testHoles && otherPad && pad->FlashLayer( aLayer ) && otherPad->HasHole() )
    {
        if( clearance > 0 && padShape->Collide( otherPad->GetEffectiveHoleShape().get(),
                                                std::max( 0, clearance - m_drcEpsilon ),
                                                &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
            wxString msg;

            msg.Printf( _( "(%s clearance %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), clearance ),
                          MessageTextFromValue( userUnits(), actual ) );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
            drce->SetItems( pad, other );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, pos, aLayer );
            testHoles = false;  // No need for multiple violations
        }
    }

    if( testHoles && otherPad && otherPad->FlashLayer( aLayer ) && pad->HasHole() )
    {
        if( clearance > 0 && otherShape->Collide( pad->GetEffectiveHoleShape().get(),
                                                  std::max( 0, clearance - m_drcEpsilon ),
                                                  &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
            wxString msg;

            msg.Printf( _( "(%s clearance %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), clearance ),
                          MessageTextFromValue( userUnits(), actual ) );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
            drce->SetItems( pad, other );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, pos, aLayer );
            testHoles = false;  // No need for multiple violations
        }
    }

    if( testHoles && otherVia && otherVia->IsOnLayer( aLayer ) )
    {
        if( clearance > 0 && padShape->Collide( otherVia->GetEffectiveHoleShape().get(),
                                                std::max( 0, clearance - m_drcEpsilon ),
                                                &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
            wxString msg;

            msg.Printf( _( "(%s clearance %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), clearance ),
                          MessageTextFromValue( userUnits(), actual ) );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
            drce->SetItems( pad, otherVia );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, pos, aLayer );
        }
    }

    return !m_drcEngine->IsCancelled();
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testPadClearances( )
{
    const int progressDelta = 100;
    size_t    count = 0;
    int       ii = 0;

    for( FOOTPRINT* footprint : m_board->Footprints() )
        count += footprint->Pads().size();

    reportAux( wxT( "Testing %d pads..." ), count );

    std::unordered_map<PTR_PTR_CACHE_KEY, int> checkedPairs;

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            for( PCB_LAYER_ID layer : pad->GetLayerSet().Seq() )
            {
                std::shared_ptr<SHAPE> padShape = pad->GetEffectiveShape( layer );

                m_board->m_CopperItemRTreeCache->QueryColliding( pad, layer, layer,
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
                        m_board->m_DRCMaxClearance );

                for( ZONE* zone : m_board->m_DRCCopperZones )
                {
                    testItemAgainstZone( pad, zone, layer );

                    if( m_drcEngine->IsCancelled() )
                        return;
                }
            }

            if( !reportProgress( ii++, count, progressDelta ) )
                return;
        }

        if( m_drcEngine->IsCancelled() )
            return;
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testZonesToZones()
{
    const int progressDelta = 50;

    SHAPE_POLY_SET  buffer;
    SHAPE_POLY_SET* boardOutline = nullptr;
    DRC_CONSTRAINT  constraint;
    int             zone2zoneClearance;

    if( m_board->GetBoardPolygonOutlines( buffer ) )
        boardOutline = &buffer;

    for( int layer_id = F_Cu; layer_id <= B_Cu; ++layer_id )
    {
        PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( layer_id );
        std::vector<SHAPE_POLY_SET> smoothed_polys;
        smoothed_polys.resize( m_board->m_DRCCopperZones.size() );

        // Skip over layers not used on the current board
        if( !m_board->IsLayerEnabled( layer ) )
            continue;

        for( size_t ii = 0; ii < m_board->m_DRCCopperZones.size(); ii++ )
        {
            if( m_board->m_DRCCopperZones[ii]->IsOnLayer( layer ) )
            {
                m_board->m_DRCCopperZones[ii]->BuildSmoothedPoly( smoothed_polys[ii], layer,
                                                                  boardOutline );
            }
        }

        // iterate through all areas
        for( size_t ia = 0; ia < m_board->m_DRCCopperZones.size(); ia++ )
        {
            if( !reportProgress( layer_id * m_board->m_DRCCopperZones.size() + ia,
                                 B_Cu * m_board->m_DRCCopperZones.size(), progressDelta ) )
            {
                return;     // DRC cancelled
            }

            ZONE* zoneA = m_board->m_DRCCopperZones[ia];

            if( !zoneA->IsOnLayer( layer ) )
                continue;

            for( size_t ia2 = ia + 1; ia2 < m_board->m_DRCCopperZones.size(); ia2++ )
            {
                ZONE* zoneB = m_board->m_DRCCopperZones[ia2];

                // test for same layer
                if( !zoneB->IsOnLayer( layer ) )
                    continue;

                // Test for same net
                if( zoneA->GetNetCode() == zoneB->GetNetCode() && zoneA->GetNetCode() >= 0 )
                    continue;

                // test for different priorities
                if( zoneA->GetAssignedPriority() != zoneB->GetAssignedPriority() )
                    continue;

                // rule areas may overlap at will
                if( zoneA->GetIsRuleArea() || zoneB->GetIsRuleArea() )
                    continue;

                // Examine a candidate zone: compare zoneB to zoneA

                // Get clearance used in zone to zone test.
                constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, zoneA, zoneB, layer );
                zone2zoneClearance = constraint.GetValue().Min();

                if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
                    continue;

                // test for some corners of zoneA inside zoneB
                for( auto iterator = smoothed_polys[ia].IterateWithHoles(); iterator; iterator++ )
                {
                    VECTOR2I currentVertex = *iterator;
                    wxPoint pt( currentVertex.x, currentVertex.y );

                    if( smoothed_polys[ia2].Contains( currentVertex ) )
                    {
                        std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_ZONES_INTERSECT );
                        drce->SetItems( zoneA, zoneB );
                        drce->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drce, pt, layer );
                    }
                }

                // test for some corners of zoneB inside zoneA
                for( auto iterator = smoothed_polys[ia2].IterateWithHoles(); iterator; iterator++ )
                {
                    VECTOR2I currentVertex = *iterator;
                    wxPoint pt( currentVertex.x, currentVertex.y );

                    if( smoothed_polys[ia].Contains( currentVertex ) )
                    {
                        std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_ZONES_INTERSECT );
                        drce->SetItems( zoneB, zoneA );
                        drce->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drce, pt, layer );
                    }
                }

                // Iterate through all the segments of refSmoothedPoly
                std::map<VECTOR2I, int> conflictPoints;

                for( auto refIt = smoothed_polys[ia].IterateSegmentsWithHoles(); refIt; refIt++ )
                {
                    // Build ref segment
                    SEG refSegment = *refIt;

                    // Iterate through all the segments in smoothed_polys[ia2]
                    for( auto testIt = smoothed_polys[ia2].IterateSegmentsWithHoles(); testIt; testIt++ )
                    {
                        // Build test segment
                        SEG testSegment = *testIt;
                        VECTOR2I pt;

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

                        int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2, 0,
                                                             ax1, ay1, ax2, ay2, 0,
                                                             zone2zoneClearance, &pt.x, &pt.y );

                        if( d < zone2zoneClearance )
                        {
                            if( conflictPoints.count( pt ) )
                                conflictPoints[ pt ] = std::min( conflictPoints[ pt ], d );
                            else
                                conflictPoints[ pt ] = d;
                        }
                    }
                }

                for( const std::pair<const VECTOR2I, int>& conflict : conflictPoints )
                {
                    int actual = conflict.second;
                    std::shared_ptr<DRC_ITEM> drce;

                    if( actual <= 0 )
                    {
                        drce = DRC_ITEM::Create( DRCE_ZONES_INTERSECT );
                    }
                    else
                    {
                        drce = DRC_ITEM::Create( DRCE_CLEARANCE );
                        wxString msg;

                        msg.Printf( _( "(%s clearance %s; actual %s)" ),
                                      constraint.GetName(),
                                      MessageTextFromValue( userUnits(), zone2zoneClearance ),
                                      MessageTextFromValue( userUnits(), conflict.second ) );

                        drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                    }

                    drce->SetItems( zoneA, zoneB );
                    drce->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drce, conflict.first, layer );
                }

                if( m_drcEngine->IsCancelled() )
                    return;
            }
        }
    }
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_COPPER_CLEARANCE> dummy;
}

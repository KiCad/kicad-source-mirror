/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2024 KiCad Developers.
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
#include <core/thread_pool.h>
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

#include <future>

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
    /**
     * Checks for track/via/hole <-> clearance
     * @param item Track to text
     * @param itemShape Primitive track shape
     * @param layer Which layer to test (in case of vias this can be multiple
     * @param other item against which to test the track item
     * @return false if there is a clearance violation reported, true if there is none
     */
    bool testSingleLayerItemAgainstItem( BOARD_CONNECTED_ITEM* item, SHAPE* itemShape,
                                         PCB_LAYER_ID layer, BOARD_ITEM* other );

    void testTrackClearances();

    void testPadAgainstItem( PAD* pad, SHAPE* padShape, PCB_LAYER_ID layer, BOARD_ITEM* other );

    void testPadClearances();

    void testGraphicClearances();

    void testZonesToZones();

    void testItemAgainstZone( BOARD_ITEM* aItem, ZONE* aZone, PCB_LAYER_ID aLayer );

    void testKnockoutTextAgainstZone( BOARD_ITEM* aText, NETINFO_ITEM** aInheritedNet, ZONE* aZone );

    typedef struct checked
    {
        checked()
            : layers(), has_error( false ) {}

        checked( PCB_LAYER_ID aLayer )
            : layers( aLayer ), has_error( false ) {}

        LSET layers;
        bool has_error;
    } layers_checked;

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
        if( !reportPhase( _( "Checking copper graphic clearances..." ) ) )
            return false;   // DRC cancelled

        testGraphicClearances();
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


bool DRC_TEST_PROVIDER_COPPER_CLEARANCE::testSingleLayerItemAgainstItem( BOARD_CONNECTED_ITEM* item,
                                                                         SHAPE* itemShape,
                                                                         PCB_LAYER_ID layer,
                                                                         BOARD_ITEM* other )
{
    bool           testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
    bool           testShorting = !m_drcEngine->IsErrorLimitExceeded( DRCE_SHORTING_ITEMS );
    bool           testHoles = !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE );
    DRC_CONSTRAINT constraint;
    int            clearance = -1;
    int            actual;
    VECTOR2I       pos;
    bool           has_error = false;
    int            otherNet = 0;

    if( BOARD_CONNECTED_ITEM* connectedItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( other ) )
        otherNet = connectedItem->GetNetCode();

    std::shared_ptr<SHAPE> otherShape = other->GetEffectiveShape( layer );

    if( other->Type() == PCB_PAD_T )
    {
        PAD* pad = static_cast<PAD*>( other );

        if( pad->GetAttribute() == PAD_ATTRIB::NPTH && !pad->FlashLayer( layer ) )
            testClearance = testShorting = false;
    }

    if( testClearance || testShorting )
    {
        constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, item, other, layer );
        clearance = constraint.GetValue().Min();
    }

    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
    {
        // Special processing for track:track intersections
        if( item->Type() == PCB_TRACE_T && other->Type() == PCB_TRACE_T )
        {
            PCB_TRACK* track = static_cast<PCB_TRACK*>( item );
            PCB_TRACK* otherTrack = static_cast<PCB_TRACK*>( other );

            SEG trackSeg( track->GetStart(), track->GetEnd() );
            SEG otherSeg( otherTrack->GetStart(), otherTrack->GetEnd() );

            if( OPT_VECTOR2I intersection = trackSeg.Intersect( otherSeg ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TRACKS_CROSSING );
                drcItem->SetItems( item, other );
                drcItem->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drcItem, *intersection, layer );

                return false;
            }
        }

        if( itemShape->Collide( otherShape.get(), clearance - m_drcEpsilon, &actual, &pos ) )
        {
            if( m_drcEngine->IsNetTieExclusion( item->GetNetCode(), layer, pos, other ) )
            {
                // Collision occurred as track was entering a pad marked as a net-tie.  We
                // allow these.
            }
            else if( actual == 0 && otherNet && testShorting )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );
                wxString msg;

                msg.Printf( _( "(nets %s and %s)" ), item->GetNetname(),
                            static_cast<BOARD_CONNECTED_ITEM*>( other )->GetNetname() );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                drce->SetItems( item, other );

                reportViolation( drce, pos, layer );
                has_error = true;

                if( !m_drcEngine->GetReportAllTrackErrors() )
                    return false;
            }
            else if( testClearance )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );
                wxString msg = formatMsg( _( "(%s clearance %s; actual %s)" ),
                                          constraint.GetName(),
                                          clearance,
                                          actual );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                drce->SetItems( item, other );
                drce->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drce, pos, layer );
                has_error = true;

                if( !m_drcEngine->GetReportAllTrackErrors() )
                    return false;
            }
        }
    }

    if( testHoles && ( item->HasHole() || other->HasHole() ) )
    {
        std::array<BOARD_ITEM*, 2> a{ item, other };
        std::array<BOARD_ITEM*, 2> b{ other, item };
        std::array<SHAPE*, 2>      a_shape{ itemShape, otherShape.get() };

        for( size_t ii = 0; ii < 2; ++ii )
        {
            std::shared_ptr<SHAPE_SEGMENT> holeShape;

            // We only test a track item here against an item with a hole.
            // If either case is not valid, simply move on
            if( !( dynamic_cast<PCB_TRACK*>( a[ii] ) ) || !b[ii]->HasHole() )
            {
                continue;
            }
            if( b[ii]->Type() == PCB_VIA_T )
            {
                if( b[ii]->GetLayerSet().Contains( layer ) )
                    holeShape = b[ii]->GetEffectiveHoleShape();
            }
            else
            {
                holeShape = b[ii]->GetEffectiveHoleShape();
            }

            constraint = m_drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, b[ii], a[ii], layer );
            clearance = constraint.GetValue().Min();

            // Test for hole to item clearance even if clearance is 0, because the item cannot be
            // inside (or intersect) the hole.
            if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
            {
                if( a_shape[ii]->Collide( holeShape.get(), std::max( 0, clearance - m_drcEpsilon ),
                                          &actual, &pos ) )
                {
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                    wxString msg = formatMsg( clearance ? _( "(%s clearance %s; actual %s)" )
                                                        : _( "(%s clearance %s; actual < 0)" ),
                                              constraint.GetName(),
                                              clearance,
                                              actual );

                    drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                    drce->SetItems( a[ii], b[ii] );
                    drce->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drce, pos, layer );
                    return false;
                }
            }
        }
    }

    return !has_error;
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

    BOX2I itemBBox = aItem->GetBoundingBox();
    BOX2I worstCaseBBox = itemBBox;

    worstCaseBBox.Inflate( m_board->m_DRCMaxClearance );

    if( !worstCaseBBox.Intersects( aZone->GetBoundingBox() ) )
        return;

    FOOTPRINT* parentFP = aItem->GetParentFootprint();

    // Ignore graphic items which implement a net-tie to the zone's net on the layer being tested.
    if( parentFP && parentFP->IsNetTie() && dynamic_cast<PCB_SHAPE*>( aItem ) )
    {
        std::set<PAD*> allowedNetTiePads;

        for( PAD* pad : parentFP->Pads() )
        {
            if( pad->GetNetCode() == aZone->GetNetCode() && aZone->GetNetCode() != 0 )
            {
                if( pad->IsOnLayer( aLayer ) )
                    allowedNetTiePads.insert( pad );

                for( PAD* other : parentFP->GetNetTiePads( pad ) )
                {
                    if( other->IsOnLayer( aLayer ) )
                        allowedNetTiePads.insert( other );
                }
            }
        }

        if( !allowedNetTiePads.empty() )
        {
            std::shared_ptr<SHAPE> itemShape = aItem->GetEffectiveShape();

            for( PAD* pad : allowedNetTiePads )
            {
                if( pad->GetBoundingBox().Intersects( itemBBox )
                        && pad->GetEffectiveShape()->Collide( itemShape.get() ) )
                {
                    return;
                }
            }
        }
    }

    bool testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
    bool testHoles = !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE );

    if( !testClearance && !testHoles )
        return;

    DRC_RTREE*  zoneTree = m_board->m_CopperZoneRTreeCache[ aZone ].get();

    if( !zoneTree )
        return;

    DRC_CONSTRAINT constraint;
    int            clearance = -1;
    int            actual;
    VECTOR2I       pos;

    if( aItem->Type() == PCB_PAD_T )
    {
        PAD* pad = static_cast<PAD*>( aItem );
        bool flashedPad = pad->FlashLayer( aLayer );
        bool platedHole = pad->HasHole() && pad->GetAttribute() == PAD_ATTRIB::PTH;

        if( !flashedPad && !platedHole )
            testClearance = false;
    }

    if( testClearance )
    {
        constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, aItem, aZone, aLayer );
        clearance = constraint.GetValue().Min();
    }

    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
    {
        std::shared_ptr<SHAPE> itemShape = aItem->GetEffectiveShape( aLayer, FLASHING::DEFAULT );

        if( zoneTree->QueryColliding( itemBBox, itemShape.get(), aLayer,
                                      std::max( 0, clearance - m_drcEpsilon ), &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );
            wxString msg = formatMsg( _( "(%s clearance %s; actual %s)" ),
                                      constraint.GetName(),
                                      clearance,
                                      actual );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
            drce->SetItems( aItem, aZone );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, pos, aLayer );
        }
    }

    if( testHoles && aItem->HasHole() )
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
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                    wxString msg = formatMsg( _( "(%s clearance %s; actual %s)" ),
                                              constraint.GetName(),
                                              clearance,
                                              actual );

                    drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                    drce->SetItems( aItem, aZone );
                    drce->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drce, pos, aLayer );
                }
            }
        }
    }
}


/*
 * We have to special-case knockout text as it's most often knocked-out of a zone, so it's
 * presumed to collide with one.  However, if it collides with more than one, and they have
 * different nets, then we have a short.
 */
void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testKnockoutTextAgainstZone( BOARD_ITEM* aText,
                                                                      NETINFO_ITEM** aInheritedNet,
                                                                      ZONE* aZone )
{
    bool testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
    bool testShorts = !m_drcEngine->IsErrorLimitExceeded( DRCE_SHORTING_ITEMS );

    if( !testClearance && !testShorts )
        return;

    PCB_LAYER_ID layer = aText->GetLayer();

    if( !aZone->GetLayerSet().test( layer ) )
        return;

    BOX2I itemBBox = aText->GetBoundingBox();
    BOX2I worstCaseBBox = itemBBox;

    worstCaseBBox.Inflate( m_board->m_DRCMaxClearance );

    if( !worstCaseBBox.Intersects( aZone->GetBoundingBox() ) )
        return;

    DRC_RTREE*  zoneTree = m_board->m_CopperZoneRTreeCache[ aZone ].get();

    if( !zoneTree )
        return;

    std::shared_ptr<SHAPE> itemShape = aText->GetEffectiveShape( layer, FLASHING::DEFAULT );

    if( *aInheritedNet == nullptr )
    {
        if( zoneTree->QueryColliding( itemBBox, itemShape.get(), layer ) )
            *aInheritedNet = aZone->GetNet();
    }

    if( *aInheritedNet == aZone->GetNet() )
        return;

    DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, aText, aZone, layer );
    int            clearance = constraint.GetValue().Min();
    int            actual;
    VECTOR2I       pos;

    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance >= 0 )
    {
        if( zoneTree->QueryColliding( itemBBox, itemShape.get(), layer,
                                      std::max( 0, clearance - m_drcEpsilon ), &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drce;
            wxString                  msg;

            if( testShorts && actual == 0 && *aInheritedNet )
            {
                drce = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );
                msg.Printf( _( "(nets %s and %s)" ),
                              ( *aInheritedNet )->GetNetname(),
                              aZone->GetNetname() );
            }
            else
            {
                drce = DRC_ITEM::Create( DRCE_CLEARANCE );
                msg = formatMsg( _( "(%s clearance %s; actual %s)" ),
                                 constraint.GetName(),
                                 clearance,
                                 actual );
            }

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
            drce->SetItems( aText, aZone );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, pos, layer );
        }
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testTrackClearances()
{
    std::map<BOARD_ITEM*, int>                            freePadsUsageMap;
    std::unordered_map<PTR_PTR_CACHE_KEY, layers_checked> checkedPairs;
    std::mutex                                            checkedPairsMutex;
    std::mutex                                            freePadsUsageMapMutex;
    std::atomic<size_t>                                   done( 0 );
    size_t                                                count = m_board->Tracks().size();

    reportAux( wxT( "Testing %d tracks & vias..." ), count );

    LSET boardCopperLayers = LSET::AllCuMask( m_board->GetCopperLayerCount() );

    auto testTrack = [&]( const int start_idx, const int end_idx )
    {
        for( int trackIdx = start_idx; trackIdx < end_idx; ++trackIdx )
        {
            PCB_TRACK* track = m_board->Tracks()[trackIdx];

            for( PCB_LAYER_ID layer : LSET( track->GetLayerSet() & boardCopperLayers ).Seq() )
            {
                std::shared_ptr<SHAPE> trackShape = track->GetEffectiveShape( layer );

                m_board->m_CopperItemRTreeCache->QueryColliding( track, layer, layer,
                        // Filter:
                        [&]( BOARD_ITEM* other ) -> bool
                        {
                            auto otherCItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( other );

                            if( otherCItem && otherCItem->GetNetCode() == track->GetNetCode() )
                                return false;

                            BOARD_ITEM* a = track;
                            BOARD_ITEM* b = other;

                            // store canonical order so we don't collide in both directions
                            // (a:b and b:a)
                            if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                                std::swap( a, b );

                            std::lock_guard<std::mutex> lock( checkedPairsMutex );
                            auto it = checkedPairs.find( { a, b } );

                            if( it != checkedPairs.end() && ( it->second.layers.test( layer )
                                    || ( it->second.has_error && !m_drcEngine->GetReportAllTrackErrors() ) ) )
                            {
                                return false;
                            }
                            else
                            {
                                checkedPairs[ { a, b } ].layers.set( layer );
                                return true;
                            }
                        },
                        // Visitor:
                        [&]( BOARD_ITEM* other ) -> bool
                        {
                            if( m_drcEngine->IsCancelled() )
                                return false;

                            if( other->Type() == PCB_PAD_T && static_cast<PAD*>( other )->IsFreePad() )
                            {
                                if( other->GetEffectiveShape( layer )->Collide( trackShape.get() ) )
                                {
                                    std::lock_guard<std::mutex> lock( freePadsUsageMapMutex );
                                    auto it = freePadsUsageMap.find( other );

                                    if( it == freePadsUsageMap.end() )
                                    {
                                        freePadsUsageMap[ other ] = track->GetNetCode();
                                        return true;    // Continue colliding tests
                                    }
                                    else if( it->second == track->GetNetCode() )
                                    {
                                        return true;    // Continue colliding tests
                                    }
                                }
                            }

                            BOARD_ITEM* a = track;
                            BOARD_ITEM* b = other;

                            // store canonical order so we don't collide in both directions
                            // (a:b and b:a)
                            if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                                std::swap( a, b );

                            // If we get an error, mark the pair as having a clearance error already
                            if( !testSingleLayerItemAgainstItem( track, trackShape.get(), layer, other ) )
                            {
                                std::lock_guard<std::mutex> lock( checkedPairsMutex );
                                auto it = checkedPairs.find( { a, b } );

                                if( it != checkedPairs.end() )
                                    it->second.has_error = true;

                                if( !m_drcEngine->GetReportAllTrackErrors() )
                                    return false;   // We're done with this track
                            }

                            return !m_drcEngine->IsCancelled();
                        },
                        m_board->m_DRCMaxClearance );

                for( ZONE* zone : m_board->m_DRCCopperZones )
                {
                    testItemAgainstZone( track, zone, layer );

                    if( m_drcEngine->IsCancelled() )
                        break;
                }
            }

            done.fetch_add( 1 );
        }
    };

    thread_pool& tp = GetKiCadThreadPool();

    tp.push_loop( m_board->Tracks().size(), testTrack );

    while( done < count )
    {
        reportProgress( done, count );

        if( m_drcEngine->IsCancelled() )
        {
            tp.wait_for_tasks();
            break;
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testPadAgainstItem( PAD* pad, SHAPE* padShape,
                                                             PCB_LAYER_ID aLayer,
                                                             BOARD_ITEM* other )
{
    bool testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
    bool testShorting = !m_drcEngine->IsErrorLimitExceeded( DRCE_SHORTING_ITEMS );
    bool testHoles = !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE );

    // Disable some tests for net-tie objects in a footprint
    if( other->GetParent() == pad->GetParent() )
    {
        FOOTPRINT*              fp = pad->GetParentFootprint();
        std::map<wxString, int> padToNetTieGroupMap = fp->MapPadNumbersToNetTieGroups();
        int                     padGroupIdx = padToNetTieGroupMap[ pad->GetNumber() ];

        if( other->Type() == PCB_PAD_T )
        {
            PAD* otherPad = static_cast<PAD*>( other );

            if( padGroupIdx >= 0 && padGroupIdx == padToNetTieGroupMap[ otherPad->GetNumber() ] )
                testClearance = testShorting = false;

            if( pad->SameLogicalPadAs( otherPad ) )
                testHoles = false;
        }

        if( other->Type() == PCB_SHAPE_T && padGroupIdx >= 0 )
            testClearance = testShorting = false;
    }

    PAD*     otherPad = nullptr;
    PCB_VIA* otherVia = nullptr;

    if( other->Type() == PCB_PAD_T )
        otherPad = static_cast<PAD*>( other );

    if( other->Type() == PCB_VIA_T )
        otherVia = static_cast<PCB_VIA*>( other );

    if( !IsCopperLayer( aLayer ) )
        testClearance = testShorting = false;

    // A NPTH has no cylinder, but it may still have pads on some layers
    if( pad->GetAttribute() == PAD_ATTRIB::NPTH && !pad->FlashLayer( aLayer ) )
        testClearance = testShorting = false;

    if( otherPad && otherPad->GetAttribute() == PAD_ATTRIB::NPTH && !otherPad->FlashLayer( aLayer ) )
        testClearance = testShorting = false;

    // Track clearances are tested in testTrackClearances()
    if( dynamic_cast<PCB_TRACK*>( other) )
        testClearance = testShorting = false;

    int padNet = pad->GetNetCode();
    int otherNet = 0;

    if( BOARD_CONNECTED_ITEM* connectedItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( other ) )
        otherNet = connectedItem->GetNetCode();

    // Other objects of the same (defined) net get a waiver on clearance and hole tests
    if( otherNet && otherNet == padNet )
    {
        testClearance = testShorting = false;
        testHoles = false;
    }

    if( !( pad->GetDrillSize().x > 0 )
            && !( otherPad && otherPad->GetDrillSize().x > 0 )
            && !( otherVia && otherVia->GetDrill() > 0 ) )
    {
        testHoles = false;
    }

    if( !testClearance && !testShorting && !testHoles )
        return;

    std::shared_ptr<SHAPE> otherShape = other->GetEffectiveShape( aLayer );
    DRC_CONSTRAINT         constraint;
    int                    clearance = 0;
    int                    actual = 0;
    VECTOR2I               pos;

    if( otherPad && pad->SameLogicalPadAs( otherPad ) )
    {
        // If pads are equivalent (ie: from the same footprint with the same pad number)...
        // ... and have "real" nets...
        // then they must be the same net
        if( testShorting )
        {
            if( pad->GetNetCode() == 0 || pad->GetNetCode() == otherPad->GetNetCode() )
                return;

            if( pad->GetShortNetname().StartsWith( wxS( "unconnected-(" ) )
                    && otherPad->GetShortNetname().StartsWith( wxS( "unconnected-(" ) ) )
            {
                return;
            }

            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );
            wxString msg;

            msg.Printf( _( "(nets %s and %s)" ),
                        pad->GetNetname(),
                        otherPad->GetNetname() );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
            drce->SetItems( pad, otherPad );

            reportViolation( drce, otherPad->GetPosition(), aLayer );
        }

        return;
    }

    if( testClearance || testShorting )
    {
        constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, pad, other, aLayer );
        clearance = constraint.GetValue().Min();

        if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
        {
            if( padShape->Collide( otherShape.get(), std::max( 0, clearance - m_drcEpsilon ),
                                   &actual, &pos ) )
            {
                if( m_drcEngine->IsNetTieExclusion( pad->GetNetCode(), aLayer, pos, other ) )
                {
                    // Pads connected to pads of a net-tie footprint are allowed to collide
                    // with the net-tie footprint's graphics.
                }
                else if( actual == 0 && otherNet && testShorting )
                {
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );
                    wxString msg;

                    msg.Printf( _( "(nets %s and %s)" ),
                                pad->GetNetname(),
                                static_cast<BOARD_CONNECTED_ITEM*>( other )->GetNetname() );

                    drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                    drce->SetItems( pad, other );

                    reportViolation( drce, pos, aLayer );
                    testHoles = false;  // No need for multiple violations
                }
                else if( testClearance )
                {
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );
                    wxString msg = formatMsg( _( "(%s clearance %s; actual %s)" ),
                                              constraint.GetName(),
                                              clearance,
                                              actual );

                    drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                    drce->SetItems( pad, other );
                    drce->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drce, pos, aLayer );
                    testHoles = false;  // No need for multiple violations
                }
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
            wxString msg = formatMsg( _( "(%s clearance %s; actual %s)" ),
                                      constraint.GetName(),
                                      clearance,
                                      actual );

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
            wxString msg = formatMsg( _( "(%s clearance %s; actual %s)" ),
                                      constraint.GetName(),
                                      clearance,
                                      actual );

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
            wxString msg = formatMsg( _( "(%s clearance %s; actual %s)" ),
                                      constraint.GetName(),
                                      clearance,
                                      actual );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
            drce->SetItems( pad, otherVia );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, pos, aLayer );
        }
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testPadClearances( )
{
    thread_pool&        tp = GetKiCadThreadPool();
    size_t              count = 0;
    std::atomic<size_t> done( 1 );

    for( FOOTPRINT* footprint : m_board->Footprints() )
        count += footprint->Pads().size();

    reportAux( wxT( "Testing %d pads..." ), count );

    std::unordered_map<PTR_PTR_CACHE_KEY, int> checkedPairs;

    LSET boardCopperLayers = LSET::AllCuMask( m_board->GetCopperLayerCount() );

    std::future<void> retn = tp.submit(
            [&]()
            {
                for( FOOTPRINT* footprint : m_board->Footprints() )
                {
                    for( PAD* pad : footprint->Pads() )
                    {
                        for( PCB_LAYER_ID layer : LSET( pad->GetLayerSet() & boardCopperLayers ).Seq() )
                        {
                            if( m_drcEngine->IsCancelled() )
                                return;

                            std::shared_ptr<SHAPE> padShape = pad->GetEffectiveShape( layer );

                            m_board->m_CopperItemRTreeCache->QueryColliding( pad, layer, layer,
                                    // Filter:
                                    [&]( BOARD_ITEM* other ) -> bool
                                    {
                                        BOARD_ITEM* a = pad;
                                        BOARD_ITEM* b = other;

                                        // store canonical order so we don't collide in both
                                        // directions (a:b and b:a)
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
                                    // Visitor
                                    [&]( BOARD_ITEM* other ) -> bool
                                    {
                                        testPadAgainstItem( pad, padShape.get(), layer, other );

                                        return !m_drcEngine->IsCancelled();
                                    },
                                    m_board->m_DRCMaxClearance );

                            for( ZONE* zone : m_board->m_DRCCopperZones )
                            {
                                testItemAgainstZone( pad, zone, layer );

                                if( m_drcEngine->IsCancelled() )
                                    return;
                            }
                        }

                        done.fetch_add( 1 );
                    }
                }
            } );

    std::future_status status = retn.wait_for( std::chrono::milliseconds( 250 ) );

    while( status != std::future_status::ready )
    {
        reportProgress( done, count );
        status = retn.wait_for( std::chrono::milliseconds( 250 ) );
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testGraphicClearances( )
{
    thread_pool&        tp = GetKiCadThreadPool();
    size_t              count = m_board->Drawings().size();
    std::atomic<size_t> done( 1 );

    for( FOOTPRINT* footprint : m_board->Footprints() )
        count += footprint->GraphicalItems().size();

    reportAux( wxT( "Testing %d graphics..." ), count );

    auto isKnockoutText =
            []( BOARD_ITEM* item )
            {
                return item->Type() == PCB_TEXT_T && static_cast<PCB_TEXT*>( item )->IsKnockout();
            };

    auto testGraphicAgainstZone =
            [&]( BOARD_ITEM* item )
            {
                if( item->Type() == PCB_REFERENCE_IMAGE_T )
                    return;

                if( !IsCopperLayer( item->GetLayer() ) )
                    return;

                // Knockout text is most often knocked-out of a zone, so it's presumed to
                // collide with one.  However, if it collides with more than one, and they
                // have different nets, then we have a short.
                NETINFO_ITEM* inheritedNet = nullptr;

                for( ZONE* zone : m_board->m_DRCCopperZones )
                {
                    if( isKnockoutText( item ) )
                        testKnockoutTextAgainstZone( item, &inheritedNet, zone );
                    else
                        testItemAgainstZone( item, zone, item->GetLayer() );

                    if( m_drcEngine->IsCancelled() )
                        return;
                }
            };

    std::unordered_map<PTR_PTR_CACHE_KEY, layers_checked> checkedPairs;

    auto testCopperGraphic =
            [&]( PCB_SHAPE* aShape )
            {
                PCB_LAYER_ID layer = aShape->GetLayer();

                m_board->m_CopperItemRTreeCache->QueryColliding( aShape, layer, layer,
                            // Filter:
                            [&]( BOARD_ITEM* other ) -> bool
                            {
                                auto otherCItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( other );

                                if( otherCItem && otherCItem->GetNetCode() == aShape->GetNetCode() )
                                    return false;

                                // Pads and tracks handled separately
                                if( other->Type() == PCB_PAD_T || other->Type() == PCB_ARC_T ||
                                    other->Type() == PCB_TRACE_T || other->Type() == PCB_VIA_T )
                                {
                                    return false;
                                }

                                BOARD_ITEM* a = aShape;
                                BOARD_ITEM* b = other;

                                // store canonical order so we don't collide in both directions
                                // (a:b and b:a)
                                if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                                    std::swap( a, b );

                                auto it = checkedPairs.find( { a, b } );

                                if( it != checkedPairs.end() && it->second.layers.test( layer ) )
                                {
                                    return false;
                                }
                                else
                                {
                                    checkedPairs[ { a, b } ].layers.set( layer );
                                    return true;
                                }
                            },
                            // Visitor:
                            [&]( BOARD_ITEM* other ) -> bool
                            {
                                BOARD_ITEM* a = aShape;
                                BOARD_ITEM* b = other;

                                // store canonical order so we don't collide in both directions
                                // (a:b and b:a)
                                if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                                    std::swap( a, b );

                                auto it = checkedPairs.find( { a, b } );

                                if( !testSingleLayerItemAgainstItem( aShape,
                                                                     aShape->GetEffectiveShape().get(),
                                                                     layer, other ) )
                                {
                                    if( it != checkedPairs.end() )
                                        it->second.has_error = true;
                                }

                                return !m_drcEngine->IsCancelled();
                            },
                            m_board->m_DRCMaxClearance );
            };

    std::future<void> retn = tp.submit(
            [&]()
            {
                for( BOARD_ITEM* item : m_board->Drawings() )
                {
                    testGraphicAgainstZone( item );

                    if( item->Type() == PCB_SHAPE_T && item->IsOnCopperLayer() )
                        testCopperGraphic( static_cast<PCB_SHAPE*>( item ) );

                    done.fetch_add( 1 );

                    if( m_drcEngine->IsCancelled() )
                        break;
                }

                for( FOOTPRINT* footprint : m_board->Footprints() )
                {
                    for( BOARD_ITEM* item : footprint->GraphicalItems() )
                    {
                        testGraphicAgainstZone( item );

                        done.fetch_add( 1 );

                        if( m_drcEngine->IsCancelled() )
                            break;
                    }
                }
        } );

    std::future_status status = retn.wait_for( std::chrono::milliseconds( 250 ) );

    while( status != std::future_status::ready )
    {
        reportProgress( done, count );
        status = retn.wait_for( std::chrono::milliseconds( 250 ) );
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testZonesToZones()
{
    bool           testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
    bool           testIntersects = !m_drcEngine->IsErrorLimitExceeded( DRCE_ZONES_INTERSECT );
    DRC_CONSTRAINT constraint;

    std::vector<std::map<PCB_LAYER_ID, std::vector<SEG>>> poly_segments;
    poly_segments.resize( m_board->m_DRCCopperZones.size() );

    // Contains the index for zoneA, zoneB, the conflict point, the actual clearance, the
    // required clearance, and the layer
    using report_data = std::tuple<int, int, VECTOR2I, int, int, PCB_LAYER_ID>;

    std::vector<std::future<report_data>> futures;
    thread_pool&                          tp = GetKiCadThreadPool();
    std::atomic<size_t>                   done( 1 );

    auto checkZones =
            [this, testClearance, testIntersects, &poly_segments, &done]
            ( int zoneA, int zoneB, int clearance, PCB_LAYER_ID layer ) -> report_data
            {
                // Iterate through all the segments of refSmoothedPoly
                std::map<VECTOR2I, int> conflictPoints;

                std::vector<SEG>& refSegments = poly_segments[zoneA][layer];
                std::vector<SEG>& testSegments = poly_segments[zoneB][layer];
                bool reported = false;
                auto invalid_result = std::make_tuple( -1, -1, VECTOR2I(), 0, 0, F_Cu );

                for( SEG& refSegment : refSegments )
                {
                    int ax1 = refSegment.A.x;
                    int ay1 = refSegment.A.y;
                    int ax2 = refSegment.B.x;
                    int ay2 = refSegment.B.y;

                    // Iterate through all the segments in smoothed_polys[ia2]
                    for( SEG& testSegment : testSegments )
                    {
                        // Build test segment
                        VECTOR2I pt;

                        int bx1 = testSegment.A.x;
                        int by1 = testSegment.A.y;
                        int bx2 = testSegment.B.x;
                        int by2 = testSegment.B.y;

                        // We have ensured that the 'A' segment starts before the 'B' segment,
                        // so if the 'A' segment ends before the 'B' segment starts, we can skip
                        // to the next 'A'
                        if( ax2 < bx1 )
                            break;

                        int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2, 0,
                                                             ax1, ay1, ax2, ay2, 0,
                                                             clearance, &pt.x, &pt.y );

                        if( d < clearance )
                        {
                            if( d == 0 && testIntersects )
                                reported = true;
                            else if( testClearance )
                                reported = true;

                            if( reported )
                            {
                                done.fetch_add( 1 );
                                return std::make_tuple( zoneA, zoneB, pt, d, clearance, layer );
                            }
                        }

                        if( m_drcEngine->IsCancelled() )
                            return invalid_result;
                    }
                }

                done.fetch_add( 1 );
                return invalid_result;
            };

    for( int layer_id = F_Cu; layer_id <= B_Cu; ++layer_id )
    {
        PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( layer_id );
        int          zone2zoneClearance;

        // Skip over layers not used on the current board
        if( !m_board->IsLayerEnabled( layer ) )
            continue;

        for( size_t ii = 0; ii < m_board->m_DRCCopperZones.size(); ii++ )
        {
            if( m_board->m_DRCCopperZones[ii]->IsOnLayer( layer ) )
            {
                SHAPE_POLY_SET poly = *m_board->m_DRCCopperZones[ii]->GetFilledPolysList( layer );
                std::vector<SEG>& zone_layer_poly_segs = poly_segments[ii][layer];

                poly.BuildBBoxCaches();
                zone_layer_poly_segs.reserve( poly.FullPointCount() );

                for( auto it = poly.IterateSegmentsWithHoles(); it; it++ )
                {
                    SEG seg = *it;

                    if( seg.A.x > seg.B.x )
                        seg.Reverse();

                    zone_layer_poly_segs.push_back( seg );
                }

                std::sort( zone_layer_poly_segs.begin(), zone_layer_poly_segs.end() );
            }
        }

        std::vector<std::pair<int, int>> zonePairs;

        for( size_t ia = 0; ia < m_board->m_DRCCopperZones.size(); ia++ )
        {
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

                // rule areas may overlap at will
                if( zoneA->GetIsRuleArea() || zoneB->GetIsRuleArea() )
                    continue;

                // Examine a candidate zone: compare zoneB to zoneA
                SHAPE_POLY_SET* polyA = m_board->m_DRCCopperZones[ia]->GetFill( layer );
                SHAPE_POLY_SET* polyB = m_board->m_DRCCopperZones[ia2]->GetFill( layer );

                if( !polyA->BBoxFromCaches().Intersects( polyB->BBoxFromCaches() ) )
                    continue;

                // Get clearance used in zone to zone test.
                constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, zoneA, zoneB, layer );
                zone2zoneClearance = constraint.GetValue().Min();

                if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE || zone2zoneClearance <= 0 )
                    continue;

                futures.push_back( tp.submit( checkZones, ia, ia2, zone2zoneClearance, layer ) );
            }
        }
    }

    size_t count = futures.size();

    for( auto& task : futures )
    {
        if( !task.valid() )
            continue;

        std::future_status result;

        while( true )
        {
            result = task.wait_for( std::chrono::milliseconds( 250 ) );

            reportProgress( done, count );

            if( m_drcEngine->IsCancelled() )
                break;

            if( result == std::future_status::ready )
            {
                report_data  data = task.get();
                int          zoneA_idx = std::get<0>( data );
                int          zoneB_idx = std::get<1>( data );
                VECTOR2I     pt = std::get<2>( data );
                int          actual = std::get<3>( data );
                int          required = std::get<4>( data );
                PCB_LAYER_ID layer = std::get<5>( data );

                if( zoneA_idx >= 0 )
                {
                    ZONE* zoneA = m_board->m_DRCCopperZones[zoneA_idx];
                    ZONE* zoneB = m_board->m_DRCCopperZones[zoneB_idx];

                    constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, zoneA, zoneB, layer );
                    std::shared_ptr<DRC_ITEM> drce;

                    if( actual <= 0 && testIntersects )
                    {
                        drce = DRC_ITEM::Create( DRCE_ZONES_INTERSECT );
                    }
                    else if( testClearance )
                    {
                        drce = DRC_ITEM::Create( DRCE_CLEARANCE );
                        wxString msg = formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                  constraint.GetName(),
                                                  required,
                                                  std::max( actual, 0 ) );

                        drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                    }

                    if( drce )
                    {
                        drce->SetItems( zoneA, zoneB );
                        drce->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drce, pt, layer );
                    }
                }

                break;
            }
        }
    }
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_COPPER_CLEARANCE> dummy;
}

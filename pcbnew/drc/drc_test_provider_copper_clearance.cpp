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
#include <layer_range.h>
#include <pcb_shape.h>
#include <pad.h>
#include <pcb_track.h>
#include <thread_pool.h>
#include <zone.h>

#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_segment.h>

#include <drc/drc_engine.h>
#include <drc/drc_rtree.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_creepage_utils.h>
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

class DRC_TEST_PROVIDER_COPPER_CLEARANCE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_COPPER_CLEARANCE () :
            DRC_TEST_PROVIDER(),
            m_drcEpsilon( 0 )
    {}

    virtual ~DRC_TEST_PROVIDER_COPPER_CLEARANCE() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "clearance" ); };

private:
    /**
     * Checks for track/via/hole <-> clearance
     * @param item Track to text
     * @param itemShape Primitive track shape
     * @param layer Which layer to test (in case of vias this can be multiple
     * @param other item against which to test the track item
     * @return false if there is a clearance violation reported, true if there is none
     */
    bool testSingleLayerItemAgainstItem( BOARD_ITEM* item, SHAPE* itemShape, PCB_LAYER_ID layer,
                                         BOARD_ITEM* other );

    void testTrackClearances();

    bool testPadAgainstItem( PAD* pad, SHAPE* padShape, PCB_LAYER_ID layer, BOARD_ITEM* other );

    void testPadClearances();

    void testGraphicClearances();

    void testZonesToZones();

    void testItemAgainstZone( BOARD_ITEM* aItem, ZONE* aZone, PCB_LAYER_ID aLayer );

    void testKnockoutTextAgainstZone( BOARD_ITEM* aText, NETINFO_ITEM** aInheritedNet, ZONE* aZone );

private:
    int m_drcEpsilon;
};


bool DRC_TEST_PROVIDER_COPPER_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();

    if( m_board->m_DRCMaxClearance <= 0 )
    {
        REPORT_AUX( wxT( "No Clearance constraints found. Tests not run." ) );
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
    else if( !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE ) )
    {
        if( !reportPhase( _( "Checking copper graphic hole clearances..." ) ) )
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

    return !m_drcEngine->IsCancelled();
}


bool DRC_TEST_PROVIDER_COPPER_CLEARANCE::testSingleLayerItemAgainstItem( BOARD_ITEM* item,
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
    NETINFO_ITEM*  itemNet = nullptr;
    NETINFO_ITEM*  otherNet = nullptr;

    if( BOARD_CONNECTED_ITEM* connectedItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
        itemNet = connectedItem->GetNet();

    if( BOARD_CONNECTED_ITEM* connectedItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( other ) )
        otherNet = connectedItem->GetNet();

    if( itemNet == otherNet )
        testClearance = testShorting = false;

    std::shared_ptr<SHAPE> otherShape_shared_ptr;

    if( other->Type() == PCB_PAD_T )
    {
        PAD* pad = static_cast<PAD*>( other );

        if( !pad->FlashLayer( layer ) )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                testClearance = testShorting = false;

            otherShape_shared_ptr = pad->GetEffectiveHoleShape();
        }
    }
    else if( other->Type() == PCB_VIA_T )
    {
        PCB_VIA* via = static_cast<PCB_VIA*>( other );

        if( !via->FlashLayer( layer ) )
            otherShape_shared_ptr = via->GetEffectiveHoleShape();
    }

    if( !otherShape_shared_ptr )
        otherShape_shared_ptr = other->GetEffectiveShape( layer );

    SHAPE* otherShape = otherShape_shared_ptr.get();

    // Collide (and generate violations) based on a well-defined order so that exclusion checking
    // against previously-generated violations will work.
    if( item->m_Uuid > other->m_Uuid )
    {
        std::swap( item, other );
        std::swap( itemShape, otherShape );
        std::swap( itemNet, otherNet );
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
                reportTwoPointGeometry( drcItem, *intersection, *intersection, *intersection, layer );
                return false;
            }
        }

        if( itemShape->Collide( otherShape, clearance - m_drcEpsilon, &actual, &pos ) )
        {
            if( itemNet && m_drcEngine->IsNetTieExclusion( itemNet->GetNetCode(), layer, pos, other ) )
            {
                // Collision occurred as track was entering a pad marked as a net-tie.  We
                // allow these.
            }
            else if( actual == 0 && otherNet && testShorting )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );
                drcItem->SetErrorDetail( wxString::Format( _( "(nets %s and %s)" ),
                                                           itemNet ? itemNet->GetNetname() : _( "<no net>" ),
                                                           otherNet ? otherNet->GetNetname() : _( "<no net>" ) ) );
                drcItem->SetItems( item, other );
                reportTwoPointGeometry( drcItem, pos, pos, pos, layer );
                has_error = true;

                if( !m_drcEngine->GetReportAllTrackErrors() )
                    return false;
            }
            else if( testClearance )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
                drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                    constraint.GetName(),
                                                    clearance,
                                                    actual ) );
                drcItem->SetItems( item, other );
                drcItem->SetViolatingRule( constraint.GetParentRule() );
                reportTwoShapeGeometry( drcItem, pos, itemShape, otherShape, layer, actual );
                has_error = true;

                if( !m_drcEngine->GetReportAllTrackErrors() )
                    return false;
            }
        }
    }

    if( testHoles && ( item->HasHole() || other->HasHole() ) )
    {
        std::array<BOARD_ITEM*, 2>   a{ item, other };
        std::array<BOARD_ITEM*, 2>   b{ other, item };
        std::array<NETINFO_ITEM*, 2> b_net{ otherNet, itemNet };
        std::array<SHAPE*, 2>        a_shape{ itemShape, otherShape };

        for( size_t ii = 0; ii < 2; ++ii )
        {
            std::shared_ptr<SHAPE_SEGMENT> holeShape;

            if( b[ii]->Type() == PCB_VIA_T )
            {
                if( b[ii]->GetLayerSet().Contains( layer ) )
                    holeShape = b[ii]->GetEffectiveHoleShape();
                else
                    continue;
            }
            else
            {
                if( b[ii]->HasHole() )
                    holeShape = b[ii]->GetEffectiveHoleShape();
                else
                    continue;
            }

            int netcode = b_net[ii] ? b_net[ii]->GetNetCode() : 0;

            if( netcode && m_drcEngine->IsNetTieExclusion( netcode, layer, holeShape->Centre(), a[ii] ) )
                continue;

            constraint = m_drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, b[ii], a[ii], layer );
            clearance = constraint.GetValue().Min();

            // Test for hole to item clearance even if clearance is 0, because the item cannot be
            // inside (or intersect) the hole.
            if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
            {
                if( a_shape[ii]->Collide( holeShape.get(), std::max( 0, clearance - m_drcEpsilon ),
                                          &actual, &pos ) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                    drcItem->SetErrorDetail( formatMsg( clearance ? _( "(%s clearance %s; actual %s)" )
                                                                  : _( "(%s clearance %s; actual < 0)" ),
                                                        constraint.GetName(),
                                                        clearance,
                                                        actual ) );
                    drcItem->SetItems( a[ii], b[ii] );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );
                    reportTwoShapeGeometry( drcItem, pos, a_shape[ii], holeShape.get(), layer, actual );
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
                        && pad->GetEffectiveShape( aLayer )->Collide( itemShape.get() ) )
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
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
            drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                constraint.GetName(),
                                                clearance,
                                                actual ) );
            drcItem->SetItems( aItem, aZone );
            drcItem->SetViolatingRule( constraint.GetParentRule() );
            reportTwoItemGeometry( drcItem, pos, aItem, aZone, aLayer, actual );
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
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                    drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                        constraint.GetName(),
                                                        clearance,
                                                        actual ) );
                    drcItem->SetItems( aItem, aZone );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    std::shared_ptr<SHAPE> zoneShape = aZone->GetEffectiveShape( aLayer );
                    reportTwoShapeGeometry( drcItem, pos, holeShape.get(), zoneShape.get(), aLayer, actual );
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
            std::shared_ptr<DRC_ITEM> drcItem;

            if( testShorts && actual == 0 && *aInheritedNet )
            {
                drcItem = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );
                drcItem->SetErrorDetail( wxString::Format( _( "(nets %s and %s)" ),
                                                           ( *aInheritedNet )->GetNetname(),
                                                           aZone->GetNetname() ) );
            }
            else
            {
                drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
                drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                   constraint.GetName(),
                                                   clearance,
                                                   actual ) );
            }

            drcItem->SetItems( aText, aZone );
            drcItem->SetViolatingRule( constraint.GetParentRule() );
            reportTwoItemGeometry( drcItem, pos, aText, aZone, layer, actual );
        }
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testTrackClearances()
{
    std::map<BOARD_ITEM*, int>                            freePadsUsageMap;
    std::unordered_map<PTR_PTR_CACHE_KEY, LAYERS_CHECKED> checkedPairs;
    std::mutex                                            checkedPairsMutex;
    std::mutex                                            freePadsUsageMapMutex;
    std::atomic<size_t>                                   done( 0 );
    size_t                                                count = m_board->Tracks().size();

    REPORT_AUX( wxString::Format( wxT( "Testing %d tracks & vias..." ), count ) );

    LSET boardCopperLayers = LSET::AllCuMask( m_board->GetCopperLayerCount() );

    auto testTrack =
            [&]( const int trackIdx )
            {
                PCB_TRACK* track = m_board->Tracks()[trackIdx];

                for( PCB_LAYER_ID layer : LSET( track->GetLayerSet() & boardCopperLayers ) )
                {
                    std::shared_ptr<SHAPE> trackShape = track->GetEffectiveShape( layer );

                    m_board->m_CopperItemRTreeCache->QueryColliding( track, layer, layer,
                            // Filter:
                            [&]( BOARD_ITEM* other ) -> bool
                            {
                                BOARD_CONNECTED_ITEM* otherCItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( other );

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

                                if( it != checkedPairs.end()
                                        && ( it->second.layers.test( layer ) || ( it->second.has_error ) ) )
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

                                // If we get an error, mark the pair as having a clearance error already
                                if( !testSingleLayerItemAgainstItem( track, trackShape.get(), layer, other ) )
                                {
                                    if( !m_drcEngine->GetReportAllTrackErrors() )
                                    {
                                        BOARD_ITEM* a = track;
                                        BOARD_ITEM* b = other;

                                        // store canonical order so we don't collide in both directions
                                        // (a:b and b:a)
                                        if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                                            std::swap( a, b );

                                        std::lock_guard<std::mutex> lock( checkedPairsMutex );
                                        auto it = checkedPairs.find( { a, b } );

                                        if( it != checkedPairs.end() )
                                            it->second.has_error = true;

                                        return false;   // We're done with this track
                                    }
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
            };

    thread_pool& tp = GetKiCadThreadPool();

    auto track_futures = tp.submit_loop( 0, m_board->Tracks().size(), testTrack );

    while( done < count )
    {
        reportProgress( done, count );

        if( m_drcEngine->IsCancelled() )
        {
            // Wait for the submitted loop tasks to finish
            track_futures.wait();
            break;
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
    }
}


bool DRC_TEST_PROVIDER_COPPER_CLEARANCE::testPadAgainstItem( PAD* pad, SHAPE* padShape,
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

    BOARD_CONNECTED_ITEM* otherCItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( other );
    PAD*                  otherPad = nullptr;
    PCB_VIA*              otherVia = nullptr;

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

    // Graphic clearances are tested in testGraphicClearances()
    if( dynamic_cast<PCB_SHAPE*>( other ) )
        testClearance = testShorting = false;

    int padNet = pad->GetNetCode();
    int otherNet = otherCItem ? otherCItem->GetNetCode() : 0;

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
        return true;

    std::shared_ptr<SHAPE> otherShape = other->GetEffectiveShape( aLayer );
    DRC_CONSTRAINT         constraint;
    int                    clearance = 0;
    int                    actual = 0;
    VECTOR2I               pos;
    bool                   has_error = false;

    auto sub_e = [this]( int aclearance )
                 {
                     return std::max( 0, aclearance - m_drcEpsilon );
                 };

    if( otherPad && pad->SameLogicalPadAs( otherPad ) )
    {
        // If pads are equivalent (ie: from the same footprint with the same pad number)...
        // ... and have "real" nets...
        // then they must be the same net
        if( testShorting )
        {
            if( pad->GetNetCode() == 0 || pad->GetNetCode() == otherPad->GetNetCode() )
                return true;

            if( pad->GetShortNetname().StartsWith( wxS( "unconnected-(" ) )
                    && otherPad->GetShortNetname().StartsWith( wxS( "unconnected-(" ) ) )
            {
                return true;
            }

            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );
            drcItem->SetErrorDetail( wxString::Format( _( "(nets %s and %s)" ),
                                                       pad->GetNetname(),
                                                       otherPad->GetNetname() ) );
            drcItem->SetItems( pad, otherPad );
            reportViolation( drcItem, otherPad->GetPosition(), aLayer );
            has_error = true;
        }

        return !has_error;
    }

    if( testClearance || testShorting )
    {
        constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, pad, other, aLayer );
        clearance = constraint.GetValue().Min();

        if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
        {
            if( padShape->Collide( otherShape.get(), sub_e( clearance ), &actual, &pos ) )
            {
                if( m_drcEngine->IsNetTieExclusion( pad->GetNetCode(), aLayer, pos, other ) )
                {
                    // Pads connected to pads of a net-tie footprint are allowed to collide
                    // with the net-tie footprint's graphics.
                }
                else if( actual == 0 && padNet && otherNet && testShorting )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );
                    drcItem->SetErrorDetail( wxString::Format( _( "(nets %s and %s)" ),
                                                               pad->GetNetname(),
                                                               otherCItem->GetNetname() ) );
                    drcItem->SetItems( pad, other );
                    reportTwoPointGeometry( drcItem, pos, pos, pos, aLayer );
                    has_error = true;
                    testHoles = false;  // No need for multiple violations
                }
                else if( testClearance )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
                    drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                        constraint.GetName(),
                                                        clearance,
                                                        actual ) );
                    drcItem->SetItems( pad, other );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );
                    reportTwoItemGeometry( drcItem, pos, pad, other, aLayer, actual );
                    has_error = true;
                    testHoles = false;  // No need for multiple violations
                }
            }
        }
    }

    auto doTestHole =
            [&]( BOARD_ITEM* item, SHAPE* shape, BOARD_ITEM* otherItem, SHAPE* aOtherShape, int aClearance )
            {
                if( shape->Collide( aOtherShape, sub_e( aClearance ), &actual, &pos ) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                    drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                        constraint.GetName(),
                                                        aClearance,
                                                        actual ) );
                    drcItem->SetItems( item, otherItem );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );
                    reportTwoShapeGeometry( drcItem, pos, shape, aOtherShape, aLayer, actual );
                    has_error = true;
                    testHoles = false;  // No need for multiple violations
                }
            };

    if( testHoles )
    {
        constraint = m_drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, pad, other, aLayer );

        if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
            testHoles = false;
    }

    if( testHoles && otherPad && otherPad->HasHole() )
    {
        clearance = constraint.GetValue().Min();

        if( !pad->FlashLayer( aLayer ) )
            clearance = 0;

        if( clearance > 0 )
            doTestHole( pad, padShape, otherPad, otherPad->GetEffectiveHoleShape().get(), clearance );
    }

    if( testHoles && otherVia && otherVia->HasHole() )
    {
        clearance = constraint.GetValue().Min();

        if( !otherVia->IsOnLayer( aLayer ) )
            clearance = 0;

        if( clearance > 0 )
            doTestHole( pad, padShape, otherVia, otherVia->GetEffectiveHoleShape().get(), clearance );
    }

    return !has_error;
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testPadClearances( )
{
    thread_pool&        tp = GetKiCadThreadPool();
    std::atomic<size_t> done( 1 );

    std::unordered_map<PTR_PTR_CACHE_KEY, LAYERS_CHECKED> checkedPairs;
    std::mutex                                            checkedPairsMutex;

    LSET boardCopperLayers = LSET::AllCuMask( m_board->GetCopperLayerCount() );

    const auto fp_check =
            [&]( size_t ii )
            {
                FOOTPRINT* footprint = m_board->Footprints()[ ii ];

                for( PAD* pad : footprint->Pads() )
                {
                    for( PCB_LAYER_ID layer : LSET( pad->GetLayerSet() & boardCopperLayers ) )
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

                                    std::lock_guard<std::mutex> lock( checkedPairsMutex );
                                    auto it = checkedPairs.find( { a, b } );

                                    if( it != checkedPairs.end()
                                            && ( it->second.layers.test( layer ) || it->second.has_error ) )
                                    {
                                        return false;
                                    }
                                    else
                                    {
                                        checkedPairs[ { a, b } ].layers.set( layer );
                                        return true;
                                    }
                                },
                                // Visitor
                                [&]( BOARD_ITEM* other ) -> bool
                                {
                                    if( !testPadAgainstItem( pad, padShape.get(), layer, other ) )
                                    {
                                        BOARD_ITEM* a = pad;
                                        BOARD_ITEM* b = other;

                                        std::lock_guard<std::mutex> lock( checkedPairsMutex );
                                        auto it = checkedPairs.find( { a, b } );

                                        if( it != checkedPairs.end() )
                                            it->second.has_error = true;
                                    }

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
                }

                done.fetch_add( 1 );
            };

    size_t numFootprints = m_board->Footprints().size();
    auto returns = tp.submit_loop( 0, numFootprints, fp_check );

    // Wait for all threads to finish
    for( size_t ii = 0; ii < returns.size(); ++ii )
    {
        while( returns[ii].wait_for( std::chrono::milliseconds( 250 ) ) != std::future_status::ready )
            reportProgress( done, numFootprints );
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testGraphicClearances()
{
    thread_pool&        tp = GetKiCadThreadPool();
    size_t              count = m_board->Drawings().size();
    std::atomic<size_t> done( 1 );

    for( FOOTPRINT* footprint : m_board->Footprints() )
        count += footprint->GraphicalItems().size();

    REPORT_AUX( wxString::Format( wxT( "Testing %d graphics..." ), count ) );

    auto isKnockoutText =
            []( BOARD_ITEM* item )
            {
                return item->Type() == PCB_TEXT_T && static_cast<PCB_TEXT*>( item )->IsKnockout();
            };

    auto testGraphicAgainstZone =
            [this, isKnockoutText]( BOARD_ITEM* item )
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

    std::unordered_map<PTR_PTR_CACHE_KEY, LAYERS_CHECKED> checkedPairs;
    std::mutex                                            checkedPairsMutex;

    auto testCopperGraphic =
            [this, &checkedPairs, &checkedPairsMutex]( BOARD_ITEM* graphic )
            {
                PCB_LAYER_ID layer = graphic->GetLayer();

                m_board->m_CopperItemRTreeCache->QueryColliding( graphic, layer, layer,
                        // Filter:
                        [&]( BOARD_ITEM* other ) -> bool
                        {
                             // Graphics are often compound shapes so ignore collisions between shapes
                             // in a single footprint.
                             if( graphic->Type() == PCB_SHAPE_T && other->Type() == PCB_SHAPE_T
                                      && graphic->GetParentFootprint()
                                      && graphic->GetParentFootprint() == other->GetParentFootprint() )
                             {
                                 return false;
                             }

                            // Track clearances are tested in testTrackClearances()
                            if( dynamic_cast<PCB_TRACK*>( other) )
                                return false;

                            BOARD_ITEM* a = graphic;
                            BOARD_ITEM* b = other;

                            // store canonical order so we don't collide in both directions
                            // (a:b and b:a)
                            if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                                std::swap( a, b );

                            std::lock_guard<std::mutex> lock( checkedPairsMutex );
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
                            testSingleLayerItemAgainstItem( graphic, graphic->GetEffectiveShape().get(),
                                                            layer, other );

                            return !m_drcEngine->IsCancelled();
                        },
                        m_board->m_DRCMaxClearance );
            };

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        (void)tp.submit_task(
                [this, item, &done, testGraphicAgainstZone, testCopperGraphic]()
                {
                    if( !m_drcEngine->IsCancelled() )
                    {
                        testGraphicAgainstZone( item );

                        if( ( item->Type() == PCB_SHAPE_T || item->Type() == PCB_BARCODE_T )
                                && item->IsOnCopperLayer() )
                        {
                            testCopperGraphic( static_cast<PCB_SHAPE*>( item ) );
                        }

                        done.fetch_add( 1 );
                    }
                } );
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        (void)tp.submit_task(
                [this, footprint, &done, testGraphicAgainstZone, testCopperGraphic]()
                {
                    for( BOARD_ITEM* item : footprint->GraphicalItems() )
                    {
                        if( !m_drcEngine->IsCancelled() )
                        {
                            testGraphicAgainstZone( item );

                            if( ( item->Type() == PCB_SHAPE_T || item->Type() == PCB_BARCODE_T )
                                    && item->IsOnCopperLayer() )
                            {
                                testCopperGraphic( static_cast<PCB_SHAPE*>( item ) );
                            }

                            done.fetch_add( 1 );
                        }
                    }
                } );
    }

    while( true )
    {
        reportProgress( done, count );

        if( m_drcEngine->IsCancelled() )
            break;

        if( tp.wait_for( std::chrono::milliseconds( 250 ) ) )
            break;
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testZonesToZones()
{
    bool testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
    bool testIntersects = !m_drcEngine->IsErrorLimitExceeded( DRCE_ZONES_INTERSECT );

    std::vector<std::map<PCB_LAYER_ID, std::vector<SEG>>> poly_segments;
    poly_segments.resize( m_board->m_DRCCopperZones.size() );

    thread_pool&        tp = GetKiCadThreadPool();
    std::atomic<size_t> done( 0 );
    size_t              count = 0;

    auto reportZoneZoneViolation =
            [this]( ZONE* zoneA, ZONE* zoneB, VECTOR2I& pt, int actual, const DRC_CONSTRAINT& constraint,
                    PCB_LAYER_ID layer ) -> void
            {
                std::shared_ptr<DRC_ITEM> drcItem;

                if( constraint.IsNull() )
                {
                    drcItem = DRC_ITEM::Create( DRCE_ZONES_INTERSECT );
                    drcItem->SetErrorDetail( _( "(intersecting zones must have distinct priorities)" ) );
                    drcItem->SetItems( zoneA, zoneB );
                    reportViolation( drcItem, pt, layer );
                }
                else
                {
                    drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
                    drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                        constraint.GetName(),
                                                        constraint.GetValue().Min(),
                                                        std::max( actual, 0 ) ) );
                    drcItem->SetItems( zoneA, zoneB );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );
                    reportTwoItemGeometry( drcItem, pt, zoneA, zoneB, layer, actual );
                }
            };

    auto checkZones =
            [this, testClearance, testIntersects, reportZoneZoneViolation, &poly_segments, &done]
            ( int zoneA_idx, int zoneB_idx, bool sameNet, PCB_LAYER_ID layer ) -> void
            {
                ZONE*    zoneA = m_board->m_DRCCopperZones[zoneA_idx];
                ZONE*    zoneB = m_board->m_DRCCopperZones[zoneB_idx];
                int      actual = 0;
                VECTOR2I pt;

                if( sameNet && testIntersects )
                {
                    if( zoneA->Outline()->Collide( zoneB->Outline(), 0, &actual, &pt ) )
                    {
                        done.fetch_add( 1 );
                        reportZoneZoneViolation( zoneA, zoneB, pt, actual, DRC_CONSTRAINT(), layer );
                        return;
                    }
                }
                else if( !sameNet && testClearance )
                {
                    DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, zoneA, zoneB, layer );
                    int            clearance = constraint.GetValue().Min();

                    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
                    {
                        std::map<VECTOR2I, int> conflictPoints;

                        std::vector<SEG>& refSegments = poly_segments[zoneA_idx][layer];
                        std::vector<SEG>& testSegments = poly_segments[zoneB_idx][layer];

                        // Iterate through all the segments in zoneA
                        for( SEG& refSegment : refSegments )
                        {
                            // Iterate through all the segments in zoneB
                            for( SEG& testSegment : testSegments )
                            {
                                // We have ensured that the 'A' segment starts before the 'B' segment, so if the
                                // 'A' segment ends before the 'B' segment starts, we can skip to the next 'A'
                                if( refSegment.B.x < testSegment.A.x )
                                    break;

                                int64_t  dist_sq = 0;
                                VECTOR2I other_pt;
                                refSegment.NearestPoints( testSegment, pt, other_pt, dist_sq );
                                actual = std::floor( std::sqrt( dist_sq ) + 0.5 );

                                if( actual < clearance )
                                {
                                    done.fetch_add( 1 );
                                    reportZoneZoneViolation( zoneA, zoneB, pt, actual, constraint, layer );
                                    return;
                                }
                            }
                        }
                    }
                }

                done.fetch_add( 1 );
            };

    // Pre-sort zones into layers
    std::map<PCB_LAYER_ID, std::vector<size_t>> zone_idx_by_layer;

    for ( size_t ii = 0; ii < m_board->m_DRCCopperZones.size(); ii++ )
    {
        ZONE* zone = m_board->m_DRCCopperZones[ii];

        for( PCB_LAYER_ID layer : zone->GetLayerSet() )
        {
            if( !IsCopperLayer( layer ) )
                continue;

            zone_idx_by_layer[layer].push_back( ii );
        }
    }

    for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, m_board->GetCopperLayerCount() ) )
    {
        // Skip over layers not used on the current board
        if( !m_board->IsLayerEnabled( layer ) )
            continue;

        for( size_t ii : zone_idx_by_layer[layer] )
        {
            if( SHAPE_POLY_SET* poly = m_board->m_DRCCopperZones[ii]->GetFill( layer ) )
            {
                std::vector<SEG>& zone_layer_poly_segs = poly_segments[ii][layer];
                zone_layer_poly_segs.reserve( poly->FullPointCount() );

                for( auto it = poly->IterateSegmentsWithHoles(); it; it++ )
                {
                    SEG seg = *it;

                    if( seg.A.x > seg.B.x )
                        seg.Reverse();

                    zone_layer_poly_segs.push_back( seg );
                }

                std::sort( zone_layer_poly_segs.begin(), zone_layer_poly_segs.end() );
            }
        }

        for( auto it_a = zone_idx_by_layer[layer].begin(); it_a != zone_idx_by_layer[layer].end(); ++it_a )
        {
            size_t ia = *it_a;
            ZONE*  zoneA = m_board->m_DRCCopperZones[ia];

            for( auto it_a2 = std::next( it_a ); it_a2 != zone_idx_by_layer[layer].end(); ++it_a2 )
            {
                size_t ia2 = *it_a2;
                ZONE*  zoneB = m_board->m_DRCCopperZones[ia2];

                bool sameNet = zoneA->GetNetCode() == zoneB->GetNetCode() && zoneA->GetNetCode() >= 0;

                if( sameNet && zoneA->GetAssignedPriority() != zoneB->GetAssignedPriority() )
                    continue;

                // rule areas may overlap at will
                if( zoneA->GetIsRuleArea() || zoneB->GetIsRuleArea() )
                    continue;

                // Examine a candidate zone: compare zoneB to zoneA
                SHAPE_POLY_SET* polyA = nullptr;
                SHAPE_POLY_SET* polyB = nullptr;

                if( sameNet )
                {
                    polyA = zoneA->Outline();
                    polyB = zoneB->Outline();
                }
                else
                {
                    polyA = zoneA->GetFill( layer );
                    polyB = zoneB->GetFill( layer );
                }

                if( !polyA->BBoxFromCaches().Intersects( polyB->BBoxFromCaches() ) )
                    continue;

                count++;
                (void)tp.submit_task(
                        [checkZones, ia, ia2, sameNet, layer]()
                        {
                            checkZones( ia, ia2, sameNet, layer );
                        } );
            }
        }
    }

    while( true )
    {
        reportProgress( done, count );

        if( m_drcEngine->IsCancelled() )
            break;

        if( tp.wait_for( std::chrono::milliseconds( 250 ) ) )
            break;
    }
}

namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_COPPER_CLEARANCE> dummy;
}

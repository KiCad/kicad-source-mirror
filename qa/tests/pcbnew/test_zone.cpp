/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <footprint.h>
#include <geometry/shape_utils.h>
#include <netinfo.h>
#include <pad.h>
#include <padstack.h>
#include <pcb_track.h>
#include <zone.h>
#include <zone_utils.h>


struct ZONE_TEST_FIXTURE
{
    BOARD m_board;
};


static std::unique_ptr<ZONE> CreateSquareZone( BOARD_ITEM_CONTAINER& aParent, BOX2I aBox, PCB_LAYER_ID aLayer )
{
    auto zone = std::make_unique<ZONE>( &aParent );
    zone->SetLayer( aLayer );

    auto outline = std::make_unique<SHAPE_POLY_SET>();
    outline->AddOutline( KIGEOM::BoxToLineChain( aBox ) );

    zone->SetOutline( outline.release() );

    return zone;
}


/**
 * Create a similar zone (same outline) on a different layer
 */
static std::unique_ptr<ZONE> CreateSimilarZone( BOARD_ITEM_CONTAINER& aParent, const ZONE& aOther, PCB_LAYER_ID aLayer )
{
    auto zone = std::make_unique<ZONE>( &aParent );
    zone->SetLayer( aLayer );

    std::unique_ptr<SHAPE_POLY_SET> outline = std::make_unique<SHAPE_POLY_SET>( *aOther.Outline() );
    zone->SetOutline( outline.release() );

    return zone;
}


BOOST_FIXTURE_TEST_SUITE( Zone, ZONE_TEST_FIXTURE )

BOOST_AUTO_TEST_CASE( SingleLayer )
{
    ZONE zone( &m_board );

    zone.SetLayer( F_Cu );

    BOOST_TEST( zone.GetLayer() == F_Cu );
    BOOST_TEST( zone.GetLayer() == zone.GetFirstLayer() );

    BOOST_TEST( zone.IsOnCopperLayer() == true );
}

BOOST_AUTO_TEST_CASE( MultipleLayers )
{
    ZONE zone( &m_board );

    zone.SetLayerSet( { F_Cu, B_Cu } );

    // There is no "the" layer in a multi-layer zone
    BOOST_TEST( zone.GetLayer() == UNDEFINED_LAYER );
    // ... but there is a first layer
    BOOST_TEST( zone.GetFirstLayer() == F_Cu );

    BOOST_TEST( zone.IsOnCopperLayer() == true );
}

/**
 * During zone loading, the layer is set to Rescue if the layer is not found.
 * This is not a UI-visible layer, so make sure it can still be retreived.
 *
 * https://gitlab.com/kicad/code/kicad/-/issues/18553
 */
BOOST_AUTO_TEST_CASE( RescuedLayers )
{
    ZONE zone( &m_board );

    zone.SetLayer( Rescue );

    BOOST_TEST( zone.GetLayer() == Rescue );
    BOOST_TEST( zone.GetLayer() == zone.GetFirstLayer() );

    BOOST_TEST( zone.IsOnCopperLayer() == false );
}

/**
 * Verify that a rule area on all inner copper layers does not produce a
 * spurious layer validation error when the footprint uses the default
 * EXPAND_INNER_LAYERS stackup mode.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23042
 */
BOOST_AUTO_TEST_CASE( RuleAreaInnerLayersExpandMode )
{
    FOOTPRINT footprint( &m_board );
    footprint.SetStackupMode( FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS );

    ZONE* ruleArea = new ZONE( &footprint );
    ruleArea->SetIsRuleArea( true );
    ruleArea->SetLayerSet( LSET::InternalCuMask() );
    footprint.Add( ruleArea );

    // Collect all layers used by the footprint (mirrors GetAllUsedFootprintLayers
    // from dialog_footprint_properties_fp_editor.cpp)
    LSET usedLayers;

    footprint.RunOnChildren(
            [&]( BOARD_ITEM* aItem )
            {
                if( aItem->Type() == PCB_ZONE_T )
                    usedLayers |= static_cast<ZONE*>( aItem )->GetLayerSet();
                else
                    usedLayers.set( aItem->GetLayer() );
            },
            RECURSE_MODE::RECURSE );

    // In EXPAND_INNER_LAYERS mode, F_Cu, B_Cu and all inner copper layers
    // are valid, along with tech, user, and user-defined layers.
    LSET allowedLayers = LSET{ F_Cu, B_Cu } | LSET::InternalCuMask();
    allowedLayers |= LSET::UserDefinedLayersMask( 4 );

    usedLayers &= ~allowedLayers;
    usedLayers &= ~LSET::AllTechMask();
    usedLayers &= ~LSET::UserMask();

    BOOST_TEST( usedLayers.none() );
}

/**
 * Verify that GetPosition() on a zone with no outline vertices does not
 * throw or crash. Empty zones can be created by importers.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23125
 */
BOOST_AUTO_TEST_CASE( EmptyZoneGetPosition )
{
    ZONE zone( &m_board );
    zone.SetLayer( F_Cu );

    BOOST_TEST( zone.GetNumCorners() == 0 );
    BOOST_CHECK_NO_THROW( zone.GetPosition() );
    BOOST_TEST( zone.GetPosition() == VECTOR2I( 0, 0 ) );
}


BOOST_AUTO_TEST_CASE( ZoneMergeNull )
{
    std::vector<std::unique_ptr<ZONE>> zones;

    zones.emplace_back( std::make_unique<ZONE>( &m_board ) );
    zones.back()->SetLayer( F_Cu );

    zones.emplace_back( std::make_unique<ZONE>( &m_board ) );
    zones.back()->SetLayer( F_Cu );

    std::vector<std::unique_ptr<ZONE>> merged = MergeZonesWithSameOutline( std::move( zones ) );

    // They are the same, so they do merge
    BOOST_TEST( merged.size() == 1 );
}


BOOST_AUTO_TEST_CASE( ZoneMergeNonNullNoMerge )
{
    std::vector<std::unique_ptr<ZONE>> zones;

    zones.emplace_back( CreateSquareZone( m_board, BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ), F_Cu ) );
    zones.emplace_back( CreateSquareZone( m_board, BOX2I( VECTOR2I( 200, 200 ), VECTOR2I( 300, 300 ) ), B_Cu ) );

    std::vector<std::unique_ptr<ZONE>> merged = MergeZonesWithSameOutline( std::move( zones ) );

    // They are different, so they don't merge
    BOOST_TEST( merged.size() == 2 );
}


BOOST_AUTO_TEST_CASE( ZoneMergeNonNullMerge )
{
    std::vector<std::unique_ptr<ZONE>> zones;

    zones.emplace_back( CreateSquareZone( m_board, BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ), F_Cu ) );
    zones.emplace_back( CreateSimilarZone( m_board, *zones.back(), B_Cu ) );

    std::vector<std::unique_ptr<ZONE>> merged = MergeZonesWithSameOutline( std::move( zones ) );

    // They are the same, so they do merge
    BOOST_REQUIRE( merged.size() == 1 );

    BOOST_TEST( merged[0]->GetLayerSet() == ( LSET{ F_Cu, B_Cu } ) );
    BOOST_TEST( merged[0]->GetNumCorners() == 4 );
}


BOOST_AUTO_TEST_CASE( ZoneMergeMergeSameGeomDifferentOrder )
{
    std::vector<std::unique_ptr<ZONE>> zones;

    zones.emplace_back( CreateSquareZone( m_board, BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ), F_Cu ) );
    zones.emplace_back( CreateSimilarZone( m_board, *zones.back(), B_Cu ) );

    // Reverse the outline of one of them
    // Don't go overboard here - detailed tests of CompareGeometry
    // should be in the SHAPE_LINE_CHAIN tests.
    auto newPolyB = std::make_unique<SHAPE_POLY_SET>( *zones.back()->Outline() );
    newPolyB->Outline( 0 ).Reverse();
    zones.back()->SetOutline( newPolyB.release() );

    std::vector<std::unique_ptr<ZONE>> merged = MergeZonesWithSameOutline( std::move( zones ) );

    // They are the same, so they do merge
    BOOST_REQUIRE( merged.size() == 1 );

    BOOST_TEST( merged[0]->GetLayerSet() == LSET( { F_Cu, B_Cu } ) );
    BOOST_TEST( merged[0]->GetNumCorners() == 4 );
}

static PCB_VIA* AddVia( BOARD& aBoard, const VECTOR2I& aPos, int aNetCode,
                        PCB_LAYER_ID aTopLayer = F_Cu, PCB_LAYER_ID aBotLayer = B_Cu )
{
    PCB_VIA* via = new PCB_VIA( &aBoard );
    via->SetPosition( aPos );
    via->SetLayerPair( aTopLayer, aBotLayer );
    via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.6 ) );
    via->SetDrill( pcbIUScale.mmToIU( 0.3 ) );
    via->SetNetCode( aNetCode );
    aBoard.Add( via );
    return via;
}


static PAD* AddPadToBoard( BOARD& aBoard, const VECTOR2I& aPos, int aNetCode,
                           PCB_LAYER_ID aLayer = F_Cu )
{
    FOOTPRINT* fp = new FOOTPRINT( &aBoard );
    fp->SetPosition( aPos );
    aBoard.Add( fp );

    PAD* pad = new PAD( fp );
    pad->SetPosition( aPos );
    pad->SetSize( PADSTACK::ALL_LAYERS,
                  VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetLayerSet( LSET( { aLayer } ) );
    pad->SetNetCode( aNetCode );
    fp->Add( pad );
    return pad;
}


BOOST_AUTO_TEST_CASE( AutoPriority_NonOverlapping )
{
    NETINFO_ITEM* netA = new NETINFO_ITEM( &m_board, wxT( "NetA" ) );
    m_board.Add( netA );
    NETINFO_ITEM* netB = new NETINFO_ITEM( &m_board, wxT( "NetB" ) );
    m_board.Add( netB );

    auto zoneA = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( 0, 0 ),
                   VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) ),
            F_Cu );
    zoneA->SetNetCode( netA->GetNetCode() );
    zoneA->SetAssignedPriority( 5 );

    auto zoneB = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( pcbIUScale.mmToIU( 20 ), 0 ),
                   VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) ),
            F_Cu );
    zoneB->SetNetCode( netB->GetNetCode() );
    zoneB->SetAssignedPriority( 10 );

    ZONE* ptrA = zoneA.get();
    ZONE* ptrB = zoneB.get();
    m_board.Add( zoneA.release() );
    m_board.Add( zoneB.release() );

    AutoAssignZonePriorities( &m_board );

    BOOST_TEST( ptrA->GetAssignedPriority() <= ptrB->GetAssignedPriority() );
}


BOOST_AUTO_TEST_CASE( AutoPriority_ItemCountWins )
{
    NETINFO_ITEM* netA = new NETINFO_ITEM( &m_board, wxT( "NetA" ) );
    m_board.Add( netA );
    NETINFO_ITEM* netB = new NETINFO_ITEM( &m_board, wxT( "NetB" ) );
    m_board.Add( netB );

    auto zoneA = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( 0, 0 ),
                   VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) ) ),
            F_Cu );
    zoneA->SetNetCode( netA->GetNetCode() );

    auto zoneB = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ),
                   VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) ),
            F_Cu );
    zoneB->SetNetCode( netB->GetNetCode() );

    ZONE* ptrA = zoneA.get();
    ZONE* ptrB = zoneB.get();
    m_board.Add( zoneA.release() );
    m_board.Add( zoneB.release() );

    for( int i = 0; i < 5; i++ )
    {
        AddVia( m_board,
                VECTOR2I( pcbIUScale.mmToIU( 7 + i ), pcbIUScale.mmToIU( 10 ) ),
                netA->GetNetCode() );
    }

    AddVia( m_board,
            VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 7 ) ),
            netB->GetNetCode() );

    AutoAssignZonePriorities( &m_board );

    BOOST_TEST( ptrA->GetAssignedPriority() > ptrB->GetAssignedPriority() );
}


BOOST_AUTO_TEST_CASE( AutoPriority_SimilarCountsSmallerWins )
{
    NETINFO_ITEM* netA = new NETINFO_ITEM( &m_board, wxT( "NetA" ) );
    m_board.Add( netA );
    NETINFO_ITEM* netB = new NETINFO_ITEM( &m_board, wxT( "NetB" ) );
    m_board.Add( netB );

    auto zoneA = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( 0, 0 ),
                   VECTOR2I( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 30 ) ) ),
            F_Cu );
    zoneA->SetNetCode( netA->GetNetCode() );

    auto zoneB = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ),
                   VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) ),
            F_Cu );
    zoneB->SetNetCode( netB->GetNetCode() );

    ZONE* ptrA = zoneA.get();
    ZONE* ptrB = zoneB.get();
    m_board.Add( zoneA.release() );
    m_board.Add( zoneB.release() );

    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 8 ), pcbIUScale.mmToIU( 8 ) ),
            netA->GetNetCode() );
    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 12 ), pcbIUScale.mmToIU( 8 ) ),
            netA->GetNetCode() );
    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 8 ), pcbIUScale.mmToIU( 12 ) ),
            netB->GetNetCode() );
    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 12 ), pcbIUScale.mmToIU( 12 ) ),
            netB->GetNetCode() );

    AutoAssignZonePriorities( &m_board );

    BOOST_TEST( ptrB->GetAssignedPriority() > ptrA->GetAssignedPriority() );
}


BOOST_AUTO_TEST_CASE( AutoPriority_MultiLayerAggregate )
{
    m_board.SetCopperLayerCount( 2 );

    NETINFO_ITEM* netA = new NETINFO_ITEM( &m_board, wxT( "NetA" ) );
    m_board.Add( netA );
    NETINFO_ITEM* netB = new NETINFO_ITEM( &m_board, wxT( "NetB" ) );
    m_board.Add( netB );

    auto zoneA = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( 0, 0 ),
                   VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) ) ),
            F_Cu );
    zoneA->SetLayerSet( LSET( { F_Cu, B_Cu } ) );
    zoneA->SetNetCode( netA->GetNetCode() );

    auto zoneB = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ),
                   VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) ),
            F_Cu );
    zoneB->SetLayerSet( LSET( { F_Cu, B_Cu } ) );
    zoneB->SetNetCode( netB->GetNetCode() );

    ZONE* ptrA = zoneA.get();
    ZONE* ptrB = zoneB.get();
    m_board.Add( zoneA.release() );
    m_board.Add( zoneB.release() );

    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 8 ), pcbIUScale.mmToIU( 8 ) ),
            netA->GetNetCode(), F_Cu, B_Cu );
    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 12 ), pcbIUScale.mmToIU( 8 ) ),
            netA->GetNetCode(), F_Cu, B_Cu );

    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 8 ), pcbIUScale.mmToIU( 12 ) ),
            netB->GetNetCode(), F_Cu, B_Cu );
    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ),
            netB->GetNetCode(), F_Cu, B_Cu );
    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 12 ), pcbIUScale.mmToIU( 12 ) ),
            netB->GetNetCode(), F_Cu, B_Cu );

    AutoAssignZonePriorities( &m_board );

    BOOST_TEST( ptrB->GetAssignedPriority() > ptrA->GetAssignedPriority() );
}


BOOST_AUTO_TEST_CASE( AutoPriority_SameNetEqualPriority )
{
    NETINFO_ITEM* net = new NETINFO_ITEM( &m_board, wxT( "SharedNet" ) );
    m_board.Add( net );

    auto zoneA = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( 0, 0 ),
                   VECTOR2I( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 30 ) ) ),
            F_Cu );
    zoneA->SetNetCode( net->GetNetCode() );

    auto zoneB = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ),
                   VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) ),
            F_Cu );
    zoneB->SetNetCode( net->GetNetCode() );

    ZONE* ptrA = zoneA.get();
    ZONE* ptrB = zoneB.get();
    m_board.Add( zoneA.release() );
    m_board.Add( zoneB.release() );

    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 8 ), pcbIUScale.mmToIU( 8 ) ),
            net->GetNetCode() );
    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 12 ), pcbIUScale.mmToIU( 12 ) ),
            net->GetNetCode() );

    AutoAssignZonePriorities( &m_board );

    // Same-net overlapping zones are cooperative and must share equal priority
    BOOST_TEST( ptrA->GetAssignedPriority() == ptrB->GetAssignedPriority() );
}


BOOST_AUTO_TEST_CASE( AutoPriority_EqualAreaNoChange )
{
    NETINFO_ITEM* netA = new NETINFO_ITEM( &m_board, wxT( "NetA" ) );
    m_board.Add( netA );
    NETINFO_ITEM* netB = new NETINFO_ITEM( &m_board, wxT( "NetB" ) );
    m_board.Add( netB );

    // Two identical-sized overlapping zones with no items in the overlap
    auto zoneA = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( 0, 0 ),
                   VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) ) ),
            F_Cu );
    zoneA->SetNetCode( netA->GetNetCode() );
    zoneA->SetAssignedPriority( 50 );

    auto zoneB = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( 0, 0 ),
                   VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) ) ),
            F_Cu );
    zoneB->SetNetCode( netB->GetNetCode() );
    zoneB->SetAssignedPriority( 50 );

    ZONE* ptrA = zoneA.get();
    ZONE* ptrB = zoneB.get();
    m_board.Add( zoneA.release() );
    m_board.Add( zoneB.release() );

    bool changed = AutoAssignZonePriorities( &m_board );

    // Equal areas, no items: no ordering evidence, priorities must not change
    BOOST_TEST( changed == false );
    BOOST_TEST( ptrA->GetAssignedPriority() == 50u );
    BOOST_TEST( ptrB->GetAssignedPriority() == 50u );
}


BOOST_AUTO_TEST_CASE( AutoPriority_SameNetGroupInheritsEdge )
{
    NETINFO_ITEM* netGND = new NETINFO_ITEM( &m_board, wxT( "GND" ) );
    m_board.Add( netGND );
    NETINFO_ITEM* netVCC = new NETINFO_ITEM( &m_board, wxT( "VCC" ) );
    m_board.Add( netVCC );

    // Large GND zone (A) overlaps with small VCC zone (C).
    // Small GND zone (B) overlaps with A but NOT with C.
    // A should beat C (more items), and B should inherit A's priority.
    auto zoneA = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( 0, 0 ),
                   VECTOR2I( pcbIUScale.mmToIU( 40 ), pcbIUScale.mmToIU( 40 ) ) ),
            F_Cu );
    zoneA->SetNetCode( netGND->GetNetCode() );

    auto zoneB = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( pcbIUScale.mmToIU( 25 ), pcbIUScale.mmToIU( 25 ) ),
                   VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) ),
            F_Cu );
    zoneB->SetNetCode( netGND->GetNetCode() );

    auto zoneC = CreateSquareZone( m_board,
            BOX2I( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ),
                   VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) ),
            F_Cu );
    zoneC->SetNetCode( netVCC->GetNetCode() );

    ZONE* ptrA = zoneA.get();
    ZONE* ptrB = zoneB.get();
    ZONE* ptrC = zoneC.get();
    m_board.Add( zoneA.release() );
    m_board.Add( zoneB.release() );
    m_board.Add( zoneC.release() );

    // GND items in the A/C overlap region
    for( int i = 0; i < 4; i++ )
    {
        AddVia( m_board,
                VECTOR2I( pcbIUScale.mmToIU( 8 + i * 2 ), pcbIUScale.mmToIU( 10 ) ),
                netGND->GetNetCode() );
    }

    AddVia( m_board, VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 8 ) ),
            netVCC->GetNetCode() );

    AutoAssignZonePriorities( &m_board );

    // A beats C because GND has more items in the overlap
    BOOST_TEST( ptrA->GetAssignedPriority() > ptrC->GetAssignedPriority() );

    // B shares A's priority because they are same-net and overlap
    BOOST_TEST( ptrA->GetAssignedPriority() == ptrB->GetAssignedPriority() );
}


BOOST_AUTO_TEST_SUITE_END()

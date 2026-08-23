/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <import_export.h>
#include <qa_utils/api_test_utils.h>
#include <qa_utils/wx_utils/wx_assert.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>

#include <api/board/board_types.pb.h>

#include <board.h>
#include <footprint.h>
#include <pcb_barcode.h>
#include <pcb_dimension.h>
#include <pcb_griditem.h>
#include <pcb_reference_image.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <zone.h>


BOOST_AUTO_TEST_SUITE( ApiProto )

struct PROTO_TEST_FIXTURE
{
    PROTO_TEST_FIXTURE()
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( BoardTypes, PROTO_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "api_kitchen_sink", m_board );

    int barcodeCount = 0;
    int referenceImageCount = 0;
    int tableCount = 0;
    int textCount = 0;
    int textBoxCount = 0;
    int shapeCount = 0;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        switch( track->Type() )
        {
        case PCB_TRACE_T:
            testProtoFromKiCadObject<kiapi::board::types::Track>( track, m_board.get() );
            break;

        case PCB_ARC_T:
            testProtoFromKiCadObject<kiapi::board::types::Arc>( static_cast<PCB_ARC*>( track ),
                                                                m_board.get() );
            break;

        case PCB_VIA_T:
            // Vias are not strict-checked at the moment because m_zoneLayerOverrides is not
            // currently exposed to the API
            // TODO(JE) enable strict when fixed
            testProtoFromKiCadObject<kiapi::board::types::Via>( static_cast<PCB_VIA*>( track ),
                                                                m_board.get(), false );
            break;

        default:
            wxFAIL;
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
        testProtoFromKiCadObject<kiapi::board::types::FootprintInstance>( footprint, m_board.get() );

    for( ZONE* zone : m_board->Zones() )
        testProtoFromKiCadObject<kiapi::board::types::Zone>( zone, m_board.get() );

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        switch( item->Type() )
        {
        case PCB_DIM_ALIGNED_T:
            testProtoFromKiCadObject<kiapi::board::types::Dimension>(
                    static_cast<PCB_DIM_ALIGNED*>( item ), m_board.get() );
            break;

        case PCB_DIM_ORTHOGONAL_T:
            testProtoFromKiCadObject<kiapi::board::types::Dimension>(
                    static_cast<PCB_DIM_ORTHOGONAL*>( item ), m_board.get() );
            break;

        case PCB_DIM_CENTER_T:
            testProtoFromKiCadObject<kiapi::board::types::Dimension>(
                    static_cast<PCB_DIM_CENTER*>( item ), m_board.get() );
            break;

        case PCB_DIM_LEADER_T:
            testProtoFromKiCadObject<kiapi::board::types::Dimension>(
                    static_cast<PCB_DIM_LEADER*>( item ), m_board.get() );
            break;

        case PCB_DIM_RADIAL_T:
            testProtoFromKiCadObject<kiapi::board::types::Dimension>(
                    static_cast<PCB_DIM_RADIAL*>( item ), m_board.get() );
            break;

        case PCB_BARCODE_T:
            testProtoFromKiCadObject<kiapi::board::types::Barcode>(
                    static_cast<PCB_BARCODE*>( item ), m_board.get() );
            ++barcodeCount;
            break;

        case PCB_REFERENCE_IMAGE_T:
            testProtoFromKiCadObject<kiapi::board::types::ReferenceImage>(
                    static_cast<PCB_REFERENCE_IMAGE*>( item ), m_board.get() );
            ++referenceImageCount;
            break;

        case PCB_TABLE_T:
            testProtoFromKiCadObject<kiapi::board::types::Table>( static_cast<PCB_TABLE*>( item ), m_board.get() );
            ++tableCount;
            break;

        case PCB_TEXT_T:
            testProtoFromKiCadObject<kiapi::board::types::BoardText>(
                    static_cast<PCB_TEXT*>( item ), m_board.get() );
            ++textCount;
            break;

        case PCB_TEXTBOX_T:
            testProtoFromKiCadObject<kiapi::board::types::BoardTextBox>(
                    static_cast<PCB_TEXTBOX*>( item ), m_board.get() );
            ++textBoxCount;
            break;

        case PCB_SHAPE_T:
            testProtoFromKiCadObject<kiapi::board::types::BoardGraphicShape>(
                    static_cast<PCB_SHAPE*>( item ), m_board.get() );
            ++shapeCount;
            break;

        default: break;
        }
    }

    BOOST_CHECK_GT( barcodeCount, 0 );
    BOOST_CHECK_GT( referenceImageCount, 0 );
    BOOST_CHECK_GT( tableCount, 0 );
    BOOST_CHECK_GT( textCount, 0 );
    BOOST_CHECK_GT( textBoxCount, 0 );
    BOOST_CHECK_GT( shapeCount, 0 );
}


BOOST_FIXTURE_TEST_CASE( Padstacks, PROTO_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "padstacks", m_board );

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        switch( track->Type() )
        {
        case PCB_VIA_T:
            // Vias are not strict-checked at the moment because m_zoneLayerOverrides is not
            // currently exposed to the API
            // TODO(JE) enable strict when fixed
            testProtoFromKiCadObject<kiapi::board::types::Via>( static_cast<PCB_VIA*>( track ),
                                                                m_board.get(), false );
            break;

        default:
            wxFAIL;
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
        testProtoFromKiCadObject<kiapi::board::types::FootprintInstance>( footprint, m_board.get() );
}

/**
 * Round-trip a copper-thieving zone through the protobuf API.  The shared
 * testProtoFromKiCadObject helper relies on ZONE::operator==, which by
 * existing precedent does not compare fill-mode or hatch/thieving fields,
 * so we hand-check every thieving field plus the netless invariant.
 */
BOOST_FIXTURE_TEST_CASE( CopperThievingZoneRoundTrip, PROTO_TEST_FIXTURE )
{
    m_board = std::make_unique<BOARD>();

    ZONE* zone = new ZONE( m_board.get() );
    zone->SetLayer( F_Cu );
    zone->AppendCorner( VECTOR2I( 0, 0 ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 5 ), 0 ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ), -1 );
    zone->AppendCorner( VECTOR2I( 0, pcbIUScale.mmToIU( 5 ) ), -1 );
    zone->SetFillMode( ZONE_FILL_MODE::COPPER_THIEVING );

    THIEVING_SETTINGS thieving;
    thieving.pattern      = THIEVING_PATTERN::SQUARES;
    thieving.element_size = pcbIUScale.mmToIU( 0.75 );
    thieving.gap        = pcbIUScale.mmToIU( 2.0 );
    thieving.line_width   = pcbIUScale.mmToIU( 0.4 );
    thieving.stagger      = true;
    thieving.orientation     = EDA_ANGLE( 15.0, DEGREES_T );
    zone->SetThievingSettings( thieving );

    m_board->Add( zone );

    google::protobuf::Any any;
    BOOST_REQUIRE_NO_THROW( zone->Serialize( any ) );

    kiapi::board::types::Zone proto;
    BOOST_REQUIRE( any.UnpackTo( &proto ) );
    BOOST_REQUIRE( proto.has_copper_settings() );
    BOOST_REQUIRE( proto.copper_settings().has_thieving_settings() );

    std::unique_ptr<ZONE> roundTripped = std::make_unique<ZONE>( m_board.get() );
    BOOST_REQUIRE( roundTripped->Deserialize( any ) );

    BOOST_CHECK( roundTripped->GetFillMode() == ZONE_FILL_MODE::COPPER_THIEVING );
    // A board with no netinfo list returns GetNetCode() == -1 for an unbound zone.
    // The invariant we want is "no real net assigned"; netcode > 0 would mean a leak.
    BOOST_CHECK_LE( roundTripped->GetNetCode(), 0 );

    const THIEVING_SETTINGS& loaded = roundTripped->GetThievingSettings();
    BOOST_CHECK( loaded.pattern == THIEVING_PATTERN::SQUARES );
    BOOST_CHECK_EQUAL( loaded.element_size, thieving.element_size );
    BOOST_CHECK_EQUAL( loaded.gap, thieving.gap );
    BOOST_CHECK_EQUAL( loaded.line_width, thieving.line_width );
    BOOST_CHECK_EQUAL( loaded.stagger, true );
    BOOST_CHECK( loaded.orientation == EDA_ANGLE( 15.0, DEGREES_T ) );
}


BOOST_AUTO_TEST_CASE( GridItems )
{
    const auto makeGridItem = []()
    {
        return std::make_unique<PCB_GRIDITEM>( nullptr );
    };

    PCB_GRIDITEM cartesian( nullptr );
    cartesian.SetGridItemType( PCB_GRIDITEM_TYPE::CARTESIAN );
    cartesian.SetPosition( VECTOR2I( 1000000, -2000000 ) );
    cartesian.SetOrientationDegrees( 30.0 );
    cartesian.SetExtent( VECTOR2I( 5000000, 4000000 ) );
    cartesian.SetSpacing( VECTOR2I( 250000, 500000 ) );
    cartesian.SetAssignedPriority( 3 );
    cartesian.SetTickInterval( 5 );
    cartesian.SetAffectsRouting( false );
    cartesian.SetLocked( true );

    testProtoFromKiCadObject<kiapi::board::types::GridItem>( &cartesian, makeGridItem );

    PCB_GRIDITEM polar( nullptr );
    polar.SetGridItemType( PCB_GRIDITEM_TYPE::POLAR );
    polar.SetPosition( VECTOR2I( -750000, 125000 ) );
    polar.SetRadiusExtent( 8000000 );
    polar.SetRadiusSpacing( 1000000 );
    polar.SetPhiExtentDegrees( 270.0 );
    polar.SetPhiSpacingDegrees( 7.5 );
    polar.SetAssignedPriority( 1 );
    polar.SetAffectsCursor( false );
    polar.SetAffectsPlacement( false );

    testProtoFromKiCadObject<kiapi::board::types::GridItem>( &polar, makeGridItem );

    // The polar half of the grid must survive, not just the shared linear fields.
    google::protobuf::Any any;
    polar.Serialize( any );

    std::unique_ptr<PCB_GRIDITEM> roundTripped = makeGridItem();
    BOOST_REQUIRE( roundTripped->Deserialize( any ) );

    BOOST_CHECK( roundTripped->GetGridItemType() == PCB_GRIDITEM_TYPE::POLAR );
    BOOST_CHECK_EQUAL( roundTripped->GetRadiusExtent(), 8000000 );
    BOOST_CHECK_EQUAL( roundTripped->GetRadiusSpacing(), 1000000 );
    BOOST_CHECK( roundTripped->GetPhiExtent() == EDA_ANGLE( 270.0, DEGREES_T ) );
    BOOST_CHECK( roundTripped->GetPhiSpacing() == EDA_ANGLE( 7.5, DEGREES_T ) );
    BOOST_CHECK_EQUAL( roundTripped->GetAffectsCursor(), false );
    BOOST_CHECK_EQUAL( roundTripped->GetAffectsRouting(), true );
    BOOST_CHECK_EQUAL( roundTripped->GetAffectsPlacement(), false );
    BOOST_CHECK_EQUAL( roundTripped->IsLocked(), false );
}


BOOST_AUTO_TEST_SUITE_END()

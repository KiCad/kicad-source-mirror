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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_drc_violation_layers_issue24941.cpp
 * @brief Tests for DRC_ITEM::GetViolationLayers, the layer a clicked violation focuses on
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <board.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <zone.h>
#include <pcb_marker.h>
#include <drc/drc_item.h>

struct DRC_VIOLATION_LAYERS_FIXTURE
{
    DRC_VIOLATION_LAYERS_FIXTURE() { m_board = std::make_unique<BOARD>(); }

    std::unique_ptr<BOARD> m_board;
};


// Issue 24941: clicking a board edge clearance violation must focus Edge.Cuts,
// not the copper layer the marker sits on
BOOST_FIXTURE_TEST_CASE( EdgeClearanceFocusesEdgeCuts, DRC_VIOLATION_LAYERS_FIXTURE )
{
    PCB_SHAPE* edge = new PCB_SHAPE( m_board.get(), SHAPE_T::SEGMENT );
    edge->SetLayer( Edge_Cuts );
    edge->SetStart( VECTOR2I( 0, 0 ) );
    edge->SetEnd( VECTOR2I( 1000000, 0 ) );
    m_board->Add( edge );

    PCB_TRACK* track = new PCB_TRACK( m_board.get() );
    track->SetLayer( F_Cu );
    track->SetStart( VECTOR2I( 0, 100000 ) );
    track->SetEnd( VECTOR2I( 1000000, 100000 ) );
    m_board->Add( track );

    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_EDGE_CLEARANCE );
    drcItem->SetItems( edge->m_Uuid, track->m_Uuid );

    // The edge clearance provider stamps the marker with the copper item's layer
    PCB_MARKER* marker = new PCB_MARKER( drcItem, VECTOR2I( 0, 100000 ), F_Cu );
    m_board->Add( marker );

    PCB_LAYER_ID principalLayer;
    LSET         violationLayers;

    DRC_ITEM::GetViolationLayers( m_board.get(), drcItem, marker, principalLayer, violationLayers );

    BOOST_CHECK_EQUAL( static_cast<int>( principalLayer ), static_cast<int>( Edge_Cuts ) );
}


// Guard for the original fix: for a violation on a multi-layer item the marker
// layer must win over the first layer of the item's layer set
BOOST_FIXTURE_TEST_CASE( MarkerLayerTakesPriority, DRC_VIOLATION_LAYERS_FIXTURE )
{
    ZONE* zone = new ZONE( m_board.get() );
    zone->SetLayerSet( LSET( { F_Cu, B_Cu } ) );
    m_board->Add( zone );

    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CONNECTION_WIDTH );
    drcItem->SetItems( zone->m_Uuid );

    // The narrow neck is on B.Cu, the provider marks it there
    PCB_MARKER* marker = new PCB_MARKER( drcItem, VECTOR2I( 0, 0 ), B_Cu );
    m_board->Add( marker );

    PCB_LAYER_ID principalLayer;
    LSET         violationLayers;

    DRC_ITEM::GetViolationLayers( m_board.get(), drcItem, marker, principalLayer, violationLayers );

    BOOST_CHECK_EQUAL( static_cast<int>( principalLayer ), static_cast<int>( B_Cu ) );
}


// Without a marker the layer comes from intersecting the items' layer sets
BOOST_FIXTURE_TEST_CASE( FallbackToItemLayers, DRC_VIOLATION_LAYERS_FIXTURE )
{
    PCB_TRACK* track1 = new PCB_TRACK( m_board.get() );
    track1->SetLayer( B_Cu );
    track1->SetStart( VECTOR2I( 0, 0 ) );
    track1->SetEnd( VECTOR2I( 1000000, 0 ) );
    m_board->Add( track1 );

    PCB_TRACK* track2 = new PCB_TRACK( m_board.get() );
    track2->SetLayer( B_Cu );
    track2->SetStart( VECTOR2I( 0, 100000 ) );
    track2->SetEnd( VECTOR2I( 1000000, 100000 ) );
    m_board->Add( track2 );

    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
    drcItem->SetItems( track1->m_Uuid, track2->m_Uuid );

    PCB_LAYER_ID principalLayer;
    LSET         violationLayers;

    DRC_ITEM::GetViolationLayers( m_board.get(), drcItem, nullptr, principalLayer, violationLayers );

    BOOST_CHECK_EQUAL( static_cast<int>( principalLayer ), static_cast<int>( B_Cu ) );
    BOOST_CHECK( violationLayers.test( B_Cu ) );
}

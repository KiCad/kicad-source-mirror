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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <algorithm>
#include <memory>

#include <api/api_pcb_utils.h>
#include <api/api_utils.h>
#include <board.h>
#include <footprint.h>
#include <kiid.h>
#include <pad.h>
#include <settings/settings_manager.h>

using kiapi::common::commands::SelectionSpec;
using kiapi::common::commands::SyncSelection;


// The two hierarchical sheets of the complex_hierarchy demo.  Both instantiate ampli_ht.kicad_sch
// and carry 29 footprints; ten more sit on the root sheet and four are board-only
static const wxString c_verticalSheet = wxS( "00000000-0000-0000-0000-00004b3a1333" );
static const wxString c_horizontalSheet = wxS( "00000000-0000-0000-0000-00004b3a13a4" );

static const std::size_t c_footprintsPerSheet = 29;
static const std::size_t c_footprintsOnBoard = 72;


struct CROSS_PROBE_SELECTION_FIXTURE
{
    CROSS_PROBE_SELECTION_FIXTURE()
    {
        KI_TEST::LoadBoard( m_settingsManager, wxS( "complex_hierarchy" ), m_board );
    }

    /// Build the sheet path the schematic editor cross-probes with when a sheet is clicked
    KIID_PATH SchematicPath( const KIID& aRoot, const wxString& aSheet ) const
    {
        KIID_PATH path;
        path.push_back( aRoot );
        path.push_back( KIID( aSheet ) );

        return path;
    }

    std::size_t CountOnSheet( const KIID_PATH& aSheetPath ) const
    {
        return static_cast<std::size_t>(
                std::ranges::count_if( m_board->Footprints(),
                                       [&]( const FOOTPRINT* aFootprint )
                                       {
                                           return aFootprint->IsWithinSchematicSheet( aSheetPath );
                                       } ) );
    }

    std::vector<BOARD_ITEM*> Resolve( const SyncSelection& aRequest ) const
    {
        return kiapi::board::FindItemsFromSyncSelection( m_board.get(), aRequest.items() );
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_SUITE( CrossProbeSelection, CROSS_PROBE_SELECTION_FIXTURE )


BOOST_AUTO_TEST_CASE( SheetSelectionFindsItsFootprints )
{
    BOOST_REQUIRE( m_board );
    BOOST_REQUIRE_EQUAL( m_board->Footprints().size(), c_footprintsOnBoard );

    BOOST_CHECK_EQUAL( CountOnSheet( SchematicPath( KIID(), c_verticalSheet ) ), c_footprintsPerSheet );
    BOOST_CHECK_EQUAL( CountOnSheet( SchematicPath( KIID(), c_horizontalSheet ) ), c_footprintsPerSheet );
}


BOOST_AUTO_TEST_CASE( SiblingSheetsDoNotOverlap )
{
    // Both sheets instantiate ampli_ht.kicad_sch, so only the sheet UUID tells their symbols apart
    KIID_PATH vertical = SchematicPath( KIID(), c_verticalSheet );
    KIID_PATH horizontal = SchematicPath( KIID(), c_horizontalSheet );
    std::size_t matched = 0;

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        bool onVertical = footprint->IsWithinSchematicSheet( vertical );
        bool onHorizontal = footprint->IsWithinSchematicSheet( horizontal );

        BOOST_CHECK( !( onVertical && onHorizontal ) );

        if( onVertical || onHorizontal )
            matched++;
    }

    BOOST_CHECK_EQUAL( matched, 2 * c_footprintsPerSheet );
}


BOOST_AUTO_TEST_CASE( RootSheetUuidIsIgnored )
{
    // The root sheet UUID is regenerated on load and never recorded in a footprint's path, so
    // two schematic sessions must cross-probe to the same footprints
    KIID_PATH firstSession = SchematicPath( KIID(), c_verticalSheet );
    KIID_PATH secondSession = SchematicPath( KIID(), c_verticalSheet );

    BOOST_REQUIRE( firstSession.front() != secondSession.front() );
    BOOST_CHECK_EQUAL( CountOnSheet( firstSession ), CountOnSheet( secondSession ) );
    BOOST_CHECK_EQUAL( CountOnSheet( firstSession ), c_footprintsPerSheet );
}


BOOST_AUTO_TEST_CASE( RootSheetSelectsTheWholeBoard )
{
    KIID_PATH rootOnly;
    rootOnly.push_back( KIID() );

    BOOST_CHECK_EQUAL( CountOnSheet( rootOnly ), c_footprintsOnBoard );
    BOOST_CHECK_EQUAL( CountOnSheet( KIID_PATH() ), 0 );
}


BOOST_AUTO_TEST_CASE( PinSpecResolvesToItsPad )
{
    SyncSelection request;
    SelectionSpec* spec = request.add_items();
    spec->mutable_pad()->set_reference( "C4" );
    spec->mutable_pad()->set_number( "2" );

    std::vector<BOARD_ITEM*> items = Resolve( request );

    BOOST_REQUIRE_EQUAL( items.size(), 1 );
    BOOST_REQUIRE_EQUAL( items[0]->Type(), PCB_PAD_T );

    PAD* pad = static_cast<PAD*>( items[0] );

    BOOST_CHECK_EQUAL( pad->GetNumber(), wxS( "2" ) );
    BOOST_CHECK_EQUAL( pad->GetParentFootprint()->GetReference(), wxS( "C4" ) );
}


BOOST_AUTO_TEST_CASE( SheetSpecResolvesThroughTheApi )
{
    SyncSelection request;
    kiapi::common::PackSheetPath( *request.add_items()->mutable_sheet_path(),
                                  SchematicPath( KIID(), c_verticalSheet ) );

    BOOST_CHECK_EQUAL( Resolve( request ).size(), c_footprintsPerSheet );
}


BOOST_AUTO_TEST_CASE( SpecsResolveInRequestOrder )
{
    SyncSelection request;
    request.add_items()->mutable_footprint()->set_reference( "C5" );
    request.add_items()->mutable_footprint()->set_reference( "C3" );

    std::vector<BOARD_ITEM*> items = Resolve( request );

    BOOST_REQUIRE_EQUAL( items.size(), 2 );
    BOOST_CHECK_EQUAL( static_cast<FOOTPRINT*>( items[0] )->GetReference(), wxS( "C5" ) );
    BOOST_CHECK_EQUAL( static_cast<FOOTPRINT*>( items[1] )->GetReference(), wxS( "C3" ) );
}


BOOST_AUTO_TEST_SUITE_END()

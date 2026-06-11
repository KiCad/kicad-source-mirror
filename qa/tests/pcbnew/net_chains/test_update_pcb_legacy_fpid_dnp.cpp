/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>
#include <lib_id.h>
#include <netlist_reader/board_netlist_updater.h>
#include <qa_utils/wx_utils/unit_test_utils.h>


BOOST_AUTO_TEST_SUITE( UpdatePcbLegacyFpidDnp )


static LIB_ID makeFpid( const wxString& aLibrary, const wxString& aItem )
{
    LIB_ID id;
    id.SetLibNickname( aLibrary );
    id.SetLibItemName( aItem );
    return id;
}


// Reproduces https://gitlab.com/kicad/code/kicad/-/issues/24586
//
// A symbol carrying a legacy (bare) footprint field such as "0603" is placed on a board whose
// footprint is fully qualified as "my_footprints:0603". The variant handler in
// BOARD_NETLIST_UPDATER decides whether a footprint is the component's base footprint via
// fpidsEquivalent(). A strict equality test fails for legacy FPIDs (the board side always carries
// a library nickname), so the matching board footprint is mistaken for a non-base variant and
// gets a spurious "Do not place" attribute. The comparison must ignore the library nickname when
// the schematic side is legacy.
BOOST_AUTO_TEST_CASE( LegacySchematicFpidMatchesQualifiedBoardFpid )
{
    LIB_ID schematicFpid;
    BOOST_REQUIRE_EQUAL( schematicFpid.Parse( wxS( "0603" ), true ), -1 );
    BOOST_REQUIRE( schematicFpid.IsLegacy() );

    LIB_ID boardFpid = makeFpid( wxS( "my_footprints" ), wxS( "0603" ) );

    BOOST_CHECK( BOARD_NETLIST_UPDATER::fpidsEquivalent( boardFpid, schematicFpid ) );
}


// A legacy schematic FPID must not match a board footprint with a different item name, otherwise
// genuine non-base variant footprints would no longer be flagged.
BOOST_AUTO_TEST_CASE( LegacySchematicFpidDoesNotMatchDifferentItemName )
{
    LIB_ID schematicFpid;
    BOOST_REQUIRE_EQUAL( schematicFpid.Parse( wxS( "0603" ), true ), -1 );

    LIB_ID boardFpid = makeFpid( wxS( "my_footprints" ), wxS( "0805" ) );

    BOOST_CHECK( !BOARD_NETLIST_UPDATER::fpidsEquivalent( boardFpid, schematicFpid ) );
}


// When the schematic side is fully qualified, the comparison must be exact so that a footprint
// from a different library is correctly treated as a distinct (non-base) variant.
BOOST_AUTO_TEST_CASE( QualifiedSchematicFpidComparesExactly )
{
    LIB_ID schematicFpid = makeFpid( wxS( "Connector_PinHeader_2.54mm" ),
                                     wxS( "PinHeader_1x04_P2.54mm_Vertical" ) );

    LIB_ID sameBoardFpid = makeFpid( wxS( "Connector_PinHeader_2.54mm" ),
                                     wxS( "PinHeader_1x04_P2.54mm_Vertical" ) );
    LIB_ID otherLibBoardFpid = makeFpid( wxS( "my_footprints" ),
                                         wxS( "PinHeader_1x04_P2.54mm_Vertical" ) );

    BOOST_CHECK( BOARD_NETLIST_UPDATER::fpidsEquivalent( sameBoardFpid, schematicFpid ) );
    BOOST_CHECK( !BOARD_NETLIST_UPDATER::fpidsEquivalent( otherLibBoardFpid, schematicFpid ) );
}


BOOST_AUTO_TEST_SUITE_END()

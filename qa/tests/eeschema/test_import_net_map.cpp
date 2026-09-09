/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <import_net_map.h>
#include <reporter.h>

BOOST_AUTO_TEST_SUITE( ImportNetMap )

BOOST_AUTO_TEST_CASE( OnlyAnUnambiguousResolvedNetIsRenamedOnTheBoard )
{
    IMPORT_NET_MAP map;
    auto add = [&]( const wxString& generated, const wxString& target, IMPORT_NET_STATUS status )
    {
        IMPORT_NET_MAP_ENTRY entry;
        entry.originalName = generated;
        entry.generatedName = generated;
        entry.nameAtImport = target;
        entry.status = status;
        map.entries.push_back( entry );
    };

    add( wxS( "N1" ), wxS( "Net-(R1-Pad1)" ), IMPORT_NET_STATUS::RESOLVED );

    // One source reaching two KiCad nets: no single name to rename to.
    add( wxS( "N2" ), wxS( "Net-(R2-Pad1)" ), IMPORT_NET_STATUS::RESOLVED );
    add( wxS( "N2" ), wxS( "Net-(R3-Pad1)" ), IMPORT_NET_STATUS::RESOLVED );

    // Two sources reaching one KiCad net: renaming both would collide, and the applier rejects a
    // colliding batch outright, failing the whole board import.
    add( wxS( "N4" ), wxS( "Net-(R4-Pad1)" ), IMPORT_NET_STATUS::RESOLVED );
    add( wxS( "N5" ), wxS( "Net-(R4-Pad1)" ), IMPORT_NET_STATUS::RESOLVED );

    for( IMPORT_NET_STATUS status : { IMPORT_NET_STATUS::NO_CONNECT, IMPORT_NET_STATUS::UNCONNECTED,
                                      IMPORT_NET_STATUS::BUS, IMPORT_NET_STATUS::SPLIT } )
    {
        add( wxS( "N6" ), wxS( "Net-(R6-Pad1)" ), status );
    }

    WX_STRING_REPORTER reporter;
    std::map<wxString, wxString> names = GetBoardNetNameMap( map, reporter );

    BOOST_REQUIRE_EQUAL( names.size(), 1 );
    BOOST_CHECK_EQUAL( names.at( wxS( "N1" ) ), wxString( "Net-(R1-Pad1)" ) );
    BOOST_CHECK( reporter.GetMessages().Contains( wxS( "N2" ) ) );
    BOOST_CHECK( reporter.GetMessages().Contains( wxS( "Net-(R4-Pad1)" ) ) );
}

BOOST_AUTO_TEST_SUITE_END()

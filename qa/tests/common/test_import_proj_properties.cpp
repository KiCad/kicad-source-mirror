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

#include <import_proj_properties.h>

BOOST_AUTO_TEST_SUITE( ImportProjProperties )

BOOST_AUTO_TEST_CASE( NetNameMapRoundTripsAndDistinguishesAbsence )
{
    const std::map<wxString, wxString> names = { { wxS( "N123" ), wxS( "Net-(R1-Pad1)" ) },
                                                 { wxS( "N456" ), wxS( "/sheet/SDA" ) } };

    auto decoded = IMPORT_PROJ_PROPS::SplitNetNameMap( IMPORT_PROJ_PROPS::JoinNetNameMap( names ) );
    BOOST_REQUIRE( decoded );
    BOOST_CHECK( *decoded == names );

    // A design that maps no nets still has to report that the import ran, because the manager reads
    // a present map as the schematic import having succeeded.
    auto empty = IMPORT_PROJ_PROPS::SplitNetNameMap( IMPORT_PROJ_PROPS::JoinNetNameMap( {} ) );
    BOOST_REQUIRE( empty );
    BOOST_CHECK( empty->empty() );

    // An import that never replied decodes to nothing at all.
    BOOST_CHECK( !IMPORT_PROJ_PROPS::SplitNetNameMap( wxString() ) );
}

BOOST_AUTO_TEST_SUITE_END()

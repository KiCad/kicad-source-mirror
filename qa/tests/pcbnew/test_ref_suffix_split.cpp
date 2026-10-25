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

#include <board.h>
#include <footprint.h>
#include <mail_type.h>
#include <ref_suffix_split.h>


BOOST_AUTO_TEST_SUITE( RefSuffixSplit )


BOOST_AUTO_TEST_CASE( CopiesShareTheFirstPagesBase )
{
    const std::string id = "00000000-0000-0000-0000-0000000000";
    const std::string a = "/" + id + "b1/";
    const std::string b = "/" + id + "b2/";
    const std::string c = "/" + id + "b3/";

    BOARD board;

    auto add =
            [&]( const std::string& aSheet, const char* aSymbol, const wxString& aRef )
            {
                FOOTPRINT* footprint = new FOOTPRINT( &board );
                footprint->SetPath( KIID_PATH( wxString( aSheet + id + aSymbol ) ) );
                footprint->SetReference( aRef );
                board.Add( footprint, ADD_MODE::APPEND );
                return footprint->m_Uuid;
            };

    // Sheet C comes first in page order, so its unsuffixed copy numbers the shared base
    std::optional<SHEET_REF_SUFFIXES> suffixes = ParseRefSuffixes( std::string( MAIL_SCH_REF_SUFFIXES_TAG ) + "\n"
                                                                   + c + "\t\n" + b + "\t_M2\n" + a + "\t_M1\n/\t\n"
                                                                   + b + id + "a5\t_M1\n" );
    BOOST_REQUIRE( suffixes );

    KIID first = add( a, "a1", wxS( "R4_M1" ) );
    KIID second = add( b, "a1", wxS( "R4_M2" ) );
    KIID stale = add( a, "a3", wxS( "R6" ) );
    KIID bare = add( c, "a1", wxS( "R4" ) );
    KIID twin = add( "/", "a4", wxS( "R8" ) );
    add( c, "a4", wxS( "R8" ) );
    KIID package = add( b, "a5", wxS( "U9_M1" ) );

    std::map<KIID, FOOTPRINT_REF_SPLIT> splits = SplitRefSuffixes( board.Footprints(), &*suffixes );

    BOOST_CHECK( splits[first].Representative == bare && splits[second].Representative == bare );
    BOOST_CHECK( splits[stale].Unknown );
    BOOST_CHECK( splits[twin].Representative == twin );

    // A package on another sheet's suffix keeps that suffix
    BOOST_CHECK_EQUAL( splits[package].Base, wxS( "U9" ) );

    // Without a schematic, a suffix cannot be told from part of the reference
    BOOST_CHECK( SplitRefSuffixes( board.Footprints(), nullptr )[first].Unknown );
}


BOOST_AUTO_TEST_SUITE_END()

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

#include <mdw/client/mdw_baseline_design.h>


namespace
{

/// A ctrl-board placement from acme-passives, member item aItemId at content digest aDigest
MDW_PLACED_INPUT placed( const wxString& aReference, const wxString& aItemId, const wxString& aDigest )
{
    MDW_PLACED_INPUT input;
    input.reference = aReference;
    input.lib_id = wxS( "acme-passives:" ) + aItemId;
    input.content_digest = aDigest;
    input.member = MDW_ITEM_REF();
    input.member->source = wxS( "acme-library" );
    input.member->item_id = aItemId;
    return input;
}

} // namespace


BOOST_AUTO_TEST_SUITE( MdwBaselineDesign )


// Commit and Push reads the saved files again, and the exceptions recorded before it must follow their parts
BOOST_AUTO_TEST_CASE( ExceptionsFollowTheirPartsAcrossAReload )
{
    std::vector<MDW_DESIGN_PART> before =
            MdwGroupPlacedParts( { placed( wxS( "R1" ), wxS( "R" ), wxS( "v1:aa" ) ),
                                   placed( wxS( "D2" ), wxS( "D_Schottky" ), wxS( "v1:bb" ) ) } );

    std::vector<MDW_BASELINE_EXCEPTION> recorded( 2 );
    recorded[0].part_index = 0;
    recorded[0].note = wxS( "R1 stays" );
    recorded[1].part_index = 1;
    recorded[1].note = wxS( "D2 stays" );

    // The pull changed R1's library copy, and its schematic now places the diode first
    std::vector<MDW_DESIGN_PART> after =
            MdwGroupPlacedParts( { placed( wxS( "D2" ), wxS( "D_Schottky" ), wxS( "v1:bb" ) ),
                                   placed( wxS( "R1" ), wxS( "R" ), wxS( "v1:cc" ) ) } );

    std::vector<MDW_BASELINE_EXCEPTION> carried = MdwCarryExceptions( before, recorded, after );

    BOOST_REQUIRE_EQUAL( carried.size(), 1 );
    BOOST_CHECK_EQUAL( carried[0].part_index, 0u );
    BOOST_CHECK( carried[0].note == wxS( "D2 stays" ) );
}


BOOST_AUTO_TEST_SUITE_END()

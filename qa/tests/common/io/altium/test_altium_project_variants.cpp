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

#include <common/io/altium/altium_project_variants.h>


BOOST_AUTO_TEST_SUITE( AltiumProjectVariants )


BOOST_AUTO_TEST_CASE( ParseVariationFields )
{
    ALTIUM_VARIANT_ENTRY entry = ParseVariationString(
            wxS( "Designator=R1|UniqueId=ROOT\\SHEET\\ABCDEFGH|Kind=0|AlternatePart=RES_ALT|Comment=10k" ) );

    BOOST_CHECK_EQUAL( entry.designator, wxS( "R1" ) );

    // Only the final backslash-delimited path segment is the component's own unique id
    BOOST_CHECK_EQUAL( entry.uniqueId, wxS( "ABCDEFGH" ) );
    BOOST_CHECK_EQUAL( entry.kind, 0 );
    BOOST_CHECK_EQUAL( entry.alternateFields[wxS( "LibReference" )], wxS( "RES_ALT" ) );
    BOOST_CHECK_EQUAL( entry.alternateFields[wxS( "Comment" )], wxS( "10k" ) );
}


BOOST_AUTO_TEST_CASE( ParseVariationBackslashBeforeSeparator )
{
    // A UniqueId path ending in a backslash sits right before the '|', so wxSplit's default
    // escape eats the separator and Kind is swallowed, leaving the entry read back as fitted
    ALTIUM_VARIANT_ENTRY entry =
            ParseVariationString( wxS( "Designator=R1|UniqueId=ROOT\\SHEET\\|Kind=1" ) );

    BOOST_CHECK_EQUAL( entry.designator, wxS( "R1" ) );
    BOOST_CHECK_EQUAL( entry.kind, 1 );
    BOOST_CHECK_EQUAL( entry.uniqueId, wxS( "" ) );
}


BOOST_AUTO_TEST_SUITE_END()

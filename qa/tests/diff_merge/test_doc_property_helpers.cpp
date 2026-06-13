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

#include <diff_merge/doc_property_helpers.h>


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( DocPropertyHelpers )


// AppendPaperDeltas ---------------------------------------------------------

BOOST_AUTO_TEST_CASE( AppendPaperDeltas_IdenticalProducesNoDeltas )
{
    PAGE_INFO before( PAGE_SIZE_TYPE::A4, /*portrait*/ false );
    PAGE_INFO after ( PAGE_SIZE_TYPE::A4, /*portrait*/ false );

    std::vector<PROPERTY_DELTA> deltas;
    AppendPaperDeltas( deltas, before, after );

    BOOST_CHECK( deltas.empty() );
}


BOOST_AUTO_TEST_CASE( AppendPaperDeltas_FormatChangeOnly )
{
    PAGE_INFO before( PAGE_SIZE_TYPE::A4, false );
    PAGE_INFO after ( PAGE_SIZE_TYPE::A3, false );

    std::vector<PROPERTY_DELTA> deltas;
    AppendPaperDeltas( deltas, before, after );

    BOOST_REQUIRE_EQUAL( deltas.size(), 1u );
    BOOST_CHECK( deltas[0].name == DOC_PROP_PAGE_FORMAT );
    // Format is encoded as ENUM with both the int (PAGE_SIZE_TYPE) and the
    // label string (GetTypeAsString). Pin both halves — a regression that
    // dropped the label would still pass an int-only check.
    BOOST_CHECK( deltas[0].before.GetType() == DIFF_VALUE::T::ENUM );
    BOOST_CHECK( deltas[0].after.GetType()  == DIFF_VALUE::T::ENUM );
    BOOST_CHECK_EQUAL( deltas[0].before.AsEnum().first,
                       static_cast<int>( PAGE_SIZE_TYPE::A4 ) );
    BOOST_CHECK_EQUAL( deltas[0].after.AsEnum().first,
                       static_cast<int>( PAGE_SIZE_TYPE::A3 ) );
    BOOST_CHECK_EQUAL( deltas[0].before.AsEnum().second,
                       before.GetTypeAsString().ToStdString() );
    BOOST_CHECK_EQUAL( deltas[0].after.AsEnum().second,
                       after.GetTypeAsString().ToStdString() );
}


BOOST_AUTO_TEST_CASE( AppendPaperDeltas_OrientationChangeOnly )
{
    PAGE_INFO before( PAGE_SIZE_TYPE::A4, /*portrait*/ false );
    PAGE_INFO after ( PAGE_SIZE_TYPE::A4, /*portrait*/ true );

    std::vector<PROPERTY_DELTA> deltas;
    AppendPaperDeltas( deltas, before, after );

    BOOST_REQUIRE_EQUAL( deltas.size(), 1u );
    BOOST_CHECK( deltas[0].name == DOC_PROP_PAGE_ORIENTATION );
    // Orientation is encoded as BOOL (the IsPortrait flag).
    BOOST_CHECK( deltas[0].before.GetType() == DIFF_VALUE::T::BOOL );
    BOOST_CHECK( deltas[0].after.GetType()  == DIFF_VALUE::T::BOOL );
    BOOST_CHECK_EQUAL( deltas[0].before.AsBool(), false );
    BOOST_CHECK_EQUAL( deltas[0].after.AsBool(),  true );
}


BOOST_AUTO_TEST_CASE( AppendPaperDeltas_BothChange_EmitsTwoInDocumentedOrder )
{
    // Format delta MUST be appended before orientation delta — downstream
    // marker rendering expects this stable order, so a refactor that flips
    // them should trip the test.
    PAGE_INFO before( PAGE_SIZE_TYPE::A4, false );
    PAGE_INFO after ( PAGE_SIZE_TYPE::A3, true );

    std::vector<PROPERTY_DELTA> deltas;
    AppendPaperDeltas( deltas, before, after );

    BOOST_REQUIRE_EQUAL( deltas.size(), 2u );
    BOOST_CHECK( deltas[0].name == DOC_PROP_PAGE_FORMAT );
    BOOST_CHECK( deltas[1].name == DOC_PROP_PAGE_ORIENTATION );
}


BOOST_AUTO_TEST_CASE( AppendPaperDeltas_AppendsToExistingVector )
{
    // The helper appends; it must not replace caller-seeded entries. The
    // PCB and SCH callers stage additional document-level deltas (board
    // thickness, drawing sheet, etc.) in the same vector before/after
    // calling AppendPaperDeltas.
    std::vector<PROPERTY_DELTA> deltas;
    PROPERTY_DELTA preExisting;
    preExisting.name = wxS( "Sentinel" );
    deltas.push_back( preExisting );

    PAGE_INFO before( PAGE_SIZE_TYPE::A4, false );
    PAGE_INFO after ( PAGE_SIZE_TYPE::A3, false );
    AppendPaperDeltas( deltas, before, after );

    BOOST_REQUIRE_EQUAL( deltas.size(), 2u );
    BOOST_CHECK( deltas[0].name == wxS( "Sentinel" ) );
    BOOST_CHECK( deltas[1].name == DOC_PROP_PAGE_FORMAT );
}


BOOST_AUTO_TEST_SUITE_END()

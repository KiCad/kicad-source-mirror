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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * Issue #23023. HTTP library fields content was missing in the symbol chooser.
 *
 * The category listing returns only minimal part data, so fields were absent until a part
 * was selected individually.  setPartExtendedData() is the shared parser that pulls the
 * field content (and exclusion flags) out of a part's JSON record; it must populate every
 * field the record carries and report whether the record contained the detailed field set,
 * which drives the per-part back-fill fetch during enumeration.
 *
 * The JSON below is the recorded RC0603FR-0710KL detail response from the KiCad HTTP test
 * server fixture (qa/tests/eeschema/http_lib_test_server.py).
 */

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <tuple>

#include <json_common.h>
#include <http_lib/http_lib_connection.h>
#include <http_lib/http_lib_settings.h>


BOOST_AUTO_TEST_SUITE( Issue23023HttpFields )


static const HTTP_LIB_PART::field_type* findField( const HTTP_LIB_PART& aPart, const std::string& aName )
{
    for( const auto& [name, properties] : aPart.fields )
    {
        if( name == aName )
            return &properties;
    }

    return nullptr;
}


/// A detail record with a fields object populates every field and reports itself as detailed.
BOOST_AUTO_TEST_CASE( DetailRecordPopulatesFields )
{
    const nlohmann::json detail = R"({
        "id": "1",
        "name": "RC0603FR-0710KL",
        "symbolIdStr": "Device:R",
        "exclude_from_bom": "False",
        "exclude_from_board": "False",
        "exclude_from_sim": "True",
        "fields": {
            "footprint": { "value": "Resistor_SMD:R_0603_1608Metric", "visible": "False" },
            "datasheet": { "value": "www.kicad.org", "visible": "False" },
            "value": { "value": "10k" },
            "reference": { "value": "R" }
        }
    })"_json;

    HTTP_LIB_PART part;

    BOOST_CHECK( setPartExtendedData( detail, part ) );

    BOOST_CHECK_EQUAL( part.symbolIdStr, "Device:R" );
    BOOST_CHECK_EQUAL( part.exclude_from_sim, true );
    BOOST_CHECK_EQUAL( part.exclude_from_bom, false );

    BOOST_REQUIRE_EQUAL( part.fields.size(), 4u );

    const HTTP_LIB_PART::field_type* value = findField( part, "value" );
    BOOST_REQUIRE( value );
    BOOST_CHECK_EQUAL( std::get<0>( *value ), "10k" );

    // A field with no explicit "visible" key must default to visible.
    BOOST_CHECK_EQUAL( std::get<1>( *value ), true );

    const HTTP_LIB_PART::field_type* footprint = findField( part, "footprint" );
    BOOST_REQUIRE( footprint );
    BOOST_CHECK_EQUAL( std::get<0>( *footprint ), "Resistor_SMD:R_0603_1608Metric" );
    BOOST_CHECK_EQUAL( std::get<1>( *footprint ), false );
}


/// A minimal category-listing record carries no fields, so the parser reports "not detailed"
/// and the enumeration back-fill is triggered.  This is the exact condition of the bug.
BOOST_AUTO_TEST_CASE( ListingRecordReportsNotDetailed )
{
    const nlohmann::json listing = R"({
        "id": "1",
        "name": "RC0603FR-0710KL",
        "description": "10 kOhms resistor"
    })"_json;

    HTTP_LIB_PART part;

    BOOST_CHECK( !setPartExtendedData( listing, part ) );
    BOOST_CHECK( part.fields.empty() );
}


/// Exclusion flags and field visibility encoded as native JSON booleans must parse the same
/// as their string equivalents rather than throwing and aborting the whole category.
BOOST_AUTO_TEST_CASE( NativeBooleanFlagsAreAccepted )
{
    const nlohmann::json detail = R"({
        "id": "1",
        "symbolIdStr": "Device:R",
        "exclude_from_bom": true,
        "fields": {
            "mpn": { "value": "RC0603FR-0710KL", "visible": false }
        }
    })"_json;

    HTTP_LIB_PART part;

    BOOST_CHECK_NO_THROW( setPartExtendedData( detail, part ) );
    BOOST_CHECK_EQUAL( part.exclude_from_bom, true );

    const HTTP_LIB_PART::field_type* mpn = findField( part, "mpn" );
    BOOST_REQUIRE( mpn );
    BOOST_CHECK_EQUAL( std::get<1>( *mpn ), false );
}


BOOST_AUTO_TEST_SUITE_END()

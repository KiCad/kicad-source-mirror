/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_io/pads/pads_sch_binary_parser.h>
#include <sch_io/pads/pads_sch_binary_reader.h>

#include <ki_exception.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

using namespace PADS_SCH_BINARY;

namespace
{

static std::vector<uint8_t> loadMinimalV13()
{
    std::ifstream file( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/minimal_v13.sch", std::ios::binary );
    BOOST_REQUIRE( file );
    return { std::istreambuf_iterator<char>( file ), std::istreambuf_iterator<char>() };
}


struct SNAPSHOT_PROPERTY
{
    std::string          name;
    std::string          value;
    PROPERTY_DISPOSITION disposition = PROPERTY_DISPOSITION::EXACT;

    bool operator==( const SNAPSHOT_PROPERTY& ) const = default;

    bool operator<( const SNAPSHOT_PROPERTY& aOther ) const
    {
        return std::tie( name, value, disposition ) < std::tie( aOther.name, aOther.value, aOther.disposition );
    }
};


struct SNAPSHOT_ITEM
{
    std::string                    sheet;
    std::string                    kind;
    std::vector<SNAPSHOT_PROPERTY> properties;
    std::string                    geometry;

    bool operator==( const SNAPSHOT_ITEM& ) const = default;

    bool operator<( const SNAPSHOT_ITEM& aOther ) const
    {
        return std::tie( sheet, kind, properties, geometry )
               < std::tie( aOther.sheet, aOther.kind, aOther.properties, aOther.geometry );
    }
};


static bool snapshotsMatch( std::vector<SNAPSHOT_ITEM> aExpected, std::vector<SNAPSHOT_ITEM> aActual,
                            const std::set<PROPERTY_DISPOSITION>& aAllowedDifferences = {} )
{
    auto stripAllowed = [&]( std::vector<SNAPSHOT_ITEM>& aItems )
    {
        for( SNAPSHOT_ITEM& item : aItems )
        {
            std::erase_if( item.properties,
                           [&]( const SNAPSHOT_PROPERTY& aProperty )
                           {
                               return aAllowedDifferences.contains( aProperty.disposition );
                           } );
            std::sort( item.properties.begin(), item.properties.end() );
        }

        std::sort( aItems.begin(), aItems.end() );
    };

    stripAllowed( aExpected );
    stripAllowed( aActual );
    return aExpected == aActual;
}

} // namespace


BOOST_AUTO_TEST_SUITE( PadsSchBinaryParser )


BOOST_AUTO_TEST_CASE( ModelContract )
{
    static_assert( !std::is_same_v<SHEET_ID, PLACEMENT_ID> );
    static_assert( !std::is_convertible_v<SHEET_ID, PLACEMENT_ID> );

    const std::vector<uint8_t> bytes = loadMinimalV13();
    PADS_SCH_BINARY_PARSER     parser;
    PADS_SCH_MODEL             first = parser.Parse( bytes, wxS( "minimal_v13.sch" ) );
    PADS_SCH_MODEL             second = parser.Parse( bytes, wxS( "minimal_v13.sch" ) );

    BOOST_CHECK_EQUAL( first.version, 0x000D );
    BOOST_REQUIRE_EQUAL( first.sheets.size(), 1 );
    BOOST_CHECK( first.AllReferencesResolved() );
    BOOST_CHECK( first.HasUniqueTypedIds() );
    BOOST_CHECK_EQUAL( first.sheets[0].index, 0 );
    BOOST_CHECK_EQUAL( first.sheets[0].id.Value(), 0 );
    BOOST_CHECK_EQUAL( first.sheets[0].source.file, wxS( "minimal_v13.sch" ) );
    BOOST_CHECK_GT( first.sheets[0].source.length, 0 );
    BOOST_CHECK_EQUAL( first.sheets[0].source.offset, second.sheets[0].source.offset );
    BOOST_CHECK_EQUAL( first.sheets[0].source.length, second.sheets[0].source.length );
    BOOST_CHECK_EQUAL( first.sheets[0].id.Value(), second.sheets[0].id.Value() );

    BOOST_CHECK_EQUAL( NormalizeAngle( -900 ), 2700 );
    BOOST_CHECK_EQUAL( NormalizeAngle( 4500 ), 900 );
    BOOST_CHECK_EQUAL( NormalizeCoordinate( -1234 ), -1234 );

    SOURCE_STRING sourceString{
        { 0x41, 0xC3, 0xA9 }, wxString::FromUTF8( "A\xC3\xA9" ), STRING_ENCODING_STATUS::UTF8, first.sheets[0].source
    };
    BOOST_CHECK_EQUAL( sourceString.raw.size(), 3 );
    BOOST_CHECK_EQUAL( sourceString.text, wxString::FromUTF8( "A\xC3\xA9" ) );
    BOOST_CHECK( sourceString.encoding == STRING_ENCODING_STATUS::UTF8 );

    first.diagnostics.push_back(
            { RPT_SEVERITY_WARNING, first.sheets[0].source, wxS( "unknown but structurally valid enum" ) } );
    BOOST_CHECK_EQUAL( first.diagnostics[0].source.offset, first.sheets[0].source.offset );

    PADS_SCH_MODEL duplicateIds = second;
    duplicateIds.sheets.push_back( duplicateIds.sheets.front() );
    BOOST_CHECK_EXCEPTION( duplicateIds.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "offset 0x0" ) )
                                      && aError.What().Contains( wxS( "duplicate" ) );
                           } );

    PADS_SCH_MODEL unresolved = second;
    unresolved.sheets[0].parent = SHEET_ID( 99 );
    BOOST_CHECK_EXCEPTION( unresolved.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "unresolved" ) );
                           } );

    PADS_SCH_MODEL cyclic = second;
    cyclic.sheets[0].parent = cyclic.sheets[0].id;
    BOOST_CHECK_EXCEPTION( cyclic.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "cyclic" ) )
                                      && aError.What().Contains( wxS( "offset" ) );
                           } );
}


BOOST_AUTO_TEST_CASE( CorpusSemanticSnapshot )
{
    const SNAPSHOT_ITEM duplicateA{ "Main", "symbol", { { "value", "10k" } }, "100,200" };
    const SNAPSHOT_ITEM duplicateB{ "Main", "symbol", { { "value", "22k" } }, "300,400" };

    std::vector<SNAPSHOT_ITEM> expected{ duplicateA, duplicateB };
    std::vector<SNAPSHOT_ITEM> reordered{ duplicateB, duplicateA };
    BOOST_CHECK( snapshotsMatch( expected, reordered ) );

    reordered[0].properties[0].value = "47k";
    BOOST_CHECK( !snapshotsMatch( expected, reordered ) );

    expected[0].properties.push_back( { "font", "PADS stroke", PROPERTY_DISPOSITION::APPROXIMATE } );
    reordered = { duplicateB, duplicateA };
    BOOST_CHECK( !snapshotsMatch( expected, reordered ) );
    BOOST_CHECK( snapshotsMatch( expected, reordered, { PROPERTY_DISPOSITION::APPROXIMATE } ) );
}


BOOST_AUTO_TEST_SUITE_END()

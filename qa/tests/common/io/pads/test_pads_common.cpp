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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file test_pads_common.cpp
 * Test suite for PADS common utilities including deterministic UUID generation.
 */

#include <boost/test/unit_test.hpp>
#include <io/pads/pads_common.h>
#include <kiid.h>
#include <wx/string.h>

#include <qa_utils/wx_utils/unit_test_utils.h>


BOOST_AUTO_TEST_SUITE( PadsCommon )


BOOST_AUTO_TEST_CASE( GenerateDeterministicUuid_Deterministic )
{
    // Same input should always produce the same UUID
    std::string identifier = "R1";

    KIID uuid1 = PADS_COMMON::GenerateDeterministicUuid( identifier );
    KIID uuid2 = PADS_COMMON::GenerateDeterministicUuid( identifier );

    BOOST_CHECK( uuid1 == uuid2 );
    BOOST_CHECK_EQUAL( uuid1.AsString(), uuid2.AsString() );
}


BOOST_AUTO_TEST_CASE( GenerateDeterministicUuid_DifferentInputs )
{
    // Different inputs should produce different UUIDs
    KIID uuid1 = PADS_COMMON::GenerateDeterministicUuid( "R1" );
    KIID uuid2 = PADS_COMMON::GenerateDeterministicUuid( "R2" );
    KIID uuid3 = PADS_COMMON::GenerateDeterministicUuid( "C1" );

    BOOST_CHECK( uuid1 != uuid2 );
    BOOST_CHECK( uuid1 != uuid3 );
    BOOST_CHECK( uuid2 != uuid3 );
}


BOOST_AUTO_TEST_CASE( GenerateDeterministicUuid_ValidFormat )
{
    // UUID should be in valid format
    KIID uuid = PADS_COMMON::GenerateDeterministicUuid( "U1" );
    wxString uuidStr = uuid.AsString();

    // UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36 chars)
    BOOST_CHECK_EQUAL( uuidStr.Length(), 36u );
    BOOST_CHECK( uuidStr[8] == '-' );
    BOOST_CHECK( uuidStr[13] == '-' );
    BOOST_CHECK( uuidStr[18] == '-' );
    BOOST_CHECK( uuidStr[23] == '-' );

    // Check version nibble is 4 (position 14)
    BOOST_CHECK( uuidStr[14] == '4' );
}


BOOST_AUTO_TEST_CASE( GenerateDeterministicUuid_EmptyString )
{
    // Empty string should still produce a valid UUID
    KIID uuid = PADS_COMMON::GenerateDeterministicUuid( "" );
    wxString uuidStr = uuid.AsString();

    BOOST_CHECK_EQUAL( uuidStr.Length(), 36u );
}


BOOST_AUTO_TEST_CASE( GenerateDeterministicUuid_LongString )
{
    // Long string should still produce a valid UUID
    std::string longId = "VERY_LONG_COMPONENT_NAME_WITH_LOTS_OF_CHARACTERS_12345";
    KIID uuid = PADS_COMMON::GenerateDeterministicUuid( longId );
    wxString uuidStr = uuid.AsString();

    BOOST_CHECK_EQUAL( uuidStr.Length(), 36u );

    // Same long string should produce same UUID
    KIID uuid2 = PADS_COMMON::GenerateDeterministicUuid( longId );
    BOOST_CHECK( uuid == uuid2 );
}


BOOST_AUTO_TEST_CASE( GenerateDeterministicUuid_CrossProbeConsistency )
{
    // Simulate cross-probe linking: same identifier from both schematic and PCB
    // should produce the same UUID
    std::string schematicId = "PADS:IC_QUAD_NAND/U1";
    std::string pcbId = "PADS:IC_QUAD_NAND/U1";

    KIID schUuid = PADS_COMMON::GenerateDeterministicUuid( schematicId );
    KIID pcbUuid = PADS_COMMON::GenerateDeterministicUuid( pcbId );

    BOOST_CHECK( schUuid == pcbUuid );
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( PadsSafeParsing )


BOOST_AUTO_TEST_CASE( ParseInt_ValidInput )
{
    BOOST_CHECK_EQUAL( PADS_COMMON::ParseInt( "42" ), 42 );
    BOOST_CHECK_EQUAL( PADS_COMMON::ParseInt( "-7" ), -7 );
    BOOST_CHECK_EQUAL( PADS_COMMON::ParseInt( "0" ), 0 );
    BOOST_CHECK_EQUAL( PADS_COMMON::ParseInt( "  123" ), 123 );
}


BOOST_AUTO_TEST_CASE( ParseInt_InvalidInput )
{
    BOOST_CHECK_EQUAL( PADS_COMMON::ParseInt( "abc" ), 0 );
    BOOST_CHECK_EQUAL( PADS_COMMON::ParseInt( "" ), 0 );
    BOOST_CHECK_EQUAL( PADS_COMMON::ParseInt( "abc", 99 ), 99 );
    BOOST_CHECK_EQUAL( PADS_COMMON::ParseInt( "12.5" ), 12 );
}


BOOST_AUTO_TEST_CASE( ParseInt_Overflow )
{
    BOOST_CHECK_EQUAL( PADS_COMMON::ParseInt( "99999999999999999", -1 ), -1 );
}


BOOST_AUTO_TEST_CASE( ParseDouble_ValidInput )
{
    BOOST_CHECK_CLOSE( PADS_COMMON::ParseDouble( "3.14" ), 3.14, 0.001 );
    BOOST_CHECK_CLOSE( PADS_COMMON::ParseDouble( "-2.5" ), -2.5, 0.001 );
    BOOST_CHECK_CLOSE( PADS_COMMON::ParseDouble( "0.0" ), 0.0, 0.001 );
    BOOST_CHECK_CLOSE( PADS_COMMON::ParseDouble( "100" ), 100.0, 0.001 );
}


BOOST_AUTO_TEST_CASE( ParseDouble_InvalidInput )
{
    BOOST_CHECK_CLOSE( PADS_COMMON::ParseDouble( "xyz" ), 0.0, 0.001 );
    BOOST_CHECK_CLOSE( PADS_COMMON::ParseDouble( "" ), 0.0, 0.001 );
    BOOST_CHECK_CLOSE( PADS_COMMON::ParseDouble( "xyz", 1.5 ), 1.5, 0.001 );
}


BOOST_AUTO_TEST_CASE( ParseDouble_Overflow )
{
    BOOST_CHECK_CLOSE( PADS_COMMON::ParseDouble( "1e999", -1.0 ), -1.0, 0.001 );
}


BOOST_AUTO_TEST_CASE( PadsLineStyleToKiCad_KnownStyles )
{
    BOOST_CHECK_EQUAL( static_cast<int>( PADS_COMMON::PadsLineStyleToKiCad( 1 ) ),
                       static_cast<int>( LINE_STYLE::SOLID ) );
    BOOST_CHECK_EQUAL( static_cast<int>( PADS_COMMON::PadsLineStyleToKiCad( -1 ) ),
                       static_cast<int>( LINE_STYLE::SOLID ) );
    BOOST_CHECK_EQUAL( static_cast<int>( PADS_COMMON::PadsLineStyleToKiCad( -2 ) ),
                       static_cast<int>( LINE_STYLE::DASH ) );
    BOOST_CHECK_EQUAL( static_cast<int>( PADS_COMMON::PadsLineStyleToKiCad( -3 ) ),
                       static_cast<int>( LINE_STYLE::DOT ) );
    BOOST_CHECK_EQUAL( static_cast<int>( PADS_COMMON::PadsLineStyleToKiCad( -4 ) ),
                       static_cast<int>( LINE_STYLE::DASHDOT ) );
    BOOST_CHECK_EQUAL( static_cast<int>( PADS_COMMON::PadsLineStyleToKiCad( -5 ) ),
                       static_cast<int>( LINE_STYLE::DASHDOTDOT ) );
}


BOOST_AUTO_TEST_CASE( PadsLineStyleToKiCad_DefaultFallback )
{
    // Style 0 is the old dashed format
    BOOST_CHECK_EQUAL( static_cast<int>( PADS_COMMON::PadsLineStyleToKiCad( 0 ) ),
                       static_cast<int>( LINE_STYLE::DASH ) );

    // Unknown values fall back to SOLID
    BOOST_CHECK_EQUAL( static_cast<int>( PADS_COMMON::PadsLineStyleToKiCad( 42 ) ),
                       static_cast<int>( LINE_STYLE::SOLID ) );
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( PadsNetNameConversion )


BOOST_AUTO_TEST_CASE( ConvertInvertedNetName_InvertedSignal )
{
    // PADS "/" prefix indicates an inverted signal, converted to KiCad overbar notation
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "/CNTRL" ), wxS( "~{CNTRL}" ) );
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "/RESET" ), wxS( "~{RESET}" ) );
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "/CS" ), wxS( "~{CS}" ) );
}


BOOST_AUTO_TEST_CASE( ConvertInvertedNetName_NormalSignal )
{
    // Non-inverted signals pass through unchanged
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "GND" ), wxS( "GND" ) );
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "VCC" ), wxS( "VCC" ) );
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "NET_1" ), wxS( "NET_1" ) );
}


BOOST_AUTO_TEST_CASE( ConvertInvertedNetName_SpecialCharacters )
{
    // Net names with special characters preserved (no sanitization)
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "$3.3V" ), wxS( "$3.3V" ) );
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "A+B" ), wxS( "A+B" ) );
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "NET 1" ), wxS( "NET 1" ) );
}


BOOST_AUTO_TEST_CASE( ConvertInvertedNetName_Empty )
{
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "" ), wxS( "" ) );
}


BOOST_AUTO_TEST_CASE( ConvertInvertedNetName_SlashOnly )
{
    // A bare "/" becomes an empty overbar
    BOOST_CHECK_EQUAL( PADS_COMMON::ConvertInvertedNetName( "/" ), wxS( "~{}" ) );
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( PadsFileDetection )


BOOST_AUTO_TEST_CASE( DetectPadsFileType_Unknown )
{
    auto type = PADS_COMMON::DetectPadsFileType( wxS( "/nonexistent/file.asc" ) );

    BOOST_CHECK_EQUAL( static_cast<int>( type ),
                       static_cast<int>( PADS_COMMON::PADS_FILE_TYPE::UNKNOWN ) );
}


BOOST_AUTO_TEST_CASE( FindRelatedPadsFiles_NonExistent )
{
    auto related = PADS_COMMON::FindRelatedPadsFiles( wxS( "/nonexistent/file.asc" ) );

    BOOST_CHECK( !related.HasPcb() );
    BOOST_CHECK( !related.HasSchematic() );
}


BOOST_AUTO_TEST_CASE( RELATED_FILES_HasMethods )
{
    PADS_COMMON::RELATED_FILES empty;
    BOOST_CHECK( !empty.HasPcb() );
    BOOST_CHECK( !empty.HasSchematic() );
    BOOST_CHECK( !empty.HasBoth() );

    PADS_COMMON::RELATED_FILES pcbOnly;
    pcbOnly.pcbFile = wxS( "test.asc" );
    BOOST_CHECK( pcbOnly.HasPcb() );
    BOOST_CHECK( !pcbOnly.HasSchematic() );
    BOOST_CHECK( !pcbOnly.HasBoth() );

    PADS_COMMON::RELATED_FILES schOnly;
    schOnly.schematicFile = wxS( "test.asc" );
    BOOST_CHECK( !schOnly.HasPcb() );
    BOOST_CHECK( schOnly.HasSchematic() );
    BOOST_CHECK( !schOnly.HasBoth() );

    PADS_COMMON::RELATED_FILES both;
    both.pcbFile = wxS( "test_pcb.asc" );
    both.schematicFile = wxS( "test_sch.asc" );
    BOOST_CHECK( both.HasPcb() );
    BOOST_CHECK( both.HasSchematic() );
    BOOST_CHECK( both.HasBoth() );
}


BOOST_AUTO_TEST_SUITE_END()

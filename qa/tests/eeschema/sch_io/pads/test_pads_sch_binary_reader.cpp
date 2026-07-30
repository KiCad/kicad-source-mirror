/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_io/pads/pads_sch_binary_reader.h>

#include <ki_exception.h>

namespace
{
using PADS_SCH_BINARY::PADS_SCH_BINARY_READER;

wxString fixture( const wxString& aName )
{
    return wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + wxS( "/plugins/pads/binary/" ) + aName
           + wxS( ".sch" );
}


std::vector<uint8_t> fixtureBytes( const wxString& aName )
{
    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_SCH_BINARY_READER::ReadFile( fixture( aName ), bytes ) );
    return bytes;
}
}


BOOST_AUTO_TEST_SUITE( PadsSchBinaryReader )


BOOST_AUTO_TEST_CASE( RecognizesFamilyIndependentlyOfVersionSupport )
{
    std::vector<uint8_t> bytes = fixtureBytes( wxS( "minimal_v13" ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinaryFamily( bytes ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinarySch( bytes ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsSupportedVersion( 0x000C ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsSupportedVersion( 0x000D ) );

    bytes[2] = 0x00;
    bytes[3] = 0xFE;
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinaryFamily( bytes ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinarySch( bytes ) );
    BOOST_CHECK( !PADS_SCH_BINARY_READER::IsSupportedVersion( 0xFE00 ) );

    bytes[1] = 0xFF;
    BOOST_CHECK( !PADS_SCH_BINARY_READER::IsBinaryFamily( bytes ) );
    BOOST_CHECK( !PADS_SCH_BINARY_READER::IsBinarySch( bytes ) );
    BOOST_CHECK( !PADS_SCH_BINARY_READER::IsBinaryFamily( { 0x00 } ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinaryFamily( { 0x00, 0xFE } ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinaryFamily( { 0x00, 0xFE, 0x0D } ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinaryFamily( { 0x00, 0xFE, 0x0D, 0x00 } ) );
    BOOST_CHECK( !PADS_SCH_BINARY_READER::IsBinaryFamily( std::vector<uint8_t>( 31, 0x00 ) ) );

    std::vector<uint8_t> truncatedHeader( 31, 0x00 );
    truncatedHeader[1] = 0xFE;
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinaryFamily( truncatedHeader ) );
}


BOOST_AUTO_TEST_CASE( OwnsTypedParserModel )
{
    PADS_SCH_BINARY_READER reader;
    std::vector<uint8_t>   bytes = fixtureBytes( wxS( "connectivity_topology" ) );
    BOOST_REQUIRE( reader.Parse( bytes, fixture( wxS( "connectivity_topology" ) ) ) );
    BOOST_CHECK_EQUAL( reader.GetModel().version, 0x000D );
    BOOST_CHECK( !reader.GetModel().sheets.empty() );
    BOOST_CHECK( !reader.GetModel().nets.empty() );
    BOOST_CHECK( !reader.GetModel().buses.empty() );
    BOOST_CHECK( !reader.GetModel().labels.empty() );
}


BOOST_AUTO_TEST_CASE( ParseRejectsMalformedAndReportsUnsupportedVersion )
{
    PADS_SCH_BINARY_READER reader;
    BOOST_CHECK_THROW( reader.Parse( { 0x00, 0xFE }, wxS( "truncated.sch" ) ), IO_ERROR );
    BOOST_CHECK_THROW( reader.GetModel(), IO_ERROR );

    std::vector<uint8_t> bytes = fixtureBytes( wxS( "minimal_v13" ) );
    bytes[2] = 0x00;
    bytes[3] = 0xFE;
    wxString message;

    try
    {
        reader.Parse( bytes, wxS( "unsupported.sch" ) );
        BOOST_FAIL( "unsupported binary schematic was accepted" );
    }
    catch( const IO_ERROR& error )
    {
        message = error.What();
    }

    BOOST_CHECK( message.Contains( wxS( "v0xFE00" ) ) );
    BOOST_CHECK( message.Contains( wxS( "unsupported PADS Logic binary version" ) ) );
    BOOST_CHECK_THROW( reader.GetModel(), IO_ERROR );
}


BOOST_AUTO_TEST_CASE( ParseReplacesOwnedModel )
{
    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( fixtureBytes( wxS( "minimal_v13" ) ), fixture( wxS( "minimal_v13" ) ) ) );
    const size_t firstPlacements = reader.GetModel().placements.size();
    BOOST_REQUIRE(
            reader.Parse( fixtureBytes( wxS( "placement_transform" ) ), fixture( wxS( "placement_transform" ) ) ) );
    BOOST_CHECK_NE( reader.GetModel().placements.size(), firstPlacements );
    BOOST_CHECK_EQUAL( reader.GetModel().settings.source.file, fixture( wxS( "placement_transform" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()

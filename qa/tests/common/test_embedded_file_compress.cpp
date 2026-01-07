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

#include <magic_enum.hpp>
#include <magic_enum_iostream.hpp>
#include <boost/test/unit_test.hpp>
#include <mmh3_hash.h>
#include <embedded_files.h>

#include <wx/wfstream.h>

#include <random>
using magic_enum::iostream_operators::operator<<;

BOOST_AUTO_TEST_SUITE( EmbeddedFiles )

BOOST_AUTO_TEST_CASE( CompressAndEncode_OK )
{
    EMBEDDED_FILES::EMBEDDED_FILE file;
    file.name = "test_file";
    std::string data = "Hello, World!";
    file.decompressedData.assign(data.begin(), data.end());

    MMH3_HASH hash( EMBEDDED_FILES::Seed() );
    hash.add( file.decompressedData );
    file.data_hash = hash.digest().ToString();

    EMBEDDED_FILES::RETURN_CODE result = EMBEDDED_FILES::CompressAndEncode(file);
    BOOST_CHECK_EQUAL(result, EMBEDDED_FILES::RETURN_CODE::OK);
}

BOOST_AUTO_TEST_CASE( DecompressAndDecode_OK )
{
    EMBEDDED_FILES::EMBEDDED_FILE file;
    file.name = "test_file";
    std::string data = "Hello, World!";
    file.decompressedData.assign( data.begin(), data.end() );

    MMH3_HASH hash( EMBEDDED_FILES::Seed() );
    hash.add( file.decompressedData );
    file.data_hash = hash.digest().ToString();

    EMBEDDED_FILES::RETURN_CODE result = EMBEDDED_FILES::CompressAndEncode( file );
    BOOST_CHECK_EQUAL( result, EMBEDDED_FILES::RETURN_CODE::OK );

    result = EMBEDDED_FILES::DecompressAndDecode( file );
    BOOST_CHECK_EQUAL( result, EMBEDDED_FILES::RETURN_CODE::OK );

    // Create a large test data
    data.clear();
    data.reserve( 13 * 100000 + 1 );

    for( int i = 0; i < 100000; ++i )
        data += "Hello, World!";

    file.decompressedData.assign( data.begin(), data.end() );

    hash.reset();
    hash.add( file.decompressedData );
    file.data_hash = hash.digest().ToString();

    result = EMBEDDED_FILES::CompressAndEncode( file );
    BOOST_CHECK_EQUAL( result, EMBEDDED_FILES::RETURN_CODE::OK );

    result = EMBEDDED_FILES::DecompressAndDecode( file );
    BOOST_CHECK_EQUAL( result, EMBEDDED_FILES::RETURN_CODE::OK );

    // Create a sequential test dataset
    data.clear();
    data.reserve( 100000 );

    for( int i = 0; i < 100000; ++i )
        data += static_cast<char>( i % 256 );

    file.decompressedData.assign( data.begin(), data.end() );
    hash.reset();
    hash.add( file.decompressedData );
    file.data_hash = hash.digest().ToString();

    result = EMBEDDED_FILES::CompressAndEncode( file );
    BOOST_CHECK_EQUAL( result, EMBEDDED_FILES::RETURN_CODE::OK );

    result = EMBEDDED_FILES::DecompressAndDecode( file );
    BOOST_CHECK_EQUAL( result, EMBEDDED_FILES::RETURN_CODE::OK );

    // Create a random test dataset with a known seed
    data.clear();
    data.reserve( 100000 );

    std::mt19937 rng;
    rng.seed( 0 );

    for( int i = 0; i < 100000; ++i )
        data += static_cast<char>( rng() % 256 );

    file.decompressedData.assign( data.begin(), data.end() );
    hash.reset();
    hash.add( file.decompressedData );
    file.data_hash = hash.digest().ToString();

    result = EMBEDDED_FILES::CompressAndEncode( file );
    BOOST_CHECK_EQUAL( result, EMBEDDED_FILES::RETURN_CODE::OK );

    result = EMBEDDED_FILES::DecompressAndDecode( file );
    BOOST_CHECK_EQUAL( result, EMBEDDED_FILES::RETURN_CODE::OK );

}

BOOST_AUTO_TEST_CASE( DecompressAndDecode_ChecksumError )
{
    EMBEDDED_FILES::EMBEDDED_FILE file;
    file.name = "test_file";
    std::string data = "Hello, World!";
    file.decompressedData.assign(data.begin(), data.end());

    EMBEDDED_FILES::RETURN_CODE result = EMBEDDED_FILES::CompressAndEncode(file);
    BOOST_CHECK_EQUAL(result, EMBEDDED_FILES::RETURN_CODE::OK);

    // Modify the checksum
    file.data_hash[0] = 'x';

    result = EMBEDDED_FILES::DecompressAndDecode(file);
    BOOST_CHECK_EQUAL(result, EMBEDDED_FILES::RETURN_CODE::CHECKSUM_ERROR);
}


BOOST_AUTO_TEST_CASE( ComputeFileHash_MatchesEmbeddedHash )
{
    // Create a temp file with known content
    wxFileName tempFile = wxFileName::CreateTempFileName( wxS( "kicad_embed_test" ) );
    std::string data = "Test file content for hash computation";

    {
        wxFFileOutputStream out( tempFile.GetFullPath() );
        BOOST_REQUIRE( out.IsOk() );
        out.Write( data.data(), data.size() );
    }

    // Compute hash via ComputeFileHash
    std::string computedHash;
    EMBEDDED_FILES::RETURN_CODE result = EMBEDDED_FILES::ComputeFileHash( tempFile, computedHash );
    BOOST_CHECK_EQUAL( result, EMBEDDED_FILES::RETURN_CODE::OK );
    BOOST_CHECK( !computedHash.empty() );

    // Embed the same file and verify hashes match
    EMBEDDED_FILES files;
    EMBEDDED_FILES::EMBEDDED_FILE* embedded = files.AddFile( tempFile, false );
    BOOST_REQUIRE( embedded != nullptr );
    BOOST_CHECK_EQUAL( computedHash, embedded->data_hash );

    // Clean up
    wxRemoveFile( tempFile.GetFullPath() );
}


BOOST_AUTO_TEST_CASE( ComputeFileHash_DifferentContent )
{
    // Create two temp files with different content
    wxFileName tempFile1 = wxFileName::CreateTempFileName( wxS( "kicad_embed_test1" ) );
    wxFileName tempFile2 = wxFileName::CreateTempFileName( wxS( "kicad_embed_test2" ) );
    std::string data1 = "Content version 1";
    std::string data2 = "Content version 2";

    {
        wxFFileOutputStream out1( tempFile1.GetFullPath() );
        BOOST_REQUIRE( out1.IsOk() );
        out1.Write( data1.data(), data1.size() );
    }

    {
        wxFFileOutputStream out2( tempFile2.GetFullPath() );
        BOOST_REQUIRE( out2.IsOk() );
        out2.Write( data2.data(), data2.size() );
    }

    // Compute hashes for both files
    std::string hash1, hash2;
    BOOST_CHECK_EQUAL( EMBEDDED_FILES::ComputeFileHash( tempFile1, hash1 ),
                       EMBEDDED_FILES::RETURN_CODE::OK );
    BOOST_CHECK_EQUAL( EMBEDDED_FILES::ComputeFileHash( tempFile2, hash2 ),
                       EMBEDDED_FILES::RETURN_CODE::OK );

    // Hashes should be different
    BOOST_CHECK_NE( hash1, hash2 );

    // Clean up
    wxRemoveFile( tempFile1.GetFullPath() );
    wxRemoveFile( tempFile2.GetFullPath() );
}


BOOST_AUTO_TEST_CASE( ComputeFileHash_NonExistentFile )
{
    wxFileName nonExistent( wxS( "/nonexistent/path/file.txt" ) );
    std::string hash;

    EMBEDDED_FILES::RETURN_CODE result = EMBEDDED_FILES::ComputeFileHash( nonExistent, hash );
    BOOST_CHECK_EQUAL( result, EMBEDDED_FILES::RETURN_CODE::FILE_NOT_FOUND );
    BOOST_CHECK( hash.empty() );
}


BOOST_AUTO_TEST_SUITE_END()

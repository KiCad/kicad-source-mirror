/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <boost/test/unit_test.hpp>
#include <picosha2.h>
#include <embedded_files.h>

#include <random>
using magic_enum::iostream_operators::operator<<;

BOOST_AUTO_TEST_SUITE( EmbeddedFiles )

BOOST_AUTO_TEST_CASE( CompressAndEncode_OK )
{
    EMBEDDED_FILES::EMBEDDED_FILE file;
    file.name = "test_file";
    std::string data = "Hello, World!";
    file.decompressedData.assign(data.begin(), data.end());

    picosha2::hash256_hex_string(file.decompressedData, file.data_sha);

    EMBEDDED_FILES::RETURN_CODE result = EMBEDDED_FILES::CompressAndEncode(file);
    BOOST_CHECK_EQUAL(result, EMBEDDED_FILES::RETURN_CODE::OK);
}

BOOST_AUTO_TEST_CASE( DecompressAndDecode_OK )
{
    EMBEDDED_FILES::EMBEDDED_FILE file;
    file.name = "test_file";
    std::string data = "Hello, World!";
    file.decompressedData.assign( data.begin(), data.end() );

    picosha2::hash256_hex_string( file.decompressedData, file.data_sha );

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

    picosha2::hash256_hex_string( file.decompressedData, file.data_sha );

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
    picosha2::hash256_hex_string( file.decompressedData, file.data_sha );

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
    picosha2::hash256_hex_string( file.decompressedData, file.data_sha );

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
    file.data_sha[0] = 'x';

    result = EMBEDDED_FILES::DecompressAndDecode(file);
    BOOST_CHECK_EQUAL(result, EMBEDDED_FILES::RETURN_CODE::CHECKSUM_ERROR);
}

BOOST_AUTO_TEST_SUITE_END()

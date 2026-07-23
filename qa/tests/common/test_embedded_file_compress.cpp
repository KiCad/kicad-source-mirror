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


BOOST_AUTO_TEST_CASE( DecompressAndDecode_V1HashFallback )
{
    // Verify that files hashed with the old (V1) MMH3 algorithm can still be
    // decoded. The V1 algorithm had a tail-byte padding bug that inflated len
    // and changed the hash for data sizes not a multiple of 16.
    auto testV1Fallback = []( const std::string& aData )
    {
        EMBEDDED_FILES::EMBEDDED_FILE file;
        file.name = "v1_hash_test";
        file.decompressedData.assign( aData.begin(), aData.end() );

        // Hash with V1 algorithm
        MMH3_HASH v1hash( EMBEDDED_FILES::Seed() );
        v1hash.addDataV1( reinterpret_cast<const uint8_t*>( aData.data() ), aData.size() );
        file.data_hash = v1hash.digest().ToString();

        // Verify V1 and current hashes differ for non-aligned data
        MMH3_HASH curHash( EMBEDDED_FILES::Seed() );
        curHash.add( file.decompressedData );
        std::string currentHash = curHash.digest().ToString();

        if( aData.size() % 16 != 0 )
            BOOST_CHECK_NE( file.data_hash, currentHash );

        EMBEDDED_FILES::RETURN_CODE rc = EMBEDDED_FILES::CompressAndEncode( file );
        BOOST_CHECK_EQUAL( rc, EMBEDDED_FILES::RETURN_CODE::OK );

        // DecompressAndDecode should succeed via V1 fallback
        rc = EMBEDDED_FILES::DecompressAndDecode( file );
        BOOST_CHECK_EQUAL( rc, EMBEDDED_FILES::RETURN_CODE::OK );

        // After successful decode, hash should be migrated to current format
        BOOST_CHECK_EQUAL( file.data_hash, currentHash );
    };

    // 13 bytes (remaining=13, old padding rounds to 16, hashTail sees 0 tail bytes)
    testV1Fallback( "Hello, World!" );

    // 17 bytes (remaining=1, old padding rounds to 4)
    testV1Fallback( "12345678901234567" );

    // 31 bytes (remaining=15, old padding rounds to 16)
    testV1Fallback( "1234567890123456789012345678901" );

    // 100000 bytes (remaining=16 mod 16 = 0, no tail, hashes should match)
    std::string aligned( 100000UL * 16, 'x' );
    EMBEDDED_FILES::EMBEDDED_FILE alignedFile;
    alignedFile.name = "aligned_test";
    alignedFile.decompressedData.assign( aligned.begin(), aligned.end() );

    MMH3_HASH v1h( EMBEDDED_FILES::Seed() );
    v1h.addDataV1( reinterpret_cast<const uint8_t*>( aligned.data() ), aligned.size() );
    std::string v1Hash = v1h.digest().ToString();

    MMH3_HASH curH( EMBEDDED_FILES::Seed() );
    curH.add( alignedFile.decompressedData );
    std::string curHash = curH.digest().ToString();

    BOOST_CHECK_EQUAL( v1Hash, curHash );
}


// Regression test for GitLab issue #24345.  Copying an EMBEDDED_FILES collection must not
// duplicate the underlying file payloads; multiple copies should share ownership through
// shared_ptr so that cloning a footprint with embedded 3D models for an undo snapshot does
// not multiply memory usage by the number of clones.
BOOST_AUTO_TEST_CASE( CopySharesEmbeddedFilePayloads )
{
    EMBEDDED_FILES original;

    auto* file = new EMBEDDED_FILES::EMBEDDED_FILE();
    file->name = wxS( "shared_model.step" );
    file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::MODEL;

    std::string payload( 1024 * 1024, 'x' );
    file->decompressedData.assign( payload.begin(), payload.end() );

    MMH3_HASH hash( EMBEDDED_FILES::Seed() );
    hash.add( file->decompressedData );
    file->data_hash = hash.digest().ToString();

    BOOST_REQUIRE_EQUAL( EMBEDDED_FILES::CompressAndEncode( *file ),
                         EMBEDDED_FILES::RETURN_CODE::OK );

    original.AddFile( file );

    EMBEDDED_FILES::EMBEDDED_FILE* originalFile = original.GetEmbeddedFile( wxS( "shared_model.step" ) );
    BOOST_REQUIRE( originalFile );

    // Copy via copy constructor; sharing means both pointers reference the same payload.
    EMBEDDED_FILES copy1( original );
    EMBEDDED_FILES::EMBEDDED_FILE* copy1File = copy1.GetEmbeddedFile( wxS( "shared_model.step" ) );
    BOOST_REQUIRE( copy1File );
    BOOST_CHECK_EQUAL( copy1File, originalFile );

    // Copy via assignment operator deep-copies (assignment targets a live object that may
    // later mutate file fields through raw pointers; aliasing would silently bleed mutations
    // back into the source).
    EMBEDDED_FILES copy2;
    copy2 = original;
    EMBEDDED_FILES::EMBEDDED_FILE* copy2File = copy2.GetEmbeddedFile( wxS( "shared_model.step" ) );
    BOOST_REQUIRE( copy2File );
    BOOST_CHECK_NE( copy2File, originalFile );
    BOOST_CHECK_EQUAL( copy2File->data_hash, originalFile->data_hash );

    // Destroying the original must keep the payload alive via the surviving copies.
    {
        EMBEDDED_FILES transient( original );
        transient.ClearEmbeddedFiles();
        BOOST_CHECK( !transient.HasFile( wxS( "shared_model.step" ) ) );
    }

    BOOST_CHECK( original.HasFile( wxS( "shared_model.step" ) ) );
    BOOST_CHECK( copy1.HasFile( wxS( "shared_model.step" ) ) );
    BOOST_CHECK( copy2.HasFile( wxS( "shared_model.step" ) ) );

    // Validate the payload via one of the shared copies (round-trip).
    BOOST_CHECK_EQUAL( EMBEDDED_FILES::DecompressAndDecode( *copy1File ),
                       EMBEDDED_FILES::RETURN_CODE::OK );
    BOOST_CHECK_EQUAL( std::string( copy1File->decompressedData.begin(),
                                    copy1File->decompressedData.end() ),
                       payload );
}


// Explicit deep-copy form must allocate independent EMBEDDED_FILE objects.
BOOST_AUTO_TEST_CASE( DeepCopyAllocatesIndependentPayloads )
{
    EMBEDDED_FILES original;

    auto* file = new EMBEDDED_FILES::EMBEDDED_FILE();
    file->name = wxS( "deep_copy.bin" );
    file->decompressedData.assign( { '1', '2', '3' } );

    MMH3_HASH hash( EMBEDDED_FILES::Seed() );
    hash.add( file->decompressedData );
    file->data_hash = hash.digest().ToString();

    BOOST_REQUIRE_EQUAL( EMBEDDED_FILES::CompressAndEncode( *file ),
                         EMBEDDED_FILES::RETURN_CODE::OK );

    original.AddFile( file );

    EMBEDDED_FILES deep( original, /*aDeepCopy=*/true );
    EMBEDDED_FILES::EMBEDDED_FILE* deepFile = deep.GetEmbeddedFile( wxS( "deep_copy.bin" ) );

    BOOST_REQUIRE( deepFile );
    BOOST_CHECK_NE( deepFile, file );
    BOOST_CHECK_EQUAL( deepFile->data_hash, file->data_hash );
}


// A paged setup dialog commits its working copy on both page change and OK, so the commit must
// leave the source intact.  A destructive commit wiped the board/schematic on the second pass and
// silently dropped the embedded drawing sheet (issue 24998).
BOOST_AUTO_TEST_CASE( AssignSharedFromIsIdempotent )
{
    EMBEDDED_FILES working;

    auto* file = new EMBEDDED_FILES::EMBEDDED_FILE();
    file->name = wxS( "ARTIDIS-KiCAD_HEADER.kicad_wks" );
    file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::WORKSHEET;
    file->decompressedData.assign( { '1', '2', '3' } );

    MMH3_HASH hash( EMBEDDED_FILES::Seed() );
    hash.add( file->decompressedData );
    file->data_hash = hash.digest().ToString();

    BOOST_REQUIRE_EQUAL( EMBEDDED_FILES::CompressAndEncode( *file ),
                         EMBEDDED_FILES::RETURN_CODE::OK );

    working.AddFile( file );

    EMBEDDED_FILES target;

    target.AssignSharedFrom( working );
    target.AssignSharedFrom( working );

    // IsEmpty() gates whether the embedded_files block is written on save.
    BOOST_CHECK( !target.IsEmpty() );
    BOOST_CHECK( target.HasFile( wxS( "ARTIDIS-KiCAD_HEADER.kicad_wks" ) ) );
    BOOST_CHECK( working.HasFile( wxS( "ARTIDIS-KiCAD_HEADER.kicad_wks" ) ) );

    // Self-assignment must not empty the collection.
    working.AssignSharedFrom( working );
    BOOST_CHECK( working.HasFile( wxS( "ARTIDIS-KiCAD_HEADER.kicad_wks" ) ) );

    std::set<wxString> exclude{ wxS( "ARTIDIS-KiCAD_HEADER.kicad_wks" ) };
    working.AssignSharedFrom( working, exclude );
    BOOST_CHECK( !working.HasFile( wxS( "ARTIDIS-KiCAD_HEADER.kicad_wks" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()

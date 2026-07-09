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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include <json_common.h>
#include <wx/filename.h>

#include <exporters/step/glb_utils.h>


static constexpr uint32_t GLB_MAGIC = 0x46546C67;
static constexpr uint32_t GLB_CHUNK_JSON = 0x4E4F534A;
static constexpr int      GLTF_LINES_MODE = 1;
static constexpr int      GLTF_TRIANGLES_MODE = 4;


struct GLB_VALIDATION
{
    int  linesCount = 0;
    int  oddLinesCount = 0;
    int  trianglesCount = 0;
    bool valid = false;
};


static GLB_VALIDATION ValidateGlbFile( const wxString& aPath )
{
    GLB_VALIDATION result;

    std::ifstream inFile( aPath.ToStdString(), std::ios::binary );

    if( !inFile )
        return result;

    std::vector<uint8_t> data( ( std::istreambuf_iterator<char>( inFile ) ),
                               std::istreambuf_iterator<char>() );
    inFile.close();

    if( data.size() < 20 )
        return result;

    uint32_t magic = 0;
    uint32_t jsonLen = 0;
    uint32_t jsonType = 0;
    std::memcpy( &magic, &data[0], 4 );
    std::memcpy( &jsonLen, &data[12], 4 );
    std::memcpy( &jsonType, &data[16], 4 );

    if( magic != GLB_MAGIC || jsonType != GLB_CHUNK_JSON )
        return result;

    if( 20 + jsonLen > data.size() )
        return result;

    std::string jsonStr( data.begin() + 20, data.begin() + 20 + jsonLen );

    nlohmann::json gltf;

    try
    {
        gltf = nlohmann::json::parse( jsonStr );
    }
    catch( const nlohmann::json::parse_error& )
    {
        return result;
    }

    result.valid = true;

    if( !gltf.contains( "meshes" ) || !gltf.contains( "accessors" ) )
        return result;

    auto& accessors = gltf["accessors"];

    for( auto& mesh : gltf["meshes"] )
    {
        if( !mesh.contains( "primitives" ) )
            continue;

        for( auto& prim : mesh["primitives"] )
        {
            int mode = prim.value( "mode", GLTF_TRIANGLES_MODE );

            if( mode == GLTF_TRIANGLES_MODE )
            {
                result.trianglesCount++;
            }
            else if( mode == GLTF_LINES_MODE )
            {
                result.linesCount++;

                if( prim.contains( "indices" ) )
                {
                    int idx = prim["indices"].get<int>();

                    if( idx >= 0 && idx < static_cast<int>( accessors.size() ) )
                    {
                        int count = accessors[idx].value( "count", 0 );

                        if( count % 2 != 0 )
                            result.oddLinesCount++;
                    }
                }
            }
        }
    }

    return result;
}


static wxString GetTestGlbPath( const wxString& aFilename )
{
    return wxString( KI_TEST::GetTestDataRootDir() ) + wxS( "common/issue21706/" ) + aFilename;
}


// Serialize a glTF document into a minimal GLB (header + JSON chunk + empty BIN chunk) and
// write it to a unique temp file. Lets the edge-case tests exercise paths the captured
// fixtures cannot reach, such as a shared indices accessor or a single-index LINES primitive.
static wxString WriteTempGlb( const nlohmann::json& aGltf, const wxString& aPrefix )
{
    std::string json = aGltf.dump();

    while( json.size() % 4 != 0 )
        json.push_back( ' ' );

    uint32_t jsonLen = static_cast<uint32_t>( json.size() );
    uint32_t version = 2;
    uint32_t binLen = 0;
    uint32_t binType = 0x004E4942;  // "BIN\0" in little-endian
    uint32_t total = 12 + 8 + jsonLen + 8 + binLen;

    std::vector<uint8_t> data;

    auto append32 = [&]( uint32_t v )
    {
        uint8_t bytes[4];
        std::memcpy( bytes, &v, 4 );
        data.insert( data.end(), bytes, bytes + 4 );
    };

    append32( GLB_MAGIC );
    append32( version );
    append32( total );
    append32( jsonLen );
    append32( GLB_CHUNK_JSON );
    data.insert( data.end(), json.begin(), json.end() );
    append32( binLen );
    append32( binType );

    wxString path( wxFileName::CreateTempFileName( aPrefix ) );
    std::ofstream out( path.ToStdString(), std::ios::binary | std::ios::trunc );
    out.write( reinterpret_cast<const char*>( data.data() ), data.size() );
    out.close();

    return path;
}


BOOST_AUTO_TEST_SUITE( GlbUtils )


BOOST_AUTO_TEST_CASE( BadGlbHasOddLinesCount )
{
    wxString badPath = GetTestGlbPath( wxS( "issue21706 (BAD 9.0.4).glb" ) );

    BOOST_REQUIRE( wxFileExists( badPath ) );

    GLB_VALIDATION v = ValidateGlbFile( badPath );
    BOOST_REQUIRE( v.valid );
    BOOST_CHECK_EQUAL( v.linesCount, 1 );
    BOOST_CHECK_EQUAL( v.oddLinesCount, 1 );
    BOOST_CHECK_GT( v.trianglesCount, 0 );
}


BOOST_AUTO_TEST_CASE( GoodGlbHasNoOddLinesCount )
{
    wxString goodPath = GetTestGlbPath( wxS( "issue21706 (GOOD 9.0.1).glb" ) );

    BOOST_REQUIRE( wxFileExists( goodPath ) );

    GLB_VALIDATION v = ValidateGlbFile( goodPath );
    BOOST_REQUIRE( v.valid );
    BOOST_CHECK_EQUAL( v.linesCount, 0 );
    BOOST_CHECK_EQUAL( v.oddLinesCount, 0 );
    BOOST_CHECK_GT( v.trianglesCount, 0 );
}


BOOST_AUTO_TEST_CASE( FixGlbCorrectsBadFile )
{
    wxString badPath = GetTestGlbPath( wxS( "issue21706 (BAD 9.0.4).glb" ) );

    BOOST_REQUIRE( wxFileExists( badPath ) );

    // Copy to a unique temp file to avoid modifying the test data
    std::filesystem::path tempFile( wxFileName::CreateTempFileName( wxS( "test_issue21706_fix" ) ).ToStdString() );
    std::filesystem::copy_file( badPath.ToStdString(), tempFile,
                                std::filesystem::copy_options::overwrite_existing );

    wxString tempPath( tempFile.string() );

    // Verify the copy has the problem
    GLB_VALIDATION before = ValidateGlbFile( tempPath );
    BOOST_REQUIRE( before.valid );
    BOOST_CHECK_EQUAL( before.oddLinesCount, 1 );

    // Apply the fix
    BOOST_CHECK( FixGlbLinesPrimitives( tempPath ) );

    // Verify the fix was applied
    GLB_VALIDATION after = ValidateGlbFile( tempPath );
    BOOST_REQUIRE( after.valid );
    BOOST_CHECK_EQUAL( after.oddLinesCount, 0 );
    BOOST_CHECK_EQUAL( after.linesCount, 1 );
    BOOST_CHECK_EQUAL( after.trianglesCount, before.trianglesCount );

    std::filesystem::remove( tempFile );
}


BOOST_AUTO_TEST_CASE( FixGlbDoesNotModifyGoodFile )
{
    wxString goodPath = GetTestGlbPath( wxS( "issue21706 (GOOD 9.0.1).glb" ) );

    BOOST_REQUIRE( wxFileExists( goodPath ) );

    std::filesystem::path tempFile( wxFileName::CreateTempFileName( wxS( "test_issue21706_good" ) ).ToStdString() );
    std::filesystem::copy_file( goodPath.ToStdString(), tempFile,
                                std::filesystem::copy_options::overwrite_existing );

    wxString tempPath( tempFile.string() );

    // Get original file size
    auto origSize = std::filesystem::file_size( tempFile );

    // Apply the fix (should be a no-op)
    BOOST_CHECK( FixGlbLinesPrimitives( tempPath ) );

    // Verify file size is unchanged (no modification occurred since there were no LINES)
    auto newSize = std::filesystem::file_size( tempFile );
    BOOST_CHECK_EQUAL( origSize, newSize );

    // Verify still valid
    GLB_VALIDATION after = ValidateGlbFile( tempPath );
    BOOST_REQUIRE( after.valid );
    BOOST_CHECK_EQUAL( after.oddLinesCount, 0 );

    std::filesystem::remove( tempFile );
}


BOOST_AUTO_TEST_CASE( FixGlbClonesSharedAccessor )
{
    // Two LINES primitives reference the same odd-count indices accessor. The fix must shorten
    // one independently of the other instead of corrupting the shared accessor in place.
    nlohmann::json gltf;
    gltf["accessors"] = nlohmann::json::array(
            { { { "count", 5 }, { "componentType", 5123 }, { "type", "SCALAR" } } } );
    gltf["meshes"] = nlohmann::json::array(
            { { { "primitives",
                  { { { "mode", GLTF_LINES_MODE }, { "indices", 0 } },
                    { { "mode", GLTF_LINES_MODE }, { "indices", 0 } } } } } } );

    wxString path = WriteTempGlb( gltf, wxS( "test_glb_shared" ) );

    BOOST_CHECK( FixGlbLinesPrimitives( path ) );

    GLB_VALIDATION after = ValidateGlbFile( path );
    BOOST_REQUIRE( after.valid );
    BOOST_CHECK_EQUAL( after.linesCount, 2 );
    BOOST_CHECK_EQUAL( after.oddLinesCount, 0 );

    std::filesystem::remove( path.ToStdString() );
}


BOOST_AUTO_TEST_CASE( FixGlbDropsSingleIndexLine )
{
    // A LINES primitive with a single index forms no segment and is unfixable by trimming, so
    // it must be dropped. A sibling triangles primitive keeps the mesh non-empty as the spec
    // requires.
    nlohmann::json gltf;
    gltf["accessors"] = nlohmann::json::array(
            { { { "count", 1 }, { "componentType", 5123 }, { "type", "SCALAR" } },
              { { "count", 3 }, { "componentType", 5123 }, { "type", "SCALAR" } } } );
    gltf["meshes"] = nlohmann::json::array(
            { { { "primitives",
                  { { { "mode", GLTF_LINES_MODE }, { "indices", 0 } },
                    { { "mode", GLTF_TRIANGLES_MODE }, { "indices", 1 } } } } } } );

    wxString path = WriteTempGlb( gltf, wxS( "test_glb_single" ) );

    BOOST_CHECK( FixGlbLinesPrimitives( path ) );

    GLB_VALIDATION after = ValidateGlbFile( path );
    BOOST_REQUIRE( after.valid );
    BOOST_CHECK_EQUAL( after.linesCount, 0 );
    BOOST_CHECK_EQUAL( after.oddLinesCount, 0 );
    BOOST_CHECK_EQUAL( after.trianglesCount, 1 );

    std::filesystem::remove( path.ToStdString() );
}


BOOST_AUTO_TEST_CASE( FixGlbBailsOnLoneSingleIndexLine )
{
    // Dropping the only primitive would leave an empty, spec-invalid mesh. The fix must refuse
    // and report failure rather than emit such a file.
    nlohmann::json gltf;
    gltf["accessors"] = nlohmann::json::array(
            { { { "count", 1 }, { "componentType", 5123 }, { "type", "SCALAR" } } } );
    gltf["meshes"] = nlohmann::json::array(
            { { { "primitives",
                  { { { "mode", GLTF_LINES_MODE }, { "indices", 0 } } } } } } );

    wxString path = WriteTempGlb( gltf, wxS( "test_glb_lone" ) );

    BOOST_CHECK( !FixGlbLinesPrimitives( path ) );

    std::filesystem::remove( path.ToStdString() );
}


BOOST_AUTO_TEST_CASE( FixGlbRejectsTruncatedFile )
{
    // A declared length exceeding the actual file marks a truncated GLB; the fix must refuse it
    // instead of reading past the buffer or rewriting garbage.
    nlohmann::json gltf;
    gltf["accessors"] = nlohmann::json::array(
            { { { "count", 3 }, { "componentType", 5123 }, { "type", "SCALAR" } } } );
    gltf["meshes"] = nlohmann::json::array(
            { { { "primitives",
                  { { { "mode", GLTF_LINES_MODE }, { "indices", 0 } } } } } } );

    wxString path = WriteTempGlb( gltf, wxS( "test_glb_trunc" ) );

    std::vector<uint8_t> data;
    {
        std::ifstream in( path.ToStdString(), std::ios::binary );
        data.assign( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    }

    uint32_t bogusTotal = static_cast<uint32_t>( data.size() ) + 1024;
    std::memcpy( &data[8], &bogusTotal, 4 );

    {
        std::ofstream out( path.ToStdString(), std::ios::binary | std::ios::trunc );
        out.write( reinterpret_cast<const char*>( data.data() ), data.size() );
    }

    BOOST_CHECK( !FixGlbLinesPrimitives( path ) );

    std::filesystem::remove( path.ToStdString() );
}


BOOST_AUTO_TEST_SUITE_END()

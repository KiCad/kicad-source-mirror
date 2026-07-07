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

#include "glb_utils.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <vector>

#include <nlohmann/json.hpp>

#include <wx/filename.h>


static constexpr uint32_t GLB_MAGIC = 0x46546C67;      // "glTF" in little-endian
static constexpr uint32_t GLB_CHUNK_JSON = 0x4E4F534A;  // "JSON" in little-endian
static constexpr int      GLTF_LINES_MODE = 1;


// Writes to a sibling temp file and renames it over aFilePath, so a crash or failed write can
// never leave a partially-written (and therefore corrupt) GLB behind.
static bool atomicWriteFile( const wxString& aFilePath, const uint8_t* aData, size_t aSize )
{
    wxFileName tmpFn( aFilePath );
    tmpFn.SetName( tmpFn.GetName() + wxT( ".tmp" ) );
    wxString tmpPath = tmpFn.GetFullPath();

    {
        std::ofstream tmpFile( tmpPath.ToStdString(), std::ios::binary | std::ios::trunc );

        if( !tmpFile.is_open() )
            return false;

        tmpFile.write( reinterpret_cast<const char*>( aData ), static_cast<std::streamsize>( aSize ) );

        if( !tmpFile.good() )
            return false;
    }

    return wxRenameFile( tmpPath, aFilePath, true );
}


bool FixGlbLinesPrimitives( const wxString& aFilePath )
{
    std::ifstream inFile( aFilePath.ToStdString(), std::ios::binary );

    if( !inFile )
        return false;

    std::vector<uint8_t> fileData( ( std::istreambuf_iterator<char>( inFile ) ),
                                   std::istreambuf_iterator<char>() );
    inFile.close();

    if( fileData.size() < 20 )
        return false;

    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t totalLength = 0;
    std::memcpy( &magic, &fileData[0], 4 );
    std::memcpy( &version, &fileData[4], 4 );
    std::memcpy( &totalLength, &fileData[8], 4 );

    // A declared length larger than the file means the GLB is truncated. Bounding the later
    // chunk copy by totalLength then keeps any trailing junk out of the rewritten file.
    if( magic != GLB_MAGIC || version != 2 || totalLength > fileData.size() )
        return false;

    uint32_t jsonChunkLength = 0;
    uint32_t jsonChunkType = 0;
    std::memcpy( &jsonChunkLength, &fileData[12], 4 );
    std::memcpy( &jsonChunkType, &fileData[16], 4 );

    // The JSON chunk is 4-byte aligned by spec; a misaligned length signals a malformed GLB.
    if( jsonChunkType != GLB_CHUNK_JSON || jsonChunkLength % 4 != 0 )
        return false;

    size_t jsonStart = 20;
    size_t jsonEnd = jsonStart + jsonChunkLength;

    if( jsonEnd > totalLength )
        return false;

    std::string jsonStr( fileData.begin() + jsonStart, fileData.begin() + jsonEnd );

    nlohmann::json gltf;

    try
    {
        gltf = nlohmann::json::parse( jsonStr );
    }
    catch( const nlohmann::json::parse_error& )
    {
        return false;
    }

    if( !gltf.contains( "meshes" ) || !gltf.contains( "accessors" ) )
        return true;

    bool modified = false;

    try
    {
        if( !gltf["meshes"].is_array() || !gltf["accessors"].is_array() )
            return true;

        auto& accessors = gltf["accessors"];

        // An indices accessor may be shared by several primitives. Reducing its count in
        // place would corrupt the others, so count references first and clone-on-write.
        std::map<size_t, int> accessorRefCount;

        for( auto& mesh : gltf["meshes"] )
        {
            if( !mesh.is_object() || !mesh.contains( "primitives" )
                || !mesh["primitives"].is_array() )
                continue;

            for( auto& prim : mesh["primitives"] )
            {
                if( prim.is_object() && prim.contains( "indices" )
                    && prim["indices"].is_number_unsigned() )
                    accessorRefCount[prim["indices"].get<size_t>()]++;
            }
        }

        for( auto& mesh : gltf["meshes"] )
        {
            if( !mesh.is_object() || !mesh.contains( "primitives" )
                || !mesh["primitives"].is_array() )
                continue;

            auto& primitives = mesh["primitives"];

            for( auto primIt = primitives.begin(); primIt != primitives.end(); )
            {
                auto& prim = *primIt;

                if( !prim.is_object() )
                {
                    ++primIt;
                    continue;
                }

                int mode = prim.value( "mode", 4 );

                if( mode != GLTF_LINES_MODE || !prim.contains( "indices" )
                    || !prim["indices"].is_number_unsigned() )
                {
                    ++primIt;
                    continue;
                }

                size_t accessorIdx = prim["indices"].get<size_t>();

                if( accessorIdx >= accessors.size() || !accessors[accessorIdx].is_object()
                    || !accessors[accessorIdx]["count"].is_number_unsigned() )
                {
                    ++primIt;
                    continue;
                }

                size_t count = accessors[accessorIdx]["count"].get<size_t>();

                if( count % 2 == 0 )
                {
                    ++primIt;
                    continue;
                }

                // A single dangling index forms no segment at all, so the primitive is
                // useless; the spec also forbids a zero count. Drop the whole primitive, but
                // only while the mesh keeps another, since the spec requires a non-empty
                // primitives array. Bail out untouched in the (OCCT-never-seen) lone case
                // rather than emit an equally invalid empty mesh.
                if( count < 2 )
                {
                    if( primitives.size() <= 1 )
                        return false;

                    primIt = primitives.erase( primIt );
                    modified = true;
                    continue;
                }

                // Dropping the dangling index leaves a valid even count. When the accessor
                // is shared, clone it so only this primitive's line list is shortened.
                if( accessorRefCount[accessorIdx] > 1 )
                {
                    accessorRefCount[accessorIdx]--;
                    nlohmann::json clone = accessors[accessorIdx];
                    clone["count"] = count - 1;
                    accessors.push_back( std::move( clone ) );
                    prim["indices"] = accessors.size() - 1;
                }
                else
                {
                    accessors[accessorIdx]["count"] = count - 1;
                }

                modified = true;
                ++primIt;
            }
        }
    }
    catch( const nlohmann::json::exception& )
    {
        return false;
    }

    if( !modified )
        return true;

    std::string fixedJson = gltf.dump();

    // GLB spec requires JSON chunk to be padded to 4-byte alignment with spaces
    while( fixedJson.size() % 4 != 0 )
        fixedJson.push_back( ' ' );

    size_t newJsonChunkLength = fixedJson.size();

    // Reconstruct the GLB as header + JSON chunk header + JSON + the remaining chunks. The
    // BIN chunk and any trailing chunks are copied verbatim, bounded by the declared length.
    size_t binChunkStart = jsonEnd;
    size_t remainingSize = totalLength - binChunkStart;
    size_t newTotalLength = 12 + 8 + newJsonChunkLength + remainingSize;

    if( newTotalLength > UINT32_MAX )
        return false;

    std::vector<uint8_t> outData;
    outData.reserve( newTotalLength );

    uint32_t newJsonChunkLength32 = static_cast<uint32_t>( newJsonChunkLength );
    uint32_t newTotalLength32 = static_cast<uint32_t>( newTotalLength );

    // GLB header
    outData.resize( 12 );
    std::memcpy( &outData[0], &magic, 4 );
    std::memcpy( &outData[4], &version, 4 );
    std::memcpy( &outData[8], &newTotalLength32, 4 );

    // JSON chunk header
    size_t pos = outData.size();
    outData.resize( pos + 8 );
    std::memcpy( &outData[pos], &newJsonChunkLength32, 4 );
    std::memcpy( &outData[pos + 4], &jsonChunkType, 4 );

    // JSON data
    outData.insert( outData.end(), fixedJson.begin(), fixedJson.end() );

    // Remaining chunks (BIN chunk, etc.)
    if( remainingSize > 0 )
        outData.insert( outData.end(), fileData.begin() + binChunkStart,
                        fileData.begin() + totalLength );

    // Replace the file atomically so a partial write or crash can never leave a corrupt GLB.
    return atomicWriteFile( aFilePath, outData.data(), outData.size() );
}

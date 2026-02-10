/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pads_binary_parser.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <set>

#include <ki_exception.h>
#include <wx/log.h>

namespace PADS_IO
{

// Expected footer GUID
static const uint8_t FOOTER_GUID[] = "{2FE18320-6448-11d1-A412-000000000000}";

// Pad shape codes from binary format
static const std::map<uint8_t, std::string> PAD_SHAPE_NAMES = {
    { 0x01, "RF" },
    { 0x02, "R" },
    { 0x03, "S" },
    { 0x04, "OF" },
};


BINARY_PARSER::BINARY_PARSER() = default;


BINARY_PARSER::~BINARY_PARSER() = default;


bool BINARY_PARSER::IsBinaryPadsFile( const wxString& aFileName )
{
    std::ifstream file( aFileName.fn_str(), std::ios::binary );

    if( !file.is_open() )
        return false;

    uint8_t header[4];
    file.read( reinterpret_cast<char*>( header ), 4 );

    if( file.gcount() < 4 )
        return false;

    // Magic bytes: 0x00 0xFF
    if( header[0] != 0x00 || header[1] != 0xFF )
        return false;

    uint16_t version = static_cast<uint16_t>( header[2] ) | ( static_cast<uint16_t>( header[3] ) << 8 );

    return version == 0x2021 || version == 0x2025 || version == 0x2026 || version == 0x2027;
}


void BINARY_PARSER::Parse( const wxString& aFileName )
{
    std::ifstream file( aFileName.fn_str(), std::ios::binary | std::ios::ate );

    if( !file.is_open() )
        THROW_IO_ERROR( "Cannot open file" );

    std::streamsize fileSize = file.tellg();
    file.seekg( 0, std::ios::beg );

    m_data.resize( static_cast<size_t>( fileSize ) );
    file.read( reinterpret_cast<char*>( m_data.data() ), fileSize );

    if( file.gcount() != fileSize )
        THROW_IO_ERROR( "Failed to read entire file" );

    parseHeader();
    parseFooter();
    parseDirectory();
    parseBoardSetup();
    parseStringPool();
    parseMetadataRegion();
    parsePartPlacements();
    parseSection19Parts();
    parsePadStacks();
    parsePartDecals();
    parseFootprintDefs();
    parseLineVertices();
    parseBoardOutline();
    parseNetNames();
    parseTextRecords();
    parseRouteVertices();
    parseCopperPours();

    // Filter out parts with empty ref des
    m_parts.erase( std::remove_if( m_parts.begin(), m_parts.end(),
                                   []( const PART& p ) { return p.name.empty(); } ),
                   m_parts.end() );
}


int BINARY_PARSER::dirEntryCount() const
{
    switch( m_version )
    {
    case 0x2021: return 73;
    case 0x2025:
    case 0x2026:
    case 0x2027: return 74;
    default:     return 0;
    }
}


uint8_t BINARY_PARSER::readU8( size_t aOffset ) const
{
    if( aOffset >= m_data.size() )
    {
        THROW_IO_ERROR( wxString::Format( "PADS binary read out of bounds at offset %zu (file size %zu)",
                                          aOffset, m_data.size() ) );
    }

    return m_data[aOffset];
}


uint16_t BINARY_PARSER::readU16( size_t aOffset ) const
{
    if( aOffset + 2 > m_data.size() )
    {
        THROW_IO_ERROR( wxString::Format( "PADS binary read out of bounds at offset %zu (file size %zu)",
                                          aOffset, m_data.size() ) );
    }

    return static_cast<uint16_t>( m_data[aOffset] )
           | ( static_cast<uint16_t>( m_data[aOffset + 1] ) << 8 );
}


uint32_t BINARY_PARSER::readU32( size_t aOffset ) const
{
    if( aOffset + 4 > m_data.size() )
    {
        THROW_IO_ERROR( wxString::Format( "PADS binary read out of bounds at offset %zu (file size %zu)",
                                          aOffset, m_data.size() ) );
    }

    return static_cast<uint32_t>( m_data[aOffset] )
           | ( static_cast<uint32_t>( m_data[aOffset + 1] ) << 8 )
           | ( static_cast<uint32_t>( m_data[aOffset + 2] ) << 16 )
           | ( static_cast<uint32_t>( m_data[aOffset + 3] ) << 24 );
}


int32_t BINARY_PARSER::readI32( size_t aOffset ) const
{
    return static_cast<int32_t>( readU32( aOffset ) );
}


std::string BINARY_PARSER::readFixedString( size_t aOffset, size_t aMaxLen ) const
{
    if( aOffset >= m_data.size() )
        return {};

    size_t available = std::min( aMaxLen, m_data.size() - aOffset );
    const uint8_t* start = &m_data[aOffset];
    const uint8_t* end = start + available;

    // Find null terminator
    const uint8_t* null_pos = std::find( start, end, 0 );
    size_t len = static_cast<size_t>( null_pos - start );

    // Validate printable ASCII
    for( size_t i = 0; i < len; ++i )
    {
        if( start[i] < 0x20 || start[i] >= 0x7F )
            return {};
    }

    std::string result( reinterpret_cast<const char*>( start ), len );

    // Trim trailing whitespace
    while( !result.empty() && result.back() == ' ' )
        result.pop_back();

    return result;
}


void BINARY_PARSER::parseHeader()
{
    if( m_data.size() < static_cast<size_t>( HEADER_SIZE + FOOTER_SIZE ) )
        THROW_IO_ERROR( "File too small for PADS binary format" );

    if( m_data[0] != 0x00 || m_data[1] != 0xFF )
        THROW_IO_ERROR( "Invalid magic bytes" );

    m_version = readU16( 2 );

    if( m_version != 0x2021 && m_version != 0x2025 && m_version != 0x2026 && m_version != 0x2027 )
        THROW_IO_ERROR( "Unsupported PADS binary version" );

    m_numDirEntries = dirEntryCount();
}


void BINARY_PARSER::parseFooter()
{
    size_t footerStart = m_data.size() - FOOTER_SIZE;

    // Verify GUID at footer offset + 4
    if( std::memcmp( &m_data[footerStart + 4], FOOTER_GUID, 38 ) != 0 )
        THROW_IO_ERROR( "Invalid footer GUID" );

    uint32_t sizeCheck = readU32( footerStart + 42 );
    uint32_t expected = static_cast<uint32_t>( m_data.size() - FOOTER_SIZE );

    if( sizeCheck != expected )
    {
        wxLogWarning( "PADS binary footer size mismatch: stored=%u, expected=%u",
                      sizeCheck, expected );
    }
}


void BINARY_PARSER::parseDirectory()
{
    size_t dirStart = HEADER_SIZE;
    size_t dirSize = static_cast<size_t>( m_numDirEntries ) * DIR_ENTRY_SIZE;

    if( dirStart + dirSize > m_data.size() )
        THROW_IO_ERROR( "File too small for section directory" );

    uint32_t dataOffset = static_cast<uint32_t>( dirStart + dirSize );

    m_dirEntries.clear();
    m_dirEntries.reserve( m_numDirEntries );

    for( int i = 0; i < m_numDirEntries; ++i )
    {
        size_t off = dirStart + static_cast<size_t>( i ) * DIR_ENTRY_SIZE;

        DirEntry entry;
        entry.index = i;
        entry.count = readU32( off );
        entry.totalBytes = readU32( off + 4 );
        entry.dataOffset = 0;
        entry.perItem = 0;

        if( i > 0 )
        {
            entry.dataOffset = dataOffset;

            if( entry.count > 0 && entry.totalBytes > 0 )
                entry.perItem = entry.totalBytes / entry.count;

            dataOffset += entry.totalBytes;
        }

        m_dirEntries.push_back( entry );
    }
}


const BINARY_PARSER::DirEntry* BINARY_PARSER::getSection( int aIndex ) const
{
    if( aIndex >= 0 && aIndex < static_cast<int>( m_dirEntries.size() ) )
        return &m_dirEntries[aIndex];

    return nullptr;
}


const uint8_t* BINARY_PARSER::sectionData( int aIndex ) const
{
    const DirEntry* entry = getSection( aIndex );

    if( !entry || entry->totalBytes == 0 )
        return nullptr;

    if( entry->dataOffset + entry->totalBytes > m_data.size() )
        return nullptr;

    return &m_data[entry->dataOffset];
}


uint32_t BINARY_PARSER::sectionSize( int aIndex ) const
{
    const DirEntry* entry = getSection( aIndex );

    if( !entry )
        return 0;

    return entry->totalBytes;
}


double BINARY_PARSER::toBasicCoordX( int32_t aRawValue ) const
{
    return static_cast<double>( aRawValue - ( m_originFound ? m_originX : 0 ) );
}


double BINARY_PARSER::toBasicCoordY( int32_t aRawValue ) const
{
    return static_cast<double>( aRawValue - ( m_originFound ? m_originY : 0 ) );
}


double BINARY_PARSER::toBasicAngle( int32_t aRawAngle ) const
{
    if( aRawAngle == 0 )
        return 0.0;

    return static_cast<double>( aRawAngle ) / static_cast<double>( ANGLE_SCALE );
}


void BINARY_PARSER::parseBoardSetup()
{
    const uint8_t* data = sectionData( 1 );
    uint32_t       size = sectionSize( 1 );

    if( !data || size < 160 )
        return;

    // Board setup section contains u32 parameters at known offsets.
    // Index 4 holds the maximum layer count.
    uint32_t maxLayer = readU32( m_dirEntries[1].dataOffset + 4 * 4 );

    if( maxLayer >= 1 && maxLayer <= 64 )
        m_parameters.layer_count = static_cast<int>( maxLayer );
    else
        m_parameters.layer_count = 2;

    // Section 1 stores the coordinate origin at offset +60/+64 as i32 LE pair.
    // This is the same value as DFT_CONFIGURATION POLAR_GRID X/Y but is always present.
    size_t secBase = m_dirEntries[1].dataOffset;

    if( size >= 68 )
    {
        m_originX = readI32( secBase + 60 );
        m_originY = readI32( secBase + 64 );
        m_originFound = true;

        m_parameters.origin.x = static_cast<double>( m_originX );
        m_parameters.origin.y = static_cast<double>( m_originY );
    }

    // Binary coordinates are in BASIC units (1 BASIC = 1/38100 mil).
    // Set MILS for the display unit; actual coordinate handling uses BASIC mode
    // in the wrapper via SetBasicUnitsMode(true).
    m_parameters.units = UNIT_TYPE::MILS;
}


void BINARY_PARSER::parseStringPool()
{
    const uint8_t* data = sectionData( 57 );
    uint32_t       size = sectionSize( 57 );

    if( !data || size == 0 )
        return;

    m_stringPoolBytes.assign( data, data + size );
}


void BINARY_PARSER::parsePartPlacements()
{
    const DirEntry* entry = getSection( 22 );

    if( !entry || entry->count == 0 || entry->perItem == 0 )
        return;

    const uint8_t* data = sectionData( 22 );

    if( !data )
        return;

    bool isOld = isOldFormat();
    uint32_t recSize = entry->perItem;

    // Field offsets differ between versions
    int nameOff = 0, xOff = 0, yOff = 0, angleOff = 0;

    if( isOld )
    {
        nameOff = 76;
        xOff = 92;
        yOff = -1;
        angleOff = 4;
    }
    else
    {
        nameOff = 44;
        xOff = 60;
        yOff = 64;
        angleOff = 68;
    }

    for( uint32_t i = 0; i < entry->count; ++i )
    {
        size_t off = static_cast<size_t>( i ) * recSize;

        if( off + recSize > entry->totalBytes )
            break;

        size_t base = entry->dataOffset + off;
        std::string refDes = readFixedString( base + nameOff, 16 );

        if( refDes.empty() || !std::isalnum( static_cast<unsigned char>( refDes[0] ) ) )
            continue;

        int32_t x = readI32( base + xOff );

        // v0x2021 Y coordinate encoding is not yet solved. Use 0 as placeholder.
        int32_t y = ( yOff >= 0 ) ? readI32( base + yOff ) : 0;
        int32_t angleRaw = readI32( base + angleOff );

        PART part;
        part.name = refDes;
        part.location.x = toBasicCoordX( x );
        part.location.y = toBasicCoordY( y );
        part.rotation = toBasicAngle( angleRaw );
        part.bottom_layer = false;
        part.units = "M";

        m_parts.push_back( part );
    }
}


void BINARY_PARSER::parseSection19Parts()
{
    // Part records can be embedded in sections other than section 22.
    // Scan sections 19 (design_rules) and 21 (board_outline) for FEFF-delimited part records.
    bool isOld = isOldFormat();
    int nameOff = 0, xOff = 0, yOff = 0, angleOff = 0, feffOff = 0;

    if( isOld )
    {
        nameOff = 76;
        xOff = 92;
        yOff = -1;
        angleOff = 4;
        feffOff = 28;
    }
    else
    {
        nameOff = 44;
        xOff = 60;
        yOff = 64;
        angleOff = 68;
        feffOff = 92;
    }

    int recSize = feffOff + 2;

    std::set<std::string> existingRefs;

    for( const auto& p : m_parts )
        existingRefs.insert( p.name );

    for( int secIdx : { 19, 21 } )
    {
        const DirEntry* entry = getSection( secIdx );

        if( !entry || entry->totalBytes == 0 )
            continue;

        const uint8_t* data = sectionData( secIdx );
        uint32_t       size = sectionSize( secIdx );

        if( !data || size == 0 )
            continue;

        for( size_t pos = 0; pos + 1 < size; ++pos )
        {
            if( data[pos] != 0xFE || data[pos + 1] != 0xFF )
                continue;

            int recStart = static_cast<int>( pos ) - feffOff;

            if( recStart < 0 || recStart + recSize > static_cast<int>( size ) )
                continue;

            size_t base = entry->dataOffset + recStart;
            std::string refDes = readFixedString( base + nameOff, 16 );

            if( refDes.empty() || !std::isalnum( static_cast<unsigned char>( refDes[0] ) ) )
                continue;

            if( existingRefs.count( refDes ) )
                continue;

            int32_t x = readI32( base + xOff );
            int32_t y = ( yOff >= 0 ) ? readI32( base + yOff ) : 0;
            int32_t angleRaw = readI32( base + angleOff );

            PART part;
            part.name = refDes;
            part.location.x = toBasicCoordX( x );
            part.location.y = toBasicCoordY( y );
            part.rotation = toBasicAngle( angleRaw );
            part.bottom_layer = false;
            part.units = "M";

            m_parts.push_back( part );
            existingRefs.insert( refDes );
        }
    }
}


void BINARY_PARSER::parsePadStacks()
{
    const DirEntry* entry = getSection( 4 );

    if( !entry || entry->count == 0 || entry->perItem == 0 )
        return;

    const uint8_t* data = sectionData( 4 );

    if( !data )
        return;

    bool isNew = !isOldFormat();
    uint32_t recSize = entry->perItem;

    // We store pad stacks indexed by their position in section 4.
    // Part decals reference these by index.
    for( uint32_t i = 0; i < entry->count; ++i )
    {
        size_t off = static_cast<size_t>( i ) * recSize;

        if( off + recSize > entry->totalBytes )
            break;

        size_t base = entry->dataOffset + off;

        int32_t  padWidth = 0, drill = 0, finLength = 0, angleRaw = 0;
        uint8_t  marker = 0, shapeCode = 0;
        uint16_t layerCount = 0;

        if( isNew )
        {
            padWidth = readI32( base + 28 );
            drill = readI32( base + 32 );
            finLength = readI32( base + 36 );
            angleRaw = readI32( base + 48 );
            marker = readU8( base + 56 );
            shapeCode = readU8( base + 57 );
            layerCount = readU16( base + 58 );
        }
        else
        {
            padWidth = readI32( base + 24 );
            drill = readI32( base + 28 );
            finLength = readI32( base + 32 );
            angleRaw = readI32( base + 40 );
            marker = readU8( base + 48 );
            shapeCode = readU8( base + 49 );
            layerCount = readU16( base + 50 );
        }

        // Only process valid pad definitions (marker == 0xFE)
        if( marker != 0xFE )
            continue;

        std::string shapeName = "R";
        auto shapeIt = PAD_SHAPE_NAMES.find( shapeCode );

        if( shapeIt != PAD_SHAPE_NAMES.end() )
            shapeName = shapeIt->second;

        double angle = toBasicAngle( angleRaw );

        // Build a PAD_STACK_LAYER for the default layer (layer 0)
        PAD_STACK_LAYER psl;
        psl.layer = 0;
        psl.shape = shapeName;
        psl.sizeA = static_cast<double>( padWidth );
        psl.sizeB = static_cast<double>( padWidth );
        psl.drill = static_cast<double>( drill );
        psl.plated = ( drill > 0 );
        psl.rotation = angle;
        psl.finger_offset = static_cast<double>( finLength );

        m_padStackCache[static_cast<int>( i )].push_back( psl );
    }
}


void BINARY_PARSER::parsePartDecals()
{
    const DirEntry* entry = getSection( 10 );

    if( !entry || entry->count == 0 || entry->perItem == 0 )
        return;

    const uint8_t* data = sectionData( 10 );

    if( !data )
        return;

    bool isNew = !isOldFormat();
    uint32_t recSize = entry->perItem;

    for( uint32_t i = 0; i < entry->count; ++i )
    {
        size_t off = static_cast<size_t>( i ) * recSize;

        if( off + recSize > entry->totalBytes )
            break;

        size_t base = entry->dataOffset + off;

        std::string name;
        std::string units = "I";

        if( isNew )
        {
            name = readFixedString( base + 44, 32 );
            uint8_t unitFlag = readU8( base + 76 );
            units = ( unitFlag == 0x4D ) ? "M" : "I";
        }
        else
        {
            name = readFixedString( base + 28, 32 );
        }

        if( name.empty() )
            continue;

        PART_DECAL decal;
        decal.name = name;
        decal.units = units;

        // TODO: Parse terminal positions from the binary format.
        // For now, create a minimal decal that the converter can reference.

        m_decals[name] = decal;
    }
}


void BINARY_PARSER::parseFootprintDefs()
{
    // Section 17 links part type names to decal indices via 224-byte records.
    // name@+156, decal_idx@+112. Old format stores UI data here instead.
    if( isOldFormat() )
        return;

    const DirEntry* entry = getSection( 17 );

    if( !entry || entry->count == 0 || entry->perItem < 188 )
        return;

    const uint8_t* data = sectionData( 17 );

    if( !data )
        return;

    // Build index of section 10 decal names for resolving decal indices
    const DirEntry* decalEntry = getSection( 10 );
    std::map<uint32_t, std::string> decalIndexToName;

    if( decalEntry && decalEntry->count > 0 && decalEntry->perItem > 0 )
    {
        for( uint32_t i = 0; i < decalEntry->count; ++i )
        {
            size_t dOff = static_cast<size_t>( i ) * decalEntry->perItem;

            if( dOff + decalEntry->perItem > decalEntry->totalBytes )
                break;

            size_t dBase = decalEntry->dataOffset + dOff;
            std::string decalName = readFixedString( dBase + 44, 32 );

            if( !decalName.empty() )
                decalIndexToName[i] = decalName;
        }
    }

    uint32_t recSize = entry->perItem;

    // Build a map from footprint type name to decal name
    std::map<std::string, std::string> fpTypeToDecal;

    for( uint32_t i = 0; i < entry->count; ++i )
    {
        size_t off = static_cast<size_t>( i ) * recSize;

        if( off + recSize > entry->totalBytes )
            break;

        size_t base = entry->dataOffset + off;
        uint32_t decalIdx = readU32( base + 112 );
        std::string fpTypeName = readFixedString( base + 156, 32 );

        if( fpTypeName.empty() )
            continue;

        auto decalIt = decalIndexToName.find( decalIdx );

        if( decalIt != decalIndexToName.end() )
            fpTypeToDecal[fpTypeName] = decalIt->second;
    }

    // Store the mapping for use by part placement linking.
    // The metadata region maps ref-des -> part-type-name, which we
    // don't parse yet. For now, store as a member for future use.
    m_fpTypeToDecal = fpTypeToDecal;
}


void BINARY_PARSER::parseLineVertices()
{
    const DirEntry* entry = getSection( 12 );

    if( !entry || entry->count == 0 )
        return;

    const uint8_t* data = sectionData( 12 );

    if( !data )
        return;

    m_lineVertices.clear();
    m_lineVertices.reserve( entry->count );

    for( uint32_t i = 0; i < entry->count; ++i )
    {
        size_t off = static_cast<size_t>( i ) * 12;

        if( off + 12 > entry->totalBytes )
            break;

        size_t base = entry->dataOffset + off;

        LineVertex v;
        v.x = readI32( base );
        v.y = readI32( base + 4 );
        v.extra = readU32( base + 8 );

        m_lineVertices.push_back( v );
    }
}


void BINARY_PARSER::parseBoardOutline()
{
    // Section 21 format varies by version:
    //   v0x2026: 16-byte records [u32 vertex_count, u32 unk1, u32 unk2, u32 sentinel=0xFFFFFFFF]
    //   v0x2025: Mixed ASCII/binary records with completely different layout
    //   v0x2027: Stores coordinates directly rather than vertex counts
    //   v0x2021: Old format with embedded outline data
    // Only v0x2026 has a decoded record layout, so restrict parsing to that version.
    if( m_version != 0x2026 )
        return;

    if( m_lineVertices.empty() )
        return;

    const DirEntry* entry = getSection( 21 );

    if( !entry || entry->count == 0 )
        return;

    const uint8_t* data = sectionData( 21 );

    if( !data )
        return;

    size_t vertexIdx = 0;

    for( uint32_t i = 0; i < entry->count; ++i )
    {
        size_t off = static_cast<size_t>( i ) * 16;

        if( off + 16 > entry->totalBytes )
            break;

        size_t base = entry->dataOffset + off;
        uint32_t vertexCount = readU32( base );
        uint32_t sentinel = readU32( base + 12 );

        if( sentinel != 0xFFFFFFFF )
            continue;

        if( vertexCount == 0 || vertexCount > 10000
            || vertexIdx + vertexCount > m_lineVertices.size() )
        {
            continue;
        }

        POLYLINE outline;
        outline.layer = 1;
        outline.width = 0.0;
        outline.closed = true;

        for( uint32_t v = 0; v < vertexCount; ++v )
        {
            const LineVertex& lv = m_lineVertices[vertexIdx + v];
            outline.points.emplace_back( static_cast<double>( lv.x ),
                                         static_cast<double>( lv.y ) );
        }

        vertexIdx += vertexCount;

        if( outline.points.size() >= 3 )
            m_boardOutlines.push_back( std::move( outline ) );
    }
}


std::string BINARY_PARSER::extractNetName( const uint8_t* aData, size_t aOffset ) const
{
    if( !aData )
        return {};

    std::string name = readFixedString( aOffset, 48 );

    if( name.empty() )
        return {};

    return name;
}


bool BINARY_PARSER::isValidNetName( const std::string& aName ) const
{
    if( aName.empty() )
        return false;

    char first = aName[0];

    return std::isalpha( static_cast<unsigned char>( first ) )
           || std::isdigit( static_cast<unsigned char>( first ) )
           || first == '+' || first == '~' || first == '_' || first == '/';
}


void BINARY_PARSER::parseNetNames()
{
    std::set<std::string> existing;

    if( !isOldFormat() )
    {
        // New format: section 23 has 424-byte records with net index at +112 and name at +116
        const DirEntry* entry23 = getSection( 23 );

        if( entry23 && entry23->count > 0 && entry23->perItem == 424 )
        {
            for( uint32_t i = 0; i < entry23->count; ++i )
            {
                size_t off = static_cast<size_t>( i ) * 424;

                if( off + 424 > entry23->totalBytes )
                    break;

                size_t base = entry23->dataOffset + off;
                std::string name = readFixedString( base + 116, 48 );

                if( !name.empty() && isValidNetName( name ) && !existing.count( name ) )
                {
                    NET net;
                    net.name = name;
                    m_nets.push_back( net );
                    existing.insert( name );
                }
            }
        }

        // Section 22 fills in power/ground nets from 112-byte records
        const DirEntry* entry22 = getSection( 22 );

        if( entry22 && entry22->count > 0 && entry22->perItem == 112 )
        {
            for( uint32_t i = 0; i < entry22->count; ++i )
            {
                size_t off = static_cast<size_t>( i ) * 112;

                if( off + 112 > entry22->totalBytes )
                    break;

                size_t base = entry22->dataOffset + off;

                for( int nameOff : { 28, 52, 76 } )
                {
                    std::string name = readFixedString( base + nameOff, 24 );

                    if( !name.empty() && isValidNetName( name ) && !existing.count( name ) )
                    {
                        uint32_t netIdx = readU32( base + nameOff - 4 );

                        if( netIdx < 100000 || netIdx >= 0xFFFF0000 )
                        {
                            NET net;
                            net.name = name;
                            m_nets.push_back( net );
                            existing.insert( name );
                            break;
                        }
                    }
                }
            }
        }
    }
    else
    {
        // Old format: section 23 has 144-byte records
        const DirEntry* entry23 = getSection( 23 );

        if( entry23 && entry23->count > 0 && entry23->perItem == 144 )
        {
            for( uint32_t i = 0; i < entry23->count; ++i )
            {
                size_t off = static_cast<size_t>( i ) * 144;

                if( off + 144 > entry23->totalBytes )
                    break;

                size_t base = entry23->dataOffset + off;
                uint32_t netIdx = readU32( base + 8 );
                std::string name = readFixedString( base + 12, 48 );

                if( !name.empty() && isValidNetName( name ) && netIdx < 100000
                    && !existing.count( name ) )
                {
                    NET net;
                    net.name = name;
                    m_nets.push_back( net );
                    existing.insert( name );
                }
            }
        }

        // Old format: section 22 has 96-byte records
        const DirEntry* entry22 = getSection( 22 );

        if( entry22 && entry22->count > 0 && entry22->perItem == 96 )
        {
            for( uint32_t i = 0; i < entry22->count; ++i )
            {
                size_t off = static_cast<size_t>( i ) * 96;

                if( off + 96 > entry22->totalBytes )
                    break;

                size_t base = entry22->dataOffset + off;

                for( int nameOff : { 12, 60 } )
                {
                    std::string name = readFixedString( base + nameOff, 48 );

                    if( !name.empty() && isValidNetName( name ) && !existing.count( name ) )
                    {
                        uint32_t netIdx = readU32( base + nameOff - 4 );

                        if( netIdx < 100000 )
                        {
                            NET net;
                            net.name = name;
                            m_nets.push_back( net );
                            existing.insert( name );
                            break;
                        }
                    }
                }
            }
        }

        // Old format: section 19 (design rules) has some nets stored after 0xFFFFFFFF markers
        const DirEntry* entry19 = getSection( 19 );

        if( entry19 && entry19->count > 0 )
        {
            const uint8_t* sec19Data = sectionData( 19 );

            if( sec19Data )
            {
                size_t sec19Size = entry19->totalBytes;

                for( size_t pos = 0; pos + 4 < sec19Size; ++pos )
                {
                    uint32_t val = static_cast<uint32_t>( sec19Data[pos] )
                                   | ( static_cast<uint32_t>( sec19Data[pos + 1] ) << 8 )
                                   | ( static_cast<uint32_t>( sec19Data[pos + 2] ) << 16 )
                                   | ( static_cast<uint32_t>( sec19Data[pos + 3] ) << 24 );

                    if( val == 0xFFFFFFFF )
                    {
                        for( size_t scan = pos + 4; scan + 2 < sec19Size && scan < pos + 40; ++scan )
                        {
                            if( sec19Data[scan] != 0
                                && std::isalpha( static_cast<unsigned char>( sec19Data[scan] ) ) )
                            {
                                std::string name = readFixedString(
                                        entry19->dataOffset + scan, 48 );

                                if( !name.empty() && isValidNetName( name )
                                    && !existing.count( name ) )
                                {
                                    NET net;
                                    net.name = name;
                                    m_nets.push_back( net );
                                    existing.insert( name );
                                }

                                break;
                            }
                        }

                        pos += 3;
                    }
                }
            }
        }
    }
}


void BINARY_PARSER::parseMetadataRegion()
{
    // Origin is already read from section 1 in parseBoardSetup().
    // Only fall back to the DFT_CONFIGURATION scan if that didn't work.
    if( m_originFound )
        return;

    size_t lastDataEnd = HEADER_SIZE + static_cast<size_t>( m_numDirEntries ) * DIR_ENTRY_SIZE;

    for( const auto& entry : m_dirEntries )
    {
        if( entry.index > 0 && entry.totalBytes > 0 )
        {
            size_t end = entry.dataOffset + entry.totalBytes;

            if( end > lastDataEnd )
                lastDataEnd = end;
        }
    }

    size_t footerStart = m_data.size() - FOOTER_SIZE;

    if( lastDataEnd >= footerStart )
        return;

    size_t dirEnd = HEADER_SIZE + static_cast<size_t>( m_numDirEntries ) * DIR_ENTRY_SIZE;
    parseDftConfig( dirEnd, footerStart );
}


void BINARY_PARSER::parseDftConfig( size_t aStart, size_t aEnd )
{
    // Search for "DFT_CONFIGURATION\0" marker
    static const char DFT_MARKER[] = "DFT_CONFIGURATION";
    size_t markerLen = std::strlen( DFT_MARKER );

    for( size_t pos = aStart; pos + markerLen + 1 < aEnd; ++pos )
    {
        if( std::memcmp( &m_data[pos], DFT_MARKER, markerLen ) == 0
            && m_data[pos + markerLen] == 0 )
        {
            size_t configStart = pos + markerLen + 1;

            // Skip PARENT markers and null bytes
            while( configStart < aEnd )
            {
                if( m_data[configStart] == 0 )
                {
                    ++configStart;
                    continue;
                }

                if( configStart + 7 <= aEnd
                    && std::memcmp( &m_data[configStart], "PARENT\0", 7 ) == 0 )
                {
                    configStart += 7;
                    continue;
                }

                break;
            }

            if( configStart >= aEnd )
                return;

            // Detect format by checking for '.' padding in the first 16 bytes
            std::map<std::string, std::string> config;
            bool hasDot = false;

            if( configStart + 16 <= aEnd )
            {
                for( size_t i = configStart; i < configStart + 16; ++i )
                {
                    if( m_data[i] == '.' )
                    {
                        hasDot = true;
                        break;
                    }
                }
            }

            if( hasDot )
                config = parseDftDotPadded( configStart, aEnd );
            else
                config = parseDftNullSeparated( configStart, aEnd );

            auto xIt = config.find( "X" );
            auto yIt = config.find( "Y" );

            if( xIt != config.end() && yIt != config.end() )
            {
                try
                {
                    m_originX = static_cast<int32_t>( std::stod( xIt->second ) );
                    m_originY = static_cast<int32_t>( std::stod( yIt->second ) );
                    m_originFound = true;

                    m_parameters.origin.x = static_cast<double>( m_originX );
                    m_parameters.origin.y = static_cast<double>( m_originY );
                }
                catch( ... )
                {
                    wxLogTrace( "PADS", "Failed to parse DFT origin values" );
                }
            }

            return;
        }
    }
}


std::map<std::string, std::string>
BINARY_PARSER::parseDftDotPadded( size_t aPos, size_t aEnd ) const
{
    std::map<std::string, std::string> config;

    while( aPos + 16 <= aEnd )
    {
        // Keys are 16-byte fields padded with ASCII '.' (0x2E)
        bool validKey = true;

        for( size_t i = aPos; i < aPos + 16; ++i )
        {
            uint8_t b = m_data[i];

            if( !( ( b >= 0x20 && b <= 0x7E ) || b == 0x00 ) )
            {
                validKey = false;
                break;
            }
        }

        if( !validKey )
            break;

        // Extract key by stripping null bytes and dot padding
        std::string key;

        for( size_t i = aPos; i < aPos + 16; ++i )
        {
            if( m_data[i] == 0 || m_data[i] == '.' )
                break;

            key += static_cast<char>( m_data[i] );
        }

        if( key.empty() )
            break;

        aPos += 16;

        // Skip optional null separator
        if( aPos < aEnd && m_data[aPos] == 0 )
            ++aPos;

        // Read null-terminated value
        size_t valStart = aPos;

        while( aPos < aEnd && m_data[aPos] != 0 )
            ++aPos;

        if( aPos > valStart )
        {
            std::string value( reinterpret_cast<const char*>( &m_data[valStart] ),
                               aPos - valStart );
            config[key] = value;
        }

        if( aPos < aEnd )
            ++aPos;

        // Skip PARENT markers
        if( aPos + 7 <= aEnd
            && std::memcmp( &m_data[aPos], "PARENT\0", 7 ) == 0 )
        {
            aPos += 7;
        }
    }

    return config;
}


std::map<std::string, std::string>
BINARY_PARSER::parseDftNullSeparated( size_t aPos, size_t aEnd ) const
{
    std::map<std::string, std::string> config;

    while( aPos < aEnd )
    {
        // Find null-terminated key
        size_t keyStart = aPos;

        while( aPos < aEnd && m_data[aPos] != 0 )
            ++aPos;

        if( aPos == keyStart )
            break;

        // Validate key is printable ASCII
        bool validKey = true;

        for( size_t i = keyStart; i < aPos; ++i )
        {
            if( m_data[i] < 0x20 || m_data[i] > 0x7E )
            {
                validKey = false;
                break;
            }
        }

        if( !validKey )
            break;

        std::string key( reinterpret_cast<const char*>( &m_data[keyStart] ), aPos - keyStart );

        // Skip null terminator
        if( aPos < aEnd )
            ++aPos;

        if( key == "PARENT" )
            continue;

        // Read null-terminated value
        size_t valStart = aPos;

        while( aPos < aEnd && m_data[aPos] != 0 )
            ++aPos;

        if( aPos <= valStart )
            break;

        std::string value( reinterpret_cast<const char*>( &m_data[valStart] ), aPos - valStart );
        config[key] = value;

        if( aPos < aEnd )
            ++aPos;
    }

    return config;
}


std::string BINARY_PARSER::resolveString( uint32_t aByteOffset ) const
{
    if( m_stringPoolBytes.empty() || aByteOffset >= m_stringPoolBytes.size() )
        return {};

    const uint8_t* start = &m_stringPoolBytes[aByteOffset];
    const uint8_t* end = m_stringPoolBytes.data() + m_stringPoolBytes.size();
    const uint8_t* null_pos = std::find( start, end, 0 );
    size_t len = static_cast<size_t>( null_pos - start );

    for( size_t i = 0; i < len; ++i )
    {
        if( start[i] < 0x20 || start[i] >= 0x7F )
            return {};
    }

    return std::string( reinterpret_cast<const char*>( start ), len );
}


void BINARY_PARSER::parseTextRecords()
{
    // Section 8 contains text records (72 bytes each, all versions)
    // Field offsets (from binary-ASC cross-reference):
    //   [0..3]   u32 string pool byte offset (into section 57)
    //   [28..31] i32 text height (confirmed via ASC height matching)
    //   [32..35] i32 linewidth
    //   [44..47] i32 X coordinate (confirmed via ASC coordinate matching)
    //   [48..51] i32 Y coordinate
    //   [52..55] i32 rotation angle (degrees * ANGLE_SCALE)
    //   [56]     u8  layer number
    //   [68..69] u16 terminator (0xFEFF)
    const DirEntry* entry = getSection( 8 );

    if( !entry || entry->count == 0 || entry->perItem < 72 )
        return;

    const uint8_t* data = sectionData( 8 );

    if( !data )
        return;

    for( uint32_t i = 0; i < entry->count; ++i )
    {
        size_t off = static_cast<size_t>( i ) * 72;

        if( off + 72 > entry->totalBytes )
            break;

        size_t base = entry->dataOffset + off;

        uint32_t strOffset = readU32( base );
        int32_t  height    = readI32( base + 28 );
        int32_t  linewidth = readI32( base + 32 );
        int32_t  x         = readI32( base + 44 );
        int32_t  y         = readI32( base + 48 );
        int32_t  angleRaw  = readI32( base + 52 );
        uint8_t  layer     = readU8( base + 56 );

        std::string content = resolveString( strOffset );

        if( content.empty() )
            continue;

        TEXT text;
        text.content = content;
        text.location.x = toBasicCoordX( x );
        text.location.y = toBasicCoordY( y );
        text.height = static_cast<double>( height );
        text.width = static_cast<double>( linewidth );
        text.layer = static_cast<int>( layer );
        text.rotation = toBasicAngle( angleRaw );

        m_texts.push_back( text );
    }
}


void BINARY_PARSER::parseRouteVertices()
{
    // Route data structure (new format only, v0x2025+):
    //   Section 24 (68 bytes/record): connection records. Records with sentinel
    //     0xFE000000 at u32@20 are track segments. u32@8 and u32@12 are indices
    //     into the section 60 vertex pool (start/end of each segment).
    //   Section 59 (32 bytes/record): via/pin connection endpoints with XY at
    //     the auto-detected marker offset (same layout as section 60 sub-records).
    //   Section 60 (64 bytes/record): route vertex pool. Each record contains
    //     two 32-byte sub-records with a 0x80 marker byte preceding XY data.
    //     The marker position varies between files and is auto-detected.
    if( isOldFormat() )
        return;

    m_routeSegments.clear();
    m_viaLocations.clear();

    // Section 60 vertex pool (primary route vertices).
    // Each 64-byte record contains two 32-byte sub-records. Within each sub-record,
    // a 0x80 marker byte precedes the XY coordinate pair (two i32 values). The marker
    // position varies between files even of the same version, so we auto-detect it by
    // scanning the first few records for the most common 0x80 position.
    const DirEntry* entry60 = getSection( 60 );

    if( !entry60 || entry60->count == 0 || entry60->perItem == 0 || !sectionData( 60 ) )
        return;

    uint32_t n60 = entry60->count;
    uint32_t r60 = entry60->perItem;

    // Auto-detect the 0x80 marker position by scanning the first 32 bytes of up to
    // 100 records and picking the position with the highest hit count.
    int markerOffset = -1;
    {
        uint32_t sampleCount = std::min( n60, static_cast<uint32_t>( 100 ) );
        int bestPos = -1;
        int bestCount = 0;

        for( int candidate = 8; candidate < 28 && candidate + 8 < static_cast<int>( r60 ); ++candidate )
        {
            int hits = 0;

            for( uint32_t s = 0; s < sampleCount; ++s )
            {
                size_t recOff = static_cast<size_t>( s ) * r60;

                if( recOff + r60 > entry60->totalBytes )
                    break;

                if( readU8( entry60->dataOffset + recOff + candidate ) == 0x80 )
                    hits++;
            }

            if( hits > bestCount )
            {
                bestCount = hits;
                bestPos = candidate;
            }
        }

        if( bestCount < static_cast<int>( sampleCount ) / 2 )
            return;

        markerOffset = bestPos;
    }

    int xyOffset = markerOffset + 1;

    // Helper to read XY from a section 60 record
    auto readSec60XY = [&]( uint32_t aRecIdx, int32_t& aX, int32_t& aY ) -> bool
    {
        if( aRecIdx >= n60 )
            return false;

        size_t off = static_cast<size_t>( aRecIdx ) * r60;

        if( off + r60 > entry60->totalBytes )
            return false;

        size_t base = entry60->dataOffset + off;

        if( readU8( base + markerOffset ) != 0x80 )
            return false;

        aX = readI32( base + xyOffset );
        aY = readI32( base + xyOffset + 4 );
        return true;
    };

    // Section 24 connection records: build route segments by following the linking
    static constexpr uint32_t SEC24_SENTINEL = 0xFE000000;
    static constexpr int      SEC24_REC_SIZE = 68;

    const DirEntry* entry24 = getSection( 24 );

    if( entry24 && entry24->count > 0 && entry24->perItem == SEC24_REC_SIZE && sectionData( 24 ) )
    {
        uint32_t n24 = entry24->count;

        for( uint32_t i = 0; i < n24; ++i )
        {
            size_t off = static_cast<size_t>( i ) * SEC24_REC_SIZE;

            if( off + SEC24_REC_SIZE > entry24->totalBytes )
                break;

            size_t base = entry24->dataOffset + off;
            uint32_t sentinel = readU32( base + 20 );

            if( sentinel != SEC24_SENTINEL )
                continue;

            int32_t sec60Start = readI32( base + 8 );
            int32_t sec60End   = readI32( base + 12 );

            if( sec60Start < 0 || sec60End < 0 )
                continue;

            int32_t x1 = 0, y1 = 0, x2 = 0, y2 = 0;

            if( !readSec60XY( static_cast<uint32_t>( sec60Start ), x1, y1 ) )
                continue;

            if( !readSec60XY( static_cast<uint32_t>( sec60End ), x2, y2 ) )
                continue;

            // Width is at u32@24 in section 24 for v0x2025.
            // For other versions it's been observed as 0, so we leave it unset.
            int32_t width = readI32( base + 24 );

            RouteSegment seg;
            seg.x1    = x1;
            seg.y1    = y1;
            seg.x2    = x2;
            seg.y2    = y2;
            seg.width = width;
            m_routeSegments.push_back( seg );
        }
    }

    // Section 59: via/pin connection endpoints
    const DirEntry* entry59 = getSection( 59 );

    if( entry59 && entry59->count > 0 && entry59->perItem > 0 && sectionData( 59 ) )
    {
        uint32_t recSize = entry59->perItem;

        for( uint32_t i = 0; i < entry59->count; ++i )
        {
            size_t off = static_cast<size_t>( i ) * recSize;

            if( off + recSize > entry59->totalBytes )
                break;

            size_t base = entry59->dataOffset + off;

            if( readU8( base + markerOffset ) != 0x80 )
                continue;

            ViaLocation via;
            via.x = readI32( base + xyOffset );
            via.y = readI32( base + xyOffset + 4 );
            m_viaLocations.push_back( via );
        }
    }

    // Build ROUTE objects from the extracted segments.
    // Without layer/net/width fully decoded, we emit a single anonymous route with
    // all segments. The loadTracksAndVias() converter assigns default layer/width.
    if( m_routeSegments.empty() && m_viaLocations.empty() )
        return;

    ROUTE route;
    route.net_name = "";

    for( const auto& seg : m_routeSegments )
    {
        TRACK track;
        track.layer = 0;
        track.width = static_cast<double>( seg.width );
        track.points.emplace_back( static_cast<double>( seg.x1 ), static_cast<double>( seg.y1 ) );
        track.points.emplace_back( static_cast<double>( seg.x2 ), static_cast<double>( seg.y2 ) );
        route.tracks.push_back( std::move( track ) );
    }

    for( const auto& via : m_viaLocations )
    {
        VIA viaDef;
        viaDef.location.x = static_cast<double>( via.x );
        viaDef.location.y = static_cast<double>( via.y );
        route.vias.push_back( std::move( viaDef ) );
    }

    m_routes.push_back( std::move( route ) );
}


void BINARY_PARSER::parseCopperPours()
{
    // Section 14 was originally labeled "copper_pours" but analysis revealed it
    // stores footprint pad position data (36-byte stride entries with XY pairs
    // relative to the footprint origin). Copper pour geometry is stored in the
    // metadata region, not in a numbered section.
    //
    // The pad position data supplements section 10 (partdecals) but is not yet
    // needed since the converter creates placeholder footprints without pads.
}


std::vector<LAYER_INFO> BINARY_PARSER::GetLayerInfos() const
{
    std::vector<LAYER_INFO> infos;
    int layerCount = m_parameters.layer_count;

    for( int i = 1; i <= layerCount; ++i )
    {
        LAYER_INFO info;
        info.number = i;
        info.name = "Layer " + std::to_string( i );
        info.is_copper = true;
        info.required = true;

        info.layer_type = PADS_LAYER_FUNCTION::ROUTING;

        infos.push_back( info );
    }

    return infos;
}

} // namespace PADS_IO

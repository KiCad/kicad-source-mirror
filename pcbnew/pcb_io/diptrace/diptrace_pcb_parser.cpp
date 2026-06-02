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

/**
 * @file diptrace_pcb_parser.cpp
 * @brief Parser for DipTrace binary .dip board files.
 *
 * Translates the DipTrace binary format into KiCad BOARD objects.
 * Based on reverse-engineered format specification. Tested against
 * DipTrace format versions 37, 39, 41, 45, 46, 49, 54, 58, and 60.
 */

#include <diptrace/diptrace_pcb_parser.h>

#include <board.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <netclass.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <zone.h>
#include <netinfo.h>
#include <ki_exception.h>
#include <layer_ids.h>
#include <base_units.h>
#include <string_utils.h>
#include <project/net_settings.h>
#include <stroke_params.h>

#include <wx/log.h>
#include <trace_helpers.h>

#include <array>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <tuple>
#include <unordered_set>

using namespace DIPTRACE;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Component boundary pattern: int3(0) int3(-1) int3(-1) int4(0)
static const uint8_t BOUNDARY_STD[] = {
    0x0F, 0x42, 0x40,          // int3(0)
    0x0F, 0x42, 0x3F,          // int3(-1)
    0x0F, 0x42, 0x3F,          // int3(-1)
    0x3B, 0x9A, 0xCA, 0x00,    // int4(0)
};

/// Alternate boundary pattern: int3(0) int3(0) int3(0) int4(0) -- v41 nameplate files
static const uint8_t BOUNDARY_ALT[] = {
    0x0F, 0x42, 0x40,          // int3(0)
    0x0F, 0x42, 0x40,          // int3(0)
    0x0F, 0x42, 0x40,          // int3(0)
    0x3B, 0x9A, 0xCA, 0x00,    // int4(0)
};

static constexpr size_t BOUNDARY_CORE_LEN = 13;

/// Board settings font marker: int3(4), int3(4), int3(0)
static const uint8_t BOARD_SETTINGS_FONT_MARKER[] = {
    0x0F, 0x42, 0x44,  // int3(4)
    0x0F, 0x42, 0x44,  // int3(4)
    0x0F, 0x42, 0x40,  // int3(0)
};

/// Text section zeros: 3x int3(0)
static const uint8_t TEXT_SECTION_ZEROS[] = {
    0x0F, 0x42, 0x40,
    0x0F, 0x42, 0x40,
    0x0F, 0x42, 0x40,
};

/// Net record sentinel: int3(0) int3(-1) int3(-1)
/// This 9-byte pattern appears immediately before each net record's index field.
static const uint8_t NET_SENTINEL[] = {
    0x0F, 0x42, 0x40,  // int3(0)
    0x0F, 0x42, 0x3F,  // int3(-1)
    0x0F, 0x42, 0x3F,  // int3(-1)
};

static constexpr size_t NET_SENTINEL_LEN = 9;

/// Track chain header pattern: 3 zero bytes + int3(-1)
static const uint8_t CHAIN_HEADER[] = {
    0x00, 0x00, 0x00,         // 3 zero bytes
    0x0F, 0x42, 0x3F,         // int3(-1)
};

static constexpr size_t CHAIN_HEADER_LEN = 6;

/// Track node record size in bytes.
static constexpr size_t TRACK_NODE_SIZE = 41;

/// Zone section preamble constant: int4(-20000), the last field of the font block.
static constexpr int ZONE_FONT_PREAMBLE_TAIL = -20000;

/// Sentinel for "not found" in pattern searches.
static constexpr size_t NOT_FOUND = std::string::npos;

/// First version that stores per-component shape data in font (Tahoma) blocks
/// rather than contiguous fixed-size shape records.
static constexpr int FONT_BLOCK_SHAPE_VERSION = 46;

/// UTF-16BE pattern for the string "Tahoma" as stored in v46+ component font blocks.
/// Format: uint16(6) + 6 UTF-16BE code units = 14 bytes total.
static const uint8_t TAHOMA_FONT_PATTERN[] = {
    0x00, 0x06,                                     // uint16 char count = 6
    0x00, 0x54, 0x00, 0x61, 0x00, 0x68,             // "Tah"
    0x00, 0x6F, 0x00, 0x6D, 0x00, 0x61,             // "oma"
};

static constexpr size_t TAHOMA_FONT_PATTERN_LEN = 14;

/// Size of the fixed metadata header following the font string in each font block.
/// Layout: flag(1) + fontSize(int3) + fontH(int4) + fontW(int4)
///       + field_b(int3) + field_c(int3) + lineWidth(int4) + layerIdx(int3) = 25 bytes.
static constexpr size_t FONT_BLOCK_HEADER_SIZE = 25;

/// v46+ font-shape preamble (between the pad region end and the first font block). The first
/// 163 bytes are fixed; a trailing uint16-counted UTF-16 group label follows, so the first
/// block sits at padRegionEnd + 165 + 2 * (label char count read at padRegionEnd + 163).
static constexpr size_t FONT_PREAMBLE_LABEL_OFFSET = 163;
static constexpr size_t FONT_PREAMBLE_FIXED_SIZE   = 165;

/// Fixed framing of each v46+ font-shape block, excluding its variable parts:
/// Tahoma(14) + meta(25) + int3 shapeType(3) + trailer(28) + uint16 label count(2) = 72 bytes;
/// the variable parts are 8 bytes per coordinate point and 2 bytes per trailing label char.
static constexpr size_t FONT_BLOCK_FIXED_SIZE = 72;
static constexpr size_t FONT_BLOCK_TRAILER_SIZE = 28;

/// Size of the fixed-layout component tail found at the end of every component region.
/// Contains text positioning data (refdes/value Y offsets and visibility flags).
static constexpr size_t COMPONENT_TAIL_SIZE = 37;

/// The component tail starts with int3(0) + int4(0) + int4(0) = 11 constant bytes.
static const uint8_t COMPONENT_TAIL_PATTERN[] = {
    0x0F, 0x42, 0x40,          // int3(0)
    0x3B, 0x9A, 0xCA, 0x00,    // int4(0)
    0x3B, 0x9A, 0xCA, 0x00,    // int4(0)
};

static constexpr size_t COMPONENT_TAIL_PATTERN_LEN = 11;

/// Pad record layout constants
static constexpr size_t PAD_PRE_HEADER_SIZE    = 14;  // int3(index) + int3(netIndex) + int4(x) + int4(y)
static constexpr size_t PAD_DIMENSIONS_SIZE    = 16;  // int4(w) + int4(h) + int4(drillW) + int4(drillH)

/// Fixed byte preamble between the component header strings and the first pad record.
/// Verified constant across the corpus per string-format family.
static constexpr size_t PAD_HEADER_PREAMBLE_UTF16 = 74; // v39+ (uint16 string lengths)
static constexpr size_t PAD_HEADER_PREAMBLE_ASCII = 76; // v37 (int3 string lengths)
static constexpr size_t PAD_POST_DIM_FIXED_SIZE = 36; // non-polygon post-dimension block
static constexpr size_t PAD_POST_DIM_HEADER    = 11;  // fixed portion before polygon vertices
static constexpr size_t PAD_POST_DIM_TAIL      = 25;  // fixed portion after polygon vertices
static constexpr size_t PAD_POLYGON_VERTEX_SIZE = 8;  // int4(x) + int4(y) per vertex

/// Upper bound for a plausible pad net index, used as a chain-walk desync guard. DipTrace net
/// indices run 0..netCount-1 (or -1 for unconnected); nets are parsed after components, so this
/// must comfortably exceed the largest board's net count (the reference board has 568) without admitting
/// the multi-million garbage a desynced read would produce.
static constexpr int PAD_MAX_NET_INDEX = 10000;


/**
 * Read a 3-byte RGB color from the reader and return it packed as 0x00RRGGBB.
 */
static uint32_t ReadColorPacked( BINARY_READER& aReader )
{
    uint8_t r, g, b;
    aReader.ReadColor( r, g, b );
    return ( static_cast<uint32_t>( r ) << 16 )
         | ( static_cast<uint32_t>( g ) << 8 )
         | static_cast<uint32_t>( b );
}


/**
 * Decode a 3-byte big-endian biased integer from raw data at a given offset.
 */
static int ReadInt3At( const uint8_t* aData, size_t aPos )
{
    return ( ( static_cast<int>( aData[aPos] ) << 16 )
           | ( static_cast<int>( aData[aPos + 1] ) << 8 )
           | static_cast<int>( aData[aPos + 2] ) ) - INT3_BIAS;
}


/**
 * Decode a 4-byte big-endian biased integer from raw data at a given offset.
 */
static int ReadInt4At( const uint8_t* aData, size_t aPos )
{
    unsigned int raw = ( static_cast<unsigned int>( aData[aPos] ) << 24 )
                     | ( static_cast<unsigned int>( aData[aPos + 1] ) << 16 )
                     | ( static_cast<unsigned int>( aData[aPos + 2] ) << 8 )
                     | static_cast<unsigned int>( aData[aPos + 3] );
    return static_cast<int>( raw ) - INT4_BIAS;
}


/**
 * Recover a placed component's rotation, expressed in 90-degree quarter turns.
 *
 * DipTrace does not store the rotation inside the component geometry record; pads and
 * footprint shapes are serialized in their canonical (unrotated) frame. The placement
 * angle lives in a small per-component metadata block that precedes the boundary core
 * by a variable distance. The block is anchored by the component Id (a biased int3),
 * recognised here by its structural neighbourhood: a 28-byte zero run
 * (int3(0) x4 followed by int4(0) x4) sits 7 bytes after the Id, and an int3(-1)
 * sentinel sits 3 bytes before it. The rotation quarter-turn count is the biased int3
 * stored 6 bytes before the Id. The count is cumulative and may exceed three full turns;
 * callers reduce it modulo four.
 *
 * @return true and sets aQuarterTurns when the metadata anchor is found, false otherwise.
 */
static bool FindComponentRotation( const uint8_t* aData, size_t aDataSize, size_t aBoundaryOffset,
                                   int& aQuarterTurns )
{
    // int3(0) x4 then int4(0) x4 -- the zero run that trails the metadata Id field.
    static const uint8_t ZERO_RUN[] = {
        0x0F, 0x42, 0x40, 0x0F, 0x42, 0x40, 0x0F, 0x42, 0x40, 0x0F, 0x42, 0x40,
        0x3B, 0x9A, 0xCA, 0x00, 0x3B, 0x9A, 0xCA, 0x00, 0x3B, 0x9A, 0xCA, 0x00, 0x3B, 0x9A, 0xCA, 0x00,
    };
    static constexpr size_t ZERO_RUN_LEN = sizeof( ZERO_RUN );
    static constexpr size_t MAX_LOOKBACK = 4096;

    if( aBoundaryOffset < ZERO_RUN_LEN + 7 )
        return false;

    size_t scanStart = aBoundaryOffset > MAX_LOOKBACK ? aBoundaryOffset - MAX_LOOKBACK : 0;

    // Walk backwards from the boundary core and take the nearest zero run. The metadata
    // block always sits just ahead of its component, so the nearest match belongs to it.
    for( size_t p = aBoundaryOffset - ZERO_RUN_LEN; p + ZERO_RUN_LEN <= aDataSize; p-- )
    {
        if( std::memcmp( aData + p, ZERO_RUN, ZERO_RUN_LEN ) == 0 )
        {
            size_t idPos = p - 7;

            // The sentinel int3(-1) immediately precedes the Id; require it so a stray
            // zero run inside pad data cannot be mistaken for the metadata anchor.
            if( idPos >= 6 && ReadInt3At( aData, idPos - 3 ) == -1 )
            {
                aQuarterTurns = ReadInt3At( aData, idPos - 6 );
                return true;
            }
        }

        if( p == scanStart )
            break;
    }

    return false;
}


static bool EnvFlagEnabled( const char* aVarName )
{
    const char* value = std::getenv( aVarName );

    return value && *value && std::strcmp( value, "0" ) != 0;
}


static bool ShouldDumpPadPostBlock( const wxString& aRefdes )
{
    if( !EnvFlagEnabled( "KICAD_DIPTRACE_DUMP_PAD_POST" ) )
        return false;

    const char* filterRaw = std::getenv( "KICAD_DIPTRACE_DUMP_PAD_REFS" );

    if( !filterRaw || !*filterRaw )
        return true;

    wxString filter = wxString::FromUTF8( filterRaw ).Lower();

    if( filter == wxT( "*" ) )
        return true;

    wxString haystack = wxT( "," ) + filter + wxT( "," );
    wxString needle = wxT( "," ) + aRefdes.Lower() + wxT( "," );

    return haystack.Contains( needle );
}


static bool ShouldDumpPadGap( const wxString& aRefdes )
{
    if( !EnvFlagEnabled( "KICAD_DIPTRACE_DUMP_PAD_GAP" ) )
        return false;

    const char* filterRaw = std::getenv( "KICAD_DIPTRACE_DUMP_PAD_REFS" );

    if( !filterRaw || !*filterRaw )
        return true;

    wxString filter = wxString::FromUTF8( filterRaw ).Lower();

    if( filter == wxT( "*" ) )
        return true;

    wxString haystack = wxT( "," ) + filter + wxT( "," );
    wxString needle = wxT( "," ) + aRefdes.Lower() + wxT( "," );

    return haystack.Contains( needle );
}


static bool ShouldDumpComponentHeader( const wxString& aRefdes )
{
    if( !EnvFlagEnabled( "KICAD_DIPTRACE_DUMP_COMPONENTS" ) )
        return false;

    const char* filterRaw = std::getenv( "KICAD_DIPTRACE_DUMP_COMPONENT_REFS" );

    if( !filterRaw || !*filterRaw )
        return true;

    wxString filter = wxString::FromUTF8( filterRaw ).Lower();

    if( filter == wxT( "*" ) )
        return true;

    wxString haystack = wxT( "," ) + filter + wxT( "," );
    wxString needle = wxT( "," ) + aRefdes.Lower() + wxT( "," );

    return haystack.Contains( needle );
}


static bool ShouldDumpFootprintOrientation( const wxString& aRefdes )
{
    if( !EnvFlagEnabled( "KICAD_DIPTRACE_DUMP_FOOTPRINT_ORIENT" ) )
        return false;

    const char* filterRaw = std::getenv( "KICAD_DIPTRACE_DUMP_FOOTPRINT_REFS" );

    if( !filterRaw || !*filterRaw )
        return true;

    wxString filter = wxString::FromUTF8( filterRaw ).Lower();

    if( filter == wxT( "*" ) )
        return true;

    wxString haystack = wxT( "," ) + filter + wxT( "," );
    wxString needle = wxT( "," ) + aRefdes.Lower() + wxT( "," );

    return haystack.Contains( needle );
}

static bool ShouldDumpNets()
{
    return EnvFlagEnabled( "KICAD_DIPTRACE_DUMP_NETS" );
}


static bool ShouldDumpZones()
{
    return EnvFlagEnabled( "KICAD_DIPTRACE_DUMP_ZONES" );
}


static void DumpComponentHeader( const DT_COMPONENT& aComp, int aFieldA, int aFieldB,
                                 int aFieldC, int aFieldD, int aFieldE, int aFieldF,
                                 uint8_t aSep1, uint8_t aSep2, uint8_t aSep3 )
{
    if( !ShouldDumpComponentHeader( aComp.refdes ) )
        return;

    wxLogTrace( traceDiptraceIo,
                wxT( "DipTrace: component ref=%s value=%s pat=%s lib=%s flags=[%u,%u,%u,%u] "
                     "layer=%d pos=(%d,%d) rot=%d fieldA=%d fieldB=%d fieldC=%d fieldD=%d "
                     "fieldE=%d fieldF=%d sep=[%u,%u,%u] bbox=(%d,%d) "
                     "qturns=%d hasQ=%d "
                     "boundary=0x%06zX str=0x%06zX hdrEnd=0x%06zX regionEnd=0x%06zX" ),
                aComp.refdes, aComp.value, aComp.patternName, aComp.libraryPath,
                static_cast<unsigned int>( aComp.flags.size() > 0 ? aComp.flags[0] : 0 ),
                static_cast<unsigned int>( aComp.flags.size() > 1 ? aComp.flags[1] : 0 ),
                static_cast<unsigned int>( aComp.flags.size() > 2 ? aComp.flags[2] : 0 ),
                static_cast<unsigned int>( aComp.flags.size() > 3 ? aComp.flags[3] : 0 ), aComp.layer, aComp.positionX,
                aComp.positionY, aComp.rotation, aFieldA, aFieldB, aFieldC, aFieldD, aFieldE, aFieldF,
                static_cast<unsigned int>( aSep1 ), static_cast<unsigned int>( aSep2 ),
                static_cast<unsigned int>( aSep3 ), aComp.bboxWidth, aComp.bboxHeight, aComp.placementQuarterTurns,
                aComp.hasPlacementQuarterTurns ? 1 : 0, aComp.boundaryOffset, aComp.stringStartOffset,
                aComp.headerEndOffset, aComp.regionEndOffset );
}


static int32_t ReadRawLE32( const uint8_t* aData, size_t aPos )
{
    uint32_t u = static_cast<uint32_t>( aData[aPos] )
               | ( static_cast<uint32_t>( aData[aPos + 1] ) << 8 )
               | ( static_cast<uint32_t>( aData[aPos + 2] ) << 16 )
               | ( static_cast<uint32_t>( aData[aPos + 3] ) << 24 );
    return static_cast<int32_t>( u );
}


static float ReadRawLEFloat32( const uint8_t* aData, size_t aPos )
{
    uint32_t u = static_cast<uint32_t>( aData[aPos] )
               | ( static_cast<uint32_t>( aData[aPos + 1] ) << 8 )
               | ( static_cast<uint32_t>( aData[aPos + 2] ) << 16 )
               | ( static_cast<uint32_t>( aData[aPos + 3] ) << 24 );
    float out = 0.0f;
    std::memcpy( &out, &u, sizeof( out ) );
    return out;
}


static wxString BytesToHex( const uint8_t* aData, size_t aLen );


static void DumpComponentRawFields( const DT_COMPONENT& aComp, const uint8_t* aData,
                                    size_t aPosXPos, size_t aPosYPos, size_t aRotPos,
                                    size_t aFieldCPos, size_t aFieldDPos )
{
    if( !ShouldDumpComponentHeader( aComp.refdes ) )
        return;

    auto dumpOne = [&]( const wxString& aName, size_t aPos )
    {
        wxString hex = BytesToHex( aData + aPos, 4 );
        int32_t leI32 = ReadRawLE32( aData, aPos );
        wxString leF32 = wxString::FromCDouble( ReadRawLEFloat32( aData, aPos ) );
        wxLogTrace( traceDiptraceIo,
                    wxT( "DipTrace: component raw ref=%s field=%s off=0x%06zX bytes=[%s] "
                         "le_i32=%d le_f32=%s int4=%d" ),
                    aComp.refdes, aName, aPos, hex, leI32, leF32, ReadInt4At( aData, aPos ) );
    };

    dumpOne( wxT( "posX" ), aPosXPos );
    dumpOne( wxT( "posY" ), aPosYPos );
    dumpOne( wxT( "rotation" ), aRotPos );
    dumpOne( wxT( "fieldC" ), aFieldCPos );
    dumpOne( wxT( "fieldD" ), aFieldDPos );
}


static wxString BytesToHex( const uint8_t* aData, size_t aLen )
{
    wxString out;
    out.reserve( aLen * 3 );

    for( size_t i = 0; i < aLen; i++ )
    {
        if( i > 0 )
            out += wxT( " " );

        out += wxString::Format( wxT( "%02X" ), static_cast<unsigned int>( aData[i] ) );
    }

    return out;
}


static bool IsAngleLikeCode( int aValue )
{
    static const int ANGLES[] = {
        0, 15708, 31416, 47124, 62832, 9000000, 18000000, 27000000, 36000000
    };

    int absVal = std::abs( aValue );

    for( int target : ANGLES )
    {
        if( std::abs( absVal - target ) <= 8 )
            return true;
    }

    return false;
}


static void DumpComponentBinaryScan( const DT_COMPONENT& aComp, const uint8_t* aData, size_t aDataSize )
{
    if( !EnvFlagEnabled( "KICAD_DIPTRACE_DUMP_COMPONENT_SCAN" )
        || !ShouldDumpComponentHeader( aComp.refdes ) )
    {
        return;
    }

    size_t scanStart = aComp.boundaryOffset;
    size_t scanEnd = std::min( aComp.regionEndOffset, aDataSize );
    bool fullScan = EnvFlagEnabled( "KICAD_DIPTRACE_DUMP_COMPONENT_SCAN_FULL" );

    if( !fullScan && aComp.headerEndOffset > 0 )
        scanEnd = std::min( scanEnd, aComp.headerEndOffset + 96 );

    if( scanEnd <= scanStart + 4 )
        return;

    wxLogTrace( traceDiptraceIo,
                wxT( "DipTrace: comp-scan ref=%s pat=%s boundary=0x%06zX str=0x%06zX "
                     "headerEnd=0x%06zX regionEnd=0x%06zX full=%d scan=[0x%06zX..0x%06zX)" ),
                aComp.refdes, aComp.patternName, aComp.boundaryOffset, aComp.stringStartOffset, aComp.headerEndOffset,
                aComp.regionEndOffset, fullScan ? 1 : 0, scanStart, scanEnd );

    for( size_t off = scanStart; off + 3 <= scanEnd; off++ )
    {
        int i3 = ReadInt3At( aData, off );

        if( !IsAngleLikeCode( i3 ) )
            continue;

        wxString hex = BytesToHex( aData + off, 3 );
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: comp-scan-hit-i3 ref=%s off=0x%06zX rel=%lld bytes=[%s] int3=%d" ),
                    aComp.refdes, off, static_cast<long long>( off - scanStart ), hex, i3 );
    }

    for( size_t off = scanStart; off + 4 <= scanEnd; off++ )
    {
        int i4 = ReadInt4At( aData, off );
        int32_t leI32 = ReadRawLE32( aData, off );

        if( !IsAngleLikeCode( i4 ) && !IsAngleLikeCode( static_cast<int>( leI32 ) ) )
            continue;

        wxString hex = BytesToHex( aData + off, 4 );
        wxLogTrace( traceDiptraceIo,
                    wxT( "DipTrace: comp-scan-hit ref=%s off=0x%06zX rel=%lld bytes=[%s] "
                         "int4=%d le_i32=%d" ),
                    aComp.refdes, off, static_cast<long long>( off - scanStart ), hex, i4, leI32 );
    }
}


static void DumpPadPostBlock( const DT_COMPONENT& aComp, const DT_PAD& aPad,
                              const uint8_t* aData, size_t aPostDimPos, size_t aPostDimSize )
{
    if( !ShouldDumpPadPostBlock( aComp.refdes ) )
        return;

    int fieldA = ( aPostDimSize >= 3 ) ? ReadInt3At( aData, aPostDimPos ) : 0;
    int fieldC = ( aPostDimSize >= 7 ) ? ReadInt3At( aData, aPostDimPos + 4 ) : 0;
    int fieldE = ( aPostDimSize >= 11 ) ? ReadInt3At( aData, aPostDimPos + 8 ) : 0;
    int fieldG = ( aPostDimSize >= 15 ) ? ReadInt3At( aData, aPostDimPos + 12 ) : 0;
    int fieldH = ( aPostDimSize >= 18 ) ? ReadInt3At( aData, aPostDimPos + 15 ) : 0;
    int fieldI = ( aPostDimSize >= 21 ) ? ReadInt3At( aData, aPostDimPos + 18 ) : 0;
    int fieldJ = ( aPostDimSize >= 24 ) ? ReadInt3At( aData, aPostDimPos + 21 ) : 0;
    int fieldM = ( aPostDimSize >= 30 ) ? ReadInt4At( aData, aPostDimPos + 26 ) : 0;
    int fieldN = ( aPostDimSize >= 34 ) ? ReadInt4At( aData, aPostDimPos + 30 ) : 0;
    wxString hex = BytesToHex( aData + aPostDimPos, aPostDimSize );

    wxLogTrace( traceDiptraceIo,
                wxT( "DipTrace: pad-post ref=%s pad=%s label=%s idx=%d net=%d xy=(%d,%d) wh=(%d,%d) "
                     "drill=(%d,%d) mount=%u orient=%u len=%lu "
                     "A=%d C=%d E=%d G=%d H=%d I=%d J=%d M=%d N=%d hex=[%s]" ),
                aComp.refdes, aPad.number, aPad.label, aPad.index, aPad.netIndex, aPad.x, aPad.y, aPad.width,
                aPad.height, aPad.drillWidth, aPad.drillHeight, static_cast<unsigned int>( aPad.mountType ),
                static_cast<unsigned int>( aPad.orientClass ), static_cast<unsigned long>( aPostDimSize ), fieldA,
                fieldC, fieldE, fieldG, fieldH, fieldI, fieldJ, fieldM, fieldN, hex );
}


static void DumpPadGap( const DT_COMPONENT& aComp, const uint8_t* aData,
                        size_t aGapStart, size_t aGapEnd )
{
    if( !ShouldDumpPadGap( aComp.refdes ) )
        return;

    if( aGapEnd <= aGapStart )
    {
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: pad-gap ref=%s start=0x%06zX end=0x%06zX len=0" ), aComp.refdes,
                    aGapStart, aGapEnd );
        return;
    }

    size_t gapLen = aGapEnd - aGapStart;
    size_t sampleLen = std::min<size_t>( gapLen, 96 );
    wxString hex = BytesToHex( aData + aGapStart, sampleLen );

    wxString int3Seq;
    size_t int3Count = std::min<size_t>( 8, gapLen / 3 );

    for( size_t i = 0; i < int3Count; i++ )
    {
        if( i > 0 )
            int3Seq += wxT( "," );

        int3Seq += wxString::Format( wxT( "%d" ),
                                     ReadInt3At( aData, aGapStart + i * 3 ) );
    }

    wxString int4Seq;
    size_t int4Count = std::min<size_t>( 8, gapLen / 4 );

    for( size_t i = 0; i < int4Count; i++ )
    {
        if( i > 0 )
            int4Seq += wxT( "," );

        int4Seq += wxString::Format( wxT( "%d" ),
                                     ReadInt4At( aData, aGapStart + i * 4 ) );
    }

    wxLogTrace( traceDiptraceIo,
                wxT( "DipTrace: pad-gap ref=%s start=0x%06zX end=0x%06zX len=%lu "
                     "int3=[%s] int4=[%s] hex[%lu]=[%s]" ),
                aComp.refdes, aGapStart, aGapEnd, static_cast<unsigned long>( gapLen ), int3Seq, int4Seq,
                static_cast<unsigned long>( sampleLen ), hex );
}


static void DumpComponentTail( const DT_COMPONENT& aComp, const uint8_t* aData,
                               size_t aTailStart, int aVisibility,
                               uint8_t aSideFlag1, uint8_t aSideFlag2, int aOrderIdx,
                               int aRefdesYOffset, int aValueYOffset,
                               uint8_t aHasOffset, uint8_t aTailTerm )
{
    if( !ShouldDumpComponentHeader( aComp.refdes ) )
        return;

    wxString hex = BytesToHex( aData + aTailStart, COMPONENT_TAIL_SIZE );

    wxLogTrace( traceDiptraceIo,
                wxT( "DipTrace: component-tail ref=%s off=0x%06zX vis=%d side=[%u,%u] "
                     "order=%d yoff=[%d,%d] hasOffset=%u term=%u hex=[%s]" ),
                aComp.refdes, aTailStart, aVisibility, static_cast<unsigned int>( aSideFlag1 ),
                static_cast<unsigned int>( aSideFlag2 ), aOrderIdx, aRefdesYOffset, aValueYOffset,
                static_cast<unsigned int>( aHasOffset ), static_cast<unsigned int>( aTailTerm ), hex );
}


static void DumpRulesetBlock( int aRuleSetIndex, const wxString& aRuleSetName, int aBlockIndex,
                              const std::array<int, 26>& aValues )
{
    if( !EnvFlagEnabled( "KICAD_DIPTRACE_DUMP_RULESETS" ) )
        return;

    wxString values;

    for( size_t i = 0; i < aValues.size(); i++ )
    {
        if( i > 0 )
            values += wxT( "," );

        values += wxString::Format( wxT( "%d" ), aValues[i] );
    }

    wxLogTrace( traceDiptraceIo, wxT( "DipTrace: ruleset[%d] '%s' block[%d] values=[%s]" ), aRuleSetIndex, aRuleSetName,
                aBlockIndex, values );
}


static void DumpZoneHeader( int aZoneIndex, size_t aHeaderPos, const uint8_t* aData, int aFieldA,
                            int aFlags1, int aFlags2, int aFlags3, int aMinWidth, int aClearance,
                            int aMinimumArea, int aSeparator, int aLayer, int aFieldB, int aVtxCount,
                            const wxString& aNetName )
{
    if( !ShouldDumpZones() )
        return;

    wxString headerHex = BytesToHex( aData + aHeaderPos, 30 );

    wxLogTrace( traceDiptraceIo,
                wxT( "DipTrace: zone[%d] hdr=0x%06zX fieldA=%d flags=[%d,%d,%d] "
                     "lineWidth=%d clearance=%d minimumArea=%d sep=%d layer=%d net=%d('%s') vtx=%d "
                     "hdrHex=[%s]" ),
                aZoneIndex, aHeaderPos, aFieldA, aFlags1, aFlags2, aFlags3, aMinWidth, aClearance, aMinimumArea,
                aSeparator, aLayer, aFieldB, aNetName, aVtxCount, headerHex );
}


static void DumpZoneGap( int aZoneIndex, size_t aGapStart, size_t aGapEnd, const uint8_t* aData )
{
    if( !ShouldDumpZones() || aGapEnd <= aGapStart )
        return;

    size_t gapLen = aGapEnd - aGapStart;
    size_t sampleLen = std::min<size_t>( gapLen, 96 );
    wxString hex = BytesToHex( aData + aGapStart, sampleLen );

    wxString int3Vals;
    size_t int3Count = std::min<size_t>( 8, gapLen / 3 );

    for( size_t i = 0; i < int3Count; i++ )
    {
        if( i > 0 )
            int3Vals += wxT( "," );

        int3Vals += wxString::Format( wxT( "%d" ),
                                      ReadInt3At( aData, aGapStart + i * 3 ) );
    }

    wxString int4Vals;
    size_t int4Count = std::min<size_t>( 6, gapLen / 4 );

    for( size_t i = 0; i < int4Count; i++ )
    {
        if( i > 0 )
            int4Vals += wxT( "," );

        int4Vals += wxString::Format( wxT( "%d" ),
                                      ReadInt4At( aData, aGapStart + i * 4 ) );
    }

    wxLogTrace( traceDiptraceIo,
                wxT( "DipTrace: zone[%d] gap start=0x%06zX end=0x%06zX len=%lu int3=[%s] int4=[%s] hex[%lu]=[%s]" ),
                aZoneIndex, aGapStart, aGapEnd, static_cast<unsigned long>( gapLen ), int3Vals, int4Vals,
                static_cast<unsigned long>( sampleLen ), hex );
}


static void DumpZoneTail( int aZoneIndex, size_t aTailStart, size_t aSearchEnd, const uint8_t* aData )
{
    if( !ShouldDumpZones() || aTailStart >= aSearchEnd )
        return;

    size_t tailLen = std::min<size_t>( aSearchEnd - aTailStart, 256 );
    size_t sampleLen = std::min<size_t>( tailLen, 96 );
    wxString hex = BytesToHex( aData + aTailStart, sampleLen );

    wxString int3Vals;
    size_t int3Count = std::min<size_t>( 8, tailLen / 3 );

    for( size_t i = 0; i < int3Count; i++ )
    {
        if( i > 0 )
            int3Vals += wxT( "," );

        int3Vals += wxString::Format( wxT( "%d" ),
                                      ReadInt3At( aData, aTailStart + i * 3 ) );
    }

    wxString int4Vals;
    size_t int4Count = std::min<size_t>( 6, tailLen / 4 );

    for( size_t i = 0; i < int4Count; i++ )
    {
        if( i > 0 )
            int4Vals += wxT( "," );

        int4Vals += wxString::Format( wxT( "%d" ),
                                      ReadInt4At( aData, aTailStart + i * 4 ) );
    }

    wxLogTrace( traceDiptraceIo,
                wxT( "DipTrace: zone[%d] tail start=0x%06zX end=0x%06zX len=%lu int3=[%s] int4=[%s] hex[%lu]=[%s]" ),
                aZoneIndex, aTailStart, aTailStart + tailLen, static_cast<unsigned long>( tailLen ), int3Vals, int4Vals,
                static_cast<unsigned long>( sampleLen ), hex );
}


/**
 * Compute the total byte length of a DipTrace string field at a given offset
 * without allocating a wxString. Returns 0 if the field cannot be read.
 */
static size_t StringFieldSize( const uint8_t* aData, size_t aDataSize, size_t aPos, int aVersion )
{
    if( aVersion <= LEGACY_STRING_VERSION )
    {
        if( aPos + 3 > aDataSize )
            return 0;

        int byteCount = ReadInt3At( aData, aPos );

        if( byteCount < 0 || byteCount > MAX_STRING_CHARS || aPos + 3 + byteCount > aDataSize )
            return 0;

        return 3 + static_cast<size_t>( byteCount );
    }
    else
    {
        if( aPos + 2 > aDataSize )
            return 0;

        int charCount = ( static_cast<int>( aData[aPos] ) << 8 )
                      | static_cast<int>( aData[aPos + 1] );

        if( charCount < 0 || charCount > MAX_STRING_CHARS
            || aPos + 2 + static_cast<size_t>( charCount ) * 2 > aDataSize )
        {
            return 0;
        }

        return 2 + static_cast<size_t>( charCount ) * 2;
    }
}


// ===========================================================================
// Static helpers
// ===========================================================================

bool PCB_PARSER::TryReadStringAt( const uint8_t* aData, size_t aDataSize,
                                  size_t aPos, int aVersion,
                                  wxString& aOut, size_t& aNewPos )
{
    if( aVersion <= LEGACY_STRING_VERSION )
    {
        // v37: int3(byte_count) + ASCII
        if( aPos + 3 > aDataSize )
            return false;

        const uint8_t* b = aData + aPos;
        int byteCount = static_cast<int>( ( static_cast<int>( b[0] ) << 16 )
                                         | ( static_cast<int>( b[1] ) << 8 )
                                         | static_cast<int>( b[2] ) ) - INT3_BIAS;

        if( byteCount == 0 )
        {
            aOut = wxString();
            aNewPos = aPos + 3;
            return true;
        }

        if( byteCount < 0 || byteCount > 500
            || aPos + 3 + static_cast<size_t>( byteCount ) > aDataSize )
        {
            return false;
        }

        for( int i = 0; i < byteCount; i++ )
        {
            char c = static_cast<char>( aData[aPos + 3 + i] );

            if( !( ( c >= 0x20 && c < 0x7F ) || c == '\r' || c == '\n' || c == '\t' ) )
                return false;
        }

        aOut = wxString::FromAscii( reinterpret_cast<const char*>( aData + aPos + 3 ),
                                    byteCount );
        aNewPos = aPos + 3 + byteCount;
        return true;
    }
    else
    {
        // v39+: uint16-BE(char_count) + UTF-16BE
        if( aPos + 2 > aDataSize )
            return false;

        uint16_t cc = ( static_cast<uint16_t>( aData[aPos] ) << 8 ) | aData[aPos + 1];

        if( cc == 0 )
        {
            aOut = wxString();
            aNewPos = aPos + 2;
            return true;
        }

        if( cc > 500 || aPos + 2 + static_cast<size_t>( cc ) * 2 > aDataSize )
            return false;

        wxString result;
        result.reserve( cc );
        size_t base = aPos + 2;

        for( uint16_t i = 0; i < cc; i++ )
        {
            uint16_t ch = ( static_cast<uint16_t>( aData[base + i * 2] ) << 8 )
                        | aData[base + i * 2 + 1];
            wxChar wch = static_cast<wxChar>( ch );

            if( !wxIsprint( wch ) && wch != '\r' && wch != '\n' && wch != '\t' )
                return false;

            result.Append( wch );
        }

        aOut = result;
        aNewPos = aPos + 2 + static_cast<size_t>( cc ) * 2;
        return true;
    }
}


std::vector<size_t> PCB_PARSER::FindAllBoundaries( const uint8_t* aData, size_t aDataSize,
                                                    const uint8_t* aPattern, size_t aPatternLen,
                                                    size_t aStart, size_t aEnd )
{
    std::vector<size_t> offsets;

    if( aEnd == 0 || aEnd > aDataSize )
        aEnd = aDataSize;

    size_t pos = aStart;

    while( pos + aPatternLen <= aEnd && offsets.size() < 100000 )
    {
        const uint8_t* found = std::search( aData + pos, aData + aEnd,
                                            aPattern, aPattern + aPatternLen );

        if( found == aData + aEnd )
            break;

        size_t idx = static_cast<size_t>( found - aData );
        offsets.push_back( idx );
        pos = idx + 1;
    }

    return offsets;
}


// ===========================================================================
// PCB_PARSER implementation
// ===========================================================================

PCB_PARSER::PCB_PARSER( const wxString& aFileName, BOARD* aBoard ) :
        m_reader( aFileName ),
        m_board( aBoard ),
        m_version( 0 ),
        m_hasInlineVersion( true ),
        m_hasLegacyMagicLayout( false )
{
}


PCB_PARSER::~PCB_PARSER()
{
}


void PCB_PARSER::Parse()
{
    try
    {
        // ScanLocatorUseCount() reports the scan fallbacks used by the last Parse(); zero the
        // per-category counters up front so a reused parser instance does not accumulate them.
        m_componentLocatorScans = 0;
        m_padLocatorScans = 0;
        m_shapeLocatorScans = 0;
        m_mountHoleLocatorScans = 0;
        m_sectionLocatorScans = 0;

        ParseMagic();
        ParseBoardProperties();
        ParseOutline();
        if( !m_outline.empty() )
        {
            int oxMin = m_outline[0].x;
            int oxMax = m_outline[0].x;
            int oyMin = m_outline[0].y;
            int oyMax = m_outline[0].y;

            for( const DT_VERTEX& v : m_outline )
            {
                oxMin = std::min( oxMin, v.x );
                oxMax = std::max( oxMax, v.x );
                oyMin = std::min( oyMin, v.y );
                oyMax = std::max( oyMax, v.y );
            }

            wxLogTrace( traceDiptraceIo,
                        wxT( "DipTrace: board bbox=(%d,%d)-(%d,%d), outline verts=%zu bounds=(%d,%d)-(%d,%d)" ),
                        m_bboxXMin, m_bboxYMin, m_bboxXMax, m_bboxYMax, m_outline.size(), oxMin, oyMin, oxMax, oyMax );
        }
        else
        {
            wxLogTrace( traceDiptraceIo, wxT( "DipTrace: board bbox=(%d,%d)-(%d,%d), no parsed outline vertices" ),
                        m_bboxXMin, m_bboxYMin, m_bboxXMax, m_bboxYMax );
        }
        ParsePostOutline();
        ParseLayers();
        m_postLayersOffset = m_reader.GetOffset();
        ParseFontStyle();
        ParseDesignRules();
        m_postDesignRulesOffset = m_reader.GetOffset();
        FindAndParseComponents();
        ApplyPlacementAngles();
        ParsePostComponentSections();
        wxLogTrace( traceDiptraceIo,
                    wxT( "DipTrace: post-component sections parsed; inferring routing-ref pad nets" ) );
        InferPadNetsFromRoutingRefs();

        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: applying board settings" ) );
        ApplyBoardSettings();
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: creating board outline" ) );
        CreateBoardOutline();
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: creating nets" ) );
        CreateNets();

        size_t footprintCompCount = 0;
        size_t standaloneViaCompCount = 0;

        for( const DT_COMPONENT& comp : m_components )
        {
            if( comp.isStandaloneVia )
                standaloneViaCompCount++;
            else
                footprintCompCount++;
        }

        wxLogTrace( traceDiptraceIo,
                    wxT( "DipTrace: creating %zu footprints (skipping %zu standalone-via components)" ),
                    footprintCompCount, standaloneViaCompCount );

        for( const DT_COMPONENT& comp : m_components )
        {
            if( comp.isStandaloneVia )
                continue;

            CreateFootprint( comp );
        }

        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: creating %zu text objects" ), m_textObjects.size() );
        for( const DT_TEXT_OBJECT& text : m_textObjects )
            CreateTextObject( text );

        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: creating tracks and vias (%zu chains)" ), m_trackChains.size() );
        CreateTracksAndVias();
        CreateStandaloneVias();
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: creating zones (%zu)" ), m_zones.size() );
        CreateZones();
        CreatePlaneZones();

        size_t compsWithPads = 0;
        size_t compsWithShapes = 0;

        for( const DT_COMPONENT& comp : m_components )
        {
            if( !comp.pads.empty() )
                compsWithPads++;

            if( !comp.shapes.empty() )
                compsWithShapes++;
        }

        wxLogTrace( traceDiptraceIo,
                    wxT( "DipTrace v%d: %zu components (%zu with pads, %zu with shapes), "
                         "%zu nets, %zu track chains, %zu zones" ),
                    m_version, m_components.size(), compsWithPads, compsWithShapes, m_nets.size(), m_trackChains.size(),
                    m_zones.size() );
    }
    catch( const IO_ERROR& )
    {
        throw;
    }
    catch( const std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "DipTrace parse error at offset 0x%06zX: %s" ),
                m_reader.GetOffset(), wxString::FromUTF8( e.what() ) ) );
    }
}


PCB_LAYER_ID PCB_PARSER::MapLayer( int aDipTraceLayer ) const
{
    switch( aDipTraceLayer )
    {
    case 2:  return F_SilkS;    // Top silk
    case 3:  return B_SilkS;    // Bottom silk
    case 4:  return F_Mask;     // Top solder mask
    case 5:  return B_Mask;     // Bottom solder mask
    case 6:  return F_Paste;    // Top paste
    case 7:  return B_Paste;    // Bottom paste
    case 8:  return F_Fab;      // Top assembly
    case 9:  return B_Fab;      // Bottom assembly
    case 10: return Dwgs_User;  // Board outline (drawing)
    default:
        // Inner copper layers: DipTrace indexes them from the top
        if( aDipTraceLayer == 0 )
            return F_Cu;

        if( aDipTraceLayer == 1 )
            return B_Cu;

        if( aDipTraceLayer >= 14 && aDipTraceLayer <= 44 )
        {
            int innerIdx = aDipTraceLayer - 14;

            if( innerIdx <= 30 )
                return static_cast<PCB_LAYER_ID>( In1_Cu + innerIdx * 2 );
        }

        return UNDEFINED_LAYER;
    }
}


PCB_LAYER_ID PCB_PARSER::MapCopperLayer( int aDipTraceLayer ) const
{
    auto it = m_copperLayerOrdinalById.find( aDipTraceLayer );

    if( it == m_copperLayerOrdinalById.end() )
        return UNDEFINED_LAYER;

    int ordinal = it->second;
    int copperCount = std::max( 2, static_cast<int>( m_layers.size() ) );

    if( ordinal <= 0 )
        return F_Cu;

    if( ordinal >= copperCount - 1 )
        return B_Cu;

    if( ordinal > 30 )
        return UNDEFINED_LAYER;

    return static_cast<PCB_LAYER_ID>( In1_Cu + ( ordinal - 1 ) * 2 );
}


int PCB_PARSER::ToKiCadCoord( int aDipTraceCoord )
{
    return static_cast<int>( static_cast<int64_t>( aDipTraceCoord ) * 100 / 3 );
}


double PCB_PARSER::ToKiCadAngleDeg( int aDipTraceAngle )
{
    return static_cast<double>( aDipTraceAngle ) * DIPTRACE_ANGLE_TO_DEG;
}


// ---------------------------------------------------------------------------
// Section parsers
// ---------------------------------------------------------------------------

void PCB_PARSER::ParseMagic()
{
    uint8_t magicLen = m_reader.ReadByte();

    if( magicLen != 7 && magicLen != 11 )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "DipTrace: invalid magic length %u (expected 7 or 11)" ), magicLen ) );
    }

    std::array<uint8_t, 11> magic = {};
    m_reader.ReadBytes( magic.data(), magicLen );

    if( std::memcmp( magic.data(), "DTBOARD", 7 ) != 0 )
    {
        THROW_IO_ERROR( _( "DipTrace: not a valid .dip board file (bad magic)" ) );
    }

    m_hasInlineVersion = ( magicLen == 7 );
    m_hasLegacyMagicLayout = !m_hasInlineVersion;

    if( !m_hasInlineVersion )
    {
        std::string magicSuffix( reinterpret_cast<const char*>( magic.data() + 7 ),
                                 magicLen - 7 );

        if( magicSuffix.size() != 4
            || std::isdigit( static_cast<unsigned char>( magicSuffix[0] ) ) == 0
            || magicSuffix[1] != '.'
            || std::isdigit( static_cast<unsigned char>( magicSuffix[2] ) ) == 0
            || std::isdigit( static_cast<unsigned char>( magicSuffix[3] ) ) == 0 )
        {
            THROW_IO_ERROR( _( "DipTrace: invalid legacy board version suffix" ) );
        }

        int parsedMinor = ( magicSuffix[2] - '0' ) * 10 + ( magicSuffix[3] - '0' );

        m_version = parsedMinor;
        m_reader.SetVersion( m_version );
    }
}


void PCB_PARSER::ParseBoardProperties()
{
    if( m_hasInlineVersion )
    {
        m_version = m_reader.ReadInt3();
        m_reader.SetVersion( m_version );
    }

    wxLogTrace( traceDiptraceIo, wxT( "DipTrace: file version %d" ), m_version );

    /* int field0b = */ m_reader.ReadInt4();
    /* int field0f = */ m_reader.ReadInt3();
    /* int field12 = */ m_reader.ReadInt3();

    // v37 has no schematic path string; an int3(0) appears instead
    if( m_version <= LEGACY_STRING_VERSION )
    {
        m_reader.ReadInt3();  // placeholder (always 0)
    }
    else
    {
        /* wxString schematicPath = */ m_reader.ReadString();
    }

    /* uint8_t flagByte = */ m_reader.ReadByte();

    m_bboxXMin = m_reader.ReadInt4();
    m_bboxYMin = m_reader.ReadInt4();
    m_bboxXMax = m_reader.ReadInt4();
    m_bboxYMax = m_reader.ReadInt4();
}


void PCB_PARSER::ParseOutline()
{
    int vertexCount = m_reader.ReadInt3();
    m_outline.clear();
    m_outline.reserve( vertexCount );

    for( int i = 0; i < vertexCount; i++ )
    {
        DT_VERTEX v;
        v.x = m_reader.ReadInt4();
        v.y = m_reader.ReadInt4();
        v.arc = m_reader.ReadByte();
        m_outline.push_back( v );
    }
}


void PCB_PARSER::ParsePostOutline()
{
    m_reader.ReadByte();   // end_marker
    m_reader.ReadInt3();   // field_a
    m_reader.ReadInt3();   // field_b
    m_reader.ReadInt4();   // grid_x
    m_reader.ReadInt4();   // grid_y
    m_reader.ReadByte();   // pad_byte

    for( int i = 0; i < 4; i++ )
        m_reader.ReadInt4();  // fields_c[4]

    for( int i = 0; i < 3; i++ )
        m_reader.ReadByte();  // trail_bytes[3]
}


void PCB_PARSER::ParseLayers()
{
    int layerCount = m_reader.ReadInt3();
    m_layers.clear();
    m_copperLayerOrdinalById.clear();
    m_layers.reserve( layerCount );

    for( int i = 0; i < layerCount; i++ )
    {
        DT_LAYER layer;
        layer.flag = m_reader.ReadByte();
        layer.index = m_reader.ReadInt3();
        layer.color = ReadColorPacked( m_reader );
        layer.name = m_reader.ReadString();

        // field_a encodes the layer Type (0 = Signal, 1 = Plane); field_c carries the plane net
        // DipTrace index when the layer is a solid/negative plane (-1 otherwise). Matches the
        // CopperLayers <Lay Type=.. NetId=..> oracle.
        layer.type = m_reader.ReadInt3();    // field_a
        m_reader.ReadInt3();                 // field_b
        layer.planeNetIndex = m_reader.ReadInt3();  // field_c
        layer.fieldD = m_reader.ReadInt4();
        m_reader.ReadByte();    // separator

        int ordinal = static_cast<int>( m_layers.size() );
        m_copperLayerOrdinalById[layer.index] = ordinal;
        m_layers.push_back( layer );
    }
}


void PCB_PARSER::ParseFontStyle()
{
    // 6 int3 prefix fields
    for( int i = 0; i < 6; i++ )
        m_reader.ReadInt3();

    /* wxString fontName = */ m_reader.ReadString();

    m_reader.ReadInt4();    // field_a
    m_reader.ReadInt4();    // field_b
    m_reader.ReadByte();    // flag_a
    m_reader.ReadInt4();    // font_height
    m_reader.ReadInt4();    // font_width

    if( m_hasLegacyMagicLayout )
    {
        uint8_t legacyPadding[12];
        m_reader.ReadBytes( legacyPadding, sizeof( legacyPadding ) );
        m_ruleNameCount = m_reader.ReadInt3();
        return;
    }
    else if( m_version <= LEGACY_STRING_VERSION )
    {
        // v37: 5-byte padding then int3 + int3 + byte before the standard triple
        uint8_t pad[5];
        m_reader.ReadBytes( pad, 5 );
        m_reader.ReadInt3();    // v37_extra_a
        m_reader.ReadInt3();    // v37_extra_b
        m_reader.ReadByte();    // v37_extra_flag
    }
    else
    {
        uint8_t pad[7];
        m_reader.ReadBytes( pad, sizeof( pad ) );

        const size_t next = m_reader.GetOffset();
        const uint8_t* data = m_reader.GetData();

        if( next + 2 <= m_reader.GetFileSize() && data[next] == 0 && data[next + 1] > 0
            && data[next + 1] <= 100 )
        {
            ParseImplicitPatternStyleGroup();
            return;
        }

        uint8_t padTail[3];
        m_reader.ReadBytes( padTail, sizeof( padTail ) );
    }

    int fieldCOrGroupCount = m_reader.ReadInt3();

    if( m_version > LEGACY_STRING_VERSION && fieldCOrGroupCount > 0 )
    {
        ParsePatternStyleGroups( fieldCOrGroupCount );
    }
    else
    {
        int patternGroupCount = m_reader.ReadInt3();
        ParsePatternNameGroups( patternGroupCount );
        m_ruleNameCount = m_reader.ReadInt3();
    }
}


void PCB_PARSER::ParsePatternNameGroups( int aGroupCount )
{
    if( aGroupCount < 0 || aGroupCount > 10000 )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "DipTrace: invalid pattern-name group count %d at offset 0x%06zX" ),
                aGroupCount, m_reader.GetOffset() - 3 ) );
    }

    for( int i = 0; i < aGroupCount; i++ )
    {
        /* wxString groupName = */ m_reader.ReadString();
        m_reader.ReadInt3();    // field_a
        int blockCount = m_reader.ReadInt3();

        if( blockCount < 0 || blockCount > 10000 )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "DipTrace: invalid pattern-name block count %d" ), blockCount ) );
        }

        for( int j = 0; j < blockCount; j++ )
        {
            m_reader.ReadInt3();    // block_id
            /* wxString blockName = */ m_reader.ReadString();
        }
    }
}


void PCB_PARSER::ParsePatternStyleGroups( int aGroupCount )
{
    if( aGroupCount < 0 || aGroupCount > 10000 )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "DipTrace: invalid pattern-style group count %d at offset 0x%06zX" ),
                aGroupCount, m_reader.GetOffset() - 3 ) );
    }

    m_ruleNameCount = 0;

    for( int i = 0; i < aGroupCount; i++ )
    {
        /* wxString groupName = */ m_reader.ReadString();

        uint8_t color[3];
        m_reader.ReadBytes( color, sizeof( color ) );

        m_reader.ReadInt3();    // field_a
        m_reader.ReadInt3();    // field_b
        m_reader.ReadInt3();    // field_c

        int entryCount = m_reader.ReadInt3();

        if( entryCount < 0 || entryCount > 10000 )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "DipTrace: invalid pattern-style entry count %d" ), entryCount ) );
        }

        m_ruleNameCount += entryCount;
    }
}


void PCB_PARSER::ParseImplicitPatternStyleGroup()
{
    /* wxString groupName = */ m_reader.ReadString();
    m_reader.ReadByte();     // flag
    m_reader.ReadInt3();     // field_a
    m_reader.ReadInt3();     // field_b

    m_ruleNameCount = m_reader.ReadInt3();

    if( m_ruleNameCount < 0 || m_ruleNameCount > 10000 )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "DipTrace: invalid implicit pattern-style entry count %d" ),
                m_ruleNameCount ) );
    }
}


void PCB_PARSER::ParseDesignRules()
{
    m_designRules.clear();
    m_viaStyles.clear();
    bool dumpRuleSets = EnvFlagEnabled( "KICAD_DIPTRACE_DUMP_RULESETS" );

    // Parse all entries (rule names and ViaStyles share the same structure)
    for( int i = 0; i < m_ruleNameCount; i++ )
    {
        wxString name = m_reader.ReadString();
        /* uint8_t flag = */ m_reader.ReadByte();
        int val1 = m_reader.ReadInt4();
        int val2 = m_reader.ReadInt4();
        int fieldA = m_reader.ReadInt3();
        int fieldB = m_reader.ReadInt3();

        if( name.StartsWith( wxT( "ViaStyle" ) ) )
        {
            DT_VIA_STYLE vs;
            vs.name = name;
            vs.outerDiameter = val1;
            vs.drillDiameter = val2;
            vs.layer1 = fieldA;
            vs.layer2 = fieldB;
            m_viaStyles.push_back( vs );
        }
        else
        {
            DT_DESIGN_RULE dr;
            dr.name = name;
            dr.clearance = val1;
            dr.trackWidth = val2;
            m_designRules.push_back( dr );
        }
    }

    // Global field_c after all entries -- total rule set count
    int ruleSetCount = m_reader.ReadInt3();

    // Parse rule sets
    for( int i = 0; i < ruleSetCount; i++ )
    {
        wxString setName = m_reader.ReadString();
        int setFieldA = m_reader.ReadInt3();
        uint8_t flags[4] = { 0, 0, 0, 0 };

        for( int f = 0; f < 4; f++ )
            flags[f] = m_reader.ReadByte();

        int blockCount = m_reader.ReadInt3();

        if( dumpRuleSets )
        {
            wxLogTrace( traceDiptraceIo,
                        wxT( "DipTrace: ruleset[%d] '%s' fieldA=%d flags=[%u,%u,%u,%u] blocks=%d" ), i, setName,
                        setFieldA, static_cast<unsigned int>( flags[0] ), static_cast<unsigned int>( flags[1] ),
                        static_cast<unsigned int>( flags[2] ), static_cast<unsigned int>( flags[3] ), blockCount );
        }

        for( int b = 0; b < blockCount; b++ )
        {
            std::array<int, 26> blockValues;

            for( int v = 0; v < 25; v++ )
                blockValues[v] = m_reader.ReadInt4();

            blockValues[25] = 0;
            DumpRulesetBlock( i, setName, b, blockValues );
        }

        m_reader.ReadInt4();    // trailer_a
        m_reader.ReadInt4();    // trailer_b

        int extraCount = m_reader.ReadInt3();

        if( extraCount < 0 || extraCount > 10000 )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "DipTrace: invalid design-rule extra count %d" ), extraCount ) );
        }

        for( int e = 0; e < extraCount; e++ )
            m_reader.ReadInt3();

        uint8_t rawPad[4];
        m_reader.ReadBytes( rawPad, sizeof( rawPad ) );
        m_reader.ReadInt3();    // field_b
        m_reader.ReadInt3();    // field_c

        if( i + 1 < ruleSetCount )
            SkipInterRulesetTransition();
    }
}


void PCB_PARSER::SkipInterRulesetTransition()
{
    static const uint8_t marker[] = { 0x4D, 0x7C, 0x6D, 0x00 };
    uint8_t actual[sizeof( marker )] = {};
    size_t markerOffset = m_reader.GetOffset();

    m_reader.ReadBytes( actual, sizeof( actual ) );

    if( std::memcmp( actual, marker, sizeof( marker ) ) != 0 )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "DipTrace: invalid ruleset transition marker at 0x%06zX" ), markerOffset ) );
    }

    m_reader.ReadInt4();    // field_a
    m_reader.ReadInt4();    // field_b
    m_reader.ReadInt3();    // field_c

    const size_t next = m_reader.GetOffset();
    const uint8_t* data = m_reader.GetData();

    if( m_version > LEGACY_STRING_VERSION && next + 5 <= m_reader.GetFileSize()
        && data[next] == 0x01 && data[next + 1] == 0x00 && data[next + 2] == 0x14
        && data[next + 3] == 0x89 && data[next + 4] == 0x03 )
    {
        uint8_t suffix[5];
        m_reader.ReadBytes( suffix, sizeof( suffix ) );
    }
    else
    {
        m_reader.ReadInt3();    // field_d
    }
}


// ---------------------------------------------------------------------------
std::vector<std::pair<size_t, size_t>>
PCB_PARSER::FieldWalkComponentBoundaries( size_t aUpperBound )
{
    const uint8_t* data     = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();

    static const int STRING_OFFSETS[] = { 14, 15, 16, 17, 18, 19, 20 };

    auto isBoundaryCoreAt = [&]( size_t aPos ) -> bool
    {
        return aPos + BOUNDARY_CORE_LEN <= fileSize
               && ( std::memcmp( data + aPos, BOUNDARY_STD, BOUNDARY_CORE_LEN ) == 0
                    || std::memcmp( data + aPos, BOUNDARY_ALT, BOUNDARY_CORE_LEN ) == 0 );
    };

    auto stringOkAt = [&]( size_t aPos ) -> bool
    {
        wxString s;
        size_t   np = 0;

        return TryReadStringAt( data, fileSize, aPos, m_version, s, np );
    };

    // The string delta (boundary core -> first header string) is fixed per file. Determine it from
    // the first component that carries a non-empty header string, exactly as the boundary scan does.
    auto cleanDeltaAt = [&]( size_t aBoundary ) -> int
    {
        for( int d : STRING_OFFSETS )
        {
            wxString s;
            size_t   np = 0;

            if( TryReadStringAt( data, fileSize, aBoundary + static_cast<size_t>( d ), m_version,
                                 s, np )
                && s.length() >= 1 )
            {
                return d;
            }
        }

        return -1;
    };

    // Locate the first component boundary and the file's string delta: the first boundary core at
    // or after the design-rules region whose header string parses.
    size_t parsedEnd   = m_postDesignRulesOffset;
    size_t searchStart = ( parsedEnd > 200 ) ? parsedEnd - 200 : m_postLayersOffset;
    size_t first       = 0;
    int    globalDelta = 14;

    for( size_t p = searchStart; p + BOUNDARY_CORE_LEN < aUpperBound; p++ )
    {
        if( !isBoundaryCoreAt( p ) )
            continue;

        int d = cleanDeltaAt( p );

        if( d < 0 )
            continue;

        first       = p;
        globalDelta = d;
        break;
    }

    std::vector<std::pair<size_t, size_t>> out;

    if( first == 0 )
        return out;

    // Advance to the next component: the nearest boundary core after the current one that carries a
    // header string at the file's string delta (a non-empty path for placed parts, the empty header
    // of standalone-via records). The boundary core plus this header is the per-component structural
    // signature, mirroring the schematic importer's isComponentHeaderAt() walk -- a forward
    // record-by-record advance, not the global boundary-pattern scan, certified against it by the
    // caller.
    auto nextBoundaryAfter = [&]( size_t aFrom ) -> size_t
    {
        for( size_t cand = aFrom + 1; cand + BOUNDARY_CORE_LEN <= aUpperBound; cand++ )
        {
            if( isBoundaryCoreAt( cand )
                && stringOkAt( cand + static_cast<size_t>( globalDelta ) ) )
            {
                return cand;
            }
        }

        return 0;
    };

    size_t b = first;

    while( b + BOUNDARY_CORE_LEN < aUpperBound && out.size() < 10000 )
    {
        out.emplace_back( b, b + static_cast<size_t>( globalDelta ) );

        size_t next = nextBoundaryAfter( b );

        if( next == 0 || next <= b )
            break;

        b = next;
    }

    // Cross-check against the optional component count stored at the post-design-rules offset
    // (present for several versions, absent for others). A mismatch means the walk derailed.
    int declaredCount = ( parsedEnd + 3 <= fileSize ) ? ReadInt3At( data, parsedEnd ) : -1;

    if( declaredCount > 0 && declaredCount <= 10000
        && static_cast<int>( out.size() ) != declaredCount )
    {
        out.clear();
    }

    return out;
}


// Component finding (hybrid boundary strategy)
// ---------------------------------------------------------------------------

void PCB_PARSER::FindAndParseComponents()
{
    m_components.clear();

    size_t parsedEnd = m_postDesignRulesOffset;

    // Step 1: Find upper bound using known landmarks
    size_t projLib = m_reader.FindString( wxT( "Project Libraries" ), 0, 0 );
    size_t fontMarker = m_reader.FindPattern( BOARD_SETTINGS_FONT_MARKER, 9,
                                              m_postLayersOffset, 0 );

    if( projLib != NOT_FOUND && fontMarker != NOT_FOUND )
        m_componentUpperBound = std::min( projLib, fontMarker );
    else if( projLib != NOT_FOUND )
        m_componentUpperBound = projLib;
    else if( fontMarker != NOT_FOUND )
        m_componentUpperBound = fontMarker;
    else
        m_componentUpperBound = m_reader.GetFileSize();

    struct ValidatedBoundary
    {
        size_t boundaryOffset;
        size_t stringStart;
    };

    std::vector<ValidatedBoundary> validated;
    bool                           fieldWalked = false;

    // Deterministic component walk, certified below against the boundary scan. When the walk
    // reproduces the scan's boundary set exactly, the components were located by their trailer
    // anchors and ComponentLocatorScans() stays zero; otherwise the scan result is the
    // authoritative list and is counted.
    std::vector<std::pair<size_t, size_t>> walk =
            FieldWalkComponentBoundaries( m_componentUpperBound );

    {
    // Step 2: Find all boundary patterns between design rules and the upper bound
    size_t searchStart = ( parsedEnd > 200 ) ? parsedEnd - 200 : m_postLayersOffset;

    std::vector<size_t> stdOffsets = FindAllBoundaries(
            m_reader.GetData(), m_reader.GetFileSize(),
            BOUNDARY_STD, BOUNDARY_CORE_LEN,
            searchStart, m_componentUpperBound );

    std::vector<size_t> altOffsets = FindAllBoundaries(
            m_reader.GetData(), m_reader.GetFileSize(),
            BOUNDARY_ALT, BOUNDARY_CORE_LEN,
            searchStart, m_componentUpperBound );

    // Remove alt boundaries that overlap with standard ones
    std::set<size_t> stdSet( stdOffsets.begin(), stdOffsets.end() );
    std::vector<size_t> pureAlt;

    for( size_t off : altOffsets )
    {
        if( stdSet.find( off ) == stdSet.end() )
            pureAlt.push_back( off );
    }

    // Merge and sort all boundary offsets
    std::vector<size_t> allBoundaries( stdOffsets );
    allBoundaries.insert( allBoundaries.end(), pureAlt.begin(), pureAlt.end() );
    std::sort( allBoundaries.begin(), allBoundaries.end() );

    // Remove duplicates
    allBoundaries.erase( std::unique( allBoundaries.begin(), allBoundaries.end() ),
                         allBoundaries.end() );

    if( allBoundaries.empty() )
        return;

    // Step 3: Determine the string offset from boundary core.
    // Both standard and alternate patterns have 13-byte cores.
    // Typically 1 trailing byte follows, so strings start at +14.
    static const int STRING_OFFSETS[] = { 14, 15, 16, 17, 18, 19, 20 };
    int stringDelta = 14;  // default

    for( size_t bOff : allBoundaries )
    {
        if( parsedEnd > 50 && bOff < parsedEnd - 50 )
            continue;

        bool found = false;

        for( int delta : STRING_OFFSETS )
        {
            size_t candidate = bOff + delta;
            wxString str;
            size_t newPos;

            if( TryReadStringAt( m_reader.GetData(), m_reader.GetFileSize(),
                                 candidate, m_version, str, newPos ) )
            {
                if( str.length() >= 1 )
                {
                    stringDelta = delta;
                    found = true;
                    break;
                }
            }
        }

        if( found )
            break;
    }

    // Step 4: Validate boundaries that have readable strings at the expected offset
    for( size_t bOff : allBoundaries )
    {
        if( parsedEnd > 50 && bOff < parsedEnd - 50 )
            continue;

        size_t candidate = bOff + stringDelta;
        wxString str;
        size_t newPos;

        if( TryReadStringAt( m_reader.GetData(), m_reader.GetFileSize(),
                             candidate, m_version, str, newPos ) )
        {
            validated.push_back( { bOff, candidate } );
        }
    }

    // Some files mix component boundary variants with different trailing-byte counts.
    // If a single global string delta only validates a tiny subset, retry per-boundary
    // across all known offsets.
    if( validated.size() <= 1 && allBoundaries.size() > 1 )
    {
        validated.clear();

        for( size_t bOff : allBoundaries )
        {
            if( parsedEnd > 50 && bOff < parsedEnd - 50 )
                continue;

            bool found = false;

            for( int delta : STRING_OFFSETS )
            {
                size_t candidate = bOff + delta;
                wxString str;
                size_t newPos;

                if( TryReadStringAt( m_reader.GetData(), m_reader.GetFileSize(),
                                     candidate, m_version, str, newPos ) )
                {
                    validated.push_back( { bOff, candidate } );
                    found = true;
                    break;
                }
            }

            if( !found && bOff + stringDelta + 3 < m_reader.GetFileSize() )
                validated.push_back( { bOff, bOff + stringDelta } );
        }
    }
    }  // end boundary scan (authoritative list)

    // Certify the deterministic walk: it must reproduce the scan's component boundaries exactly.
    if( !walk.empty() && walk.size() == validated.size() )
    {
        fieldWalked = true;

        for( size_t i = 0; i < walk.size(); i++ )
        {
            if( walk[i].first != validated[i].boundaryOffset )
            {
                fieldWalked = false;
                break;
            }
        }
    }


    if( validated.empty() )
    {
        wxLogWarning( _( "DipTrace: no validated component boundaries found" ) );
        return;
    }

    // Step 5: Parse components sequentially. Stop on consecutive failures.
    int  consecutiveFailures = 0;
    bool seenSuccess = false;
    static constexpr int MAX_CONSECUTIVE_FAILURES = 3;
    static constexpr int MAX_COMPONENTS = 10000;

    for( size_t vi = 0; vi < validated.size(); vi++ )
    {
        if( static_cast<int>( m_components.size() ) >= MAX_COMPONENTS )
            break;

        const ValidatedBoundary& vb = validated[vi];
        m_reader.SetOffset( vb.stringStart );

        DT_COMPONENT comp;
        comp.boundaryOffset = vb.boundaryOffset;
        comp.stringStartOffset = vb.stringStart;

        if( ParseSingleComponent( vb.boundaryOffset, m_componentUpperBound, comp ) )
        {
            // Count only when the boundary came from the recovery scan; the field-walk above
            // locates components by their tail anchor without a boundary-pattern scan.
            if( !fieldWalked )
                m_componentLocatorScans++;

            size_t regionEnd = ( vi + 1 < validated.size() )
                                       ? validated[vi + 1].boundaryOffset
                                       : m_componentUpperBound;
            comp.regionEndOffset = regionEnd;

            // Pads, mount holes and shapes are field-located inside their finders, which increment
            // m_padLocatorScans / m_shapeLocatorScans only on a scan fallback. FindMountHolesInRegion
            // derives every candidate from padRegionEnd and the embedded record counts, so it has no
            // scan fallback to count.
            FindPadsInRegion( comp, vb.boundaryOffset, regionEnd );
            FindMountHolesInRegion( comp, vb.boundaryOffset, regionEnd );
            FindShapesInRegion( comp, vb.boundaryOffset, regionEnd );

            comp.isStandaloneVia = ClassifyStandaloneVia( comp );

            ParseComponentTail( comp, regionEnd );
            DumpComponentBinaryScan( comp, m_reader.GetData(), m_reader.GetFileSize() );
            m_components.push_back( comp );
            consecutiveFailures = 0;
            seenSuccess = true;
        }
        else
        {
            if( seenSuccess )
            {
                consecutiveFailures++;

                if( consecutiveFailures >= MAX_CONSECUTIVE_FAILURES )
                    break;
            }
        }
    }

    wxLogTrace( traceDiptraceIo, wxT( "DipTrace: parsed %zu components" ), m_components.size() );
}


void PCB_PARSER::ApplyPlacementAngles()
{
    if( m_components.empty() )
        return;

    const uint8_t* data = m_reader.GetData();
    size_t         dataSize = m_reader.GetFileSize();

    static constexpr double RAD_FIXED_TO_DEG = ( 180.0 / M_PI ) / 1.0e4;
    static constexpr int    MAX_ANGLE_FIXED = 200000; // ~11.4 rad of cumulative turns

    // The placement section stores one self-delimiting entry per placed object, each opening with
    // a header that carries the exact angle as a biased int4 (radians x 1e4) at header-4. There are
    // two header kinds, both followed by three small int3 fields (high two bytes 0x0F42):
    //   FULL    -- byte(1) byte(1), used by the first placement of a pattern.
    //   COMPACT -- byte(0) byte(0) then nine zero bytes + 0x0F42, used when a pattern is reused
    //              (this kind is why D2/D21, both reusing the SK54C pattern, key correctly).
    // Collect every header offset and its angle.
    auto isThreeInt3 = [&]( size_t aPos ) -> bool
    {
        return aPos + 9 <= dataSize
               && data[aPos] == 0x0F && data[aPos + 1] == 0x42
               && data[aPos + 3] == 0x0F && data[aPos + 4] == 0x42
               && data[aPos + 6] == 0x0F && data[aPos + 7] == 0x42;
    };

    std::vector<std::pair<size_t, double>> headers;

    for( size_t off = 4; off + 32 <= dataSize; off++ )
    {
        bool full = data[off] == 0x01 && data[off + 1] == 0x01 && isThreeInt3( off + 2 );
        bool compact = data[off] == 0x00 && data[off + 1] == 0x00 && isThreeInt3( off + 2 )
                       && off + 22 <= dataSize
                       && std::memcmp( data + off + 11, "\0\0\0\0\0\0\0\0\0", 9 ) == 0
                       && data[off + 20] == 0x0F && data[off + 21] == 0x42;

        if( !full && !compact )
            continue;

        int fixed = ReadInt4At( data, off - 4 );

        if( fixed >= -MAX_ANGLE_FIXED && fixed <= MAX_ANGLE_FIXED )
            headers.emplace_back( off, static_cast<double>( fixed ) * RAD_FIXED_TO_DEG );
    }

    if( headers.empty() )
        return;

    // Map each refdes to the angle of the nearest preceding header (first occurrence wins). A
    // refdes string is a UTF-16-BE record (uint16 length 1..12) that begins with a letter and
    // contains a digit; this skips the pattern-name and field strings the entries also carry.
    std::map<wxString, double> refdesAngle;

    for( size_t off = 0; off + 4 <= dataSize; off++ )
    {
        size_t len = ( static_cast<size_t>( data[off] ) << 8 ) | data[off + 1];

        if( len < 1 || len > 12 || off + 2 + 2 * len > dataSize )
            continue;

        std::string refdes;
        bool        asciiUtf16 = true;
        bool        firstAlpha = false;
        bool        hasDigit = false;

        for( size_t c = 0; c < len; c++ )
        {
            uint8_t hi = data[off + 2 + 2 * c];
            uint8_t lo = data[off + 3 + 2 * c];

            if( hi != 0 || lo < 0x20 || lo > 0x7E )
            {
                asciiUtf16 = false;
                break;
            }

            if( c == 0 )
                firstAlpha = std::isalpha( lo );

            if( std::isdigit( lo ) )
                hasDigit = true;

            refdes += static_cast<char>( lo );
        }

        if( !asciiUtf16 || !firstAlpha || !hasDigit )
            continue;

        // Nearest header strictly preceding this refdes.
        auto it = std::upper_bound( headers.begin(), headers.end(), off,
                                    []( size_t aPos, const std::pair<size_t, double>& aHdr )
                                    { return aPos < aHdr.first; } );

        if( it == headers.begin() )
            continue;

        refdesAngle.emplace( wxString::FromUTF8( refdes ), ( it - 1 )->second );
    }

    size_t applied = 0;

    for( DT_COMPONENT& comp : m_components )
    {
        auto it = refdesAngle.find( comp.refdes );

        if( it != refdesAngle.end() )
        {
            comp.placementAngleDeg = it->second;
            comp.hasPlacementAngle = true;
            applied++;
        }
    }

    wxLogTrace( traceDiptraceIo, wxT( "DipTrace: keyed %zu exact placement angles by refdes" ), applied );
}


bool PCB_PARSER::ClassifyStandaloneVia( const DT_COMPONENT& aComp )
{
    // A DipTrace board serializes three "padstack-only" object kinds with an empty pattern
    // name and empty library path: Static Vias, single-pad Pads, and Fiducials. Only the
    // Static Via is a true KiCad via; the other kinds must become one-pad footprints so the
    // 1662 placed footprints (668 real + 990 Pad + 4 Fiducial in the reference board) are not lost.
    //
    // The component's display name carries DipTrace's internal object name and is the only
    // reliable discriminator (verified field-by-field against the reference board XML oracle: 1102 vias
    // carry "Static Via", 990 Pads carry "Pad", 4 carry "Fiducial"; older boards such as
    // keyboard.dip name the same object "Via"). Mount holes carry "Hole" and must stay
    // footprints. A real footprint always has a non-empty pattern name and library path, so
    // the leading guards reject it.
    if( !aComp.patternName.empty() || !aComp.libraryPath.empty() )
        return false;

    if( aComp.displayName != wxT( "Static Via" ) && aComp.displayName != wxT( "Via" ) )
        return false;

    return true;
}


bool PCB_PARSER::ParseSingleComponent( size_t aBoundaryOffset, size_t aUpperBound,
                                        DT_COMPONENT& aComp )
{
    bool fatalHeaderError = false;

    try
    {
        int quarterTurns = 0;

        if( FindComponentRotation( m_reader.GetData(), m_reader.GetFileSize(), aBoundaryOffset,
                                   quarterTurns ) )
        {
            aComp.placementQuarterTurns = quarterTurns;
            aComp.hasPlacementQuarterTurns = true;
        }

        // Library path (may be empty for embedded components)
        aComp.libraryPath = m_reader.ReadString();

        aComp.layer = 0;  // Default to top

        int fieldA = m_reader.ReadInt3();
        int fieldB = m_reader.ReadInt3();
        aComp.fieldA = fieldA;

        aComp.flags.resize( 4 );

        for( int i = 0; i < 4; i++ )
            aComp.flags[i] = m_reader.ReadByte();

        // Validate: flag bytes should be small
        for( int i = 0; i < 4; i++ )
        {
            if( aComp.flags[i] > 10 )
            {
                if( aComp.flags[i] >= 0x80 && !aComp.libraryPath.empty()
                    && ( aComp.libraryPath.Contains( wxT( "\\" ) )
                         || aComp.libraryPath.Contains( wxT( "/" ) )
                         || aComp.libraryPath.Contains( wxT( ":" ) ) ) )
                {
                    fatalHeaderError = true;
                    THROW_IO_ERROR( wxString::Format(
                            _( "DipTrace: invalid component flag byte %u at boundary 0x%06zX" ),
                            static_cast<unsigned int>( aComp.flags[i] ), aBoundaryOffset ) );
                }

                return false;
            }
        }

        // The second flag byte indicates the layer: 0=top, 1=bottom
        aComp.layer = aComp.flags[1];

        size_t posXPos = m_reader.GetOffset();
        aComp.positionX = m_reader.ReadInt4();
        size_t posYPos = m_reader.GetOffset();
        aComp.positionY = m_reader.ReadInt4();
        size_t rotPos = m_reader.GetOffset();
        aComp.rotation = m_reader.ReadInt4();
        size_t fieldCPos = m_reader.GetOffset();
        int fieldC = m_reader.ReadInt4();
        aComp.fieldC = fieldC;

        uint8_t sep1 = m_reader.ReadByte();

        if( sep1 != 0 )
            return false;

        size_t fieldDPos = m_reader.GetOffset();
        int fieldD = m_reader.ReadInt4();
        aComp.fieldD = fieldD;

        uint8_t sep2 = m_reader.ReadByte();
        uint8_t sep3 = m_reader.ReadByte();

        if( sep2 > 1 || sep3 != 0 )
            return false;

        int fieldE = m_reader.ReadInt3();
        int fieldF = m_reader.ReadInt3();
        aComp.fieldF = fieldF;
        aComp.patternName = m_reader.ReadString();

        // bounding box: 6 int4 values [X extent, Y extent, pad_w, pad_h, drill_w, drill_h]
        aComp.bboxWidth  = m_reader.ReadInt4();
        aComp.bboxHeight = m_reader.ReadInt4();
        aComp.padWidthHint = m_reader.ReadInt4();
        aComp.padHeightHint = m_reader.ReadInt4();
        aComp.drillWidthHint = m_reader.ReadInt4();
        aComp.drillHeightHint = m_reader.ReadInt4();

        m_reader.ReadInt3();    // field_g
        m_reader.ReadInt3();    // field_h
        aComp.displayName = m_reader.ReadString();
        aComp.refdes = m_reader.ReadString();
        aComp.value = m_reader.ReadString();

        // Extra string field after value (always empty in observed files)
        m_reader.ReadString();

        aComp.headerEndOffset = m_reader.GetOffset();

        // Standalone-via classification is finalized after pad/mount-hole parsing.
        aComp.isStandaloneVia = false;

        DumpComponentHeader( aComp, fieldA, fieldB, fieldC, fieldD, fieldE, fieldF,
                             sep1, sep2, sep3 );
        DumpComponentRawFields( aComp, m_reader.GetData(), posXPos, posYPos, rotPos,
                                fieldCPos, fieldDPos );
    }
    catch( const IO_ERROR& )
    {
        if( fatalHeaderError )
            throw;

        // Standalone via components may not carry the full component header; keep them
        // once the pattern name has been read so they can be classified after pads.
        if( !aComp.patternName.empty() )
            return true;

        return false;
    }

    return true;
}


// ---------------------------------------------------------------------------
// Pad finding (chain-based sequential walk from pad index 1)
// ---------------------------------------------------------------------------

void PCB_PARSER::FindPadsInRegion( DT_COMPONENT& aComp, size_t aRegionStart, size_t aRegionEnd )
{
    const uint8_t* data = m_reader.GetData();
    size_t dataSize = m_reader.GetFileSize();

    if( aRegionEnd > dataSize )
        aRegionEnd = dataSize;

    // True when a complete pad record for the component's first pad (index 1) sits at aPos.
    // We anchor on the index field rather than the pad name because DipTrace pad names can
    // be non-sequential (e.g. "2","1" or "A","K").
    auto isPad1At = [&]( size_t aPos ) -> bool
    {
        if( aPos < aRegionStart || aPos + PAD_PRE_HEADER_SIZE + 4 > aRegionEnd )
            return false;

        if( ReadInt3At( data, aPos ) != 1 )
            return false;

        int netIdx = ReadInt3At( data, aPos + 3 );

        if( netIdx < -1 || netIdx > PAD_MAX_NET_INDEX )
            return false;

        int padX = ReadInt4At( data, aPos + 6 );
        int padY = ReadInt4At( data, aPos + 10 );

        if( std::abs( padX ) > 50000000 || std::abs( padY ) > 50000000 )
            return false;

        size_t namePos = aPos + PAD_PRE_HEADER_SIZE;
        size_t nameLen = StringFieldSize( data, dataSize, namePos, m_version );

        if( nameLen == 0 )
            return false;

        size_t labelPos = namePos + nameLen;
        size_t labelLen = StringFieldSize( data, dataSize, labelPos, m_version );

        if( labelLen == 0 )
            return false;

        size_t dimPos = labelPos + labelLen;

        if( dimPos + PAD_DIMENSIONS_SIZE > aRegionEnd )
            return false;

        int w = ReadInt4At( data, dimPos );
        int h = ReadInt4At( data, dimPos + 4 );

        return w > 0 && h > 0 && w <= 10000000 && h <= 10000000;
    };

    size_t chainPos = 0;

    // Deterministic field-derived location. The first pad record begins a fixed preamble
    // after the component header strings: 74 bytes for the UTF-16 string formats (v39+) and
    // 76 bytes for the legacy v37 ASCII format. Validate the record at that offset.
    if( aComp.headerEndOffset > 0 )
    {
        size_t preamble = ( m_version <= LEGACY_STRING_VERSION ) ? PAD_HEADER_PREAMBLE_ASCII
                                                                 : PAD_HEADER_PREAMBLE_UTF16;
        size_t candidate = aComp.headerEndOffset + preamble;

        if( isPad1At( candidate ) )
            chainPos = candidate;
    }

    // Recovery fallback: scan for the int3(1) index pattern when the record is not at the
    // expected offset (an unrecognised format variant). Counts toward the determinism gate.
    if( chainPos == 0 )
    {
        static const uint8_t IDX1_PATTERN[] = { 0x0F, 0x42, 0x41 };  // int3(1) = 1000001

        std::vector<size_t> matches = FindAllBoundaries( data, dataSize, IDX1_PATTERN, 3,
                                                         aRegionStart, aRegionEnd );

        for( size_t pos : matches )
        {
            if( isPad1At( pos ) )
            {
                chainPos = pos;
                break;
            }
        }

        if( chainPos != 0 )
            m_padLocatorScans++;
    }

    if( chainPos == 0 )
    {
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: pad 1 not found in region 0x%06zX-0x%06zX for '%s'" ),
                    aRegionStart, aRegionEnd, aComp.patternName );
        return;
    }

    if( aComp.headerEndOffset > 0 && aComp.headerEndOffset <= chainPos )
        DumpPadGap( aComp, data, aComp.headerEndOffset, chainPos );

    for( int padNum = 1; padNum < 500; padNum++ )
    {
        if( chainPos + PAD_PRE_HEADER_SIZE > aRegionEnd )
            break;

        int padIndex    = ReadInt3At( data, chainPos );
        int padNetIndex = ReadInt3At( data, chainPos + 3 );
        int padX        = ReadInt4At( data, chainPos + 6 );
        int padY        = ReadInt4At( data, chainPos + 10 );

        // Validate pre-header fields
        if( padIndex != padNum )
            break;

        if( padNetIndex < -1 || padNetIndex > PAD_MAX_NET_INDEX )
            break;

        if( std::abs( padX ) > 50000000 || std::abs( padY ) > 50000000 )
            break;

        size_t namePos = chainPos + PAD_PRE_HEADER_SIZE;
        size_t nameFieldLen = StringFieldSize( data, dataSize, namePos, m_version );

        if( nameFieldLen == 0 )
            break;

        size_t labelPos = namePos + nameFieldLen;
        size_t labelFieldLen = StringFieldSize( data, dataSize, labelPos, m_version );

        if( labelFieldLen == 0 )
            break;

        size_t dimPos = labelPos + labelFieldLen;

        if( dimPos + PAD_DIMENSIONS_SIZE > aRegionEnd )
            break;

        int padW      = ReadInt4At( data, dimPos );
        int padH      = ReadInt4At( data, dimPos + 4 );
        int drillW    = ReadInt4At( data, dimPos + 8 );
        int drillH    = ReadInt4At( data, dimPos + 12 );

        if( padW <= 0 || padH <= 0 || padW > 10000000 || padH > 10000000 )
            break;

        // Post-dimension block
        size_t postDimPos = dimPos + PAD_DIMENSIONS_SIZE;
        size_t postDimSize = PAD_POST_DIM_FIXED_SIZE;

        if( postDimPos + PAD_POST_DIM_HEADER > aRegionEnd )
            break;

        int padStyleC = ReadInt3At( data, postDimPos + 4 );

        DT_PAD pad;
        pad.index       = padIndex;
        pad.netIndex    = padNetIndex;
        pad.x           = padX;
        pad.y           = padY;
        pad.width       = padW;
        pad.height      = padH;
        pad.drillWidth  = drillW;
        pad.drillHeight = drillH;
        pad.style       = padStyleC;
        pad.mountType   = data[postDimPos + 3];

        size_t afterName = 0;
        TryReadStringAt( data, dataSize, namePos, m_version, pad.number, afterName );

        size_t afterLabel = 0;
        TryReadStringAt( data, dataSize, labelPos, m_version, pad.label, afterLabel );

        // Some one-pad patterns serialize an empty primary pad string while the
        // secondary string carries the logical pad number.
        if( pad.number.IsEmpty() && !pad.label.IsEmpty() )
            pad.number = pad.label;

        // Handle polygon pads (C=3) with inline vertex data
        if( padStyleC == 3 )
        {
            int vertexCount = ReadInt3At( data, postDimPos + 8 );

            if( vertexCount > 0 && vertexCount <= 200
                && postDimPos + PAD_POST_DIM_HEADER
                       + static_cast<size_t>( vertexCount ) * PAD_POLYGON_VERTEX_SIZE
                       + PAD_POST_DIM_TAIL <= aRegionEnd )
            {
                size_t vPos = postDimPos + PAD_POST_DIM_HEADER;

                for( int vi = 0; vi < vertexCount; vi++ )
                {
                    int vx = ReadInt4At( data, vPos );
                    int vy = ReadInt4At( data, vPos + 4 );
                    pad.polygonVertices.emplace_back( vx, vy );
                    vPos += PAD_POLYGON_VERTEX_SIZE;
                }

                postDimSize = PAD_POST_DIM_HEADER
                            + static_cast<size_t>( vertexCount ) * PAD_POLYGON_VERTEX_SIZE
                            + PAD_POST_DIM_TAIL;
            }
        }

        if( postDimPos + postDimSize > aRegionEnd )
            break;

        pad.orientClass = data[postDimPos + postDimSize - 1];
        DumpPadPostBlock( aComp, pad, data, postDimPos, postDimSize );

        aComp.pads.push_back( pad );

        chainPos = postDimPos + postDimSize;
    }

    aComp.padRegionEnd = chainPos;
}


// ---------------------------------------------------------------------------
// Footprint shape records
// ---------------------------------------------------------------------------

/// Shape count (int3) is at this offset past the end of the pad region.
static constexpr int FP_SHAPE_COUNT_OFFSET = 71;

/// Shape record data starts at this offset past the end of the pad region.
static constexpr int FP_SHAPE_DATA_OFFSET = 74;

/// Record size for v37 (legacy) format.
static constexpr int FP_SHAPE_RECORD_SIZE_V37 = 62;

/// Record size for v45+ format.
static constexpr int FP_SHAPE_RECORD_SIZE_V45 = 60;

/// v46+ chained-shape block layout used by some footprints (e.g. TO-92 in PCB_6).
static constexpr int FP_CHAIN_SHAPE_COUNT_OFFSET = 69;
static constexpr int FP_CHAIN_SHAPE_DATA_OFFSET = 72;
static constexpr int FP_CHAIN_SHAPE_RECORD_SIZE = 76;
static constexpr int FP_CHAIN_SHAPE_WIDTH_OFFSET = 37;
static constexpr int FP_CHAIN_SHAPE_TYPE_OFFSET = 41;
static constexpr int FP_CHAIN_SHAPE_X1_OFFSET = 44;
static constexpr int FP_CHAIN_SHAPE_Y1_OFFSET = 48;
static constexpr int FP_CHAIN_SHAPE_X2_OFFSET = 52;
static constexpr int FP_CHAIN_SHAPE_Y2_OFFSET = 56;
static constexpr int FP_CHAIN_SHAPE_X3_OFFSET = 60;
static constexpr int FP_CHAIN_SHAPE_Y3_OFFSET = 64;
static constexpr int FP_CHAIN_TYPE_LINE = 2;
static constexpr int FP_CHAIN_TYPE_ARC = 3;
static constexpr int FP_CHAIN_MOUNT_HOLE_OFFSET = 44;

/// Normalized coordinate range used by DipTrace shape records.
static constexpr int FP_SHAPE_NORM_RANGE = 10000;

/// Sentinel value for "use default line width" in shape width field.
static constexpr int FP_SHAPE_DEFAULT_WIDTH = -10000;

/// DipTrace footprint-graphic layer enum (int3 stored 5 bytes ahead of each shape's font block).
static constexpr int DT_FP_LAYER_TOP_SILK      = 0;
static constexpr int DT_FP_LAYER_TOP_ASSY      = 1;
static constexpr int DT_FP_LAYER_TOP_MASK      = 2;
static constexpr int DT_FP_LAYER_TOP_PASTE     = 3;
static constexpr int DT_FP_LAYER_TOP_KEEPOUT   = 9;
static constexpr int DT_FP_LAYER_TOP_COURTYARD = 16;
static constexpr int DT_FP_LAYER_TOP_OUTLINE   = 18;


// ---------------------------------------------------------------------------
// Footprint mount-hole records
// ---------------------------------------------------------------------------

/// Hole block header: int3(hole_count + 2) + byte(flag) + 4 * int4(0)
static constexpr size_t MOUNT_HOLE_HEADER_SIZE = 20;

/// Per-hole record: byte + byte + int4(x) + int4(y) + int4(outer_diam) + int4(drill_diam)
static constexpr size_t MOUNT_HOLE_RECORD_SIZE = 18;

/// Hole block terminator bytes: 0x00 0x00
static constexpr size_t MOUNT_HOLE_TERM_SIZE = 2;

/// Zero trailer following the hole block in observed v54 files.
static constexpr size_t MOUNT_HOLE_TRAILER_SIZE = 16;


static bool decodeMountHoleBlockAt( const uint8_t* aData, size_t aBlockStart,
                                    size_t aSearchEnd, std::vector<DT_MOUNT_HOLE>& aHoles )
{
    static constexpr int MAX_REASONABLE_DIM = 50000000;
    static constexpr int MAX_HOLE_COUNT = 64;

    if( aBlockStart + MOUNT_HOLE_HEADER_SIZE + MOUNT_HOLE_RECORD_SIZE
            + MOUNT_HOLE_TERM_SIZE + MOUNT_HOLE_TRAILER_SIZE > aSearchEnd )
    {
        return false;
    }

    int countField = ReadInt3At( aData, aBlockStart );
    int holeCount = countField - 2;

    if( holeCount <= 0 || holeCount > MAX_HOLE_COUNT )
        return false;

    uint8_t headerFlag = aData[aBlockStart + 3];

    if( headerFlag > 1 )
        return false;

    for( int i = 0; i < 4; i++ )
    {
        if( ReadInt4At( aData, aBlockStart + 4 + static_cast<size_t>( i ) * 4 ) != 0 )
            return false;
    }

    size_t holeStart = aBlockStart + MOUNT_HOLE_HEADER_SIZE;
    size_t holesBytes = static_cast<size_t>( holeCount ) * MOUNT_HOLE_RECORD_SIZE;
    size_t holeEnd = holeStart + holesBytes;
    size_t termPos = holeEnd;
    size_t trailerPos = termPos + MOUNT_HOLE_TERM_SIZE;

    if( trailerPos + MOUNT_HOLE_TRAILER_SIZE > aSearchEnd )
        return false;

    std::vector<DT_MOUNT_HOLE> parsedHoles;
    parsedHoles.reserve( static_cast<size_t>( holeCount ) );

    for( int hi = 0; hi < holeCount; hi++ )
    {
        size_t hp = holeStart + static_cast<size_t>( hi ) * MOUNT_HOLE_RECORD_SIZE;
        uint8_t holeFlagA = aData[hp];
        uint8_t holeFlagB = aData[hp + 1];

        if( holeFlagA != 0 || holeFlagB > 1 )
            return false;

        int x = ReadInt4At( aData, hp + 2 );
        int y = ReadInt4At( aData, hp + 6 );
        int outer = ReadInt4At( aData, hp + 10 );
        int drill = ReadInt4At( aData, hp + 14 );

        if( std::abs( x ) > MAX_REASONABLE_DIM || std::abs( y ) > MAX_REASONABLE_DIM
            || outer <= 0 || outer > MAX_REASONABLE_DIM
            || drill <= 0 || drill > MAX_REASONABLE_DIM
            || drill > outer )
        {
            return false;
        }

        DT_MOUNT_HOLE hole;
        hole.x = x;
        hole.y = y;
        hole.outerDiameter = outer;
        hole.drillDiameter = drill;
        parsedHoles.push_back( hole );
    }

    if( aData[termPos] != 0 || aData[termPos + 1] != 0 )
        return false;

    for( int i = 0; i < 4; i++ )
    {
        if( ReadInt4At( aData, trailerPos + static_cast<size_t>( i ) * 4 ) != 0 )
            return false;
    }

    aHoles = std::move( parsedHoles );
    return true;
}


void PCB_PARSER::FindMountHolesInRegion( DT_COMPONENT& aComp, size_t aRegionStart,
                                          size_t aRegionEnd )
{
    wxUnusedVar( aRegionStart );
    aComp.holes.clear();

    if( aComp.padRegionEnd == 0 || aComp.pads.empty() )
        return;

    const uint8_t* data = m_reader.GetData();
    size_t         dataSize = m_reader.GetFileSize();

    if( aRegionEnd > dataSize )
        aRegionEnd = dataSize;

    size_t searchEnd = aRegionEnd;

    if( searchEnd > COMPONENT_TAIL_SIZE )
        searchEnd -= COMPONENT_TAIL_SIZE;

    std::vector<size_t> candidates;

    if( aComp.padRegionEnd + FP_SHAPE_DATA_OFFSET <= searchEnd )
    {
        int recSize = ( m_version <= LEGACY_STRING_VERSION ) ? FP_SHAPE_RECORD_SIZE_V37
                                                             : FP_SHAPE_RECORD_SIZE_V45;
        int shapeCount = ReadInt3At( data, aComp.padRegionEnd + FP_SHAPE_COUNT_OFFSET );

        if( shapeCount > 0 && shapeCount <= 500 )
        {
            size_t shapeStart = aComp.padRegionEnd + FP_SHAPE_DATA_OFFSET;
            candidates.push_back( shapeStart + static_cast<size_t>( shapeCount ) * recSize );
        }
    }

    if( m_version >= FONT_BLOCK_SHAPE_VERSION
        && aComp.padRegionEnd + FP_CHAIN_SHAPE_DATA_OFFSET <= searchEnd )
    {
        int shapeCount = ReadInt3At( data, aComp.padRegionEnd + FP_CHAIN_SHAPE_COUNT_OFFSET );

        if( shapeCount >= 3 && shapeCount <= 200 )
        {
            size_t lastFrame = aComp.padRegionEnd + FP_CHAIN_SHAPE_DATA_OFFSET
                             + static_cast<size_t>( shapeCount - 1 )
                                       * FP_CHAIN_SHAPE_RECORD_SIZE;
            candidates.push_back( lastFrame + FP_CHAIN_MOUNT_HOLE_OFFSET );
        }
    }

    candidates.push_back( aComp.padRegionEnd );

    std::vector<size_t> uniqueCandidates;
    uniqueCandidates.reserve( candidates.size() );

    for( size_t pos : candidates )
    {
        if( std::find( uniqueCandidates.begin(), uniqueCandidates.end(), pos ) == uniqueCandidates.end() )
            uniqueCandidates.push_back( pos );
    }

    size_t decodedAt = 0;

    for( size_t pos : uniqueCandidates )
    {
        if( decodeMountHoleBlockAt( data, pos, searchEnd, aComp.holes ) )
        {
            decodedAt = pos;
            break;
        }
    }

    if( decodedAt == 0 )
        return;

    if( ShouldDumpComponentHeader( aComp.refdes ) )
    {
        wxLogTrace( traceDiptraceIo,
                    wxT( "DipTrace: mount-holes ref=%s count=%zu off=0x%06zX "
                         "padEnd=0x%06zX gap=%zu" ),
                    aComp.refdes, aComp.holes.size(), decodedAt, aComp.padRegionEnd,
                    decodedAt - aComp.padRegionEnd );
    }
}


void PCB_PARSER::FindShapesInRegion( DT_COMPONENT& aComp, size_t aRegionStart, size_t aRegionEnd )
{
    if( aComp.bboxWidth == 0 || aComp.bboxHeight == 0 )
        return;

    const uint8_t* data = m_reader.GetData();
    size_t dataSize = m_reader.GetFileSize();

    if( aRegionEnd > dataSize )
        aRegionEnd = dataSize;

    if( m_version >= FONT_BLOCK_SHAPE_VERSION )
    {
        FindShapesInFontBlocks( aComp, aRegionStart, aRegionEnd );

        if( aComp.shapes.empty() )
            FindShapesInChainedBlocks( aComp, aRegionStart, aRegionEnd );

        return;
    }

    // Use the chain-derived pad region end position for shape lookup
    if( aComp.padRegionEnd == 0 || aComp.pads.empty() )
    {
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: no pad region end for shape finding in '%s'" ),
                    aComp.patternName );
        return;
    }

    size_t padRegionEnd = aComp.padRegionEnd;

    if( padRegionEnd + FP_SHAPE_DATA_OFFSET > aRegionEnd )
    {
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: shape region beyond component bounds for '%s'" ),
                    aComp.patternName );
        return;
    }

    int recSize = ( m_version <= LEGACY_STRING_VERSION ) ? FP_SHAPE_RECORD_SIZE_V37
                                                         : FP_SHAPE_RECORD_SIZE_V45;

    int shapeCount = ReadInt3At( data, padRegionEnd + FP_SHAPE_COUNT_OFFSET );

    if( shapeCount <= 0 || shapeCount > 500 )
        return;

    size_t shapeStart = padRegionEnd + FP_SHAPE_DATA_OFFSET;
    size_t shapeEnd = shapeStart + static_cast<size_t>( shapeCount ) * recSize;

    if( shapeEnd > aRegionEnd )
        return;

    for( int i = 0; i < shapeCount; i++ )
    {
        size_t rp = shapeStart + static_cast<size_t>( i ) * recSize;

        DT_FP_SHAPE shape;
        shape.type = ReadInt3At( data, rp );

        if( shape.type == DT_SHAPE_END || shape.type == DT_SHAPE_EMPTY )
            continue;

        if( shape.type != DT_SHAPE_LINE && shape.type != DT_SHAPE_CIRCLE
            && shape.type != DT_SHAPE_ARC )
        {
            continue;
        }

        shape.x1 = ReadInt4At( data, rp + 3 );
        shape.y1 = ReadInt4At( data, rp + 7 );
        shape.x2 = ReadInt4At( data, rp + 11 );
        shape.y2 = ReadInt4At( data, rp + 15 );
        shape.midX = ReadInt4At( data, rp + 19 );
        shape.midY = ReadInt4At( data, rp + 23 );

        if( m_version <= LEGACY_STRING_VERSION )
        {
            shape.width = ReadInt4At( data, rp + 55 );
            shape.layer = ReadInt3At( data, rp + 59 );
        }
        else
        {
            shape.width = ReadInt4At( data, rp + 53 );
            shape.layer = ReadInt3At( data, rp + 57 );
        }

        aComp.shapes.push_back( shape );
    }
}


void PCB_PARSER::FindShapesInFontBlocks( DT_COMPONENT& aComp, size_t aRegionStart,
                                          size_t aRegionEnd )
{
    const uint8_t* data = m_reader.GetData();
    size_t         dataSize = m_reader.GetFileSize();

    if( aRegionEnd > dataSize )
        aRegionEnd = dataSize;

    auto isTahomaAt = [&]( size_t aPos ) -> bool
    {
        return aPos + TAHOMA_FONT_PATTERN_LEN <= aRegionEnd
               && std::memcmp( data + aPos, TAHOMA_FONT_PATTERN, TAHOMA_FONT_PATTERN_LEN ) == 0;
    };

    auto u16beAt = [&]( size_t aPos ) -> int
    {
        return ( static_cast<int>( data[aPos] ) << 8 ) | static_cast<int>( data[aPos + 1] );
    };

    // Deterministic field-walk of the self-describing font-shape blocks from the pad region
    // end. Each block carries its own point count and trailing label length, so its size is
    // computable; the run begins a fixed preamble after padRegionEnd and ends when the next
    // computed offset is no longer a Tahoma block (the trailing value/framing label record).
    std::vector<size_t> fontBlocks;
    bool                fieldWalked = false;

    if( aComp.padRegionEnd != 0
        && aComp.padRegionEnd + FONT_PREAMBLE_LABEL_OFFSET + 2 <= aRegionEnd )
    {
        size_t pre      = aComp.padRegionEnd;
        int    preLabel = u16beAt( pre + FONT_PREAMBLE_LABEL_OFFSET );

        if( preLabel >= 0 && preLabel <= 256 )
        {
            size_t bs = pre + FONT_PREAMBLE_FIXED_SIZE + 2 * static_cast<size_t>( preLabel );
            bool   walkOk = true;

            while( isTahomaAt( bs ) )
            {
                size_t body         = bs + TAHOMA_FONT_PATTERN_LEN + FONT_BLOCK_HEADER_SIZE;
                int    npts         = -1;
                bool   slot0InBounds = ( body + 3 <= aRegionEnd );

                for( int n = 0; n <= 3; n++ )
                {
                    if( body + static_cast<size_t>( n ) * 8 + 3 > aRegionEnd )
                        break;

                    int st = ReadInt3At( data, body + static_cast<size_t>( n ) * 8 );

                    if( st == 0 || st == 1 || st == 2 || st == 3 || st == 5 || st == 6 || st == 7
                        || st == 700 )
                    {
                        npts = n;
                        break;
                    }
                }

                // A block whose leading slot carries shape-type 0 (no coordinate points) or no
                // shape-type code at all is the trailing value/reference text label that closes the
                // silk run. Record it and stop; its size never needs computing. Only a block
                // truncated past the region edge, whose structure can no longer be read, is an
                // abnormal break that must fall back to the scan.
                if( npts <= 0 )
                {
                    if( slot0InBounds )
                        fontBlocks.push_back( bs );
                    else
                        walkOk = false;

                    break;
                }

                // npts >= 1: a real coordinate shape block. Advancing past it needs a valid trailing
                // label length; if that field is out of range the block size is uncomputable and the
                // walk would skip the following shapes, so treat it as a divergence.
                size_t lcPos = body + static_cast<size_t>( npts ) * 8 + 3 + FONT_BLOCK_TRAILER_SIZE;

                if( lcPos + 2 > aRegionEnd )
                {
                    walkOk = false;
                    break;
                }

                int lc = u16beAt( lcPos );

                if( lc < 0 || lc > 256 )
                {
                    walkOk = false;
                    break;
                }

                // Record the block only after its full size is validated, so an abnormal break never
                // leaves a truncated entry behind.
                fontBlocks.push_back( bs );

                bs += FONT_BLOCK_FIXED_SIZE + static_cast<size_t>( 8 * npts )
                      + 2 * static_cast<size_t>( lc );
            }

            // Only certify the field-walk when it terminated cleanly. A mid-run break means the
            // block structure diverged from the model, so fall back to the Tahoma scan rather than
            // import a truncated silk run with the gate still reading zero.
            fieldWalked = walkOk && !fontBlocks.empty();
        }
    }

    if( !fieldWalked )
    {
        // Recovery fallback: locate the font blocks by scanning for the Tahoma font literal.
        // Counts toward the determinism gate.
        fontBlocks = FindAllBoundaries( data, dataSize, TAHOMA_FONT_PATTERN, TAHOMA_FONT_PATTERN_LEN,
                                        aRegionStart, aRegionEnd );
    }

    if( fontBlocks.empty() )
        return;

    size_t shapesBefore = aComp.shapes.size();

    for( size_t bi = 0; bi < fontBlocks.size(); bi++ )
    {
        size_t blockStart = fontBlocks[bi];
        size_t nextBoundary = ( bi + 1 < fontBlocks.size() ) ? fontBlocks[bi + 1]
                                                              : aRegionEnd;

        size_t headerEnd = blockStart + TAHOMA_FONT_PATTERN_LEN + FONT_BLOCK_HEADER_SIZE;

        if( headerEnd + 19 > nextBoundary )
            continue;

        size_t metaStart = blockStart + TAHOMA_FONT_PATTERN_LEN;
        int lineWidth = ReadInt4At( data, metaStart + 18 );

        // The footprint-graphic layer is a small int3 enum stored 5 bytes ahead of the block's
        // "Tahoma" font literal (0 Top Silk, 1 Top Assembly, 2 Top Mask, 3 Top Paste,
        // 16 Top Courtyard, 18 Top Outline). The metaStart+22 field is unrelated and constant.
        int layerIdx = ( blockStart >= 5 ) ? ReadInt3At( data, blockStart - 5 ) : 0;

        size_t bodyPos = headerEnd;

        int x1 = ReadInt4At( data, bodyPos );
        int y1 = ReadInt4At( data, bodyPos + 4 );
        int x2 = ReadInt4At( data, bodyPos + 8 );
        int y2 = ReadInt4At( data, bodyPos + 12 );

        if( x1 < -FP_SHAPE_NORM_RANGE || x1 > FP_SHAPE_NORM_RANGE
            || y1 < -FP_SHAPE_NORM_RANGE || y1 > FP_SHAPE_NORM_RANGE
            || x2 < -FP_SHAPE_NORM_RANGE || x2 > FP_SHAPE_NORM_RANGE
            || y2 < -FP_SHAPE_NORM_RANGE || y2 > FP_SHAPE_NORM_RANGE )
        {
            continue;
        }

        if( x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0 )
            continue;

        int shapeType = ReadInt3At( data, bodyPos + 16 );

        DT_FP_SHAPE shape;
        shape.x1 = x1;
        shape.y1 = y1;
        shape.x2 = x2;
        shape.y2 = y2;
        shape.width = lineWidth;
        shape.layer = layerIdx;

        // v46+ font-block shape-type codes: 0 = axis-aligned rectangle, 1/5 = line,
        // 2/6 = arc, 3 = circle, 700 = filled obround marker (polarity / pin-1 dot).
        if( shapeType == 0 )
        {
            shape.type = DT_SHAPE_RECT;
        }
        else if( shapeType == 1 || shapeType == 5 )
        {
            shape.type = DT_SHAPE_LINE;
        }
        else if( shapeType == 3 )
        {
            shape.type = DT_SHAPE_CIRCLE;
        }
        else if( shapeType == 2 || shapeType == DT_SHAPE_ARC )
        {
            shape.type = DT_SHAPE_ARC;

            if( bodyPos + 43 <= nextBoundary )
            {
                shape.midX = ReadInt4At( data, bodyPos + 35 );
                shape.midY = ReadInt4At( data, bodyPos + 39 );
            }
            else
            {
                continue;
            }
        }
        else if( shapeType == DT_SHAPE_FILLOBROUND )
        {
            shape.type = DT_SHAPE_FILLOBROUND;
        }
        else
        {
            continue;
        }

        aComp.shapes.push_back( shape );
    }

    if( !fieldWalked && aComp.shapes.size() > shapesBefore )
        m_shapeLocatorScans++;
}


void PCB_PARSER::FindShapesInChainedBlocks( DT_COMPONENT& aComp, size_t aRegionStart,
                                             size_t aRegionEnd )
{
    wxUnusedVar( aRegionStart );

    if( aComp.padRegionEnd == 0 || aComp.pads.empty() )
        return;

    const uint8_t* data = m_reader.GetData();
    size_t         dataSize = m_reader.GetFileSize();

    if( aRegionEnd > dataSize )
        aRegionEnd = dataSize;

    if( aComp.padRegionEnd + FP_CHAIN_SHAPE_DATA_OFFSET > aRegionEnd )
        return;

    int shapeCount = ReadInt3At( data, aComp.padRegionEnd + FP_CHAIN_SHAPE_COUNT_OFFSET );

    // Observed range in viewer examples: 4..13 (count includes a non-shape head/tail record).
    if( shapeCount < 3 || shapeCount > 200 )
        return;

    size_t shapeStart = aComp.padRegionEnd + FP_CHAIN_SHAPE_DATA_OFFSET;
    size_t shapeEnd = shapeStart + static_cast<size_t>( shapeCount ) * FP_CHAIN_SHAPE_RECORD_SIZE;

    if( shapeEnd > aRegionEnd )
        return;

    std::vector<DT_FP_SHAPE> decoded;
    decoded.reserve( static_cast<size_t>( shapeCount ) );

    auto inNormRange = []( int aVal ) -> bool
    {
        return aVal >= -FP_SHAPE_NORM_RANGE && aVal <= FP_SHAPE_NORM_RANGE;
    };

    // Record 0 and the last record are framing entries; real graphics are in 1..N-2.
    for( int i = 1; i + 1 < shapeCount; i++ )
    {
        size_t rp = shapeStart + static_cast<size_t>( i ) * FP_CHAIN_SHAPE_RECORD_SIZE;
        int rawType = ReadInt3At( data, rp + FP_CHAIN_SHAPE_TYPE_OFFSET );
        int width = ReadInt4At( data, rp + FP_CHAIN_SHAPE_WIDTH_OFFSET );
        int x1 = ReadInt4At( data, rp + FP_CHAIN_SHAPE_X1_OFFSET );
        int y1 = ReadInt4At( data, rp + FP_CHAIN_SHAPE_Y1_OFFSET );
        int x2 = ReadInt4At( data, rp + FP_CHAIN_SHAPE_X2_OFFSET );
        int y2 = ReadInt4At( data, rp + FP_CHAIN_SHAPE_Y2_OFFSET );

        if( !inNormRange( x1 ) || !inNormRange( y1 ) || !inNormRange( x2 ) || !inNormRange( y2 ) )
            continue;

        DT_FP_SHAPE shape;
        shape.width = width;
        // Chained records are footprint-local silk graphics on the component side.
        shape.layer = ( aComp.layer == 1 ) ? 3 : 2;

        if( rawType == FP_CHAIN_TYPE_LINE )
        {
            shape.type = DT_SHAPE_LINE;
            shape.x1 = x1;
            shape.y1 = y1;
            shape.x2 = x2;
            shape.y2 = y2;
        }
        else if( rawType == FP_CHAIN_TYPE_ARC )
        {
            int x3 = ReadInt4At( data, rp + FP_CHAIN_SHAPE_X3_OFFSET );
            int y3 = ReadInt4At( data, rp + FP_CHAIN_SHAPE_Y3_OFFSET );

            if( !inNormRange( x3 ) || !inNormRange( y3 ) )
                continue;

            shape.type = DT_SHAPE_ARC;
            shape.x1 = x1;
            shape.y1 = y1;
            shape.midX = x2;
            shape.midY = y2;
            shape.x2 = x3;
            shape.y2 = y3;
        }
        else
        {
            continue;
        }

        decoded.push_back( shape );
    }

    if( !decoded.empty() )
        aComp.shapes.insert( aComp.shapes.end(), decoded.begin(), decoded.end() );
}


// ---------------------------------------------------------------------------
// Component tail (text positioning)
// ---------------------------------------------------------------------------

void PCB_PARSER::ParseComponentTail( DT_COMPONENT& aComp, size_t aRegionEnd )
{
    if( aRegionEnd < COMPONENT_TAIL_SIZE )
        return;

    const uint8_t* data = m_reader.GetData();

    size_t savedOffset = m_reader.GetOffset();
    bool parsed = false;

    auto tryParseTailAt = [&]( size_t aTailStart ) -> bool
    {
        if( aTailStart + COMPONENT_TAIL_SIZE > m_reader.GetFileSize() )
            return false;

        if( std::memcmp( data + aTailStart, COMPONENT_TAIL_PATTERN, COMPONENT_TAIL_PATTERN_LEN ) != 0 )
            return false;

        // Tail layout (37 bytes total, confirmed across v37-v54):
        //   +0:  int3(0)    constant
        //   +3:  int4(0)    constant
        //   +7:  int4(0)    constant
        //   +11: int4(0)    constant
        //   +15: int4(0)    constant
        //   +19: byte       text side flag
        //   +20: int3       text visibility (0 = visible, -1 = hidden)
        //   +23: byte       text side flag 2 (mirrors +19)
        //   +24: int3       ordering index
        //   +27: int4       refdes Y offset (DipTrace units)
        //   +31: int4       value Y offset (DipTrace units)
        //   +35: byte       has-offset flag
        //   +36: byte(0)    constant

        try
        {
            m_reader.SetOffset( aTailStart + 11 );

            int check1 = m_reader.ReadInt4();
            int check2 = m_reader.ReadInt4();

            if( check1 != 0 || check2 != 0 )
                return false;

            uint8_t sideFlag1 = m_reader.ReadByte();
            int visibility = m_reader.ReadInt3();
            uint8_t sideFlag2 = m_reader.ReadByte();
            int orderIdx = m_reader.ReadInt3();

            int refdesYOffset = m_reader.ReadInt4();
            int valueYOffset = m_reader.ReadInt4();
            uint8_t hasOffset = m_reader.ReadByte();
            uint8_t tailTerm = m_reader.ReadByte();

            if( visibility != 0 && visibility != -1 )
                return false;

            if( hasOffset > 1 || tailTerm != 0 )
                return false;

            if( std::abs( refdesYOffset ) > 50000000 || std::abs( valueYOffset ) > 50000000 )
                return false;

            aComp.refdesYOffset = refdesYOffset;
            aComp.valueYOffset = valueYOffset;
            aComp.refdesVisible = ( visibility != -1 );
            aComp.valueVisible = ( visibility != -1 );
            aComp.hasTailData = true;

            // The component side is carried by the tail mirror flags, not the
            // header flag byte (which is always 0 in v49+ files). Both flags are
            // set together for bottom-side parts. Only promote to bottom here;
            // never override a header-determined bottom back to top, so older
            // files that did populate the header flag are never regressed.
            if( sideFlag1 == 1 && sideFlag2 == 1 )
                aComp.layer = 1;

            DumpComponentTail( aComp, data, aTailStart, visibility, sideFlag1, sideFlag2,
                               orderIdx, refdesYOffset, valueYOffset, hasOffset, tailTerm );
            return true;
        }
        catch( const IO_ERROR& )
        {
            return false;
        }
    };

    size_t canonicalTailStart = aRegionEnd - COMPONENT_TAIL_SIZE;
    parsed = tryParseTailAt( canonicalTailStart );

    if( !parsed && ShouldDumpComponentHeader( aComp.refdes ) )
    {
        size_t dumpLen = std::min<size_t>( 96, aRegionEnd );
        size_t dumpStart = aRegionEnd - dumpLen;
        wxString hex = BytesToHex( data + dumpStart, dumpLen );

        wxLogTrace( traceDiptraceIo,
                    wxT( "DipTrace: component-tail-missing ref=%s regionEnd=0x%06zX "
                         "tailHexStart=0x%06zX len=%lu hex=[%s]" ),
                    aComp.refdes, aRegionEnd, dumpStart, static_cast<unsigned long>( dumpLen ), hex );
    }

    m_reader.SetOffset( savedOffset );
}


// ---------------------------------------------------------------------------
// Post-component sections
// ---------------------------------------------------------------------------

void PCB_PARSER::ParsePostComponentSections()
{
    size_t postComp = m_reader.GetOffset();

    if( postComp > m_componentUpperBound )
        postComp = m_componentUpperBound;

    size_t projLibOffset = m_reader.FindString( wxT( "Project Libraries" ), 0, 0 );
    size_t gapEnd = ( projLibOffset != NOT_FOUND ) ? projLibOffset : m_reader.GetFileSize();

    // Each post-component section is located by its own structural anchor and counts a scan-locate
    // (SectionLocatorScans) only when that anchor is absent: the board TEXT section by its
    // nine-byte zero separator + record count + 01 00 flags + valid record walk (here);
    // the NET section by the record count five bytes ahead of the index-0 sentinel; the ZONE
    // section by its font preamble. Text records are only parsed after the full structural
    // validation below, so a located text section is always anchored.
    FindAndParseTextObjects( postComp, gapEnd );
    FindAndParseNets( postComp, gapEnd );
    FindAndParseZones( postComp, gapEnd );
}


void PCB_PARSER::FindAndParseTextObjects( size_t aSearchStart, size_t aSearchEnd )
{
    size_t pos = aSearchStart;
    auto textRecordsLookValid =
            [&]( size_t aRecordStart, int aCount, size_t aSectionEnd ) -> bool
            {
                size_t savedOffset = m_reader.GetOffset();
                m_reader.SetOffset( aRecordStart );

                try
                {
                    for( int ti = 0; ti < aCount; ti++ )
                    {
                        m_reader.ReadInt3();    // type_a
                        m_reader.ReadByte();    // flag_a
                        m_reader.ReadInt3();    // type_b
                        m_reader.ReadInt3();    // field_a
                        m_reader.ReadInt3();    // field_b
                        m_reader.ReadInt3();    // field_c
                        m_reader.ReadInt3();    // field_d
                        m_reader.ReadInt3();    // field_e

                        ReadColorPacked( m_reader );
                        ReadColorPacked( m_reader );
                        ReadColorPacked( m_reader );

                        int lineWidth = m_reader.ReadInt4();
                        int layer = m_reader.ReadInt3();

                        if( lineWidth < 0 || lineWidth > 10000000 || layer < -100 || layer > 100 )
                            throw std::runtime_error( "invalid text metrics" );

                        m_reader.ReadInt4();    // x1
                        m_reader.ReadInt4();    // y1
                        m_reader.ReadInt4();    // x2
                        m_reader.ReadInt4();    // y2

                        m_reader.ReadString();
                        m_reader.ReadString();

                        m_reader.ReadByte();    // separator
                        m_reader.ReadInt3();    // field_pf_1
                        m_reader.ReadByte();    // flag_pf
                        m_reader.ReadInt4();    // text_offset_1
                        m_reader.ReadInt4();    // text_offset_2
                        m_reader.ReadInt3();    // record_index
                        m_reader.ReadByte();    // end_flag

                        if( ti < aCount - 1 )
                        {
                            m_reader.ReadByte();
                            m_reader.ReadByte();
                        }

                        if( m_reader.GetOffset() > aSectionEnd )
                            throw std::runtime_error( "text section overrun" );
                    }

                    m_reader.SetOffset( savedOffset );
                    return true;
                }
                catch( const std::exception& )
                {
                    m_reader.SetOffset( savedOffset );
                    return false;
                }
            };

    while( pos + 20 < aSearchEnd )
    {
        size_t idx = m_reader.FindPattern( TEXT_SECTION_ZEROS, 9, pos, aSearchEnd );

        if( idx == NOT_FOUND )
            break;

        size_t countPos = idx + 9;

        if( countPos + 5 > aSearchEnd )
            break;

        const uint8_t* data = m_reader.GetData();
        const uint8_t* b = data + countPos;
        int countVal = static_cast<int>( ( static_cast<int>( b[0] ) << 16 )
                                        | ( static_cast<int>( b[1] ) << 8 )
                                        | static_cast<int>( b[2] ) ) - INT3_BIAS;

        if( countVal >= 1 && countVal <= 1000 )
        {
            uint8_t flag1 = data[countPos + 3];
            uint8_t flag2 = data[countPos + 4];

            if( flag1 == 1 && flag2 == 0 )
            {
                size_t recordStart = countPos + 5;

                if( textRecordsLookValid( recordStart, countVal, aSearchEnd ) )
                {
                    m_reader.SetOffset( recordStart );
                    ParseTextRecords( countVal );
                    return;
                }
            }
        }

        pos = idx + 1;
    }
}


void PCB_PARSER::ParseTextRecords( int aCount )
{
    for( int ti = 0; ti < aCount; ti++ )
    {
        try
        {
            DT_TEXT_OBJECT text;

            // Header fields
            m_reader.ReadInt3();    // type_a
            m_reader.ReadByte();    // flag_a
            m_reader.ReadInt3();    // type_b
            m_reader.ReadInt3();    // field_a
            m_reader.ReadInt3();    // field_b
            m_reader.ReadInt3();    // field_c
            m_reader.ReadInt3();    // field_d
            m_reader.ReadInt3();    // field_e

            // Colors
            text.color = ReadColorPacked( m_reader );
            ReadColorPacked( m_reader );   // color2
            ReadColorPacked( m_reader );   // color3

            // Line/Layer
            text.lineWidth = m_reader.ReadInt4();
            text.layer = m_reader.ReadInt3();

            // Geometry
            text.x1 = m_reader.ReadInt4();
            text.y1 = m_reader.ReadInt4();
            text.x2 = m_reader.ReadInt4();
            text.y2 = m_reader.ReadInt4();

            // Content
            text.text = m_reader.ReadString();
            text.fontName = m_reader.ReadString();

            // Post-font fields
            m_reader.ReadByte();    // separator
            m_reader.ReadInt3();    // field_pf_1
            m_reader.ReadByte();    // flag_pf
            m_reader.ReadInt4();    // text_offset_1
            m_reader.ReadInt4();    // text_offset_2
            m_reader.ReadInt3();    // record_index
            m_reader.ReadByte();    // end_flag

            // Inter-record separator bytes (observed 0x01 0x00), absent after the last record.
            if( ti < aCount - 1 )
            {
                m_reader.ReadByte();
                m_reader.ReadByte();
            }

            m_textObjects.push_back( text );
        }
        catch( const IO_ERROR& e )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "DipTrace: text object [%d] parse error: %s" ),
                    ti, e.What() ) );
        }
    }
}


// ---------------------------------------------------------------------------
// Net name parsing
// ---------------------------------------------------------------------------

void PCB_PARSER::FindAndParseNets( size_t aSearchStart, size_t aSearchEnd )
{
    m_nets.clear();
    m_trackChains.clear();
    m_routingAnchorsByNet.clear();
    m_routingAnchorCacheBuilt = false;

    // Net records in the .dip binary format are preceded by a 9-byte sentinel:
    //   int3(0) int3(-1) int3(-1)
    // After the sentinel, the record contains:
    //   int3(net_index)  int3(0)  int4(trace_width)  int4(field)  string(net_name)
    //
    // We validate each match by checking that the second int3 is in the observed
    // route-flag range, the widths are bounded, and the stored net-name string
    // parses.  DipTrace permits empty stored net names.

    std::vector<size_t> sentinelOffsets = FindAllBoundaries(
            m_reader.GetData(), m_reader.GetFileSize(),
            NET_SENTINEL, NET_SENTINEL_LEN,
            aSearchStart, aSearchEnd );

    static constexpr int MAX_NETS = 10000;
    static constexpr int MAX_REASONABLE_WIDTH = 5000000;  // 50mm in 10nm units

    size_t firstNetSentinel = 0;

    for( size_t sentOff : sentinelOffsets )
    {
        if( static_cast<int>( m_nets.size() ) >= MAX_NETS )
            break;

        size_t pos = sentOff + NET_SENTINEL_LEN;

        // Need at least: int3 + int3 + int4 + int4 + 2 bytes (empty string)
        if( pos + 3 + 3 + 4 + 4 + 2 > m_reader.GetFileSize() )
            continue;

        m_reader.SetOffset( pos );

        DT_NET net;
        bool acceptedNetRecord = false;

        try
        {
            int netIndex = m_reader.ReadInt3();
            int field0 = m_reader.ReadInt3();
            int width1 = m_reader.ReadInt4();
            int width2 = m_reader.ReadInt4();

            // Validate: net index must be non-negative. field0 is a small
            // per-net mode/route flag (observed values include 0 and 3), so
            // accept a bounded range instead of hardcoding zero.
            if( field0 < 0 || field0 > 10 || netIndex < 0 || netIndex >= MAX_NETS )
                continue;

            if( width1 < 0 || width1 > MAX_REASONABLE_WIDTH
                || width2 < 0 || width2 > MAX_REASONABLE_WIDTH )
            {
                continue;
            }

            bool expectedNetIndex = netIndex == static_cast<int>( m_nets.size() );
            acceptedNetRecord = expectedNetIndex;
            wxString name;

            if( !m_reader.TryReadString( name ) )
            {
                if( expectedNetIndex )
                {
                    THROW_IO_ERROR( wxString::Format(
                            _( "DipTrace import: invalid net name for net index %d at "
                               "offset 0x%06zX." ),
                            netIndex, m_reader.GetOffset() ) );
                }

                continue;
            }

            net.index = netIndex;
            net.name = name;
            net.traceWidth = width1;

            if( ShouldDumpNets() )
            {
                wxLogTrace( traceDiptraceIo, wxT( "DipTrace: net idx=%d name=%s width1=%d width2=%d" ), netIndex, name,
                            width1, width2 );
            }
        }
        catch( const IO_ERROR& )
        {
            if( acceptedNetRecord )
                throw;

            // Skip malformed false-positive sentinel records.
            continue;
        }

        if( firstNetSentinel == 0 )
            firstNetSentinel = sentOff;

        ParseNetRouting( net );

        m_nets.push_back( std::move( net ) );
    }

    // The net section is field-located when the first accepted net record (index 0) is immediately
    // preceded by its record count: int3(netCount) sits at firstSentinel - 5 across all observed
    // versions (some prepend an int3(0) and a 01 00 flag pair, which we do not require). That count
    // is the section's deterministic structural anchor; the sequential net-index walk above rejects
    // the false sentinel hits in the component region. Only count a scan-locate when it is absent.
    bool netSectionFieldAnchored = false;

    if( firstNetSentinel >= 5 && !m_nets.empty() )
    {
        netSectionFieldAnchored = ReadInt3At( m_reader.GetData(), firstNetSentinel - 5 )
                                  == static_cast<int>( m_nets.size() );
    }

    if( !m_nets.empty() && !netSectionFieldAnchored )
        m_sectionLocatorScans++;

    size_t totalNodes = 0;
    size_t viaStyleNodes = 0;
    size_t routeFlagNodes = 0;
    size_t viaStyleAndRouteFlagNodes = 0;
    size_t routeFlagOnlyNodes = 0;
    size_t routeMode0Nodes = 0;
    size_t routeMode1Nodes = 0;
    size_t routeMode3Nodes = 0;
    size_t routeModeOtherNodes = 0;

    for( const DT_TRACK_CHAIN& chain : m_trackChains )
    {
        for( const DT_TRACK_NODE& node : chain.nodes )
        {
            totalNodes++;

            bool hasStyle = node.viaStyleIdx >= 0;
            bool hasFlag = node.routeFlag != 0;

            if( hasStyle )
                viaStyleNodes++;

            if( hasFlag )
                routeFlagNodes++;

            if( hasStyle && hasFlag )
                viaStyleAndRouteFlagNodes++;
            else if( hasFlag )
                routeFlagOnlyNodes++;

            switch( node.routeMode )
            {
            case 0:
                routeMode0Nodes++;
                break;

            case 1:
                routeMode1Nodes++;
                break;

            case 3:
                routeMode3Nodes++;
                break;

            default:
                routeModeOtherNodes++;
                break;
            }
        }
    }

    wxLogTrace( traceDiptraceIo,
                wxT( "DipTrace: parsed %zu net names, %zu track chains, %zu nodes "
                     "(viaStyle=%zu, routeFlag=%zu, both=%zu, flagOnly=%zu, "
                     "routeMode[0]=%zu, routeMode[1]=%zu, routeMode[3]=%zu, routeMode[other]=%zu)" ),
                m_nets.size(), m_trackChains.size(), totalNodes, viaStyleNodes, routeFlagNodes,
                viaStyleAndRouteFlagNodes, routeFlagOnlyNodes, routeMode0Nodes, routeMode1Nodes, routeMode3Nodes,
                routeModeOtherNodes );
}


// ---------------------------------------------------------------------------
// Net routing (track chains and vias)
// ---------------------------------------------------------------------------

void PCB_PARSER::ParseNetRouting( DT_NET& aNet )
{
    // After the net name string, the record body contains:
    //   int4(via_od_default) int4(via_drill_default) byte(1) + 7 zero bytes
    //   int3(0) int3(pad_ref_count) + pairs of int3(comp_idx)+int3(pad_idx)
    //   variable routing metadata
    //   chain headers: pattern 00 00 00 0F 42 3F + int3(chain_idx) + int3(node_count)
    //   node_count * 41-byte track node records per chain
    //
    // We scan forward from the current offset looking for chain header patterns
    // within a bounded region (up to the next net sentinel or 64KB, whichever comes first).

    size_t startPos = m_reader.GetOffset();

    // Determine scan boundary: next net sentinel or capped distance
    static constexpr size_t MAX_NET_BODY = 65536;
    size_t scanEnd = std::min( startPos + MAX_NET_BODY, m_reader.GetFileSize() );

    // Find the next net sentinel to avoid reading into the next net
    size_t nextSentinel = m_reader.FindPattern( NET_SENTINEL, NET_SENTINEL_LEN,
                                                 startPos + 20, scanEnd );

    if( nextSentinel != std::string::npos )
        scanEnd = nextSentinel;

    const uint8_t* data = m_reader.GetData();
    size_t fileSize = m_reader.GetFileSize();

    size_t chainScanStart = startPos;
    static constexpr int MAX_REASONABLE_VIA_DIM = 5000000;   // 50mm
    static constexpr int MAX_PADREFS_PER_COMPONENT = 12;

    int componentCount = static_cast<int>( m_components.size() );
    int maxReasonablePadRefs = std::max( 512, componentCount * MAX_PADREFS_PER_COMPONENT );

    // Parse optional net-level routing preamble:
    // [via OD, via drill, marker, 7x0, int3(0), int3(padRefCount), pad refs...]
    if( startPos + 22 <= scanEnd )
    {
        int viaOuterDefault = ReadInt4At( data, startPos );
        int viaDrillDefault = ReadInt4At( data, startPos + 4 );
        uint8_t marker = data[startPos + 8];
        bool zeroBlock = std::all_of( data + startPos + 9, data + startPos + 16,
                                      []( uint8_t b ) { return b == 0; } );
        int separator = ReadInt3At( data, startPos + 16 );
        int padRefCount = ReadInt3At( data, startPos + 19 );

        if( marker <= 1 && zeroBlock && separator == 0 && padRefCount >= 0
            && padRefCount <= maxReasonablePadRefs )
        {
            size_t refsStart = startPos + 22;
            size_t refsEnd = refsStart + static_cast<size_t>( padRefCount ) * 6;

            if( refsEnd <= scanEnd )
            {
                std::vector<DT_PAD_REF> parsedRefs;
                parsedRefs.reserve( static_cast<size_t>( padRefCount ) );
                int rangeHitsBase0 = 0;
                int rangeHitsBase1 = 0;

                for( int i = 0; i < padRefCount; i++ )
                {
                    size_t refPos = refsStart + static_cast<size_t>( i ) * 6;
                    int compIndex = ReadInt3At( data, refPos );
                    int padIndex = ReadInt3At( data, refPos + 3 );

                    if( compIndex >= 0 && padIndex > 0 )
                    {
                        parsedRefs.push_back( { compIndex, padIndex } );

                        if( compIndex >= 0 && compIndex < componentCount )
                            rangeHitsBase0++;

                        if( compIndex >= 1 && compIndex <= componentCount )
                            rangeHitsBase1++;
                    }
                }

                int requiredRangeHits = std::min( 4, static_cast<int>( parsedRefs.size() ) );
                bool plausibleRefs = parsedRefs.empty()
                                   || std::max( rangeHitsBase0, rangeHitsBase1 ) >= requiredRangeHits;

                if( plausibleRefs )
                {
                    if( viaOuterDefault > 0 && viaOuterDefault <= MAX_REASONABLE_VIA_DIM )
                        aNet.defaultViaOuterDiam = viaOuterDefault;

                    if( viaDrillDefault > 0 && viaDrillDefault <= MAX_REASONABLE_VIA_DIM )
                        aNet.defaultViaDrillDiam = viaDrillDefault;

                    aNet.padRefs = std::move( parsedRefs );
                    chainScanStart = refsEnd;
                }
            }
        }
    }

    // Scan for chain headers within this net body
    size_t pos = chainScanStart;

    while( pos + CHAIN_HEADER_LEN + 6 + TRACK_NODE_SIZE <= scanEnd )
    {
        size_t chainPos = m_reader.FindPattern( CHAIN_HEADER, CHAIN_HEADER_LEN, pos, scanEnd );

        if( chainPos == std::string::npos )
            break;

        size_t headerStart = chainPos + CHAIN_HEADER_LEN;

        if( headerStart + 6 > fileSize )
            break;

        const uint8_t* h = data + headerStart;
        int chainIdx = ( ( static_cast<int>( h[0] ) << 16 )
                       | ( static_cast<int>( h[1] ) << 8 )
                       | static_cast<int>( h[2] ) ) - INT3_BIAS;
        int nodeCount = ( ( static_cast<int>( h[3] ) << 16 )
                        | ( static_cast<int>( h[4] ) << 8 )
                        | static_cast<int>( h[5] ) ) - INT3_BIAS;

        auto firstNodeLooksPlausible = [&]() -> bool
        {
            size_t firstNode = headerStart + 6;

            if( firstNode + TRACK_NODE_SIZE > scanEnd )
                return false;

            const uint8_t* n = data + firstNode;

            int x = static_cast<int>(
                    ( static_cast<unsigned int>( n[0] ) << 24 )
                  | ( static_cast<unsigned int>( n[1] ) << 16 )
                  | ( static_cast<unsigned int>( n[2] ) << 8 )
                  | static_cast<unsigned int>( n[3] ) ) - INT4_BIAS;

            int y = static_cast<int>(
                    ( static_cast<unsigned int>( n[4] ) << 24 )
                  | ( static_cast<unsigned int>( n[5] ) << 16 )
                  | ( static_cast<unsigned int>( n[6] ) << 8 )
                  | static_cast<unsigned int>( n[7] ) ) - INT4_BIAS;

            int layer = ( ( static_cast<int>( n[8] ) << 16 )
                        | ( static_cast<int>( n[9] ) << 8 )
                        | static_cast<int>( n[10] ) ) - INT3_BIAS;

            int width = static_cast<int>(
                    ( static_cast<unsigned int>( n[14] ) << 24 )
                  | ( static_cast<unsigned int>( n[15] ) << 16 )
                  | ( static_cast<unsigned int>( n[16] ) << 8 )
                  | static_cast<unsigned int>( n[17] ) ) - INT4_BIAS;

            int viaStyleIdx = ( ( static_cast<int>( n[27] ) << 16 )
                              | ( static_cast<int>( n[28] ) << 8 )
                              | static_cast<int>( n[29] ) ) - INT3_BIAS;

            int routeMode = ( ( static_cast<int>( n[37] ) << 16 )
                            | ( static_cast<int>( n[38] ) << 8 )
                            | static_cast<int>( n[39] ) ) - INT3_BIAS;

            return x > -100000000 && x < 100000000
                   && y > -100000000 && y < 100000000
                   && layer >= 0 && layer <= 50
                   && width > 0 && width <= 5000000
                   && viaStyleIdx >= -1 && viaStyleIdx <= 10000
                   && routeMode >= 0 && routeMode <= 10
                   && n[40] <= 10;
        };

        if( chainIdx < 0 || nodeCount < 1 || nodeCount > 10000 )
        {
            if( chainIdx >= 0 && firstNodeLooksPlausible() )
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "DipTrace import: invalid route-chain node count %d for net '%s' "
                           "at offset 0x%06zX." ),
                        nodeCount, aNet.name, headerStart + 3 ) );
            }

            pos = chainPos + 1;
            continue;
        }

        size_t nodesStart = headerStart + 6;
        size_t nodesEnd = nodesStart + static_cast<size_t>( nodeCount ) * TRACK_NODE_SIZE;

        if( nodesEnd > scanEnd )
        {
            if( firstNodeLooksPlausible() )
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "DipTrace import: route-chain node count %d for net '%s' overruns "
                           "record at offset 0x%06zX." ),
                        nodeCount, aNet.name, headerStart + 3 ) );
            }

            pos = chainPos + 1;
            continue;
        }

        DT_TRACK_CHAIN chain;
        chain.netIndex = aNet.index;
        chain.nodes.reserve( nodeCount );

        bool valid = true;

        for( int i = 0; i < nodeCount; i++ )
        {
            const uint8_t* n = data + nodesStart + static_cast<size_t>( i ) * TRACK_NODE_SIZE;

            DT_TRACK_NODE node;

            // +0: int4 X
            node.x = static_cast<int>(
                    ( static_cast<unsigned int>( n[0] ) << 24 )
                  | ( static_cast<unsigned int>( n[1] ) << 16 )
                  | ( static_cast<unsigned int>( n[2] ) << 8 )
                  | static_cast<unsigned int>( n[3] ) ) - INT4_BIAS;

            // +4: int4 Y
            node.y = static_cast<int>(
                    ( static_cast<unsigned int>( n[4] ) << 24 )
                  | ( static_cast<unsigned int>( n[5] ) << 16 )
                  | ( static_cast<unsigned int>( n[6] ) << 8 )
                  | static_cast<unsigned int>( n[7] ) ) - INT4_BIAS;

            // +8: int3 layer
            node.layer = ( ( static_cast<int>( n[8] ) << 16 )
                         | ( static_cast<int>( n[9] ) << 8 )
                         | static_cast<int>( n[10] ) ) - INT3_BIAS;

            // +14: int4 track width
            node.width = static_cast<int>(
                    ( static_cast<unsigned int>( n[14] ) << 24 )
                  | ( static_cast<unsigned int>( n[15] ) << 16 )
                  | ( static_cast<unsigned int>( n[16] ) << 8 )
                  | static_cast<unsigned int>( n[17] ) ) - INT4_BIAS;

            // +27: int3 via style index
            node.viaStyleIdx = ( ( static_cast<int>( n[27] ) << 16 )
                               | ( static_cast<int>( n[28] ) << 8 )
                               | static_cast<int>( n[29] ) ) - INT3_BIAS;
            node.viaOuterDiam = static_cast<int>(
                    ( static_cast<unsigned int>( n[18] ) << 24 )
                  | ( static_cast<unsigned int>( n[19] ) << 16 )
                  | ( static_cast<unsigned int>( n[20] ) << 8 )
                  | static_cast<unsigned int>( n[21] ) ) - INT4_BIAS;
            node.routeFlag = n[22];
            node.viaDrillDiam = static_cast<int>(
                    ( static_cast<unsigned int>( n[30] ) << 24 )
                  | ( static_cast<unsigned int>( n[31] ) << 16 )
                  | ( static_cast<unsigned int>( n[32] ) << 8 )
                  | static_cast<unsigned int>( n[33] ) ) - INT4_BIAS;
            // +37: int3 route mode/class. Observed values in sample corpus are 0, 1, and 3.
            // +40 is a trailing byte (0 in sampled files).
            node.routeMode = ( ( static_cast<int>( n[37] ) << 16 )
                             | ( static_cast<int>( n[38] ) << 8 )
                             | static_cast<int>( n[39] ) ) - INT3_BIAS;
            // DipTrace route points use ViaStyle as the explicit via indicator:
            // ViaStyle = -1 means no via at this point.
            // byte(+22) is not reliable for via placement (set on many non-via nodes).
            node.hasVia = ( node.viaStyleIdx >= 0 );

            // Sanity checks
            if( node.width <= 0 || node.width > 5000000 )
            {
                valid = false;
                break;
            }

            if( node.layer < 0 || node.layer > 50 )
            {
                valid = false;
                break;
            }

            chain.nodes.push_back( node );
        }

        if( valid && !chain.nodes.empty() )
            m_trackChains.push_back( std::move( chain ) );

        pos = nodesEnd;
    }

}


void PCB_PARSER::InferPadNetsFromRoutingRefs()
{
    if( m_components.empty() || m_nets.empty() )
        return;

    size_t totalRefs = 0;

    for( const DT_NET& net : m_nets )
        totalRefs += net.padRefs.size();

    if( totalRefs == 0 )
        return;

    size_t maxReasonableRefs = std::max<size_t>( 5000, m_components.size() * 64 );

    if( totalRefs > maxReasonableRefs )
    {
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: skipping routing-ref pad inference (%zu refs exceeds %zu cap)" ),
                    totalRefs, maxReasonableRefs );
        return;
    }

    std::vector<std::unordered_map<int, size_t>> padPosByIndex( m_components.size() );

    for( size_t c = 0; c < m_components.size(); c++ )
    {
        const std::vector<DT_PAD>& pads = m_components[c].pads;
        auto& padMap = padPosByIndex[c];

        padMap.reserve( pads.size() );

        for( size_t p = 0; p < pads.size(); p++ )
            padMap.emplace( pads[p].index, p );
    }

    struct SCORE
    {
        int base = 0;
        int refs = 0;
        int hits = 0;
        int fillable = 0;
    };

    auto scoreBase = [&]( int aBase ) -> SCORE
    {
        SCORE score;
        score.base = aBase;

        for( const DT_NET& net : m_nets )
        {
            for( const DT_PAD_REF& ref : net.padRefs )
            {
                score.refs++;

                int compPos = ref.componentIndex - aBase;

                if( compPos < 0 || compPos >= static_cast<int>( m_components.size() ) )
                    continue;

                const auto& padMap = padPosByIndex[compPos];
                auto it = padMap.find( ref.padIndex );

                if( it == padMap.end() )
                    continue;

                const DT_PAD& pad = m_components[compPos].pads[it->second];
                score.hits++;

                if( pad.netIndex < 0 )
                    score.fillable++;
            }
        }

        return score;
    };

    SCORE scoreBase0 = scoreBase( 0 );
    SCORE scoreBase1 = scoreBase( 1 );

    if( scoreBase0.hits == 0 && scoreBase1.hits == 0 )
        return;

    if( scoreBase0.hits == scoreBase1.hits && scoreBase0.fillable == scoreBase1.fillable )
        return;

    SCORE best = scoreBase0;

    if( scoreBase1.hits > scoreBase0.hits
        || ( scoreBase1.hits == scoreBase0.hits && scoreBase1.fillable > scoreBase0.fillable ) )
    {
        best = scoreBase1;
    }

    if( best.hits < 8 )
        return;

    int assigned = 0;
    int conflicts = 0;

    for( const DT_NET& net : m_nets )
    {
        for( const DT_PAD_REF& ref : net.padRefs )
        {
            int compPos = ref.componentIndex - best.base;

            if( compPos < 0 || compPos >= static_cast<int>( m_components.size() ) )
                continue;

            auto& padMap = padPosByIndex[compPos];
            auto it = padMap.find( ref.padIndex );

            if( it == padMap.end() )
                continue;

            DT_PAD& pad = m_components[compPos].pads[it->second];

            if( pad.netIndex < 0 )
            {
                pad.netIndex = net.index;
                assigned++;
            }
            else if( pad.netIndex != net.index )
            {
                conflicts++;
            }
        }
    }

    if( assigned > 0 || conflicts > 0 )
    {
        wxLogTrace( traceDiptraceIo,
                    wxT( "DipTrace: routing-ref pad net inference (base=%d): %d refs, %d hits, "
                         "%d assigned, %d conflicts" ),
                    best.base, best.refs, best.hits, assigned, conflicts );
    }
}


// ---------------------------------------------------------------------------
// Zone parsing
// ---------------------------------------------------------------------------

void PCB_PARSER::FindAndParseZones( size_t aSearchStart, size_t aSearchEnd )
{
    m_zones.clear();

    const uint8_t* data = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();

    if( aSearchEnd > fileSize )
        aSearchEnd = fileSize;

    static constexpr int MAX_ZONES = 100;
    static constexpr int MAX_VERTICES = 50000;
    static constexpr int MAX_REASONABLE_DIM = 5000000;

    // Prepare board outline bounds for zone vertex validation.
    // We use these bounds for both section discovery and zone parsing.
    int bboxXMin = m_bboxXMin - 5000000;
    int bboxXMax = m_bboxXMax + 5000000;
    int bboxYMin = m_bboxYMin - 5000000;
    int bboxYMax = m_bboxYMax + 5000000;

    if( !m_outline.empty() )
    {
        int oxMin = m_outline[0].x, oxMax = m_outline[0].x;
        int oyMin = m_outline[0].y, oyMax = m_outline[0].y;

        for( const DT_VERTEX& v : m_outline )
        {
            oxMin = std::min( oxMin, v.x );
            oxMax = std::max( oxMax, v.x );
            oyMin = std::min( oyMin, v.y );
            oyMax = std::max( oyMax, v.y );
        }

        bboxXMin = oxMin - 5000000;
        bboxXMax = oxMax + 5000000;
        bboxYMin = oyMin - 5000000;
        bboxYMax = oyMax + 5000000;
    }

    auto headerLooksPlausible = [&]( size_t aPos, bool aCheckVertexSamples ) -> bool
    {
        if( aPos + 30 > aSearchEnd )
            return false;

        int fieldA    = ReadInt3At( data, aPos );
        int flags1    = data[aPos + 3];
        int flags3    = data[aPos + 5];
        int minWidth  = ReadInt4At( data, aPos + 6 );
        int clearance = ReadInt4At( data, aPos + 10 );
        int minimumArea = ReadInt4At( data, aPos + 14 );
        int separator = ReadInt3At( data, aPos + 18 );
        int layer     = ReadInt3At( data, aPos + 21 );
        int fieldB    = ReadInt3At( data, aPos + 24 );
        int vtxCount  = ReadInt3At( data, aPos + 27 );

        if( fieldA < 0 || fieldA > 10000 )
            return false;

        if( fieldB < -1 || fieldB > 10000 )
            return false;

        if( flags1 > 2 || flags3 > 2 )
            return false;

        if( clearance <= 0 || clearance > MAX_REASONABLE_DIM )
            return false;

        if( minWidth <= 0 || minWidth > MAX_REASONABLE_DIM )
            return false;

        if( minimumArea < 0 || minimumArea > MAX_REASONABLE_DIM )
            return false;

        if( separator > 0 )
            return false;

        if( layer < -10 || layer > 100 )
            return false;

        if( vtxCount < 3 || vtxCount > MAX_VERTICES )
            return false;

        size_t vtxStart = aPos + 30;
        size_t vtxEnd = vtxStart + static_cast<size_t>( vtxCount ) * 8;

        if( vtxEnd > aSearchEnd )
            return false;

        if( !aCheckVertexSamples )
            return true;

        int sampleCount = std::min( 3, vtxCount );

        auto vertexInBounds = [&]( size_t aVertexPos ) -> bool
        {
            int x = ReadInt4At( data, aVertexPos );
            int y = ReadInt4At( data, aVertexPos + 4 );
            return x >= bboxXMin && x <= bboxXMax && y >= bboxYMin && y <= bboxYMax;
        };

        for( int i = 0; i < sampleCount; i++ )
        {
            size_t vp = vtxStart + static_cast<size_t>( i ) * 8;

            if( !vertexInBounds( vp ) )
                return false;
        }

        size_t lastVp = vtxStart + static_cast<size_t>( vtxCount - 1 ) * 8;

        return vertexInBounds( lastVp );
    };

    auto headerHasZoneSectionShape = [&]( size_t aPos ) -> bool
    {
        if( aPos + 30 > aSearchEnd )
            return false;

        int fieldA    = ReadInt3At( data, aPos );
        int flags1    = data[aPos + 3];
        int flags3    = data[aPos + 5];
        int separator = ReadInt3At( data, aPos + 18 );
        int layer     = ReadInt3At( data, aPos + 21 );
        int fieldB    = ReadInt3At( data, aPos + 24 );
        int vtxCount  = ReadInt3At( data, aPos + 27 );

        return fieldA >= 0 && fieldA <= 10000
               && fieldB >= -1 && fieldB <= 10000
               && flags1 <= 2 && flags3 <= 2
               && separator <= 0
               && layer >= -10 && layer <= 100
               && vtxCount >= 3 && vtxCount <= MAX_VERTICES
               && aPos + 30 + static_cast<size_t>( vtxCount ) * 8 <= aSearchEnd;
    };

    auto parseZoneTrailer = [&]( DT_ZONE& aZone, size_t aSearchStartPos, size_t aSearchEndPos,
                                 int aZoneIndex ) -> void
    {
        // Trailer block observed near the end of each inter-zone gap:
        //   int3(regions_counted), int4(0), int4(board_clearance),
        //   byte(island_region), byte(island_internal), byte(island_connection),
        //   int3(zone_id), byte(via_direct), byte(smd_separate),
        //   int3(smd_spoke_mode), int4(smd_spoke_width),
        //   byte(ratline_mode), byte(regions_done)
        static constexpr size_t TRAILER_LEN = 28;
        static constexpr size_t STYLE_BLOCK_LEN = 14;
        static constexpr int    CACHED_RECORD_LEN = 23;

        if( aSearchEndPos <= aSearchStartPos || aSearchEndPos - aSearchStartPos < TRAILER_LEN )
            return;

        size_t lastStart = aSearchEndPos - TRAILER_LEN;

        for( size_t trailerPos = lastStart + 1; trailerPos-- > aSearchStartPos; )
        {
            int lead         = ReadInt3At( data, trailerPos );
            int zeroInt4     = ReadInt4At( data, trailerPos + 3 );
            int boardClr     = ReadInt4At( data, trailerPos + 7 );
            uint8_t islandR  = data[trailerPos + 11];
            uint8_t islandI  = data[trailerPos + 12];
            uint8_t islandC  = data[trailerPos + 13];
            int zoneId       = ReadInt3At( data, trailerPos + 14 );
            uint8_t viaDir   = data[trailerPos + 17];
            uint8_t smdSep   = data[trailerPos + 18];
            int smdSpokeMode = ReadInt3At( data, trailerPos + 19 );
            int smdSpokeW    = ReadInt4At( data, trailerPos + 22 );
            uint8_t ratMode  = data[trailerPos + 26];
            uint8_t doneFlag = data[trailerPos + 27];

            if( lead < 0 || lead > 100000 || zeroInt4 != -INT4_BIAS )
                continue;

            if( boardClr < 0 || boardClr > MAX_REASONABLE_DIM )
                continue;

            if( islandR > 1 || islandI > 1 || islandC > 1 )
                continue;

            if( zoneId < 0 || zoneId > 100000 )
                continue;

            if( viaDir > 1 || smdSep > 1 )
                continue;

            if( smdSpokeMode < 0 || smdSpokeMode > 4 )
                continue;

            if( smdSpokeW <= 0 || smdSpokeW > MAX_REASONABLE_DIM )
                continue;

            if( ratMode > 2 )
                continue;

            if( doneFlag > 1 )
                continue;

            size_t payloadStart = aSearchStartPos;

            if( aSearchStartPos + STYLE_BLOCK_LEN <= aSearchEndPos )
            {
                int styleLead = ReadInt3At( data, aSearchStartPos );
                int styleSpokeMode = ReadInt3At( data, aSearchStartPos + 3 );
                int styleLineSpacing = ReadInt4At( data, aSearchStartPos + 6 );
                int styleSpokeWidth = ReadInt4At( data, aSearchStartPos + 10 );

                if( styleLead == 0
                    && styleSpokeMode >= 0 && styleSpokeMode <= 4
                    && styleLineSpacing > 0 && styleLineSpacing <= MAX_REASONABLE_DIM
                    && styleSpokeWidth > 0 && styleSpokeWidth <= MAX_REASONABLE_DIM )
                {
                    payloadStart += STYLE_BLOCK_LEN;
                }
            }

            int cachedBytes = 0;
            int cachedRecords = 0;
            aZone.cachedFillRecords.clear();

            if( trailerPos > payloadStart )
            {
                cachedBytes = static_cast<int>( trailerPos - payloadStart );

                if( cachedBytes % CACHED_RECORD_LEN == 0 )
                {
                    cachedRecords = cachedBytes / CACHED_RECORD_LEN;
                    aZone.cachedFillRecords.reserve( static_cast<size_t>( cachedRecords ) );

                    for( int recIdx = 0; recIdx < cachedRecords; recIdx++ )
                    {
                        size_t recPos = payloadStart + static_cast<size_t>( recIdx ) * CACHED_RECORD_LEN;

                        if( recPos + CACHED_RECORD_LEN > trailerPos )
                            break;

                        DT_ZONE_CACHED_FILL_RECORD rec;
                        rec.field0 = ReadInt3At( data, recPos );
                        rec.field1 = ReadInt4At( data, recPos + 3 );
                        rec.field2 = ReadInt4At( data, recPos + 7 );
                        rec.field3 = ReadInt4At( data, recPos + 11 );
                        rec.field4 = ReadInt4At( data, recPos + 15 );
                        rec.field5 = ReadInt4At( data, recPos + 19 );
                        aZone.cachedFillRecords.push_back( rec );
                    }

                    cachedRecords = static_cast<int>( aZone.cachedFillRecords.size() );
                }
            }

            aZone.regionsCounted = lead;
            aZone.cachedFillByteLen = cachedBytes;
            aZone.cachedFillRecordCount = cachedRecords;
            aZone.boardClearance = boardClr;
            aZone.zoneId = zoneId;
            aZone.viaDirect = viaDir;
            aZone.smdSeparate = smdSep;
            aZone.smdSpokeMode = smdSpokeMode;
            aZone.smdSpokeWidth = smdSpokeW;
            aZone.islandRegion = islandR;
            aZone.islandInternal = islandI;
            aZone.islandConnection = islandC;
            aZone.ratlineMode = ratMode;
            aZone.regionsDone = doneFlag;

            if( ShouldDumpZones() )
            {
                wxLogTrace( traceDiptraceIo,
                            wxT( "DipTrace: zone[%d] trailer off=0x%06zX regionsCounted=%d "
                                 "cachedBytes=%d cachedRecords=%d boardClr=%d "
                                 "islands=[%u,%u,%u] id=%d viaDirect=%u smdSeparate=%u "
                                 "smdSpokeMode=%d smdSpokeWidth=%d ratMode=%u done=%u" ),
                            aZoneIndex, trailerPos, lead, cachedBytes, cachedRecords, boardClr,
                            static_cast<unsigned int>( islandR ), static_cast<unsigned int>( islandI ),
                            static_cast<unsigned int>( islandC ), zoneId, static_cast<unsigned int>( viaDir ),
                            static_cast<unsigned int>( smdSep ), smdSpokeMode, smdSpokeW,
                            static_cast<unsigned int>( ratMode ), static_cast<unsigned int>( doneFlag ) );

                if( !aZone.cachedFillRecords.empty() )
                {
                    int zoneXMin = aZone.outline[0].first;
                    int zoneXMax = aZone.outline[0].first;
                    int zoneYMin = aZone.outline[0].second;
                    int zoneYMax = aZone.outline[0].second;

                    for( const auto& p : aZone.outline )
                    {
                        zoneXMin = std::min( zoneXMin, p.first );
                        zoneXMax = std::max( zoneXMax, p.first );
                        zoneYMin = std::min( zoneYMin, p.second );
                        zoneYMax = std::max( zoneYMax, p.second );
                    }

                    const DT_ZONE_CACHED_FILL_RECORD& firstRec = aZone.cachedFillRecords.front();
                    std::array<int, 6> minVals = {
                        firstRec.field0, firstRec.field1, firstRec.field2,
                        firstRec.field3, firstRec.field4, firstRec.field5
                    };
                    std::array<int, 6> maxVals = minVals;
                    std::array<int, 6> inXHits = { 0, 0, 0, 0, 0, 0 };
                    std::array<int, 6> inYHits = { 0, 0, 0, 0, 0, 0 };
                    std::array<int, 6> nonNegHits = { 0, 0, 0, 0, 0, 0 };
                    std::map<int, int> field0Hist;
                    std::map<int, int> field5Hist;
                    int f0EqRegions = 0;
                    int xEqualCount = 0;
                    int yEqualCount = 0;
                    int bothEqualCount = 0;

                    for( const DT_ZONE_CACHED_FILL_RECORD& rec : aZone.cachedFillRecords )
                    {
                        std::array<int, 6> vals = {
                            rec.field0, rec.field1, rec.field2,
                            rec.field3, rec.field4, rec.field5
                        };

                        field0Hist[rec.field0]++;
                        field5Hist[rec.field5]++;

                        if( rec.field0 == aZone.regionsCounted )
                            f0EqRegions++;

                        bool xEq = ( rec.field1 == rec.field3 );
                        bool yEq = ( rec.field2 == rec.field4 );

                        if( xEq )
                            xEqualCount++;

                        if( yEq )
                            yEqualCount++;

                        if( xEq && yEq )
                            bothEqualCount++;

                        for( size_t fi = 0; fi < vals.size(); fi++ )
                        {
                            minVals[fi] = std::min( minVals[fi], vals[fi] );
                            maxVals[fi] = std::max( maxVals[fi], vals[fi] );

                            if( vals[fi] >= zoneXMin && vals[fi] <= zoneXMax )
                                inXHits[fi]++;

                            if( vals[fi] >= zoneYMin && vals[fi] <= zoneYMax )
                                inYHits[fi]++;

                            if( vals[fi] >= 0 )
                                nonNegHits[fi]++;
                        }
                    }

                    wxLogTrace( traceDiptraceIo,
                                wxT( "DipTrace: zone[%d] cached-range "
                                     "f0=[%d,%d] f1=[%d,%d] f2=[%d,%d] f3=[%d,%d] f4=[%d,%d] f5=[%d,%d]" ),
                                aZoneIndex, minVals[0], maxVals[0], minVals[1], maxVals[1], minVals[2], maxVals[2],
                                minVals[3], maxVals[3], minVals[4], maxVals[4], minVals[5], maxVals[5] );

                    wxLogTrace( traceDiptraceIo,
                                wxT( "DipTrace: zone[%d] cached-hits "
                                     "xHits=[%d,%d,%d,%d,%d,%d] yHits=[%d,%d,%d,%d,%d,%d] "
                                     "nonNeg=[%d,%d,%d,%d,%d,%d]" ),
                                aZoneIndex, inXHits[0], inXHits[1], inXHits[2], inXHits[3], inXHits[4], inXHits[5],
                                inYHits[0], inYHits[1], inYHits[2], inYHits[3], inYHits[4], inYHits[5], nonNegHits[0],
                                nonNegHits[1], nonNegHits[2], nonNegHits[3], nonNegHits[4], nonNegHits[5] );

                    wxLogTrace( traceDiptraceIo, wxT( "DipTrace: zone[%d] cached-zone-bbox=[%d,%d,%d,%d]" ), aZoneIndex,
                                zoneXMin, zoneXMax, zoneYMin, zoneYMax );

                    auto histToString = []( const std::map<int, int>& aHist ) -> wxString
                    {
                        wxString out;
                        bool first = true;

                        for( const auto& kv : aHist )
                        {
                            if( !first )
                                out += wxT( ";" );

                            first = false;
                            out += wxString::Format( wxT( "%d:%d" ), kv.first, kv.second );
                        }

                        return out;
                    };

                    wxLogTrace( traceDiptraceIo,
                                wxT( "DipTrace: zone[%d] cached-hist f0={%s} f5={%s} "
                                     "f0EqRegions=%d xEq=%d yEq=%d bothEq=%d" ),
                                aZoneIndex, histToString( field0Hist ), histToString( field5Hist ), f0EqRegions,
                                xEqualCount, yEqualCount, bothEqualCount );

                    size_t sampleCount = std::min<size_t>( 6, aZone.cachedFillRecords.size() );

                    for( size_t ri = 0; ri < sampleCount; ri++ )
                    {
                        const DT_ZONE_CACHED_FILL_RECORD& rec = aZone.cachedFillRecords[ri];
                        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: zone[%d] cached-rec[%zu]={%d,%d,%d,%d,%d,%d}" ),
                                    aZoneIndex, ri, rec.field0, rec.field1, rec.field2, rec.field3, rec.field4,
                                    rec.field5 );
                    }
                }
            }

            break;
        }
    };

    size_t zoneHeaderStart = NOT_FOUND;
    bool   zoneViaPreamble = false;

    auto findZoneFontPreambleDataStart = [&]( size_t aStart ) -> size_t
    {
        static const wxString fontNames[] = {
            wxT( "Arial" ), wxT( "Tahoma" ), wxT( "Times New Roman" ),
            wxT( "Courier New" ), wxT( "Verdana" ), wxT( "Calibri" )
        };

        size_t bestDataStart = NOT_FOUND;

        for( const wxString& fontName : fontNames )
        {
            size_t fontPos = m_reader.FindString( fontName, aStart, aSearchEnd );

            while( fontPos != NOT_FOUND )
            {
                size_t strEnd;

                if( m_version <= LEGACY_STRING_VERSION )
                {
                    int bc = ReadInt3At( data, fontPos );

                    if( bc < 0 || bc > 500 )
                    {
                        fontPos = m_reader.FindString( fontName, fontPos + 3, aSearchEnd );
                        continue;
                    }

                    strEnd = fontPos + 3 + bc;
                }
                else
                {
                    uint16_t cc = ( static_cast<uint16_t>( data[fontPos] ) << 8 )
                                | data[fontPos + 1];
                    strEnd = fontPos + 2 + static_cast<size_t>( cc ) * 2;
                }

                if( strEnd + 16 > aSearchEnd )
                    break;

                int fontSize = ReadInt3At( data, strEnd );
                int bold = data[strEnd + 3];
                int fontH = ReadInt4At( data, strEnd + 4 );
                int fontW = ReadInt4At( data, strEnd + 8 );
                int tail = ReadInt4At( data, strEnd + 12 );

                if( fontSize >= 5 && fontSize <= 30 && bold <= 1
                    && fontH > 0 && fontH < 10000000
                    && fontW > 0 && fontW < 10000000
                    && tail == ZONE_FONT_PREAMBLE_TAIL )
                {
                    bestDataStart = std::min( bestDataStart, strEnd + 16 );
                    break;
                }

                fontPos = m_reader.FindString( fontName, strEnd, aSearchEnd );
            }
        }

        return bestDataStart;
    };

    size_t zoneDataStart = findZoneFontPreambleDataStart( aSearchStart );

    while( zoneDataStart != NOT_FOUND )
    {
        size_t preambleHeaderStart = zoneDataStart + 3;

        if( preambleHeaderStart + 30 <= aSearchEnd
            && headerLooksPlausible( preambleHeaderStart, true ) )
        {
            zoneHeaderStart = preambleHeaderStart;
            zoneViaPreamble = true;
            break;
        }

        if( preambleHeaderStart + 30 <= aSearchEnd
            && headerHasZoneSectionShape( preambleHeaderStart ) )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "DipTrace import: invalid copper-pour zone header after font preamble "
                       "at offset 0x%06zX." ),
                    preambleHeaderStart ) );
        }

        zoneDataStart = findZoneFontPreambleDataStart( zoneDataStart + 1 );
    }

    // Primary locator: scan structurally for plausible zone headers.
    for( size_t scanPos = aSearchStart; zoneHeaderStart == NOT_FOUND
                                      && scanPos + 30 <= aSearchEnd; scanPos++ )
    {
        if( headerLooksPlausible( scanPos, true ) )
        {
            zoneHeaderStart = scanPos;
            wxLogTrace( traceDiptraceIo, wxT( "DipTrace: zone section found by structural scan at 0x%06zX" ),
                        zoneHeaderStart );
            break;
        }
    }

    // Fallback locator: zone section may be preceded by a font preamble:
    //   string(font_name)  int3(font_size)  byte(bold)
    //   int4(font_height)  int4(font_width)  int4(-20000)
    //
    // Historically, zones follow after int3(0) separator.
    if( zoneHeaderStart == NOT_FOUND )
    {
        zoneDataStart = findZoneFontPreambleDataStart( aSearchStart );

        if( zoneDataStart != NOT_FOUND )
        {
            size_t fallbackStart = zoneDataStart + 3;

            if( fallbackStart + 30 <= aSearchEnd
                && headerLooksPlausible( fallbackStart, true ) )
            {
                zoneHeaderStart = fallbackStart;
                zoneViaPreamble = true;
            }
            else
            {
                size_t fallbackEnd = std::min( aSearchEnd, zoneDataStart + 256 );

                for( size_t scanPos = zoneDataStart; scanPos + 30 <= fallbackEnd; scanPos++ )
                {
                    if( headerLooksPlausible( scanPos, true ) )
                    {
                        zoneHeaderStart = scanPos;
                        break;
                    }
                }
            }

            if( zoneHeaderStart != NOT_FOUND )
            {
                wxLogTrace( traceDiptraceIo, wxT( "DipTrace: zone section found by font fallback at 0x%06zX" ),
                            zoneHeaderStart );
            }
        }
    }

    if( zoneHeaderStart == NOT_FOUND )
        return;

    size_t pos = zoneHeaderStart;

    for( int zi = 0; zi < MAX_ZONES && pos + 30 < aSearchEnd; zi++ )
    {
        // Zone header: 30 bytes
        //   int3(fieldA) + 3 flag bytes + int4(line_width) + int4(clearance)
        //   + int4(minimum_area) + int3(separator=-1) + int3(layer) + int3(fieldB) + int3(vertex_count)
        //
        // Empirical mapping against viewer examples:
        //   fieldB (+24) = connected net id
        //   fieldA (+0)  = filled-region count, NOT the CopperPour priority.
        //                  The viewer XML shows zones with Priority=0 carrying
        //                  fieldA values of 1 and 5, so it tracks fill complexity.
        //                  True priority storage is not yet located; the only
        //                  non-zero-priority samples are multi-region planes,
        //                  which confounds priority with the region count.
        int fieldA    = ReadInt3At( data, pos );
        int flags1    = data[pos + 3];
        int flags2    = data[pos + 4];
        int flags3    = data[pos + 5];
        int minWidth  = ReadInt4At( data, pos + 6 );
        int clearance = ReadInt4At( data, pos + 10 );
        int minimumArea = ReadInt4At( data, pos + 14 );
        int separator = ReadInt3At( data, pos + 18 );
        int layer     = ReadInt3At( data, pos + 21 );
        int fieldB    = ReadInt3At( data, pos + 24 );
        int vtxCount  = ReadInt3At( data, pos + 27 );

        // Validate header fields
        if( fieldA < 0 || fieldA > 10000 )
            break;

        if( fieldB < -1 || fieldB > 10000 )
            break;

        if( flags1 > 2 || flags3 > 2 )
            break;

        if( clearance <= 0 || clearance > MAX_REASONABLE_DIM )
            break;

        if( minWidth <= 0 || minWidth > MAX_REASONABLE_DIM )
            break;

        if( minimumArea < 0 || minimumArea > MAX_REASONABLE_DIM )
            break;

        if( separator > 0 )
            break;

        if( layer < -10 || layer > 100 )
            break;

        if( vtxCount < 3 || vtxCount > MAX_VERTICES )
            break;

        size_t vtxStart = pos + 30;
        size_t vtxEnd = vtxStart + static_cast<size_t>( vtxCount ) * 8;

        if( vtxEnd > aSearchEnd )
            break;

        // Parse outline vertices and validate they're within board bounds
        DT_ZONE zone;
        zone.netIndex  = fieldB;
        zone.layer     = layer;
        zone.fillMode  = static_cast<uint8_t>( flags1 );
        zone.rawFlag2  = static_cast<uint8_t>( flags2 );
        zone.connectionMode = static_cast<uint8_t>( flags3 );
        zone.separator = separator;
        zone.clearance = clearance;
        zone.minWidth  = minWidth;
        zone.minimumArea = minimumArea;
        zone.outline.reserve( vtxCount );

        bool validOutline = true;

        for( int vi = 0; vi < vtxCount; vi++ )
        {
            size_t vp = vtxStart + static_cast<size_t>( vi ) * 8;
            int    x = ReadInt4At( data, vp );
            int    y = ReadInt4At( data, vp + 4 );

            if( x < bboxXMin || x > bboxXMax || y < bboxYMin || y > bboxYMax )
            {
                validOutline = false;
                break;
            }

            zone.outline.emplace_back( x, y );
        }

        if( !validOutline )
            break;

        wxString zoneNetName;

        for( const DT_NET& net : m_nets )
        {
            if( net.index == fieldB )
            {
                zoneNetName = net.name;
                break;
            }
        }

        DumpZoneHeader( zi, pos, data, fieldA, flags1, flags2, flags3, minWidth, clearance,
                        minimumArea, separator, layer, fieldB, vtxCount, zoneNetName );

        m_zones.push_back( std::move( zone ) );
        DT_ZONE& parsedZone = m_zones.back();

        // Skip fill segments: int3(seg_count) + seg_count * 19 bytes
        pos = vtxEnd;

        if( pos + 3 > aSearchEnd )
            break;

        int fillSegCount = ReadInt3At( data, pos );

        if( fillSegCount < 0 || fillSegCount > 500000 )
            break;

        if( ShouldDumpZones() )
        {
            wxLogTrace( traceDiptraceIo, wxT( "DipTrace: zone[%d] fill-segments off=0x%06zX count=%d" ), zi, pos,
                        fillSegCount );
        }

        pos += 3 + static_cast<size_t>( fillSegCount ) * 19;

        // Skip fill polygons: int3(poly_count) + each polygon
        if( pos + 3 > aSearchEnd )
            break;

        int fillPolyCount = ReadInt3At( data, pos );

        if( fillPolyCount < 0 || fillPolyCount > 50000 )
            break;

        if( ShouldDumpZones() )
        {
            wxLogTrace( traceDiptraceIo, wxT( "DipTrace: zone[%d] fill-polys off=0x%06zX count=%d" ), zi, pos,
                        fillPolyCount );
        }

        pos += 3;

        for( int pi = 0; pi < fillPolyCount; pi++ )
        {
            if( pos + 3 > aSearchEnd )
            {
                fillPolyCount = pi;
                break;
            }

            int pvc = ReadInt3At( data, pos );

            if( pvc < 0 || pvc > MAX_VERTICES )
            {
                fillPolyCount = pi;
                break;
            }

            pos += 3 + static_cast<size_t>( pvc ) * 8 + 3;

            if( pos > aSearchEnd )
            {
                fillPolyCount = pi;
                break;
            }
        }

        // Post-fill style block starts immediately after fill polygon payload.
        // First fields observed in viewer examples:
        //   int3(0), int3(spoke_mode), int4(line_spacing), int4(spoke_width)
        // where spoke_mode maps to the UI enum:
        //   0=Direct, 1=2 spoke 90, 2=2 spoke, 3=4 spoke 45, 4=4 spoke.
        if( pos + 14 <= aSearchEnd )
        {
            int styleLead = ReadInt3At( data, pos );
            int spokeMode = ReadInt3At( data, pos + 3 );
            int lineSpacing = ReadInt4At( data, pos + 6 );
            int spokeWidth = ReadInt4At( data, pos + 10 );

            if( styleLead == 0
                && spokeMode >= 0 && spokeMode <= 4
                && lineSpacing > 0 && lineSpacing <= MAX_REASONABLE_DIM
                && spokeWidth > 0 && spokeWidth <= MAX_REASONABLE_DIM )
            {
                parsedZone.spokeMode = spokeMode;
                parsedZone.lineSpacing = lineSpacing;
                parsedZone.spokeWidth = spokeWidth;

                if( ShouldDumpZones() )
                {
                    wxLogTrace( traceDiptraceIo,
                                wxT( "DipTrace: zone[%d] style lead=%d spokeMode=%d lineSpacing=%d spokeWidth=%d" ), zi,
                                styleLead, spokeMode, lineSpacing, spokeWidth );
                }
            }
        }

        // Scan for the next zone header. Some boards (e.g. PCB_6) have large
        // filled-data blocks between zone records; a short scan window misses
        // later valid headers.
        bool foundNext = false;
        size_t scanStart = pos;

        for( size_t testPos = pos; testPos + 30 <= aSearchEnd; testPos++ )
        {
            if( !headerLooksPlausible( testPos, false ) )
                continue;

            if( !headerLooksPlausible( testPos, true ) )
                continue;

            pos = testPos;
            foundNext = true;
            break;
        }

        if( foundNext && pos > scanStart )
        {
            parseZoneTrailer( parsedZone, scanStart, pos, zi );
            DumpZoneGap( zi, scanStart, pos, data );
        }

        if( !foundNext )
        {
            parseZoneTrailer( parsedZone, scanStart, aSearchEnd, zi );
            DumpZoneTail( zi, scanStart, aSearchEnd, data );
            break;
        }
    }

    if( !m_zones.empty() )
    {
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: parsed %zu copper zones" ), m_zones.size() );
    }

    // The zone section is field-located when anchored by its font preamble (font name + sizing +
    // an int4(-20000) tail), the structural prefix that immediately precedes the zone header. Count
    // a scan-locate only when zones were instead recovered by the plausible-header structural scan.
    if( !m_zones.empty() && !zoneViaPreamble )
        m_sectionLocatorScans++;
}


// ---------------------------------------------------------------------------
// Board object creation
// ---------------------------------------------------------------------------

void PCB_PARSER::ApplyBoardSettings()
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // The parsed layer list corresponds to project copper layers in top-to-bottom order.
    int copperCount = static_cast<int>( m_layers.size() );

    if( copperCount < 2 )
        copperCount = 2;

    // KiCad requires an even copper layer count
    if( copperCount % 2 != 0 )
        copperCount++;

    m_board->SetCopperLayerCount( copperCount );

    // Build the board stackup from the copper layer count
    BOARD_STACKUP& stackup = bds.GetStackupDescriptor();
    stackup.RemoveAll();
    stackup.BuildDefaultStackupList( &bds, copperCount );

    // Enable the layers used by the design
    LSET enabledLayers = m_board->GetEnabledLayers();

    for( const DT_LAYER& layer : m_layers )
    {
        PCB_LAYER_ID kiLayer = MapCopperLayer( layer.index );

        if( kiLayer == UNDEFINED_LAYER )
            kiLayer = MapLayer( layer.index );

        if( kiLayer != UNDEFINED_LAYER )
        {
            enabledLayers.set( kiLayer );

            if( !layer.name.empty() )
                m_board->SetLayerName( kiLayer, layer.name );
        }
    }

    m_board->SetEnabledLayers( enabledLayers );

    // Apply default track width and clearance from the first design rule
    std::shared_ptr<NETCLASS> defNetclass = bds.m_NetSettings->GetDefaultNetclass();

    if( !m_designRules.empty() )
    {
        const DT_DESIGN_RULE& firstRule = m_designRules[0];

        if( firstRule.trackWidth > 0 )
            defNetclass->SetTrackWidth( ToKiCadCoord( firstRule.trackWidth ) );

        if( firstRule.clearance > 0 )
            defNetclass->SetClearance( ToKiCadCoord( firstRule.clearance ) );
    }

    // Apply via definitions from parsed ViaStyles
    if( !m_viaStyles.empty() )
    {
        const DT_VIA_STYLE& firstVia = m_viaStyles[0];

        if( firstVia.outerDiameter > 0 )
            defNetclass->SetViaDiameter( ToKiCadCoord( firstVia.outerDiameter ) );

        if( firstVia.drillDiameter > 0 )
            defNetclass->SetViaDrill( ToKiCadCoord( firstVia.drillDiameter ) );
    }
}


void PCB_PARSER::CreateBoardOutline()
{
    if( m_outline.empty() && m_bboxXMin == 0 && m_bboxXMax == 0 )
        return;

    STROKE_PARAMS stroke( pcbIUScale.mmToIU( 0.05 ), LINE_STYLE::SOLID );

    if( !m_outline.empty() )
    {
        // Build a list of converted points and their arc flags.
        size_t n = m_outline.size();

        if( n < 2 )
            return;

        std::vector<VECTOR2I> pts;
        std::vector<uint8_t>  arcs;
        pts.reserve( n );
        arcs.reserve( n );

        for( const DT_VERTEX& v : m_outline )
        {
            pts.push_back( VECTOR2I( ToKiCadCoord( v.x ), ToKiCadCoord( v.y ) ) );
            arcs.push_back( v.arc );
        }

        // Walk the vertex list and emit individual Edge_Cuts segments and arcs.
        // In DipTrace, a vertex with arc=1 is an arc midpoint: the arc runs from the
        // previous (non-arc) vertex through this midpoint to the next (non-arc) vertex.
        size_t i = 0;

        while( i < n && arcs[i] == 1 )
            i++;

        if( i == n )
            return;

        size_t startIndex = i;
        size_t steps = 0;
        size_t maxSteps = n * 4;

        struct OUTLINE_PRIM
        {
            bool     isArc = false;
            VECTOR2I start;
            VECTOR2I mid;
            VECTOR2I end;
        };

        std::vector<OUTLINE_PRIM> outlinePrims;
        outlinePrims.reserve( n );

        auto addSegment = [&]( const VECTOR2I& aStart, const VECTOR2I& aEnd )
        {
            OUTLINE_PRIM prim;
            prim.isArc = false;
            prim.start = aStart;
            prim.end = aEnd;
            outlinePrims.push_back( prim );
        };

        auto addArc = [&]( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd )
        {
            OUTLINE_PRIM prim;
            prim.isArc = true;
            prim.start = aStart;
            prim.mid = aMid;
            prim.end = aEnd;
            outlinePrims.push_back( prim );
        };

        while( i < n && steps++ < maxSteps )
        {
            size_t next = ( i + 1 ) % n;

            if( arcs[next] == 1 )
            {
                // Next vertex is an arc midpoint.  The arc goes from pts[i] (start)
                // through pts[next] (mid) to the vertex after that (end).
                size_t afterArc = ( next + 1 ) % n;

                const VECTOR2I& start = pts[i];
                const VECTOR2I& mid = pts[next];
                const VECTOR2I& end = pts[afterArc];
                VECTOR2I v1 = mid - start;
                VECTOR2I v2 = end - mid;
                long long cross = static_cast<long long>( v1.x ) * static_cast<long long>( v2.y )
                                - static_cast<long long>( v1.y ) * static_cast<long long>( v2.x );
                bool degenerateArc = ( start == mid ) || ( mid == end ) || ( start == end )
                                   || ( std::llabs( cross ) < 100 );

                if( degenerateArc )
                {
                    addSegment( start, end );
                }
                else
                {
                    addArc( start, mid, end );
                }

                // Advance past the arc midpoint; the end point becomes the new start
                i = afterArc;
            }
            else
            {
                // Straight segment from pts[i] to pts[next]
                addSegment( pts[i], pts[next] );

                i = next;
            }

            // We've wrapped back to the chosen non-arc start -- outline is closed.
            if( i == startIndex )
                break;
        }

        if( steps >= maxSteps )
        {
            wxLogWarning( _( "DipTrace: outline traversal aborted after %zu steps (%zu vertices)" ),
                          steps, n );

            // Fallback: emit a closed polyline through all outline vertices.
            outlinePrims.clear();

            for( size_t v = 0; v < n; v++ )
                addSegment( pts[v], pts[( v + 1 ) % n] );
        }

        for( const OUTLINE_PRIM& prim : outlinePrims )
        {
            if( prim.isArc )
            {
                PCB_SHAPE* arc = new PCB_SHAPE( m_board, SHAPE_T::ARC );
                arc->SetLayer( Edge_Cuts );
                arc->SetStroke( stroke );
                arc->SetArcGeometry( prim.start, prim.mid, prim.end );
                m_board->Add( arc, ADD_MODE::APPEND );
            }
            else
            {
                PCB_SHAPE* seg = new PCB_SHAPE( m_board, SHAPE_T::SEGMENT );
                seg->SetLayer( Edge_Cuts );
                seg->SetStroke( stroke );
                seg->SetStart( prim.start );
                seg->SetEnd( prim.end );
                m_board->Add( seg, ADD_MODE::APPEND );
            }
        }
    }
    else
    {
        // Use bounding box as a closed rectangular outline (4 segments)
        int x1 = ToKiCadCoord( m_bboxXMin );
        int y1 = ToKiCadCoord( m_bboxYMin );
        int x2 = ToKiCadCoord( m_bboxXMax );
        int y2 = ToKiCadCoord( m_bboxYMax );

        VECTOR2I corners[4] = {
            VECTOR2I( x1, y1 ), VECTOR2I( x2, y1 ),
            VECTOR2I( x2, y2 ), VECTOR2I( x1, y2 )
        };

        for( int i = 0; i < 4; i++ )
        {
            PCB_SHAPE* seg = new PCB_SHAPE( m_board, SHAPE_T::SEGMENT );
            seg->SetLayer( Edge_Cuts );
            seg->SetStroke( stroke );
            seg->SetStart( corners[i] );
            seg->SetEnd( corners[( i + 1 ) % 4] );
            m_board->Add( seg, ADD_MODE::APPEND );
        }
    }
}


void PCB_PARSER::CreateFootprint( const DT_COMPONENT& aComp )
{
    if( aComp.isStandaloneVia )
        return;

    FOOTPRINT* footprint = new FOOTPRINT( m_board );

    if( !m_routingAnchorCacheBuilt )
    {
        for( const DT_TRACK_CHAIN& chain : m_trackChains )
        {
            if( chain.netIndex < 0 )
                continue;

            auto& anchors = m_routingAnchorsByNet[chain.netIndex];
            anchors.reserve( anchors.size() + chain.nodes.size() );

            for( const DT_TRACK_NODE& node : chain.nodes )
                anchors.emplace_back( ToKiCadCoord( node.x ), ToKiCadCoord( node.y ) );
        }

        m_routingAnchorCacheBuilt = true;
    }

    footprint->SetReference( aComp.refdes );
    footprint->SetValue( aComp.value );

    if( !aComp.patternName.empty() )
    {
        LIB_ID libId;
        libId.SetLibItemName( aComp.patternName );
        footprint->SetFPID( libId );
    }

    // Add pads while footprint is at origin so SetOrientation/SetPosition
    // will transform them correctly via Rotate()/Move().
    for( size_t padIdx = 0; padIdx < aComp.pads.size(); padIdx++ )
    {
        const DT_PAD& dtPad = aComp.pads[padIdx];
        PAD* pad = new PAD( footprint );

        VECTOR2I padLocal( ToKiCadCoord( dtPad.x ), ToKiCadCoord( dtPad.y ) );
        pad->SetPosition( padLocal );
        pad->SetNumber( dtPad.number );

        VECTOR2I padSize( ToKiCadCoord( dtPad.width ), ToKiCadCoord( dtPad.height ) );
        pad->SetSize( PADSTACK::ALL_LAYERS, padSize );

        if( !dtPad.polygonVertices.empty() )
        {
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
            pad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );

            int anchorDim = std::min( ToKiCadCoord( dtPad.width ),
                                      ToKiCadCoord( dtPad.height ) );
            pad->SetSize( PADSTACK::ALL_LAYERS, { anchorDim, anchorDim } );

            std::vector<VECTOR2I> polyPts;
            polyPts.reserve( dtPad.polygonVertices.size() );

            for( const auto& [vx, vy] : dtPad.polygonVertices )
                polyPts.emplace_back( ToKiCadCoord( vx ), ToKiCadCoord( vy ) );

            pad->AddPrimitivePoly( PADSTACK::ALL_LAYERS, polyPts, 0, true );
        }
        else if( dtPad.style == 2 )
        {
            // DipTrace pad style 2 = Rectangle
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
        }
        else if( dtPad.width == dtPad.height )
        {
            // Styles 0 (ellipse) and 1 (oval) both render as circles when w==h
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        }
        else
        {
            // Style 0 (ellipse) and 1 (oval) with w!=h both map to OVAL (stadium)
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
        }

        int drillW = dtPad.drillWidth;
        int drillH = dtPad.drillHeight;
        bool isSmd = ( dtPad.mountType == 1 );

        if( drillW > 0 && drillH <= 0 )
            drillH = drillW;

        if( drillH > 0 && drillW <= 0 )
            drillW = drillH;

        if( isSmd )
        {
            pad->SetAttribute( PAD_ATTRIB::SMD );
            pad->SetLayerSet( PAD::SMDMask() );
        }
        else
        {
            pad->SetAttribute( PAD_ATTRIB::PTH );
            pad->SetLayerSet( PAD::PTHMask() );

            if( drillW > 0 && drillH > 0 )
            {
                VECTOR2I drill( ToKiCadCoord( drillW ),
                                ToKiCadCoord( drillH ) );
                pad->SetDrillSize( drill );

                if( drillW == drillH )
                    pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
                else
                    pad->SetDrillShape( PAD_DRILL_SHAPE::OBLONG );
            }
        }

        if( dtPad.width != dtPad.height && !aComp.pads.empty() )
        {
            // DipTrace pad post-block orientation class is serialized one step ahead
            // around the footprint perimeter:
            // pad1 uses direct class; padN>1 uses previous pad's class.
            uint8_t orientClass = dtPad.orientClass;

            if( padIdx > 0 )
                orientClass = aComp.pads[padIdx - 1].orientClass;

            if( orientClass == 1 )
                pad->SetOrientation( EDA_ANGLE( 90.0, DEGREES_T ) );
            else
                pad->SetOrientation( EDA_ANGLE( 0.0, DEGREES_T ) );
        }

        if( NETINFO_ITEM* net = ResolveNetByIndex( dtPad.netIndex ) )
            pad->SetNet( net );

        footprint->Add( pad, ADD_MODE::APPEND );
    }

    for( const DT_MOUNT_HOLE& dtHole : aComp.holes )
    {
        PAD* holePad = new PAD( footprint );
        int holeOuter = std::max( dtHole.outerDiameter, dtHole.drillDiameter );
        int holeDrill = dtHole.drillDiameter;

        holePad->SetPosition( VECTOR2I( ToKiCadCoord( dtHole.x ),
                                        ToKiCadCoord( dtHole.y ) ) );
        holePad->SetNumber( wxString() );
        holePad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        holePad->SetSize( PADSTACK::ALL_LAYERS,
                          VECTOR2I( ToKiCadCoord( holeOuter ),
                                    ToKiCadCoord( holeOuter ) ) );
        holePad->SetAttribute( PAD_ATTRIB::NPTH );
        holePad->SetLayerSet( PAD::UnplatedHoleMask() );
        holePad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
        holePad->SetDrillSize( VECTOR2I( ToKiCadCoord( holeDrill ),
                                         ToKiCadCoord( holeDrill ) ) );
        footprint->Add( holePad, ADD_MODE::APPEND );
    }

    // When a footprint carries a Top Assembly body outline, DipTrace treats it as the body
    // graphic and suppresses the redundant Top Silk outline, leaving only silk markers such as
    // the polarity dot. Footprints without an assembly outline (e.g. two-terminal caps) keep
    // their silk outline. Detect the assembly outline up front so the silk lines can be dropped.
    bool hasAssemblyOutline = false;

    for( const DT_FP_SHAPE& s : aComp.shapes )
    {
        if( s.layer == DT_FP_LAYER_TOP_ASSY )
        {
            hasAssemblyOutline = true;
            break;
        }
    }

    // Add footprint outline shapes (silkscreen / fab layer graphics)
    if( !aComp.shapes.empty() && aComp.bboxWidth != 0 && aComp.bboxHeight != 0 )
    {
        int scaleX = aComp.bboxWidth;
        int scaleY = aComp.bboxHeight;

        // The bbox stores the PLACED (rotated) extent while the shape coordinates are canonical
        // (the unrotated pattern frame). A 90/270-degree placement transposes the bbox width and
        // height, so restore the canonical axes before scaling; SetOrientation() then rotates the
        // shapes into place alongside the pads. Without this, a rotated connector's silk lands 90
        // degrees off its pad field. The quarter-turn parity drives the swap exactly.
        if( aComp.hasPlacementQuarterTurns )
        {
            if( ( ( aComp.placementQuarterTurns % 2 ) + 2 ) % 2 == 1 )
                std::swap( scaleX, scaleY );
        }
        else
        {
            // Legacy fallback when the placement metadata is absent: detect a transposed bbox from
            // an extreme physical aspect ratio relative to the normalized shape extents.
            int minSX = INT_MAX, maxSX = INT_MIN;
            int minSY = INT_MAX, maxSY = INT_MIN;

            for( const DT_FP_SHAPE& s : aComp.shapes )
            {
                minSX = std::min( { minSX, s.x1, s.x2 } );
                maxSX = std::max( { maxSX, s.x1, s.x2 } );
                minSY = std::min( { minSY, s.y1, s.y2 } );
                maxSY = std::max( { maxSY, s.y1, s.y2 } );
            }

            int shapeXRange = maxSX - minSX;
            int shapeYRange = maxSY - minSY;

            if( shapeXRange > 0 && shapeYRange > 0 )
            {
                double physX = static_cast<double>( shapeXRange ) * std::abs( scaleX )
                               / FP_SHAPE_NORM_RANGE;
                double physY = static_cast<double>( shapeYRange ) * std::abs( scaleY )
                               / FP_SHAPE_NORM_RANGE;
                double aspect = physX / physY;

                if( aspect < 0.2 || aspect > 5.0 )
                    std::swap( scaleX, scaleY );
            }
        }

        // Default line width when shape record uses the sentinel value
        static constexpr int DEFAULT_LINE_WIDTH_DT = 3000;  // ~0.1mm

        auto scaleShapeCoord = [&]( int aShapeVal, int aBboxDim ) -> int
        {
            return ToKiCadCoord(
                    static_cast<int>( static_cast<int64_t>( aShapeVal ) * aBboxDim
                                      / FP_SHAPE_NORM_RANGE ) );
        };

        for( const DT_FP_SHAPE& dtShape : aComp.shapes )
        {
            PCB_SHAPE* shape = new PCB_SHAPE( footprint );

            int lineWidth = ( dtShape.width == FP_SHAPE_DEFAULT_WIDTH || dtShape.width <= 0 )
                                    ? ToKiCadCoord( DEFAULT_LINE_WIDTH_DT )
                                    : ToKiCadCoord( dtShape.width );

            shape->SetWidth( lineWidth );

            // Drop the redundant Top Silk OUTLINE when the assembly outline already describes the
            // body, but keep silk fill markers such as the polarity dot. Footprints without an
            // assembly outline (e.g. two-terminal caps) keep their silk outline.
            if( hasAssemblyOutline && dtShape.layer == DT_FP_LAYER_TOP_SILK
                && dtShape.type != DT_SHAPE_FILLOBROUND )
            {
                delete shape;
                continue;
            }

            // Map the DipTrace footprint-graphic layer enum to a top-relative KiCad layer.
            // Bottom-side footprints are mirrored wholesale by Flip() below, so always assign
            // the front layer here; assigning the back layer too would double-flip it.
            PCB_LAYER_ID shapeLayer;

            switch( dtShape.layer )
            {
            case DT_FP_LAYER_TOP_ASSY:      shapeLayer = F_Fab;   break;
            case DT_FP_LAYER_TOP_MASK:      shapeLayer = F_Mask;  break;
            case DT_FP_LAYER_TOP_PASTE:     shapeLayer = F_Paste; break;
            case DT_FP_LAYER_TOP_KEEPOUT:   shapeLayer = F_CrtYd; break;
            case DT_FP_LAYER_TOP_COURTYARD: shapeLayer = F_CrtYd; break;
            case DT_FP_LAYER_TOP_OUTLINE:   shapeLayer = F_Fab;   break;
            case DT_FP_LAYER_TOP_SILK:
            default:                        shapeLayer = F_SilkS; break;
            }

            shape->SetLayer( shapeLayer );

            VECTOR2I p1( scaleShapeCoord( dtShape.x1, scaleX ),
                         scaleShapeCoord( dtShape.y1, scaleY ) );
            VECTOR2I p2( scaleShapeCoord( dtShape.x2, scaleX ),
                         scaleShapeCoord( dtShape.y2, scaleY ) );

            if( dtShape.type == DT_SHAPE_RECT )
            {
                delete shape;

                const VECTOR2I corners[4] = {
                    VECTOR2I( p1.x, p1.y ),
                    VECTOR2I( p2.x, p1.y ),
                    VECTOR2I( p2.x, p2.y ),
                    VECTOR2I( p1.x, p2.y )
                };

                for( int i = 0; i < 4; i++ )
                {
                    PCB_SHAPE* edge = new PCB_SHAPE( footprint );
                    edge->SetWidth( lineWidth );
                    edge->SetLayer( shapeLayer );
                    edge->SetShape( SHAPE_T::SEGMENT );
                    edge->SetStart( corners[i] );
                    edge->SetEnd( corners[( i + 1 ) % 4] );
                    footprint->Add( edge, ADD_MODE::APPEND );
                }

                continue;
            }
            else if( dtShape.type == DT_SHAPE_LINE )
            {
                shape->SetShape( SHAPE_T::SEGMENT );
                shape->SetStart( p1 );
                shape->SetEnd( p2 );
            }
            else if( dtShape.type == DT_SHAPE_CIRCLE )
            {
                shape->SetShape( SHAPE_T::CIRCLE );
                VECTOR2I center( ( p1.x + p2.x ) / 2, ( p1.y + p2.y ) / 2 );
                int radius = ( p2 - p1 ).EuclideanNorm() / 2;
                shape->SetCenter( center );
                shape->SetEnd( VECTOR2I( center.x + radius, center.y ) );
            }
            else if( dtShape.type == DT_SHAPE_ARC )
            {
                VECTOR2I mid( scaleShapeCoord( dtShape.midX, scaleX ),
                              scaleShapeCoord( dtShape.midY, scaleY ) );

                // A collinear midpoint yields a degenerate arc whose centre runs off to infinity
                // (DipTrace stores some straight edges as a zero-bulge arc). Such a centre overflows
                // the integer rotation math on a non-cardinal placement angle, so emit a segment.
                int64_t cross = static_cast<int64_t>( p2.x - p1.x ) * ( mid.y - p1.y )
                                - static_cast<int64_t>( p2.y - p1.y ) * ( mid.x - p1.x );
                int64_t chordSq = static_cast<int64_t>( p2.x - p1.x ) * ( p2.x - p1.x )
                                  + static_cast<int64_t>( p2.y - p1.y ) * ( p2.y - p1.y );

                if( chordSq == 0 || std::abs( cross ) * 1000 < chordSq )
                {
                    shape->SetShape( SHAPE_T::SEGMENT );
                    shape->SetStart( p1 );
                    shape->SetEnd( p2 );
                }
                else
                {
                    shape->SetShape( SHAPE_T::ARC );
                    shape->SetArcGeometry( p1, mid, p2 );
                }
            }
            else if( dtShape.type == DT_SHAPE_FILLOBROUND )
            {
                // A small filled obround marker (the diode cathode / pin-1 dot). The two points
                // are the bounding box corners; render it as a filled circle of that diameter.
                VECTOR2I center( ( p1.x + p2.x ) / 2, ( p1.y + p2.y ) / 2 );
                int radius = std::min( std::abs( p2.x - p1.x ), std::abs( p2.y - p1.y ) ) / 2;

                shape->SetShape( SHAPE_T::CIRCLE );
                shape->SetCenter( center );
                shape->SetEnd( VECTOR2I( center.x + radius, center.y ) );
                shape->SetFilled( true );
                shape->SetWidth( 0 );
            }
            else
            {
                delete shape;
                continue;
            }

            footprint->Add( shape, ADD_MODE::APPEND );
        }
    }

    VECTOR2I pos( ToKiCadCoord( aComp.positionX ), ToKiCadCoord( aComp.positionY ) );

    // Prefer the exact placement-section angle; otherwise snap to the metadata quarter turn.
    double orientationDeg;

    if( aComp.hasPlacementAngle )
    {
        // The stored angle is cumulative (it can exceed a full turn, e.g. 630 or 990 degrees);
        // reduce to a single turn for placement.
        orientationDeg = std::fmod( aComp.placementAngleDeg, 360.0 );

        if( orientationDeg < 0.0 )
            orientationDeg += 360.0;

        // The angle is recovered from a rounded-radian field, so a quarter-turn lands a hair off
        // an exact multiple of 90 (e.g. 269.9994). Snap those back to the cardinal value so the
        // footprint rotation takes the exact axis-swap path rather than a floating-point rotation.
        double nearest90 = std::round( orientationDeg / 90.0 ) * 90.0;

        if( std::abs( orientationDeg - nearest90 ) < 0.02 )
            orientationDeg = std::fmod( nearest90, 360.0 );
    }
    else
    {
        int orientationQuarterTurns = aComp.hasPlacementQuarterTurns
                                              ? aComp.placementQuarterTurns
                                              : static_cast<int>( std::lround(
                                                      ToKiCadAngleDeg( aComp.rotation ) / 90.0 ) );
        orientationDeg = ( ( orientationQuarterTurns % 4 ) + 4 ) % 4 * 90.0;
    }

    if( ShouldDumpFootprintOrientation( aComp.refdes ) )
    {
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: fp-orient ref=%s pat=%s qturn=%d hasQ=%d exact=%d chosen=%.2f" ), // format:allow
                    aComp.refdes, aComp.patternName, aComp.placementQuarterTurns,
                    aComp.hasPlacementQuarterTurns ? 1 : 0, aComp.hasPlacementAngle ? 1 : 0, orientationDeg );
    }

    // Set layer before orientation so bottom-side flip is handled first
    if( aComp.layer == 1 )
        footprint->Flip( VECTOR2I( 0, 0 ), FLIP_DIRECTION::TOP_BOTTOM );

    footprint->SetOrientation( EDA_ANGLE( orientationDeg, DEGREES_T ) );
    footprint->SetPosition( pos );

    // Tail offsets appear to be board-global Y offsets, not local-footprint offsets.
    // Apply after footprint placement so orientation does not rotate text offsets.
    if( aComp.hasTailData )
    {
        VECTOR2I fpPos = footprint->GetPosition();

        footprint->Reference().SetPosition(
                fpPos + VECTOR2I( 0, ToKiCadCoord( aComp.refdesYOffset ) ) );
        footprint->Reference().SetVisible( aComp.refdesVisible );

        footprint->Value().SetPosition(
                fpPos + VECTOR2I( 0, ToKiCadCoord( aComp.valueYOffset ) ) );
        footprint->Value().SetVisible( aComp.valueVisible );
    }

    m_board->Add( footprint, ADD_MODE::APPEND );
}


void PCB_PARSER::CreateTextObject( const DT_TEXT_OBJECT& aText )
{
    if( aText.text.empty() )
        return;

    PCB_TEXT* text = new PCB_TEXT( m_board );
    text->SetText( aText.text );
    PCB_LAYER_ID textLayer = MapLayer( aText.layer );

    if( textLayer == UNDEFINED_LAYER )
        textLayer = F_SilkS;

    text->SetLayer( textLayer );

    // Position at the center of the text bounding box
    int cx = ToKiCadCoord( ( aText.x1 + aText.x2 ) / 2 );
    int cy = ToKiCadCoord( ( aText.y1 + aText.y2 ) / 2 );
    text->SetPosition( VECTOR2I( cx, cy ) );

    int height = std::abs( ToKiCadCoord( aText.y2 - aText.y1 ) );

    if( height > 0 )
        text->SetTextSize( VECTOR2I( height, height ) );
    else
        text->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );

    if( aText.lineWidth > 0 )
        text->SetTextThickness( ToKiCadCoord( aText.lineWidth ) );
    else
        text->SetTextThickness( pcbIUScale.mmToIU( 0.15 ) );

    m_board->Add( text, ADD_MODE::APPEND );
}


NETINFO_ITEM* PCB_PARSER::ResolveNetByIndex( int aDipTraceNetIndex ) const
{
    if( aDipTraceNetIndex < 0 )
        return nullptr;

    auto it = m_kicadNetByDipTraceIndex.find( aDipTraceNetIndex );

    if( it != m_kicadNetByDipTraceIndex.end() )
        return it->second;

    return nullptr;
}


const DT_NET* PCB_PARSER::ResolveDipTraceNetByIndex( int aDipTraceNetIndex ) const
{
    if( aDipTraceNetIndex < 0 )
        return nullptr;

    auto it = m_dipTraceNetByIndex.find( aDipTraceNetIndex );

    if( it != m_dipTraceNetByIndex.end() )
        return it->second;

    return nullptr;
}


void PCB_PARSER::CreateNets()
{
    m_kicadNetByDipTraceIndex.clear();
    m_dipTraceNetByIndex.clear();

    for( const DT_NET& net : m_nets )
    {
        wxString netName = net.name;

        if( netName.empty() )
            netName = wxString::Format( wxS( "DipTrace_Net_%d" ), net.index );

        NETINFO_ITEM* netinfo = m_board->FindNet( netName );

        if( !netinfo )
        {
            netinfo = new NETINFO_ITEM( m_board, netName );
            m_board->Add( netinfo, ADD_MODE::APPEND );
        }

        if( net.index >= 0 )
        {
            auto [it, inserted] = m_kicadNetByDipTraceIndex.emplace( net.index, netinfo );

            if( !inserted && it->second != netinfo )
            {
                wxLogWarning( _( "DipTrace: net index %d maps to multiple net names (%s, %s)" ),
                              net.index, it->second->GetNetname(), netinfo->GetNetname() );
            }

            auto [dtIt, dtInserted] = m_dipTraceNetByIndex.emplace( net.index, &net );

            if( !dtInserted && dtIt->second != &net )
            {
                wxLogWarning( _( "DipTrace: duplicate net metadata for net index %d" ),
                              net.index );
            }
        }
    }
}


void PCB_PARSER::CreateStandaloneVias()
{
    int created = 0;
    int duplicates = 0;
    int missingGeometry = 0;
    std::set<std::tuple<int, int, int>> createdKeys;

    auto hasBoardViaAt = [&]( const VECTOR2I& aPos, int aNetCode ) -> bool
    {
        for( const PCB_TRACK* track : m_board->Tracks() )
        {
            if( track->Type() != PCB_VIA_T )
                continue;

            const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

            if( via->GetPosition() == aPos && via->GetNetCode() == aNetCode )
                return true;
        }

        return false;
    };

    for( const DT_COMPONENT& comp : m_components )
    {
        if( !comp.isStandaloneVia )
            continue;

        int viaX = comp.positionX;
        int viaY = comp.positionY;
        int viaOuter = std::max( comp.bboxWidth, comp.padWidthHint );
        int viaDrill = std::max( comp.drillWidthHint, comp.drillHeightHint );
        int netIndex = -1;

        if( !comp.pads.empty() )
        {
            const DT_PAD& pad = comp.pads.front();
            viaX += pad.x;
            viaY += pad.y;
            netIndex = pad.netIndex;

            if( pad.width > 0 )
                viaOuter = pad.width;

            if( pad.drillWidth > 0 )
                viaDrill = pad.drillWidth;
            else if( pad.drillHeight > 0 )
                viaDrill = pad.drillHeight;
        }

        if( viaOuter <= 0 || viaDrill <= 0 )
        {
            missingGeometry++;
            continue;
        }

        VECTOR2I viaPos( ToKiCadCoord( viaX ), ToKiCadCoord( viaY ) );
        NETINFO_ITEM* net = ResolveNetByIndex( netIndex );
        int netCode = net ? net->GetNetCode() : -1;
        std::tuple<int, int, int> key( viaPos.x, viaPos.y, netCode );

        if( createdKeys.find( key ) != createdKeys.end() || hasBoardViaAt( viaPos, netCode ) )
        {
            duplicates++;
            continue;
        }

        PCB_VIA* via = new PCB_VIA( m_board );
        via->SetPosition( viaPos );
        via->SetWidth( ToKiCadCoord( viaOuter ) );
        via->SetDrill( ToKiCadCoord( viaDrill ) );
        via->SetLayerPair( F_Cu, B_Cu );
        via->SetViaType( VIATYPE::THROUGH );

        if( net )
            via->SetNet( net );

        m_board->Add( via, ADD_MODE::APPEND );
        createdKeys.insert( key );
        created++;
    }

    if( created > 0 || duplicates > 0 || missingGeometry > 0 )
    {
        wxLogTrace( traceDiptraceIo,
                    wxT( "DipTrace: created %d standalone vias from component records "
                         "(duplicates=%d, missingGeometry=%d)" ),
                    created, duplicates, missingGeometry );
    }
}


void PCB_PARSER::CreateTracksAndVias()
{
    int trackCount = 0;
    int viaCount = 0;
    int missingChainNets = 0;
    int nonCopperTrackSkips = 0;
    int nonCopperViaSkips = 0;
    int inferredLayerChangeVias = 0;
    int explicitViaStyleVias = 0;
    int duplicateViaSkips = 0;
    int crossNetViaSkips = 0;

    struct POS_NET_KEY
    {
        int net = -1;
        int x = 0;
        int y = 0;

        bool operator==( const POS_NET_KEY& aOther ) const
        {
            return net == aOther.net && x == aOther.x && y == aOther.y;
        }
    };

    struct POS_NET_HASH
    {
        size_t operator()( const POS_NET_KEY& aKey ) const
        {
            size_t h1 = static_cast<size_t>( static_cast<uint32_t>( aKey.net ) );
            size_t h2 = static_cast<size_t>( static_cast<uint32_t>( aKey.x ) );
            size_t h3 = static_cast<size_t>( static_cast<uint32_t>( aKey.y ) );
            return h1 ^ ( h2 * 0x9e3779b1U ) ^ ( h3 * 0x85ebca6bU );
        }
    };

    auto makeKey = []( int aNet, int aX, int aY ) -> POS_NET_KEY
    {
        POS_NET_KEY key;
        key.net = aNet;
        key.x = aX;
        key.y = aY;
        return key;
    };

    struct POS_KEY
    {
        int x = 0;
        int y = 0;

        bool operator==( const POS_KEY& aOther ) const
        {
            return x == aOther.x && y == aOther.y;
        }
    };

    struct POS_HASH
    {
        size_t operator()( const POS_KEY& aKey ) const
        {
            size_t h1 = static_cast<size_t>( static_cast<uint32_t>( aKey.x ) );
            size_t h2 = static_cast<size_t>( static_cast<uint32_t>( aKey.y ) );
            return ( h1 * 0x9e3779b1U ) ^ ( h2 * 0x85ebca6bU );
        }
    };

    auto makePosKey = []( int aX, int aY ) -> POS_KEY
    {
        POS_KEY key;
        key.x = aX;
        key.y = aY;
        return key;
    };

    auto resolveCopperLayer = [&]( int aDipTraceLayer ) -> PCB_LAYER_ID
    {
        PCB_LAYER_ID layer = MapCopperLayer( aDipTraceLayer );

        if( layer == UNDEFINED_LAYER )
            layer = MapLayer( aDipTraceLayer );

        if( !IsCopperLayer( layer ) )
            return UNDEFINED_LAYER;

        return layer;
    };

    std::unordered_map<POS_NET_KEY, std::set<int>, POS_NET_HASH> layersByNetPos;
    std::unordered_set<POS_NET_KEY, POS_NET_HASH>                createdVias;
    std::unordered_map<POS_KEY, std::set<int>, POS_HASH>         netsByPos;

    for( const DT_TRACK_CHAIN& chain : m_trackChains )
    {
        for( const DT_TRACK_NODE& node : chain.nodes )
        {
            layersByNetPos[makeKey( chain.netIndex, node.x, node.y )].insert( node.layer );

            if( chain.netIndex >= 0 )
                netsByPos[makePosKey( node.x, node.y )].insert( chain.netIndex );
        }
    }

    std::shared_ptr<NETCLASS> defNetclass =
            m_board->GetDesignSettings().m_NetSettings->GetDefaultNetclass();

    for( const DT_TRACK_CHAIN& chain : m_trackChains )
    {
        NETINFO_ITEM* net = ResolveNetByIndex( chain.netIndex );
        const DT_NET* dtNet = ResolveDipTraceNetByIndex( chain.netIndex );

        if( !net && chain.netIndex >= 0 )
            missingChainNets++;

        // Create track segments between consecutive nodes
        for( size_t i = 0; i + 1 < chain.nodes.size(); i++ )
        {
            const DT_TRACK_NODE& n0 = chain.nodes[i];
            const DT_TRACK_NODE& n1 = chain.nodes[i + 1];

            PCB_TRACK* track = new PCB_TRACK( m_board );

            track->SetStart( VECTOR2I( ToKiCadCoord( n0.x ), ToKiCadCoord( n0.y ) ) );
            track->SetEnd( VECTOR2I( ToKiCadCoord( n1.x ), ToKiCadCoord( n1.y ) ) );
            int segWidth = ( n1.width > 0 ) ? n1.width : n0.width;
            track->SetWidth( ToKiCadCoord( segWidth ) );
            PCB_LAYER_ID trackLayer = resolveCopperLayer( n1.layer );

            if( trackLayer == UNDEFINED_LAYER )
            {
                nonCopperTrackSkips++;
                delete track;
                continue;
            }

            track->SetLayer( trackLayer );

            if( net )
                track->SetNet( net );

            m_board->Add( track, ADD_MODE::APPEND );
            trackCount++;
        }

        for( size_t nodeIdx = 0; nodeIdx < chain.nodes.size(); nodeIdx++ )
        {
            const DT_TRACK_NODE& node = chain.nodes[nodeIdx];
            POS_NET_KEY key = makeKey( chain.netIndex, node.x, node.y );
            auto posIt = layersByNetPos.find( key );
            bool explicitVia = node.hasVia;
            bool explicitViaStyle = node.viaStyleIdx >= 0;

            if( !explicitVia )
                continue;

            auto netPosIt = netsByPos.find( makePosKey( node.x, node.y ) );

            if( netPosIt != netsByPos.end() && netPosIt->second.size() > 1 )
            {
                crossNetViaSkips++;
                continue;
            }

            if( createdVias.find( key ) != createdVias.end() )
            {
                duplicateViaSkips++;
                continue;
            }

            if( explicitViaStyle )
                explicitViaStyleVias++;

            int viaOuterDiam = node.viaOuterDiam;
            int viaDrillDiam = node.viaDrillDiam;
            PCB_LAYER_ID styleViaLayerA = UNDEFINED_LAYER;
            PCB_LAYER_ID styleViaLayerB = UNDEFINED_LAYER;

            if( node.viaStyleIdx >= 0 && node.viaStyleIdx < static_cast<int>( m_viaStyles.size() ) )
            {
                const DT_VIA_STYLE& viaStyle = m_viaStyles[node.viaStyleIdx];
                viaOuterDiam = viaStyle.outerDiameter;
                viaDrillDiam = viaStyle.drillDiameter;

                PCB_LAYER_ID l1 = resolveCopperLayer( viaStyle.layer1 );
                PCB_LAYER_ID l2 = resolveCopperLayer( viaStyle.layer2 );

                if( l1 != UNDEFINED_LAYER && l2 != UNDEFINED_LAYER && l1 != l2 )
                {
                    if( CopperLayerToOrdinal( l1 ) <= CopperLayerToOrdinal( l2 ) )
                    {
                        styleViaLayerA = l1;
                        styleViaLayerB = l2;
                    }
                    else
                    {
                        styleViaLayerA = l2;
                        styleViaLayerB = l1;
                    }
                }
            }

            if( dtNet )
            {
                if( viaOuterDiam <= 0 )
                    viaOuterDiam = dtNet->defaultViaOuterDiam;

                if( viaDrillDiam <= 0 )
                    viaDrillDiam = dtNet->defaultViaDrillDiam;
            }

            int viaWidthIU = ( viaOuterDiam > 0 ) ? ToKiCadCoord( viaOuterDiam ) : 0;
            int viaDrillIU = ( viaDrillDiam > 0 ) ? ToKiCadCoord( viaDrillDiam ) : 0;

            if( viaWidthIU <= 0 && defNetclass )
                viaWidthIU = defNetclass->GetViaDiameter();

            if( viaDrillIU <= 0 && defNetclass )
                viaDrillIU = defNetclass->GetViaDrill();

            if( viaWidthIU <= 0 )
                viaWidthIU = pcbIUScale.mmToIU( 0.6 );

            PCB_VIA* via = new PCB_VIA( m_board );

            via->SetPosition( VECTOR2I( ToKiCadCoord( node.x ), ToKiCadCoord( node.y ) ) );
            via->SetWidth( viaWidthIU );

            if( viaDrillIU > 0 )
                via->SetDrill( viaDrillIU );

            auto ordinalToLayer = [&]( size_t aOrdinal ) -> PCB_LAYER_ID
            {
                int copperCount = m_board->GetCopperLayerCount();

                if( aOrdinal == 0 )
                    return F_Cu;

                if( aOrdinal >= static_cast<size_t>( copperCount - 1 ) )
                    return B_Cu;

                int innerIdx = static_cast<int>( aOrdinal ) - 1;

                if( innerIdx < 0 || innerIdx > 29 )
                    return UNDEFINED_LAYER;

                return static_cast<PCB_LAYER_ID>( In1_Cu + innerIdx * 2 );
            };

            bool haveLayerRange = false;
            size_t minOrd = 0;
            size_t maxOrd = 0;

            auto accumulateDipLayer = [&]( int aDipLayer )
            {
                PCB_LAYER_ID layer = resolveCopperLayer( aDipLayer );

                if( layer == UNDEFINED_LAYER )
                    return;

                size_t ord = CopperLayerToOrdinal( layer );

                if( !haveLayerRange )
                {
                    minOrd = ord;
                    maxOrd = ord;
                    haveLayerRange = true;
                }
                else
                {
                    minOrd = std::min( minOrd, ord );
                    maxOrd = std::max( maxOrd, ord );
                }
            };

            accumulateDipLayer( node.layer );

            if( nodeIdx > 0 )
                accumulateDipLayer( chain.nodes[nodeIdx - 1].layer );

            if( nodeIdx + 1 < chain.nodes.size() )
                accumulateDipLayer( chain.nodes[nodeIdx + 1].layer );

            if( posIt != layersByNetPos.end() )
            {
                for( int dipLayer : posIt->second )
                    accumulateDipLayer( dipLayer );
            }

            PCB_LAYER_ID viaLayerA = F_Cu;
            PCB_LAYER_ID viaLayerB = B_Cu;

            if( styleViaLayerA != UNDEFINED_LAYER && styleViaLayerB != UNDEFINED_LAYER )
            {
                viaLayerA = styleViaLayerA;
                viaLayerB = styleViaLayerB;
            }
            else if( haveLayerRange && minOrd < maxOrd )
            {
                PCB_LAYER_ID low = ordinalToLayer( minOrd );
                PCB_LAYER_ID high = ordinalToLayer( maxOrd );

                if( low != UNDEFINED_LAYER && high != UNDEFINED_LAYER && low != high )
                {
                    viaLayerA = low;
                    viaLayerB = high;
                }
            }

            via->SetLayerPair( viaLayerA, viaLayerB );

            if( viaLayerA == F_Cu && viaLayerB == B_Cu )
                via->SetViaType( VIATYPE::THROUGH );
            else if( viaLayerA == F_Cu || viaLayerB == B_Cu )
                via->SetViaType( VIATYPE::BLIND );
            else
                via->SetViaType( VIATYPE::BURIED );

            if( net )
                via->SetNet( net );

            m_board->Add( via, ADD_MODE::APPEND );
            createdVias.insert( key );
            viaCount++;
        }
    }

    wxLogTrace( traceDiptraceIo,
                wxT( "DipTrace: created %d tracks and %d vias (%d chains with unresolved nets, "
                     "%d non-copper tracks skipped, %d non-copper vias skipped, "
                     "%d style vias, %d inferred layer-change vias, %d duplicate vias skipped, "
                     "%d cross-net vias skipped)" ),
                trackCount, viaCount, missingChainNets, nonCopperTrackSkips, nonCopperViaSkips, explicitViaStyleVias,
                inferredLayerChangeVias, duplicateViaSkips, crossNetViaSkips );
}


void PCB_PARSER::CreateZones()
{
    std::unordered_map<int, int> zoneSpokeModeByDipNet;
    std::unordered_set<int>       zoneSpokeModeConflicts;

    for( const DT_ZONE& dtZone : m_zones )
    {
        if( dtZone.outline.size() < 3 )
            continue;

        ZONE* zone = new ZONE( m_board );

        PCB_LAYER_ID zoneLayer = MapCopperLayer( dtZone.layer );

        if( zoneLayer == UNDEFINED_LAYER )
            zoneLayer = MapLayer( dtZone.layer );

        if( zoneLayer == UNDEFINED_LAYER )
            zoneLayer = F_Cu;

        zone->SetLayer( zoneLayer );
        zone->SetAssignedPriority( dtZone.priority );

        if( dtZone.clearance > 0 )
            zone->SetLocalClearance( ToKiCadCoord( dtZone.clearance ) );

        if( dtZone.minWidth > 0 )
        {
            zone->SetMinThickness( std::max( ToKiCadCoord( dtZone.minWidth ),
                                             static_cast<int>( ZONE_THICKNESS_MIN_VALUE_MM
                                                                * pcbIUScale.IU_PER_MM ) ) );
        }

        if( dtZone.spokeWidth > 0 )
            zone->SetThermalReliefSpokeWidth( ToKiCadCoord( dtZone.spokeWidth ) );

        if( dtZone.spokeMode == 0 )
        {
            zone->SetPadConnection( ZONE_CONNECTION::FULL );
        }
        else if( dtZone.spokeMode > 0 && dtZone.smdSpokeMode == 0 )
        {
            // DipTrace encodes independent spoke modes for THT and SMD objects.
            // When THT uses thermal spokes but SMD is direct, KiCad's closest
            // representation is THT thermal (SMD solid).
            zone->SetPadConnection( ZONE_CONNECTION::THT_THERMAL );
        }
        else if( dtZone.spokeMode > 0 )
        {
            zone->SetPadConnection( ZONE_CONNECTION::THERMAL );
        }

        // DipTrace exposes three independent island-removal toggles:
        // - Minimum Area (IslandRegion)
        // - Internal (IslandInternal)
        // - Unconnected (IslandConnection)
        // KiCad has coarser zone-level modes (ALWAYS/NEVER/AREA), so map to
        // the closest deterministic representation.
        if( dtZone.islandInternal || dtZone.islandConnection )
        {
            zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::ALWAYS );
        }
        else if( dtZone.islandRegion )
        {
            zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::AREA );

            if( dtZone.minimumArea > 0 )
            {
                long long minIslandLinearIU = static_cast<long long>( ToKiCadCoord( dtZone.minimumArea ) );
                zone->SetMinIslandArea( minIslandLinearIU * minIslandLinearIU );
            }
        }
        else
        {
            zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );
        }

        if( NETINFO_ITEM* netinfo = ResolveNetByIndex( dtZone.netIndex ) )
            zone->SetNet( netinfo );

        SHAPE_POLY_SET outline;
        outline.NewOutline();

        for( const auto& [x, y] : dtZone.outline )
            outline.Append( ToKiCadCoord( x ), ToKiCadCoord( y ) );

        zone->AddPolygon( outline.COutline( 0 ) );

        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     ZONE::GetDefaultHatchPitch(), true );

        m_board->Add( zone, ADD_MODE::APPEND );

        if( dtZone.netIndex >= 0 && dtZone.spokeMode >= 0 )
        {
            auto [it, inserted] = zoneSpokeModeByDipNet.emplace( dtZone.netIndex, dtZone.spokeMode );

            if( !inserted && it->second != dtZone.spokeMode )
                zoneSpokeModeConflicts.insert( dtZone.netIndex );
        }
    }

    // KiCad has no zone-level thermal spoke angle; apply the parsed DipTrace
    // spoke mode to through-hole pads by net when the mode is unambiguous.
    std::unordered_map<int, int> spokeModeByNetCode;

    for( const auto& [dipNetIdx, spokeMode] : zoneSpokeModeByDipNet )
    {
        if( zoneSpokeModeConflicts.count( dipNetIdx ) )
            continue;

        auto netIt = m_kicadNetByDipTraceIndex.find( dipNetIdx );

        if( netIt == m_kicadNetByDipTraceIndex.end() || !netIt->second )
            continue;

        spokeModeByNetCode[netIt->second->GetNetCode()] = spokeMode;
    }

    int adjustedPadThermalAngles = 0;

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::SMD )
                continue;

            auto modeIt = spokeModeByNetCode.find( pad->GetNetCode() );

            if( modeIt == spokeModeByNetCode.end() )
                continue;

            int spokeMode = modeIt->second;

            if( spokeMode == 1 || spokeMode == 4 )
            {
                pad->SetThermalSpokeAngle( ANGLE_90 );
                adjustedPadThermalAngles++;
            }
            else if( spokeMode == 2 || spokeMode == 3 )
            {
                pad->SetThermalSpokeAngle( ANGLE_45 );
                adjustedPadThermalAngles++;
            }
        }
    }

    if( adjustedPadThermalAngles > 0 )
    {
        wxLogTrace( traceDiptraceIo, wxT( "DipTrace: applied thermal spoke angle overrides to %d pads" ),
                    adjustedPadThermalAngles );
    }
}


void PCB_PARSER::CreatePlaneZones()
{
    // DipTrace negative/solid planes are described at the layer level (CopperLayers <Lay
    // Type="Plane" NetId=..>) rather than as stored CopperPour records, so they never appear in
    // m_zones. Synthesize a board-outline-bounded ZONE on each plane layer, tied to the plane net.
    // Build the bounding polygon once: prefer the real outline, else fall back to the board bbox
    // rectangle (mirroring CreateBoardOutline so plane fills are not lost on bbox-only boards).
    SHAPE_POLY_SET planeOutline;
    planeOutline.NewOutline();

    if( m_outline.size() >= 3 )
    {
        for( const DT_VERTEX& v : m_outline )
            planeOutline.Append( ToKiCadCoord( v.x ), ToKiCadCoord( v.y ) );
    }
    else if( m_bboxXMin != 0 || m_bboxXMax != 0 )
    {
        int x1 = ToKiCadCoord( m_bboxXMin );
        int y1 = ToKiCadCoord( m_bboxYMin );
        int x2 = ToKiCadCoord( m_bboxXMax );
        int y2 = ToKiCadCoord( m_bboxYMax );

        planeOutline.Append( x1, y1 );
        planeOutline.Append( x2, y1 );
        planeOutline.Append( x2, y2 );
        planeOutline.Append( x1, y2 );
    }

    if( planeOutline.OutlineCount() == 0 || planeOutline.COutline( 0 ).PointCount() < 3 )
        return;

    for( const DT_LAYER& layer : m_layers )
    {
        if( layer.type != 1 || layer.planeNetIndex < 0 )
            continue;

        PCB_LAYER_ID kiLayer = MapCopperLayer( layer.index );

        if( kiLayer == UNDEFINED_LAYER )
            continue;

        NETINFO_ITEM* netinfo = ResolveNetByIndex( layer.planeNetIndex );

        if( !netinfo )
        {
            wxLogTrace( traceDiptraceIo,
                        wxT( "DipTrace: plane layer %d references unresolved net index %d; skipping" ),
                        layer.index, layer.planeNetIndex );
            continue;
        }

        SHAPE_POLY_SET outline = planeOutline;

        ZONE* zone = new ZONE( m_board );
        zone->SetLayer( kiLayer );
        zone->SetNet( netinfo );
        zone->SetAssignedPriority( 0 );
        zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );

        // Mark this as a synthesized plane fill (not a stored CopperPour) so consumers can tell
        // it apart from explicit pours.
        zone->SetZoneName( wxT( "DipTrace Plane" ) );

        zone->AddPolygon( outline.COutline( 0 ) );
        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     ZONE::GetDefaultHatchPitch(), true );

        m_board->Add( zone, ADD_MODE::APPEND );

        wxLogTrace( traceDiptraceIo,
                    wxT( "DipTrace: synthesized plane zone on layer %d net '%s' (dt net %d)" ),
                    layer.index, netinfo->GetNetname(), layer.planeNetIndex );
    }
}


wxString PCB_PARSER::GenerateDesignRules() const
{
    // DipTrace's per-zone board-edge clearance is board-constant in practice;
    // collapse it to the largest value seen so a single edge_clearance rule covers
    // every pour.
    int edgeClearanceIU = 0;

    // ViaDirect connects vias to the pour solidly instead of through thermal
    // spokes. KiCad has no per-zone via setting, so express it per net.
    std::map<wxString, bool> viaDirectNets;

    for( const DT_ZONE& zone : m_zones )
    {
        if( zone.boardClearance > 0 )
            edgeClearanceIU = std::max( edgeClearanceIU, ToKiCadCoord( zone.boardClearance ) );

        if( zone.viaDirect && zone.netIndex >= 0 )
        {
            if( NETINFO_ITEM* net = ResolveNetByIndex( zone.netIndex ) )
            {
                if( !net->GetNetname().IsEmpty() )
                    viaDirectNets[net->GetNetname()] = true;
            }
        }
    }

    if( edgeClearanceIU <= 0 && viaDirectNets.empty() )
        return wxEmptyString;

    wxString rules = wxT( "(version 1)\n" );

    if( edgeClearanceIU > 0 )
    {
        wxString mm = wxString::FromUTF8( FormatDouble2Str( pcbIUScale.IUTomm( edgeClearanceIU ) ) );

        rules += wxString::Format( wxT( "\n(rule \"DipTrace zone board clearance\"\n" )
                                   wxT( "  (condition \"A.Type == 'Zone'\")\n" )
                                   wxT( "  (constraint edge_clearance (min %smm)))\n" ),
                                   mm );
    }

    for( const auto& [netName, direct] : viaDirectNets )
    {
        // Net names may contain the single quotes that delimit the NetName literal.
        wxString escaped = netName;
        escaped.Replace( wxT( "'" ), wxT( "\\'" ) );

        rules += wxString::Format( wxT( "\n(rule \"DipTrace via direct %s\"\n" )
                                   wxT( "  (condition \"A.Type == 'Via' && A.NetName == '%s'\")\n" )
                                   wxT( "  (constraint zone_connection solid))\n" ),
                                   netName, escaped );
    }

    return rules;
}

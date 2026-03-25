/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_io/pads/pads_sch_binary_reader.h>

#include <io/pads/pads_binary_utils.h>

#include <lib_id.h>
#include <lib_symbol.h>
#include <page_info.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <schematic.h>

#include <base_units.h>
#include <math/util.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <limits>

namespace PADS_SCH_BINARY
{

static constexpr uint8_t  MAGIC0 = 0x00;
static constexpr uint8_t  MAGIC1 = 0xFE;
static constexpr uint16_t VERSION = 0x000D;
static constexpr size_t   DATA_STREAM_OFFSET = 0x250;

// Schematic page coordinates are stored as a biased half-mil u16:
//     design_mil = 2 * u16 - 99072
// 99072 = 0x18300 is a fixed PADS Logic page constant (same in every file).
static constexpr int PAGE_BIAS = 99072;

// The default page extent when the stored sheet-size token is absent.  PADS
// stores the sheet size symbolically as WDITBSIZE<X>; B = 17000 x 11000 mil.
static constexpr int DEFAULT_PAGE_WIDTH_MIL = 17000;
static constexpr int DEFAULT_PAGE_HEIGHT_MIL = 11000;

// Part-placement record: stride 136, refdes ASCII at +0, with the placement
// fields interleaved one block ahead of the refdes.
static constexpr size_t PART_STRIDE = 136;
static constexpr int    PART_X_OFF = -0x3e;  // u16, relative to refdes
static constexpr int    PART_Y_OFF = -0x3c;  // u16
static constexpr int    PART_ORI_OFF = -0x3a;// byte: 0x00 = 0deg, 0x84 = 90deg
static constexpr uint8_t PART_ORI_90 = 0x84;

// MFC object-class word at +0x28: 00 <class-id> marks a part-instance slot.
static constexpr int                    PART_CLASS_OFF = 0x28;
static constexpr std::array<uint8_t, 5> PART_CLASS_IDS = { 0x02, 0x06, 0x0A, 0x12, 0x1A };

// Connection split-header pool: stride-40 records, marker FD (active) / FC at
// byte +1, vertex count at +2, cumulative next-start index (u32le) at +0x0b.
static constexpr size_t  SPLIT_STRIDE = 40;
static constexpr int     SPLIT_MARKER_OFF = 0x01;
static constexpr int     SPLIT_NVERTS_OFF = 0x02;
static constexpr int     SPLIT_CUMULATIVE_OFF = 0x0b;
static constexpr uint8_t SPLIT_MARKER_FD = 0xFD;
static constexpr uint8_t SPLIT_MARKER_FC = 0xFC;

// Each wire vertex is an 8-byte record: 00 00 00 Xlo Xhi Ylo Yhi T.
static constexpr size_t VERTEX_STRIDE = 8;


static int designMil( uint16_t aRaw )
{
    return 2 * static_cast<int>( aRaw ) - PAGE_BIAS;
}


static uint16_t readU16( const std::vector<uint8_t>& d, size_t o )
{
    return PADS_IO::BINARY_CURSOR( d ).U16At( o );
}


static uint32_t readU32( const std::vector<uint8_t>& d, size_t o )
{
    return PADS_IO::BINARY_CURSOR( d ).U32At( o );
}


bool PADS_SCH_BINARY_READER::IsBinarySch( const std::vector<uint8_t>& aData )
{
    if( aData.size() < DATA_STREAM_OFFSET )
        return false;

    if( aData[0] != MAGIC0 || aData[1] != MAGIC1 )
        return false;

    return readU16( aData, 2 ) == VERSION;
}


bool PADS_SCH_BINARY_READER::ReadFile( const wxString& aFileName, std::vector<uint8_t>& aData )
{
    return PADS_IO::ReadFileToBuffer( aFileName, aData );
}


bool PADS_SCH_BINARY_READER::Parse( const std::vector<uint8_t>& aData )
{
    m_placements.clear();
    m_wireVertices.clear();
    m_wirePolylines.clear();
    m_busPolylines.clear();
    m_texts.clear();
    m_junctions.clear();

    if( !IsBinarySch( aData ) )
        return false;

    decodePlacements( aData );
    decodeWires( aData );
    decodeTexts( aData );
    decodeJunctions( aData );

    return true;
}


// ---------------------------------------------------------------------------
// PAGE SIZE
//
// The sheet size is stored symbolically as WDITBSIZE<X> (X = A..E).  Only the
// B page (17000 x 11000 mil) appears in the corpus; map the token to its
// physical extent, used as a structural in-page bound for vertices.
// ---------------------------------------------------------------------------
static void pageExtent( const std::vector<uint8_t>& d, int& aWidth, int& aHeight )
{
    aWidth = DEFAULT_PAGE_WIDTH_MIL;
    aHeight = DEFAULT_PAGE_HEIGHT_MIL;

    static const uint8_t token[] = { 'W', 'D', 'I', 'T', 'B', 'S', 'I', 'Z', 'E' };

    if( d.size() < sizeof( token ) + 1 )
        return;

    for( size_t i = 0; i + sizeof( token ) < d.size(); ++i )
    {
        if( std::equal( std::begin( token ), std::end( token ), &d[i] ) )
        {
            uint8_t size = d[i + sizeof( token )];

            switch( size )
            {
            case 'A':
                aWidth = 11000;
                aHeight = 8500;
                break;
            case 'B':
                aWidth = 17000;
                aHeight = 11000;
                break;
            case 'C':
                aWidth = 22000;
                aHeight = 17000;
                break;
            case 'D':
                aWidth = 34000;
                aHeight = 22000;
                break;
            case 'E':
                aWidth = 44000;
                aHeight = 34000;
                break;
            default: break;
            }

            return;
        }
    }
}


// ---------------------------------------------------------------------------
// PART PLACEMENT
//
// Part-instance records are fixed stride-136 slots grouped as one run per
// non-empty sheet, in sheet order.  Each slot carries a NUL-padded refdes
// buffer, the MFC class tag at +0x28, and the serialized text-style trailer
// immediately before the refdes.  X/Y/orientation decode from the
// refdes-relative placement fields.
// ---------------------------------------------------------------------------
static bool refdesOk( const std::vector<uint8_t>& d, size_t o )
{
    if( o + 40 > d.size() )
        return false;

    // First char is a letter; subsequent chars alnum / _ / - / . / ; then NUL;
    // then zeros through byte +39.
    uint8_t c0 = d[o];

    if( !std::isalpha( c0 ) )
        return false;

    size_t nul = 0;

    for( size_t i = o; i < o + 40; ++i )
    {
        uint8_t c = d[i];

        if( c == 0 )
        {
            nul = i;
            break;
        }

        if( !( std::isalnum( c ) || c == '_' || c == '-' || c == '.' || c == '/' ) )
            return false;
    }

    if( nul == 0 )
        return false;

    for( size_t i = nul; i < o + 40; ++i )
    {
        if( d[i] != 0 )
            return false;
    }

    return true;
}


static bool classTagOk( const std::vector<uint8_t>& d, size_t o )
{
    if( o + PART_CLASS_OFF + 2 > d.size() )
        return false;

    if( d[o + PART_CLASS_OFF] != 0x00 )
        return false;

    uint8_t id = d[o + PART_CLASS_OFF + 1];

    return std::find( PART_CLASS_IDS.begin(), PART_CLASS_IDS.end(), id ) != PART_CLASS_IDS.end();
}


static bool partTrailerOk( const std::vector<uint8_t>& d, size_t o )
{
    if( o < 14 )
        return false;

    const uint8_t* t = &d[o - 14];

    // Four little-endian text-style height words (high byte 0, low byte a known
    // style id), four 0x0a linewidth bytes, a 00 status byte, a small status.
    if( t[1] || t[3] || t[5] || t[7] )
        return false;

    for( int i : { 0, 2, 4, 6 } )
    {
        if( t[i] != 0x61 && t[i] != 0x64 && t[i] != 0x6F )
            return false;
    }

    if( t[8] != 0x0A || t[9] != 0x0A || t[10] != 0x0A || t[11] != 0x0A )
        return false;

    return t[12] == 0x00 && ( t[13] == 0x00 || t[13] == 0x01 || t[13] == 0x03 );
}


static bool isPartSlot( const std::vector<uint8_t>& d, size_t o )
{
    if( o < DATA_STREAM_OFFSET )
        return false;

    if( static_cast<int>( o ) + PART_X_OFF < 0 )
        return false;

    return refdesOk( d, o ) && classTagOk( d, o ) && partTrailerOk( d, o );
}


void PADS_SCH_BINARY_READER::decodePlacements( const std::vector<uint8_t>& d )
{
    size_t n = d.size();

    for( size_t i = DATA_STREAM_OFFSET; i + PART_STRIDE < n; ++i )
    {
        if( !isPartSlot( d, i ) )
            continue;

        // A run starts where the previous stride-136 slot is not a part slot.
        if( i >= DATA_STREAM_OFFSET + PART_STRIDE && isPartSlot( d, i - PART_STRIDE ) )
            continue;

        size_t p = i;

        while( p + PART_STRIDE < n && isPartSlot( d, p ) )
        {
            PLACEMENT pl;
            size_t    z = p;

            while( z < p + 40 && d[z] != 0 )
                ++z;

            pl.reference.assign( reinterpret_cast<const char*>( &d[p] ), z - p );
            pl.x_mils = designMil( readU16( d, p + PART_X_OFF ) );
            pl.y_mils = designMil( readU16( d, p + PART_Y_OFF ) );
            pl.rotation = ( d[p + PART_ORI_OFF] == PART_ORI_90 ) ? 90 : 0;
            m_placements.push_back( std::move( pl ) );

            p += PART_STRIDE;
        }

        i = p - 1;  // continue past the run
    }
}


// ---------------------------------------------------------------------------
// WIRES
//
// Wire vertices live in a flat pool of 8-byte records:
//     00 00 00  Xlo Xhi  Ylo Yhi  T      (X,Y = design = 2*u16 - 99072)
// Each per-sheet pool is preceded by a stride-40 split-header run whose
// cumulative-index chain tiles the pool exactly into connection polylines; the
// explicit gap slices between cumulative jumps are bus geometry.  Runs appear
// in sheet order, the first framing the active sheet.
// ---------------------------------------------------------------------------
static bool isVertexRecord( const std::vector<uint8_t>& d, size_t o, int aPageWidth, int aPageHeight )
{
    if( o + VERTEX_STRIDE > d.size() )
        return false;

    // Slot grammar: first three bytes zero; trailer 0x00, with 0xff allowed for
    // the terminal slot abutting the trailing label pool.
    if( d[o] != 0 || d[o + 1] != 0 || d[o + 2] != 0 )
        return false;

    if( d[o + 7] != 0x00 && d[o + 7] != 0xFF )
        return false;

    int x = designMil( readU16( d, o + 3 ) );
    int y = designMil( readU16( d, o + 5 ) );

    if( x % 50 != 0 || y % 50 != 0 )
        return false;

    return x >= 0 && x <= aPageWidth && y >= 0 && y <= aPageHeight;
}


// One split-header run framing a wire vertex pool.  The cumulative-index chain
// gives the exact connection slice boundaries; the trailing gap before each
// cumulative jump is bus geometry.
struct SPLIT_RUN
{
    size_t              vertexOffset = 0;
    std::vector<int>    nverts;     ///< Connection vertex counts, in order.
    std::vector<size_t> cumulative; ///< Cumulative next-start per full header.
    size_t              rawVertexCount = 0;
};


static bool parseSplitRun( const std::vector<uint8_t>& d, size_t aOffset, uint8_t aMarker, int aPageWidth,
                           int aPageHeight, SPLIT_RUN& aRun )
{
    size_t pos = aOffset;
    size_t previousStart = 0;
    size_t maxVertices = d.size() / VERTEX_STRIDE;

    std::vector<int>    nverts;
    std::vector<size_t> cumulative;

    while( pos + SPLIT_STRIDE + VERTEX_STRIDE <= d.size() && d[pos + SPLIT_MARKER_OFF] == aMarker )
    {
        int    nv = d[pos + SPLIT_NVERTS_OFF];
        size_t cum = readU32( d, pos + SPLIT_CUMULATIVE_OFF );

        if( nv == 0 )
            break;

        if( cum < previousStart + static_cast<size_t>( nv ) )
            break;

        if( cum > maxVertices )
            break;

        nverts.push_back( nv );
        cumulative.push_back( cum );
        previousStart = cum;
        pos += SPLIT_STRIDE;
    }

    if( nverts.empty() )
        return false;

    if( pos + VERTEX_STRIDE > d.size() || d[pos + SPLIT_MARKER_OFF] != aMarker )
        return false;

    int terminalNverts = d[pos + SPLIT_NVERTS_OFF];

    if( terminalNverts == 0 )
        return false;

    size_t vertexOffset = pos + VERTEX_STRIDE;
    size_t rawVertexCount = previousStart + static_cast<size_t>( terminalNverts );

    if( rawVertexCount == 0 || vertexOffset + rawVertexCount * VERTEX_STRIDE > d.size() )
        return false;

    for( size_t idx = 0; idx < rawVertexCount; ++idx )
    {
        if( !isVertexRecord( d, vertexOffset + idx * VERTEX_STRIDE, aPageWidth, aPageHeight ) )
            return false;
    }

    nverts.push_back( terminalNverts );

    aRun.vertexOffset = vertexOffset;
    aRun.nverts = std::move( nverts );
    aRun.cumulative = std::move( cumulative );
    aRun.rawVertexCount = rawVertexCount;

    return true;
}


void PADS_SCH_BINARY_READER::decodeWires( const std::vector<uint8_t>& d )
{
    int pageWidth = 0;
    int pageHeight = 0;
    pageExtent( d, pageWidth, pageHeight );

    if( d.size() < SPLIT_STRIDE + VERTEX_STRIDE )
        return;

    size_t i = DATA_STREAM_OFFSET;

    while( i + SPLIT_STRIDE + VERTEX_STRIDE <= d.size() )
    {
        uint8_t marker = d[i + SPLIT_MARKER_OFF];

        if( marker != SPLIT_MARKER_FD && marker != SPLIT_MARKER_FC )
        {
            ++i;
            continue;
        }

        SPLIT_RUN run;

        if( !parseSplitRun( d, i, marker, pageWidth, pageHeight, run ) )
        {
            ++i;
            continue;
        }

        std::vector<WIRE_VERTEX> vertices;
        vertices.reserve( run.rawVertexCount );

        for( size_t idx = 0; idx < run.rawVertexCount; ++idx )
        {
            size_t      o = run.vertexOffset + idx * VERTEX_STRIDE;
            WIRE_VERTEX v;
            v.x_mils = designMil( readU16( d, o + 3 ) );
            v.y_mils = designMil( readU16( d, o + 5 ) );
            vertices.push_back( v );
            m_wireVertices.push_back( v );
        }

        // Tile the pool with the cumulative chain.  Each full header emits one
        // connection slice of nverts; the gap up to the next cumulative start
        // is a bus polyline.  The terminal header emits the final connection.
        size_t prev = 0;

        for( size_t k = 0; k < run.cumulative.size(); ++k )
        {
            size_t connEnd = prev + static_cast<size_t>( run.nverts[k] );

            if( connEnd > vertices.size() )
                break;

            m_wirePolylines.emplace_back( vertices.begin() + prev, vertices.begin() + connEnd );

            // The explicit slice between this connection's end and the next
            // cumulative start is a bus polyline.
            size_t gapEnd = run.cumulative[k];

            if( gapEnd > connEnd && gapEnd <= vertices.size() )
                m_busPolylines.emplace_back( vertices.begin() + connEnd, vertices.begin() + gapEnd );

            prev = run.cumulative[k];
        }

        size_t terminalNverts = static_cast<size_t>( run.nverts.back() );

        if( prev + terminalNverts <= vertices.size() )
        {
            m_wirePolylines.emplace_back( vertices.begin() + prev, vertices.begin() + prev + terminalNverts );
        }

        // Resume past this run's vertex pool; runs do not overlap.
        i = run.vertexOffset + run.rawVertexCount * VERTEX_STRIDE;
    }
}


// ---------------------------------------------------------------------------
// FREE TEXT
//
// Free-text items are fixed 32-byte records anchored on the duplicated counter
// at +12 == +14, carrying position, orientation, justification, height and
// linewidth inline plus the string length at +8 and a string-pool cursor at
// +28.  The string CONTENT lives in a shared NUL-delimited pool interleaved with
// component-attribute strings, so it is recovered by an ordered length-matched
// walk anchored on the pool whose matched offsets best track the +28 cursor.
// ---------------------------------------------------------------------------
static constexpr size_t TEXT_STRIDE = 32;
static constexpr int    TEXT_REF_OFF = 28;  // u32 string-pool cursor


struct TEXT_RECORD
{
    int    x_mils = 0;
    int    y_mils = 0;
    int    orientation_deg = 0;
    int    justification = 0;
    int    height_mils = 0;
    int    linewidth_mils = 0;
    int    strlen = 0;
    uint32_t ref = 0;
};


static bool isTextRecord( const std::vector<uint8_t>& d, size_t o, TEXT_RECORD& aRec )
{
    if( o + TEXT_STRIDE > d.size() )
        return false;

    uint16_t rx = readU16( d, o + 0 );
    uint16_t ry = readU16( d, o + 2 );
    uint16_t ori = readU16( d, o + 4 );
    uint16_t just = readU16( d, o + 6 );
    uint16_t slen = readU16( d, o + 8 );
    uint16_t height = readU16( d, o + 10 );
    uint16_t c0 = readU16( d, o + 12 );
    uint16_t c1 = readU16( d, o + 14 );
    uint16_t lw = readU16( d, o + 18 );

    if( c0 != c1 || c0 == 0 || c0 > 4000 )
        return false;

    if( ori % 10 || ori > 3600 )
        return false;

    if( slen < 2 || slen > 400 )
        return false;

    if( height < 40 || height > 400 )
        return false;

    if( lw != 5 && lw != 10 && lw != 15 && lw != 20 && lw != 25 && lw != 30 )
        return false;

    if( just > 15 )
        return false;

    aRec.x_mils = designMil( rx );
    aRec.y_mils = designMil( ry );
    aRec.orientation_deg = ori / 10;
    aRec.justification = just;
    aRec.height_mils = height;
    aRec.linewidth_mils = lw;
    aRec.strlen = slen - 1;
    aRec.ref = readU32( d, o + TEXT_REF_OFF );

    return true;
}


// One NUL-delimited printable string in the shared pool.
struct POOL_STRING
{
    size_t      offset = 0;
    std::string text;
};


static std::vector<POOL_STRING> collectPoolStrings( const std::vector<uint8_t>& d )
{
    std::vector<POOL_STRING> pool;
    size_t                   n = d.size();
    size_t                   o = DATA_STREAM_OFFSET;

    while( o < n )
    {
        size_t e = o;

        while( e < n && d[e] != 0 )
            ++e;

        size_t len = e - o;

        if( len >= 1 && len <= 400 )
        {
            bool printable = true;

            for( size_t i = o; i < e; ++i )
            {
                if( d[i] < 0x20 || d[i] >= 0x7f )
                {
                    printable = false;
                    break;
                }
            }

            if( printable )
                pool.push_back( { o, std::string( reinterpret_cast<const char*>( &d[o] ), len ) } );
        }

        o = ( e > o ) ? e + 1 : o + 1;
    }

    return pool;
}


// Walk the pool from @p aStart, taking the next string of each record's length in
// order.  Returns false if any record cannot be matched or offsets are not
// strictly increasing.  Fills @p aStrings and @p aOffsets on success.
static bool lengthMatchedWalk( const std::vector<POOL_STRING>& aPool, size_t aStart,
                               const std::vector<TEXT_RECORD>& aRecords,
                               std::vector<std::string>& aStrings, std::vector<size_t>& aOffsets )
{
    aStrings.clear();
    aOffsets.clear();
    size_t idx = aStart;

    for( const TEXT_RECORD& rec : aRecords )
    {
        while( idx < aPool.size() && static_cast<int>( aPool[idx].text.size() ) != rec.strlen )
            ++idx;

        if( idx >= aPool.size() )
            return false;

        if( !aOffsets.empty() && aPool[idx].offset <= aOffsets.back() )
            return false;

        aStrings.push_back( aPool[idx].text );
        aOffsets.push_back( aPool[idx].offset );
        ++idx;
    }

    return true;
}


void PADS_SCH_BINARY_READER::decodeTexts( const std::vector<uint8_t>& d )
{
    std::vector<TEXT_RECORD> records;

    for( size_t i = DATA_STREAM_OFFSET; i + TEXT_STRIDE <= d.size(); ++i )
    {
        TEXT_RECORD rec;

        if( isTextRecord( d, i, rec ) )
            records.push_back( rec );
    }

    if( records.empty() )
        return;

    // Recover string content: anchor the pool start on the candidate whose
    // length-matched walk fully covers the records and whose matched offsets best
    // track the per-record +28 cursor, then take each record's string in order.
    std::vector<POOL_STRING> pool = collectPoolStrings( d );

    std::vector<std::string> bestStrings;
    double                   bestResidual = std::numeric_limits<double>::max();

    for( size_t si = 0; si < pool.size(); ++si )
    {
        if( static_cast<int>( pool[si].text.size() ) != records[0].strlen )
            continue;

        std::vector<std::string> strings;
        std::vector<size_t>      offsets;

        if( !lengthMatchedWalk( pool, si, records, strings, offsets ) )
            continue;

        long long base0 = static_cast<long long>( offsets[0] ) - records[0].ref;
        double    residual = 0.0;

        for( size_t k = 0; k < offsets.size(); ++k )
        {
            long long predicted = base0 + records[k].ref;
            residual += static_cast<double>( std::llabs( static_cast<long long>( offsets[k] ) - predicted ) );
        }

        residual /= static_cast<double>( offsets.size() );

        if( residual < bestResidual )
        {
            bestResidual = residual;
            bestStrings = std::move( strings );
        }
    }

    for( size_t k = 0; k < records.size(); ++k )
    {
        TEXT_ITEM item;
        item.x_mils = records[k].x_mils;
        item.y_mils = records[k].y_mils;
        item.orientation_deg = records[k].orientation_deg;
        item.justification = records[k].justification;
        item.height_mils = records[k].height_mils;
        item.linewidth_mils = records[k].linewidth_mils;

        if( k < bestStrings.size() )
            item.text = bestStrings[k];

        m_texts.push_back( std::move( item ) );
    }
}


// ---------------------------------------------------------------------------
// JUNCTIONS (PADS tie-dots)
//
// Tie-dots are fixed 12-byte records: X (u16) and Y (u16) in the page-biased
// half-mil encoding, a net-index word at +4, and a constant 0xfc marker at +6
// with a zero tail at +7..+11.  PADS stores one contiguous run per sheet; the
// runs appear in sheet order (the first frames the active sheet), so every run
// is a tie-dot array and all are collected.
// ---------------------------------------------------------------------------
static constexpr size_t   JUNCTION_STRIDE = 12;
static constexpr int      JUNCTION_MARKER_OFF = 6;
static constexpr uint8_t  JUNCTION_MARKER = 0xFC;


static bool isJunctionRecord( const std::vector<uint8_t>& d, size_t o, int aPageWidth, int aPageHeight )
{
    if( o + JUNCTION_STRIDE > d.size() )
        return false;

    if( d[o + JUNCTION_MARKER_OFF] != JUNCTION_MARKER || d[o + 7] != 0 )
        return false;

    if( d[o + 8] != 0 || d[o + 9] != 0 || d[o + 10] != 0 || d[o + 11] != 0 )
        return false;

    int x = designMil( readU16( d, o ) );
    int y = designMil( readU16( d, o + 2 ) );

    if( x % 50 != 0 || y % 50 != 0 )
        return false;

    return x >= 0 && x <= aPageWidth && y >= 0 && y <= aPageHeight;
}


void PADS_SCH_BINARY_READER::decodeJunctions( const std::vector<uint8_t>& d )
{
    int pageWidth = 0;
    int pageHeight = 0;
    pageExtent( d, pageWidth, pageHeight );

    size_t i = DATA_STREAM_OFFSET;

    while( i + JUNCTION_STRIDE <= d.size() )
    {
        if( !isJunctionRecord( d, i, pageWidth, pageHeight ) )
        {
            ++i;
            continue;
        }

        // A run must hold at least two tie-dots to distinguish it from a stray
        // 0xfc-marked record; collect every record of the run.
        size_t j = i;
        std::vector<JUNCTION> run;

        while( j + JUNCTION_STRIDE <= d.size() && isJunctionRecord( d, j, pageWidth, pageHeight ) )
        {
            JUNCTION jct;
            jct.x_mils = designMil( readU16( d, j ) );
            jct.y_mils = designMil( readU16( d, j + 2 ) );
            run.push_back( jct );
            j += JUNCTION_STRIDE;
        }

        if( run.size() >= 2 )
            m_junctions.insert( m_junctions.end(), run.begin(), run.end() );

        i = j;
    }
}


// ---------------------------------------------------------------------------
// SCHEMATIC BUILD
// ---------------------------------------------------------------------------
int PADS_SCH_BINARY_READER::BuildSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet ) const
{
    if( !aSchematic || !aRootSheet || !aRootSheet->GetScreen() )
        return 0;

    SCH_SCREEN* screen = aRootSheet->GetScreen();

    SCH_SHEET_PATH path;
    path.push_back( aRootSheet );

    PAGE_INFO   pageInfo = screen->GetPageSettings();
    const int   pageHeightIU = pageInfo.GetHeightIU( schIUScale.IU_PER_MILS );

    auto milToY = [&]( int aMil ) -> int
    {
        return pageHeightIU - schIUScale.MilsToIU( aMil );
    };

    int appended = 0;

    // --- SYMBOLS (generic placeholder at recovered position + orientation) ---
    for( const PLACEMENT& pl : m_placements )
    {
        wxString name = wxString::Format( wxT( "PADS_%s" ), wxString::FromUTF8( pl.reference ) );

        // A minimal generic library symbol: the placement->graphic link is
        // heap-walled, so we emit an empty body carrying the recovered
        // reference and position.  The user can re-link to a real symbol.
        std::unique_ptr<LIB_SYMBOL> libSym = std::make_unique<LIB_SYMBOL>( name );

        std::unique_ptr<SCH_SYMBOL> symbol = std::make_unique<SCH_SYMBOL>();

        LIB_ID libId;
        libId.SetLibNickname( wxT( "pads_import" ) );
        libId.SetLibItemName( name );
        symbol->SetLibId( libId );
        symbol->SetLibSymbol( new LIB_SYMBOL( *libSym ) );

        symbol->SetPosition( VECTOR2I( schIUScale.MilsToIU( pl.x_mils ), milToY( pl.y_mils ) ) );

        if( pl.rotation == 90 )
            symbol->SetOrientation( SYMBOL_ORIENTATION_T::SYM_ORIENT_90 );
        else
            symbol->SetOrientation( SYMBOL_ORIENTATION_T::SYM_ORIENT_0 );

        symbol->SetUnit( 1 );
        symbol->SetRef( &path, wxString::FromUTF8( pl.reference ) );
        symbol->AddHierarchicalReference( path.Path(), wxString::FromUTF8( pl.reference ), 1 );

        screen->Append( symbol.release() );
        ++appended;
    }

    // --- WIRES (one polyline per recovered connection) ---
    for( const std::vector<WIRE_VERTEX>& poly : m_wirePolylines )
    {
        for( size_t k = 0; k + 1 < poly.size(); ++k )
        {
            VECTOR2I start( schIUScale.MilsToIU( poly[k].x_mils ), milToY( poly[k].y_mils ) );
            VECTOR2I end( schIUScale.MilsToIU( poly[k + 1].x_mils ), milToY( poly[k + 1].y_mils ) );

            if( start == end )
                continue;

            SCH_LINE* line = new SCH_LINE( start, SCH_LAYER_ID::LAYER_WIRE );
            line->SetEndPoint( end );
            screen->Append( line );
            ++appended;
        }
    }

    // --- BUSES (the split-run gap polylines) ---
    for( const std::vector<WIRE_VERTEX>& poly : m_busPolylines )
    {
        for( size_t k = 0; k + 1 < poly.size(); ++k )
        {
            VECTOR2I start( schIUScale.MilsToIU( poly[k].x_mils ), milToY( poly[k].y_mils ) );
            VECTOR2I end( schIUScale.MilsToIU( poly[k + 1].x_mils ), milToY( poly[k + 1].y_mils ) );

            if( start == end )
                continue;

            SCH_LINE* line = new SCH_LINE( start, SCH_LAYER_ID::LAYER_BUS );
            line->SetEndPoint( end );
            screen->Append( line );
            ++appended;
        }
    }

    // --- FREE TEXT (recovered geometry, style and string content) ---
    for( const TEXT_ITEM& txt : m_texts )
    {
        if( txt.text.empty() )
            continue;

        SCH_TEXT* text = new SCH_TEXT( VECTOR2I( schIUScale.MilsToIU( txt.x_mils ),
                                                 milToY( txt.y_mils ) ),
                                       wxString::FromUTF8( txt.text ) );

        text->SetTextHeight( schIUScale.MilsToIU( txt.height_mils ) );
        text->SetTextWidth( schIUScale.MilsToIU( txt.height_mils ) );
        text->SetTextThickness( schIUScale.MilsToIU( txt.linewidth_mils ) );

        if( txt.orientation_deg == 90 )
            text->SetTextAngle( ANGLE_VERTICAL );
        else
            text->SetTextAngle( ANGLE_HORIZONTAL );

        screen->Append( text );
        ++appended;
    }

    // --- JUNCTIONS (PADS tie-dots) ---
    for( const JUNCTION& jct : m_junctions )
    {
        VECTOR2I pos( schIUScale.MilsToIU( jct.x_mils ), milToY( jct.y_mils ) );
        screen->Append( new SCH_JUNCTION( pos ) );
        ++appended;
    }

    return appended;
}

} // namespace PADS_SCH_BINARY

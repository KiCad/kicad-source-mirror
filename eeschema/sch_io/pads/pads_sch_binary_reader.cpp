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
#include <sch_field.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <stroke_params.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <schematic.h>

#include <base_units.h>
#include <math/util.h>
#include <wildcards_and_files_ext.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <set>

namespace PADS_SCH_BINARY
{

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
static constexpr int    PART_ORI_OFF = -0x3a;// u16 angle in tenths of a degree (900 = 90deg)

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


static void pageExtent( const std::vector<uint8_t>& d, int& aWidth, int& aHeight );


void POOL_DIRECTORY::Parse( const std::vector<uint8_t>& aData )
{
    valid = false;
    usedCount.fill( 0 );
    usedBytes.fill( 0 );

    if( aData.size() < TABLE_OFFSET + POOL_COUNT * DESCRIPTOR_SIZE )
        return;

    PADS_IO::BINARY_CURSOR cursor( aData );

    for( size_t i = 0; i < POOL_COUNT; ++i )
    {
        PADS_IO::SDB_RECORD rec( cursor, TABLE_OFFSET + i * DESCRIPTOR_SIZE );
        usedCount[i] = rec.U32( USED_COUNT_OFFSET );
        usedBytes[i] = rec.U32( USED_BYTES_OFFSET );
    }

    valid = true;
}


uint32_t POOL_DIRECTORY::Count( int aPool ) const
{
    if( aPool < 0 || aPool >= static_cast<int>( POOL_COUNT ) )
        return 0;

    return usedCount[aPool];
}


bool PADS_SCH_BINARY_READER::IsBinarySch( const std::vector<uint8_t>& aData )
{
    if( aData.size() < DATA_STREAM_OFFSET )
        return false;

    // Shared container magic with the `.pcb` reader; only the second magic byte and
    // the supported version differ between the two PADS SDB formats.
    if( !PADS_IO::HasSdbMagic( aData, MAGIC1 ) )
        return false;

    return readU16( aData, 2 ) == VERSION;
}


bool PADS_SCH_BINARY_READER::ReadFile( const wxString& aFileName, std::vector<uint8_t>& aData )
{
    return PADS_IO::ReadFileToBuffer( aFileName, aData );
}


bool PADS_SCH_BINARY_READER::Parse( const std::vector<uint8_t>& aData )
{
    m_sheetOffsets.clear();
    m_streamEnd = 0;
    m_sheetNames.clear();
    m_decals.clear();
    m_decalIndex.clear();
    m_usedDecalTables.clear();
    m_decalBuiltinCount = 0;
    m_partTypeNames.clear();
    m_partTypePools.clear();
    m_partTypePins.clear();
    m_partTypeGatePins.clear();
    m_partTypeFields.clear();
    m_placements.clear();
    m_wireVertices.clear();
    m_wirePolylines.clear();
    m_busPolylines.clear();
    m_wirePolylineSheets.clear();
    m_busPolylineSheets.clear();
    m_texts.clear();
    m_junctions.clear();
    m_netLabels.clear();
    m_netTableScanCount = 0;

    if( !IsBinarySch( aData ) )
        return false;

    // Read the SDB pool directory once; the decoders take their authoritative
    // object counts from it instead of rediscovering them from payload scans.
    m_pools.Parse( aData );

    pageExtent( aData, m_pageWidthMils, m_pageHeightMils );

    decodeSheets( aData );
    decodeDecals( aData );
    decodeFields( aData );
    decodePinNames( aData );
    decodePlacements( aData );
    decodeWires( aData );
    decodeTexts( aData );
    decodeJunctions( aData );
    decodeNetLabels( aData );

    return true;
}


// ---------------------------------------------------------------------------
// SHEETS
//
// The authoritative per-sheet partition is the pool3 sheet table: a contiguous
// stride-48 array (count == pool3.used_count) whose records carry the absolute
// start offset of each sheet's object block (+0x00), its byte length (+0x04), a
// 0xFFFF marker (+0x0a) and the "[N]NAME" sheet name (+0x0c).  The ranges
// [A_k, A_{k+1}) tile the object stream with zero gaps.  The per-sheet CAE view
// record (signature 80 00 00 00 30 00 00 00) sits exactly 664 bytes into each
// block (A_k + 664), so it is a worse boundary -- it leaks each block's leading
// text/wires into the previous sheet.  We anchor the table on the first validated
// signature minus 664.  Sheets ARE named, and file order is NOT active-first.
// ---------------------------------------------------------------------------
static const std::array<uint8_t, 8> SHEET_SIGNATURE = { 0x80, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00 };
static constexpr uint32_t SHEET_SIG_TO_BLOCK = 664;  // block_off_A = signature - 664


void PADS_SCH_BINARY_READER::decodeSheets( const std::vector<uint8_t>& d )
{
    // The sheet count is the sheet controller's authoritative object count.
    size_t sheetCount = m_pools.Count( POOL_DIRECTORY::SHEETS );

    if( sheetCount == 0 || sheetCount > 4096 )
        return;

    // The first VALID per-sheet CAE signature: SCALE f32 in [1,60] at +8, and a
    // heap pointer (high byte >= 0xC0) at +12, rejecting stray byte coincidences.
    size_t firstSig = 0;
    bool   haveSig = false;

    for( size_t i = DATA_STREAM_OFFSET; i + SHEET_SIGNATURE.size() + 8 <= d.size(); ++i )
    {
        if( !std::equal( SHEET_SIGNATURE.begin(), SHEET_SIGNATURE.end(), &d[i] ) )
            continue;

        uint32_t scaleBits = readU32( d, i + 8 );
        float    scale = 0.0f;
        std::memcpy( &scale, &scaleBits, sizeof( scale ) );
        uint32_t heapPtr = readU32( d, i + 12 );

        if( scale >= 1.0f && scale <= 60.0f && ( heapPtr >> 24 ) >= 0xC0 )
        {
            firstSig = i;
            haveSig = true;
            break;
        }
    }

    if( !haveSig || firstSig < SHEET_SIG_TO_BLOCK )
        return;

    uint32_t targetA0 = static_cast<uint32_t>( firstSig - SHEET_SIG_TO_BLOCK );

    // Locate the stride-48 sheet table: a record whose block offset equals targetA0
    // and whose 0xFFFF marker is present, followed by sheetCount strictly-increasing
    // in-bounds block offsets.
    for( size_t o = DATA_STREAM_OFFSET; o + sheetCount * 48 <= d.size(); ++o )
    {
        if( readU32( d, o ) != targetA0 || readU16( d, o + 0x0a ) != 0xFFFF )
            continue;

        std::vector<uint32_t> offs;
        bool                  ok = true;

        for( size_t k = 0; k < sheetCount; ++k )
        {
            uint32_t a = readU32( d, o + k * 48 );

            if( a < DATA_STREAM_OFFSET || a >= d.size() || ( k > 0 && a <= offs.back() ) )
            {
                ok = false;
                break;
            }

            offs.push_back( a );
        }

        if( !ok )
            continue;

        for( size_t k = 0; k < sheetCount; ++k )
        {
            m_sheetOffsets.push_back( offs[k] );

            // Each sheet record carries its block byte-length at +4; the blocks tile
            // [start, start+length) contiguously, so the highest end is the end of the
            // schematic SDB payload (the embedded OLE preview region follows it).
            uint32_t blockLen = readU32( d, o + k * 48 + 4 );
            size_t   blockEnd = static_cast<size_t>( offs[k] ) + blockLen;

            if( blockEnd <= d.size() && blockEnd > m_streamEnd )
                m_streamEnd = blockEnd;

            std::string name;

            for( size_t j = o + k * 48 + 0x0c; j < o + k * 48 + 48; ++j )
            {
                uint8_t c = d[j];

                if( c == 0x00 )
                    break;

                if( c >= 0x20 && c < 0x7f )
                    name.push_back( static_cast<char>( c ) );
            }

            m_sheetNames.push_back( name );
        }

        return;
    }
}


size_t PADS_SCH_BINARY_READER::streamLimit( const std::vector<uint8_t>& aData ) const
{
    return ( m_streamEnd > 0 && m_streamEnd <= aData.size() ) ? m_streamEnd : aData.size();
}


int PADS_SCH_BINARY_READER::sheetIndexForOffset( size_t aOffset ) const
{
    if( m_sheetOffsets.empty() )
        return 0;

    // The block for sheet k spans [m_sheetOffsets[k], m_sheetOffsets[k+1]); records
    // before the first signature belong to the first sheet.
    auto it = std::upper_bound( m_sheetOffsets.begin(), m_sheetOffsets.end(), aOffset );
    int  idx = static_cast<int>( it - m_sheetOffsets.begin() ) - 1;

    return idx < 0 ? 0 : idx;
}


// ---------------------------------------------------------------------------
// CAE DECALS (gate-symbol geometry library) + placement->decal binding
//
// Geometry library: a stride-0x50 record table (name@+0, class 0x06@+0x29,
// cumulative-VERTEX u32@+0x34), followed by a stride-6 piece pool (marker@+1 =
// 0xff open / 0x00 closed, nverts@+2, linewidth@+4) and a stride-6 vertex pool
// (x=2*i16, y=2*i16, decal-relative).  A decal's vertices are the pool slice
// [cumVtx[i], cumVtx[i+1]); its pieces are those whose vertices fall in that slice.
//
// Binding: each placement carries a u16 decal handle at refdes-0x1a; the decal
// name is used_decal_table[handle - BUILTIN], where the used-decal table is a
// stride-0x6c name run (per sheet) and BUILTIN = pool5.used_count.
// ---------------------------------------------------------------------------
static constexpr size_t DECAL_STRIDE = 0x50;
static constexpr size_t USED_DECAL_STRIDE = 0x6c;


static int decalMil( const std::vector<uint8_t>& d, size_t o )
{
    return 2 * static_cast<int>( static_cast<int16_t>( readU16( d, o ) ) );
}


static std::string nameAt( const std::vector<uint8_t>& d, size_t o, size_t maxlen )
{
    if( o + 1 > d.size() )
        return std::string();

    size_t end = o;

    while( end < d.size() && end < o + maxlen && d[end] != 0 )
        ++end;

    if( end == o )
        return std::string();

    for( size_t i = o; i < end; ++i )
    {
        if( d[i] < 0x20 || d[i] >= 0x7f )
            return std::string();
    }

    return std::string( reinterpret_cast<const char*>( &d[o] ), end - o );
}


void PADS_SCH_BINARY_READER::decodeDecals( const std::vector<uint8_t>& d )
{
    // BUILTIN handle base = the decal controller's authoritative object count.
    m_decalBuiltinCount = m_pools.Count( POOL_DIRECTORY::DECAL_HANDLE_BASE );

    size_t n = streamLimit( d );

    // --- Geometry library: find the decal-record table base (a run of >= 8 records
    // with 0x06@+0x29 and cumVertex@+0x34 monotone non-decreasing from 0). ---
    size_t base = 0;
    bool   found = false;

    for( size_t i = DATA_STREAM_OFFSET; i + DECAL_STRIDE * 8 < n; ++i )
    {
        if( d[i + 0x29] != 0x06 || readU32( d, i + 0x34 ) != 0 )
            continue;

        uint32_t prev = 0;
        int      good = 0;

        for( int k = 0; k < 8; ++k )
        {
            size_t   rec = i + DECAL_STRIDE * k;
            uint32_t cv = readU32( d, rec + 0x34 );

            if( d[rec + 0x29] == 0x06 && cv >= prev && cv < 100000 )
            {
                ++good;
                prev = cv;
            }
            else
            {
                break;
            }
        }

        if( good >= 8 )
        {
            base = i;
            found = true;
            break;
        }
    }

    if( found )
    {
        std::vector<std::pair<std::string, uint32_t>> recs;
        uint32_t                                      prev = 0;

        for( size_t k = 0; ; ++k )
        {
            size_t rec = base + DECAL_STRIDE * k;

            if( rec + DECAL_STRIDE > n )
                break;

            uint32_t    cv = readU32( d, rec + 0x34 );
            std::string nm = nameAt( d, rec, 0x26 );

            if( cv < prev || cv > 100000 || nm.empty() )
                break;

            recs.emplace_back( nm, cv );
            prev = cv;
        }

        // Piece pool follows the record table.
        size_t pieceOff = base + DECAL_STRIDE * recs.size();
        struct PIECE_REC { bool closed; int nverts; int width; size_t vstart; };
        std::vector<PIECE_REC> pieces;
        size_t                 vcursor = 0;
        size_t                 j = pieceOff;

        while( j + 6 <= n )
        {
            uint8_t marker = d[j + 1];
            int     nv = d[j + 2];

            if( ( marker != 0x00 && marker != 0xFF ) || nv == 0 || nv > 80 )
                break;

            pieces.push_back( { marker == 0x00, nv, d[j + 4], vcursor } );
            vcursor += static_cast<size_t>( nv );
            j += 6;
        }

        // Vertex pool follows the piece pool.
        size_t              vertOff = j;
        std::vector<std::pair<int, int>> verts;

        for( size_t q = 0; q < vcursor; ++q )
        {
            size_t o = vertOff + 6 * q;

            if( o + 6 > n )
                break;

            verts.emplace_back( decalMil( d, o ), decalMil( d, o + 2 ) );
        }

        // Split pieces into decals by the cumulative-vertex ranges.
        for( size_t di = 0; di < recs.size(); ++di )
        {
            size_t lo = recs[di].second;
            size_t hi = ( di + 1 < recs.size() ) ? recs[di + 1].second : verts.size();

            if( m_decalIndex.count( recs[di].first ) )
                continue;

            DECAL decal;
            decal.name = recs[di].first;

            for( const PIECE_REC& pr : pieces )
            {
                if( pr.vstart < lo || pr.vstart >= hi )
                    continue;

                DECAL_PIECE piece;
                piece.closed = pr.closed;
                piece.width_mils = pr.width;

                for( int v = 0; v < pr.nverts; ++v )
                {
                    size_t idx = pr.vstart + static_cast<size_t>( v );

                    if( idx < verts.size() )
                        piece.verts.push_back( verts[idx] );
                }

                if( !piece.verts.empty() )
                    decal.pieces.push_back( std::move( piece ) );
            }

            m_decalIndex[decal.name] = m_decals.size();
            m_decals.push_back( std::move( decal ) );
        }
    }

    // --- Used-decal tables (stride 0x6c, a 1-byte lead precedes the name), one per
    // sheet, plus the stride-26 pin terminal pool that immediately follows each.
    // Per record: name@+1, terminal count@+0x2b, cumulative-terminal start@+0x2d (a
    // running prefix sum).  Terminal: X = 2*i16@+3, Y = 2*i16@+5 (decal-relative). ---
    struct UREC { std::string name; int count; int cum; };
    size_t i = DATA_STREAM_OFFSET;

    while( i + USED_DECAL_STRIDE * 4 < n )
    {
        std::string seed0 = nameAt( d, i + 1, 0x20 );
        std::string seed1 = nameAt( d, i + 1 + USED_DECAL_STRIDE, 0x20 );

        if( m_decalIndex.count( seed0 ) && m_decalIndex.count( seed1 ) )
        {
            std::vector<UREC> recs;
            int               expect = 0;

            for( size_t k = 0; ; ++k )
            {
                size_t rec = i + USED_DECAL_STRIDE * k;

                if( rec + USED_DECAL_STRIDE > n )
                    break;

                std::string nm = nameAt( d, rec + 1, 0x20 );
                int         cnt = d[rec + 0x2b];
                int         cum = readU16( d, rec + 0x2d );

                // The running cumulative prefix sum (cum == expect) is the table's integrity
                // invariant; it terminates the run at the first non-record. cnt is a u8 gate
                // terminal count with no cap - a large-pin gate (e.g. a 73-pin FPGA bank)
                // must not truncate the table, or every placement past it falls off and binds
                // to nothing on that sheet.
                if( nm.empty() || cum != expect )
                    break;

                recs.push_back( { nm, cnt, cum } );
                expect = cum + cnt;
            }

            if( recs.size() >= 4 )
            {
                std::vector<std::string> names;
                names.reserve( recs.size() );

                for( const UREC& r : recs )
                    names.push_back( r.name );

                m_usedDecalTables.emplace_back( i, std::move( names ) );

                // The terminal pool follows the table; snap the base +/-2 so the
                // single-terminal power/$OSR decals read their (0,0) origin.
                int    total = recs.back().cum + recs.back().count;
                size_t nominal = i + USED_DECAL_STRIDE * recs.size();
                size_t poolBase = nominal;
                int    bestScore = -1;

                for( size_t cand = ( nominal >= 2 ? nominal - 2 : 0 ); cand <= nominal + 2; ++cand )
                {
                    if( cand + 26 * static_cast<size_t>( total ) > n )
                        continue;

                    int score = 0;

                    for( const UREC& r : recs )
                    {
                        bool anchor = r.count == 1
                                      && ( r.name.rfind( "$OSR", 0 ) == 0 || r.name.rfind( "GND", 0 ) == 0
                                           || r.name == "+5V" || r.name == "+12V" || r.name == "-5V"
                                           || r.name == "-12V" || r.name == "+5VA" );

                        if( !anchor )
                            continue;

                        size_t o = cand + 26 * static_cast<size_t>( r.cum );

                        if( decalMil( d, o + 3 ) == 0 && decalMil( d, o + 5 ) == 0 )
                            ++score;
                    }

                    if( score > bestScore )
                    {
                        bestScore = score;
                        poolBase = cand;
                    }
                }

                // Assign each decal's terminal slice to the geometry-library decal.
                for( const UREC& r : recs )
                {
                    auto it = m_decalIndex.find( r.name );

                    if( it == m_decalIndex.end() || !m_decals[it->second].terminals.empty() )
                        continue;

                    for( int t = 0; t < r.count; ++t )
                    {
                        size_t o = poolBase + 26 * static_cast<size_t>( r.cum + t );

                        if( o + 6 <= n )
                            m_decals[it->second].terminals.emplace_back( decalMil( d, o + 3 ),
                                                                         decalMil( d, o + 5 ) );
                    }
                }

                i = nominal;
                continue;
            }
        }

        ++i;
    }
}


const std::vector<std::string>* PADS_SCH_BINARY_READER::usedDecalTableForOffset( size_t aOffset ) const
{
    const std::vector<std::string>* best = nullptr;

    for( const auto& tbl : m_usedDecalTables )
    {
        if( tbl.first <= aOffset )
            best = &tbl.second;
        else
            break;
    }

    return best;
}


const std::vector<std::string>* PADS_SCH_BINARY_READER::partTypePoolForOffset( size_t aOffset ) const
{
    const std::vector<std::string>* best = nullptr;

    for( const std::pair<size_t, std::vector<std::string>>& pool : m_partTypePools )
    {
        if( pool.first <= aOffset )
            best = &pool.second;
        else
            break;
    }

    return best;
}


// ---------------------------------------------------------------------------
// COMPONENT FIELDS (per-part-type user attributes)
//
// The part-type pool ($OSR_SYMS/$GND_SYMS/$PWR_SYMS header, stride 0x4c) names
// every part-type; a placement's part-type is pool[u16 @ (refdes-0x1c)].  The
// attribute pool (after the global *FIELDS* block) is a flat tagged stream where
// the control byte before each printable string is 0x00 (a KEY or a part-type
// HEADER) or 0x01 (the VALUE of the preceding key).  A header is the part-type
// whose next string is a known attribute key.  Values accumulate per part-type;
// "repeat-lock" stops a block at the first repeated key (the sub-record divider)
// so cross-record bleed does not corrupt the values.
// ---------------------------------------------------------------------------
static const std::set<std::string> FIELD_ATTR_KEYS = {
    "DESCRIPTION", "MFR1", "MFR1 P/N", "MFR2", "MFR2 P/N", "MFR3", "MFR3 P/N",
    "MFR P/N", "VALUE", "Value", "Tolerance", "Voltage Rating", "Power",
    "WDI.Install Option", "WDI P/N", "PCB DECAL", "Geometry.Height", "HOLE",
    "Revision", "Current Rating",
};


struct TAGGED_STR { int tag; size_t off; std::string text; };


static std::vector<TAGGED_STR> walkTaggedStrings( const std::vector<uint8_t>& d, size_t start, size_t end )
{
    std::vector<TAGGED_STR> out;
    int                     prevTag = -1;
    size_t                  i = start;

    while( i < end )
    {
        if( d[i] >= 0x20 && d[i] < 0x7f )
        {
            size_t j = i;

            while( j < end && d[j] >= 0x20 && d[j] < 0x7f )
                ++j;

            out.push_back( { prevTag, i, std::string( reinterpret_cast<const char*>( &d[i] ), j - i ) } );
            prevTag = -1;
            i = j;
        }
        else
        {
            prevTag = d[i];
            ++i;
        }
    }

    return out;
}


void PADS_SCH_BINARY_READER::decodeFields( const std::vector<uint8_t>& d )
{
    static constexpr size_t PT_STRIDE = 0x4c;

    // 1. Part-type pools.  Each schematic sheet carries its own stride-0x4c pool anchored
    // on the $OSR_SYMS/$GND_SYMS/$PWR_SYMS header; a placement's ptidx is the ordinal into
    // the pool of ITS sheet (resolved per sheet in decodePlacements).  Binding every
    // placement to only the first sheet's pool dropped every ptidx past that pool's size.
    for( size_t i = DATA_STREAM_OFFSET; i + 2 * PT_STRIDE + 16 < streamLimit( d ); ++i )
    {
        if( nameAt( d, i, 0x26 ) != "$OSR_SYMS"
            || nameAt( d, i + PT_STRIDE, 0x26 ) != "$GND_SYMS"
            || nameAt( d, i + 2 * PT_STRIDE, 0x26 ) != "$PWR_SYMS" )
            continue;

        std::vector<std::string> names;

        for( size_t j = 0; j < 64; ++j )
        {
            std::string nm = nameAt( d, i + PT_STRIDE * j, 0x26 );

            // Stop at the pool terminator: an empty record, or the trailing 1-char
            // sentinel ('i'/'d') that is not a $-group name.
            if( nm.empty() || ( nm.size() < 2 && nm[0] != '$' ) )
                break;

            names.push_back( nm );
        }

        m_partTypePools.emplace_back( i, std::move( names ) );
    }

    if( m_partTypePools.empty() )
        return;

    // The union of every sheet pool (sheet-0 first, so ordinals 0/1/2 stay the symbol
    // groups) keys the field index below.
    std::set<std::string> seen;

    for( const std::pair<size_t, std::vector<std::string>>& pool : m_partTypePools )
    {
        for( const std::string& nm : pool.second )
        {
            if( seen.insert( nm ).second )
                m_partTypeNames.push_back( nm );
        }
    }

    size_t ptBase = m_partTypePools.front().first;

    // 2. Preferred path: the u32 offset-index table (compaction-saved files) gives
    // each part-type its exact key instances -> 100% precision + recall.
    if( decodeFieldsViaIndex( d, ptBase ) )
        return;

    // 3. Fallback (edit-log files with no consolidated index): the head resolved-
    // attribute stream right after the title-block *FIELDS*.  It is a flat tagged
    // key/value run grouped into per-part-type blocks; an edit/compaction append
    // re-emits whole field groups, so the stream is split into SUB-RECORDS (a
    // repeated key, or a part-type-name token, opens a new one) and each sub-record
    // is bound to its part-type by its own 'WDI P/N' value (the canonical identity),
    // else the name token that opened it.  This recovers the design-controlled
    // fields (DESCRIPTION, VALUE, PCB DECAL, ...) for every named+WDI-bound part-
    // type; the manufacturer fields read stale on edit-log saves and are not
    // corrected here (their resolved value is only serialized via the offset-index
    // table consumed by the compaction path above).
    std::set<std::string> ptNameSet;

    for( const std::string& nm : m_partTypeNames )
    {
        if( nm.size() > 1 && nm[0] != '$' && nm != "d" )
            ptNameSet.insert( nm );
    }

    // Region start = first known part-type name immediately followed by a key (this
    // skips the title *FIELDS* run and any free-standing net-name run before it).
    size_t                  scanEnd = std::min( d.size(), static_cast<size_t>( 0x4000 ) );
    std::vector<TAGGED_STR> head = walkTaggedStrings( d, DATA_STREAM_OFFSET, scanEnd );
    size_t                  attrStart = std::string::npos;

    for( size_t k = 0; k + 1 < head.size(); ++k )
    {
        bool zero = ( head[k].tag == 0 || head[k].tag == -1 );

        if( zero && ptNameSet.count( head[k].text )
            && head[k + 1].tag == 0 && FIELD_ATTR_KEYS.count( head[k + 1].text ) )
        {
            attrStart = head[k].off;
            break;
        }
    }

    if( attrStart == std::string::npos )
        return;

    // End = the first >= 64-byte zero run after the start.
    size_t attrEnd = d.size();
    size_t run = 0;

    for( size_t o = attrStart; o < d.size(); ++o )
    {
        if( d[o] == 0 )
        {
            if( ++run >= 64 )
            {
                attrEnd = o - run + 1;
                break;
            }
        }
        else
        {
            run = 0;
        }
    }

    size_t walkFrom = ( attrStart > 0 && d[attrStart - 1] < 0x20 ) ? attrStart - 1 : attrStart;
    std::vector<TAGGED_STR> toks = walkTaggedStrings( d, walkFrom, attrEnd );

    // Split into sub-records.
    struct SUBREC
    {
        std::string                                      name;
        std::vector<std::pair<std::string, std::string>> kv;
    };

    std::vector<SUBREC> subs;
    SUBREC              cur;

    auto hasKey = []( const SUBREC& r, const std::string& k )
    {
        for( const std::pair<std::string, std::string>& p : r.kv )
        {
            if( p.first == k )
                return true;
        }

        return false;
    };

    auto flush = [&]()
    {
        if( !cur.kv.empty() || !cur.name.empty() )
            subs.push_back( cur );

        cur = SUBREC{};
    };

    for( size_t idx = 0; idx < toks.size(); )
    {
        const TAGGED_STR& t = toks[idx];
        bool              zero = ( t.tag == 0 || t.tag == -1 );

        if( !zero )
        {
            ++idx;
            continue;
        }

        if( FIELD_ATTR_KEYS.count( t.text ) )
        {
            std::string key = t.text;
            std::string val;

            if( idx + 1 < toks.size() && toks[idx + 1].tag == 1 )
            {
                val = toks[idx + 1].text;
                idx += 2;
            }
            else
            {
                ++idx;
            }

            if( hasKey( cur, key ) )
                flush();

            cur.kv.emplace_back( key, val );
        }
        else
        {
            flush();
            cur.name = t.text;
            ++idx;
        }
    }

    flush();

    // Bind each sub-record to its part-type and union the fields (first non-empty
    // value wins).
    for( const SUBREC& r : subs )
    {
        std::string wdi;

        for( const std::pair<std::string, std::string>& p : r.kv )
        {
            if( p.first == "WDI P/N" )
            {
                wdi = p.second;
                break;
            }
        }

        std::string key;

        if( !wdi.empty() && ptNameSet.count( wdi ) )
            key = wdi;
        else if( !r.name.empty() && ptNameSet.count( r.name ) )
            key = r.name;

        if( key.empty() )
            continue;

        std::vector<std::pair<std::string, std::string>>& dst = m_partTypeFields[key];

        for( const std::pair<std::string, std::string>& p : r.kv )
        {
            bool present = false;

            for( std::pair<std::string, std::string>& q : dst )
            {
                if( q.first == p.first )
                {
                    present = true;

                    if( q.second.empty() && !p.second.empty() )
                        q.second = p.second;

                    break;
                }
            }

            if( !present )
                dst.emplace_back( p.first, p.second );
        }
    }

    // Fabrication-free recall fallbacks for the edit-log head path (the compaction
    // index path returns above and never reaches here):
    //   (a) fill an EMPTY VALUE/Tolerance/Voltage Rating from the part-type's OWN
    //       DESCRIPTION CSV (RES/CAP positional schema) -- the description is the
    //       part-type's own serialized attribute, so this fabricates nothing.  Power
    //       is deliberately NOT parsed (a RES description's power token is itself
    //       sometimes stale vs the export).
    //   (b) a self-referential SMD test-point decal (TP_SMD_*) resolves to its own
    //       name; gated to TP_SMD_ so it can never touch a through-hole test point.
    for( std::pair<const std::string, std::vector<std::pair<std::string, std::string>>>& pt :
         m_partTypeFields )
    {
        std::vector<std::pair<std::string, std::string>>& kv = pt.second;

        auto getField = [&]( const std::string& k ) -> std::string
        {
            for( const std::pair<std::string, std::string>& f : kv )
            {
                if( f.first == k )
                    return f.second;
            }

            return std::string();
        };

        auto fillEmpty = [&]( const std::string& k, const std::string& v )
        {
            if( v.empty() )
                return;

            for( std::pair<std::string, std::string>& f : kv )
            {
                if( f.first == k )
                {
                    if( f.second.empty() )
                        f.second = v;

                    return;
                }
            }

            kv.emplace_back( k, v );
        };

        std::string desc = getField( "DESCRIPTION" );

        if( !desc.empty() )
        {
            std::vector<std::string> parts;

            for( size_t s = 0; s <= desc.size(); )
            {
                size_t      comma = desc.find( ',', s );
                size_t      stop = ( comma == std::string::npos ) ? desc.size() : comma;
                std::string tok = desc.substr( s, stop - s );

                size_t a = tok.find_first_not_of( " \t" );
                size_t b = tok.find_last_not_of( " \t" );

                parts.push_back( a == std::string::npos ? std::string() : tok.substr( a, b - a + 1 ) );

                if( comma == std::string::npos )
                    break;

                s = comma + 1;
            }

            const std::string& cls = parts.empty() ? desc : parts[0];

            if( cls == "RES" && parts.size() >= 5 )
            {
                fillEmpty( "VALUE", parts[2] );

                if( !parts[3].empty() && parts[3].back() == '%' )
                    fillEmpty( "Tolerance", parts[3] );
            }
            else if( cls == "CAP" && parts.size() >= 5 )
            {
                fillEmpty( "VALUE", parts[2] );

                if( !parts[3].empty() && parts[3].back() == '%' )
                    fillEmpty( "Tolerance", parts[3] );

                if( !parts[4].empty() && parts[4].back() == 'V' )
                    fillEmpty( "Voltage Rating", parts[4] );
            }
        }

        if( pt.first.rfind( "TP_SMD_", 0 ) == 0 && getField( "PCB DECAL" ) != pt.first )
        {
            bool present = false;

            for( std::pair<std::string, std::string>& f : kv )
            {
                if( f.first == "PCB DECAL" )
                {
                    f.second = pt.first;
                    present = true;
                    break;
                }
            }

            if( !present )
                kv.emplace_back( "PCB DECAL", pt.first );
        }
    }
}


// The flat attribute pool is only string STORAGE; in a compaction-saved file the
// part-type -> attribute association is a separate u32 offset-index table after
// the gate/pin table.  Each record is [class_off][name_off] key_off..., offsets
// relative to the attribute area start (the first part-type-name string).  Each
// key's value is the 0x01-tagged string following that exact key instance, so a
// part-type recovers its own (or an inherited family member's) values exactly.
bool PADS_SCH_BINARY_READER::decodeFieldsViaIndex( const std::vector<uint8_t>& d, size_t poolBase )
{
    auto printable = [&]( size_t o ) { return o < d.size() && d[o] >= 0x20 && d[o] < 0x7f; };

    auto readStr = [&]( size_t o, size_t end ) -> std::string
    {
        size_t j = o;

        while( j < end && d[j] >= 0x20 && d[j] < 0x7f )
            ++j;

        return std::string( reinterpret_cast<const char*>( &d[o] ), j - o );
    };

    // Part-type names (drop the 3 symbol groups and single-char garbage).
    std::set<std::string> names;

    for( size_t k = 3; k < m_partTypeNames.size(); ++k )
    {
        if( m_partTypeNames[k].size() > 1 )
            names.insert( m_partTypeNames[k] );
    }

    if( names.empty() )
        return false;

    // Attribute area = [first part-type-name string, first 16-byte zero run).
    size_t astart = std::string::npos;

    for( size_t i = DATA_STREAM_OFFSET; i < poolBase; )
    {
        if( printable( i ) )
        {
            std::string s = readStr( i, poolBase );

            if( names.count( s ) )
            {
                astart = i;
                break;
            }

            i += s.size();
        }
        else
        {
            ++i;
        }
    }

    if( astart == std::string::npos )
        return false;

    size_t aend = poolBase;
    size_t run = 0;

    for( size_t i = astart; i < poolBase; ++i )
    {
        if( d[i] == 0 )
        {
            if( ++run >= 16 )
            {
                aend = i - run + 1;
                break;
            }
        }
        else
        {
            run = 0;
        }
    }

    // rel-offset -> string, and the subset of rels that are part-type names.
    std::map<uint32_t, std::string> rel2str;
    std::set<uint32_t>              nameRels;

    for( size_t i = astart; i < aend; )
    {
        if( printable( i ) )
        {
            std::string s = readStr( i, aend );
            uint32_t    rel = static_cast<uint32_t>( i - astart );
            rel2str[rel] = s;

            if( names.count( s ) )
                nameRels.insert( rel );

            i += s.size();
        }
        else
        {
            ++i;
        }
    }

    auto isClass = [&]( uint32_t v ) -> bool
    {
        auto it = rel2str.find( v );

        if( it == rel2str.end() || nameRels.count( v ) )
            return false;

        const std::string& s = it->second;

        if( s.empty() || s.size() > 4 )
            return false;

        for( char c : s )
        {
            if( !std::isalpha( static_cast<unsigned char>( c ) ) && c != '.' )
                return false;
        }

        return true;
    };

    // Index table = first u32 class-offset immediately followed by a name-offset.
    size_t tstart = std::string::npos;

    for( size_t o = aend; o + 8 <= poolBase; ++o )
    {
        if( nameRels.count( readU32( d, o + 4 ) ) && isClass( readU32( d, o ) ) )
        {
            tstart = o;
            break;
        }
    }

    if( tstart == std::string::npos )
        return false;

    // Read u32 offsets until two consecutive non-offsets (-1 marks a gap).
    std::vector<long long> vals;
    int                    bad = 0;

    for( size_t o = tstart; o + 4 <= poolBase; o += 4 )
    {
        uint32_t v = readU32( d, o );

        if( rel2str.count( v ) || v == 0 )
        {
            vals.push_back( v );
            bad = 0;
        }
        else if( ++bad >= 2 )
        {
            break;
        }
        else
        {
            vals.push_back( -1 );
        }
    }

    while( !vals.empty() && vals.back() == -1 )
        vals.pop_back();

    // Segment into records on each [class][name] pair.
    struct REC { std::string name; std::vector<uint32_t> keys; };
    std::vector<REC> recs;
    bool             haveCur = false;
    REC              cur;

    for( size_t k = 0; k < vals.size(); )
    {
        if( vals[k] < 0 )
        {
            ++k;
            continue;
        }

        uint32_t  v = static_cast<uint32_t>( vals[k] );
        long long nxt = ( k + 1 < vals.size() ) ? vals[k + 1] : -1;

        if( nxt >= 0 && nameRels.count( static_cast<uint32_t>( nxt ) ) && isClass( v ) )
        {
            if( haveCur )
                recs.push_back( cur );

            cur = REC{ rel2str[static_cast<uint32_t>( nxt )], {} };
            haveCur = true;
            k += 2;
            continue;
        }

        if( haveCur && rel2str.count( v ) )
            cur.keys.push_back( v );

        ++k;
    }

    if( haveCur )
        recs.push_back( cur );

    // Guard the rel-0 false positive: require >= 3 records over >= 3 distinct
    // part-type names (a real consolidated index frames many part-types).
    std::set<std::string> distinct;

    for( const REC& r : recs )
    {
        if( names.count( r.name ) )
            distinct.insert( r.name );
    }

    if( recs.size() < 3 || distinct.size() < 3
        || distinct.size() < std::min<size_t>( 3, m_partTypeNames.size() / 2 ) )
        return false;

    // Build the per-part-type fields; the value is the 0x01-tagged string after
    // each exact key instance, unioned across the part-type's records.
    for( const REC& r : recs )
    {
        std::vector<std::pair<std::string, std::string>>& kv = m_partTypeFields[r.name];

        for( uint32_t kr : r.keys )
        {
            std::string key = readStr( astart + kr, aend );
            size_t      p = astart + kr + key.size();

            if( p < aend && d[p] == 0 )
                ++p;

            std::string val;

            if( p < aend && d[p] == 0x01 && printable( p + 1 ) )
                val = readStr( p + 1, aend );

            bool found = false;

            for( std::pair<std::string, std::string>& pr : kv )
            {
                if( pr.first == key )
                {
                    found = true;

                    if( !val.empty() && pr.second.empty() )
                        pr.second = val;

                    break;
                }
            }

            if( !found )
                kv.emplace_back( key, val );
        }
    }

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
    size_t n = streamLimit( d );

    for( size_t i = DATA_STREAM_OFFSET; i + PART_STRIDE < n; ++i )
    {
        if( !isPartSlot( d, i ) )
            continue;

        // A run starts where the previous stride-136 slot is not a part slot.
        if( i >= DATA_STREAM_OFFSET + PART_STRIDE && isPartSlot( d, i - PART_STRIDE ) )
            continue;

        size_t              p = i;
        size_t              runFirst = m_placements.size();
        std::vector<size_t> runOffsets;

        while( p + PART_STRIDE < n && isPartSlot( d, p ) )
        {
            PLACEMENT pl;
            size_t    z = p;

            while( z < p + 40 && d[z] != 0 )
                ++z;

            pl.reference.assign( reinterpret_cast<const char*>( &d[p] ), z - p );
            pl.x_mils = designMil( readU16( d, p + PART_X_OFF ) );
            pl.y_mils = designMil( readU16( d, p + PART_Y_OFF ) );

            // Inline REF-DES field placement (subrecord at ksy+0x08, ksy = refdes-0x3e):
            // half-mil deltas page-relative to the symbol origin, u16 tenths-degree angle.
            pl.refdesPlace.dx_mils = decalMil( d, p - 0x36 );
            pl.refdesPlace.dy_mils = decalMil( d, p - 0x34 );
            pl.refdesPlace.orientation_deg = readU16( d, p - 0x32 ) / 10;
            pl.refdesPlace.visible = true;
            pl.refdesPlace.valid = true;
            // Orientation is a u16 angle in tenths of a degree; the prior 0x84 byte test
            // read only its low byte (0x0384 = 900 = 90deg) and so could not see 180/270.
            pl.rotation = readU16( d, p + PART_ORI_OFF ) / 10;
            pl.sheetIndex = sheetIndexForOffset( p );

            // Bind the gate decal: handle at refdes-0x1a indexes this sheet's
            // used-decal table (base = the built-in handle count).
            if( p >= 0x1a )
            {
                const std::vector<std::string>* table = usedDecalTableForOffset( p );
                uint16_t handle = readU16( d, p - 0x1a );

                if( table && handle >= m_decalBuiltinCount )
                {
                    size_t idx = handle - m_decalBuiltinCount;

                    if( idx < table->size() )
                        pl.decalName = ( *table )[idx];
                }
            }

            // Bind the part-type (block+4 ordinal into THIS sheet's part-type pool) and its
            // component attribute fields.  ptidx indexes the placement's own sheet pool, not
            // the global first-sheet pool.
            if( p >= 0x1c )
            {
                uint16_t                        ptIdx = readU16( d, p - 0x1c );
                const std::vector<std::string>* pool = partTypePoolForOffset( p );

                if( pool && ptIdx < pool->size() )
                {
                    pl.partType = ( *pool )[ptIdx];
                    auto it = m_partTypeFields.find( pl.partType );

                    if( it != m_partTypeFields.end() )
                        pl.fields = it->second;
                }
            }

            // Multi-gate grouping: a part-type with more than one real (pin-bearing) gate is
            // a multi-unit symbol. Its placements carry a -<gate> suffix; the base reference
            // groups them and the unit slot is at block+0xc (refdes-0x14, 0-based).
            pl.baseRef = pl.reference;

            auto gpIt = m_partTypeGatePins.find( pl.partType );

            if( gpIt != m_partTypeGatePins.end() )
            {
                int realGates = 0;

                for( const std::vector<PIN_INFO>& g : gpIt->second )
                    realGates += g.empty() ? 0 : 1;

                size_t dash = pl.reference.rfind( '-' );

                if( realGates > 1 && dash != std::string::npos && dash > 0 )
                {
                    pl.baseRef = pl.reference.substr( 0, dash );
                    pl.multiUnit = true;
                    int slot = static_cast<int>( readU16( d, p - 0x14 ) ) + 1;
                    pl.unit = ( slot >= 1 && slot <= static_cast<int>( gpIt->second.size() ) ) ? slot : 1;
                }
            }

            runOffsets.push_back( p );
            m_placements.push_back( std::move( pl ) );

            p += PART_STRIDE;
        }

        // The 24-byte per-field placement array for this sheet follows its placement block.
        assignFieldPlacements( d, p, runFirst, runOffsets );

        i = p - 1;  // continue past the run
    }

    // Field text height is a single design-wide value; parts with no field array (only a
    // refdes/value) carry no height record, so backfill them from the design's field height.
    int designFieldHeight = 0;

    for( const PLACEMENT& pl : m_placements )
    {
        for( const FIELD_PLACEMENT& fp : pl.fieldPlaces )
        {
            if( fp.height_mils > 0 )
            {
                designFieldHeight = fp.height_mils;
                break;
            }
        }

        if( designFieldHeight > 0 )
            break;
    }

    if( designFieldHeight > 0 )
    {
        for( PLACEMENT& pl : m_placements )
        {
            if( pl.refdesPlace.valid && pl.refdesPlace.height_mils == 0 )
                pl.refdesPlace.height_mils = designFieldHeight;
        }
    }
}


// Each sheet serializes its stride-136 placement block, then a 24-byte field-placement array.
// The array start is the first 24-byte record after the block whose 6-byte lead is zero and
// whose +0x16 terminator is 0xFFFF (and whose 24-byte predecessor is not such a record). Each
// placement consumes u16 @ (refdes-0x3e + 0x2e) records, in placement order.
void PADS_SCH_BINARY_READER::assignFieldPlacements( const std::vector<uint8_t>& d, size_t aBlockEnd,
                                                    size_t aRunFirst,
                                                    const std::vector<size_t>& aRunOffsets )
{
    static constexpr size_t REC = 24;

    auto isFieldRec = [&]( size_t o ) -> bool
    {
        if( o + REC > d.size() )
            return false;

        for( size_t k = 0; k < 6; ++k )
        {
            if( d[o + k] != 0 )
                return false;
        }

        return readU16( d, o + 0x16 ) == 0xFFFF;
    };

    size_t start = std::string::npos;

    for( size_t o = aBlockEnd; o + REC <= d.size() && o < aBlockEnd + 0x4000; ++o )
    {
        if( isFieldRec( o ) && !( o >= REC && isFieldRec( o - REC ) ) )
        {
            start = o;
            break;
        }
    }

    if( start == std::string::npos )
        return;

    size_t cursor = start;

    for( size_t idx = 0; idx < aRunOffsets.size(); ++idx )
    {
        size_t    ksy = aRunOffsets[idx] - 0x3e;
        uint16_t  fieldCount = readU16( d, ksy + 0x2e );
        PLACEMENT& pl = m_placements[aRunFirst + idx];

        for( uint16_t k = 0; k < fieldCount; ++k )
        {
            if( cursor + REC > d.size() )
                break;

            uint8_t vis = d[cursor + 0x13];

            // A real field record carries a {0,1,3} visibility code; anything else means the
            // array has run into unrelated data (the 420B edit-log sheet) - stop, never fake.
            if( vis != 0 && vis != 1 && vis != 3 )
                break;

            FIELD_PLACEMENT fp;
            fp.dx_mils = decalMil( d, cursor + 0x06 );
            fp.dy_mils = decalMil( d, cursor + 0x08 );
            fp.orientation_deg = readU16( d, cursor + 0x0a ) / 10;
            fp.height_mils = readU16( d, cursor + 0x10 );
            fp.visible = ( vis != 0 );
            fp.valid = true;
            pl.fieldPlaces.push_back( fp );

            cursor += REC;
        }

        // The inline REF-DES record carries no text height; share the part's field height.
        if( pl.refdesPlace.valid && !pl.fieldPlaces.empty() )
            pl.refdesPlace.height_mils = pl.fieldPlaces.front().height_mils;
    }
}


void PADS_SCH_BINARY_READER::decodePinNames( const std::vector<uint8_t>& d )
{
    static constexpr size_t      PT_STRIDE = 0x4c;
    static constexpr size_t      PIN_STRIDE = 24;
    static const std::string     TYPES = "USLBTCPGZ";

    // A stride-24 pin record: type letter @+21, ASCII pin number @+4 (NUL-terminated), a u32
    // named-flag @+0 (0xFFFFFFFF = unnamed).  The strict zero tail is intentionally NOT
    // required so the handle-bearing variant (a live heap handle in bytes 22/23) still reads.
    auto isPinRec = [&]( size_t o ) -> bool
    {
        if( o + PIN_STRIDE > d.size() || TYPES.find( static_cast<char>( d[o + 21] ) ) == std::string::npos )
            return false;

        uint8_t c = d[o + 4];

        if( !( ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'Z' ) ) )
            return false;

        size_t z = o + 4;

        while( z < o + 0x14 && d[z] != 0 )
        {
            if( d[z] < 0x21 || d[z] >= 0x7f )
                return false;

            ++z;
        }

        return z > o + 4 && z < o + 0x14;
    };

    // A name-pool token: a printable run (first char non-space) up to 24 bytes, NUL-terminated.
    auto tokenEnd = [&]( size_t o ) -> size_t
    {
        if( o >= d.size() || d[o] < 0x21 || d[o] >= 0x7f )
            return 0;

        size_t z = o;

        while( z < d.size() && z < o + 24 && d[z] != 0 )
        {
            if( d[z] < 0x20 || d[z] >= 0x7f )
                return 0;

            ++z;
        }

        return ( z < d.size() && d[z] == 0 ) ? z + 1 : 0;
    };

    auto isNameRun = [&]( size_t o, int want ) -> bool
    {
        int    c = 0;
        size_t j = o;

        for( size_t e = tokenEnd( j ); e; e = tokenEnd( j ) )
        {
            j = e;

            if( ++c >= want )
                return true;
        }

        return false;
    };

    for( const std::pair<size_t, std::vector<std::string>>& pool : m_partTypePools )
    {
        size_t base = pool.first;

        // Re-walk the pool record run reading the +0x30 pin cursor (and the +0x2c gate cursor
        // as the monotone guard), stopping where either wraps - the junk boundary record.
        struct POOLREC { std::string name; uint32_t f2c; uint32_t f30; };
        std::vector<POOLREC> recs;
        uint32_t             prev30 = 0;
        uint32_t             prev2c = 0;

        for( size_t j = 0;; ++j )
        {
            size_t rec = base + PT_STRIDE * j;

            if( rec + PT_STRIDE > d.size() )
                break;

            size_t z = rec;

            while( z < rec + 0x26 && z < d.size() && d[z] != 0 && d[z] >= 0x20 && d[z] < 0x7f )
                ++z;

            if( z == rec || ( z < rec + 0x26 && d[z] != 0 ) )
                break;

            uint32_t f2c = readU32( d, rec + 0x2c );
            uint32_t f30 = readU32( d, rec + 0x30 );

            if( ( j && ( f30 < prev30 || f2c < prev2c ) ) || f30 > 1000000 || f2c > 1000000 )
                break;

            recs.push_back( { std::string( reinterpret_cast<const char*>( &d[rec] ), z - rec ), f2c, f30 } );
            prev30 = f30;
            prev2c = f2c;
        }

        if( recs.size() < 4 )
            continue;

        size_t pp = base + PT_STRIDE * ( recs.size() + 1 );
        size_t scanEnd = std::min( pp + 0x2000, d.size() );
        size_t start = std::string::npos;

        for( size_t o = pp; o + PIN_STRIDE < scanEnd; ++o )
        {
            if( isPinRec( o ) )
            {
                start = o;
                break;
            }
        }

        if( start == std::string::npos )
            continue;

        // The stride-12 GATE descriptor pool sits between the part-type pool and the pin
        // pool; byte 8 of each record is that gate's pin count, indexed by the part-type
        // record's +0x2c cumulative gate cursor.
        std::vector<uint8_t> gateNpins;

        for( size_t go = base + PT_STRIDE * recs.size(); go + 12 <= start; go += 12 )
            gateNpins.push_back( d[go + 8] );

        struct PINREC { std::string number; char type; bool named; };
        std::vector<PINREC> pins;
        size_t              o = start;

        while( o + PIN_STRIDE <= d.size() )
        {
            if( isNameRun( o, 6 ) || !isPinRec( o ) )
                break;

            size_t z = o + 4;

            while( z < o + 0x14 && d[z] != 0 )
                ++z;

            pins.push_back( { std::string( reinterpret_cast<const char*>( &d[o + 4] ), z - ( o + 4 ) ),
                              static_cast<char>( d[o + 21] ), readU32( d, o ) != 0xFFFFFFFF } );
            o += PIN_STRIDE;
        }

        int    want = 0;
        size_t lastStop = std::min<size_t>( recs.back().f30, pins.size() );

        for( size_t i = 0; i < lastStop; ++i )
            want += pins[i].named ? 1 : 0;

        size_t nameBase = std::string::npos;

        for( size_t s = o; s + 1 < d.size(); ++s )
        {
            if( isNameRun( s, std::max( 4, want ) ) )
            {
                nameBase = s;
                break;
            }
        }

        std::vector<std::string> run;

        for( size_t t = nameBase; nameBase != std::string::npos; )
        {
            size_t e = tokenEnd( t );

            if( !e )
                break;

            run.emplace_back( reinterpret_cast<const char*>( &d[t] ), e - t - 1 );
            t = e;
        }

        size_t cursor = 0;

        for( size_t k = 0; k < recs.size(); ++k )
        {
            size_t sliceStart = recs[k].f30;
            size_t sliceStop = ( k + 1 < recs.size() ) ? recs[k + 1].f30 : pins.size();
            sliceStop = std::min( sliceStop, pins.size() );

            std::vector<PIN_INFO> out;

            for( size_t i = sliceStart; i < sliceStop; ++i )
            {
                PIN_INFO pi;
                pi.number = pins[i].number;
                pi.type = pins[i].type;

                if( pins[i].named )
                {
                    if( cursor < run.size() )
                        pi.name = run[cursor];

                    ++cursor;
                }

                out.push_back( std::move( pi ) );
            }

            const std::string& ptName = recs[k].name;

            if( ptName.empty() || ptName[0] == '$' )
                continue;

            // Split this part-type's flat pin list into its gates for multi-unit symbols.
            std::vector<std::vector<PIN_INFO>> gates;
            uint32_t                           g0 = recs[k].f2c;
            uint32_t                           g1 = ( k + 1 < recs.size() ) ? recs[k + 1].f2c
                                                                           : static_cast<uint32_t>( gateNpins.size() );
            size_t                             gi = 0;

            for( uint32_t g = g0; g < g1 && g < gateNpins.size(); ++g )
            {
                std::vector<PIN_INFO> gate;

                for( uint8_t n = 0; n < gateNpins[g] && gi < out.size(); ++n )
                    gate.push_back( out[gi++] );

                gates.push_back( std::move( gate ) );
            }

            // The library is redefined per sheet; keep the first complete (named) definition.
            auto it = m_partTypePins.find( ptName );

            if( it == m_partTypePins.end()
                || std::all_of( it->second.begin(), it->second.end(),
                                []( const PIN_INFO& p ) { return p.name.empty(); } ) )
            {
                m_partTypeGatePins[ptName] = std::move( gates );
                m_partTypePins[ptName] = std::move( out );
            }
        }
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
// in sheet order (file order, not active-first; see decodeSheets).
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

    while( i + SPLIT_STRIDE + VERTEX_STRIDE <= streamLimit( d ) )
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
        int    runSheet = sheetIndexForOffset( run.vertexOffset );
        size_t prev = 0;

        for( size_t k = 0; k < run.cumulative.size(); ++k )
        {
            size_t connEnd = prev + static_cast<size_t>( run.nverts[k] );

            if( connEnd > vertices.size() )
                break;

            m_wirePolylines.emplace_back( vertices.begin() + prev, vertices.begin() + connEnd );
            m_wirePolylineSheets.push_back( runSheet );

            // The explicit slice between this connection's end and the next
            // cumulative start is a bus polyline.
            size_t gapEnd = run.cumulative[k];

            if( gapEnd > connEnd && gapEnd <= vertices.size() )
            {
                m_busPolylines.emplace_back( vertices.begin() + connEnd, vertices.begin() + gapEnd );
                m_busPolylineSheets.push_back( runSheet );
            }

            prev = run.cumulative[k];
        }

        size_t terminalNverts = static_cast<size_t>( run.nverts.back() );

        if( prev + terminalNverts <= vertices.size() )
        {
            m_wirePolylines.emplace_back( vertices.begin() + prev, vertices.begin() + prev + terminalNverts );
            m_wirePolylineSheets.push_back( runSheet );
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
    size_t offset = 0;
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

    for( size_t i = DATA_STREAM_OFFSET; i + TEXT_STRIDE <= streamLimit( d ); ++i )
    {
        TEXT_RECORD rec;

        if( isTextRecord( d, i, rec ) )
        {
            rec.offset = i;
            records.push_back( rec );
        }
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
        item.sheetIndex = sheetIndexForOffset( records[k].offset );

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
// runs appear in sheet order (file order, not active-first), so every run
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

    while( i + JUNCTION_STRIDE <= streamLimit( d ) )
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

        int runSheet = sheetIndexForOffset( i );

        while( j + JUNCTION_STRIDE <= d.size() && isJunctionRecord( d, j, pageWidth, pageHeight ) )
        {
            JUNCTION jct;
            jct.x_mils = designMil( readU16( d, j ) );
            jct.y_mils = designMil( readU16( d, j + 2 ) );
            jct.sheetIndex = runSheet;
            run.push_back( jct );
            j += JUNCTION_STRIDE;
        }

        if( run.size() >= 2 )
            m_junctions.insert( m_junctions.end(), run.begin(), run.end() );

        i = j;
    }
}


// ---------------------------------------------------------------------------
// NET LABELS (off-page refs / power ports)
//
// Each connection segment (stride 40, marker 0x02fd/0x03fd @+0x0c) carries the
// net's ordinal into the 88-byte net table at +0x1a and two endpoint refs at
// +0x1e/+0x20 whose high nibble 2 means "off-page ref O<idx>".  The off-page
// symbols live in a stride-0x20 array (X@+0, Y@+2 page-biased; 1-based index at
// +0x14).  Joining them per sheet places a net label at each off-page position.
// ---------------------------------------------------------------------------
static constexpr size_t NET_STRIDE = 88;
static constexpr size_t SEG_STRIDE = 40;
static constexpr size_t OFFPAGE_STRIDE = 0x20;


static bool isNetRecord( const std::vector<uint8_t>& d, size_t o )
{
    if( o + NET_STRIDE > d.size() )
        return false;

    if( d[o + 0x48] != 0xFF || d[o + 0x49] != 0xFF || d[o + 0x4a] != 0xFF || d[o + 0x4b] != 0xFF )
        return false;

    size_t e = o + 0x10;

    while( e < o + 0x48 && d[e] != 0 )
        ++e;

    size_t len = e - ( o + 0x10 );

    if( len == 0 || len > 47 )
        return false;

    for( size_t i = o + 0x10; i < e; ++i )
    {
        if( d[i] < 0x20 || d[i] >= 0x7f )
            return false;
    }

    return true;
}


static bool isSegmentMarker( uint16_t aMarker )
{
    return aMarker == 0x02FD || aMarker == 0x03FD;
}


void PADS_SCH_BINARY_READER::decodeNetLabels( const std::vector<uint8_t>& d )
{
    // 1. Net table = the longest contiguous stride-88 net-record run.
    size_t           netBase = 0;
    size_t           netCount = 0;
    std::set<size_t> seenNet;

    for( size_t i = DATA_STREAM_OFFSET; i + NET_STRIDE < streamLimit( d ); )
    {
        if( !isNetRecord( d, i ) )
        {
            ++i;
            continue;
        }

        size_t start = i;

        while( start >= DATA_STREAM_OFFSET + NET_STRIDE && isNetRecord( d, start - NET_STRIDE ) )
            start -= NET_STRIDE;

        if( seenNet.count( start ) )
        {
            i += NET_STRIDE;
            continue;
        }

        seenNet.insert( start );
        size_t p = start;
        size_t cnt = 0;

        while( isNetRecord( d, p ) )
        {
            ++cnt;
            p += NET_STRIDE;
        }

        if( cnt > netCount )
        {
            netCount = cnt;
            netBase = start;
        }

        i = p;
    }

    if( netCount == 0 )
        return;

    // The longest contiguous stride-88 run is the net table; its row count is the
    // net controller's authoritative object count (pool8.used_count). Prefer the
    // directory count over the scan's run-length terminator when present, retiring
    // the heuristic tail; the scan still anchors the table base until the @0x250
    // block walk lands. m_netTableScanCount preserves the scanned length so a corpus
    // test can prove the two agree.
    m_netTableScanCount = netCount;

    if( size_t poolNetCount = m_pools.Count( POOL_DIRECTORY::NETS ) )
        netCount = poolNetCount;

    auto netName = [&]( size_t k ) -> std::string
    {
        return nameAt( d, netBase + NET_STRIDE * k + 0x10, 0x38 );
    };

    // 2. Segment pools -> (sheet, off-page index) -> net name (first segment wins;
    // all referencing segments agree).
    std::map<std::pair<int, int>, std::string> offNet;
    std::set<size_t>                           seenSeg;

    for( size_t i = DATA_STREAM_OFFSET; i + SEG_STRIDE < streamLimit( d ); )
    {
        if( !isSegmentMarker( readU16( d, i + 0x0c ) ) )
        {
            ++i;
            continue;
        }

        size_t start = i;

        while( start >= DATA_STREAM_OFFSET + SEG_STRIDE
               && isSegmentMarker( readU16( d, start - SEG_STRIDE + 0x0c ) ) )
            start -= SEG_STRIDE;

        if( seenSeg.count( start ) )
        {
            i += SEG_STRIDE;
            continue;
        }

        seenSeg.insert( start );
        size_t p = start;

        while( p + SEG_STRIDE <= d.size() && isSegmentMarker( readU16( d, p + 0x0c ) ) )
        {
            uint16_t ni = readU16( d, p + 0x1a );

            if( ni < netCount )
            {
                std::string net = netName( ni );
                int         sheet = sheetIndexForOffset( p );

                for( int off : { 0x1e, 0x20 } )
                {
                    uint16_t v = readU16( d, p + off );

                    if( ( v >> 12 ) == 2 )
                        offNet.emplace( std::make_pair( sheet, v & 0xfff ), net );
                }
            }

            p += SEG_STRIDE;
        }

        i = p;
    }

    // The symbol-group of an off-page is read from its used-decal handle (interleaved one
    // slot back at record-0x12), indexed into the CANONICAL group table: the used-decal
    // table whose leading entries are the full $OSR_* off-sheet-reference orientation set.
    // Per-sheet tables that drop $OSR variants shift every handle, so the nearest table is
    // wrong here; the canonical (most-$OSR-leading) table is the design-global group block.
    const std::vector<std::string>* canonical = nullptr;
    size_t                          bestOsr = 0;

    for( const auto& tbl : m_usedDecalTables )
    {
        size_t osr = 0;

        for( const std::string& nm : tbl.second )
        {
            if( nm.rfind( "$OSR", 0 ) == 0 )
                ++osr;
            else
                break;
        }

        if( osr > bestOsr )
        {
            bestOsr = osr;
            canonical = &tbl.second;
        }
    }

    auto offpageKind = [&]( size_t recOff ) -> NETLABEL_KIND
    {
        uint8_t flag = d[recOff + 0x08];

        if( flag == 0xFF )
            return NETLABEL_KIND::BUS;

        if( flag == 0xFE )
            return NETLABEL_KIND::LOCAL;

        // A symbol off-page: off-sheet ref ($OSR group) vs power/ground (any other group).
        if( recOff >= 0x12 && canonical )
        {
            uint16_t handle = readU16( d, recOff - 0x12 );

            if( handle >= m_decalBuiltinCount
                && ( handle - m_decalBuiltinCount ) < canonical->size() )
            {
                const std::string& grp = ( *canonical )[handle - m_decalBuiltinCount];

                if( grp.rfind( "$OSR", 0 ) != 0 )
                    return NETLABEL_KIND::POWER;
            }
        }

        return NETLABEL_KIND::GLOBAL;
    };

    // 3. Off-page arrays -> emit a label per decoded (sheet, index).
    int pageWidth = 0;
    int pageHeight = 0;
    pageExtent( d, pageWidth, pageHeight );

    auto coordOk = [&]( size_t o ) -> bool
    {
        if( o + 4 > d.size() )
            return false;

        int x = designMil( readU16( d, o ) );
        int y = designMil( readU16( d, o + 2 ) );

        return x % 50 == 0 && y % 50 == 0 && x >= 0 && x <= pageWidth && y >= 0 && y <= pageHeight;
    };

    std::set<std::pair<int, int>> emitted;

    for( size_t i = DATA_STREAM_OFFSET; i + OFFPAGE_STRIDE * 2 < streamLimit( d ); )
    {
        if( !coordOk( i ) || !coordOk( i + OFFPAGE_STRIDE )
            || readU16( d, i + OFFPAGE_STRIDE + 0x14 ) != readU16( d, i + 0x14 ) + 1 )
        {
            ++i;
            continue;
        }

        size_t j = i;
        int    prevSeq = static_cast<int>( readU16( d, i + 0x14 ) ) - 1;

        while( j + OFFPAGE_STRIDE <= d.size() && coordOk( j ) )
        {
            int seq = readU16( d, j + 0x14 );

            if( seq != prevSeq + 1 )
                break;

            int  sheet = sheetIndexForOffset( j );
            auto key = std::make_pair( sheet, seq - 1 );
            auto it = offNet.find( key );

            if( it != offNet.end() && !emitted.count( key ) )
            {
                emitted.insert( key );

                NET_LABEL lbl;
                lbl.x_mils = designMil( readU16( d, j ) );
                lbl.y_mils = designMil( readU16( d, j + 2 ) );
                lbl.netName = it->second;
                lbl.sheetIndex = sheet;
                lbl.kind = offpageKind( j );
                m_netLabels.push_back( std::move( lbl ) );
            }

            prevSeq = seq;
            j += OFFPAGE_STRIDE;
        }

        i = j;
    }
}


// ---------------------------------------------------------------------------
// SCHEMATIC BUILD
// ---------------------------------------------------------------------------
static ELECTRICAL_PINTYPE pinTypeFromLetter( char aLetter )
{
    switch( aLetter )
    {
    case 'S': return ELECTRICAL_PINTYPE::PT_OUTPUT;
    case 'L': return ELECTRICAL_PINTYPE::PT_INPUT;
    case 'B': return ELECTRICAL_PINTYPE::PT_BIDI;
    case 'T':
    case 'Z': return ELECTRICAL_PINTYPE::PT_TRISTATE;
    case 'C': return ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR;
    case 'P':
    case 'G': return ELECTRICAL_PINTYPE::PT_POWER_IN;
    default:  return ELECTRICAL_PINTYPE::PT_PASSIVE;
    }
}


// Orient a stub pin from its decal geometry. A PADS terminal is the pin's connection point and
// sits outside the gate body outline; the pin faces the bbox edge it lies beyond and its length
// reaches that edge. Decal coords are PADS Y-up, so an above-body terminal points down and a
// below-body one points up. A terminal inside the outline (no edge crossed) falls back to the
// body centroid direction. @p aMinX..aMaxY bound the body, @p aCx/@p aCy are its centroid and
// @p aTx/@p aTy the terminal, all in decal mils.
static void orientStubPin( SCH_PIN* aPin, int aMinX, int aMaxX, int aMinY, int aMaxY, double aCx,
                           double aCy, int aTx, int aTy )
{
    int leftOver = aMinX - aTx;
    int rightOver = aTx - aMaxX;
    int topOver = aTy - aMaxY;
    int botOver = aMinY - aTy;
    int over = std::max( std::max( leftOver, rightOver ), std::max( topOver, botOver ) );
    int lenMils = 100;

    if( over <= 0 )
    {
        int ddx = static_cast<int>( aCx ) - aTx;
        int ddy = static_cast<int>( aCy ) - aTy;

        if( std::abs( ddx ) >= std::abs( ddy ) )
            aPin->SetOrientation( ddx >= 0 ? PIN_ORIENTATION::PIN_RIGHT : PIN_ORIENTATION::PIN_LEFT );
        else
            aPin->SetOrientation( ddy < 0 ? PIN_ORIENTATION::PIN_DOWN : PIN_ORIENTATION::PIN_UP );
    }
    else if( over == leftOver )
    {
        aPin->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );
        lenMils = leftOver;
    }
    else if( over == rightOver )
    {
        aPin->SetOrientation( PIN_ORIENTATION::PIN_LEFT );
        lenMils = rightOver;
    }
    else if( over == topOver )
    {
        aPin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );
        lenMils = topOver;
    }
    else
    {
        aPin->SetOrientation( PIN_ORIENTATION::PIN_UP );
        lenMils = botOver;
    }

    aPin->SetLength( schIUScale.MilsToIU( lenMils > 0 ? lenMils : 100 ) );
}


// Add one gate's body shapes and pins to a LIB_SYMBOL on unit @p aUnit (0 = common to all
// units, for single-unit symbols). When @p aDecal is null (an unplaced or unbound gate) the
// pins are laid out in a column so the unit stays electrically complete. Pins are labelled
// from @p aGatePins when their count matches the decal's terminals; otherwise bare-numbered.
static void addGateUnit( LIB_SYMBOL* aLib, const PADS_SCH_BINARY::DECAL* aDecal,
                         const std::vector<PADS_SCH_BINARY::PIN_INFO>& aGatePins, int aUnit )
{
    using namespace PADS_SCH_BINARY;

    if( aDecal )
    {
        for( const DECAL_PIECE& piece : aDecal->pieces )
        {
            SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

            for( const std::pair<int, int>& v : piece.verts )
                shape->AddPoint( VECTOR2I( schIUScale.MilsToIU( v.first ),
                                           -schIUScale.MilsToIU( v.second ) ) );

            int width = piece.width_mils > 0 ? schIUScale.MilsToIU( piece.width_mils ) : 0;
            shape->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );

            if( piece.closed )
                shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );

            shape->SetUnit( aUnit );
            aLib->AddDrawItem( shape );
        }

        int    minX = 0, maxX = 0, minY = 0, maxY = 0;
        double cx = 0.0;
        double cy = 0.0;
        int    nv = 0;

        for( const DECAL_PIECE& piece : aDecal->pieces )
        {
            for( const std::pair<int, int>& v : piece.verts )
            {
                if( nv == 0 )
                {
                    minX = maxX = v.first;
                    minY = maxY = v.second;
                }
                else
                {
                    minX = std::min( minX, v.first );
                    maxX = std::max( maxX, v.first );
                    minY = std::min( minY, v.second );
                    maxY = std::max( maxY, v.second );
                }

                cx += v.first;
                cy += v.second;
                ++nv;
            }
        }

        if( nv > 0 )
        {
            cx /= nv;
            cy /= nv;
        }

        bool   label = aGatePins.size() == aDecal->terminals.size();
        int    pinNumber = 1;
        size_t ti = 0;

        for( const std::pair<int, int>& term : aDecal->terminals )
        {
            SCH_PIN* pin = new SCH_PIN( aLib );
            pin->SetPosition( VECTOR2I( schIUScale.MilsToIU( term.first ),
                                        -schIUScale.MilsToIU( term.second ) ) );

            if( label )
            {
                const PIN_INFO& info = aGatePins[ti];
                pin->SetNumber( wxString::FromUTF8( info.number ) );

                if( !info.name.empty() )
                    pin->SetName( wxString::FromUTF8( info.name ) );

                pin->SetType( pinTypeFromLetter( info.type ) );
            }
            else
            {
                pin->SetNumber( wxString::Format( wxT( "%d" ), pinNumber++ ) );
                pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
            }

            ++ti;

            orientStubPin( pin, minX, maxX, minY, maxY, cx, cy, term.first, term.second );
            pin->SetUnit( aUnit );
            aLib->AddDrawItem( pin );
        }
    }
    else
    {
        int y = 0;

        for( const PIN_INFO& info : aGatePins )
        {
            SCH_PIN* pin = new SCH_PIN( aLib );
            pin->SetPosition( VECTOR2I( 0, schIUScale.MilsToIU( y ) ) );
            y -= 100;
            pin->SetNumber( wxString::FromUTF8( info.number ) );

            if( !info.name.empty() )
                pin->SetName( wxString::FromUTF8( info.name ) );

            pin->SetType( pinTypeFromLetter( info.type ) );
            pin->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );
            pin->SetLength( schIUScale.MilsToIU( 100 ) );
            pin->SetUnit( aUnit );
            aLib->AddDrawItem( pin );
        }
    }
}


std::unique_ptr<LIB_SYMBOL> PADS_SCH_BINARY_READER::buildMultiUnitLib( const std::string& aBase,
                                                                      const std::string& aPartType ) const
{
    auto git = m_partTypeGatePins.find( aPartType );

    if( git == m_partTypeGatePins.end() || git->second.size() < 2 )
        return nullptr;

    const std::vector<std::vector<PIN_INFO>>& slices = git->second;
    int                                       ngates = static_cast<int>( slices.size() );

    auto lib = std::make_unique<LIB_SYMBOL>( wxString::FromUTF8( aBase ) );
    lib->SetUnitCount( ngates, false );
    lib->LockUnits( true );

    // The placed gates of this part give each unit its body decal.
    std::map<int, std::string> placed;

    for( const PLACEMENT& pl : m_placements )
    {
        if( pl.multiUnit && pl.baseRef == aBase && pl.unit >= 1 && pl.unit <= ngates )
            placed[pl.unit] = pl.decalName;
    }

    for( int u = 1; u <= ngates; ++u )
    {
        const DECAL* decal = nullptr;
        auto         pit = placed.find( u );

        if( pit != placed.end() && !pit->second.empty() )
        {
            auto dit = m_decalIndex.find( pit->second );

            if( dit != m_decalIndex.end() )
                decal = &m_decals[dit->second];
        }

        addGateUnit( lib.get(), decal, slices[u - 1], u );
    }

    return lib;
}


int PADS_SCH_BINARY_READER::appendSheetContent( SCH_SCREEN* aScreen, const SCH_SHEET_PATH& aPath,
                                                int aSheetIndex, int aPageHeightIU ) const
{
    auto milToY = [&]( int aMil ) -> int
    {
        return aPageHeightIU - schIUScale.MilsToIU( aMil );
    };

    auto onSheet = [&]( int aIdx ) -> bool
    {
        return aSheetIndex < 0 || aIdx == aSheetIndex;
    };

    SCH_SHEET_PATH path = aPath;
    int            appended = 0;

    // --- SYMBOLS (generic placeholder at recovered position + orientation) ---
    for( const PLACEMENT& pl : m_placements )
    {
        if( !onSheet( pl.sheetIndex ) )
            continue;

        // A multi-gate part becomes one shared N-unit LIB_SYMBOL; this placement is one of
        // its units, sharing the base reference. Single-gate parts use the bound CAE-decal as
        // the symbol identity + body, or a generic placeholder when the decal is unbound.
        auto    decalIt = m_decalIndex.find( pl.decalName );
        bool    haveDecal = !pl.decalName.empty() && decalIt != m_decalIndex.end();
        wxString name = haveDecal ? wxString::FromUTF8( pl.decalName )
                                  : wxString::Format( wxT( "PADS_%s" ), wxString::FromUTF8( pl.reference ) );

        std::unique_ptr<LIB_SYMBOL> libSym;
        bool                        multiUnitBuilt = false;

        if( pl.multiUnit )
        {
            libSym = buildMultiUnitLib( pl.baseRef, pl.partType );

            if( libSym )
            {
                name = wxString::FromUTF8( pl.baseRef );
                multiUnitBuilt = true;
            }
        }

        if( !libSym )
            libSym = std::make_unique<LIB_SYMBOL>( name );

        if( !multiUnitBuilt && haveDecal )
        {
            // Each decal drawing piece becomes a polyline body shape; PADS decal
            // geometry is Y-up, so the symbol-local Y is negated (library convention).
            for( const DECAL_PIECE& piece : m_decals[decalIt->second].pieces )
            {
                SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

                for( const std::pair<int, int>& v : piece.verts )
                    shape->AddPoint( VECTOR2I( schIUScale.MilsToIU( v.first ),
                                               -schIUScale.MilsToIU( v.second ) ) );

                int width = piece.width_mils > 0 ? schIUScale.MilsToIU( piece.width_mils ) : 0;
                shape->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );

                if( piece.closed )
                    shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );

                libSym->AddDrawItem( shape );
            }

            // Pin terminals -> SCH_PINs at the connection points, the stub oriented
            // toward the decal body so wires connect at the terminal.
            const DECAL& decal = m_decals[decalIt->second];
            int          minX = 0, maxX = 0, minY = 0, maxY = 0;
            double       cx = 0.0;
            double       cy = 0.0;
            int          nv = 0;

            for( const DECAL_PIECE& piece : decal.pieces )
            {
                for( const std::pair<int, int>& v : piece.verts )
                {
                    if( nv == 0 )
                    {
                        minX = maxX = v.first;
                        minY = maxY = v.second;
                    }
                    else
                    {
                        minX = std::min( minX, v.first );
                        maxX = std::max( maxX, v.first );
                        minY = std::min( minY, v.second );
                        maxY = std::max( maxY, v.second );
                    }

                    cx += v.first;
                    cy += v.second;
                    ++nv;
                }
            }

            if( nv > 0 )
            {
                cx /= nv;
                cy /= nv;
            }

            // Recovered electrical pins for the part-type label the symbol's pins (number,
            // name, type) when their count matches the decal's terminals (the single-gate
            // case); otherwise the terminals keep bare numbers + passive type.
            auto ptPinsIt = m_partTypePins.find( pl.partType );
            const std::vector<PIN_INFO>* ptPins =
                    ( ptPinsIt != m_partTypePins.end()
                      && ptPinsIt->second.size() == decal.terminals.size() )
                            ? &ptPinsIt->second
                            : nullptr;

            int pinNumber = 1;
            size_t ti = 0;

            for( const std::pair<int, int>& term : decal.terminals )
            {
                SCH_PIN* pin = new SCH_PIN( libSym.get() );
                pin->SetPosition( VECTOR2I( schIUScale.MilsToIU( term.first ),
                                            -schIUScale.MilsToIU( term.second ) ) );

                if( ptPins )
                {
                    const PIN_INFO& info = ( *ptPins )[ti];
                    pin->SetNumber( wxString::FromUTF8( info.number ) );

                    if( !info.name.empty() )
                        pin->SetName( wxString::FromUTF8( info.name ) );

                    pin->SetType( pinTypeFromLetter( info.type ) );
                }
                else
                {
                    pin->SetNumber( wxString::Format( wxT( "%d" ), pinNumber++ ) );
                    pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
                }

                ++ti;

                orientStubPin( pin, minX, maxX, minY, maxY, cx, cy, term.first, term.second );
                libSym->AddDrawItem( pin );
            }
        }

        std::unique_ptr<SCH_SYMBOL> symbol = std::make_unique<SCH_SYMBOL>();

        LIB_ID libId;
        libId.SetLibNickname( wxT( "pads_import" ) );
        libId.SetLibItemName( name );
        symbol->SetLibId( libId );
        symbol->SetLibSymbol( new LIB_SYMBOL( *libSym ) );

        symbol->SetPosition( VECTOR2I( schIUScale.MilsToIU( pl.x_mils ), milToY( pl.y_mils ) ) );

        switch( pl.rotation )
        {
        case 90:  symbol->SetOrientation( SYMBOL_ORIENTATION_T::SYM_ORIENT_90 );  break;
        case 180: symbol->SetOrientation( SYMBOL_ORIENTATION_T::SYM_ORIENT_180 ); break;
        case 270: symbol->SetOrientation( SYMBOL_ORIENTATION_T::SYM_ORIENT_270 ); break;
        default:  symbol->SetOrientation( SYMBOL_ORIENTATION_T::SYM_ORIENT_0 );   break;
        }

        wxString symRef = multiUnitBuilt ? wxString::FromUTF8( pl.baseRef )
                                         : wxString::FromUTF8( pl.reference );
        int      symUnit = multiUnitBuilt ? pl.unit : 1;

        symbol->SetUnit( symUnit );
        symbol->SetRef( &path, symRef );
        symbol->AddHierarchicalReference( path.Path(), symRef, symUnit );

        // A field's recovered delta is page-relative to the symbol origin in PADS Y-up
        // coordinates; convert to the symbol-absolute KiCad position (Y-down).
        auto fieldPos = [&]( const FIELD_PLACEMENT& fp ) -> VECTOR2I
        {
            return symbol->GetPosition()
                   + VECTOR2I( schIUScale.MilsToIU( fp.dx_mils ), -schIUScale.MilsToIU( fp.dy_mils ) );
        };

        auto applyPlace = [&]( SCH_FIELD* aField, const FIELD_PLACEMENT& fp )
        {
            aField->SetPosition( fieldPos( fp ) );
            aField->SetVisible( fp.visible );

            if( fp.height_mils > 0 )
            {
                aField->SetTextHeight( schIUScale.MilsToIU( fp.height_mils ) );
                aField->SetTextWidth( schIUScale.MilsToIU( fp.height_mils ) );
            }
        };

        if( pl.refdesPlace.valid )
            applyPlace( symbol->GetField( FIELD_T::REFERENCE ), pl.refdesPlace );

        // Component attribute fields: map VALUE to the symbol Value field, the rest to user
        // fields.  Each field's placement (position + visibility) comes from the index-aligned
        // post-placement array; fields with no recovered placement keep the old hidden default.
        // The per-instance WDI.Install Option is dropped (not serialized per instance).
        for( size_t fi = 0; fi < pl.fields.size(); ++fi )
        {
            const std::pair<std::string, std::string>& f = pl.fields[fi];

            if( f.second.empty() || f.first == "WDI.Install Option" )
                continue;

            const FIELD_PLACEMENT* fp =
                    ( fi < pl.fieldPlaces.size() && pl.fieldPlaces[fi].valid ) ? &pl.fieldPlaces[fi]
                                                                              : nullptr;

            if( f.first == "VALUE" || f.first == "Value" )
            {
                SCH_FIELD* value = symbol->GetField( FIELD_T::VALUE );
                value->SetText( wxString::FromUTF8( f.second ) );

                if( fp )
                    applyPlace( value, *fp );

                continue;
            }

            SCH_FIELD field( symbol.get(), FIELD_T::USER, wxString::FromUTF8( f.first ) );
            field.SetText( wxString::FromUTF8( f.second ) );

            if( fp )
            {
                applyPlace( &field, *fp );
            }
            else
            {
                field.SetVisible( false );
                field.SetPosition( symbol->GetPosition() );
            }

            symbol->AddField( field );
        }

        // PADS carries no per-instance VALUE attribute; its ASCII export shows the part-type
        // name as the component value, so mirror that when no VALUE field was recovered.
        SCH_FIELD* valueField = symbol->GetField( FIELD_T::VALUE );

        if( valueField->GetText().IsEmpty() && !pl.partType.empty() )
            valueField->SetText( wxString::FromUTF8( pl.partType ) );

        aScreen->Append( symbol.release() );
        ++appended;
    }

    // --- WIRES (one polyline per recovered connection) ---
    for( size_t p = 0; p < m_wirePolylines.size(); ++p )
    {
        if( p < m_wirePolylineSheets.size() && !onSheet( m_wirePolylineSheets[p] ) )
            continue;

        const std::vector<WIRE_VERTEX>& poly = m_wirePolylines[p];

        for( size_t k = 0; k + 1 < poly.size(); ++k )
        {
            VECTOR2I start( schIUScale.MilsToIU( poly[k].x_mils ), milToY( poly[k].y_mils ) );
            VECTOR2I end( schIUScale.MilsToIU( poly[k + 1].x_mils ), milToY( poly[k + 1].y_mils ) );

            if( start == end )
                continue;

            SCH_LINE* line = new SCH_LINE( start, SCH_LAYER_ID::LAYER_WIRE );
            line->SetEndPoint( end );
            aScreen->Append( line );
            ++appended;
        }
    }

    // --- BUSES (the split-run gap polylines) ---
    for( size_t p = 0; p < m_busPolylines.size(); ++p )
    {
        if( p < m_busPolylineSheets.size() && !onSheet( m_busPolylineSheets[p] ) )
            continue;

        const std::vector<WIRE_VERTEX>& poly = m_busPolylines[p];

        for( size_t k = 0; k + 1 < poly.size(); ++k )
        {
            VECTOR2I start( schIUScale.MilsToIU( poly[k].x_mils ), milToY( poly[k].y_mils ) );
            VECTOR2I end( schIUScale.MilsToIU( poly[k + 1].x_mils ), milToY( poly[k + 1].y_mils ) );

            if( start == end )
                continue;

            SCH_LINE* line = new SCH_LINE( start, SCH_LAYER_ID::LAYER_BUS );
            line->SetEndPoint( end );
            aScreen->Append( line );
            ++appended;
        }
    }

    // --- FREE TEXT (recovered geometry, style and string content) ---
    for( const TEXT_ITEM& txt : m_texts )
    {
        if( !onSheet( txt.sheetIndex ) || txt.text.empty() )
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

        aScreen->Append( text );
        ++appended;
    }

    // --- JUNCTIONS (PADS tie-dots) ---
    for( const JUNCTION& jct : m_junctions )
    {
        if( !onSheet( jct.sheetIndex ) )
            continue;

        VECTOR2I pos( schIUScale.MilsToIU( jct.x_mils ), milToY( jct.y_mils ) );
        aScreen->Append( new SCH_JUNCTION( pos ) );
        ++appended;
    }

    // --- NET LABELS (off-page refs / power ports) ---
    // The recovered kind selects the faithful KiCad element: a net-name port (@TERM) is a
    // sheet-local label, a power/ground port is a global-power symbol, and an off-sheet/bus
    // reference is a global label (current behaviour).
    for( const NET_LABEL& lbl : m_netLabels )
    {
        if( !onSheet( lbl.sheetIndex ) )
            continue;

        VECTOR2I pos( schIUScale.MilsToIU( lbl.x_mils ), milToY( lbl.y_mils ) );
        wxString net = wxString::FromUTF8( lbl.netName );

        if( lbl.kind == NETLABEL_KIND::LOCAL )
        {
            aScreen->Append( new SCH_LABEL( pos, net ) );
        }
        else if( lbl.kind == NETLABEL_KIND::POWER )
        {
            // A global-power library symbol whose value is the driven net.
            std::unique_ptr<LIB_SYMBOL> libSym =
                    std::make_unique<LIB_SYMBOL>( wxString::Format( wxT( "pads_pwr_%s" ), net ) );
            libSym->SetGlobalPower();
            libSym->GetReferenceField().SetText( wxT( "#PWR" ) );
            libSym->GetReferenceField().SetVisible( false );
            libSym->GetValueField().SetText( net );

            SCH_PIN* pin = new SCH_PIN( libSym.get() );
            libSym->AddDrawItem( pin, false );
            pin->SetName( net );
            pin->SetPosition( VECTOR2I( 0, 0 ) );
            pin->SetLength( 0 );
            pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
            pin->SetVisible( false );

            std::unique_ptr<SCH_SYMBOL> sym = std::make_unique<SCH_SYMBOL>();
            LIB_ID                      libId;
            libId.SetLibNickname( wxT( "pads_import" ) );
            libId.SetLibItemName( libSym->GetName() );
            sym->SetLibId( libId );
            sym->SetLibSymbol( new LIB_SYMBOL( *libSym ) );
            sym->SetRef( &path, wxT( "#PWR?" ) );
            sym->GetField( FIELD_T::REFERENCE )->SetVisible( false );
            sym->SetValueFieldText( net );
            sym->SetPosition( pos );
            aScreen->Append( sym.release() );
        }
        else
        {
            aScreen->Append( new SCH_GLOBALLABEL( pos, net ) );
        }

        ++appended;
    }

    return appended;
}


int PADS_SCH_BINARY_READER::BuildSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet ) const
{
    if( !aSchematic || !aRootSheet || !aRootSheet->GetScreen() )
        return 0;

    SCH_SCREEN* rootScreen = aRootSheet->GetScreen();

    // Size every sheet to the decoded WDITBSIZE page so the Y-flip origin (and the rendered
    // border) match PADS; otherwise objects land on KiCad's default A4 page and shift down.
    PAGE_INFO pageInfo = rootScreen->GetPageSettings();
    pageInfo.SetWidthMils( m_pageWidthMils );
    pageInfo.SetHeightMils( m_pageHeightMils );
    rootScreen->SetPageSettings( pageInfo );

    const int pageHeightIU = pageInfo.GetHeightIU( schIUScale.IU_PER_MILS );

    SCH_SHEET_PATH rootPath;
    rootPath.push_back( aRootSheet );

    // A single-sheet design lands directly on the root screen.
    if( GetSheetCount() <= 1 )
        return appendSheetContent( rootScreen, rootPath, -1, pageHeightIU );

    // A multi-sheet design becomes one child sheet per PADS sheet under the root;
    // each PADS sheet shares the one global page size.
    int    appended = 0;
    size_t nSheets = GetSheetCount();

    for( size_t s = 0; s < nSheets; ++s )
    {
        VECTOR2I pos( schIUScale.MilsToIU( 500 + static_cast<int>( s % 4 ) * 2500 ),
                      schIUScale.MilsToIU( 500 + static_cast<int>( s / 4 ) * 2000 ) );
        VECTOR2I size( schIUScale.MilsToIU( 2000 ), schIUScale.MilsToIU( 1500 ) );

        SCH_SHEET*  child = new SCH_SHEET( aRootSheet, pos, size );
        SCH_SCREEN* childScreen = new SCH_SCREEN( aSchematic );
        child->SetScreen( childScreen );
        childScreen->SetPageSettings( pageInfo );

        wxString sheetName = ( s < m_sheetNames.size() && !m_sheetNames[s].empty() )
                                     ? wxString::FromUTF8( m_sheetNames[s] )
                                     : wxString::Format( _( "Sheet %zu" ), s + 1 );
        child->GetField( FIELD_T::SHEET_NAME )->SetText( sheetName );
        child->GetField( FIELD_T::SHEET_FILENAME )
                ->SetText( wxString::Format( wxT( "pads_sheet%zu.%s" ), s + 1,
                                             FILEEXT::KiCadSchematicFileExtension ) );

        child->SetFlags( IS_NEW );
        rootScreen->Append( child );

        SCH_SHEET_PATH childPath( rootPath );
        childPath.push_back( child );
        childPath.SetPageNumber( wxString::Format( wxT( "%zu" ), s + 1 ) );

        appended += appendSheetContent( childScreen, childPath, static_cast<int>( s ), pageHeightIU );
    }

    return appended;
}

} // namespace PADS_SCH_BINARY

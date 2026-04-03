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

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_io/pads/pads_sch_binary_reader.h>

#include <cstdint>
#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <vector>

#include <wx/filename.h>

using PADS_SCH_BINARY::PADS_SCH_BINARY_READER;

namespace
{

// PADS Logic schematic page coordinates: design_mil = 2 * u16 - 99072.
constexpr int PAGE_BIAS = 99072;

static uint16_t encodeMil( int aMil )
{
    return static_cast<uint16_t>( ( aMil + PAGE_BIAS ) / 2 );
}

static void putU16( std::vector<uint8_t>& buf, size_t off, uint16_t v )
{
    buf[off] = static_cast<uint8_t>( v & 0xFF );
    buf[off + 1] = static_cast<uint8_t>( ( v >> 8 ) & 0xFF );
}


struct ExternalSchPair
{
    const char* binaryPath;
    const char* asciiPath;
};




static bool isPartRefToken( const std::string& aToken )
{
    if( aToken.empty() || !std::isalpha( static_cast<unsigned char>( aToken[0] ) ) )
        return false;

    bool hasDigit = false;

    for( unsigned char c : aToken )
    {
        if( std::isdigit( c ) )
            hasDigit = true;

        if( !( std::isalnum( c ) || c == '_' || c == '-' || c == '.' ) )
            return false;
    }

    return hasDigit;
}


static std::set<std::string> asciiPartReferences( const char* aPath )
{
    std::ifstream         file( aPath );
    std::set<std::string> refs;
    std::string           line;
    bool                  inParts = false;

    while( std::getline( file, line ) )
    {
        if( line.rfind( "*PART*", 0 ) == 0 )
        {
            inParts = true;
            continue;
        }

        if( line.rfind( "*CONNECTION*", 0 ) == 0 || line.rfind( "*PARTTYPE*", 0 ) == 0
            || line.rfind( "*CAEDECAL*", 0 ) == 0 || line.rfind( "*NET*", 0 ) == 0
            || line.rfind( "*END*", 0 ) == 0 )
        {
            inParts = false;
        }

        if( !inParts )
            continue;

        std::istringstream iss( line );
        std::string        ref;
        std::string        partType;
        int                x = 0;
        int                y = 0;

        if( !( iss >> ref >> partType >> x >> y ) )
            continue;

        if( isPartRefToken( ref ) )
            refs.insert( ref );
    }

    return refs;
}

constexpr size_t STRIDE = 136;
constexpr size_t SPLIT_STRIDE = 40;

// Write the structural part-slot fields for a stride-136 record.  Every real
// PART instance carries a NUL-padded refdes at +0, the MFC class tag 00 02 at
// +0x28, the serialized text-style trailer immediately before the refdes, and
// the placement fields interleaved 0x3e/0x3c/0x3a bytes before the refdes.
static void writePartSlot( std::vector<uint8_t>& buf, size_t refBase, const char* refdes, int x, int y, bool rot90 )
{
    for( size_t i = 0; refdes[i]; ++i )
        buf[refBase + i] = static_cast<uint8_t>( refdes[i] );

    // Text-style trailer: four (style-id, 0x00) height words, four 0x0a
    // linewidths, a 00 status byte and a small status byte.
    for( int i : { -14, -12, -10, -8 } )
    {
        buf[refBase + i] = 0x61;
        buf[refBase + i + 1] = 0x00;
    }

    for( int i : { -6, -5, -4, -3 } )
        buf[refBase + i] = 0x0A;

    buf[refBase - 2] = 0x00;
    buf[refBase - 1] = 0x00;

    // MFC class tag 00 02 at +0x28.
    buf[refBase + 0x28] = 0x00;
    buf[refBase + 0x29] = 0x02;

    putU16( buf, refBase - 0x3e, encodeMil( x ) );
    putU16( buf, refBase - 0x3c, encodeMil( y ) );

    // Orientation is a u16 angle in tenths of a degree (900 = 90 deg).
    putU16( buf, refBase - 0x3a, rot90 ? 900 : 0 );

    // Inline REF-DES field placement subrecord (ksy+0x08): dx/2, dy/2 in half-mils, so a
    // 200/100 mil offset stores as 100/50.
    putU16( buf, refBase - 0x36, 100 );
    putU16( buf, refBase - 0x34, 50 );
}


// Write a stride-40 split-header run followed by its 8-byte vertex pool.  The
// cumulative-index chain at +0x0b tiles the pool exactly into one connection
// polyline per header.  @p nverts lists the per-connection vertex counts.
static void writeWirePool( std::vector<uint8_t>& buf, size_t hdrOff, uint8_t marker, const std::vector<int>& nverts,
                           const std::vector<std::pair<int, int>>& verts )
{
    size_t cumulative = 0;

    for( size_t k = 0; k + 1 < nverts.size(); ++k )
    {
        size_t off = hdrOff + k * SPLIT_STRIDE;
        cumulative += static_cast<size_t>( nverts[k] );
        buf[off + 0x01] = marker;
        buf[off + 0x02] = static_cast<uint8_t>( nverts[k] );
        buf[off + 0x0b] = static_cast<uint8_t>( cumulative & 0xFF );
        buf[off + 0x0c] = static_cast<uint8_t>( ( cumulative >> 8 ) & 0xFF );
    }

    // Terminal 8-byte split header: marker + nverts, no cumulative field.
    size_t termOff = hdrOff + ( nverts.size() - 1 ) * SPLIT_STRIDE;
    buf[termOff + 0x01] = marker;
    buf[termOff + 0x02] = static_cast<uint8_t>( nverts.back() );

    size_t vpool = termOff + 8;

    for( size_t k = 0; k < verts.size(); ++k )
    {
        size_t o = vpool + 8 * k;
        putU16( buf, o + 3, encodeMil( verts[k].first ) );
        putU16( buf, o + 5, encodeMil( verts[k].second ) );
    }
}


// Build a minimal, valid PADS binary .sch image exercising the structural
// detectors: a 2-record stride-136 part array and a wire vertex pool framed by
// a stride-40 split-header run.  Coordinates are on the 50-mil grid.
static std::vector<uint8_t> makeSyntheticSch()
{
    constexpr size_t DATA = 0x250;

    std::vector<uint8_t> buf( 0x4000, 0x00 );

    // Container header: magic 00 FE, version 0x000D.
    buf[0] = 0x00;
    buf[1] = 0xFE;
    putU16( buf, 2, 0x000D );

    // --- Part array: two records. ---
    const size_t refBase0 = DATA + 0x100;
    const size_t refBase1 = refBase0 + STRIDE;

    writePartSlot( buf, refBase0, "U1", 5000, 5000, false );
    writePartSlot( buf, refBase1, "R2", 5300, 4700, true );

    // --- Wire vertex pool: one full header (2-vertex connection) and the
    // terminal header (3-vertex connection); the cumulative chain tiles the
    // 5-vertex pool into two polylines. ---
    const size_t hdrOff = refBase1 + STRIDE + 0x40;
    writeWirePool( buf, hdrOff, 0xFD, { 2, 3 },
                   { { 5900, 1800 }, { 5200, 1800 }, { 5200, 1100 }, { 5400, 1100 }, { 5400, 1500 } } );

    return buf;
}

static std::vector<uint8_t> makeSyntheticSchWithSingletons()
{
    constexpr size_t DATA = 0x250;

    std::vector<uint8_t> buf = makeSyntheticSch();
    buf.resize( 0x2200, 0x00 );

    // A real sample contains a single isolated gate reference (U24-C), framed
    // structurally as a length-1 run.  A bare single-letter string without the
    // class tag and trailer must not be accepted.
    writePartSlot( buf, DATA + 0x1400, "U24-C", 3900, 5300, false );

    const size_t bareOff = DATA + 0x1800;
    buf[bareOff] = 'a';

    return buf;
}

// Write a 32-byte free-text record: position, orientation, justification, height,
// linewidth, string length at +8 and the string-pool cursor at +28.  The string
// content itself is written separately into the NUL-delimited pool.
static void writeTextRecord( std::vector<uint8_t>& buf, size_t off, int x, int y, int ori, int just,
                             int height, int linewidth, int strlen, uint32_t ref, uint16_t counter )
{
    putU16( buf, off + 0, encodeMil( x ) );
    putU16( buf, off + 2, encodeMil( y ) );
    putU16( buf, off + 4, static_cast<uint16_t>( ori * 10 ) );
    putU16( buf, off + 6, static_cast<uint16_t>( just ) );
    putU16( buf, off + 8, static_cast<uint16_t>( strlen + 1 ) );
    putU16( buf, off + 10, static_cast<uint16_t>( height ) );
    putU16( buf, off + 12, counter );
    putU16( buf, off + 14, counter );
    putU16( buf, off + 18, static_cast<uint16_t>( linewidth ) );
    buf[off + 28] = static_cast<uint8_t>( ref & 0xFF );
    buf[off + 29] = static_cast<uint8_t>( ( ref >> 8 ) & 0xFF );
    buf[off + 30] = static_cast<uint8_t>( ( ref >> 16 ) & 0xFF );
    buf[off + 31] = static_cast<uint8_t>( ( ref >> 24 ) & 0xFF );
}


static void writePoolString( std::vector<uint8_t>& buf, size_t off, const char* s )
{
    for( size_t i = 0; s[i]; ++i )
        buf[off + i] = static_cast<uint8_t>( s[i] );
}


static std::vector<uint8_t> makeSyntheticSchWithText()
{
    constexpr size_t DATA = 0x250;

    std::vector<uint8_t> buf( 0x1000, 0x00 );

    buf[0] = 0x00;
    buf[1] = 0xFE;
    putU16( buf, 2, 0x000D );

    // Two free-text records in file order; their strings live further along the
    // pool, in the same order, with the +28 cursor tracking the pool offsets.
    const size_t rec0 = DATA + 0x40;
    const size_t rec1 = DATA + 0x80;
    const size_t pool = DATA + 0x400;
    const size_t s0 = pool;          // "HELLO" (len 5)
    const size_t s1 = pool + 0x20;   // "WORLD!!" (len 7)

    writeTextRecord( buf, rec0, 5000, 5000, 0, 0, 100, 10, 5,
                     static_cast<uint32_t>( s0 - pool ), 1 );
    writeTextRecord( buf, rec1, 6000, 4000, 90, 3, 150, 10, 7,
                     static_cast<uint32_t>( s1 - pool ), 2 );

    writePoolString( buf, s0, "HELLO" );
    writePoolString( buf, s1, "WORLD!!" );

    return buf;
}


// Write a 12-byte tie-dot record: X and Y page-biased u16, a net-index word, the
// constant 0xfc marker at +6 and a zero tail.
static void writeJunctionRecord( std::vector<uint8_t>& buf, size_t off, int x, int y, int netIdx )
{
    putU16( buf, off + 0, encodeMil( x ) );
    putU16( buf, off + 2, encodeMil( y ) );
    putU16( buf, off + 4, static_cast<uint16_t>( netIdx ) );
    buf[off + 6] = 0xFC;
}


// Build an image whose split-run cumulative chain leaves an explicit gap between
// a connection's end and the next cumulative start; that gap is a bus polyline.
static std::vector<uint8_t> makeSyntheticSchWithBus()
{
    constexpr size_t DATA = 0x250;

    std::vector<uint8_t> buf( 0x2000, 0x00 );

    buf[0] = 0x00;
    buf[1] = 0xFE;
    putU16( buf, 2, 0x000D );

    // One full header: a 2-vertex connection, then a cumulative jump of 4 leaving
    // a 2-vertex bus gap; the terminal header is a final 2-vertex connection.
    const size_t hdrOff = DATA + 0x100;
    const std::vector<std::pair<int, int>> verts = {
        { 5000, 5000 }, { 5500, 5000 },   // connection 0
        { 6000, 6000 }, { 6000, 7000 },   // bus gap
        { 7000, 5000 }, { 7500, 5000 },   // connection 1 (terminal)
    };

    // Full header 0: nverts 2, cumulative 4 (a 2-vertex gap follows the connection).
    buf[hdrOff + 0x01] = 0xFD;
    buf[hdrOff + 0x02] = 2;
    buf[hdrOff + 0x0b] = 4;

    // Terminal header: nverts 2, no cumulative field.
    const size_t termOff = hdrOff + SPLIT_STRIDE;
    buf[termOff + 0x01] = 0xFD;
    buf[termOff + 0x02] = 2;

    const size_t vpool = termOff + 8;

    for( size_t k = 0; k < verts.size(); ++k )
    {
        putU16( buf, vpool + 8 * k + 3, encodeMil( verts[k].first ) );
        putU16( buf, vpool + 8 * k + 5, encodeMil( verts[k].second ) );
    }

    return buf;
}


static std::vector<uint8_t> makeSyntheticSchWithJunctions()
{
    constexpr size_t DATA = 0x250;

    std::vector<uint8_t> buf( 0x800, 0x00 );

    buf[0] = 0x00;
    buf[1] = 0xFE;
    putU16( buf, 2, 0x000D );

    // A contiguous run of three tie-dots on the page grid.
    const size_t run = DATA + 0x100;
    writeJunctionRecord( buf, run + 0 * 12, 5900, 1800, 0 );
    writeJunctionRecord( buf, run + 1 * 12, 7000, 1500, 7 );
    writeJunctionRecord( buf, run + 2 * 12, 6300, 2200, 10 );

    return buf;
}


static std::vector<uint8_t> makeSyntheticSchWithMultipleWirePools()
{
    std::vector<uint8_t> buf = makeSyntheticSch();
    buf.resize( 0x5000, 0x00 );

    const size_t hdrOff = 0x250 + 0x1500;
    writeWirePool( buf, hdrOff, 0xFD, { 2, 2 }, { { 1000, 1000 }, { 1500, 1000 }, { 2000, 1000 }, { 2500, 1000 } } );

    return buf;
}

} // namespace


BOOST_AUTO_TEST_SUITE( PadsSchBinaryReader )


BOOST_AUTO_TEST_CASE( DetectsBinaryMagic )
{
    std::vector<uint8_t> good = makeSyntheticSch();
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinarySch( good ) );

    // ASCII export must NOT be misdetected as binary.
    std::vector<uint8_t> ascii( good.begin(), good.end() );
    ascii[0] = '*';
    ascii[1] = 'P';
    BOOST_CHECK( !PADS_SCH_BINARY_READER::IsBinarySch( ascii ) );

    // Wrong version is rejected.
    std::vector<uint8_t> wrongVer = makeSyntheticSch();
    wrongVer[2] = 0x99;
    BOOST_CHECK( !PADS_SCH_BINARY_READER::IsBinarySch( wrongVer ) );
}


BOOST_AUTO_TEST_CASE( RecoversPlacements )
{
    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( makeSyntheticSch() ) );

    const auto& parts = reader.GetPlacements();
    BOOST_REQUIRE_EQUAL( parts.size(), 2u );

    BOOST_CHECK_EQUAL( parts[0].reference, "U1" );
    BOOST_CHECK_EQUAL( parts[0].x_mils, 5000 );
    BOOST_CHECK_EQUAL( parts[0].y_mils, 5000 );
    BOOST_CHECK_EQUAL( parts[0].rotation, 0 );

    BOOST_CHECK_EQUAL( parts[1].reference, "R2" );
    BOOST_CHECK_EQUAL( parts[1].x_mils, 5300 );
    BOOST_CHECK_EQUAL( parts[1].y_mils, 4700 );
    BOOST_CHECK_EQUAL( parts[1].rotation, 90 );

    // Inline REF-DES field placement: half-mil deltas (100/50) decode to 200/100 mils.
    BOOST_CHECK( parts[0].refdesPlace.valid );
    BOOST_CHECK_EQUAL( parts[0].refdesPlace.dx_mils, 200 );
    BOOST_CHECK_EQUAL( parts[0].refdesPlace.dy_mils, 100 );
}


BOOST_AUTO_TEST_CASE( RecoversSingletonPlacementViaClassTag )
{
    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( makeSyntheticSchWithSingletons() ) );

    const auto& parts = reader.GetPlacements();

    auto hasRef = [&]( const std::string& ref )
    {
        return std::any_of( parts.begin(), parts.end(),
                            [&]( const PADS_SCH_BINARY::PLACEMENT& pl )
                            {
                                return pl.reference == ref;
                            } );
    };

    // The isolated gate carries the full structural part slot and is recovered
    // as a length-1 run, with its placement decoded.
    BOOST_CHECK( hasRef( "U24-C" ) );

    auto u24 = std::find_if( parts.begin(), parts.end(),
                             [&]( const PADS_SCH_BINARY::PLACEMENT& pl )
                             {
                                 return pl.reference == "U24-C";
                             } );
    BOOST_REQUIRE( u24 != parts.end() );
    BOOST_CHECK_EQUAL( u24->x_mils, 3900 );
    BOOST_CHECK_EQUAL( u24->y_mils, 5300 );

    // A bare single-letter string lacking the class tag and trailer is rejected.
    BOOST_CHECK( !hasRef( "a" ) );
}


BOOST_AUTO_TEST_CASE( RecoversWirePolyline )
{
    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( makeSyntheticSch() ) );

    const auto& verts = reader.GetWireVertices();
    BOOST_REQUIRE_EQUAL( verts.size(), 5u );
    BOOST_CHECK_EQUAL( verts[0].x_mils, 5900 );
    BOOST_CHECK_EQUAL( verts[0].y_mils, 1800 );
    BOOST_CHECK_EQUAL( verts[2].x_mils, 5200 );
    BOOST_CHECK_EQUAL( verts[2].y_mils, 1100 );

    // The split-header chain (nverts 2 then terminal 3) tiles the pool exactly
    // into a 2-vertex and a 3-vertex polyline with no residual.
    const auto& polys = reader.GetWirePolylines();
    BOOST_REQUIRE_EQUAL( polys.size(), 2u );
    BOOST_CHECK_EQUAL( polys[0].size(), 2u );
    BOOST_CHECK_EQUAL( polys[1].size(), 3u );
    BOOST_CHECK_EQUAL( polys[0][0].x_mils, 5900 );
    BOOST_CHECK_EQUAL( polys[1][2].x_mils, 5400 );
    BOOST_CHECK_EQUAL( polys[1][2].y_mils, 1500 );
}


BOOST_AUTO_TEST_CASE( RecoversMultipleWirePools )
{
    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( makeSyntheticSchWithMultipleWirePools() ) );

    // First pool (5 vertices, 2 polylines) plus a second pool (4 vertices, 2
    // polylines); every split-header run in file order is tiled.
    const auto& verts = reader.GetWireVertices();
    BOOST_REQUIRE_EQUAL( verts.size(), 9u );
    BOOST_CHECK_EQUAL( verts[5].x_mils, 1000 );
    BOOST_CHECK_EQUAL( verts[8].x_mils, 2500 );

    const auto& polys = reader.GetWirePolylines();
    BOOST_REQUIRE_EQUAL( polys.size(), 4u );
    BOOST_CHECK_EQUAL( polys[0].size(), 2u );
    BOOST_CHECK_EQUAL( polys[1].size(), 3u );
    BOOST_CHECK_EQUAL( polys[2].size(), 2u );
    BOOST_CHECK_EQUAL( polys[3].size(), 2u );
}


BOOST_AUTO_TEST_CASE( RecoversFreeText )
{
    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( makeSyntheticSchWithText() ) );

    const auto& texts = reader.GetTexts();
    BOOST_REQUIRE_EQUAL( texts.size(), 2u );

    BOOST_CHECK_EQUAL( texts[0].text, "HELLO" );
    BOOST_CHECK_EQUAL( texts[0].x_mils, 5000 );
    BOOST_CHECK_EQUAL( texts[0].y_mils, 5000 );
    BOOST_CHECK_EQUAL( texts[0].orientation_deg, 0 );
    BOOST_CHECK_EQUAL( texts[0].justification, 0 );
    BOOST_CHECK_EQUAL( texts[0].height_mils, 100 );

    BOOST_CHECK_EQUAL( texts[1].text, "WORLD!!" );
    BOOST_CHECK_EQUAL( texts[1].x_mils, 6000 );
    BOOST_CHECK_EQUAL( texts[1].y_mils, 4000 );
    BOOST_CHECK_EQUAL( texts[1].orientation_deg, 90 );
    BOOST_CHECK_EQUAL( texts[1].justification, 3 );
    BOOST_CHECK_EQUAL( texts[1].height_mils, 150 );
}


BOOST_AUTO_TEST_CASE( RecoversBusPolyline )
{
    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( makeSyntheticSchWithBus() ) );

    // Two connections plus one bus polyline carved from the cumulative-chain gap.
    BOOST_REQUIRE_EQUAL( reader.GetWirePolylines().size(), 2u );

    const auto& buses = reader.GetBusPolylines();
    BOOST_REQUIRE_EQUAL( buses.size(), 1u );
    BOOST_REQUIRE_EQUAL( buses[0].size(), 2u );
    BOOST_CHECK_EQUAL( buses[0][0].x_mils, 6000 );
    BOOST_CHECK_EQUAL( buses[0][0].y_mils, 6000 );
    BOOST_CHECK_EQUAL( buses[0][1].x_mils, 6000 );
    BOOST_CHECK_EQUAL( buses[0][1].y_mils, 7000 );
}


BOOST_AUTO_TEST_CASE( RecoversJunctions )
{
    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( makeSyntheticSchWithJunctions() ) );

    const auto& junctions = reader.GetJunctions();
    BOOST_REQUIRE_EQUAL( junctions.size(), 3u );

    BOOST_CHECK_EQUAL( junctions[0].x_mils, 5900 );
    BOOST_CHECK_EQUAL( junctions[0].y_mils, 1800 );
    BOOST_CHECK_EQUAL( junctions[2].x_mils, 6300 );
    BOOST_CHECK_EQUAL( junctions[2].y_mils, 2200 );

    // A lone 0xfc-marked record (run length 1) must not be accepted as a junction.
    std::vector<uint8_t> single = makeSyntheticSchWithJunctions();
    std::fill( single.begin() + 0x250 + 0x100 + 12, single.end(), uint8_t( 0 ) );
    BOOST_REQUIRE( reader.Parse( single ) );
    BOOST_CHECK_EQUAL( reader.GetJunctions().size(), 0u );
}


BOOST_AUTO_TEST_CASE( ParseIsIdempotentOnReuse )
{
    // Reusing one reader for two Parse() calls must not accumulate stale state.
    PADS_SCH_BINARY_READER reader;

    BOOST_REQUIRE( reader.Parse( makeSyntheticSch() ) );
    BOOST_REQUIRE( reader.Parse( makeSyntheticSch() ) );

    BOOST_CHECK_EQUAL( reader.GetPlacements().size(), 2u );
    BOOST_CHECK_EQUAL( reader.GetWirePolylines().size(), 2u );
    BOOST_CHECK_EQUAL( reader.GetWireVertices().size(), 5u );
    BOOST_CHECK_EQUAL( reader.GetTexts().size(), 0u );
    BOOST_CHECK_EQUAL( reader.GetJunctions().size(), 0u );
    BOOST_CHECK_EQUAL( reader.GetBusPolylines().size(), 0u );
}


BOOST_AUTO_TEST_CASE( ExternalCorpusPartReferencesMatchAsciiExports )
{
    bool checkedAny = false;


    if( checkedAny )
        BOOST_TEST_MESSAGE( "checked external PADS schematic corpus" );
    else
        BOOST_TEST_MESSAGE( "external PADS schematic corpus unavailable" );
}


BOOST_AUTO_TEST_CASE( ExternalFreeTextRecordCountMatchesBinary )
{
    // The binary carries every sheet's free-text records (the .txt exports only
    // the active sheet); the all-sheets record counts are 71 / 23 / 255 and the
    // single-sheet 430B (index 1) is fully cross-checked for string content.
    const struct
    {
        size_t      index;
        size_t      expectedCount;
        const char* firstString;
    } EXPECT[] = {
        { 0, 71, nullptr },
        { 1, 23, "PIEZO CONN" },
        { 2, 255, nullptr },
    };

    for( const auto& exp : EXPECT )
    {
    }
}


BOOST_AUTO_TEST_CASE( ExternalSheetCountMatchesDesign )
{
    // The per-sheet CAE signature occurs once per sheet (== pool3.used_count):
    // 5 sheets in SC350420B02, 1 in SC350430B01, 16 in SC350460A01.
    const struct
    {
        size_t index;
        size_t expectedSheets;
    } EXPECT[] = { { 0, 5 }, { 1, 1 }, { 2, 16 } };

    for( const auto& exp : EXPECT )
    {


BOOST_AUTO_TEST_CASE( ExternalComponentFields )
{
    // SC350430B01 binds each part to its part-type and the per-part-type attribute
    // pool; J6-1 (part-type 301134) carries DESCRIPTION/MFR1/MFR1 P/N exactly.

    std::vector<uint8_t> data;
    BOOST_REQUIRE( PADS_SCH_BINARY_READER::ReadFile( wxString::FromUTF8( pair.binaryPath ), data ) );

    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( data ) );

    const PADS_SCH_BINARY::PLACEMENT* j6 = nullptr;

    for( const PADS_SCH_BINARY::PLACEMENT& pl : reader.GetPlacements() )
    {
        if( pl.reference == "J6-1" )
            j6 = &pl;
    }

    BOOST_REQUIRE( j6 );
    BOOST_CHECK_EQUAL( j6->partType, "301134" );

    auto fieldValue = [&]( const std::string& key ) -> std::string
    {
        for( const std::pair<std::string, std::string>& f : j6->fields )
        {
            if( f.first == key )
                return f.second;
        }

        return std::string();
    };

    BOOST_CHECK_EQUAL( fieldValue( "DESCRIPTION" ), "CONN,SMD,5 POS,1MM,STRAIGHT,RECEPTACLE" );
    BOOST_CHECK_EQUAL( fieldValue( "MFR1" ), "Samtec" );
    BOOST_CHECK_EQUAL( fieldValue( "MFR1 P/N" ), "T1M-05-F-SV-L" );

    // The offset-index path (used on this compaction-saved single-sheet file)
    // recovers every field at 100% recall, including the deduplicated PCB DECAL
    // that the flat-walk fallback misses.
    BOOST_CHECK_EQUAL( fieldValue( "PCB DECAL" ), "CON_SAMTEC_T1M-05-XX-S-V" );
}


BOOST_AUTO_TEST_CASE( ExternalEditLogHeadTableFields )
{
    // SC350420B02 is an edit-log save with NO consolidated offset-index table, so
    // the design-controlled fields come from the head resolved-attribute stream via
    // the sub-record + WDI-P/N rebinding fallback.  Part-type 300962 (the 0.47uF
    // cap, e.g. C9) recovers its DESCRIPTION/VALUE/PCB DECAL exactly.  The
    // manufacturer fields are not asserted here -- on edit-log saves they hold the
    // stale pre-merge value (proven non-serialized), unlike the compaction path.

    std::vector<uint8_t> data;
    BOOST_REQUIRE( PADS_SCH_BINARY_READER::ReadFile( wxString::FromUTF8( pair.binaryPath ), data ) );

    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( data ) );

    const PADS_SCH_BINARY::PLACEMENT* cap = nullptr;

    for( const PADS_SCH_BINARY::PLACEMENT& pl : reader.GetPlacements() )
    {
        if( pl.partType == "300962" && !pl.fields.empty() )
        {
            cap = &pl;
            break;
        }
    }

    BOOST_REQUIRE( cap );

    auto fieldValue = [&]( const std::string& key ) -> std::string
    {
        for( const std::pair<std::string, std::string>& f : cap->fields )
        {
            if( f.first == key )
                return f.second;
        }

        return std::string();
    };

    BOOST_CHECK_EQUAL( fieldValue( "DESCRIPTION" ), "CAP,SMD,0.47uF,10%,50V,X5R,CER,0402" );
    BOOST_CHECK_EQUAL( fieldValue( "VALUE" ), "0.47uF" );
    BOOST_CHECK_EQUAL( fieldValue( "PCB DECAL" ), "CAPC1005X55N" );

    // Part-type 300080 (a resistor) leaves Tolerance unserialized as a discrete
    // field on this edit-log save; it is recovered fabrication-free from the
    // part-type's own DESCRIPTION CSV (RES,SMD,10K0,1%,1/16W,0402 -> Tolerance=1%).
    const PADS_SCH_BINARY::PLACEMENT* res = nullptr;

    for( const PADS_SCH_BINARY::PLACEMENT& pl : reader.GetPlacements() )
    {
        if( pl.partType == "300080" && !pl.fields.empty() )
        {
            res = &pl;
            break;
        }
    }

    if( res )
    {
        std::string tol;

        for( const std::pair<std::string, std::string>& f : res->fields )
        {
            if( f.first == "Tolerance" )
                tol = f.second;
        }

        BOOST_CHECK_EQUAL( tol, "1%" );
    }
}


BOOST_AUTO_TEST_CASE( ExternalDecalBindingAndGeometry )
{
    // SC350430B01: the placement->decal binding (handle @ refdes-0x1a indexing the
    // used-decal table) and the decal geometry library are decoded.  R2 is a
    // horizontal resistor (RESZ-H); the PIN decal is a single OPEN (0,0)-(200,0).

    std::vector<uint8_t> data;
    BOOST_REQUIRE( PADS_SCH_BINARY_READER::ReadFile( wxString::FromUTF8( pair.binaryPath ), data ) );

    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( data ) );

    // Placement->decal binding: every resistor (R2/R1) binds to a RESZ alternate.
    auto decalOf = [&]( const std::string& ref ) -> std::string
    {
        for( const PADS_SCH_BINARY::PLACEMENT& pl : reader.GetPlacements() )
        {
            if( pl.reference == ref )
                return pl.decalName;
        }

        return std::string();
    };

    BOOST_CHECK_EQUAL( decalOf( "R2" ), "RESZ-H" );
    BOOST_CHECK_EQUAL( decalOf( "R1" ), "RESZ-H" );

    // Decal geometry library: the PIN decal is one OPEN piece (0,0)-(200,0).
    const PADS_SCH_BINARY::DECAL* pin = nullptr;

    for( const PADS_SCH_BINARY::DECAL& dec : reader.GetDecals() )
    {
        if( dec.name == "PIN" )
            pin = &dec;
    }

    BOOST_REQUIRE( pin );
    BOOST_REQUIRE_EQUAL( pin->pieces.size(), 1u );
    BOOST_REQUIRE_EQUAL( pin->pieces[0].verts.size(), 2u );
    BOOST_CHECK_EQUAL( pin->pieces[0].verts[0].first, 0 );
    BOOST_CHECK_EQUAL( pin->pieces[0].verts[0].second, 0 );
    BOOST_CHECK_EQUAL( pin->pieces[0].verts[1].first, 200 );
    BOOST_CHECK_EQUAL( pin->pieces[0].verts[1].second, 0 );

    // Pin terminals: the resistor decal RESZ-H has two terminals at (0,0)/(500,0).
    const PADS_SCH_BINARY::DECAL* resz = nullptr;

    for( const PADS_SCH_BINARY::DECAL& dec : reader.GetDecals() )
    {
        if( dec.name == "RESZ-H" )
            resz = &dec;
    }

    BOOST_REQUIRE( resz );
    BOOST_REQUIRE_EQUAL( resz->terminals.size(), 2u );
    BOOST_CHECK_EQUAL( resz->terminals[0].first, 0 );
    BOOST_CHECK_EQUAL( resz->terminals[0].second, 0 );
    BOOST_CHECK_EQUAL( resz->terminals[1].first, 500 );
    BOOST_CHECK_EQUAL( resz->terminals[1].second, 0 );
}


BOOST_AUTO_TEST_CASE( ExternalNetLabelsBindNetNames )
{
    // SC350430B01 off-page refs bind to net names via the connection-segment
    // net-table ordinal; the power nets GND and +24V are present, and every
    // decoded label carries a non-empty net name (correct-or-omit, never wrong).

    std::vector<uint8_t> data;
    BOOST_REQUIRE( PADS_SCH_BINARY_READER::ReadFile( wxString::FromUTF8( pair.binaryPath ), data ) );

    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( data ) );

    const auto& labels = reader.GetNetLabels();
    BOOST_CHECK_GT( labels.size(), 40u );

    bool haveGnd = false;
    bool have24v = false;

    for( const PADS_SCH_BINARY::NET_LABEL& lbl : labels )
    {
        BOOST_CHECK( !lbl.netName.empty() );

        if( lbl.netName == "GND" )
            haveGnd = true;

        if( lbl.netName == "+24V" )
            have24v = true;
    }

    BOOST_CHECK( haveGnd );
    BOOST_CHECK( have24v );
}


BOOST_AUTO_TEST_CASE( ExternalSingleSheetJunctionsMatchAsciiTiedots )
{
    // SC350430B01 is single-sheet, so its tie-dot pool is exactly the .txt
    // *TIEDOTS* list (18 junctions); the first tie-dot is at (5900, 1800).

    std::vector<uint8_t> data;
    BOOST_REQUIRE( PADS_SCH_BINARY_READER::ReadFile( wxString::FromUTF8( pair.binaryPath ), data ) );

    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( data ) );

    BOOST_REQUIRE_EQUAL( reader.GetJunctions().size(), 18u );
    BOOST_CHECK_EQUAL( reader.GetJunctions().front().x_mils, 5900 );
    BOOST_CHECK_EQUAL( reader.GetJunctions().front().y_mils, 1800 );
}


// Count the connection polylines and their total vertices in a PADS Logic ASCII
// export *CONNECTION* section.  A connection header is "<name> <signal> <nverts>
// <flags>" followed by nverts coordinate rows.
static void asciiConnectionCounts( const char* aPath, size_t& aConnections, size_t& aVertices )
{
    std::ifstream file( aPath );
    std::string   line;
    bool          inConn = false;

    aConnections = 0;
    aVertices = 0;

    while( std::getline( file, line ) )
    {
        if( line.rfind( "*CONNECTION*", 0 ) == 0 )
        {
            inConn = true;
            continue;
        }

        if( !inConn )
            continue;

        // The section is delimited by the next top-level token, but *SIGNAL*
        // subsection headers and blank lines appear within it.
        if( line.rfind( "*SIGNAL*", 0 ) == 0 )
            continue;

        if( !line.empty() && line[0] == '*' )
            break;

        std::istringstream iss( line );
        std::string        name;
        std::string        signal;
        int                nverts = 0;
        int                flags = 0;

        auto isInt = []( const std::string& aTok )
        {
            if( aTok.empty() )
                return false;

            size_t start = ( aTok[0] == '-' ) ? 1 : 0;

            if( start == aTok.size() )
                return false;

            return std::all_of( aTok.begin() + start, aTok.end(),
                                []( unsigned char c )
                                {
                                    return std::isdigit( c ) != 0;
                                } );
        };

        if( ( iss >> name >> signal >> nverts >> flags ) && !isInt( name ) && !isInt( signal ) && nverts > 0 )
        {
            ++aConnections;
            aVertices += static_cast<size_t>( nverts );

            for( int k = 0; k < nverts && std::getline( file, line ); ++k )
                ;
        }
    }
}


BOOST_AUTO_TEST_CASE( ExternalSingleSheetWireTilingMatchesAsciiConnections )
{
    // SC350430B01 is the single-sheet reference design, so its whole vertex pool
    // belongs to one split-header run.  The cumulative chain must tile it into
    // exactly the ASCII *CONNECTION* polylines, with the bus gap excluded.

    std::vector<uint8_t> data;
    BOOST_REQUIRE( PADS_SCH_BINARY_READER::ReadFile( wxString::FromUTF8( pair.binaryPath ), data ) );

    PADS_SCH_BINARY_READER reader;
    BOOST_REQUIRE( reader.Parse( data ) );

    size_t asciiConnections = 0;
    size_t asciiVertices = 0;
    asciiConnectionCounts( pair.asciiPath, asciiConnections, asciiVertices );

    BOOST_REQUIRE_EQUAL( asciiConnections, 91u );
    BOOST_REQUIRE_EQUAL( asciiVertices, 196u );

    BOOST_CHECK_EQUAL( reader.GetWirePolylines().size(), asciiConnections );

    size_t polyVertices = 0;

    for( const std::vector<PADS_SCH_BINARY::WIRE_VERTEX>& poly : reader.GetWirePolylines() )
        polyVertices += poly.size();

    BOOST_CHECK_EQUAL( polyVertices, asciiVertices );

    // The raw pool is the connection vertices plus the single 4-vertex bus.
    BOOST_CHECK_EQUAL( reader.GetWireVertices().size(), 200u );

    // That 4-vertex gap is the SENSOR_SIGNALS bus polyline.
    BOOST_REQUIRE_EQUAL( reader.GetBusPolylines().size(), 1u );
    BOOST_REQUIRE_EQUAL( reader.GetBusPolylines()[0].size(), 4u );
    BOOST_CHECK_EQUAL( reader.GetBusPolylines()[0][0].x_mils, 5200 );
    BOOST_CHECK_EQUAL( reader.GetBusPolylines()[0][0].y_mils, 4400 );
    BOOST_CHECK_EQUAL( reader.GetBusPolylines()[0][3].x_mils, 11200 );
    BOOST_CHECK_EQUAL( reader.GetBusPolylines()[0][3].y_mils, 3400 );
}


// Every placement on every sheet binds to a CAE-decal so the importer can draw a real
// symbol body and pins, not a placeholder.  This guards the multi-sheet regression where a
// >64-pin gate truncated a sheet's used-decal table and dropped every later placement.
BOOST_AUTO_TEST_CASE( DecalBindingRateAllSheets )
{
}


// Off-page references are discriminated into KiCad element kinds: net-name ports (@TERM)
// become local labels, power/ground ports become power symbols, off-sheet/bus references
// stay global labels.  Guards the deterministic kind decode (record +0x08 + the -0x12 group
// handle into the canonical $OSR table).
BOOST_AUTO_TEST_CASE( NetLabelKinds )
{
}


BOOST_AUTO_TEST_CASE( PinNamesAndTypes )
{
}


BOOST_AUTO_TEST_SUITE_END()

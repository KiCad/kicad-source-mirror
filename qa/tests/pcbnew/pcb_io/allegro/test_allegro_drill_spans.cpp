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
 * @file test_allegro_drill_spans.cpp
 *
 * Cross-validate Allegro via and through-hole-pad import against the fabrication Excellon drill
 * files (.drl), whose filenames encode the layer span (e.g. "...-3-6.drl" spans layers 3 to 6).
 * Verifies that blind, buried, and through vias are classified onto the right layers instead of
 * being flattened to through holes.
 */

#include "allegro_test_utils.h"
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <lset.h>
#include <pad.h>
#include <pcb_track.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <tuple>
#include <vector>

using namespace KI_TEST;


namespace
{
/// A copper-layer span expressed in 1-based fabrication layer indices (1 == top copper).
using LAYER_SPAN = std::pair<int, int>;

/// Per-span table of drill diameter (in whole micrometres) to the number of hits.
using SPAN_DRILL_TABLE = std::map<LAYER_SPAN, std::map<int, int>>;


/// Round a dimension in KiCad internal units (nanometres) to whole micrometres.
int toMicrons( int aNanometres )
{
    return static_cast<int>( std::llround( aNanometres / 1000.0 ) );
}


/**
 * Parse a single Excellon (.drl) drill file, returning the span (from the filename) and
 * a tally of drill diameter (µm) to hit count. Excellon @c Rnn repeat commands expand to
 * @c nn additional hits of the active tool.
 */
std::pair<LAYER_SPAN, std::map<int, int>> parseDrillFile( const std::filesystem::path& aPath )
{
    static const std::regex spanRe( R"(-(\d+)-(\d+)\.drl$)" );
    static const std::regex toolDefRe( R"(^T(\d+)C([0-9.]+))" );
    static const std::regex toolSelRe( R"(^T(\d+)\s*$)" );
    static const std::regex coordRe( R"(^X.*Y)" );
    static const std::regex repeatRe( R"(^R(\d+))" );

    LAYER_SPAN span{ 0, 0 };
    std::smatch m;
    std::string name = aPath.filename().string();

    if( std::regex_search( name, m, spanRe ) )
        span = { std::stoi( m[1] ), std::stoi( m[2] ) };

    std::map<int, int> toolDiaUm;   // tool number -> diameter in µm
    std::map<int, int> diaToCount;  // diameter µm -> hit count
    int                activeTool = -1;

    std::ifstream file( aPath );
    std::string   line;

    while( std::getline( file, line ) )
    {
        // Excellon files use CRLF; strip the trailing CR so regexes anchor cleanly.
        while( !line.empty() && ( line.back() == '\r' || line.back() == '\n' ) )
            line.pop_back();

        if( std::regex_search( line, m, toolDefRe ) )
        {
            int    tool = std::stoi( m[1] );
            double mm = std::stod( m[2] );
            toolDiaUm[tool] = static_cast<int>( std::llround( mm * 1000.0 ) );
        }
        else if( std::regex_search( line, m, toolSelRe ) )
        {
            activeTool = std::stoi( m[1] );
        }
        else if( std::regex_search( line, m, coordRe ) )
        {
            if( activeTool >= 0 && toolDiaUm.count( activeTool ) )
                diaToCount[toolDiaUm[activeTool]] += 1;
        }
        else if( std::regex_search( line, m, repeatRe ) )
        {
            if( activeTool >= 0 && toolDiaUm.count( activeTool ) )
                diaToCount[toolDiaUm[activeTool]] += std::stoi( m[1] );
        }
    }

    return { span, diaToCount };
}


/// Load the ground-truth span/drill table from every .drl file in a board directory.
SPAN_DRILL_TABLE loadDrillGroundTruth( const std::string& aBoardDir )
{
    SPAN_DRILL_TABLE table;

    for( const auto& entry : std::filesystem::directory_iterator( aBoardDir ) )
    {
        if( entry.path().extension() != ".drl" )
            continue;

        auto [span, counts] = parseDrillFile( entry.path() );

        for( const auto& [dia, count] : counts )
            table[span][dia] += count;
    }

    return table;
}


/// Map each enabled copper layer to its 1-based fabrication index (F_Cu == 1).
std::map<PCB_LAYER_ID, int> buildCopperOrdinal( const BOARD& aBoard )
{
    std::map<PCB_LAYER_ID, int> ordinal;
    int                         idx = 1;

    for( PCB_LAYER_ID layer : aBoard.GetEnabledLayers().CuStack() )
        ordinal[layer] = idx++;

    return ordinal;
}


/// Normalize a via's KiCad top/bottom layers to a 1-based fabrication span (top <= bottom).
LAYER_SPAN viaLayerSpan( const PCB_VIA& aVia, const std::map<PCB_LAYER_ID, int>& aOrdinal, int aTotal )
{
    auto index = [&aOrdinal]( PCB_LAYER_ID aLayer, int aDefault )
    {
        auto it = aOrdinal.find( aLayer );
        return it != aOrdinal.end() ? it->second : aDefault;
    };

    int top = index( aVia.TopLayer(), 1 );
    int bot = index( aVia.BottomLayer(), aTotal );

    if( top > bot )
        std::swap( top, bot );

    return { top, bot };
}


/// Collect every drilled feature (via + through-hole pad) from the imported board.
SPAN_DRILL_TABLE collectBoardDrillTable( const BOARD& aBoard )
{
    SPAN_DRILL_TABLE            table;
    std::map<PCB_LAYER_ID, int> ordinal = buildCopperOrdinal( aBoard );
    const int                   total = aBoard.GetCopperLayerCount();

    for( PCB_TRACK* track : aBoard.Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( track );
        int      drill = toMicrons( via->GetDrillValue() );

        if( drill <= 0 )
            continue;

        table[viaLayerSpan( *via, ordinal, total )][drill] += 1;
    }

    for( FOOTPRINT* fp : aBoard.Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetAttribute() != PAD_ATTRIB::PTH && pad->GetAttribute() != PAD_ATTRIB::NPTH )
                continue;

            // The Excellon .drl files describe round drills only; oblong slots are routed and
            // live in the companion .rou file, so they are out of scope for this comparison.
            if( pad->GetDrillShape() == PAD_DRILL_SHAPE::OBLONG )
                continue;

            int drill = toMicrons( pad->GetDrillSizeX() );

            if( drill <= 0 )
                continue;

            // KiCad through-hole pads always span the full copper stack.
            table[{ 1, total }][drill] += 1;
        }
    }

    return table;
}


/// Human-readable dump of a span/drill table for diagnostic output.
std::string describeTable( const SPAN_DRILL_TABLE& aTable )
{
    std::string out;

    for( const auto& [span, sizes] : aTable )
    {
        for( const auto& [dia, count] : sizes )
        {
            out += "    span " + std::to_string( span.first ) + "-" + std::to_string( span.second )
                   + "  drill " + std::to_string( dia ) + "um  x " + std::to_string( count ) + "\n";
        }
    }

    return out;
}


struct DRILL_SPAN_FIXTURE
{
    DRILL_SPAN_FIXTURE()
    {
        m_boardDir = KI_TEST::AllegroBoardDataDir( "ABX00162_UNOQ" );
        m_groundTruth = loadDrillGroundTruth( m_boardDir );
        m_board = KI_TEST::ALLEGRO_CACHED_LOADER::GetInstance().GetCachedBoard(
                m_boardDir + "20250806_UNOQ.brd" );
    }

    std::string      m_boardDir;
    SPAN_DRILL_TABLE m_groundTruth;
    BOARD*           m_board = nullptr;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( AllegroDrillSpans, DRILL_SPAN_FIXTURE )


/**
 * The fixture and its ground truth must be well-formed before the cross-validation tests
 * have any meaning. This guards against a missing board file or a drill-file parse error
 * silently turning the real assertions into no-ops.
 */
BOOST_AUTO_TEST_CASE( GroundTruthAndBoardLoad )
{
    BOOST_REQUIRE_MESSAGE( m_board != nullptr, "20250806_UNOQ.brd failed to import" );

    // The .drl set describes six spans: 1-2, 2-3, 3-6, 6-7, 7-8 and the 1-8 through holes.
    BOOST_REQUIRE_MESSAGE( m_groundTruth.size() >= 6,
                           "Expected at least 6 drill spans, parsed " << m_groundTruth.size() );

    BOOST_TEST_MESSAGE( "Ground-truth drill table:\n" << describeTable( m_groundTruth ) );

    BOOST_CHECK_EQUAL( m_board->GetCopperLayerCount(), 8 );
}


/**
 * The board has exactly six via spans, five of them blind/buried. Guards against the old importer
 * which hardcoded every via to a through hole.
 */
BOOST_AUTO_TEST_CASE( BlindAndBuriedViasPresent )
{
    BOOST_REQUIRE( m_board != nullptr );

    std::map<VIATYPE, int>      typeCounts;
    std::set<LAYER_SPAN>        viaSpans;
    std::map<PCB_LAYER_ID, int> ordinal = buildCopperOrdinal( *m_board );
    const int                   total = m_board->GetCopperLayerCount();

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( track );
        typeCounts[via->GetViaType()]++;
        viaSpans.insert( viaLayerSpan( *via, ordinal, total ) );
    }

    BOOST_TEST_MESSAGE( "Via type counts: THROUGH=" << typeCounts[VIATYPE::THROUGH]
                        << " BLIND=" << typeCounts[VIATYPE::BLIND]
                        << " BURIED=" << typeCounts[VIATYPE::BURIED]
                        << " MICROVIA=" << typeCounts[VIATYPE::MICROVIA] );

    std::string spanList;

    for( const LAYER_SPAN& s : viaSpans )
        spanList += " " + std::to_string( s.first ) + "-" + std::to_string( s.second );

    BOOST_TEST_MESSAGE( "Distinct via spans:" << spanList );

    int nonThrough = typeCounts[VIATYPE::BLIND] + typeCounts[VIATYPE::BURIED]
                     + typeCounts[VIATYPE::MICROVIA];

    BOOST_CHECK_MESSAGE( nonThrough > 0, "Importer produced no blind/buried/microvia vias" );

    // The board has exactly six via spans: five blind/buried plus the 1-8 through span.
    const std::set<LAYER_SPAN> expectedViaSpans = { { 1, 2 }, { 1, 8 }, { 2, 3 },
                                                    { 3, 6 }, { 6, 7 }, { 7, 8 } };

    BOOST_CHECK_MESSAGE( viaSpans == expectedViaSpans, "Unexpected via span set:" << spanList );
}


/**
 * Every (span, drill-diameter) pair in the drill files must appear in the imported board and vice
 * versa, so each diameter both survives the padstack parse and lands on the right layer span.
 */
BOOST_AUTO_TEST_CASE( DrillSpansMatchExcellon )
{
    BOOST_REQUIRE( m_board != nullptr );

    SPAN_DRILL_TABLE boardTable = collectBoardDrillTable( *m_board );

    BOOST_TEST_MESSAGE( "Imported board drill table:\n" << describeTable( boardTable ) );

    // Reduce both tables to the set of (span, diameter) keys, ignoring counts.
    auto keySet = []( const SPAN_DRILL_TABLE& aTable )
    {
        std::set<std::tuple<int, int, int>> keys;

        for( const auto& [span, sizes] : aTable )
            for( const auto& [dia, count] : sizes )
                keys.insert( { span.first, span.second, dia } );

        return keys;
    };

    std::set<std::tuple<int, int, int>> truthKeys = keySet( m_groundTruth );
    std::set<std::tuple<int, int, int>> boardKeys = keySet( boardTable );

    std::vector<std::tuple<int, int, int>> missing; // in .drl but not in board
    std::vector<std::tuple<int, int, int>> extra;   // in board but not in .drl

    std::set_difference( truthKeys.begin(), truthKeys.end(), boardKeys.begin(), boardKeys.end(),
                         std::back_inserter( missing ) );
    std::set_difference( boardKeys.begin(), boardKeys.end(), truthKeys.begin(), truthKeys.end(),
                         std::back_inserter( extra ) );

    for( const auto& [t, b, d] : missing )
        BOOST_TEST_MESSAGE( "  MISSING from board: span " << t << "-" << b << " drill " << d << "um" );

    for( const auto& [t, b, d] : extra )
        BOOST_TEST_MESSAGE( "  EXTRA in board: span " << t << "-" << b << " drill " << d << "um" );

    BOOST_CHECK_MESSAGE( missing.empty(),
                         missing.size() << " (span, drill) combinations from the .drl files are "
                         "absent from the imported board" );
    BOOST_CHECK_MESSAGE( extra.empty(),
                         extra.size() << " (span, drill) combinations in the imported board do not "
                         "exist in any .drl file" );

    // The key set alone would miss holes shifted between two spans that both exist (e.g. some 1-2
    // microvias landing on 2-3), so also compare counts. The tolerance absorbs the small drift from
    // the board being a revision newer than the drill files while still catching gross shifts.
    for( const auto& [span, sizes] : m_groundTruth )
    {
        for( const auto& [dia, truthCount] : sizes )
        {
            int boardCount = 0;

            if( auto sIt = boardTable.find( span ); sIt != boardTable.end() )
            {
                if( auto dIt = sIt->second.find( dia ); dIt != sIt->second.end() )
                    boardCount = dIt->second;
            }

            const int tolerance = std::max( 5, truthCount / 100 );

            BOOST_CHECK_MESSAGE( std::abs( boardCount - truthCount ) <= tolerance,
                                 "span " << span.first << "-" << span.second << " drill " << dia
                                 << "um: board has " << boardCount << " holes, .drl has "
                                 << truthCount << " (tolerance " << tolerance << ")" );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()

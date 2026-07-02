/*
 * This program is part of KiCad, a free EDA CAD application.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/shape_poly_set.h>
#include <geometry/geometry_predicates.h>
#include <core/profile.h>

#include "geom_test_utils.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{

struct ZONE_ENTRY
{
    std::string    layer;
    std::string    net;
    int            outlineCount = 0;
    int            vertexCount  = 0;
    SHAPE_POLY_SET polySet;
};


struct BOARD_ENTRY
{
    std::string             source;
    std::vector<ZONE_ENTRY> zones;
};


struct ZONE_STATS
{
    std::string layer;
    std::string net;
    int         outlineCount     = 0;
    int         vertexCount      = 0;
    int         triangleCount    = 0;
    int64_t     timeUs           = 0;
    double      originalArea     = 0.0;
    double      triangulatedArea = 0.0;
    double      areaCoverage     = 0.0;
    double      meanTriArea      = 0.0;
    double      stddevTriArea    = 0.0;
    int         spikeyTriangles  = 0;
    double      spikeyRatio      = 0.0;

    // Regularity metrics. minAngle percentiles describe the tail of thin triangles; the
    // worst 1% (p1) is the sliver signal, p50 the typical shape. Angles are degrees.
    double      minAnglePctl1    = 0.0;
    double      minAnglePctl5    = 0.0;
    double      minAnglePctl50   = 0.0;
    int         trisBelow5Deg    = 0;
    int         trisBelow10Deg   = 0;
    int         trisBelow15Deg   = 0;
    double      meanRadiusRatio  = 0.0;

    // Per-triangle minimum interior angles, retained so the global run can aggregate
    // corpus-wide percentiles rather than averaging per-zone summaries.
    std::vector<double> minAngles;
};


struct BASELINE_ZONE
{
    std::string layer;
    std::string net;
    int         triangleCount   = 0;
    double      areaCoverage    = 0.0;
    double      spikeyRatio     = 0.0;
    double      stddevTriArea   = 0.0;
    int         spikeyTriangles = 0;
    double      originalArea    = 0.0;
    double      minAnglePctl1   = 0.0;
    double      minAnglePctl5   = 0.0;
    int         trisBelow10Deg  = 0;
    int64_t     timeUs          = 0;
};


struct BASELINE_BOARD
{
    std::vector<BASELINE_ZONE> zones;
};


struct BASELINE_DATA
{
    std::map<std::string, BASELINE_BOARD> boards;
    int    totalTriangles = 0;
    int    totalSpikeyTri = 0;
    double spikeyRatio    = 0.0;
    int    zoneCount      = 0;
    int    boardCount     = 0;
    bool   valid          = false;
};


enum class CHANGE_TYPE
{
    BREAKING,
    REGRESSION,
    IMPROVEMENT,
    UNCHANGED
};


struct ZONE_COMPARISON
{
    CHANGE_TYPE type = CHANGE_TYPE::UNCHANGED;
    std::string source;
    std::string layer;
    std::string net;

    int    baseTriangles    = 0;
    int    curTriangles     = 0;
    double baseSpikeyRatio  = 0.0;
    double curSpikeyRatio   = 0.0;
    double baseStddev       = 0.0;
    double curStddev        = 0.0;
    double baseCoverage     = 0.0;
    double curCoverage      = 0.0;
    double baseMinAngleP5   = 0.0;
    double curMinAngleP5    = 0.0;
    int    baseBelow10Deg   = 0;
    int    curBelow10Deg    = 0;
    int64_t baseTimeUs      = 0;
    int64_t curTimeUs       = 0;

    double spikeyDeltaPp() const { return ( curSpikeyRatio - baseSpikeyRatio ) * 100.0; }

    // Positive means the current run's worst-tail triangles opened up (more regular).
    double minAngleP5DeltaDeg() const { return curMinAngleP5 - baseMinAngleP5; }

    double triangleDeltaPct() const
    {
        if( baseTriangles == 0 )
            return curTriangles == 0 ? 0.0 : 100.0;

        return ( curTriangles - baseTriangles ) / static_cast<double>( baseTriangles ) * 100.0;
    }

    double stddevDeltaPct() const
    {
        if( baseStddev == 0.0 )
            return curStddev == 0.0 ? 0.0 : 100.0;

        return ( curStddev - baseStddev ) / baseStddev * 100.0;
    }

    double timeDeltaPct() const
    {
        if( baseTimeUs == 0 )
            return curTimeUs == 0 ? 0.0 : 100.0;

        return static_cast<double>( curTimeUs - baseTimeUs ) / static_cast<double>( baseTimeUs )
               * 100.0;
    }
};


BASELINE_DATA LoadBaseline( const fs::path& aJsonPath )
{
    BASELINE_DATA baseline;

    if( !fs::exists( aJsonPath ) )
        return baseline;

    std::ifstream file( aJsonPath );

    if( !file.is_open() )
        return baseline;

    nlohmann::json j;

    try
    {
        j = nlohmann::json::parse( file );
    }
    catch( const nlohmann::json::exception& )
    {
        return baseline;
    }

    if( j.contains( "metadata" ) )
    {
        baseline.boardCount = j["metadata"].value( "board_count", 0 );
        baseline.zoneCount  = j["metadata"].value( "zone_count", 0 );
    }

    if( j.contains( "global" ) )
    {
        baseline.totalTriangles = j["global"].value( "total_triangles", 0 );
        baseline.totalSpikeyTri = j["global"].value( "total_spikey_triangles", 0 );
        baseline.spikeyRatio    = j["global"].value( "spikey_ratio", 0.0 );
    }

    if( j.contains( "boards" ) )
    {
        for( const auto& boardJson : j["boards"] )
        {
            std::string source = boardJson.value( "source", "" );
            BASELINE_BOARD board;

            if( boardJson.contains( "zones" ) )
            {
                for( const auto& zoneJson : boardJson["zones"] )
                {
                    BASELINE_ZONE zone;
                    zone.layer           = zoneJson.value( "layer", "" );
                    zone.net             = zoneJson.value( "net", "" );
                    zone.triangleCount   = zoneJson.value( "triangle_count", 0 );
                    zone.areaCoverage    = zoneJson.value( "area_coverage", 0.0 );
                    zone.spikeyRatio     = zoneJson.value( "spikey_ratio", 0.0 );
                    zone.stddevTriArea   = zoneJson.value( "stddev_triangle_area_nm2", 0.0 );
                    zone.spikeyTriangles = zoneJson.value( "spikey_triangles", 0 );
                    zone.originalArea    = zoneJson.value( "original_area_nm2", 0.0 );
                    zone.minAnglePctl1   = zoneJson.value( "min_angle_p1_deg", 0.0 );
                    zone.minAnglePctl5   = zoneJson.value( "min_angle_p5_deg", 0.0 );
                    zone.trisBelow10Deg  = zoneJson.value( "tris_below_10deg", 0 );
                    zone.timeUs          = zoneJson.value( "time_us", (int64_t) 0 );
                    board.zones.push_back( zone );
                }
            }

            baseline.boards[source] = std::move( board );
        }
    }

    baseline.valid = true;
    return baseline;
}


bool ParsePolyFile( const fs::path& aPath, BOARD_ENTRY& aBoard )
{
    std::ifstream file( aPath );

    if( !file.is_open() )
        return false;

    std::string content( ( std::istreambuf_iterator<char>( file ) ),
                         std::istreambuf_iterator<char>() );

    size_t srcStart = content.find( "(source \"" );

    if( srcStart != std::string::npos )
    {
        srcStart += 9;
        size_t srcEnd = content.find( "\")", srcStart );

        if( srcEnd != std::string::npos )
            aBoard.source = content.substr( srcStart, srcEnd - srcStart );
    }

    size_t zonePos = 0;

    while( ( zonePos = content.find( "(zone (layer \"", zonePos ) ) != std::string::npos )
    {
        ZONE_ENTRY entry;

        size_t layerStart = zonePos + 14;
        size_t layerEnd = content.find( "\")", layerStart );
        entry.layer = content.substr( layerStart, layerEnd - layerStart );

        size_t netStart = content.find( "(net \"", layerEnd );

        if( netStart != std::string::npos )
        {
            netStart += 6;
            size_t netEnd = content.find( "\")", netStart );
            entry.net = content.substr( netStart, netEnd - netStart );
        }

        size_t ocStart = content.find( "(outline_count ", layerEnd );

        if( ocStart != std::string::npos )
        {
            ocStart += 15;
            entry.outlineCount = std::stoi( content.substr( ocStart ) );
        }

        size_t vcStart = content.find( "(vertex_count ", layerEnd );

        if( vcStart != std::string::npos )
        {
            vcStart += 14;
            entry.vertexCount = std::stoi( content.substr( vcStart ) );
        }

        size_t polysetStart = content.find( "polyset ", zonePos );

        if( polysetStart != std::string::npos )
        {
            std::string remainder = content.substr( polysetStart );
            std::stringstream ss( remainder );

            if( entry.polySet.Parse( ss ) )
                aBoard.zones.push_back( std::move( entry ) );
        }

        zonePos = layerEnd + 1;
    }

    return !aBoard.zones.empty();
}


// Number of cold re-triangulations timed per zone; the median rejects scheduler jitter.
constexpr int TIMING_ITERATIONS = 5;


// Radius ratio 2r/R normalized to [0,1]; 1.0 is equilateral, 0.0 is degenerate.
double TriangleRadiusRatio( const VECTOR2I& a, const VECTOR2I& b, const VECTOR2I& c )
{
    double ab = a.Distance( b );
    double bc = b.Distance( c );
    double ca = c.Distance( a );

    double s = ( ab + bc + ca ) / 2.0;

    if( s <= 0.0 || ab <= 0.0 || bc <= 0.0 || ca <= 0.0 )
        return 0.0;

    double area = std::sqrt( std::max( 0.0, s * ( s - ab ) * ( s - bc ) * ( s - ca ) ) );

    return 8.0 * area * area / ( s * ab * bc * ca );
}


// Linear-interpolated percentile over a pre-sorted ascending vector.
double Percentile( const std::vector<double>& aSorted, double aPct )
{
    if( aSorted.empty() )
        return 0.0;

    if( aSorted.size() == 1 )
        return aSorted.front();

    double rank = aPct / 100.0 * static_cast<double>( aSorted.size() - 1 );
    size_t lo   = static_cast<size_t>( std::floor( rank ) );
    size_t hi   = static_cast<size_t>( std::ceil( rank ) );
    double frac = rank - static_cast<double>( lo );

    return aSorted[lo] + frac * ( aSorted[hi] - aSorted[lo] );
}


ZONE_STATS ComputeZoneStats( ZONE_ENTRY& aZone )
{
    ZONE_STATS stats;
    stats.layer        = aZone.layer;
    stats.net          = aZone.net;
    stats.outlineCount = aZone.outlineCount;
    stats.vertexCount  = aZone.vertexCount;
    stats.originalArea = aZone.polySet.Area();

    // Cold-cache timing: aZone.polySet is not triangulated until after this loop, so each copy
    // starts with an invalid cache and the timer captures a full re-triangulation, never a cache
    // hit. Report the median across iterations to reject scheduler jitter.
    std::vector<int64_t> timings;

    for( int iter = 0; iter < TIMING_ITERATIONS; iter++ )
    {
        SHAPE_POLY_SET cold( aZone.polySet );

        PROF_TIMER timer;
        cold.CacheTriangulation();
        timer.Stop();
        timings.push_back( static_cast<int64_t>( timer.msecs() * 1000.0 ) );
    }

    std::sort( timings.begin(), timings.end() );
    stats.timeUs = timings[timings.size() / 2];

    // Populate the zone's own cache once for the geometry walk below.
    aZone.polySet.CacheTriangulation();

    std::vector<double> triAreas;

    for( unsigned int i = 0; i < aZone.polySet.TriangulatedPolyCount(); i++ )
    {
        const auto* triPoly = aZone.polySet.TriangulatedPolygon( static_cast<int>( i ) );

        for( const auto& tri : triPoly->Triangles() )
            triAreas.push_back( tri.Area() );
    }

    stats.triangleCount = static_cast<int>( triAreas.size() );
    stats.triangulatedArea = std::accumulate( triAreas.begin(), triAreas.end(), 0.0 );

    if( stats.originalArea > 0.0 )
        stats.areaCoverage = stats.triangulatedArea / stats.originalArea;

    if( !triAreas.empty() )
    {
        stats.meanTriArea = stats.triangulatedArea / static_cast<double>( triAreas.size() );

        double sumSqDiff = 0.0;

        for( double a : triAreas )
        {
            double diff = a - stats.meanTriArea;
            sumSqDiff += diff * diff;
        }

        stats.stddevTriArea = std::sqrt( sumSqDiff / static_cast<double>( triAreas.size() ) );
    }

    double radiusRatioSum = 0.0;

    for( unsigned int i = 0; i < aZone.polySet.TriangulatedPolyCount(); i++ )
    {
        const auto* triPoly = aZone.polySet.TriangulatedPolygon( static_cast<int>( i ) );

        for( const auto& tri : triPoly->Triangles() )
        {
            VECTOR2I pa = tri.GetPoint( 0 );
            VECTOR2I pb = tri.GetPoint( 1 );
            VECTOR2I pc = tri.GetPoint( 2 );

            if( KIGEOM::IsSliverTriangle( pa, pb, pc ) )
                stats.spikeyTriangles++;

            double minAngle = GEOM_TEST::TriangleMinAngleDeg( pa, pb, pc );
            stats.minAngles.push_back( minAngle );

            if( minAngle < 5.0 )
                stats.trisBelow5Deg++;

            if( minAngle < 10.0 )
                stats.trisBelow10Deg++;

            if( minAngle < 15.0 )
                stats.trisBelow15Deg++;

            radiusRatioSum += TriangleRadiusRatio( pa, pb, pc );
        }
    }

    if( stats.triangleCount > 0 )
    {
        stats.spikeyRatio      = static_cast<double>( stats.spikeyTriangles ) / stats.triangleCount;
        stats.meanRadiusRatio  = radiusRatioSum / stats.triangleCount;

        std::vector<double> sorted = stats.minAngles;
        std::sort( sorted.begin(), sorted.end() );
        stats.minAnglePctl1  = Percentile( sorted, 1.0 );
        stats.minAnglePctl5  = Percentile( sorted, 5.0 );
        stats.minAnglePctl50 = Percentile( sorted, 50.0 );
    }

    return stats;
}


nlohmann::json ZoneStatsToJson( const ZONE_STATS& aStats )
{
    nlohmann::json j;
    j["layer"]                    = aStats.layer;
    j["net"]                      = aStats.net;
    j["outline_count"]            = aStats.outlineCount;
    j["vertex_count"]             = aStats.vertexCount;
    j["triangle_count"]           = aStats.triangleCount;
    j["time_us"]                  = aStats.timeUs;
    j["original_area_nm2"]        = aStats.originalArea;
    j["triangulated_area_nm2"]    = aStats.triangulatedArea;
    j["area_coverage"]            = aStats.areaCoverage;
    j["mean_triangle_area_nm2"]   = aStats.meanTriArea;
    j["stddev_triangle_area_nm2"] = aStats.stddevTriArea;
    j["spikey_triangles"]         = aStats.spikeyTriangles;
    j["spikey_ratio"]             = aStats.spikeyRatio;
    j["min_angle_p1_deg"]         = aStats.minAnglePctl1;
    j["min_angle_p5_deg"]         = aStats.minAnglePctl5;
    j["min_angle_p50_deg"]        = aStats.minAnglePctl50;
    j["tris_below_5deg"]          = aStats.trisBelow5Deg;
    j["tris_below_10deg"]         = aStats.trisBelow10Deg;
    j["tris_below_15deg"]         = aStats.trisBelow15Deg;
    j["mean_radius_ratio"]        = aStats.meanRadiusRatio;
    return j;
}


ZONE_COMPARISON CompareZone( const std::string& aSource, const ZONE_STATS& aCurrent,
                             const BASELINE_ZONE* aBaseline )
{
    ZONE_COMPARISON cmp;
    cmp.source          = aSource;
    cmp.layer           = aCurrent.layer;
    cmp.net             = aCurrent.net;
    cmp.curTriangles    = aCurrent.triangleCount;
    cmp.curSpikeyRatio  = aCurrent.spikeyRatio;
    cmp.curStddev       = aCurrent.stddevTriArea;
    cmp.curCoverage     = aCurrent.areaCoverage;
    cmp.curMinAngleP5   = aCurrent.minAnglePctl5;
    cmp.curBelow10Deg   = aCurrent.trisBelow10Deg;
    cmp.curTimeUs       = aCurrent.timeUs;

    if( !aBaseline )
    {
        cmp.type = CHANGE_TYPE::UNCHANGED;
        return cmp;
    }

    cmp.baseTriangles   = aBaseline->triangleCount;
    cmp.baseSpikeyRatio = aBaseline->spikeyRatio;
    cmp.baseStddev      = aBaseline->stddevTriArea;
    cmp.baseCoverage    = aBaseline->areaCoverage;
    cmp.baseMinAngleP5  = aBaseline->minAnglePctl5;
    cmp.baseBelow10Deg  = aBaseline->trisBelow10Deg;
    cmp.baseTimeUs      = aBaseline->timeUs;

    bool coverageBroke = aCurrent.originalArea > 0.0
                         && ( aCurrent.areaCoverage < 0.99 || aCurrent.areaCoverage > 1.01 );
    bool newFailure = aCurrent.triangleCount == 0 && aBaseline->triangleCount > 0
                      && aCurrent.originalArea > 0.0;

    if( coverageBroke || newFailure )
    {
        cmp.type = CHANGE_TYPE::BREAKING;
        return cmp;
    }

    // Lexicographic classification honoring the project priority speed > regularity > count.
    // The first axis that moves beyond its noise threshold decides the verdict; lower-priority
    // axes are only consulted when every higher-priority axis is unchanged. This prevents a
    // regularity or count win from masking a speed regression.
    struct AXIS
    {
        double delta;      // signed change, current minus baseline
        double threshold;  // magnitude below which the axis counts as unchanged
        bool   lowerBetter;
    };

    // Regularity blends p5 min-angle (want higher) and sub-10 degree count (want lower) into a
    // single signed score where positive is worse, matching the lowerBetter axes.
    double regressScore = ( cmp.baseMinAngleP5 - cmp.curMinAngleP5 )
                          + ( cmp.curBelow10Deg - cmp.baseBelow10Deg )
                                    / std::max( 1.0, static_cast<double>( cmp.baseTriangles ) )
                                    * 100.0;

    // Per-zone timing under ~100 us is dominated by scheduler jitter, so a percent delta there is
    // meaningless; below the floor the speed axis is neutralized and the verdict falls through to
    // regularity then count. Aggregate speed is judged corpus-wide (total time), not per zone.
    constexpr int64_t SPEED_FLOOR_US = 100;
    bool speedMeasurable = std::max( cmp.baseTimeUs, cmp.curTimeUs ) >= SPEED_FLOOR_US;

    std::vector<AXIS> axes = {
        { speedMeasurable ? cmp.timeDeltaPct() : 0.0, 15.0, true },  // speed (slower is worse)
        { regressScore,                                1.0, true },  // regularity (positive worse)
        { cmp.triangleDeltaPct(),                      5.0, true },  // count (more tris is worse)
    };

    cmp.type = CHANGE_TYPE::UNCHANGED;

    for( const AXIS& axis : axes )
    {
        double signedWorse = axis.lowerBetter ? axis.delta : -axis.delta;

        if( signedWorse > axis.threshold )
        {
            cmp.type = CHANGE_TYPE::REGRESSION;
            break;
        }

        if( signedWorse < -axis.threshold )
        {
            cmp.type = CHANGE_TYPE::IMPROVEMENT;
            break;
        }
    }

    return cmp;
}


std::string FormatSign( double aValue, const std::string& aSuffix )
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision( 1 );

    if( aValue > 0.0 )
        ss << "+";

    ss << aValue << aSuffix;
    return ss.str();
}


std::string FormatZoneDetail( const ZONE_COMPARISON& aCmp )
{
    std::ostringstream ss;
    ss << "  " << aCmp.source << " " << aCmp.layer << " \"" << aCmp.net << "\"" << "\n";
    ss << std::fixed << std::setprecision( 1 );
    ss << "    time: " << FormatSign( aCmp.timeDeltaPct(), "%" );
    ss << "  minAngleP5: " << aCmp.baseMinAngleP5 << " -> " << aCmp.curMinAngleP5
       << " deg (" << FormatSign( aCmp.minAngleP5DeltaDeg(), "deg" ) << ")";
    ss << "  <10deg: " << aCmp.baseBelow10Deg << " -> " << aCmp.curBelow10Deg;
    ss << "\n    spikey: " << ( aCmp.baseSpikeyRatio * 100.0 ) << "% -> "
       << ( aCmp.curSpikeyRatio * 100.0 ) << "% (" << FormatSign( aCmp.spikeyDeltaPp(), "pp" )
       << ")";
    ss << "  triangles: " << aCmp.baseTriangles << " -> " << aCmp.curTriangles
       << " (" << FormatSign( aCmp.triangleDeltaPct(), "%" ) << ")";

    if( aCmp.baseStddev > 0.0 || aCmp.curStddev > 0.0 )
    {
        ss << "  stddev: " << FormatSign( aCmp.stddevDeltaPct(), "%" );
    }

    return ss.str();
}


void OutputComparisonReport( const BASELINE_DATA& aBaseline,
                             const std::vector<ZONE_COMPARISON>& aComparisons,
                             int aTotalTriangles, int aTotalSpikeyTri, int aTotalZones )
{
    std::vector<ZONE_COMPARISON> breaking;
    std::vector<ZONE_COMPARISON> regressions;
    std::vector<ZONE_COMPARISON> improvements;
    int                          unchanged = 0;

    for( const auto& cmp : aComparisons )
    {
        switch( cmp.type )
        {
        case CHANGE_TYPE::BREAKING:    breaking.push_back( cmp ); break;
        case CHANGE_TYPE::REGRESSION:  regressions.push_back( cmp ); break;
        case CHANGE_TYPE::IMPROVEMENT: improvements.push_back( cmp ); break;
        case CHANGE_TYPE::UNCHANGED:   unchanged++; break;
        }
    }

    std::sort( improvements.begin(), improvements.end(),
               []( const ZONE_COMPARISON& a, const ZONE_COMPARISON& b )
               {
                   return a.spikeyDeltaPp() < b.spikeyDeltaPp();
               } );

    std::sort( regressions.begin(), regressions.end(),
               []( const ZONE_COMPARISON& a, const ZONE_COMPARISON& b )
               {
                   return a.spikeyDeltaPp() > b.spikeyDeltaPp();
               } );

    std::ostringstream report;
    report << std::fixed << std::setprecision( 1 );

    report << "\n=== Triangulation Comparison vs Baseline ===\n\n";

    report << "Baseline: " << aBaseline.boardCount << " boards, "
           << aBaseline.zoneCount << " zones\n";
    report << "Current:  " << aTotalZones << " zones\n\n";

    double baseSpikey = aBaseline.spikeyRatio * 100.0;
    double curSpikey = aTotalTriangles > 0
                               ? static_cast<double>( aTotalSpikeyTri ) / aTotalTriangles * 100.0
                               : 0.0;

    report << "Global:\n";
    report << "  Triangles:  " << aBaseline.totalTriangles << " -> " << aTotalTriangles
           << " (" << FormatSign(
                          aTotalTriangles - aBaseline.totalTriangles == 0
                                  ? 0.0
                                  : ( aTotalTriangles - aBaseline.totalTriangles )
                                            / static_cast<double>( aBaseline.totalTriangles )
                                            * 100.0,
                          "%" )
           << ")\n";
    report << "  Spikey:     " << baseSpikey << "% -> " << curSpikey << "% ("
           << FormatSign( curSpikey - baseSpikey, "pp" ) << ")\n";
    report << "  Spikey ct:  " << aBaseline.totalSpikeyTri << " -> " << aTotalSpikeyTri
           << "\n\n";

    report << "BREAKING: " << breaking.size() << " zones\n";

    for( const auto& cmp : breaking )
        report << FormatZoneDetail( cmp ) << "\n";

    if( breaking.empty() )
        report << "  (none)\n";

    report << "\nREGRESSIONS: " << regressions.size() << " zones"
           << " (lexicographic: speed >+5%, else regularity worse, else triangles >+5%)\n";

    int shown = 0;

    for( const auto& cmp : regressions )
    {
        if( shown >= 20 )
        {
            report << "  ... and " << ( regressions.size() - 20 ) << " more\n";
            break;
        }

        report << FormatZoneDetail( cmp ) << "\n";
        shown++;
    }

    if( regressions.empty() )
        report << "  (none)\n";

    report << "\nIMPROVEMENTS: " << improvements.size() << " zones\n";

    shown = 0;

    for( const auto& cmp : improvements )
    {
        if( shown >= 20 )
        {
            report << "  ... and " << ( improvements.size() - 20 ) << " more\n";
            break;
        }

        report << FormatZoneDetail( cmp ) << "\n";
        shown++;
    }

    if( improvements.empty() )
        report << "  (none)\n";

    report << "\nSummary: " << improvements.size() << " improved, "
           << regressions.size() << " regressed, "
           << breaking.size() << " breaking, "
           << unchanged << " unchanged\n";

    BOOST_TEST_MESSAGE( report.str() );

    BOOST_CHECK_MESSAGE( breaking.empty(),
                         std::to_string( breaking.size() )
                                 + " zone(s) have breaking triangulation changes" );
}


std::string GetTriangulationDataDir()
{
    return KI_TEST::GetTestDataRootDir() + "triangulation/";
}

}; // anonymous namespace


BOOST_AUTO_TEST_SUITE( TriangulationBenchmark )


BOOST_AUTO_TEST_CASE( BenchmarkAllExtractedPolygons )
{
    std::string dataDir = GetTriangulationDataDir();

    if( !fs::exists( dataDir ) || fs::is_empty( dataDir ) )
    {
        BOOST_TEST_MESSAGE( "No triangulation data in " << dataDir << ", skipping benchmark" );
        return;
    }

    fs::path jsonPath = fs::path( dataDir ) / "triangulation_status.json";
    BASELINE_DATA baseline = LoadBaseline( jsonPath );

    if( baseline.valid )
    {
        BOOST_TEST_MESSAGE( "Loaded baseline: " << baseline.boardCount << " boards, "
                            << baseline.zoneCount << " zones, "
                            << baseline.totalTriangles << " triangles" );
    }
    else
    {
        BOOST_TEST_MESSAGE( "No baseline found, running without comparison" );
    }

    std::vector<fs::path> polyFiles;

    for( const auto& entry : fs::directory_iterator( dataDir ) )
    {
        if( entry.path().extension() == ".kicad_polys" )
            polyFiles.push_back( entry.path() );
    }

    std::sort( polyFiles.begin(), polyFiles.end() );

    BOOST_TEST_MESSAGE( "Found " << polyFiles.size() << " polygon files" );

    // Stamp the report with the experimental variant under test (set by the harness / an env
    // var when A/B testing an algorithm change against the recorded baseline).
    if( const char* variant = std::getenv( "KICAD_TRI_VARIANT" ) )
        BOOST_TEST_MESSAGE( "Variant: " << variant );

    int    totalTriangles = 0;
    int    totalSpikeyTri = 0;
    int    totalZones     = 0;
    int    totalBelow10   = 0;
    double totalTimeUs    = 0.0;

    // Corpus-wide min-angle percentiles are computed from every triangle rather than by
    // averaging per-zone summaries, so one huge zone cannot dominate the tail statistics.
    std::vector<double> globalMinAngles;

    std::vector<ZONE_COMPARISON>    comparisons;

    for( const auto& polyFile : polyFiles )
    {
        BOARD_ENTRY board;

        if( !ParsePolyFile( polyFile, board ) )
        {
            BOOST_TEST_MESSAGE( "Failed to parse: " << polyFile.filename() );
            continue;
        }

        int    boardTriangles = 0;
        int    boardSpikey    = 0;
        double boardTimeUs    = 0.0;

        const BASELINE_BOARD* baseBoard = nullptr;
        auto                  it = baseline.boards.find( board.source );

        if( it != baseline.boards.end() )
            baseBoard = &it->second;

        for( size_t zi = 0; zi < board.zones.size(); zi++ )
        {
            ZONE_STATS stats = ComputeZoneStats( board.zones[zi] );

            BOOST_CHECK_MESSAGE(
                    stats.triangleCount > 0 || stats.originalArea == 0.0,
                    board.source + " " + stats.layer + " " + stats.net
                            + " produced 0 triangles with non-zero area" );

            if( stats.originalArea > 0.0 )
            {
                BOOST_CHECK_MESSAGE(
                        stats.areaCoverage > 0.999 && stats.areaCoverage < 1.001,
                        board.source + " " + stats.layer + " " + stats.net
                                + " area coverage: " + std::to_string( stats.areaCoverage ) );
            }

            if( baseline.valid )
            {
                const BASELINE_ZONE* baseZone = nullptr;

                if( baseBoard && zi < baseBoard->zones.size() )
                    baseZone = &baseBoard->zones[zi];

                comparisons.push_back( CompareZone( board.source, stats, baseZone ) );
            }

            boardTriangles += stats.triangleCount;
            boardSpikey    += stats.spikeyTriangles;
            boardTimeUs    += static_cast<double>( stats.timeUs );
            totalBelow10   += stats.trisBelow10Deg;
            globalMinAngles.insert( globalMinAngles.end(), stats.minAngles.begin(),
                                    stats.minAngles.end() );
            totalZones++;
        }

        totalTriangles += boardTriangles;
        totalSpikeyTri += boardSpikey;
        totalTimeUs    += boardTimeUs;
    }

    BOOST_TEST_MESSAGE( "Total triangles: " << totalTriangles
                        << "  Spikey: " << totalSpikeyTri
                        << " (" << ( totalTriangles > 0
                                         ? 100.0 * totalSpikeyTri / totalTriangles
                                         : 0.0 )
                        << "%)" );

    std::sort( globalMinAngles.begin(), globalMinAngles.end() );
    BOOST_TEST_MESSAGE( "Global min-angle deg  p1: " << Percentile( globalMinAngles, 1.0 )
                        << "  p5: " << Percentile( globalMinAngles, 5.0 )
                        << "  p50: " << Percentile( globalMinAngles, 50.0 )
                        << "  (<10deg: " << totalBelow10 << " = "
                        << ( totalTriangles > 0 ? 100.0 * totalBelow10 / totalTriangles : 0.0 )
                        << "%)" );
    BOOST_TEST_MESSAGE( "Total time (median-per-zone sum): " << totalTimeUs / 1000.0 << " ms" );

    if( baseline.valid )
        OutputComparisonReport( baseline, comparisons, totalTriangles, totalSpikeyTri, totalZones );
}


BOOST_AUTO_TEST_CASE( UpdateTriangulationStatus, * boost::unit_test::disabled() )
{
    std::string dataDir = GetTriangulationDataDir();

    if( !fs::exists( dataDir ) || fs::is_empty( dataDir ) )
    {
        BOOST_TEST_MESSAGE( "No triangulation data in " << dataDir << ", skipping" );
        return;
    }

    std::vector<fs::path> polyFiles;

    for( const auto& entry : fs::directory_iterator( dataDir ) )
    {
        if( entry.path().extension() == ".kicad_polys" )
            polyFiles.push_back( entry.path() );
    }

    std::sort( polyFiles.begin(), polyFiles.end() );

    int    totalTriangles = 0;
    int    totalSpikeyTri = 0;
    int    totalZones     = 0;
    double totalTimeUs    = 0.0;
    double totalArea      = 0.0;

    nlohmann::json boardsJson = nlohmann::json::array();

    for( const auto& polyFile : polyFiles )
    {
        BOARD_ENTRY board;

        if( !ParsePolyFile( polyFile, board ) )
            continue;

        nlohmann::json boardJson;
        boardJson["source"] = board.source;
        nlohmann::json zonesJson = nlohmann::json::array();

        int    boardTriangles = 0;
        int    boardSpikey    = 0;
        double boardTimeUs    = 0.0;

        for( ZONE_ENTRY& zone : board.zones )
        {
            ZONE_STATS stats = ComputeZoneStats( zone );
            zonesJson.push_back( ZoneStatsToJson( stats ) );

            boardTriangles += stats.triangleCount;
            boardSpikey    += stats.spikeyTriangles;
            boardTimeUs    += static_cast<double>( stats.timeUs );
            totalArea      += stats.triangulatedArea;
            totalZones++;
        }

        boardJson["zones"] = zonesJson;

        nlohmann::json boardTotals;
        boardTotals["triangle_count"] = boardTriangles;
        boardTotals["time_us"]        = static_cast<int64_t>( boardTimeUs );
        boardTotals["spikey_ratio"]   = boardTriangles > 0
                                                ? static_cast<double>( boardSpikey ) / boardTriangles
                                                : 0.0;
        boardJson["board_totals"] = boardTotals;
        boardsJson.push_back( boardJson );

        totalTriangles += boardTriangles;
        totalSpikeyTri += boardSpikey;
        totalTimeUs    += boardTimeUs;
    }

    nlohmann::json globalJson;
    globalJson["total_triangles"]        = totalTriangles;
    globalJson["total_time_us"]          = static_cast<int64_t>( totalTimeUs );
    globalJson["total_area_nm2"]         = totalArea;
    globalJson["total_spikey_triangles"] = totalSpikeyTri;
    globalJson["spikey_ratio"]           = totalTriangles > 0
                                                   ? static_cast<double>( totalSpikeyTri ) / totalTriangles
                                                   : 0.0;

    nlohmann::json metadataJson;
    metadataJson["board_count"] = static_cast<int>( polyFiles.size() );
    metadataJson["zone_count"]  = totalZones;

    nlohmann::json jsonOutput;
    jsonOutput["metadata"] = metadataJson;
    jsonOutput["global"]   = globalJson;
    jsonOutput["boards"]   = boardsJson;

    fs::path jsonPath = fs::path( dataDir ) / "triangulation_status.json";

    std::ofstream jsonFile( jsonPath );
    jsonFile << jsonOutput.dump( 2 ) << "\n";
    BOOST_CHECK( jsonFile.good() );
    jsonFile.close();

    BOOST_TEST_MESSAGE( "Wrote triangulation status to " << jsonPath );
    BOOST_TEST_MESSAGE( "Boards: " << polyFiles.size() << "  Zones: " << totalZones
                        << "  Triangles: " << totalTriangles );
}


BOOST_AUTO_TEST_SUITE_END()

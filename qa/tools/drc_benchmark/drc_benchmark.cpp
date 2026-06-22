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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * Standalone benchmark harness for the DRC rule compiler and the pcbexpr
 * expression evaluator. The two metrics of interest are InitEngine() time
 * (rule load + condition compile) and RunTests() time (the per-item evaluation
 * driven by the test providers). Geometric DRC cost is incidental here.
 *
 * The headline evaluator metric is eval_overhead_ms, the difference between the
 * check time with a rules file installed and the check time with no rules at
 * all on the same board and config. That isolates what the expression evaluator
 * adds on top of the fixed geometric work, which is exactly what the rule-engine
 * optimizer needs to watch.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/init.h>
#include <wx/log.h>

#include <core/profile.h>
#include <pgm_base.h>
#include <properties/property_mgr.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <thread_pool.h>
#include <wildcards_and_files_ext.h>

#include <board.h>
#include <board_design_settings.h>
#include <connectivity/connectivity_data.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <pcb_marker.h>
#include <project.h>
#include <widgets/progress_reporter_base.h>

#include <pcbnew_utils/board_file_utils.h>

#include "corpus.h"
#include "trace_capture.h"


namespace
{

/**
 * Minimal concrete PGM_BASE so SetPgm() has something to install before
 * InitPgm(). The global Pgm() singleton is otherwise null in a standalone tool
 * and every settings-manager access dereferences it.
 */
struct BENCH_PGM : public PGM_BASE
{
    void MacOpenFile( const wxString& aFileName ) override {}
};


BENCH_PGM g_program;


/// Which rules to compile and evaluate for a given run.
enum class RULES_VARIANT
{
    NONE,      ///< Empty wxFileName to InitEngine; baseline geometric check only.
    DEFAULT,   ///< The board's sidecar .kicad_dru (or an explicit --rules path).
    HEAVY      ///< A heavy sibling .kicad_dru that maximizes compile + evaluation cost.
};


/// Cold reloads the board between repeats; warm reuses the loaded board and engine.
enum class CACHE_MODE
{
    COLD,
    WARM
};


struct BENCH_CONFIG
{
    RULES_VARIANT rulesVariant = RULES_VARIANT::DEFAULT;
    CACHE_MODE    cache = CACHE_MODE::COLD;
    int           threads = 0;      ///< 0 means all hardware threads.
    int           repeat = 5;       ///< Timed repeats after a discarded warm-up.
};


/// One timed pass over a single board+config.
struct RUN_SAMPLE
{
    double                     compileMs = 0.0;
    double                     checkMs = 0.0;
    double                     cacheGenMs = 0.0;
    int                        violations = 0;
    std::map<wxString, double> providerMs;
    bool                       timedOut = false;   ///< Check phase hit the per-pass deadline.
    double                     fraction = 1.0;      ///< Completion fraction (0..1) when timed out.
};


/// Median plus median-absolute-deviation summary across the timed repeats.
struct STAT
{
    double median = 0.0;
    double mad = 0.0;
};


STAT computeStat( std::vector<double> aValues )
{
    STAT stat;

    if( aValues.empty() )
        return stat;

    std::sort( aValues.begin(), aValues.end() );

    size_t n = aValues.size();
    stat.median = ( n % 2 ) ? aValues[n / 2] : 0.5 * ( aValues[n / 2 - 1] + aValues[n / 2] );

    std::vector<double> devs;
    devs.reserve( n );

    for( double v : aValues )
        devs.push_back( std::fabs( v - stat.median ) );

    std::sort( devs.begin(), devs.end() );
    stat.mad = ( n % 2 ) ? devs[n / 2] : 0.5 * ( devs[n / 2 - 1] + devs[n / 2] );

    return stat;
}


/**
 * Read the 1-minute load average from /proc/loadavg. Returns a negative sentinel when the
 * file is unavailable so callers can treat the guard as disabled rather than aborting on a
 * platform that lacks it.
 */
double readOneMinuteLoad()
{
    std::ifstream in( "/proc/loadavg" );

    if( !in.is_open() )
        return -1.0;

    double load = -1.0;
    in >> load;

    return load;
}


/**
 * Resize the process thread pool for the requested config and re-cache the
 * shared pointer. Single-thread runs give deterministic per-provider attribution
 * because the providers stop interleaving across workers, which is why the sweep
 * pins it explicitly rather than trusting the default. A threads value of 0 maps
 * to the hardware concurrency the pool picks for itself.
 */
void applyThreadConfig( int aThreads )
{
    size_t n = aThreads > 0 ? static_cast<size_t>( aThreads ) : 0;

    Pgm().GetThreadPool().reset( n );

    // The cached GetKiCadThreadPool() pointer still aims at the same PGM-owned pool
    // object, but invalidating it keeps us honest with the documented contract and
    // forces a fresh fetch on the next provider call.
    InvalidateKiCadThreadPool();
}


/// Resolve the .kicad_dru path for a variant, or an empty name for NONE / a missing file.
wxFileName resolveRules( const wxFileName& aBoardName, const std::optional<wxString>& aDefaultRules,
                         const std::optional<wxString>& aHeavyRules, RULES_VARIANT aVariant )
{
    if( aVariant == RULES_VARIANT::NONE )
        return wxFileName();

    if( aVariant == RULES_VARIANT::HEAVY )
    {
        if( aHeavyRules )
            return wxFileName( *aHeavyRules );

        return wxFileName();
    }

    if( aDefaultRules )
        return wxFileName( *aDefaultRules );

    wxFileName sidecar( aBoardName );
    sidecar.SetExt( FILEEXT::DesignRulesFileExtension );

    if( sidecar.Exists() )
        return sidecar;

    return wxFileName();
}


/**
 * Load a board (and any sibling project) the same way the QA board test utilities
 * do. Connectivity build ordering is load-bearing because RunTests() relies on a
 * built connectivity graph.
 */
std::unique_ptr<BOARD> loadBoard( const wxFileName& aBoardName, SETTINGS_MANAGER& aManager,
                                  const wxFileName& aProjectName )
{
    std::unique_ptr<BOARD> board;

    try
    {
        board = KI_TEST::ReadBoardFromFileOrStream(
                std::string( aBoardName.GetFullPath().ToUTF8() ) );
    }
    catch( const IO_ERROR& ioe )
    {
        std::printf( "error loading board: %s\n", TO_UTF8( ioe.What() ) );
        return nullptr;
    }

    if( !board )
    {
        std::printf( "error: board failed to load\n" );
        return nullptr;
    }

    if( aProjectName.Exists() )
        board->SetProject( &aManager.Prj() );

    board->BuildListOfNets();
    board->BuildConnectivity();
    board->GetLengthCalculation()->SynchronizeTuningProfileProperties();

    if( board->GetProject() )
    {
        std::unordered_set<wxString> dummy;
        board->SynchronizeComponentClasses( dummy );
    }

    return board;
}


/**
 * Provider name to the DRC error codes it can report, taken directly from the DRCE_* uses in
 * each pcbnew/drc/drc_test_provider_*.cpp. --isolate uses this to leave only the target
 * provider's codes active so every other provider self-skips on its raised error limit, which
 * attributes the evaluation cost to the one provider under study. A provider absent from this
 * table cannot be isolated precisely, so --isolate reports that and runs the full set.
 */
const std::map<wxString, std::vector<int>>& providerErrorCodes()
{
    static const std::map<wxString, std::vector<int>> codes = {
        { wxT( "annular_width" ),          { DRCE_ANNULAR_WIDTH } },
        { wxT( "copper width" ),           { DRCE_CONNECTION_WIDTH } },
        { wxT( "connectivity" ),           { DRCE_DANGLING_TRACK, DRCE_DANGLING_VIA,
                                             DRCE_ISOLATED_COPPER,
                                             DRCE_TRACK_NOT_CENTERED_ON_VIA,
                                             DRCE_TRACK_ON_POST_MACHINED_LAYER,
                                             DRCE_UNCONNECTED_ITEMS } },
        { wxT( "clearance" ),              { DRCE_CLEARANCE, DRCE_HOLE_CLEARANCE,
                                             DRCE_SHORTING_ITEMS, DRCE_TRACKS_CROSSING,
                                             DRCE_ZONES_INTERSECT } },
        { wxT( "courtyard_clearance" ),    { DRCE_MALFORMED_COURTYARD, DRCE_MISSING_COURTYARD,
                                             DRCE_NPTH_IN_COURTYARD, DRCE_OVERLAPPING_FOOTPRINTS,
                                             DRCE_PTH_IN_COURTYARD } },
        { wxT( "creepage" ),               { DRCE_CREEPAGE } },
        { wxT( "diff_pair_coupling" ),     { DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE,
                                             DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG } },
        { wxT( "disallow" ),               { DRCE_ALLOWED_ITEMS, DRCE_TEXT_ON_EDGECUTS } },
        { wxT( "edge_clearance" ),         { DRCE_EDGE_CLEARANCE, DRCE_SILK_EDGE_CLEARANCE } },
        { wxT( "footprint checks" ),       { DRCE_FOOTPRINT_TYPE_MISMATCH, DRCE_PADSTACK,
                                             DRCE_PAD_TH_WITH_NO_HOLE, DRCE_SHORTING_ITEMS } },
        { wxT( "hole_size" ),              { DRCE_DRILL_OUT_OF_RANGE,
                                             DRCE_MICROVIA_DRILL_OUT_OF_RANGE, DRCE_PADSTACK } },
        { wxT( "hole_to_hole_clearance" ), { DRCE_DRILLED_HOLES_COLOCATED,
                                             DRCE_DRILLED_HOLES_TOO_CLOSE } },
        { wxT( "length" ),                 { DRCE_LENGTH_OUT_OF_RANGE,
                                             DRCE_NET_CHAIN_RETURN_PATH_BREAK,
                                             DRCE_NET_CHAIN_STUB_TOO_LONG, DRCE_SKEW_OUT_OF_RANGE,
                                             DRCE_VIA_COUNT_OUT_OF_RANGE } },
        { wxT( "physical_clearance" ),     { DRCE_CLEARANCE, DRCE_HOLE_CLEARANCE } },
        { wxT( "silk_clearance" ),         { DRCE_SILK_CLEARANCE, DRCE_SILK_MASK_CLEARANCE } },
        { wxT( "sliver checker" ),         { DRCE_COPPER_SLIVER } },
        { wxT( "solder_mask_issues" ),     { DRCE_SILK_MASK_CLEARANCE, DRCE_SOLDERMASK_BRIDGE } },
        { wxT( "text_dimensions" ),        { DRCE_TEXT_HEIGHT, DRCE_TEXT_THICKNESS } },
        { wxT( "text_mirroring" ),         { DRCE_MIRRORED_TEXT_ON_FRONT_LAYER,
                                             DRCE_NONMIRRORED_TEXT_ON_BACK_LAYER } },
        { wxT( "angle" ),                  { DRCE_TRACK_ANGLE } },
        { wxT( "segment_length" ),         { DRCE_TRACK_SEGMENT_LENGTH } },
        { wxT( "width" ),                  { DRCE_TRACK_WIDTH } },
        { wxT( "diameter" ),               { DRCE_VIA_DIAMETER } },
        { wxT( "zone connections" ),       { DRCE_STARVED_THERMAL } }
    };

    return codes;
}


/**
 * Leave only the named provider's error codes active and ignore everything else so the other
 * providers self-skip on their raised limits. Returns false when the provider has no entry in
 * the code table, in which case the caller should run the unmodified severity set.
 */
bool applyIsolate( BOARD* aBoard, const wxString& aProvider )
{
    auto it = providerErrorCodes().find( aProvider );

    if( it == providerErrorCodes().end() )
        return false;

    std::set<int> keep( it->second.begin(), it->second.end() );

    BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

    for( int code = DRCE_FIRST; code <= DRCE_LAST; ++code )
    {
        if( !keep.count( code ) )
            bds.m_DRCSeverities[code] = RPT_SEVERITY_IGNORE;
    }

    return true;
}


/// Time a single InitEngine() (rule load + condition compile) on an already-loaded board.
/**
 * Headless progress reporter that cancels a DRC check once a wall-clock deadline passes.
 * The engine polls KeepRefreshing()/IsCancelled() from its providers, so cancellation is
 * cooperative and lands at the next poll rather than instantly, which is why a single very
 * slow provider can overshoot the deadline by its own polling interval. updateUI() runs from
 * worker threads as well as the main thread, so it touches only atomics. The completion
 * fraction is not derived here; the caller computes it from the count of providers that
 * finished, which is a more meaningful "how far did it get" than KiCad's per-phase counters.
 */
class BENCH_PROGRESS : public PROGRESS_REPORTER_BASE
{
public:
    explicit BENCH_PROGRESS( double aTimeoutSec ) :
            PROGRESS_REPORTER_BASE( 1 ),
            m_enabled( aTimeoutSec > 0.0 ),
            m_deadline( std::chrono::steady_clock::now()
                        + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                std::chrono::duration<double>( aTimeoutSec ) ) )
    {
    }

    bool TimedOut() const { return m_timedOut.load(); }

protected:
    bool updateUI() override
    {
        if( m_enabled && std::chrono::steady_clock::now() >= m_deadline )
        {
            m_timedOut.store( true );
            m_cancelled.store( true );

            return false;
        }

        return true;
    }

private:
    bool                                  m_enabled;
    std::chrono::steady_clock::time_point m_deadline;
    std::atomic_bool                      m_timedOut{ false };
};


double timeCompile( BOARD* aBoard, const wxFileName& aRulesFile )
{
    std::shared_ptr<DRC_ENGINE> engine =
            std::make_shared<DRC_ENGINE>( aBoard, &aBoard->GetDesignSettings() );

    aBoard->GetDesignSettings().m_DRCEngine = engine;

    PROF_TIMER timer;
    engine->InitEngine( aRulesFile );
    timer.Stop();

    return timer.msecs();
}


/**
 * Run a full compile + check pass and scrape the per-provider timings. The engine
 * is rebuilt every call so each pass starts from the same compile state.
 */
RUN_SAMPLE timeRun( BOARD* aBoard, const wxFileName& aRulesFile, double aTimeoutSec )
{
    RUN_SAMPLE sample;

    std::shared_ptr<DRC_ENGINE> engine =
            std::make_shared<DRC_ENGINE>( aBoard, &aBoard->GetDesignSettings() );

    aBoard->GetDesignSettings().m_DRCEngine = engine;

    std::atomic<int> violationCount{ 0 };

    engine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aCreateMarker )
            {
                violationCount.fetch_add( 1, std::memory_order_relaxed );
            } );

    PROF_TIMER compileTimer;
    engine->InitEngine( aRulesFile );
    compileTimer.Stop();
    sample.compileMs = compileTimer.msecs();

    // Total provider count anchors the "% complete" reported on a timeout. The profile log
    // records one entry per provider that finished, so completed/total is how far the check got.
    size_t totalProviders = engine->GetTestProviders().size();

    // Install the profile-trace scraper for the duration of RunTests so the engine's
    // per-provider "DRC provider ... took" lines land in our maps. The previous target is
    // chained, so ordinary logging still reaches the console.
    wxLog::AddTraceMask( wxT( "KICAD_DRC_PROFILE" ) );

    DRC_PROFILE_LOG* profileLog = new DRC_PROFILE_LOG();
    wxLog*           prevTarget = wxLog::SetActiveTarget( profileLog );

    // Deadline starts at the check phase so a slow rule compile spends its own time without
    // eating the evaluator's budget. InitEngine is not cancellable, so the timeout only bounds
    // RunTests, which is the long pole this tool exists to measure.
    BENCH_PROGRESS progress( aTimeoutSec );
    engine->SetProgressReporter( &progress );

    PROF_TIMER checkTimer;

    try
    {
        engine->RunTests( EDA_UNITS::MM, true, false );
    }
    catch( const std::exception& e )
    {
        std::printf( "error during RunTests: %s\n", e.what() );
    }

    checkTimer.Stop();

    engine->SetProgressReporter( nullptr );
    wxLog::SetActiveTarget( prevTarget );

    sample.providerMs = profileLog->ProviderMs();

    sample.timedOut = progress.TimedOut();

    if( sample.timedOut )
        sample.fraction = totalProviders > 0
                                  ? static_cast<double>( sample.providerMs.size() )
                                            / static_cast<double>( totalProviders )
                                  : 0.0;

    // Prefer the engine's own "DRC took" total as the check denominator since it brackets
    // the same span the provider rows live in. Fall back to our outer timer if the trace
    // line did not arrive (e.g. RunTests bailed early).
    double engineTotal = profileLog->TotalMs();

    sample.checkMs = engineTotal > 0.0 ? engineTotal : checkTimer.msecs();

    double providerSum = 0.0;

    for( const auto& [name, ms] : sample.providerMs )
        providerSum += ms;

    // Cache generation has no trace line of its own; it is everything the check spent that
    // is not attributable to a provider. Clamp at zero to absorb timer jitter.
    sample.cacheGenMs = std::max( 0.0, sample.checkMs - providerSum );

    sample.violations = violationCount.load( std::memory_order_relaxed );

    delete profileLog;

    return sample;
}


/// Aggregated result for one board+config sweep cell.
struct SWEEP_RESULT
{
    BENCH_CONFIG                config;
    bool                        ran = false;
    bool                        underLoad = false;
    STAT                        compile;
    STAT                        check;
    STAT                        cacheGen;
    int                         violations = 0;
    bool                        timedOut = false;   ///< A timed pass hit the deadline; repeats aborted.
    double                      fraction = 1.0;      ///< Completion fraction (0..1) at the timeout.
    std::map<wxString, STAT>    providerStats;   ///< Per-provider median+MAD over the repeats.
};


/**
 * Time one (board, config) cell. Cold reloads the board before every timed pass so
 * file-system and connectivity caches are rebuilt; warm reuses a single loaded board
 * across passes. The first pass is always discarded as a warm-up.
 *
 * The three leading wxFileName arguments are board, project and rules in that fixed
 * order; they are distinct files and only ever passed positionally from one call site.
 */
SWEEP_RESULT runConfig( const wxFileName& aBoardName, SETTINGS_MANAGER& aManager,
                        const wxFileName& aProjectName, const wxFileName& aRulesFile,
                        const BENCH_CONFIG& aConfig, const std::optional<wxString>& aIsolate,
                        double aMaxLoad, double aTimeoutSec )
{
    SWEEP_RESULT result;
    result.config = aConfig;

    applyThreadConfig( aConfig.threads );

    std::vector<double>                     compileSamples;
    std::vector<double>                     checkSamples;
    std::vector<double>                     cacheSamples;
    std::map<wxString, std::vector<double>> providerSamples;

    std::unique_ptr<BOARD> warmBoard;

    if( aConfig.cache == CACHE_MODE::WARM )
    {
        warmBoard = loadBoard( aBoardName, aManager, aProjectName );

        if( !warmBoard )
            return result;

        if( aIsolate )
            applyIsolate( warmBoard.get(), *aIsolate );
    }

    for( int i = 0; i <= aConfig.repeat; ++i )
    {
        BOARD* board = warmBoard.get();

        std::unique_ptr<BOARD> coldBoard;

        if( aConfig.cache == CACHE_MODE::COLD )
        {
            coldBoard = loadBoard( aBoardName, aManager, aProjectName );

            if( !coldBoard )
                return result;

            if( aIsolate )
                applyIsolate( coldBoard.get(), *aIsolate );

            board = coldBoard.get();
        }

        // Re-check load right before a timed pass so a spike that arrives mid-sweep taints
        // only the rows it actually touched instead of silently corrupting the medians.
        if( aMaxLoad > 0.0 && i > 0 )
        {
            double load = readOneMinuteLoad();

            if( load > aMaxLoad )
                result.underLoad = true;
        }

        RUN_SAMPLE sample = timeRun( board, aRulesFile, aTimeoutSec );

        // A timeout means this cell is too slow to be worth repeating; bail immediately so a
        // single deadline does not cost timeout x repeat. Keep the compile number (it finished
        // before the check deadline) so the row still reports useful compiler data.
        if( sample.timedOut )
        {
            result.timedOut = true;
            result.fraction = sample.fraction;

            if( compileSamples.empty() )
                compileSamples.push_back( sample.compileMs );

            break;
        }

        // The first pass primes allocators and any lazily built board state, so discard it.
        if( i > 0 )
        {
            compileSamples.push_back( sample.compileMs );
            checkSamples.push_back( sample.checkMs );
            cacheSamples.push_back( sample.cacheGenMs );
            result.violations = sample.violations;

            for( const auto& [name, ms] : sample.providerMs )
                providerSamples[name].push_back( ms );
        }
    }

    if( warmBoard )
        warmBoard->SetProject( nullptr );

    result.ran = true;
    result.compile = computeStat( compileSamples );
    result.check = computeStat( checkSamples );
    result.cacheGen = computeStat( cacheSamples );

    for( auto& [name, samples] : providerSamples )
        result.providerStats[name] = computeStat( samples );

    return result;
}


/**
 * Pure compiler benchmark. Repeatedly time only InitEngine() with no board checks
 * so the rule loader and libeval condition compiler are isolated from evaluation.
 *
 * Board, project and rules wxFileNames are positional and distinct; only one call site
 * supplies them so the same-type ordering cannot be confused.
 */
STAT runCompileOnly( const wxFileName& aBoardName, SETTINGS_MANAGER& aManager,
                     const wxFileName& aProjectName, const wxFileName& aRulesFile, int aRepeat )
{
    std::unique_ptr<BOARD> board = loadBoard( aBoardName, aManager, aProjectName );

    if( !board )
        return STAT();

    std::vector<double> samples;

    for( int i = 0; i <= aRepeat; ++i )
    {
        double ms = timeCompile( board.get(), aRulesFile );

        if( i > 0 )
            samples.push_back( ms );
    }

    board->SetProject( nullptr );

    return computeStat( samples );
}


const char* variantName( RULES_VARIANT aVariant )
{
    switch( aVariant )
    {
    case RULES_VARIANT::NONE:    return "none";
    case RULES_VARIANT::DEFAULT: return "default";
    case RULES_VARIANT::HEAVY:   return "heavy";
    }

    return "?";
}


bool parseVariant( const wxString& aArg, RULES_VARIANT& aVariant )
{
    if( aArg == wxT( "none" ) )
        aVariant = RULES_VARIANT::NONE;
    else if( aArg == wxT( "default" ) )
        aVariant = RULES_VARIANT::DEFAULT;
    else if( aArg == wxT( "heavy" ) )
        aVariant = RULES_VARIANT::HEAVY;
    else
        return false;

    return true;
}


/// Read a whole file into a wxString, returning empty when it cannot be opened.
wxString slurp( const wxFileName& aFile )
{
    std::ifstream in( aFile.GetFullPath().fn_str() );

    if( !in.is_open() )
        return wxEmptyString;

    std::stringstream buffer;
    buffer << in.rdbuf();

    return wxString::FromUTF8( buffer.str().c_str() );
}


/// JSON string escaper for the hand-written output below; no JSON dependency at the call sites.
std::string jsonEscape( const wxString& aStr )
{
    std::string utf8( aStr.utf8_str() );
    std::string out;
    out.reserve( utf8.size() + 8 );

    for( char c : utf8 )
    {
        switch( c )
        {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:   out += c; break;
        }
    }

    return out;
}


/// One coverage cell for a single board, recording which constraints and predicates it exercises.
struct COVERAGE_ROW
{
    wxString                board;
    std::set<DRC_CONSTRAINT_T> constraints;
    std::set<wxString>      predicates;
};


/**
 * Probe which constraint types the loaded engine carries rules for and which predicates the raw
 * rules text invokes. The constraint probe is authoritative because it asks the live engine; the
 * predicate probe is textual because predicates live inside condition strings, not in the engine
 * constraint map.
 *
 * Board, project and rules wxFileNames are positional and distinct, supplied from a single
 * call site, so the shared parameter type cannot lead to a swap.
 */
COVERAGE_ROW collectCoverage( const wxFileName& aBoardName, SETTINGS_MANAGER& aManager,
                              const wxFileName& aProjectName, const wxFileName& aRulesFile )
{
    COVERAGE_ROW row;
    row.board = aBoardName.GetFullName();

    std::unique_ptr<BOARD> board = loadBoard( aBoardName, aManager, aProjectName );

    if( !board )
        return row;

    std::shared_ptr<DRC_ENGINE> engine =
            std::make_shared<DRC_ENGINE>( board.get(), &board->GetDesignSettings() );

    board->GetDesignSettings().m_DRCEngine = engine;
    engine->InitEngine( aRulesFile );

    for( DRC_CONSTRAINT_T type : AllConstraintTypes() )
    {
        if( engine->HasRulesForConstraintType( type ) )
            row.constraints.insert( type );
    }

    if( aRulesFile.IsOk() && aRulesFile.Exists() )
    {
        for( const wxString& pred : ScanPredicatesInRules( slurp( aRulesFile ) ) )
            row.predicates.insert( pred );
    }

    board->SetProject( nullptr );

    return row;
}


/**
 * Print the coverage matrix to stdout and write coverage.json. The explicit UNCOVERED lines
 * drive corpus authoring until every constraint and watched predicate is exercised somewhere.
 */
void emitCoverage( const std::vector<COVERAGE_ROW>& aRows, const wxString& aOutDir )
{
    std::set<DRC_CONSTRAINT_T> coveredConstraints;
    std::set<wxString>         coveredPredicates;

    for( const COVERAGE_ROW& row : aRows )
    {
        coveredConstraints.insert( row.constraints.begin(), row.constraints.end() );
        coveredPredicates.insert( row.predicates.begin(), row.predicates.end() );
    }

    std::printf( "=== coverage matrix ===\n" );
    std::printf( "%-28s %s\n", "board", "constraints / predicates" );
    std::printf( "%-28s %s\n", "----------------------------",
                 "-------------------------------------" );

    for( const COVERAGE_ROW& row : aRows )
    {
        std::string cons;

        for( DRC_CONSTRAINT_T type : row.constraints )
        {
            if( !cons.empty() )
                cons += ",";

            cons += ConstraintTypeName( type );
        }

        std::string preds;

        for( const wxString& pred : row.predicates )
        {
            if( !preds.empty() )
                preds += ",";

            preds += std::string( pred.utf8_str() );
        }

        std::printf( "%-28s C[%s] P[%s]\n",
                     static_cast<const char*>( row.board.utf8_str() ), cons.c_str(),
                     preds.c_str() );
    }

    std::vector<const char*> uncoveredConstraints;

    for( DRC_CONSTRAINT_T type : AllConstraintTypes() )
    {
        if( !coveredConstraints.count( type ) )
            uncoveredConstraints.push_back( ConstraintTypeName( type ) );
    }

    std::vector<wxString> uncoveredPredicates;

    for( const wxString& pred : AllPredicateNames() )
    {
        if( !coveredPredicates.count( pred ) )
            uncoveredPredicates.push_back( pred );
    }

    std::printf( "\nUNCOVERED constraints:" );

    for( const char* name : uncoveredConstraints )
        std::printf( " %s", name );

    std::printf( "%s\n", uncoveredConstraints.empty() ? " (none)" : "" );

    std::printf( "UNCOVERED predicates:" );

    for( const wxString& pred : uncoveredPredicates )
        std::printf( " %s", static_cast<const char*>( pred.utf8_str() ) );

    std::printf( "%s\n\n", uncoveredPredicates.empty() ? " (none)" : "" );

    wxFileName outFile( aOutDir, wxT( "coverage.json" ) );
    std::ofstream out( outFile.GetFullPath().fn_str() );

    if( !out.is_open() )
        return;

    out << "{\n  \"boards\": [\n";

    for( size_t i = 0; i < aRows.size(); ++i )
    {
        const COVERAGE_ROW& row = aRows[i];

        out << "    {\n      \"board\": \"" << jsonEscape( row.board ) << "\",\n";
        out << "      \"constraints\": [";

        bool first = true;

        for( DRC_CONSTRAINT_T type : row.constraints )
        {
            out << ( first ? "" : ", " ) << "\"" << ConstraintTypeName( type ) << "\"";
            first = false;
        }

        out << "],\n      \"predicates\": [";

        first = true;

        for( const wxString& pred : row.predicates )
        {
            out << ( first ? "" : ", " ) << "\"" << jsonEscape( pred ) << "\"";
            first = false;
        }

        out << "]\n    }" << ( i + 1 < aRows.size() ? "," : "" ) << "\n";
    }

    out << "  ],\n  \"uncovered_constraints\": [";

    for( size_t i = 0; i < uncoveredConstraints.size(); ++i )
        out << ( i ? ", " : "" ) << "\"" << uncoveredConstraints[i] << "\"";

    out << "],\n  \"uncovered_predicates\": [";

    for( size_t i = 0; i < uncoveredPredicates.size(); ++i )
        out << ( i ? ", " : "" ) << "\"" << jsonEscape( uncoveredPredicates[i] ) << "\"";

    out << "]\n}\n";
}


/// A fully aggregated row destined for results.json and the worst-offender rankings.
struct RESULT_ROW
{
    wxString                 board;
    wxString                 config;
    STAT                     compile;
    STAT                     cacheGen;
    STAT                     check;
    double                   evalOverheadMs = 0.0;
    bool                     evalOverheadValid = false;
    int                      violations = 0;
    bool                     underLoad = false;
    bool                     timedOut = false;
    double                   fraction = 1.0;
    std::map<wxString, STAT> perProvider;
};


void writeResultsJson( const std::vector<RESULT_ROW>& aRows, const wxString& aOutDir )
{
    wxFileName outFile( aOutDir, wxT( "results.json" ) );
    std::ofstream out( outFile.GetFullPath().fn_str() );

    if( !out.is_open() )
        return;

    out << "[\n";

    for( size_t i = 0; i < aRows.size(); ++i )
    {
        const RESULT_ROW& row = aRows[i];

        out << "  {\n";
        out << "    \"board\": \"" << jsonEscape( row.board ) << "\",\n";
        out << "    \"config\": \"" << jsonEscape( row.config ) << "\",\n";
        out << "    \"compile_ms\": " << row.compile.median << ",\n";
        out << "    \"compile_mad\": " << row.compile.mad << ",\n";
        out << "    \"cache_gen_ms\": " << row.cacheGen.median << ",\n";
        out << "    \"cache_gen_mad\": " << row.cacheGen.mad << ",\n";
        out << "    \"check_ms\": " << row.check.median << ",\n";
        out << "    \"check_mad\": " << row.check.mad << ",\n";

        if( row.evalOverheadValid )
            out << "    \"eval_overhead_ms\": " << row.evalOverheadMs << ",\n";
        else
            out << "    \"eval_overhead_ms\": null,\n";

        out << "    \"n_violations\": " << row.violations << ",\n";
        out << "    \"under_load\": " << ( row.underLoad ? "true" : "false" ) << ",\n";
        out << "    \"timed_out\": " << ( row.timedOut ? "true" : "false" ) << ",\n";
        out << "    \"percent_complete\": " << ( row.timedOut ? row.fraction * 100.0 : 100.0 )
            << ",\n";
        out << "    \"per_provider\": {";

        bool first = true;

        for( const auto& [name, stat] : row.perProvider )
        {
            out << ( first ? "\n" : ",\n" );
            out << "      \"" << jsonEscape( name ) << "\": { \"median_ms\": " << stat.median
                << ", \"mad_ms\": " << stat.mad << " }";
            first = false;
        }

        out << ( first ? "}" : "\n    }" ) << "\n";
        out << "  }" << ( i + 1 < aRows.size() ? "," : "" ) << "\n";
    }

    out << "]\n";
}


/**
 * Write the two worst-offender rankings the optimizer watches. The compile list ranks the
 * heaviest rule compiles by compile_ms; the eval list ranks the heaviest rule evaluations by
 * eval_overhead_ms, which is the evaluator cost net of the rules-free geometric baseline.
 */
void writeWorstOffenders( const std::vector<RESULT_ROW>& aRows, const wxString& aOutDir, int aTopN )
{
    std::vector<const RESULT_ROW*> byCompile;
    std::vector<const RESULT_ROW*> byEval;
    std::vector<const RESULT_ROW*> timedOut;

    for( const RESULT_ROW& row : aRows )
    {
        byCompile.push_back( &row );

        if( row.evalOverheadValid )
            byEval.push_back( &row );

        if( row.timedOut )
            timedOut.push_back( &row );
    }

    std::sort( byCompile.begin(), byCompile.end(),
               []( const RESULT_ROW* a, const RESULT_ROW* b )
               {
                   return a->compile.median > b->compile.median;
               } );

    std::sort( byEval.begin(), byEval.end(),
               []( const RESULT_ROW* a, const RESULT_ROW* b )
               {
                   return a->evalOverheadMs > b->evalOverheadMs;
               } );

    // Least-complete first: a cell that only reached 10% before the deadline is a worse
    // offender than one that reached 90%.
    std::sort( timedOut.begin(), timedOut.end(),
               []( const RESULT_ROW* a, const RESULT_ROW* b )
               {
                   return a->fraction < b->fraction;
               } );

    auto emitList = [&]( const char* aLabel )
    {
        std::printf( "=== worst offenders by %s ===\n", aLabel );
        std::printf( "%-28s %-22s %12s\n", "board", "config", aLabel );
        std::printf( "%-28s %-22s %12s\n", "----------------------------",
                     "----------------------", "------------" );
    };

    emitList( "compile_ms" );

    for( int i = 0; i < aTopN && i < static_cast<int>( byCompile.size() ); ++i )
    {
        const RESULT_ROW* row = byCompile[i];

        std::printf( "%-28s %-22s %12.3f\n", static_cast<const char*>( row->board.utf8_str() ),
                     static_cast<const char*>( row->config.utf8_str() ), row->compile.median );
    }

    std::printf( "\n" );
    emitList( "eval_overhead_ms" );

    for( int i = 0; i < aTopN && i < static_cast<int>( byEval.size() ); ++i )
    {
        const RESULT_ROW* row = byEval[i];

        std::printf( "%-28s %-22s %12.3f\n", static_cast<const char*>( row->board.utf8_str() ),
                     static_cast<const char*>( row->config.utf8_str() ), row->evalOverheadMs );
    }

    std::printf( "\n" );

    if( !timedOut.empty() )
    {
        std::printf( "=== timed out (eval unbounded; ranked least-complete first) ===\n" );
        std::printf( "%-28s %-22s %12s\n", "board", "config", "percent" );
        std::printf( "%-28s %-22s %12s\n", "----------------------------",
                     "----------------------", "------------" );

        for( const RESULT_ROW* row : timedOut )
        {
            std::printf( "%-28s %-22s %11.1f%%\n",
                         static_cast<const char*>( row->board.utf8_str() ),
                         static_cast<const char*>( row->config.utf8_str() ), row->fraction * 100.0 );
        }

        std::printf( "\n" );
    }

    wxFileName outFile( aOutDir, wxT( "worst_offenders.json" ) );
    std::ofstream out( outFile.GetFullPath().fn_str() );

    if( !out.is_open() )
        return;

    auto writeRanked = [&]( const char* aKey, const std::vector<const RESULT_ROW*>& aList,
                            bool aUseEval )
    {
        out << "  \"" << aKey << "\": [\n";

        int count = std::min<int>( aTopN, static_cast<int>( aList.size() ) );

        for( int i = 0; i < count; ++i )
        {
            const RESULT_ROW* row = aList[i];
            double            value = aUseEval ? row->evalOverheadMs : row->compile.median;

            out << "    { \"board\": \"" << jsonEscape( row->board ) << "\", \"config\": \""
                << jsonEscape( row->config ) << "\", \"" << ( aUseEval ? "eval_overhead_ms"
                                                                       : "compile_ms" )
                << "\": " << value << " }" << ( i + 1 < count ? "," : "" ) << "\n";
        }

        out << "  ]";
    };

    out << "{\n";
    writeRanked( "by_compile_ms", byCompile, false );
    out << ",\n";
    writeRanked( "by_eval_overhead_ms", byEval, true );
    out << ",\n";

    out << "  \"timed_out\": [\n";

    for( size_t i = 0; i < timedOut.size(); ++i )
    {
        const RESULT_ROW* row = timedOut[i];

        out << "    { \"board\": \"" << jsonEscape( row->board ) << "\", \"config\": \""
            << jsonEscape( row->config ) << "\", \"percent_complete\": " << row->fraction * 100.0
            << " }" << ( i + 1 < timedOut.size() ? "," : "" ) << "\n";
    }

    out << "  ]\n}\n";
}


/// Compact tag describing a config cell, used as the config column and the JSON config key.
wxString configTag( CACHE_MODE aCache, RULES_VARIANT aVariant, int aThreads )
{
    return wxString::Format( wxT( "%s/%s/t%d" ), aCache == CACHE_MODE::COLD ? "cold" : "warm",
                             variantName( aVariant ), aThreads );
}

} // namespace


int main( int argc, char** argv )
{
    wxInitialize( argc, argv );

    // Force the C locale so numeric parsing of the trace lines stays on a dot decimal
    // separator regardless of the environment.
    std::setlocale( LC_ALL, "C" );

    // Line-buffer stdout so per-board progress streams to a redirected log during a long sweep
    // instead of materializing only at exit.
    std::setvbuf( stdout, nullptr, _IOLBF, 0 );

    // The self-check exercises the trace parser alone and needs no engine, project, or
    // board, so handle it before any heavier initialization.
    for( int i = 1; i < argc; ++i )
    {
        if( std::string( argv[i] ) == "--selftest" )
        {
            int rv = RunTraceCaptureSelftest();
            wxUninitialize();

            return rv;
        }
    }

    SetPgm( &g_program );
    Pgm().InitPgm( true, true );

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    static const wxCmdLineEntryDesc cmdLineDesc[] = {
        { wxCMD_LINE_SWITCH, nullptr, "selftest", "run the trace-parser self-check and exit",
          wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "board", "ad-hoc board override (skips the corpus manifest)",
          wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, "r", "rules", "default-variant design rules file (.kicad_dru)",
          wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "heavy-rules", "heavy-variant design rules file (.kicad_dru)",
          wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "rules-variant",
          "none|default|heavy (default: sweep all three)", wxCMD_LINE_VAL_STRING,
          wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_SWITCH, nullptr, "rules-only",
          "time only InitEngine() compile in a loop, no checks", wxCMD_LINE_VAL_NONE,
          wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "threads", "worker threads, 0=all (default 0)",
          wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "repeat", "timed repeats after warm-up (default 5)",
          wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "cache", "cold|warm|both (default both)",
          wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "isolate",
          "ignore every provider but this one to attribute its eval cost",
          wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "top-n", "worst-offender list length (default 10)",
          wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "out", "output directory for JSON artifacts (default cwd)",
          wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "max-load",
          "abort when 1-min loadavg exceeds this (default ncores*0.5)", wxCMD_LINE_VAL_STRING,
          wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "timeout",
          "per-test deadline in seconds; 0 disables (default 60)", wxCMD_LINE_VAL_NUMBER,
          wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_SWITCH, nullptr, "quick",
          "fast iteration set: small/synthetic boards, warm none+heavy, repeat 3",
          wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_OPTION, nullptr, "quick-max-mb",
          "in --quick, also keep real boards smaller than this many MB (default 0 = synthetic only)",
          wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_PARAM, nullptr, nullptr, "board.kicad_pcb", wxCMD_LINE_VAL_STRING,
          wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_NONE }
    };

    wxCmdLineParser parser( argc, argv );
    parser.SetDesc( cmdLineDesc );
    parser.SetLogo( "qa_drc_benchmark: time DRC rule compile + expression evaluation" );

    if( parser.Parse() != 0 )
    {
        Pgm().Destroy();
        wxUninitialize();
        return 1;
    }

    SETTINGS_MANAGER& manager = Pgm().GetSettingsManager();

    auto cleanupExit = [&]( int aCode )
    {
        Pgm().Destroy();
        wxUninitialize();
        return aCode;
    };

    // Resolve which boards to run. An explicit --board or a positional board is an ad-hoc run;
    // otherwise the corpus manifest drives the sweep. An unconfigured corpus with no ad-hoc
    // board is a clean skip, never a failure.
    std::vector<CORPUS_ENTRY> entries;

    wxString adHocBoard;
    bool     haveAdHocBoard = parser.Found( "board", &adHocBoard );

    if( !haveAdHocBoard && parser.GetParamCount() > 0 )
    {
        adHocBoard = parser.GetParam( 0 );
        haveAdHocBoard = true;
    }

    std::optional<wxString> cliDefaultRules;
    wxString                rulesArg;

    if( parser.Found( "r", &rulesArg ) )
        cliDefaultRules = rulesArg;

    std::optional<wxString> cliHeavyRules;
    wxString                heavyArg;

    if( parser.Found( "heavy-rules", &heavyArg ) )
        cliHeavyRules = heavyArg;

    if( haveAdHocBoard )
    {
        CORPUS_ENTRY entry;
        wxFileName   board( adHocBoard );
        board.MakeAbsolute();
        entry.board = board.GetFullPath();

        if( cliDefaultRules )
        {
            wxFileName rules( *cliDefaultRules );
            rules.MakeAbsolute();
            entry.rules = rules.GetFullPath();
        }

        entry.tier = wxT( "adhoc" );
        entries.push_back( entry );
    }
    else
    {
        if( !CORPUS::IsConfigured() )
        {
            std::printf( "KICAD_DRC_BENCH_CORPUS is unset or does not name a directory; "
                         "nothing to benchmark.\n"
                         "Set it to a corpus root containing corpus.json, or pass a board "
                         "with --board <file.kicad_pcb>. Skipping.\n" );
            return cleanupExit( 0 );
        }

        wxString loadError;

        if( !CORPUS::Load( entries, loadError ) )
        {
            std::printf( "error loading corpus: %s\n",
                         static_cast<const char*>( loadError.utf8_str() ) );
            return cleanupExit( 1 );
        }

        if( entries.empty() )
        {
            std::printf( "corpus at %s has no entries; nothing to benchmark.\n",
                         static_cast<const char*>( CORPUS::Root().utf8_str() ) );
            return cleanupExit( 0 );
        }
    }

    long threadsArg = 0;
    int  threads = 0;

    if( parser.Found( "threads", &threadsArg ) )
        threads = static_cast<int>( threadsArg );

    bool quick = parser.Found( "quick" );

    double timeoutSec = 60.0;
    long   timeoutArg = 60;

    if( parser.Found( "timeout", &timeoutArg ) )
        timeoutSec = static_cast<double>( std::max( 0L, timeoutArg ) );

    // Default 0 keeps the quick set to the curated synthetic stressors only; a positive value
    // opts small real boards back in for a slightly broader but slower iteration loop.
    double quickMaxMb = 0.0;
    long   quickMaxArg = 0;

    if( parser.Found( "quick-max-mb", &quickMaxArg ) )
        quickMaxMb = static_cast<double>( std::max( 0L, quickMaxArg ) );

    long repeatArg = 5;
    int  repeat = quick ? 3 : 5;

    if( parser.Found( "repeat", &repeatArg ) )
        repeat = std::max( 1, static_cast<int>( repeatArg ) );

    long topNArg = 10;
    int  topN = 10;

    if( parser.Found( "top-n", &topNArg ) )
        topN = std::max( 1, static_cast<int>( topNArg ) );

    std::optional<wxString> isolate;
    wxString                isolateArg;

    if( parser.Found( "isolate", &isolateArg ) )
    {
        isolate = isolateArg;

        if( !providerErrorCodes().count( isolateArg ) )
            std::printf( "warning: --isolate '%s' has no known error codes; the full provider "
                         "set will run.\n",
                         static_cast<const char*>( isolateArg.utf8_str() ) );
    }

    wxString outDir = wxFileName::GetCwd();
    wxString outArg;

    if( parser.Found( "out", &outArg ) )
        outDir = outArg;

    unsigned ncores = std::max( 1u, std::thread::hardware_concurrency() );
    double   maxLoad = ncores * 0.5;
    wxString maxLoadArg;

    if( parser.Found( "max-load", &maxLoadArg ) )
        maxLoadArg.ToCDouble( &maxLoad );

    // Low-load guard. The benchmark only means anything on an otherwise-idle machine, so refuse
    // to start under contention rather than emit noisy numbers the optimizer might trust.
    double startLoad = readOneMinuteLoad();

    if( maxLoad > 0.0 && startLoad > maxLoad )
    {
        std::printf( "aborting: 1-min loadavg %.2f exceeds limit %.2f (cores=%u). "
                     "Run on an idle machine or raise --max-load.\n",
                     startLoad, maxLoad, ncores );
        return cleanupExit( 2 );
    }

    std::vector<CACHE_MODE> cacheModes = { CACHE_MODE::COLD, CACHE_MODE::WARM };
    wxString                cacheArg;

    if( parser.Found( "cache", &cacheArg ) )
    {
        if( cacheArg == wxT( "cold" ) )
            cacheModes = { CACHE_MODE::COLD };
        else if( cacheArg == wxT( "warm" ) )
            cacheModes = { CACHE_MODE::WARM };
        else if( cacheArg != wxT( "both" ) )
        {
            std::printf( "error: --cache must be cold|warm|both\n" );
            return cleanupExit( 1 );
        }
    }
    else if( quick )
    {
        // Warm is the iteration-relevant number and halves the work versus cold+warm.
        cacheModes = { CACHE_MODE::WARM };
    }

    std::vector<RULES_VARIANT> variants = { RULES_VARIANT::NONE, RULES_VARIANT::DEFAULT,
                                            RULES_VARIANT::HEAVY };
    wxString                   variantArg;

    if( parser.Found( "rules-variant", &variantArg ) )
    {
        RULES_VARIANT v = RULES_VARIANT::DEFAULT;

        if( !parseVariant( variantArg, v ) )
        {
            std::printf( "error: --rules-variant must be none|default|heavy\n" );
            return cleanupExit( 1 );
        }

        variants = { v };
    }
    else if( quick )
    {
        // none gives the geometric baseline and heavy gives compile_ms plus the eval overhead
        // over that baseline; the default variant adds cost without new signal in quick mode.
        variants = { RULES_VARIANT::NONE, RULES_VARIANT::HEAVY };
    }

    // In quick mode keep only the immediately meaningful boards: the curated fast cells that
    // expose compiler and evaluator cost in seconds. Honor explicit "quick" manifest flags when
    // present, otherwise fall back to the synthetic tier C. --quick-max-mb opts small real boards
    // back in. The multi-minute giants belong to the full sweep, not the iteration loop.
    if( quick && !haveAdHocBoard )
    {
        bool anyFlagged = std::any_of( entries.begin(), entries.end(),
                                       []( const CORPUS_ENTRY& e ) { return e.quick; } );

        std::vector<CORPUS_ENTRY> kept;

        for( const CORPUS_ENTRY& entry : entries )
        {
            wxULongLong size = wxFileName::GetSize( entry.board );
            bool        small = quickMaxMb > 0.0 && size != wxInvalidSize
                         && size.ToDouble() < quickMaxMb * 1.0e6;
            bool        flagged = anyFlagged ? entry.quick : ( entry.tier == wxT( "C" ) );

            if( flagged || small )
                kept.push_back( entry );
        }

        entries.swap( kept );
    }

    std::printf( "corpus:    %s\n", haveAdHocBoard
                                            ? "ad-hoc"
                                            : static_cast<const char*>( CORPUS::Root().utf8_str() ) );
    std::printf( "boards:    %zu%s\n", entries.size(), quick ? " (quick set)" : "" );
    std::printf( "threads:   %d (0=all)\n", threads );
    std::printf( "repeat:    %d\n", repeat );
    std::printf( "timeout:   %.0f s/test%s\n", timeoutSec, timeoutSec > 0.0 ? "" : " (disabled)" );
    std::printf( "max-load:  %.2f (start %.2f, cores %u)\n", maxLoad, startLoad, ncores );

    if( isolate )
        std::printf( "isolate:   %s\n", static_cast<const char*>( isolate->utf8_str() ) );

    std::printf( "out:       %s\n\n", static_cast<const char*>( outDir.utf8_str() ) );

    int                       rv = 0;
    std::vector<COVERAGE_ROW> coverageRows;
    std::vector<RESULT_ROW>   resultRows;

    bool rulesOnly = parser.Found( "rules-only" );

    for( const CORPUS_ENTRY& entry : entries )
    {
        wxFileName boardName( entry.board );
        wxFileName projectName( boardName );
        projectName.SetExt( FILEEXT::ProjectFileExtension );

        // A single malformed corpus board must not take down the whole sweep, so isolate every
        // board's load + timing behind a catch that records the failure and moves on.
        try
        {

        if( projectName.Exists() )
            manager.LoadProject( projectName.GetFullPath() );

        // The manifest rules path feeds both the default and heavy variants; ad-hoc runs may
        // instead carry an explicit --rules. Heavy falls back to --heavy-rules when present.
        std::optional<wxString> entryDefaultRules;

        if( !entry.rules.IsEmpty() )
            entryDefaultRules = entry.rules;
        else if( cliDefaultRules )
            entryDefaultRules = cliDefaultRules;

        std::optional<wxString> entryHeavyRules = cliHeavyRules ? cliHeavyRules : entryDefaultRules;

        std::printf( "### board: %s (tier %s)\n",
                     static_cast<const char*>( boardName.GetFullName().utf8_str() ),
                     static_cast<const char*>( entry.tier.utf8_str() ) );

        // Coverage uses the default-variant rules so the matrix reflects the rule set the corpus
        // ships, independent of any heavy sibling used only for stress timing.
        wxFileName coverageRules =
                resolveRules( boardName, entryDefaultRules, entryHeavyRules, RULES_VARIANT::DEFAULT );

        coverageRows.push_back(
                collectCoverage( boardName, manager, projectName, coverageRules ) );

        if( rulesOnly )
        {
            std::printf( "%-10s %14s %12s\n", "variant", "compile_med", "compile_mad" );
            std::printf( "%-10s %14s %12s\n", "----------", "--------------", "------------" );

            for( RULES_VARIANT variant : variants )
            {
                wxFileName rulesFile =
                        resolveRules( boardName, entryDefaultRules, entryHeavyRules, variant );

                STAT compile =
                        runCompileOnly( boardName, manager, projectName, rulesFile, repeat );

                std::printf( "%-10s %14.3f %12.3f\n", variantName( variant ), compile.median,
                             compile.mad );

                RESULT_ROW resultRow;
                resultRow.board = boardName.GetFullName();
                resultRow.config = configTag( CACHE_MODE::COLD, variant, threads ) + wxT( "/compile" );
                resultRow.compile = compile;
                resultRows.push_back( resultRow );
            }

            std::printf( "\n" );
            continue;
        }

        for( CACHE_MODE cache : cacheModes )
        {
            const char* cacheLabel = cache == CACHE_MODE::COLD ? "cold" : "warm";

            std::printf( "--- cache: %s ---\n", cacheLabel );
            std::printf( "%-10s %12s %10s %12s %12s %12s %10s\n", "variant", "compile_ms", "(mad)",
                         "cache_gen", "check_ms", "eval_ovhd", "violations" );
            std::printf( "%-10s %12s %10s %12s %12s %12s %10s\n", "----------", "------------",
                         "----------", "------------", "------------", "------------",
                         "----------" );

            std::optional<double> noneCheck;

            for( RULES_VARIANT variant : variants )
            {
                BENCH_CONFIG config;
                config.rulesVariant = variant;
                config.cache = cache;
                config.threads = threads;
                config.repeat = repeat;

                wxFileName rulesFile =
                        resolveRules( boardName, entryDefaultRules, entryHeavyRules, variant );

                SWEEP_RESULT result = runConfig( boardName, manager, projectName, rulesFile, config,
                                                 isolate, maxLoad, timeoutSec );

                if( !result.ran )
                {
                    rv = 1;
                    continue;
                }

                // A timed-out none baseline cannot anchor eval_overhead, so only record it when
                // the check actually completed.
                if( variant == RULES_VARIANT::NONE && !result.timedOut )
                    noneCheck = result.check.median;

                RESULT_ROW resultRow;
                resultRow.board = boardName.GetFullName();
                resultRow.config = configTag( cache, variant, threads );
                resultRow.compile = result.compile;
                resultRow.cacheGen = result.cacheGen;
                resultRow.check = result.check;
                resultRow.violations = result.violations;
                resultRow.underLoad = result.underLoad;
                resultRow.timedOut = result.timedOut;
                resultRow.fraction = result.fraction;
                resultRow.perProvider = result.providerStats;

                // eval_overhead isolates what the evaluator adds over the rules-free geometric
                // baseline for the same board and cache mode. It only has meaning when both this
                // check and the none baseline completed in this same sweep; a timeout leaves the
                // check time partial, so the overhead stays unrecorded.
                wxString ovhd = wxT( "n/a" );

                if( result.timedOut )
                {
                    ovhd = wxT( "timeout" );
                }
                else if( variant == RULES_VARIANT::NONE )
                {
                    resultRow.evalOverheadMs = 0.0;
                    resultRow.evalOverheadValid = true;
                    ovhd = wxT( "0.000" );
                }
                else if( noneCheck )
                {
                    resultRow.evalOverheadMs = result.check.median - *noneCheck;
                    resultRow.evalOverheadValid = true;
                    ovhd = wxString::Format( wxT( "%.3f" ), resultRow.evalOverheadMs );
                }

                resultRows.push_back( resultRow );

                if( result.timedOut )
                {
                    std::printf( "%-10s %12.3f %10.3f %12s %12s %12s   [TIMEOUT %.0f%%]\n",
                                 variantName( variant ), result.compile.median, result.compile.mad,
                                 "-", "-", static_cast<const char*>( ovhd.utf8_str() ),
                                 result.fraction * 100.0 );
                }
                else
                {
                    std::printf( "%-10s %12.3f %10.3f %12.3f %12.3f %12s %10d%s\n",
                                 variantName( variant ), result.compile.median, result.compile.mad,
                                 result.cacheGen.median, result.check.median,
                                 static_cast<const char*>( ovhd.utf8_str() ), result.violations,
                                 result.underLoad ? " [UNDER_LOAD]" : "" );
                }
            }

            std::printf( "\n" );
        }

        }
        catch( const std::exception& e )
        {
            std::printf( "error: board '%s' failed and was skipped: %s\n\n",
                         static_cast<const char*>( boardName.GetFullName().utf8_str() ), e.what() );
            rv = 1;
        }
    }

    emitCoverage( coverageRows, outDir );
    writeWorstOffenders( resultRows, outDir, topN );
    writeResultsJson( resultRows, outDir );

    std::printf( "wrote results.json, worst_offenders.json, coverage.json to %s\n",
                 static_cast<const char*>( outDir.utf8_str() ) );

    return cleanupExit( rv );
}

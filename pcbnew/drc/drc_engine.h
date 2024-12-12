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

#pragma once

#include <memory>
#include <shared_mutex>
#include <vector>
#include <unordered_map>

#include <kiid.h>
#include <layer_ids.h>
#include <units_provider.h>
#include <pcb_shape.h>
#include <lset.h>
#include <drc/drc_rule.h>


/**
 * Cache key for own clearance lookups, combining item UUID and layer.
 */
struct DRC_OWN_CLEARANCE_CACHE_KEY
{
    KIID         m_uuid;
    PCB_LAYER_ID m_layer;

    bool operator==( const DRC_OWN_CLEARANCE_CACHE_KEY& aOther ) const
    {
        return m_uuid == aOther.m_uuid && m_layer == aOther.m_layer;
    }
};


namespace std
{
    template <>
    struct hash<DRC_OWN_CLEARANCE_CACHE_KEY>
    {
        std::size_t operator()( const DRC_OWN_CLEARANCE_CACHE_KEY& aKey ) const
        {
            std::size_t seed = 0xa82de1c0;
            seed ^= std::hash<KIID>{}( aKey.m_uuid ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
            seed ^= std::hash<int>{}( static_cast<int>( aKey.m_layer ) ) + 0x9e3779b9
                    + ( seed << 6 ) + ( seed >> 2 );
            return seed;
        }
    };
}


class BOARD_COMMIT;
class BOARD_DESIGN_SETTINGS;
class DRC_TEST_PROVIDER;
class DRC_TEST_PROVIDER_CREEPAGE;
class PCB_EDIT_FRAME;
class DS_PROXY_VIEW_ITEM;
class BOARD_ITEM;
class BOARD;
class PCB_MARKER;
class NETCLASS;
class NETLIST;
class NETINFO_ITEM;
class PROGRESS_REPORTER;
class REPORTER;
class wxFileName;

namespace KIGFX
{
    class VIEW_OVERLAY;
};

void drcPrintDebugMessage( int level, const wxString& msg, const char *function, int line );

#define drc_dbg(level, fmt, ...) \
    drcPrintDebugMessage( level, wxString::Format( fmt, __VA_ARGS__ ), __FUNCTION__, __LINE__ );

class DRC_RULE_CONDITION;
class DRC_ITEM;
class DRC_RULE;
class DRC_CONSTRAINT;


typedef std::function<void( const std::shared_ptr<DRC_ITEM>& aItem,
                            const VECTOR2I& aPos, int aLayer,
                            const std::function<void( PCB_MARKER* )>& aPathGenerator )> DRC_VIOLATION_HANDLER;


/**
 * Batch result for clearance-related constraints to reduce per-query overhead during PNS routing.
 */
struct DRC_CLEARANCE_BATCH
{
    int clearance = 0;
    int holeClearance = 0;
    int holeToHole = 0;
    int edgeClearance = 0;
    int physicalClearance = 0;
};


/**
 * Design Rule Checker object that performs all the DRC tests.
 *
 * Optionally reports violations via a DRC_VIOLATION_HANDLER, user-level progress via a
 * PROGRESS_REPORTER and rule parse errors via a REPORTER, all set through various setter
 * calls.
 *
 * Note that EvalRules() has yet another optional REPORTER for reporting resolution info to
 * the user.
 */
class DRC_ENGINE : public UNITS_PROVIDER
{
    // They need to change / restore the violation handler
    friend class DRC_TEST_PROVIDER_CREEPAGE;

public:
    DRC_ENGINE( BOARD* aBoard = nullptr, BOARD_DESIGN_SETTINGS* aSettings = nullptr );
    virtual ~DRC_ENGINE();

    // We own several lists of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    DRC_ENGINE( const DRC_ENGINE& ) = delete;
    DRC_ENGINE& operator=( const DRC_ENGINE& ) = delete;

    void SetBoard( BOARD* aBoard ) { m_board = aBoard; }
    BOARD* GetBoard() const { return m_board; }

    void SetDesignSettings( BOARD_DESIGN_SETTINGS* aSettings ) { m_designSettings = aSettings; }
    BOARD_DESIGN_SETTINGS* GetDesignSettings() const { return m_designSettings; }

    void SetSchematicNetlist( NETLIST* aNetlist ) { m_schematicNetlist = aNetlist; }
    NETLIST* GetSchematicNetlist() const { return m_schematicNetlist; }

    void SetDrawingSheet( DS_PROXY_VIEW_ITEM* aDrawingSheet ) { m_drawingSheet = aDrawingSheet; }
    DS_PROXY_VIEW_ITEM* GetDrawingSheet() const { return m_drawingSheet; }

    void SetDebugOverlay( std::shared_ptr<KIGFX::VIEW_OVERLAY> aOverlay )
    {
        m_debugOverlay = aOverlay;
    }

    std::shared_ptr<KIGFX::VIEW_OVERLAY> GetDebugOverlay() const { return m_debugOverlay; }

    /**
     * Set an optional DRC violation handler (receives DRC_ITEMs and positions).
     */
    void SetViolationHandler( DRC_VIOLATION_HANDLER aHandler )
    {
        m_violationHandler = std::move( aHandler );
    }

    void ClearViolationHandler()
    {
        m_violationHandler = DRC_VIOLATION_HANDLER();
    }

    /**
     * Set an optional reporter for user-level progress info.
     */
    void SetProgressReporter( PROGRESS_REPORTER* aProgRep ) { m_progressReporter = aProgRep; }
    PROGRESS_REPORTER* GetProgressReporter() const { return m_progressReporter; }

    /*
     * Set an optional reporter for rule parse/compile/run-time errors and log-level progress
     * information.
     *
     * Note: if no log reporter is installed rule parse/compile/run-time errors are returned
     * via a thrown PARSE_ERROR exception.
     */
    void SetLogReporter( REPORTER* aReporter ) { m_logReporter = aReporter; }

    /**
     * Initialize the DRC engine.
     *
     * @throws PARSE_ERROR if the rules file contains errors
     */
    void InitEngine( const wxFileName& aRulePath );

    void InitEngine( const std::shared_ptr<DRC_RULE>& rule );

    /**
     * Run the DRC tests.
     */
    void RunTests( EDA_UNITS aUnits, bool aReportAllTrackErrors, bool aTestFootprints,
                   BOARD_COMMIT* aCommit = nullptr );

    bool IsErrorLimitExceeded( int error_code );

    DRC_CONSTRAINT EvalRules( DRC_CONSTRAINT_T aConstraintType, const BOARD_ITEM* a,
                              const BOARD_ITEM* b, PCB_LAYER_ID aLayer,
                              REPORTER* aReporter = nullptr );

    DRC_CONSTRAINT EvalZoneConnection( const BOARD_ITEM* a, const BOARD_ITEM* b,
                                       PCB_LAYER_ID aLayer, REPORTER* aReporter = nullptr );

    /**
     * Evaluate all clearance-related constraints in a single batch call.
     * This reduces per-call overhead during interactive PNS routing.
     *
     * @param a First board item
     * @param b Second board item (may be nullptr)
     * @param aLayer Layer to evaluate constraints on
     * @return DRC_CLEARANCE_BATCH containing all clearance constraint values
     */
    DRC_CLEARANCE_BATCH EvalClearanceBatch( const BOARD_ITEM* a, const BOARD_ITEM* b,
                                            PCB_LAYER_ID aLayer );

    /**
     * Get the cached own clearance for an item on a specific layer.
     *
     * This is used by BOARD_CONNECTED_ITEM::GetOwnClearance() to avoid re-evaluating
     * DRC rules on every paint refresh.
     *
     * @param aItem the item to get clearance for.
     * @param aLayer the layer in question.
     * @param aSource optionally reports the source as a user-readable string.
     * @return the clearance in internal units.
     */
    int GetCachedOwnClearance( const BOARD_ITEM* aItem, PCB_LAYER_ID aLayer,
                               wxString* aSource = nullptr );

    /**
     * Invalidate the clearance cache for a specific item.
     *
     * Called when item properties that could affect clearance (net, type, layer) change.
     *
     * @param aUuid the UUID of the item to invalidate.
     */
    void InvalidateClearanceCache( const KIID& aUuid );

    /**
     * Clear the entire clearance cache.
     *
     * Called when DRC rules change or board design settings change.
     */
    void ClearClearanceCache();

    /**
     * Initialize the clearance cache for all items on the board.
     *
     * Pre-populates the cache to avoid delays during first render. Should be called
     * after InitEngine() when a board is loaded.
     */
    void InitializeClearanceCache();

    void ProcessAssertions( const BOARD_ITEM* a,
                            std::function<void( const DRC_CONSTRAINT* )> aFailureHandler,
                            REPORTER* aReporter = nullptr );

    bool HasRulesForConstraintType( DRC_CONSTRAINT_T constraintID );

    bool GetReportAllTrackErrors() const { return m_reportAllTrackErrors; }
    bool GetTestFootprints() const { return m_testFootprints; }

    bool RulesValid() { return m_rulesValid; }

    void ReportViolation( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos,
                          int aMarkerLayer, const std::function<void( PCB_MARKER* )>& aPathGenerator = {} );

    bool KeepRefreshing( bool aWait = false );
    void AdvanceProgress();
    void SetMaxProgress( int aSize );
    bool ReportProgress( double aProgress );
    bool ReportPhase( const wxString& aMessage );
    bool IsCancelled() const;

    REPORTER* GetLogReporter() const { return m_logReporter; }

    bool QueryWorstConstraint( DRC_CONSTRAINT_T aRuleId, DRC_CONSTRAINT& aConstraint );
    std::set<int> QueryDistinctConstraints( DRC_CONSTRAINT_T aConstraintId );

    std::vector<DRC_TEST_PROVIDER*> GetTestProviders() const { return m_testProviders; };

    DRC_TEST_PROVIDER* GetTestProvider( const wxString& name ) const;

    /**
     * Evaluate a DRC condition against all board items and return matches.
     *
     * @param aExpression Expression to evaluate
     * @param aConstraint Constraint context for expression evaluation
     * @param aReporter Reporter for compile or evaluation errors
     */
    std::vector<BOARD_ITEM*> GetItemsMatchingCondition( const wxString& aExpression,
                                                        DRC_CONSTRAINT_T aConstraint = ASSERTION_CONSTRAINT,
                                                        REPORTER* aReporter = nullptr );

    static bool IsNetADiffPair( BOARD* aBoard, NETINFO_ITEM* aNet, int& aNetP, int& aNetN );

    /**
     * Check if the given net is a diff pair, returning its polarity and complement if so
     * @param aNetName is the input net name, like DIFF_P
     * @param aComplementNet will be filled with the complement, like DIFF_N
     * @param aBaseDpName will be filled with the base name, like DIFF
     * @return 1 if aNetName is the positive half of a pair, -1 if negative, 0 if not a diff pair
     */
    static int MatchDpSuffix( const wxString& aNetName, wxString& aComplementNet,
                              wxString& aBaseDpName );

    /**
     * Check if the given collision between a track and another item occurs during the track's
     * entry into a net-tie pad.
     */
    bool IsNetTieExclusion( int aTrackNetCode, PCB_LAYER_ID aTrackLayer,
                            const VECTOR2I& aCollisionPos, BOARD_ITEM* aCollidingItem );

private:
    void addRule( std::shared_ptr<DRC_RULE>& rule )
    {
        m_rules.push_back(rule);
    }

    /**
     * Load and parse a rule set from an sexpr text file.
     *
     * @throws PARSE_ERROR
     */
    void loadRules( const wxFileName& aPath );

    void compileRules();

    struct DRC_ENGINE_CONSTRAINT
    {
        LSET                       layerTest;
        DRC_RULE_CONDITION*        condition;
        std::shared_ptr<DRC_RULE>  parentRule;
        DRC_CONSTRAINT             constraint;
    };

    void loadImplicitRules();
    std::shared_ptr<DRC_RULE> createImplicitRule( const wxString& name, DRC_IMPLICIT_SOURCE aImplicitSource );

protected:
    BOARD_DESIGN_SETTINGS*     m_designSettings;
    BOARD*                     m_board;
    DS_PROXY_VIEW_ITEM*        m_drawingSheet;
    NETLIST*                   m_schematicNetlist;

    std::vector<std::shared_ptr<DRC_RULE>>  m_rules;
    bool                                    m_rulesValid;
    std::vector<DRC_TEST_PROVIDER*>         m_testProviders;

    std::vector<int>           m_errorLimits;
    bool                       m_reportAllTrackErrors;
    bool                       m_testFootprints;

    // constraint -> rule -> provider
    std::map<DRC_CONSTRAINT_T, std::vector<DRC_ENGINE_CONSTRAINT*>*> m_constraintMap;

    DRC_VIOLATION_HANDLER      m_violationHandler;
    REPORTER*                  m_logReporter;
    PROGRESS_REPORTER*         m_progressReporter;

    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_debugOverlay;

    // Cache for GetOwnClearance lookups to improve rendering performance.
    // Key is (UUID, layer), value is clearance in internal units.
    // Protected by m_clearanceCacheMutex for thread-safe access during rendering.
    std::unordered_map<DRC_OWN_CLEARANCE_CACHE_KEY, int> m_ownClearanceCache;

    // Netclass name -> clearance mapping for fast lookup in EvalRules.
    // Only written during InitEngine(), read during DRC and rendering.
    // Protected by m_clearanceCacheMutex for thread-safe access.
    std::unordered_map<wxString, int> m_netclassClearances;

    // Mutex protecting clearance caches for thread-safe access.
    // Uses shared_mutex for reader-writer pattern (many concurrent reads, exclusive writes).
    mutable std::shared_mutex m_clearanceCacheMutex;
    bool m_hasExplicitClearanceRules = false;
    bool m_hasDiffPairClearanceOverrides = false;
    std::map<DRC_CONSTRAINT_T, std::vector<DRC_ENGINE_CONSTRAINT*>> m_explicitConstraints;
};

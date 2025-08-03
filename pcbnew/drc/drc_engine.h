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

#ifndef DRC_ENGINE_H
#define DRC_ENGINE_H

#include <memory>
#include <vector>
#include <unordered_map>

#include <units_provider.h>
#include <geometry/shape.h>
#include <lset.h>
#include <drc/drc_rule.h>


class BOARD_COMMIT;
class BOARD_DESIGN_SETTINGS;
class DRC_TEST_PROVIDER;
class DRC_TEST_PROVIDER_CLEARANCE_BASE;
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
    drcPrintDebugMessage(level, wxString::Format( fmt, __VA_ARGS__ ), __FUNCTION__, __LINE__ );

class DRC_RULE_CONDITION;
class DRC_ITEM;
class DRC_RULE;
class DRC_CONSTRAINT;

typedef std::function<void( PCB_MARKER* aMarker )> DRC_CUSTOM_MARKER_HANDLER;

typedef std::function<void( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos,
                            int aLayer, DRC_CUSTOM_MARKER_HANDLER* aCustomHandler )>
        DRC_VIOLATION_HANDLER;

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
    friend class DRC_TEST_PROVIDER_CLEARANCE_BASE;
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
    void SetLogReporter( REPORTER* aReporter ) { m_reporter = aReporter; }

    /**
     * Initialize the DRC engine.
     *
     * @throws PARSE_ERROR if the rules file contains errors
     */
    void InitEngine( const wxFileName& aRulePath );

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

    void ProcessAssertions( const BOARD_ITEM* a,
                            std::function<void( const DRC_CONSTRAINT* )> aFailureHandler,
                            REPORTER* aReporter = nullptr );

    bool HasRulesForConstraintType( DRC_CONSTRAINT_T constraintID );

    bool GetReportAllTrackErrors() const { return m_reportAllTrackErrors; }
    bool GetTestFootprints() const { return m_testFootprints; }

    bool RulesValid() { return m_rulesValid; }

    void ReportViolation( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos,
                          int aMarkerLayer, DRC_CUSTOM_MARKER_HANDLER* aCustomHandler = nullptr );

    bool KeepRefreshing( bool aWait = false );
    void AdvanceProgress();
    void SetMaxProgress( int aSize );
    bool ReportProgress( double aProgress );
    bool ReportPhase( const wxString& aMessage );
    void ReportAux( const wxString& aStr );
    bool IsCancelled() const;

    bool QueryWorstConstraint( DRC_CONSTRAINT_T aRuleId, DRC_CONSTRAINT& aConstraint );
    std::set<int> QueryDistinctConstraints( DRC_CONSTRAINT_T aConstraintId );

    std::vector<DRC_TEST_PROVIDER*> GetTestProviders() const { return m_testProviders; };

    DRC_TEST_PROVIDER* GetTestProvider( const wxString& name ) const;

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
    std::shared_ptr<DRC_RULE> createImplicitRule( const wxString& name );

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
    REPORTER*                  m_reporter;
    PROGRESS_REPORTER*         m_progressReporter;

    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_debugOverlay;
};

#endif // DRC_H

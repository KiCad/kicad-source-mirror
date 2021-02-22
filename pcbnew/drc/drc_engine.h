/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <geometry/shape.h>

#include <drc/drc_rule.h>


class BOARD_DESIGN_SETTINGS;
class DRC_TEST_PROVIDER;
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


typedef
std::function<void( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )> DRC_VIOLATION_HANDLER;


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
class DRC_ENGINE
{
public:
    DRC_ENGINE( BOARD* aBoard = nullptr, BOARD_DESIGN_SETTINGS* aSettings = nullptr );
    ~DRC_ENGINE();

    void SetBoard( BOARD* aBoard ) { m_board = aBoard; }
    BOARD* GetBoard() const { return m_board; }

    void SetDesignSettings( BOARD_DESIGN_SETTINGS* aSettings ) { m_designSettings = aSettings; }
    BOARD_DESIGN_SETTINGS* GetDesignSettings() const { return m_designSettings; }

    void SetSchematicNetlist( NETLIST* aNetlist ) { m_schematicNetlist = aNetlist; }
    NETLIST* GetSchematicNetlist() const { return m_schematicNetlist; }

    void SetDrawingSheet( DS_PROXY_VIEW_ITEM* aDrawingSheet ) { m_drawingSheet = aDrawingSheet; }
    DS_PROXY_VIEW_ITEM* GetDrawingSheet() const { return m_drawingSheet; }

    void SetDebugOverlay( std::shared_ptr<KIGFX::VIEW_OVERLAY> aOverlay ) { m_debugOverlay = aOverlay; }
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
     * Initializes the DRC engine.
     *
     * @throws PARSE_ERROR if the rules file contains errors
     */
    void InitEngine( const wxFileName& aRulePath );

    /**
     * Runs the DRC tests.
     */
    void RunTests( EDA_UNITS aUnits,  bool aReportAllTrackErrors, bool aTestFootprints );


    bool IsErrorLimitExceeded( int error_code );

    DRC_CONSTRAINT EvalRules( DRC_CONSTRAINT_T aConstraintId, const BOARD_ITEM* a,
                              const BOARD_ITEM* b, PCB_LAYER_ID aLayer,
                              REPORTER* aReporter = nullptr );

    bool HasRulesForConstraintType( DRC_CONSTRAINT_T constraintID );

    EDA_UNITS UserUnits() const { return m_userUnits; }
    bool GetReportAllTrackErrors() const { return m_reportAllTrackErrors; }
    bool GetTestFootprints() const { return m_testFootprints; }

    bool RulesValid() { return m_rulesValid; }

    void ReportViolation( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos );
    bool ReportProgress( double aProgress );
    bool ReportPhase( const wxString& aMessage );
    void ReportAux( const wxString& aStr );

    bool QueryWorstConstraint( DRC_CONSTRAINT_T aRuleId, DRC_CONSTRAINT& aConstraint );

    std::vector<DRC_TEST_PROVIDER* > GetTestProviders() const { return m_testProviders; };

    DRC_TEST_PROVIDER* GetTestProvider( const wxString& name ) const;

    static bool IsNetADiffPair( BOARD* aBoard, NETINFO_ITEM* aNet, int& aNetP, int& aNetN );

    static bool IsNetTie( BOARD_ITEM* aItem );

    static std::shared_ptr<SHAPE> GetShape( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer );

private:
    void addRule( DRC_RULE* rule )
    {
        m_rules.push_back(rule);
    }

    /**
     * Loads and parses a rule set from an sexpr text file.
     *
     * @throws PARSE_ERROR
     */
    void loadRules( const wxFileName& aPath );

    void compileRules();

    struct DRC_ENGINE_CONSTRAINT
    {
        LSET                 layerTest;
        DRC_RULE_CONDITION*  condition;
        DRC_RULE*            parentRule;
        DRC_CONSTRAINT       constraint;
    };

    void loadImplicitRules();
    DRC_RULE* createImplicitRule( const wxString& name );

protected:
    BOARD_DESIGN_SETTINGS*           m_designSettings;
    BOARD*                           m_board;
    DS_PROXY_VIEW_ITEM*              m_drawingSheet;
    NETLIST*                         m_schematicNetlist;

    std::vector<DRC_RULE*>           m_rules;
    bool                             m_rulesValid;
    std::vector<DRC_TEST_PROVIDER*>  m_testProviders;

    EDA_UNITS                        m_userUnits;
    std::vector<int>                 m_errorLimits;
    bool                             m_reportAllTrackErrors;
    bool                             m_testFootprints;

    // constraint -> rule -> provider
    std::unordered_map<DRC_CONSTRAINT_T, std::vector<DRC_ENGINE_CONSTRAINT*>*> m_constraintMap;

    DRC_VIOLATION_HANDLER            m_violationHandler;
    REPORTER*                        m_reporter;
    PROGRESS_REPORTER*               m_progressReporter;

    wxString m_msg;  // Allocating strings gets expensive enough to want to avoid it
    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_debugOverlay;
};

#endif // DRC_H

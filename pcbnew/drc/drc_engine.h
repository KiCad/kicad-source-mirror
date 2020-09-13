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

#include <drc/drc_rule.h>


class BOARD_DESIGN_SETTINGS;
class DRC_TEST_PROVIDER;
class PCB_EDIT_FRAME;
class BOARD_ITEM;
class BOARD;
class ZONE_CONTAINER;
class MARKER_PCB;
class NETCLASS;
class NETLIST;
class PROGRESS_REPORTER;
class REPORTER;

namespace KIGFX
{
    class WS_PROXY_VIEW_ITEM;
};

void drcPrintDebugMessage( int level, wxString msg, const char *function, int line );

#define drc_dbg(level, fmt, ...) \
    drcPrintDebugMessage(level, wxString::Format( fmt, __VA_ARGS__ ), __FUNCTION__, __LINE__ );

class DRC_RULE_CONDITION;
class DRC_ITEM;
class DRC_RULE;
class LEGACY_DRC_TEST_PROVIDER;
class DRC_CONSTRAINT;

enum DRC_CONSTRAINT_QUERY_T
{
    DRCCQ_LARGEST_MINIMUM = 0
    // fixme: more to come I guess...
};

typedef
std::function<void( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )> DRC_VIOLATION_HANDLER;


/**
 * Design Rule Checker object that performs all the DRC tests.  The output of
 * the checking goes to the BOARD file in the form of two MARKER lists.  Those
 * two lists are displayable in the drc dialog box.  And they can optionally
 * be sent to a text file on disk.
 * This class is given access to the windows and the BOARD
 * that it needs via its constructor or public access functions.
 */
class DRC_ENGINE
{

public:
    DRC_ENGINE( BOARD* aBoard, BOARD_DESIGN_SETTINGS* aSettings );
    ~DRC_ENGINE();

    void SetSchematicNetlist( NETLIST* aNetlist ) { m_schematicNetlist = aNetlist; }
    NETLIST* GetSchematicNetlist() const { return m_schematicNetlist; }

    // JEY TODO: why isn't this called?
    void SetWorksheet( KIGFX::WS_PROXY_VIEW_ITEM* aWorksheet ) { m_worksheet = aWorksheet; }
    KIGFX::WS_PROXY_VIEW_ITEM* GetWorksheet() const { return m_worksheet; }

    // JEY TODO: rationalize old progress report style with new...
    void SetProgressReporter( PROGRESS_REPORTER* aProgRep ) { m_progressReporter = aProgRep; }

    void SetLogReporter( REPORTER* aReporter ) { m_reporter = aReporter; }

    bool LoadRules( const wxFileName& aPath );

    void InitEngine( const wxFileName& aRulePath );

    void RunTests( DRC_VIOLATION_HANDLER aDRCItemHandler );

    BOARD_DESIGN_SETTINGS* GetDesignSettings() const { return m_designSettings; }

    BOARD* GetBoard() const { return m_board; }

    bool IsErrorLimitExceeded( int error_code )
    {
        m_errorLimits[ error_code ] -= 1;

        return m_errorLimits[ error_code ] <= 0;
    }

    DRC_CONSTRAINT EvalRulesForItems( DRC_CONSTRAINT_TYPE_T ruleID, BOARD_ITEM* a,
                                      BOARD_ITEM* b = nullptr,
                                      PCB_LAYER_ID aLayer = UNDEFINED_LAYER,
                                      REPORTER* aReporter = nullptr );

    std::vector<DRC_CONSTRAINT> QueryConstraintsById( DRC_CONSTRAINT_TYPE_T ruleID );

    bool HasRulesForConstraintType( DRC_CONSTRAINT_TYPE_T constraintID );

    EDA_UNITS UserUnits() const
    {
        // JEY TODO
        return EDA_UNITS::MILLIMETRES;
    }

    bool GetTestTracksAgainstZones() const
    {
        // JEY TODO
        return true;
    }

    bool CompileRules();

    void ReportViolation( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos );
    void ReportProgress( double aProgress );
    void ReportStage ( const wxString& aStageName, int index, int total );
    void ReportAux( const wxString& aStr );

    bool QueryWorstConstraint( DRC_CONSTRAINT_TYPE_T aRuleId, DRC_CONSTRAINT& aConstraint,
                               DRC_CONSTRAINT_QUERY_T aQueryType );

private:
    void addRule( DRC_RULE* rule )
    {
        m_rules.push_back(rule);
    }

    void freeCompiledRules();

    struct CONSTRAINT_WITH_CONDITIONS
    {
        LSET                 layerTest;
        DRC_RULE_CONDITION*  condition;
        DRC_RULE*            parentRule;
        DRC_CONSTRAINT       constraint;
    };

    struct CONSTRAINT_SET
    {
        std::vector<CONSTRAINT_WITH_CONDITIONS*> sortedConstraints;
        DRC_TEST_PROVIDER*                       provider;
    };

    void loadImplicitRules();
    void loadTestProviders();
    DRC_RULE* createImplicitRule( const wxString& name );

protected:
    BOARD_DESIGN_SETTINGS*           m_designSettings;
    BOARD*                           m_board;
    KIGFX::WS_PROXY_VIEW_ITEM*       m_worksheet;
    NETLIST*                         m_schematicNetlist;

    std::vector<DRC_RULE_CONDITION*> m_ruleConditions;
    std::vector<DRC_RULE*>           m_rules;
    std::vector<DRC_TEST_PROVIDER*>  m_testProviders;
    std::vector<int>                 m_errorLimits;

    // constraint -> rule -> provider
    std::unordered_map<DRC_CONSTRAINT_TYPE_T, CONSTRAINT_SET*> m_constraintMap;

    DRC_VIOLATION_HANDLER            m_violationHandler;
    REPORTER*                        m_reporter;
    PROGRESS_REPORTER*               m_progressReporter;

    wxString m_msg;  // Allocating strings gets expensive enough to want to avoid it
};

#endif // DRC_H

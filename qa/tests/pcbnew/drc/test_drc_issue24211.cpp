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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>

#include <set>

#include <board.h>
#include <board_design_settings.h>
#include <board_loader.h>
#include <component_classes/component_class.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <footprint.h>
#include <pcb_io/pcb_io_mgr.h>
#include <project.h>
#include <settings/settings_manager.h>
#include <wx/filename.h>


struct DRC_ISSUE_24211_FIXTURE
{
    SETTINGS_MANAGER m_settingsManager;

    PROJECT* loadProject()
    {
        wxFileName projectFile( wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() )
                                + "issue24211/issue24211.kicad_pro" );

        m_settingsManager.LoadProject( projectFile.GetFullPath() );

        PROJECT* project = m_settingsManager.GetProject( projectFile.GetFullPath() );
        BOOST_REQUIRE_MESSAGE( project, "Could not load project" );
        return project;
    }

    wxString boardPath() const
    {
        return wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() )
               + "issue24211/issue24211.kicad_pcb";
    }
};


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24211
//
// BOARD_LOADER::Load is the entry point for CLI and API consumers (kicad-cli pcb drc and the
// api-server). It must produce a board with component class assignments applied so that custom
// DRC rules using A.hasComponentClass(...) match the same items they would in the interactive
// DRC dialog. Before the fix, dynamic class assignments were only applied in the GUI load path.
BOOST_FIXTURE_TEST_CASE( DRCIssue24211ComponentClassesAfterLoad, DRC_ISSUE_24211_FIXTURE )
{
    PROJECT* project = loadProject();

    std::unique_ptr<BOARD> board = BOARD_LOADER::Load( boardPath(), PCB_IO_MGR::KICAD_SEXP,
                                                       project );

    BOOST_REQUIRE( board );

    // The reproduction project assigns the "Inductor" class to reference L1. After
    // BOARD_LOADER::Load with initialize_after_load=true (the CLI / API default), every footprint
    // matched by an assignment rule must already report that class.
    FOOTPRINT* l1 = board->FindFootprintByReference( wxT( "L1" ) );

    BOOST_REQUIRE_MESSAGE( l1, "Reference L1 not present in test board" );

    const COMPONENT_CLASS* compClass = l1->GetComponentClass();

    BOOST_REQUIRE_MESSAGE( compClass, "L1 has no component class after BOARD_LOADER::Load" );
    BOOST_TEST_MESSAGE( "L1 class: " << compClass->GetName().ToStdString() );
    BOOST_CHECK( compClass->ContainsClassName( wxT( "Inductor" ) ) );
}


// Runs the custom-rule DRC against the issue24211 board through the same load path the CLI uses
// (BOARD_LOADER::Load). Without the fix, the second custom rule
// ("pcblint_inductor_sensitive_trace_keepout_no_track") never fires because no footprint has the
// 'Inductor' component class assigned; only the first ("buck_feedback_away_from_switch_node")
// reports violations. After the fix, both rules report violations, matching the interactive DRC.
BOOST_FIXTURE_TEST_CASE( DRCIssue24211BothCustomRulesFire, DRC_ISSUE_24211_FIXTURE )
{
    PROJECT* project = loadProject();

    std::unique_ptr<BOARD> board = BOARD_LOADER::Load( boardPath(), PCB_IO_MGR::KICAD_SEXP,
                                                       project );

    BOOST_REQUIRE( board );

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

    BOOST_REQUIRE( bds.m_DRCEngine );

    // Ignore checks unrelated to the custom clearance rules under test
    for( int code = DRCE_FIRST; code <= DRCE_LAST; ++code )
        bds.m_DRCSeverities[code] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CLEARANCE] = SEVERITY::RPT_SEVERITY_ERROR;

    std::set<wxString> firedRules;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetViolatingRule() )
                    firedRules.insert( aItem->GetViolatingRule()->m_Name );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_TEST_MESSAGE( "Fired rules:" );

    for( const wxString& r : firedRules )
        BOOST_TEST_MESSAGE( "  " << r.ToStdString() );

    BOOST_CHECK_MESSAGE( firedRules.count( wxT( "pcblint_buck_feedback_away_from_switch_node" ) ),
                         "Netclass-based custom rule did not fire" );

    BOOST_CHECK_MESSAGE(
            firedRules.count( wxT( "pcblint_inductor_sensitive_trace_keepout_no_track" ) ),
            "Component-class-based custom rule did not fire after BOARD_LOADER::Load" );
}

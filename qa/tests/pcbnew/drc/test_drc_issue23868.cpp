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

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <board.h>
#include <board_design_settings.h>
#include <board_loader.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <pcb_io/pcb_io_mgr.h>
#include <project.h>
#include <settings/settings_manager.h>
#include <wx/filename.h>


struct DRC_ISSUE_23868_FIXTURE
{
    SETTINGS_MANAGER m_settingsManager;

    PROJECT* loadProject()
    {
        wxFileName projectFile( wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() )
                                + "issue23868/issue23868.kicad_pro" );

        m_settingsManager.LoadProject( projectFile.GetFullPath() );

        PROJECT* project = m_settingsManager.GetProject( projectFile.GetFullPath() );
        BOOST_REQUIRE_MESSAGE( project, "Could not load project" );
        return project;
    }

    wxString boardPath() const
    {
        return wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() )
               + "issue23868/issue23868.kicad_pcb";
    }
};


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23868
//
// Length-domain DRC rules expressed in propagation delay (e.g. "min 85ps") would spuriously fire
// when run through kicad-cli pcb drc because BOARD_LOADER::Load skipped the tuning-profile cache
// rebuild that PCB_EDIT_FRAME::OpenProjectFiles performs. Without the cache, the matched-length
// provider sees every track's propagation delay as 0 ps and reports it as below the rule minimum.
//
// IMPORTANT: this test must use BOARD_LOADER::Load directly, NOT KI_TEST::LoadBoard.
// KI_TEST::LoadBoard calls SynchronizeTuningProfileProperties itself, which would mask the bug.
BOOST_FIXTURE_TEST_CASE( DRCIssue23868PropagationDelayCacheAfterLoad, DRC_ISSUE_23868_FIXTURE )
{
    PROJECT* project = loadProject();

    std::unique_ptr<BOARD> board = BOARD_LOADER::Load( boardPath(), PCB_IO_MGR::KICAD_SEXP,
                                                       project );

    BOOST_REQUIRE( board );

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

    BOOST_REQUIRE( bds.m_DRCEngine );
    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine->RulesValid(),
                           "BOARD_LOADER did not parse the .kicad_dru file" );

    for( int code = DRCE_FIRST; code <= DRCE_LAST; ++code )
        bds.m_DRCSeverities[code] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_LENGTH_OUT_OF_RANGE] = SEVERITY::RPT_SEVERITY_ERROR;

    std::vector<std::pair<wxString, wxString>> violations;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                wxString ruleName;

                if( aItem->GetViolatingRule() )
                    ruleName = aItem->GetViolatingRule()->m_Name;

                violations.emplace_back( ruleName, aItem->GetErrorMessage( false ) );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_TEST_MESSAGE( "Total violations: " << violations.size() );

    for( const auto& [rule, msg] : violations )
        BOOST_TEST_MESSAGE( "Violation rule='" << rule.ToStdString()
                            << "' msg='" << msg.ToStdString() << "'" );

    // The reproduction board routes its DIFF_100 pairs to lengths whose propagation delay is well
    // inside the 85 ps .. 1020 ps window defined by the diff_propagation_delay rule. The GUI
    // confirms zero length-out-of-range violations. Before the fix, BOARD_LOADER::Load returned a
    // board whose tuning-profile cache was empty, so every routed pair reported 0 ps and tripped
    // the min-length check.
    for( const auto& [rule, msg] : violations )
    {
        BOOST_CHECK_MESSAGE( rule != wxT( "diff_propagation_delay" ),
                             "diff_propagation_delay reported a spurious violation: "
                             << msg.ToStdString() );
    }
}

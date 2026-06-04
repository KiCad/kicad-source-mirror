/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <api/headless_pcb_context.h>
#include <board.h>
#include <board_loader.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcbnew_utils/board_file_utils.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <wx/filename.h>


struct HEADLESS_PCB_CONTEXT_TEST_FIXTURE
{
    PROJECT* loadProject( const wxString& aBoardName )
    {
        wxFileName projectFile( wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() )
                                + aBoardName + ".kicad_pro" );

        m_settingsManager.LoadProject( projectFile.GetFullPath() );

        PROJECT* project = m_settingsManager.GetProject( projectFile.GetFullPath() );

        BOOST_REQUIRE_MESSAGE( project, "Could not load project" );
        return project;
    }

    wxString boardPath( const wxString& aBoardName ) const
    {
        return wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() ) + aBoardName + ".kicad_pcb";
    }

    SETTINGS_MANAGER m_settingsManager;
};


BOOST_FIXTURE_TEST_CASE( HeadlessBoardContextBasics, HEADLESS_PCB_CONTEXT_TEST_FIXTURE )
{
    PROJECT* project = loadProject( "reference_images_load_save" );

    std::unique_ptr<BOARD> board =
            BOARD_LOADER::Load( boardPath( "reference_images_load_save" ),
                                PCB_IO_MGR::KICAD_SEXP,
                                project );

    BOOST_REQUIRE( board );

    HEADLESS_PCB_CONTEXT context( std::move( board ), project, nullptr );

    BOOST_CHECK( context.GetBoard() );
    BOOST_CHECK_EQUAL( context.GetBoard()->GetProject(), project );
    BOOST_CHECK_EQUAL( context.Prj().GetProjectFullName(), project->GetProjectFullName() );
    BOOST_CHECK( context.GetToolManager() );
    BOOST_CHECK_EQUAL( context.GetToolManager()->GetModel(), context.GetBoard() );
    BOOST_CHECK_EQUAL( context.GetCurrentFileName(), context.GetBoard()->GetFileName() );
    BOOST_CHECK( context.CanAcceptApiCommands() );
}

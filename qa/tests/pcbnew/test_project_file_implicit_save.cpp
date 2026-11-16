/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <board_loader.h>
#include <json_common.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcbnew_utils/board_file_utils.h>
#include <project.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;


/**
 * Opens a copy of a project last saved by an older KiCad (project and board design settings at
 * schema 3) the way the board editor does.
 */
struct PROJECT_FILE_IMPLICIT_SAVE_FIXTURE
{
    PROJECT_FILE_IMPLICIT_SAVE_FIXTURE()
    {
        m_dir = fs::temp_directory_path() / "kicad_qa_project_file_implicit_save";
        fs::remove_all( m_dir );
        fs::create_directories( m_dir );

        fs::path source = fs::path( KI_TEST::GetPcbnewTestDataDir() ) / "mdw_ctrl_board";

        for( const char* name : { "ctrl-board.kicad_pro", "ctrl-board.kicad_pcb" } )
            fs::copy_file( source / name, m_dir / name );

        m_proPath = m_dir / "ctrl-board.kicad_pro";
        m_original = readFile( m_proPath );
    }

    ~PROJECT_FILE_IMPLICIT_SAVE_FIXTURE()
    {
        m_board.reset();

        if( m_mgr.IsProjectOpen() )
            m_mgr.UnloadProject( &m_mgr.Prj(), false );

        fs::remove_all( m_dir );
    }

    static std::string readFile( const fs::path& aPath )
    {
        std::ifstream     in( aPath, std::ios::binary );
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

    void openBoard()
    {
        BOOST_REQUIRE( m_mgr.LoadProject( wxString::FromUTF8( m_proPath.string() ) ) );

        m_board = BOARD_LOADER::Load( wxString::FromUTF8( ( m_dir / "ctrl-board.kicad_pcb" ).string() ),
                                      PCB_IO_MGR::KICAD_SEXP, &m_mgr.Prj() );

        BOOST_REQUIRE( m_board );
    }

    fs::path               m_dir;
    fs::path               m_proPath;
    std::string            m_original;
    SETTINGS_MANAGER       m_mgr;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_SUITE( ProjectFileImplicitSave, PROJECT_FILE_IMPLICIT_SAVE_FIXTURE )


BOOST_AUTO_TEST_CASE( OpeningAndClosingAnOlderProjectLeavesItsFileUntouched )
{
    openBoard();

    // The editor still works on the migrated settings, where min_text_thickness became min_silk_line_width
    BOOST_CHECK_EQUAL( m_board->GetDesignSettings().m_MinSilkLineWidth, pcbIUScale.mmToIU( 0.12 ) );

    // The board editor saves the project when it closes
    BOOST_REQUIRE( m_mgr.SaveProject() );

    BOOST_CHECK( readFile( m_proPath ) == m_original );
}


BOOST_AUTO_TEST_CASE( RealSettingChangePersistsMigration )
{
    openBoard();

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.m_MinClearance += pcbIUScale.mmToIU( 0.05 );

    BOOST_REQUIRE( m_mgr.SaveProject() );

    nlohmann::json saved = nlohmann::json::parse( readFile( m_proPath ) );
    const nlohmann::json& savedBds = saved["board"]["design_settings"];

    BOOST_CHECK_CLOSE( savedBds["rules"]["min_clearance"].get<double>(), 0.05, 1e-6 );
    BOOST_CHECK_CLOSE( savedBds["rules"]["min_silk_line_width"].get<double>(), 0.12, 1e-6 );

    // Defaults the older file lacked are written with the change
    BOOST_CHECK( savedBds["rule_severities"].contains( "hole_aspect_ratio" ) );
}


BOOST_AUTO_TEST_SUITE_END()

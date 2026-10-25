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
#include <board_loader.h>
#include <board_design_settings.h>
#include <pcb_reference_image.h>
#include <pcb_track.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcbnew_utils/board_file_utils.h>
#include <project.h>
#include <settings/settings_manager.h>
#include <wx/filename.h>


struct BOARD_LOADER_TEST_FIXTURE
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


BOOST_FIXTURE_TEST_CASE( BoardLoaderWithInit, BOARD_LOADER_TEST_FIXTURE )
{
    PROJECT* project = loadProject( "reference_images_load_save" );

    std::unique_ptr<BOARD> board =
            BOARD_LOADER::Load( boardPath( "reference_images_load_save" ),
                                PCB_IO_MGR::KICAD_SEXP,
                                project );

    BOOST_REQUIRE( board );
    BOOST_CHECK_EQUAL( board->GetProject(), project );
    BOOST_CHECK( board->GetDesignSettings().m_DRCEngine );
    BOOST_CHECK( board->GetConnectivity() );
    BOOST_CHECK_GT( board->Drawings().size(), 0 );
}


BOOST_FIXTURE_TEST_CASE( BoardLoaderWithoutInit, BOARD_LOADER_TEST_FIXTURE )
{
    PROJECT* project = loadProject( "reference_images_load_save" );

    BOARD_LOADER::OPTIONS options;
    options.initialize_after_load = false;

    std::unique_ptr<BOARD> board =
            BOARD_LOADER::Load( boardPath( "reference_images_load_save" ),
                                PCB_IO_MGR::KICAD_SEXP,
                                project,
                                options );

    BOOST_REQUIRE( board );
    BOOST_CHECK_NE( board->GetProject(), project );
    BOOST_CHECK( board->GetDesignSettings().m_DRCEngine == nullptr );
}


BOOST_AUTO_TEST_CASE( CollinearTrackArcLoadsAsTrack )
{
    const std::string path = KI_TEST::GetPcbnewTestDataDir()
                             + "plugins/allegro/boards/VCU118_REV2-0/8851_HW-U1-VCU118_REV2-0_071417.kicad_pcb";

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( path );

    BOOST_REQUIRE( board );

    // Collinear (arc ...) record in the file
    BOARD_ITEM* item = board->ResolveItem( KIID( "945ee847-b394-409c-b580-2568b3573ad8" ) );

    BOOST_REQUIRE( item );
    BOOST_CHECK_EQUAL( item->Type(), PCB_TRACE_T );

    PCB_TRACK* track = static_cast<PCB_TRACK*>( item );

    BOOST_CHECK_EQUAL( track->GetStart(),
                       VECTOR2I( pcbIUScale.mmToIU( 81.471125 ), pcbIUScale.mmToIU( -136.690775 ) ) );
    BOOST_CHECK_EQUAL( track->GetEnd(),
                       VECTOR2I( pcbIUScale.mmToIU( 81.476825 ), pcbIUScale.mmToIU( -136.685025 ) ) );
    BOOST_CHECK_EQUAL( track->GetWidth(), pcbIUScale.mmToIU( 0.15 ) );
    BOOST_CHECK_EQUAL( track->GetLayer(), In2_Cu );
    BOOST_CHECK_EQUAL( track->GetNetname(), wxString( "FMCP_HSPC_DP11_C2M_N" ) );
}

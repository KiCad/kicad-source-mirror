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
 * @file test_easyeda_import.cpp
 * Test suite for import of EasyEDA standard-format boards.
 */

#include <pcbnew_utils/board_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/easyeda/pcb_io_easyeda_parser.h>
#include <pcbnew/pcb_io/pcb_io.h>
#include <pcbnew/pcb_io/pcb_io_mgr.h>

#include <board.h>
#include <board_design_settings.h>
#include <netclass.h>
#include <project/net_settings.h>


struct EASYEDA_IMPORT_FIXTURE
{
    EASYEDA_IMPORT_FIXTURE() {}
};


BOOST_FIXTURE_TEST_SUITE( EasyedaImport, EASYEDA_IMPORT_FIXTURE )


BOOST_AUTO_TEST_CASE( BoardLoadImportsDefaultClearance )
{
    wxString dataPath = wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir()
                                            + "plugins/easyeda/PCB_PCB_ESP32-PICO-D4 smart watch_2023-09-02.json" );

    IO_RELEASER<PCB_IO> plugin( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::EASYEDA ) );
    BOOST_REQUIRE( plugin );

    std::unique_ptr<BOARD> board( plugin->LoadBoard( dataPath, nullptr ) );
    BOOST_REQUIRE( board );

    std::shared_ptr<NETCLASS> defaultNetclass = board->GetDesignSettings().m_NetSettings->GetDefaultNetclass();

    BOOST_CHECK_EQUAL( defaultNetclass->GetClearance(), PCB_IO_EASYEDA_PARSER( nullptr ).ScaleSize( 0.6 ) );
    BOOST_CHECK( board->m_LegacyNetclassesLoaded );
}


BOOST_AUTO_TEST_SUITE_END()

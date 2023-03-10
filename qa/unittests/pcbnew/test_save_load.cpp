/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <filesystem>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <board.h>
#include <settings/settings_manager.h>


struct SAVE_LOAD_TEST_FIXTURE
{
    SAVE_LOAD_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( RegressionSaveLoadTests, SAVE_LOAD_TEST_FIXTURE )
{
    std::vector<wxString> tests = { "issue18",
                                    "issue832",
                                    "issue2568",
                                    "issue5313",
                                    "issue5854",
                                    "issue6260",
                                    "issue6945",
                                    "issue7267",
                                    "issue8003" };

    auto savePath = std::filesystem::temp_directory_path() / "group_saveload_tst.kicad_pcb";

    for( const wxString& relPath : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );
        KI_TEST::DumpBoardToFile( *m_board.get(), savePath.string() );

        std::unique_ptr<BOARD> board2 = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    }
}


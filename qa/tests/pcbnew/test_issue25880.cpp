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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <footprint.h>
#include <pcb_field.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>

#include <filesystem>

// Regression test for https://gitlab.com/kicad/code/kicad/-/work_items/25880
//
// Saving a scaled footprint unbaked and rebaked text size through a lossy round trip, so the live
// text and the saved library-frame size both drifted by a nanometre.

BOOST_AUTO_TEST_SUITE( Issue25880 )


BOOST_AUTO_TEST_CASE( SaveKeepsFootprintTextLibrarySize )
{
    SETTINGS_MANAGER       settingsManager;
    std::unique_ptr<BOARD> board;

    KI_TEST::LoadBoard( settingsManager, "issue18", board );

    FOOTPRINT* fp = *board->Footprints().begin();
    BOOST_REQUIRE( fp );

    fp->SetTransformScale( 1.0 / 3.0, 1.0 / 3.0 );

    const VECTOR2I libSize = fp->Reference().EDA_TEXT::GetTextSize();
    const int      libThickness = fp->Reference().EDA_TEXT::GetTextThickness();

    const std::filesystem::path savePath = std::filesystem::temp_directory_path() / "issue25880.kicad_pcb";

    KI_TEST::DumpBoardToFile( *board, savePath );

    BOOST_CHECK( fp->Reference().EDA_TEXT::GetTextSize() == libSize );
    BOOST_CHECK_EQUAL( fp->Reference().EDA_TEXT::GetTextThickness(), libThickness );

    std::unique_ptr<BOARD> reloaded = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( reloaded );

    FOOTPRINT* fp2 = *reloaded->Footprints().begin();
    BOOST_REQUIRE( fp2 );

    BOOST_CHECK( fp2->Reference().EDA_TEXT::GetTextSize() == libSize );
    BOOST_CHECK_EQUAL( fp2->Reference().EDA_TEXT::GetTextThickness(), libThickness );

    std::filesystem::remove( savePath );
}


BOOST_AUTO_TEST_SUITE_END()

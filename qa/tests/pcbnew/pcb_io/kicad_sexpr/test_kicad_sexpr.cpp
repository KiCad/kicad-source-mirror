/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <string>

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

#include <board.h>
#include <zone.h>


struct KICAD_SEXPR_FIXTURE
{
    KICAD_SEXPR_FIXTURE() {}

    PCB_IO_KICAD_SEXPR kicadPlugin;
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( KiCadSexprIO, KICAD_SEXPR_FIXTURE )


/**
 * Compare all footprints declared in a *.lbr file with their KiCad reference footprint
 */
BOOST_AUTO_TEST_CASE( Issue19775_ZoneLayerWildcards )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/kicad_sexpr/Issue19775_ZoneLayers/";

    BOOST_TEST_CONTEXT( "Zone layers with wildcards" )
    {
        std::unique_ptr<BOARD> testBoard = std::make_unique<BOARD>();

        kicadPlugin.LoadBoard( dataPath + "LayerWildcard.kicad_pcb", testBoard.get() );

        // One zone in the file
        BOOST_CHECK( testBoard->Zones().size() == 1 );

        ZONE* z = testBoard->Zones()[0];

        // On both front and back layers, with zone fill on both
        BOOST_CHECK( z->GetLayerSet().Contains( F_Cu ) && z->GetLayerSet().Contains( B_Cu ) );
        BOOST_CHECK( z->GetFilledPolysList( F_Cu )->TotalVertices() > 0 );
        BOOST_CHECK( z->GetFilledPolysList( B_Cu )->TotalVertices() > 0 );
    }

    BOOST_TEST_CONTEXT( "Round trip layers" )
    {
        auto tmpBoard = std::filesystem::temp_directory_path() / "Issue19775_RoundTrip.kicad_pcb";

        // Load and save the board from above to test how we write the zones into it
        {
            std::unique_ptr<BOARD> testBoard = std::make_unique<BOARD>();
            kicadPlugin.LoadBoard( dataPath + "LayerEnumerate.kicad_pcb", testBoard.get() );
            kicadPlugin.SaveBoard( tmpBoard.string(), testBoard.get() );
        }

        // Read the new board
        std::unique_ptr<BOARD> testBoard = std::make_unique<BOARD>();
        kicadPlugin.LoadBoard( tmpBoard.string(), testBoard.get() );

        // One zone in the file
        BOOST_CHECK( testBoard->Zones().size() == 1 );

        ZONE* z = testBoard->Zones()[0];

        // On both front and back layers, with zone fill on both
        BOOST_CHECK( z->GetLayerSet().Contains( F_Cu ) && z->GetLayerSet().Contains( B_Cu ) );
        BOOST_CHECK( z->GetFilledPolysList( F_Cu )->TotalVertices() > 0 );
        BOOST_CHECK( z->GetFilledPolysList( B_Cu )->TotalVertices() > 0 );
        BOOST_CHECK( z->LayerProperties().contains( F_Cu ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()

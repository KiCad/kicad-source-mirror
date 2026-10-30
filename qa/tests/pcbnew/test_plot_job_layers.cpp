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
#include <pcbnew_utils/board_file_utils.h>

#include <board.h>
#include <jobs/job_export_pcb_gerbers.h>
#include <json_common.h>
#include <pcb_plotter.h>


namespace
{

std::unique_ptr<BOARD> loadBoard( const std::string& aName )
{
    return KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir() + aName );
}

} // namespace


BOOST_AUTO_TEST_SUITE( PlotJobLayers )


// A fab drawing job saved on a 2-layer board must not plot inner copper on a 4-layer board
BOOST_AUTO_TEST_CASE( JobDialogKeepsOnlyShownLayers )
{
    std::unique_ptr<BOARD> twoLayer = loadBoard( "issue17559.kicad_pcb" );
    std::unique_ptr<BOARD> fourLayer = loadBoard( "issue22536.kicad_pcb" );
    BOOST_REQUIRE( twoLayer && fourLayer );
    BOOST_REQUIRE_EQUAL( twoLayer->GetCopperLayerCount(), 2 );
    BOOST_REQUIRE_EQUAL( fourLayer->GetCopperLayerCount(), 4 );

    LSET stored = PCB_PLOTTER::DialogLayerSelection( LSET( { F_Fab } ), twoLayer->GetEnabledLayers(), true );

    BOOST_CHECK_EQUAL( ( stored & fourLayer->GetEnabledLayers() ).FmtHex(), LSET( { F_Fab } ).FmtHex() );
}


// A job listing every copper layer, re-saved on a 2-layer board, must still plot all copper of a 4-layer board
BOOST_AUTO_TEST_CASE( AllCopperLayersSurviveSmallerBoard )
{
    std::unique_ptr<BOARD> twoLayer = loadBoard( "issue17559.kicad_pcb" );
    std::unique_ptr<BOARD> fourLayer = loadBoard( "issue22536.kicad_pcb" );
    BOOST_REQUIRE( twoLayer && fourLayer );

    // Older plot job dialogs wrote every copper layer and no all_copper_layers key
    nlohmann::json legacy;
    {
        JOB_EXPORT_PCB_GERBERS job;
        LSET layers = LSET::AllCuMask() | LSET( { F_Mask, B_Mask, Edge_Cuts } );
        job.m_plotLayerSequence = layers.SeqStackupForPlotting();
        job.ToJson( legacy );
        legacy.erase( "all_copper_layers" );
    }

    JOB_EXPORT_PCB_GERBERS gerberJob;
    gerberJob.FromJson( legacy );
    gerberJob.m_plotLayerSequence = PCB_PLOTTER::DialogLayerSelection( LSET( gerberJob.m_plotLayerSequence ),
                                                                       twoLayer->GetEnabledLayers(), true )
                                            .SeqStackupForPlotting();

    nlohmann::json saved;
    gerberJob.ToJson( saved );

    JOB_EXPORT_PCB_GERBERS reloaded;
    reloaded.FromJson( saved );

    LSET plotted = LSET( reloaded.ResolvePlotLayers( fourLayer->GetEnabledLayers() ) );
    LSET fourLayerCopper = fourLayer->GetEnabledLayers() & LSET::AllCuMask();

    BOOST_CHECK_EQUAL( ( plotted & LSET::AllCuMask() ).FmtHex(), fourLayerCopper.FmtHex() );
}


BOOST_AUTO_TEST_SUITE_END()

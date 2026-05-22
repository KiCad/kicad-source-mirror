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
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa_utils/pdf_test_utils.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcbplot.h>
#include <plotters/plotter_gerber.h>
#include <pcb_plot_params.h>
#include <layer_ids.h>
#include <lset.h>

#include <wx/filename.h>
#include <wx/ffile.h>

#include <memory>
#include <regex>
#include <string>

BOOST_AUTO_TEST_SUITE( PlotEdgeCutsDrillMarks )

// Regression for #24416: drill marks must never be flashed onto a layer the pad
// isn't on (Edge_Cuts, paste, silk).  Plot Edge_Cuts with drill marks enabled and
// assert the gerber contains no flash (D03) apertures.
BOOST_AUTO_TEST_CASE( NoDrillFlashesOnEdgeCuts )
{
    const int padDia = pcbIUScale.mmToIU( 1.4 );
    const int drill = pcbIUScale.mmToIU( 0.8 );

    BOARD board;
    auto  footprint = std::make_unique<FOOTPRINT>( &board );
    footprint->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 50.0 ), pcbIUScale.mmToIU( 50.0 ) ) );

    auto pad = new PAD( footprint.get() );
    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( padDia, padDia ) );
    pad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
    pad->SetDrillSize( VECTOR2I( drill, drill ) );
    pad->SetLayerSet( LSET::AllCuMask() | LSET( { F_Mask, B_Mask } ) ); // THT pad: NOT on Edge_Cuts
    pad->SetPosition( footprint->GetPosition() );
    footprint->Add( pad );
    board.Add( footprint.release() );

    GERBER_PLOTTER         plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;
    plotter.SetRenderSettings( &renderSettings );

    wxString gbrPath = wxFileName::CreateTempFileName( wxT( "kicad_gbr_24416" ) );
    BOOST_REQUIRE( !gbrPath.IsEmpty() );
    BOOST_REQUIRE( plotter.OpenFile( gbrPath ) );
    plotter.SetViewport( VECTOR2I( 0, 0 ), pcbIUScale.IU_PER_MILS / 10, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ) ) );

    PCB_PLOT_PARAMS plotOpts;
    plotOpts.SetFormat( PLOT_FORMAT::GERBER );
    plotOpts.SetDrillMarksType( DRILL_MARKS::FULL_DRILL_SHAPE ); // triggers PlotDrillMarks()

    PlotBoardLayers( &board, &plotter, LSEQ{ Edge_Cuts }, plotOpts );
    BOOST_REQUIRE( plotter.EndPlot() );

    wxFFile file( gbrPath, wxT( "rb" ) );
    BOOST_REQUIRE( file.IsOpened() );
    wxString contents;
    BOOST_REQUIRE( file.ReadAll( &contents ) );
    file.Close();
    wxRemoveFile( gbrPath );

    std::string buf = contents.ToStdString();
    std::regex  flashRe( R"(D0*3\*)" ); // D03 = flash a pad/aperture
    long flashes = std::distance( std::sregex_iterator( buf.begin(), buf.end(), flashRe ), std::sregex_iterator() );

    BOOST_CHECK_MESSAGE( flashes == 0,
                         "Edge_Cuts gerber unexpectedly contains " << flashes << " drill-mark flash(es) (#24416)" );
}

BOOST_AUTO_TEST_SUITE_END()

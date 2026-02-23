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

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <memory>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_text.h>
#include <pcbplot.h>
#include <plotcontroller.h>
#include <pcb_plot_params.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>

namespace
{

struct PLOT_CONTROLLER_FIXTURE
{
    PLOT_CONTROLLER_FIXTURE()
    {
        KI_TEST::LoadBoard( m_settingsManager, "complex_hierarchy", m_board );
    }

    ~PLOT_CONTROLLER_FIXTURE()
    {
        // Clean up temp files
        for( const wxString& path : m_tempFiles )
        {
            if( wxFileExists( path ) )
                wxRemoveFile( path );
        }
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
    std::vector<wxString>  m_tempFiles;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( PlotController, PLOT_CONTROLLER_FIXTURE )


/**
 * Test that plotting multiple layers sequentially via PLOT_CONTROLLER does not crash.
 * This reproduces GitLab issue #23171 where plotting 2 layers from the Python API
 * causes a SIGSEGV because Pgm() is called during a code path that is only valid
 * in a GUI context.
 */
BOOST_AUTO_TEST_CASE( MultiLayerDxfPlot )
{
    BOOST_REQUIRE( m_board );
    BOOST_REQUIRE( m_board->Footprints().size() > 0 );

    PLOT_CONTROLLER plotController( m_board.get() );

    wxString tempDir = wxFileName::GetTempDir();

    // Plot F.Cu (copper layer - uses PlotLayerOutlines path for DXF)
    plotController.SetLayer( F_Cu );
    bool opened = plotController.OpenPlotfile( wxT( "test_fcu" ), PLOT_FORMAT::DXF, wxT( "test" ) );
    BOOST_REQUIRE( opened );

    m_tempFiles.push_back( plotController.GetPlotFileName() );

    BOOST_CHECK_NO_THROW( plotController.PlotLayer() );
    plotController.ClosePlot();

    // Plot F.Fab (non-copper layer - uses PlotStandardLayer path for DXF)
    // This is the plot that triggers the crash in issue #23171
    plotController.SetLayer( F_Fab );
    opened = plotController.OpenPlotfile( wxT( "test_ffab" ), PLOT_FORMAT::DXF, wxT( "test" ) );
    BOOST_REQUIRE( opened );

    m_tempFiles.push_back( plotController.GetPlotFileName() );

    BOOST_CHECK_NO_THROW( plotController.PlotLayer() );
    plotController.ClosePlot();
}


/**
 * Test plotting multiple layers with SVG format.
 */
BOOST_AUTO_TEST_CASE( MultiLayerSvgPlot )
{
    BOOST_REQUIRE( m_board );

    PLOT_CONTROLLER plotController( m_board.get() );

    plotController.SetLayer( F_Cu );
    bool opened = plotController.OpenPlotfile( wxT( "test_fcu" ), PLOT_FORMAT::SVG, wxT( "test" ) );
    BOOST_REQUIRE( opened );

    m_tempFiles.push_back( plotController.GetPlotFileName() );

    BOOST_CHECK_NO_THROW( plotController.PlotLayer() );
    plotController.ClosePlot();

    plotController.SetLayer( F_Fab );
    opened = plotController.OpenPlotfile( wxT( "test_ffab" ), PLOT_FORMAT::SVG, wxT( "test" ) );
    BOOST_REQUIRE( opened );

    m_tempFiles.push_back( plotController.GetPlotFileName() );

    BOOST_CHECK_NO_THROW( plotController.PlotLayer() );
    plotController.ClosePlot();
}


/**
 * Test plotting multiple layers with Gerber format.
 */
BOOST_AUTO_TEST_CASE( MultiLayerGerberPlot )
{
    BOOST_REQUIRE( m_board );

    PLOT_CONTROLLER plotController( m_board.get() );

    plotController.SetLayer( F_Cu );
    bool opened = plotController.OpenPlotfile( wxT( "test_fcu" ), PLOT_FORMAT::GERBER, wxT( "test" ) );
    BOOST_REQUIRE( opened );

    m_tempFiles.push_back( plotController.GetPlotFileName() );

    BOOST_CHECK_NO_THROW( plotController.PlotLayer() );
    plotController.ClosePlot();

    plotController.SetLayer( F_Fab );
    opened = plotController.OpenPlotfile( wxT( "test_ffab" ), PLOT_FORMAT::GERBER, wxT( "test" ) );
    BOOST_REQUIRE( opened );

    m_tempFiles.push_back( plotController.GetPlotFileName() );

    BOOST_CHECK_NO_THROW( plotController.PlotLayer() );
    plotController.ClosePlot();
}


/**
 * Test plotting multiple layers with PDF format (should work without crash).
 */
BOOST_AUTO_TEST_CASE( MultiLayerPdfPlot )
{
    BOOST_REQUIRE( m_board );

    PLOT_CONTROLLER plotController( m_board.get() );

    plotController.SetLayer( F_Cu );
    bool opened = plotController.OpenPlotfile( wxT( "test_fcu" ), PLOT_FORMAT::PDF, wxT( "test" ) );
    BOOST_REQUIRE( opened );

    m_tempFiles.push_back( plotController.GetPlotFileName() );

    BOOST_CHECK_NO_THROW( plotController.PlotLayer() );
    plotController.ClosePlot();

    plotController.SetLayer( F_Fab );
    opened = plotController.OpenPlotfile( wxT( "test_ffab" ), PLOT_FORMAT::PDF, wxT( "test" ) );
    BOOST_REQUIRE( opened );

    m_tempFiles.push_back( plotController.GetPlotFileName() );

    BOOST_CHECK_NO_THROW( plotController.PlotLayer() );
    plotController.ClosePlot();
}


BOOST_AUTO_TEST_SUITE_END()

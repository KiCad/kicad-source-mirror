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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcbplot.h>
#include <plotters/plotter_gerber.h>
#include <pcb_plot_params.h>
#include <geometry/shape_poly_set.h>
#include <qa_utils/pdf_test_utils.h>

#include <wx/filename.h>
#include <wx/ffile.h>

#include <memory>
#include <string>
#include <cmath>
#include <regex>


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24327
//
// When plotting the solder mask layer for an SMD rounded-rectangle pad, the
// expanded outline must be the Minkowski sum of the rounded rectangle with a
// disk of radius equal to the solder-mask clearance. That sum is itself a
// rounded rectangle whose sides grow by 2*clearance and whose corner radius
// grows by clearance. The buggy code preserved the round_rect_radius_ratio
// when resizing the pad, producing a corner radius that scaled with the
// inflated size instead of growing linearly.
BOOST_AUTO_TEST_SUITE( PlotPadMask )


// Geometric invariant: when the lambda inside PlotStandardLayer mutates a
// ROUNDRECT pad for mask plotting, the resulting pad outline must equal the
// original pad outline inflated by mask_clearance.
BOOST_AUTO_TEST_CASE( RoundRectMaskMatchesInflatedOutline )
{
    const int padW            = pcbIUScale.mmToIU( 2.0 );
    const int padH            = pcbIUScale.mmToIU( 2.0 );
    const int maskClearance   = pcbIUScale.mmToIU( 1.0 );
    const double radiusRatio  = 0.125;
    const int originalRadius  = KiROUND( std::min( padW, padH ) * radiusRatio );
    const int maxError        = pcbIUScale.mmToIU( 0.005 );

    BOARD board;
    auto footprint = std::make_unique<FOOTPRINT>( &board );

    auto pad = new PAD( footprint.get() );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::ROUNDRECT );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( padW, padH ) );
    pad->SetRoundRectRadiusRatio( PADSTACK::ALL_LAYERS, radiusRatio );
    pad->SetLayerSet( LSET( { F_Cu, F_Mask } ) );
    pad->SetPosition( VECTOR2I( 0, 0 ) );
    footprint->Add( pad );

    // Reference outline: PAD::TransformShapeToPolygon with clearance correctly
    // expands the rounded rectangle by the Minkowski sum.
    SHAPE_POLY_SET expected;
    pad->TransformShapeToPolygon( expected, F_Mask, maskClearance, maxError, ERROR_INSIDE );

    // Replicate the fixed plot-path mutation: grow the pad to padPlotsSize and
    // grow the corner radius by mask_clearance.
    pad->SetSize( PADSTACK::ALL_LAYERS,
                  VECTOR2I( padW + 2 * maskClearance, padH + 2 * maskClearance ) );
    pad->SetRoundRectCornerRadius( PADSTACK::ALL_LAYERS, originalRadius + maskClearance );

    SHAPE_POLY_SET produced;
    pad->TransformShapeToPolygon( produced, F_Mask, 0, maxError, ERROR_INSIDE );

    const BOX2I expectedBB = expected.BBox();
    const BOX2I producedBB = produced.BBox();
    BOOST_CHECK_EQUAL( expectedBB.GetWidth(), producedBB.GetWidth() );
    BOOST_CHECK_EQUAL( expectedBB.GetHeight(), producedBB.GetHeight() );

    SHAPE_POLY_SET diff = expected;
    diff.BooleanSubtract( produced );
    SHAPE_POLY_SET reverseDiff = produced;
    reverseDiff.BooleanSubtract( expected );

    // Tolerance is 1 IU^2-equivalent (chord/arc approximation slack)
    const double tolerance = std::pow( pcbIUScale.mmToIU( 0.001 ), 2.0 );
    BOOST_CHECK_LE( std::abs( diff.Area() ), tolerance );
    BOOST_CHECK_LE( std::abs( reverseDiff.Area() ), tolerance );
}


// End-to-end test: emit a Gerber file for the F.Mask layer of a board with a
// roundrect SMD pad and verify the mask aperture has the geometrically
// correct corner radius. With the bug, the corner radius (visible in the
// arc I/J offsets) would scale with the inflated pad size, producing a ratio
// to the bounding box equal to radius_ratio rather than the geometrically
// correct (original_radius + mask_clearance) / (size + 2 * mask_clearance).
BOOST_AUTO_TEST_CASE( RoundRectGerberMaskApertureHasCorrectRadius )
{
    const int padW            = pcbIUScale.mmToIU( 2.0 );
    const int padH            = pcbIUScale.mmToIU( 2.0 );
    const int maskClearance   = pcbIUScale.mmToIU( 1.0 );
    const double radiusRatio  = 0.125;
    const int originalRadius  = KiROUND( std::min( padW, padH ) * radiusRatio );

    BOARD board;
    board.GetDesignSettings().m_SolderMaskExpansion = 0;

    auto footprint = std::make_unique<FOOTPRINT>( &board );
    footprint->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 50.0 ),
                                      pcbIUScale.mmToIU( 50.0 ) ) );

    auto pad = new PAD( footprint.get() );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::ROUNDRECT );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( padW, padH ) );
    pad->SetRoundRectRadiusRatio( PADSTACK::ALL_LAYERS, radiusRatio );
    pad->SetLayerSet( LSET( { F_Cu, F_Mask } ) );
    pad->SetPosition( footprint->GetPosition() );
    pad->SetLocalSolderMaskMargin( maskClearance );
    footprint->Add( pad );
    board.Add( footprint.release() );

    GERBER_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;
    plotter.SetRenderSettings( &renderSettings );

    // Disable aperture macros so the roundrect is emitted as a Gerber region
    // whose arc I/J offsets we can read directly.
    plotter.UseX2format( true );
    plotter.DisableApertMacros( true );

    wxString gbrPath = wxFileName::CreateTempFileName( wxT( "kicad_gbr_24327" ) );
    BOOST_REQUIRE( !gbrPath.IsEmpty() );
    BOOST_TEST_MESSAGE( "Gerber output: " << gbrPath.ToStdString() );
    BOOST_REQUIRE( plotter.OpenFile( gbrPath ) );

    plotter.SetViewport( VECTOR2I( 0, 0 ), pcbIUScale.IU_PER_MILS / 10, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ) ) );

    PCB_PLOT_PARAMS plotOpts;
    plotOpts.SetFormat( PLOT_FORMAT::GERBER );
    plotOpts.SetUseGerberX2format( true );
    plotOpts.SetDisableGerberMacros( true );

    PlotStandardLayer( &board, &plotter, LSET( { F_Mask } ), plotOpts );

    BOOST_REQUIRE( plotter.EndPlot() );

    wxFFile file( gbrPath, wxT( "rb" ) );
    BOOST_REQUIRE( file.IsOpened() );
    wxFileOffset len = file.Length();
    BOOST_REQUIRE_GT( len, 0 );

    std::string buffer;
    buffer.resize( static_cast<size_t>( len ) );
    BOOST_REQUIRE_EQUAL( file.Read( buffer.data(), len ), static_cast<size_t>( len ) );
    file.Close();

    // Sample every D02/D01 coordinate to get the plotted region's bounding box,
    // and every arc command (X..Y..I..J..D01) to read the corner radius. We
    // care only about ratios, so we don't need to interpret the Gerber unit.
    std::regex coordRe( R"(X(-?\d+)Y(-?\d+)D0[12]\*)" );
    auto coordBegin = std::sregex_iterator( buffer.begin(), buffer.end(), coordRe );
    auto coordEnd   = std::sregex_iterator();

    long long minX = std::numeric_limits<long long>::max();
    long long maxX = std::numeric_limits<long long>::min();
    long long minY = std::numeric_limits<long long>::max();
    long long maxY = std::numeric_limits<long long>::min();
    int       coordCount = 0;

    for( auto it = coordBegin; it != coordEnd; ++it )
    {
        long long x = std::stoll( ( *it )[1] );
        long long y = std::stoll( ( *it )[2] );
        minX = std::min( minX, x );
        maxX = std::max( maxX, x );
        minY = std::min( minY, y );
        maxY = std::max( maxY, y );
        coordCount++;
    }

    BOOST_REQUIRE_GT( coordCount, 0 );
    BOOST_CHECK_EQUAL( maxX - minX, maxY - minY );

    const long long bboxExtent = maxX - minX;
    BOOST_REQUIRE_GT( bboxExtent, 0 );

    // The expected corner radius is (originalRadius + maskClearance), and the
    // expected bounding-box extent is (padW + 2 * maskClearance). With the
    // bug the ratio would be radiusRatio (= 0.125). The correct ratio for the
    // chosen inputs is 1.25 / 4.0 = 0.3125.
    const double expectedRadiusRatio = static_cast<double>( originalRadius + maskClearance )
                                       / static_cast<double>( padW + 2 * maskClearance );
    const double buggyRadiusRatio    = radiusRatio;
    BOOST_TEST_MESSAGE( "Expected r/bbox " << expectedRadiusRatio
                                           << " buggy " << buggyRadiusRatio );

    std::regex arcRe( R"(X(-?\d+)Y(-?\d+)I(-?\d+)J(-?\d+)D01\*)" );
    auto arcBegin = std::sregex_iterator( buffer.begin(), buffer.end(), arcRe );
    auto arcEnd   = std::sregex_iterator();
    BOOST_REQUIRE( arcBegin != arcEnd );

    int arcCount = 0;
    for( auto it = arcBegin; it != arcEnd; ++it )
    {
        double i = std::stoll( ( *it )[3] );
        double j = std::stoll( ( *it )[4] );
        double r = std::hypot( i, j );
        double ratio = r / static_cast<double>( bboxExtent );

        BOOST_CHECK_MESSAGE( std::abs( ratio - expectedRadiusRatio ) < 1e-3,
                             "Arc radius/bbox ratio " << ratio
                                                      << " does not match expected "
                                                      << expectedRadiusRatio
                                                      << " (buggy value would be "
                                                      << buggyRadiusRatio << ")" );
        arcCount++;
    }

    BOOST_CHECK_EQUAL( arcCount, 4 );

    if( wxFileExists( gbrPath ) )
        wxRemoveFile( gbrPath );
}


BOOST_AUTO_TEST_SUITE_END()

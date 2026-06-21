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
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_field.h>
#include <pcb_text.h>
#include <pcbplot.h>
#include <plotters/plotter_gerber.h>
#include <pcb_plot_params.h>
#include <qa_utils/pdf_test_utils.h>

#include <wx/filename.h>
#include <wx/ffile.h>

#include <limits>
#include <regex>
#include <string>


// Plot F.SilkS to Gerber and return the bbox of all plotted coords. The box
// scales with the drawn text size.
static BOX2I plotSilkBBox( BOARD& aBoard )
{
    GERBER_PLOTTER         plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;
    plotter.SetRenderSettings( &renderSettings );

    wxString gbrPath = wxFileName::CreateTempFileName( wxT( "kicad_gbr_scaletext" ) );
    BOOST_REQUIRE( !gbrPath.IsEmpty() );
    BOOST_REQUIRE( plotter.OpenFile( gbrPath ) );

    plotter.SetViewport( VECTOR2I( 0, 0 ), pcbIUScale.IU_PER_MILS / 10, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ) ) );

    PCB_PLOT_PARAMS plotOpts;
    plotOpts.SetFormat( PLOT_FORMAT::GERBER );

    PlotStandardLayer( &aBoard, &plotter, LSET( { F_SilkS } ), plotOpts );

    BOOST_REQUIRE( plotter.EndPlot() );

    wxFFile file( gbrPath, wxT( "rb" ) );
    BOOST_REQUIRE( file.IsOpened() );
    wxFileOffset len = file.Length();

    std::string buffer;
    buffer.resize( static_cast<size_t>( len ) );
    BOOST_REQUIRE_EQUAL( file.Read( buffer.data(), len ), static_cast<size_t>( len ) );
    file.Close();
    wxRemoveFile( gbrPath );

    std::regex coordRe( R"(X(-?\d+)Y(-?\d+)D0[12]\*)" );
    auto       it = std::sregex_iterator( buffer.begin(), buffer.end(), coordRe );
    auto       end = std::sregex_iterator();

    long long minX = std::numeric_limits<long long>::max();
    long long maxX = std::numeric_limits<long long>::min();
    long long minY = std::numeric_limits<long long>::max();
    long long maxY = std::numeric_limits<long long>::min();

    for( ; it != end; ++it )
    {
        long long x = std::stoll( ( *it )[1] );
        long long y = std::stoll( ( *it )[2] );
        minX = std::min( minX, x );
        maxX = std::max( maxX, x );
        minY = std::min( minY, y );
        maxY = std::max( maxY, y );
    }

    if( minX > maxX )
        return BOX2I();

    return BOX2I( VECTOR2I( (int) minX, (int) minY ), VECTOR2I( (int) ( maxX - minX ), (int) ( maxY - minY ) ) );
}


BOOST_AUTO_TEST_SUITE( PlotFootprintScaledText )


// A scaled footprint's reference (REF**) must plot at the scaled size, not lib size.
BOOST_AUTO_TEST_CASE( ScaledFootprintReferencePlotsScaled )
{
    BOARD board;
    auto  footprint = std::make_unique<FOOTPRINT>( &board );
    footprint->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 50.0 ), pcbIUScale.mmToIU( 50.0 ) ) );

    PCB_FIELD& ref = footprint->Reference();
    ref.SetText( wxT( "U1" ) );
    ref.SetLayer( F_SilkS );
    ref.SetVisible( true );
    ref.SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );
    ref.SetTextThickness( pcbIUScale.mmToIU( 0.15 ) );

    footprint->Value().SetVisible( false );

    FOOTPRINT* fp = footprint.get();
    board.Add( footprint.release() );

    const BOX2I bbox1 = plotSilkBBox( board );
    BOOST_REQUIRE( bbox1.GetWidth() > 0 && bbox1.GetHeight() > 0 );

    fp->SetTransformScale( 2.0, 2.0 );

    const BOX2I bbox2 = plotSilkBBox( board );

    // Plotted text doubles. Slack covers stroke width and glyph metric rounding.
    BOOST_CHECK_CLOSE( (double) bbox2.GetWidth(), 2.0 * bbox1.GetWidth(), 12.0 );
    BOOST_CHECK_CLOSE( (double) bbox2.GetHeight(), 2.0 * bbox1.GetHeight(), 12.0 );
}


// Same guarantee for a plain graphic text item on silk.
BOOST_AUTO_TEST_CASE( ScaledFootprintGraphicTextPlotsScaled )
{
    BOARD board;
    auto  footprint = std::make_unique<FOOTPRINT>( &board );
    footprint->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 50.0 ), pcbIUScale.mmToIU( 50.0 ) ) );

    footprint->Reference().SetVisible( false );
    footprint->Value().SetVisible( false );

    PCB_TEXT* text = new PCB_TEXT( footprint.get() );
    text->SetText( wxT( "TEXT" ) );
    text->SetLayer( F_SilkS );
    text->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );
    text->SetTextThickness( pcbIUScale.mmToIU( 0.15 ) );
    footprint->Add( text );

    FOOTPRINT* fp = footprint.get();
    board.Add( footprint.release() );

    const BOX2I bbox1 = plotSilkBBox( board );
    BOOST_REQUIRE( bbox1.GetWidth() > 0 && bbox1.GetHeight() > 0 );

    fp->SetTransformScale( 2.0, 2.0 );

    const BOX2I bbox2 = plotSilkBBox( board );

    BOOST_CHECK_CLOSE( (double) bbox2.GetWidth(), 2.0 * bbox1.GetWidth(), 12.0 );
    BOOST_CHECK_CLOSE( (double) bbox2.GetHeight(), 2.0 * bbox1.GetHeight(), 12.0 );
}


BOOST_AUTO_TEST_SUITE_END()

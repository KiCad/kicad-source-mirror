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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <wx/string.h>

#include <plotters/plotters_pslike.h>
#include <render_settings.h>

#include <qa_utils/file_utils.h>
#include <qa_utils/pdf_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

BOOST_AUTO_TEST_SUITE( PDFCirclePlot )

namespace
{

struct CIRCLE_PATH
{
    char   m_PaintOp = '\0';
    double m_Radius = 0.0;
};


bool isPaintOperator( const std::string& aToken )
{
    return aToken == "s" || aToken == "b" || aToken == "f";
}


bool isPathOperator( const std::string& aToken )
{
    return aToken == "m" || aToken == "c" || isPaintOperator( aToken );
}


CIRCLE_PATH parseCirclePath( const std::string& aBuffer )
{
    CIRCLE_PATH        result;
    std::istringstream buffer( aBuffer );
    std::string        line;

    while( std::getline( buffer, line ) )
    {
        std::vector<std::string> tokens;
        std::istringstream       lineStream( line );
        std::string              token;

        while( lineStream >> token )
            tokens.push_back( token );

        if( tokens.size() < 27 || !isPaintOperator( tokens.back() ) )
            continue;

        std::vector<double> coordinates;

        for( const std::string& tok : tokens )
        {
            if( isPathOperator( tok ) )
                continue;

            try
            {
                coordinates.push_back( std::stod( tok ) );
            }
            catch( const std::exception& )
            {
                coordinates.clear();
                break;
            }
        }

        if( coordinates.size() != 26 )
            continue;

        double minX = coordinates[0];
        double maxX = coordinates[0];

        for( size_t ii = 0; ii < coordinates.size(); ii += 2 )
        {
            minX = std::min( minX, coordinates[ii] );
            maxX = std::max( maxX, coordinates[ii] );
        }

        result.m_PaintOp = tokens.back()[0];
        result.m_Radius = ( maxX - minX ) / 2.0;

        return result;
    }

    return result;
}


CIRCLE_PATH plotOneCircle( int aDiameter, FILL_T aFill, int aWidth, int aDefaultPenWidth,
                           double aIusPerDecimil = 1.0 )
{
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad_pdf_circle" ) );
    const wxString           pdfPath = tempDir.CreateChildFileStr( wxT( "output.pdf" ) );

    PDF_PLOTTER            plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    renderSettings.SetDefaultPenWidth( aDefaultPenWidth );
    plotter.SetRenderSettings( &renderSettings );

    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I( 0, 0 ), aIusPerDecimil, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "TestPage" ) ) );

    plotter.Circle( VECTOR2I( 0, 0 ), aDiameter, aFill, aWidth );

    plotter.EndPlot();

    std::string buffer;
    BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer ) );

    CIRCLE_PATH path = parseCirclePath( buffer );
    BOOST_REQUIRE_MESSAGE( path.m_PaintOp != '\0', "No circle path found in the PDF content" );

    return path;
}

} // namespace


BOOST_AUTO_TEST_CASE( StrokeNarrowerThanDiameterIsStroked )
{
    CIRCLE_PATH path = plotOneCircle( 20000, FILL_T::NO_FILL, 2000, 5000 );

    BOOST_CHECK_EQUAL( path.m_PaintOp, 's' );
    BOOST_CHECK_CLOSE( path.m_Radius, 10000.0, 0.01 );
}


BOOST_AUTO_TEST_CASE( FilledCircleWithOutlineIsFilledAndStroked )
{
    CIRCLE_PATH path = plotOneCircle( 20000, FILL_T::FILLED_SHAPE, 2000, 5000 );

    BOOST_CHECK_EQUAL( path.m_PaintOp, 'b' );
    BOOST_CHECK_CLOSE( path.m_Radius, 10000.0, 0.01 );
}


BOOST_AUTO_TEST_CASE( StrokeWiderThanDiameterIsFilledOnly )
{
    CIRCLE_PATH path = plotOneCircle( 2000, FILL_T::NO_FILL, 20000, 5000 );

    BOOST_CHECK_EQUAL( path.m_PaintOp, 'f' );
    BOOST_CHECK_CLOSE( path.m_Radius, 11000.0, 0.01 );
}


BOOST_AUTO_TEST_CASE( StrokeWiderThanDiameterUsesDeviceUnits )
{
    CIRCLE_PATH path = plotOneCircle( 2000, FILL_T::NO_FILL, 20000, 5000, 25.4 );

    BOOST_CHECK_EQUAL( path.m_PaintOp, 'f' );
    BOOST_CHECK_CLOSE( path.m_Radius, 11000.0 / 25.4, 0.01 );
}


BOOST_AUTO_TEST_CASE( DefaultPenWidthIsResolvedBeforeSizingTheDisk )
{
    CIRCLE_PATH path = plotOneCircle( 2000, FILL_T::NO_FILL, PLOTTER::USE_DEFAULT_LINE_WIDTH,
                                      20000 );

    BOOST_CHECK_EQUAL( path.m_PaintOp, 'f' );
    BOOST_CHECK_CLOSE( path.m_Radius, 11000.0, 0.01 );
}


BOOST_AUTO_TEST_CASE( FilledCircleWithoutOutlineIsFilledOnly )
{
    CIRCLE_PATH path = plotOneCircle( 20000, FILL_T::FILLED_SHAPE, 0, 5000 );

    BOOST_CHECK_EQUAL( path.m_PaintOp, 'f' );
    BOOST_CHECK_CLOSE( path.m_Radius, 10000.0, 0.01 );
}


BOOST_AUTO_TEST_SUITE_END()

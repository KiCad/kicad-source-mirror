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

#include <boost/test/unit_test.hpp>

#include <wx/filename.h>
#include <wx/image.h>

#include <plotters/plotter_png.h>
#include <geometry/shape_poly_set.h>
#include <gal/color4d.h>


BOOST_AUTO_TEST_SUITE( PngPlotter )


BOOST_AUTO_TEST_CASE( BasicOutput )
{
    PNG_PLOTTER plotter;
    plotter.SetPixelSize( 100, 100 );
    plotter.SetResolution( 300 );

    BOOST_CHECK( plotter.StartPlot( wxEmptyString ) );
    BOOST_CHECK( plotter.EndPlot() );

    wxString tempFile = wxFileName::CreateTempFileName( wxS( "png_test" ) ) + wxS( ".png" );
    BOOST_CHECK( plotter.SaveFile( tempFile ) );

    // Verify file exists
    BOOST_CHECK( wxFileExists( tempFile ) );

    // Verify dimensions via wxImage
    wxImage img( tempFile );
    BOOST_CHECK( img.IsOk() );
    BOOST_CHECK_EQUAL( img.GetWidth(), 100 );
    BOOST_CHECK_EQUAL( img.GetHeight(), 100 );

    wxRemoveFile( tempFile );
}


BOOST_AUTO_TEST_CASE( DrawRect )
{
    PNG_PLOTTER plotter;
    plotter.SetPixelSize( 100, 100 );
    plotter.SetResolution( 100 );
    plotter.SetColorMode( true );

    plotter.StartPlot( wxEmptyString );
    plotter.SetColor( KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 ) ); // Red
    plotter.Rect( VECTOR2I( 10, 10 ), VECTOR2I( 90, 90 ), FILL_T::FILLED_SHAPE, 0 );
    plotter.EndPlot();

    wxString tempFile = wxFileName::CreateTempFileName( wxS( "png_rect" ) ) + wxS( ".png" );
    BOOST_CHECK( plotter.SaveFile( tempFile ) );

    wxImage img( tempFile );
    BOOST_CHECK( img.IsOk() );

    // Center pixel should be red (or close to it, accounting for anti-aliasing)
    unsigned char r = img.GetRed( 50, 50 );
    unsigned char g = img.GetGreen( 50, 50 );
    unsigned char b = img.GetBlue( 50, 50 );
    BOOST_CHECK_GT( r, 200 ); // Should be mostly red
    BOOST_CHECK_LT( g, 50 );
    BOOST_CHECK_LT( b, 50 );

    wxRemoveFile( tempFile );
}


BOOST_AUTO_TEST_CASE( DrawShapePolySet )
{
    // Create a simple square polygon
    SHAPE_POLY_SET poly;
    poly.NewOutline();
    poly.Append( VECTOR2I( 0, 0 ) );
    poly.Append( VECTOR2I( 1000000, 0 ) ); // 1mm in nm
    poly.Append( VECTOR2I( 1000000, 1000000 ) );
    poly.Append( VECTOR2I( 0, 1000000 ) );

    PNG_PLOTTER plotter;
    plotter.SetPixelSize( 100, 100 );
    plotter.SetResolution( 254 ); // 254 DPI = 0.1mm per pixel

    // Set viewport for 1mm x 1mm area
    plotter.SetViewport( VECTOR2I( 0, 0 ), 254000, 1.0, false );

    plotter.StartPlot( wxEmptyString );
    plotter.SetColor( KIGFX::COLOR4D::BLACK );

    // Render the polygon
    for( int i = 0; i < poly.OutlineCount(); i++ )
    {
        const SHAPE_LINE_CHAIN& outline = poly.Outline( i );
        std::vector<VECTOR2I>   pts;

        for( int j = 0; j < outline.PointCount(); j++ )
            pts.push_back( outline.CPoint( j ) );

        plotter.PlotPoly( pts, FILL_T::FILLED_SHAPE, 0 );
    }

    plotter.EndPlot();

    wxString tempFile = wxFileName::CreateTempFileName( wxS( "png_polyset" ) ) + wxS( ".png" );
    BOOST_CHECK( plotter.SaveFile( tempFile ) );

    wxImage img( tempFile );
    BOOST_CHECK( img.IsOk() );

    wxRemoveFile( tempFile );
}


BOOST_AUTO_TEST_CASE( AntialiasControl )
{
    PNG_PLOTTER plotterAA;
    plotterAA.SetPixelSize( 100, 100 );
    plotterAA.SetAntialias( true );

    PNG_PLOTTER plotterNoAA;
    plotterNoAA.SetPixelSize( 100, 100 );
    plotterNoAA.SetAntialias( false );

    // Draw diagonal line on both
    plotterAA.StartPlot( wxEmptyString );
    plotterAA.SetCurrentLineWidth( 2 );
    plotterAA.SetColor( KIGFX::COLOR4D::BLACK );
    plotterAA.MoveTo( VECTOR2I( 0, 0 ) );
    plotterAA.FinishTo( VECTOR2I( 100, 100 ) );
    plotterAA.EndPlot();

    plotterNoAA.StartPlot( wxEmptyString );
    plotterNoAA.SetCurrentLineWidth( 2 );
    plotterNoAA.SetColor( KIGFX::COLOR4D::BLACK );
    plotterNoAA.MoveTo( VECTOR2I( 0, 0 ) );
    plotterNoAA.FinishTo( VECTOR2I( 100, 100 ) );
    plotterNoAA.EndPlot();

    wxString tempAA = wxFileName::CreateTempFileName( wxS( "png_aa" ) ) + wxS( ".png" );
    wxString tempNoAA = wxFileName::CreateTempFileName( wxS( "png_noaa" ) ) + wxS( ".png" );

    BOOST_CHECK( plotterAA.SaveFile( tempAA ) );
    BOOST_CHECK( plotterNoAA.SaveFile( tempNoAA ) );

    // Both files should exist
    BOOST_CHECK( wxFileExists( tempAA ) );
    BOOST_CHECK( wxFileExists( tempNoAA ) );

    wxRemoveFile( tempAA );
    wxRemoveFile( tempNoAA );
}


BOOST_AUTO_TEST_SUITE_END()

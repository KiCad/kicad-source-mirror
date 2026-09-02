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

#include <page_info.h>
#include <plotters/plotters_pslike.h>

#include <wx/ffile.h>
#include <wx/filename.h>

#include <set>
#include <string>
#include <vector>

namespace
{

/// Plot one filled shape at the requested colour and return the whole SVG document.
wxString plotFilledShape( const COLOR4D& aColor )
{
    wxFileName tempFile( wxFileName::CreateTempFileName( wxT( "kicad_svg_plotter" ) ) );
    tempFile.SetExt( wxT( "svg" ) );

    SVG_PLOTTER plotter;
    PAGE_INFO   pageInfo;

    pageInfo.SetWidthMils( 1000 );
    pageInfo.SetHeightMils( 1000 );
    plotter.SetPageSettings( pageInfo );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1, 1.0, false );
    plotter.SetColorMode( true );

    BOOST_REQUIRE( plotter.OpenFile( tempFile.GetFullPath() ) );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ) ) );

    plotter.SetColor( aColor );
    plotter.Rect( VECTOR2I( 100, 100 ), VECTOR2I( 500, 500 ), FILL_T::FILLED_SHAPE, 10 );

    BOOST_REQUIRE( plotter.EndPlot() );

    wxFFile file( tempFile.GetFullPath(), wxT( "r" ) );
    BOOST_REQUIRE( file.IsOpened() );

    wxString content;
    file.ReadAll( &content );
    file.Close();

    wxRemoveFile( tempFile.GetFullPath() );

    return content;
}


/// Every distinct fill-opacity value the document carries.
std::set<std::string> fillOpacities( const wxString& aSvg )
{
    const std::string     doc = aSvg.ToStdString();
    const std::string     key = "fill-opacity:";
    std::set<std::string> values;

    for( size_t pos = doc.find( key ); pos != std::string::npos; pos = doc.find( key, pos + 1 ) )
    {
        size_t start = pos + key.size();
        size_t end = doc.find_first_not_of( "0123456789.", start );

        values.insert( doc.substr( start, end - start ) );
    }

    return values;
}


std::set<std::string> strokeOpacities( const wxString& aSvg )
{
    const std::string     doc = aSvg.ToStdString();
    const std::string     key = "stroke-opacity:";
    std::set<std::string> values;

    for( size_t pos = doc.find( key ); pos != std::string::npos; pos = doc.find( key, pos + 1 ) )
    {
        size_t start = pos + key.size();
        size_t end = doc.find_first_not_of( "0123456789.", start );

        values.insert( doc.substr( start, end - start ) );
    }

    return values;
}

} // anonymous namespace


BOOST_AUTO_TEST_SUITE( SvgPlotter )


BOOST_AUTO_TEST_CASE( FillOpacityMatchesColorAlpha )
{
    std::set<std::string> values = fillOpacities( plotFilledShape( COLOR4D( 1.0, 0.0, 0.0, 0.5 ) ) );

    // The plotter writes m_precision (4) digits, so a half-transparent brush is exactly 0.5000
    BOOST_CHECK_EQUAL( values.count( "0.5000" ), 1 );
    BOOST_CHECK_EQUAL( values.count( "0.0000" ), 0 );
}


BOOST_AUTO_TEST_CASE( FillOpacityFullyOpaque )
{
    std::set<std::string> values = fillOpacities( plotFilledShape( COLOR4D( 0.0, 0.0, 1.0, 1.0 ) ) );

    BOOST_REQUIRE_EQUAL( values.count( "1.0000" ), 1 );

    // Nothing may be drawn semi-transparently when the brush is opaque
    for( const std::string& value : values )
        BOOST_CHECK_EQUAL( value, "1.0000" );
}


BOOST_AUTO_TEST_CASE( FillOpacityPreservedThroughSetColor )
{
    const std::vector<std::pair<double, std::string>> cases = {
        { 0.0, "0.0000" }, { 0.25, "0.2500" }, { 0.5, "0.5000" }, { 0.75, "0.7500" },
        { 1.0, "1.0000" }
    };

    for( const auto& [alpha, expected] : cases )
    {
        BOOST_TEST_CONTEXT( "alpha " << alpha )
        {
            std::set<std::string> values =
                    fillOpacities( plotFilledShape( COLOR4D( 0.5, 0.5, 0.5, alpha ) ) );

            BOOST_CHECK_EQUAL( values.count( expected ), 1 );

            // No other requested alpha may appear, or SetColor is not reaching the output
            for( const auto& [otherAlpha, other] : cases )
            {
                if( other != expected && other != "1.0000" )
                    BOOST_CHECK_EQUAL( values.count( other ), 0 );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( StrokeOpacityMatchesColorAlpha )
{
    std::set<std::string> values = strokeOpacities( plotFilledShape( COLOR4D( 1.0, 0.0, 0.0, 0.25 ) ) );

    BOOST_CHECK_EQUAL( values.count( "0.2500" ), 1 );
}


BOOST_AUTO_TEST_SUITE_END()

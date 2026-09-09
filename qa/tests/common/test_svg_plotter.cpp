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
#include <qa_utils/svg_test_utils.h>

#include <page_info.h>
#include <plotters/plotters_pslike.h>

#include <wx/ffile.h>
#include <wx/filename.h>

#include <memory>
#include <set>
#include <string>
#include <vector>


namespace utf = boost::unit_test;

namespace
{

/// Plot one filled shape at the requested colour and return the whole SVG document.
wxString plotFilledShape( const COLOR4D& aColor, bool aMirrored = false, bool aFitToBoard = false )
{
    wxFileName tempFile( wxFileName::CreateTempFileName( wxT( "kicad_svg_plotter" ) ) );
    tempFile.SetExt( wxT( "svg" ) );

    SVG_PLOTTER plotter;
    PAGE_INFO   pageInfo;

    pageInfo.SetWidthMils( 1000 );
    pageInfo.SetHeightMils( 1000 );
    plotter.SetPageSettings( pageInfo );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1, 1.0, aMirrored );

    // If fit to board, define the board at (20000, 20000) with size 10000x10000 IU
    if( aFitToBoard )
        plotter.SetPlotBBox( BOX2I( VECTOR2I( 20000, 20000 ), VECTOR2I( 10000, 10000 ) ) );

    plotter.SetColorMode( true );

    BOOST_REQUIRE( plotter.OpenFile( tempFile.GetFullPath() ) );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ) ) );

    plotter.SetColor( aColor );

    VECTOR2I shapeOrigin;
    VECTOR2I shapeSize{ 4000, 4000 };

    // Draw the shape in the page somewhere.
    if( aFitToBoard )
        shapeOrigin = VECTOR2I( 20000, 20000 );
    else
        shapeOrigin = VECTOR2I( 100, 100 );

    // Actually draw the shape.
    plotter.Rect( shapeOrigin, shapeOrigin + shapeSize, FILL_T::FILLED_SHAPE, 10 );

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


/**
 * Represents a plotted SVG document of the kind produced by plotFilledShape().
 */
class SVG_PLOT
{
public:
    SVG_PLOT( const wxString& aSvg )
    {
        tl::expected<wxXmlDocument, wxString> document = KI_TEST::LoadSvg( aSvg );

        if( !document.has_value() )
            BOOST_FAIL( document.error() );

        m_document = std::make_unique<wxXmlDocument>( std::move( *document ) );

        tl::expected<KI_TEST::SVG_VIEWBOX, wxString> viewBox = KI_TEST::ParseViewBox( *m_document->GetRoot() );

        if( !viewBox.has_value() )
            BOOST_FAIL( viewBox.error() );

        m_viewBox = *viewBox;
        m_rect = KI_TEST::FindFirstRect( *m_document->GetRoot() );
        BOOST_REQUIRE( m_rect );
    }

    const KI_TEST::SVG_VIEWBOX& ViewBox() const { return m_viewBox; }

    wxString RectAttribute( const wxString& aName ) const { return m_rect->GetAttribute( aName ); }

private:
    std::unique_ptr<wxXmlDocument> m_document;
    KI_TEST::SVG_VIEWBOX           m_viewBox = {};
    const wxXmlNode*               m_rect = nullptr;
};

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
        { 0.0, "0.0000" }, { 0.25, "0.2500" }, { 0.5, "0.5000" }, { 0.75, "0.7500" }, { 1.0, "1.0000" }
    };

    for( const auto& [alpha, expected] : cases )
    {
        BOOST_TEST_CONTEXT( "alpha " << alpha )
        {
            std::set<std::string> values = fillOpacities( plotFilledShape( COLOR4D( 0.5, 0.5, 0.5, alpha ) ) );

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
    const wxString        svg = plotFilledShape( COLOR4D( 1.0, 0.0, 0.0, 0.25 ) );
    std::set<std::string> values = strokeOpacities( svg );

    BOOST_CHECK_EQUAL( values.count( "0.2500" ), 1 );
}


static const auto kSvgValTol = utf::tolerance( 0.00001 );


/*
 * The default documents looks like this,
 * with a 400-mil on the 1000x1000 mil page
 *
 *         +---------+
 *         | +---+   |
 *         | |   |   |
 *         | +---+   |
 *         |         |
 *         +---------+
 */
BOOST_AUTO_TEST_CASE( DefaultPlotViewBoxIsOrigin, *kSvgValTol )
{
    const bool isMirrored = false;
    const bool isFitToBoard = false;

    const wxString              svg = plotFilledShape( COLOR4D( 1.0, 0.0, 0.0, 0.25 ), isMirrored, isFitToBoard );
    const SVG_PLOT              svgPlot( svg );
    const KI_TEST::SVG_VIEWBOX& viewBox = svgPlot.ViewBox();

    // The default plotter origin is at (0, 0) IU
    // And the page is 1000 mils square, so the viewBox is 25.4 * 25.4 mm
    BOOST_TEST( viewBox.m_X == 0.0 );
    BOOST_TEST( viewBox.m_Y == 0.0 );
    BOOST_TEST( viewBox.m_Width == 25.4 );
    BOOST_TEST( viewBox.m_Height == 25.4 );

    // The rectangle's left edge is 10 mils = 0.254 mm
    BOOST_TEST( svgPlot.RectAttribute( wxT( "x" ) ) == wxT( "0.254000" ) );
}


BOOST_AUTO_TEST_CASE( MirroredSvgPreservesPlotOrigin, *kSvgValTol )
{
    const bool                  isMirrored = true;
    const bool                  isFitToBoard = false;
    const wxString              svg = plotFilledShape( COLOR4D( 1.0, 0.0, 0.0, 0.25 ), isMirrored, isFitToBoard );
    const SVG_PLOT              svgPlot( svg );
    const KI_TEST::SVG_VIEWBOX& viewBox = svgPlot.ViewBox();

    // We didn't fit to the board, so the unmirrored page was from 0 to 25.4 mm (in x)
    // And the mirrored page is from -25.4 to 0 mm (in x)
    BOOST_TEST( viewBox.m_X == -25.4 );
    BOOST_TEST( viewBox.m_Y == 0.0 );
    BOOST_TEST( viewBox.m_Width == 25.4 );
    BOOST_TEST( viewBox.m_Height == 25.4 );

    // And the rectangle's left edge is -410 mils = -10.414 mm
    BOOST_TEST( svgPlot.RectAttribute( wxT( "x" ) ) == wxT( "-10.414000" ) );
}


BOOST_AUTO_TEST_CASE( FitToBoardUsesViewBox, *kSvgValTol )
{
    const bool                  isMirrored = false;
    const bool                  isFitToBoard = true;
    const wxString              svg = plotFilledShape( COLOR4D( 1.0, 0.0, 0.0, 0.25 ), isMirrored, isFitToBoard );
    const SVG_PLOT              svgPlot( svg );
    const KI_TEST::SVG_VIEWBOX& viewBox = svgPlot.ViewBox();

    // The board is 1000 mils square
    // And the viewBox is offset by 2000 mils
    BOOST_TEST( viewBox.m_X == 50.8 );
    BOOST_TEST( viewBox.m_Y == 50.8 );
    BOOST_TEST( viewBox.m_Width == 25.4 );
    BOOST_TEST( viewBox.m_Height == 25.4 );

    // The rectangle's left edge is +2000mils
    BOOST_TEST( svgPlot.RectAttribute( wxT( "x" ) ) == wxT( "50.800000" ) );
}


BOOST_AUTO_TEST_CASE( MirroredFitToBoardUsesMirroredViewBox, *kSvgValTol )
{
    const bool     isMirrored = true;
    const bool     isFitToBoard = true;
    const wxString svg = plotFilledShape( COLOR4D( 1.0, 0.0, 0.0, 0.25 ), isMirrored, isFitToBoard );
    const SVG_PLOT svgPlot( svg );

    const KI_TEST::SVG_VIEWBOX& viewBox = svgPlot.ViewBox();

    // For the mirrored case, the viewBox is flipped horizontally
    BOOST_TEST( viewBox.m_X == -76.2 );
    BOOST_TEST( viewBox.m_Y == 50.8 );
    BOOST_TEST( viewBox.m_Width == 25.4 );
    BOOST_TEST( viewBox.m_Height == 25.4 );

    // The rectangle spans 20000 to 24000 IU, which mirrors to -20000 to -24000 IU.
    BOOST_TEST( svgPlot.RectAttribute( wxT( "x" ) ) == wxT( "-60.960000" ) );
}


BOOST_AUTO_TEST_SUITE_END()

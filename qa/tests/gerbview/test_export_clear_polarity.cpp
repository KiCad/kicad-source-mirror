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

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <export_to_pcbnew.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <geometry/shape_line_chain.h>
#include <layer_ids.h>


static VECTOR2I MillimetrePoint( double aX, double aY )
{
    return VECTOR2I( KiROUND( aX * 1000 ), KiROUND( aY * 1000 ) );
}


static std::vector<SHAPE_LINE_CHAIN> ReadPolygons( const wxString& aBoard )
{
    std::vector<SHAPE_LINE_CHAIN> polygons;
    size_t                        pos = 0;

    while( ( pos = aBoard.find( wxT( "(gr_poly" ), pos ) ) != wxString::npos )
    {
        size_t           end = aBoard.find( wxT( "(stroke" ), pos );
        wxString         block = aBoard.Mid( pos, end - pos );
        SHAPE_LINE_CHAIN polygon;
        size_t           xy = 0;

        while( ( xy = block.find( wxT( "(xy " ), xy ) ) != wxString::npos )
        {
            wxString coords = block.Mid( xy + 4 );
            double   x = 0;
            double   y = 0;

            coords.BeforeFirst( ' ' ).ToCDouble( &x );
            coords.AfterFirst( ' ' ).BeforeFirst( ')' ).ToCDouble( &y );
            polygon.Append( MillimetrePoint( x, y ) );
            xy += 4;
        }

        polygon.SetClosed( true );
        polygons.push_back( polygon );
        pos = end;
    }

    return polygons;
}


static bool InsideAnyPolygon( const std::vector<SHAPE_LINE_CHAIN>& aPolygons, const VECTOR2I& aPoint )
{
    for( const SHAPE_LINE_CHAIN& polygon : aPolygons )
    {
        if( polygon.PointInside( aPoint ) )
            return true;
    }

    return false;
}


BOOST_AUTO_TEST_SUITE( ExportToPcbnew )


BOOST_AUTO_TEST_CASE( ClearRegionCutsCopperPlane )
{
    GERBER_FILE_IMAGE_LIST& images = GERBER_FILE_IMAGE_LIST::GetImagesList();
    GERBER_FILE_IMAGE*      image = new GERBER_FILE_IMAGE( 0 );
    std::string             fn = KI_TEST::GetTestDataRootDir() + "gerbview/clear_polarity_plane.gbr";

    BOOST_REQUIRE( image->LoadGerberFile( wxString::FromUTF8( fn ) ) );
    BOOST_REQUIRE_EQUAL( images.AddGbrImage( image, 0 ), 0 );

    int layerLookUpTable[GERBER_DRAWLAYERS_COUNT];
    std::fill( std::begin( layerLookUpTable ), std::end( layerLookUpTable ), (int) UNSELECTED_LAYER );
    layerLookUpTable[0] = F_Cu;

    wxString            boardFile = wxFileName::CreateTempFileName( wxT( "clear_polarity" ) );
    GBR_TO_PCB_EXPORTER exporter( &images, nullptr, boardFile );

    BOOST_REQUIRE( exporter.ExportPcb( layerLookUpTable, 2 ) );

    wxString board;
    wxFFile  file( boardFile );

    BOOST_REQUIRE( file.ReadAll( &board ) );
    file.Close();
    wxRemoveFile( boardFile );
    images.DeleteAllImages();

    std::vector<SHAPE_LINE_CHAIN> polygons = ReadPolygons( board );

    // The exported board has the Y axis pointing down.
    BOOST_CHECK_MESSAGE( !InsideAnyPolygon( polygons, MillimetrePoint( 10, -5 ) ),
                         "the copper plane still covers the cleared channel" );
    BOOST_CHECK( InsideAnyPolygon( polygons, MillimetrePoint( 10, -2 ) ) );
    BOOST_CHECK( InsideAnyPolygon( polygons, MillimetrePoint( 10, -8 ) ) );
}


BOOST_AUTO_TEST_SUITE_END()

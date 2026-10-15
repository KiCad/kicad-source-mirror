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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <bitmap_base.h>
#include <bitmap_tracing.h>
#include <eda_item.h>
#include <geometry/eda_angle.h>
#include <import_gfx/bitmap_import_plugin.h>
#include <import_gfx/graphics_importer_buffer.h>
#include <reporter.h>

#include <wx/imagjpeg.h>
#include <wx/mstream.h>

BOOST_AUTO_TEST_SUITE( BitmapTracing )


static wxImage makeImage( int aWidth, int aHeight, unsigned char aFill )
{
    wxImage image( aWidth, aHeight );
    image.SetRGB( wxRect( 0, 0, aWidth, aHeight ), aFill, aFill, aFill );
    return image;
}


static void fillRect( wxImage& aImage, const wxRect& aRect, unsigned char aValue )
{
    aImage.SetRGB( aRect, aValue, aValue, aValue );
}


BOOST_AUTO_TEST_CASE( ThresholdSplitsLightFromDark )
{
    wxImage image = makeImage( 16, 16, 200 );
    fillRect( image, wxRect( 8, 0, 8, 16 ), 20 );

    wxImage bilevel = ConvertImageToBilevel( image, 0.5, false );

    BOOST_CHECK_EQUAL( (int) bilevel.GetGreen( 2, 8 ), 255 );
    BOOST_CHECK_EQUAL( (int) bilevel.GetGreen( 12, 8 ), 0 );
}


BOOST_AUTO_TEST_CASE( InvertSwapsBlackAndWhite )
{
    wxImage image = makeImage( 16, 16, 200 );
    fillRect( image, wxRect( 8, 0, 8, 16 ), 20 );

    wxImage bilevel = ConvertImageToBilevel( image, 0.5, true );

    BOOST_CHECK_EQUAL( (int) bilevel.GetGreen( 2, 8 ), 0 );
    BOOST_CHECK_EQUAL( (int) bilevel.GetGreen( 12, 8 ), 255 );
}


BOOST_AUTO_TEST_CASE( TransparentPixelsDoNotBecomeBlack )
{
    wxImage image = makeImage( 16, 16, 0 );
    image.InitAlpha();

    for( int y = 0; y < 16; y++ )
    {
        for( int x = 0; x < 16; x++ )
            image.SetAlpha( x, y, x < 8 ? wxIMAGE_ALPHA_TRANSPARENT : wxIMAGE_ALPHA_OPAQUE );
    }

    wxImage bilevel = ConvertImageToBilevel( image, 0.5, false );

    BOOST_CHECK_MESSAGE( bilevel.GetGreen( 2, 8 ) == 255, "a transparent dark pixel must not be traced" );
    BOOST_CHECK_MESSAGE( bilevel.GetGreen( 12, 8 ) == 0, "an opaque dark pixel must be traced" );
}


BOOST_AUTO_TEST_CASE( RingTracesToOneOutlineWithItsHoleRemoved )
{
    wxImage image = makeImage( 64, 64, 255 );
    fillRect( image, wxRect( 12, 12, 40, 40 ), 0 );
    fillRect( image, wxRect( 22, 22, 20, 20 ), 255 );

    wxImage bilevel = ConvertImageToBilevel( image, 0.5, false );

    NULL_REPORTER  reporter;
    SHAPE_POLY_SET traced = TraceBilevelImage( bilevel, 1.0, 1.0, reporter );

    BOOST_REQUIRE( !traced.IsEmpty() );

    // Fracturing turns the hole into a slit, so the ring is a single outline.
    BOOST_CHECK_EQUAL( traced.OutlineCount(), 1 );

    const double expectedArea = ( 40.0 * 40.0 ) - ( 20.0 * 20.0 );
    BOOST_CHECK_CLOSE( std::abs( traced.Area() ), expectedArea, 5.0 );

    BOX2I bbox = traced.BBox();
    BOOST_CHECK_LE( std::abs( bbox.GetWidth() - 40 ), 2 );
    BOOST_CHECK_LE( std::abs( bbox.GetHeight() - 40 ), 2 );
}


struct PNG_FIXTURE
{
    PNG_FIXTURE()
    {
        if( !wxImage::FindHandler( wxBITMAP_TYPE_PNG ) )
            wxImage::AddHandler( new wxPNGHandler );
    }

    wxMemoryBuffer EncodeAsPng( const wxImage& aImage )
    {
        wxMemoryOutputStream stream;

        BOOST_REQUIRE( aImage.SaveFile( stream, wxBITMAP_TYPE_PNG ) );

        size_t         length = stream.GetLength();
        wxMemoryBuffer buffer( length );

        stream.CopyTo( buffer.GetWriteBuf( length ), length );
        buffer.SetDataLen( length );

        return buffer;
    }
};


BOOST_FIXTURE_TEST_CASE( PluginReportsSizeInMillimetres, PNG_FIXTURE )
{
    BITMAP_IMPORT_PLUGIN plugin;

    BOOST_REQUIRE( plugin.LoadFromMemory( EncodeAsPng( makeImage( 300, 150, 255 ) ) ) );

    // An image with no resolution is read at 300 DPI, so 300 pixels is one inch.
    BOOST_CHECK_CLOSE( plugin.GetImageWidth(), 25.4, 0.01 );
    BOOST_CHECK_CLOSE( plugin.GetImageHeight(), 12.7, 0.01 );
}


BOOST_FIXTURE_TEST_CASE( PluginReportsFilledPolygons, PNG_FIXTURE )
{
    wxImage image = makeImage( 300, 300, 255 );
    fillRect( image, wxRect( 60, 60, 60, 60 ), 0 );
    fillRect( image, wxRect( 180, 180, 60, 60 ), 0 );

    BITMAP_IMPORT_PLUGIN     plugin;
    GRAPHICS_IMPORTER_BUFFER importer;

    plugin.SetImporter( &importer );

    BOOST_REQUIRE( plugin.LoadFromMemory( EncodeAsPng( image ) ) );
    BOOST_REQUIRE( plugin.Import() );

    BOOST_CHECK_EQUAL( importer.GetShapes().size(), 2 );

    for( const std::unique_ptr<IMPORTED_SHAPE>& shape : importer.GetShapes() )
    {
        const IMPORTED_POLYGON* polygon = dynamic_cast<const IMPORTED_POLYGON*>( shape.get() );

        BOOST_REQUIRE_MESSAGE( polygon, "a traced area must arrive as a polygon" );
        BOOST_CHECK_MESSAGE( polygon->IsFilled(), "a traced area must arrive filled" );

        // Width 0 would be read as "use the default line width" and outline the fill.
        BOOST_CHECK_MESSAGE( polygon->GetStroke().GetWidth() == -1, "a traced area must arrive with no outline" );
    }
}


BOOST_FIXTURE_TEST_CASE( PluginKeepsDetailFinerThanAMillimetre, PNG_FIXTURE )
{
    wxImage image = makeImage( 300, 300, 255 );

    for( int bar = 0; bar < 3; bar++ )
        fillRect( image, wxRect( 100 + bar * 6, 100, 4, 100 ), 0 );

    BITMAP_IMPORT_PLUGIN     plugin;
    GRAPHICS_IMPORTER_BUFFER importer;

    plugin.SetImporter( &importer );

    BOOST_REQUIRE( plugin.LoadFromMemory( EncodeAsPng( image ) ) );
    BOOST_REQUIRE( plugin.Import() );

    BOOST_CHECK_EQUAL( importer.GetShapes().size(), 3 );

    for( const std::unique_ptr<IMPORTED_SHAPE>& shape : importer.GetShapes() )
    {
        BOX2D bbox = shape->GetBoundingBox();

        BOOST_CHECK_CLOSE( bbox.GetWidth(), 4.0 * 25.4 / 300.0, 15.0 );
        BOOST_CHECK_CLOSE( bbox.GetHeight(), 100.0 * 25.4 / 300.0, 5.0 );
    }
}


/// Load an image the way a board does, through encoded file data.
static void loadAsJpeg( BITMAP_BASE& aBitmap, const wxImage& aImage )
{
    if( !wxImage::FindHandler( wxBITMAP_TYPE_JPEG ) )
        wxImage::AddHandler( new wxJPEGHandler );

    wxMemoryOutputStream jpeg;
    BOOST_REQUIRE( aImage.SaveFile( jpeg, wxBITMAP_TYPE_JPEG ) );

    wxMemoryInputStream stream( jpeg );
    BOOST_REQUIRE( aBitmap.ReadImageFile( stream ) );
}


/// The bytes the image would be written to a board file with.
static std::string storedImageData( const BITMAP_BASE& aBitmap )
{
    wxMemoryOutputStream out;
    BOOST_REQUIRE( aBitmap.SaveImageData( out ) );

    std::string bytes( out.GetLength(), '\0' );
    out.CopyTo( bytes.data(), bytes.size() );

    return bytes;
}


BOOST_AUTO_TEST_CASE( APreviewPutsTheRealImageBack )
{
    wxImage source = makeImage( 40, 20, 255 );
    fillRect( source, wxRect( 0, 0, 10, 20 ), 0 );

    BITMAP_BASE bitmap;
    loadAsJpeg( bitmap, source );

    const KIID        id = bitmap.GetImageID();
    const std::string stored = storedImageData( bitmap );
    const wxImage     shown = *bitmap.GetImageData();

    bitmap.BeginPreviewImage( ConvertImageToBilevel( shown, 0.5, true ) );

    // Or a renderer caching by id would hand back the image the preview replaced.
    BOOST_CHECK_MESSAGE( bitmap.GetImageID() != id, "a preview must not keep the image id" );

    // A document can be written while a preview is up, by an autosave for one, and what it
    // saves has to be the picture rather than the stand-in shown over it.
    BOOST_CHECK_MESSAGE( storedImageData( bitmap ) == stored, "a preview must not change the stored data" );

    bitmap.EndPreviewImage();

    BOOST_CHECK_MESSAGE( bitmap.GetImageID() == id, "ending a preview must restore the image id" );
    BOOST_CHECK_EQUAL( (int) bitmap.GetImageData()->GetGreen( 2, 10 ), (int) shown.GetGreen( 2, 10 ) );

    // A preview the user cancelled must leave nothing to save, or the picture is encoded
    // again and a JPEG loses quality every time.
    BOOST_CHECK_MESSAGE( storedImageData( bitmap ) == stored, "ending a preview must restore the stored data" );
}


BOOST_AUTO_TEST_CASE( APreviewedTurnIsNotAppliedTwice )
{
    BITMAP_BASE bitmap;
    BOOST_REQUIRE( bitmap.SetImage( makeImage( 40, 20, 255 ) ) );

    bitmap.Rotate( true );

    BOOST_REQUIRE_EQUAL( bitmap.GetSizePixels().x, 20 );
    BOOST_REQUIRE_EQUAL( bitmap.GetOriginalImageData()->GetWidth(), 40 );

    const wxImage rotated = *bitmap.GetImageData();

    bitmap.BeginPreviewImage( rotated );

    BOOST_CHECK_MESSAGE( bitmap.Rotation() == ANGLE_0,
                         "a preview already holding the rotation must not be turned again" );

    bitmap.EndPreviewImage();

    BOOST_CHECK_MESSAGE( bitmap.Rotation() != ANGLE_0, "ending a preview must restore the rotation" );
    BOOST_CHECK_EQUAL( bitmap.GetSizePixels().x, 20 );
    BOOST_CHECK_EQUAL( bitmap.GetOriginalImageData()->GetWidth(), 40 );
}


BOOST_AUTO_TEST_CASE( AFailedRegionDoesNotSwallowTheRest )
{
    wxImage image = makeImage( 200, 200, 255 );

    // A hairline first, which cannot be normalised at this coarse scale, then a solid block.
    fillRect( image, wxRect( 20, 20, 1, 160 ), 0 );
    fillRect( image, wxRect( 120, 20, 60, 60 ), 0 );

    wxImage bilevel = ConvertImageToBilevel( image, 0.5, false );

    WX_STRING_REPORTER reporter;
    SHAPE_POLY_SET     traced = TraceBilevelImage( bilevel, 1.0, 1.0, reporter );

    bool foundBlock = false;

    for( int ii = 0; ii < traced.OutlineCount(); ii++ )
    {
        BOX2I bbox = traced.COutline( ii ).BBox();

        if( bbox.GetWidth() > 50 && bbox.GetHeight() > 50 )
            foundBlock = true;
    }

    BOOST_CHECK_MESSAGE( foundBlock, "a region traced after a failing one must survive" );

    // Whatever the tracer makes of the hairline, a region it drops has to be reported.
    BOOST_CHECK_MESSAGE( traced.OutlineCount() == 2 || reporter.HasMessage(),
                         "a region that could not be traced must be reported" );
}


BOOST_AUTO_TEST_SUITE_END()

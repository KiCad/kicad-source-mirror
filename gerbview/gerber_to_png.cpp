/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "gerber_to_png.h"
#include "gerber_file_image.h"
#include "gerber_draw_item.h"
#include "dcode.h"
#include "excellon_image.h"
#include "excellon_defaults.h"
#include <convert_basic_shapes_to_polygon.h>
#include <jobs/job_gerber_export_png.h>
#include <plotters/plotter_png.h>
#include <geometry/shape_poly_set.h>
#include <base_units.h>
#include <trigo.h>
#include <cmath>
#include <wx/filename.h>


bool IsExcellonFile( const wxString& aPath )
{
    wxFileName fn( aPath );
    wxString   ext = fn.GetExt().Lower();
    return ext == wxS( "drl" ) || ext == wxS( "xln" ) || ext == wxS( "exc" ) || ext == wxS( "ncd" );
}


BOX2I CalculateGerberBoundingBox( GERBER_FILE_IMAGE* aImage )
{
    BOX2I bbox;
    bool  first = true;

    for( GERBER_DRAW_ITEM* item : aImage->GetItems() )
    {
        BOX2I itemBox = item->GetBoundingBox();

        if( first )
        {
            bbox = itemBox;
            first = false;
        }
        else
        {
            bbox.Merge( itemBox );
        }
    }

    return bbox;
}


std::unique_ptr<GERBER_FILE_IMAGE> LoadGerberOrExcellon( const wxString& aPath, wxString* aErrorMsg,
                                                         wxArrayString* aMessages )
{
    std::unique_ptr<GERBER_FILE_IMAGE> image;

    if( IsExcellonFile( aPath ) )
    {
        auto              excellon = std::make_unique<EXCELLON_IMAGE>( 0 );
        EXCELLON_DEFAULTS defaults;

        if( !excellon->LoadFile( aPath, &defaults ) )
        {
            if( aErrorMsg )
                *aErrorMsg = wxString::Format( wxS( "Failed to load Excellon file: %s" ), aPath );

            return nullptr;
        }

        image = std::move( excellon );
    }
    else
    {
        image = std::make_unique<GERBER_FILE_IMAGE>( 0 );

        if( !image->LoadGerberFile( aPath ) )
        {
            if( aErrorMsg )
                *aErrorMsg = wxString::Format( wxS( "Failed to load Gerber file: %s" ), aPath );

            return nullptr;
        }
    }

    if( aMessages )
        *aMessages = image->GetMessages();

    return image;
}


namespace
{

/**
 * Render a single draw item to the plotter.
 */
void RenderItem( GERBER_DRAW_ITEM* aItem, PNG_PLOTTER& aPlotter, const KIGFX::COLOR4D& aColor )
{
    SHAPE_POLY_SET itemPoly;
    bool           needsFlashOffset = false;

    if( aItem->m_ShapeAsPolygon.OutlineCount() > 0 )
    {
        itemPoly = aItem->m_ShapeAsPolygon;
    }
    else if( aItem->m_ShapeType == GBR_SEGMENT )
    {
        D_CODE* dcode = aItem->GetDcodeDescr();

        if( dcode && dcode->m_ApertType != APT_RECT )
        {
            int arcError = static_cast<int>( gerbIUScale.IU_PER_MM * ARC_LOW_DEF_MM );
            TransformOvalToPolygon( itemPoly, aItem->m_Start, aItem->m_End,
                                    aItem->m_Size.x, arcError, ERROR_INSIDE );
        }
        else
        {
            aItem->ConvertSegmentToPolygon( &itemPoly );
        }
    }
    else if( aItem->m_ShapeType == GBR_ARC )
    {
        const int arcError = gerbIUScale.mmToIU( 0.005 );

        if( aItem->m_Start == aItem->m_End )
        {
            int radius = KiROUND( aItem->m_Start.Distance( aItem->m_ArcCentre ) );
            TransformRingToPolygon( itemPoly, aItem->m_ArcCentre, radius, aItem->m_Size.x,
                                    arcError, ERROR_INSIDE );
        }
        else
        {
            double startAngle = atan2( static_cast<double>( aItem->m_Start.y - aItem->m_ArcCentre.y ),
                                       static_cast<double>( aItem->m_Start.x - aItem->m_ArcCentre.x ) );
            double endAngle = atan2( static_cast<double>( aItem->m_End.y - aItem->m_ArcCentre.y ),
                                     static_cast<double>( aItem->m_End.x - aItem->m_ArcCentre.x ) );

            if( startAngle > endAngle )
                endAngle += 2.0 * M_PI;

            VECTOR2I mid = GetRotated( aItem->m_Start, aItem->m_ArcCentre,
                                       -EDA_ANGLE( ( endAngle - startAngle ) / 2.0, RADIANS_T ) );

            TransformArcToPolygon( itemPoly, aItem->m_Start, mid, aItem->m_End, aItem->m_Size.x,
                                   arcError, ERROR_INSIDE );
        }
    }
    else if( aItem->m_Flashed )
    {
        D_CODE* dcode = aItem->GetDcodeDescr();

        if( dcode )
        {
            dcode->ConvertShapeToPolygon( aItem );
            itemPoly = dcode->m_Polygon;
            needsFlashOffset = true;
        }
    }

    if( itemPoly.OutlineCount() == 0 )
        return;

    // Flashed shapes from ConvertShapeToPolygon are centered at (0,0).
    // Offset by the item's position before applying the AB transform.
    VECTOR2I offset = needsFlashOffset ? VECTOR2I( aItem->m_Start ) : VECTOR2I( 0, 0 );

    aPlotter.SetColor( aColor );

    for( int i = 0; i < itemPoly.OutlineCount(); i++ )
    {
        const SHAPE_LINE_CHAIN& outline = itemPoly.COutline( i );
        std::vector<VECTOR2I>   pts;
        pts.reserve( outline.PointCount() );

        for( int j = 0; j < outline.PointCount(); j++ )
            pts.push_back( aItem->GetABPosition( outline.CPoint( j ) + offset ) );

        if( pts.size() >= 3 )
            aPlotter.PlotPoly( pts, FILL_T::FILLED_SHAPE, 0 );
    }
}

} // anonymous namespace


GERBER_PLOTTER_VIEWPORT CalculatePlotterViewport( const BOX2I& aBBox, int aDpi, int aWidth, int aHeight )
{
    GERBER_PLOTTER_VIEWPORT vp;
    vp.width = aWidth;
    vp.height = aHeight;

    if( vp.width == 0 && vp.height == 0 )
    {
        double iuPerInch = gerbIUScale.IU_PER_MM * 25.4;
        double widthInches = static_cast<double>( aBBox.GetWidth() ) / iuPerInch;
        double heightInches = static_cast<double>( aBBox.GetHeight() ) / iuPerInch;

        vp.width = static_cast<int>( std::ceil( widthInches * aDpi ) );
        vp.height = static_cast<int>( std::ceil( heightInches * aDpi ) );

        if( vp.width < MIN_PIXEL_SIZE )
            vp.width = MIN_PIXEL_SIZE;

        if( vp.height < MIN_PIXEL_SIZE )
            vp.height = MIN_PIXEL_SIZE;
    }
    else if( vp.width == 0 )
    {
        double aspect = static_cast<double>( aBBox.GetWidth() ) / aBBox.GetHeight();
        vp.width = static_cast<int>( vp.height * aspect );
    }
    else if( vp.height == 0 )
    {
        double aspect = static_cast<double>( aBBox.GetHeight() ) / aBBox.GetWidth();
        vp.height = static_cast<int>( vp.width * aspect );
    }

    vp.iuPerDecimil = gerbIUScale.IU_PER_MILS / 10.0;

    double scaleX = static_cast<double>( vp.width ) * vp.iuPerDecimil * 10000.0 / ( aBBox.GetWidth() * aDpi );
    double scaleY = static_cast<double>( vp.height ) * vp.iuPerDecimil * 10000.0 / ( aBBox.GetHeight() * aDpi );
    vp.plotScale = std::min( scaleX, scaleY );
    vp.offset = aBBox.GetOrigin();

    return vp;
}


bool RenderGerberToPng( const wxString& aInputPath, const wxString& aOutputPath, const GERBER_RENDER_OPTIONS& aOptions,
                        wxString* aErrorMsg, wxArrayString* aMessages )
{
    auto image = LoadGerberOrExcellon( aInputPath, aErrorMsg, aMessages );

    if( !image )
        return false;

    if( image->GetItemsCount() == 0 )
    {
        if( aErrorMsg )
            *aErrorMsg = wxS( "Gerber file contains no draw items" );

        return false;
    }

    BOX2I bbox;

    if( aOptions.HasViewportOverride() )
    {
        // Viewport is specified in Gerber-native coordinates (Y increases upward).
        // KiCad stores gerber items with Y negated (Y increases downward), so
        // negate the origin Y and flip the window vertically.
        double iuPerMm = gerbIUScale.IU_PER_MM;
        int    ox = static_cast<int>( std::round( aOptions.originXMm * iuPerMm ) );
        int    oy = static_cast<int>( std::round(
                -( aOptions.originYMm + aOptions.windowHeightMm ) * iuPerMm ) );
        int    w = static_cast<int>( std::round( aOptions.windowWidthMm * iuPerMm ) );
        int    h = static_cast<int>( std::round( aOptions.windowHeightMm * iuPerMm ) );

        bbox = BOX2I( VECTOR2I( ox, oy ), VECTOR2I( w, h ) );
    }
    else
    {
        bbox = CalculateGerberBoundingBox( image.get() );
    }

    if( bbox.GetWidth() == 0 || bbox.GetHeight() == 0 )
    {
        if( aErrorMsg )
            *aErrorMsg = wxS( "Gerber file has zero-size bounding box" );

        return false;
    }

    // When using a viewport override with DPI, calculate pixel dimensions from the window
    int reqWidth = aOptions.width;
    int reqHeight = aOptions.height;

    if( aOptions.HasViewportOverride() && reqWidth == 0 && reqHeight == 0 )
    {
        double mmPerInch = 25.4;
        reqWidth = static_cast<int>( std::ceil( aOptions.windowWidthMm / mmPerInch * aOptions.dpi ) );
        reqHeight = static_cast<int>( std::ceil( aOptions.windowHeightMm / mmPerInch * aOptions.dpi ) );
    }

    GERBER_PLOTTER_VIEWPORT vp = CalculatePlotterViewport( bbox, aOptions.dpi, reqWidth, reqHeight );

    PNG_PLOTTER plotter;
    plotter.SetColorMode( true );
    plotter.SetPixelSize( vp.width, vp.height );
    plotter.SetResolution( aOptions.dpi );
    plotter.SetAntialias( aOptions.antialias );
    plotter.SetBackgroundColor( aOptions.backgroundColor );
    plotter.SetViewport( vp.offset, vp.iuPerDecimil, vp.plotScale, false );
    plotter.OpenFile( aOutputPath );

    if( !plotter.StartPlot( wxEmptyString ) )
    {
        if( aErrorMsg )
            *aErrorMsg = wxS( "Failed to start PNG plotter" );

        return false;
    }

    // Render all items with proper polarity handling.
    // Negative (clear) polarity items erase copper by drawing with the background color.
    // On transparent exports the background is alpha=0, so source-over is a no-op.
    // Switch to CAIRO_OPERATOR_CLEAR so those regions become fully transparent.
    bool transparentBg = ( aOptions.backgroundColor.a == 0 );

    for( GERBER_DRAW_ITEM* item : image->GetItems() )
    {
        if( item->GetLayerPolarity() )
        {
            plotter.SetClearCompositing( transparentBg );
            RenderItem( item, plotter, aOptions.backgroundColor );
            plotter.SetClearCompositing( false );
        }
        else
        {
            RenderItem( item, plotter, aOptions.foregroundColor );
        }
    }

    if( !plotter.EndPlot() )
    {
        if( aErrorMsg )
            *aErrorMsg = wxString::Format( wxS( "Failed to save PNG file: %s" ), aOutputPath );

        return false;
    }

    return true;
}


bool RenderGerberToPng( const wxString& aInputPath, const wxString& aOutputPath, const JOB_GERBER_EXPORT_PNG& aJob,
                        wxString* aErrorMsg, wxArrayString* aMessages )
{
    GERBER_RENDER_OPTIONS options;
    options.dpi = aJob.m_dpi;
    options.width = aJob.m_width;
    options.height = aJob.m_height;
    options.antialias = aJob.m_antialias;
    options.backgroundColor =
            aJob.m_transparentBackground ? KIGFX::COLOR4D( 1.0, 1.0, 1.0, 0.0 ) : KIGFX::COLOR4D::WHITE;

    if( !aJob.m_foregroundColor.IsEmpty() )
        options.foregroundColor = KIGFX::COLOR4D( aJob.m_foregroundColor );

    if( !aJob.m_backgroundColor.IsEmpty() )
        options.backgroundColor = KIGFX::COLOR4D( aJob.m_backgroundColor );

    double toMm = 1.0;

    switch( aJob.m_units )
    {
    case JOB_GERBER_EXPORT_PNG::UNITS::INCH: toMm = 25.4;   break;
    case JOB_GERBER_EXPORT_PNG::UNITS::MILS: toMm = 0.0254; break;
    case JOB_GERBER_EXPORT_PNG::UNITS::MM:   toMm = 1.0;    break;
    }

    options.originXMm = aJob.m_originX * toMm;
    options.originYMm = aJob.m_originY * toMm;
    options.windowWidthMm = aJob.m_windowWidth * toMm;
    options.windowHeightMm = aJob.m_windowHeight * toMm;

    return RenderGerberToPng( aInputPath, aOutputPath, options, aErrorMsg, aMessages );
}

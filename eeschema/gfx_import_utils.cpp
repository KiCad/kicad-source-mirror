/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include "gfx_import_utils.h"

#include <stdint.h>
#include <lib_symbol.h>
#include <sch_shape.h>
#include <import_gfx/graphics_importer_lib_symbol.h>
#include <import_gfx/svg_import_plugin.h>


std::unordered_map<uint32_t, SHAPE_POLY_SET> ConvertImageToPolygons( wxImage  img,
                                                                     VECTOR2D pixelScale )
{
    bool hasAlpha = img.HasAlpha();

    // Quantize the image
    for( int y = 0; y < img.GetHeight(); y++ )
    {
        for( int x = 0; x < img.GetWidth(); x++ )
        {
            int r = img.GetRed( x, y );
            int g = img.GetGreen( x, y );
            int b = img.GetBlue( x, y );
            int a = hasAlpha ? img.GetAlpha( x, y ) : 255;

            int roundBits = 5; // 32
            r = std::min( r >> roundBits << roundBits, 0xFF );
            g = std::min( g >> roundBits << roundBits, 0xFF );
            b = std::min( b >> roundBits << roundBits, 0xFF );
            a = std::min( a >> roundBits << roundBits, 0xFF );

            img.SetRGB( x, y, r, g, b );

            if( hasAlpha )
                img.SetAlpha( x, y, a );
        }
    }

    std::unordered_map<uint32_t, SHAPE_POLY_SET> colorPolys;

    // Create polygon sets
    for( int y = 0; y < img.GetHeight(); y++ )
    {
        for( int x = 0; x < img.GetWidth(); x++ )
        {
            uint32_t r = img.GetRed( x, y );
            uint32_t g = img.GetGreen( x, y );
            uint32_t b = img.GetBlue( x, y );
            uint32_t a = hasAlpha ? img.GetAlpha( x, y ) : 255;

            if( a > 0 )
            {
                uint32_t color = r | ( g << 8 ) | ( b << 16 ) | ( a << 24 );

                SHAPE_POLY_SET& colorPoly = colorPolys[color];

                SHAPE_LINE_CHAIN chain;
                chain.Append( x * pixelScale.x, y * pixelScale.y, true );
                chain.Append( ( x + 1 ) * pixelScale.x, y * pixelScale.y, true );
                chain.Append( ( x + 1 ) * pixelScale.x, ( y + 1 ) * pixelScale.y, true );
                chain.Append( x * pixelScale.x, ( y + 1 ) * pixelScale.y, true );
                chain.SetClosed( true );

                colorPoly.AddOutline( chain );
            }
        }
    }

    for( auto& [color, polySet] : colorPolys )
    {
        polySet.Simplify();

        for( int i = 0; i < polySet.OutlineCount(); i++ )
        {
            SHAPE_POLY_SET::POLYGON& poly = polySet.Polygon( i );

            for( SHAPE_LINE_CHAIN& chain : poly )
                chain.Simplify();
        }
    }

    return colorPolys;
}


void ConvertImageToLibShapes( LIB_SYMBOL* aSymbol, int unit, wxImage img, VECTOR2D pixelScale,
                              VECTOR2D offset )
{
    std::unordered_map<uint32_t, SHAPE_POLY_SET> colorPolys =
            ConvertImageToPolygons( img, pixelScale );

    for( auto& [color, polySet] : colorPolys )
    {
        polySet.Fracture();

        for( const SHAPE_POLY_SET::POLYGON& poly : polySet.CPolygons() )
        {
            auto shape = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE );

            shape->SetPolyShape( poly );

            int r = color & 0xFF;
            int g = ( color >> 8 ) & 0xFF;
            int b = ( color >> 16 ) & 0xFF;
            int a = ( color >> 24 ) & 0xFF;

            shape->SetWidth( -1 );
            shape->SetFillMode( FILL_T::FILLED_WITH_COLOR );
            shape->SetFillColor( COLOR4D( r / 255.0, g / 255.0, b / 255.0, a / 255.0 ) );

            shape->SetUnit( unit );

            shape->Move( offset );

            aSymbol->AddDrawItem( shape.release(), false );
        }
    }

    aSymbol->GetDrawItems().sort();
}


void ConvertSVGToLibShapes( LIB_SYMBOL* aSymbol, int unit, const wxMemoryBuffer& aImageData,
                            VECTOR2D pixelScale, VECTOR2D offset )
{
    SVG_IMPORT_PLUGIN            svgImportPlugin;
    GRAPHICS_IMPORTER_LIB_SYMBOL libeditImporter( aSymbol, unit );

    libeditImporter.SetScale( pixelScale );
    libeditImporter.SetImportOffsetMM(
            VECTOR2D( schIUScale.IUTomm( offset.x ), schIUScale.IUTomm( offset.y ) ) );

    svgImportPlugin.SetImporter( &libeditImporter );
    svgImportPlugin.LoadFromMemory( aImageData );

    svgImportPlugin.Import();
}
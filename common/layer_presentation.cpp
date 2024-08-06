/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "layer_presentation.h"

#include <wx/bitmap.h>
#include <wx/dcmemory.h>

#include <gal/color4d.h>


using namespace KIGFX;


void LAYER_PRESENTATION::DrawColorSwatch( wxBitmap& aLayerbmp, const COLOR4D& aBackground,
                                      const COLOR4D& aColor )
{
    wxMemoryDC bmpDC;
    wxBrush    brush;

    // Prepare Bitmap
    bmpDC.SelectObject( aLayerbmp );

    brush.SetStyle( wxBRUSHSTYLE_SOLID );

    if( aBackground != COLOR4D::UNSPECIFIED )
    {
        brush.SetColour( aBackground.WithAlpha( 1.0 ).ToColour() );
        bmpDC.SetBrush( brush );
        bmpDC.DrawRectangle( 0, 0, aLayerbmp.GetWidth(), aLayerbmp.GetHeight() );
    }

    brush.SetColour( aColor.ToColour() );
    bmpDC.SetBrush( brush );
    bmpDC.DrawRectangle( 0, 0, aLayerbmp.GetWidth(), aLayerbmp.GetHeight() );

    bmpDC.SetBrush( *wxTRANSPARENT_BRUSH );
    bmpDC.SetPen( *wxBLACK_PEN );
    bmpDC.DrawRectangle( 0, 0, aLayerbmp.GetWidth(), aLayerbmp.GetHeight() );
}


void LAYER_PRESENTATION::DrawColorSwatch( wxBitmap& aLayerbmp, int aLayer ) const
{
    const COLOR4D bgColor = getLayerColor( LAYER_PCB_BACKGROUND );
    const COLOR4D color = getLayerColor( aLayer );

    DrawColorSwatch( aLayerbmp, bgColor, color );
}


static constexpr unsigned BM_LAYERICON_SIZE = 24;
static const char         s_BitmapLayerIcon[BM_LAYERICON_SIZE][BM_LAYERICON_SIZE] = {
    // 0 = draw pixel with white
    // 1 = draw pixel with black
    // 2 = draw pixel with top layer from router pair
    // 3 = draw pixel with bottom layer from router pair
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 2, 2, 2, 2, 2, 0, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
};

static COLOR4D ICON_WHITE{ 0.86, 0.86, 0.86, 1.0 };
static COLOR4D ICON_BLACK{ 0.28, 0.28, 0.28, 1.0 };


std::unique_ptr<wxBitmap> LAYER_PRESENTATION::CreateLayerPairIcon( const COLOR4D& aBgColor, const COLOR4D& aTopColor,
                                               const COLOR4D& aBottomColor, int aScale )
{
    auto layerPairBitmap = std::make_unique<wxBitmap>( BM_LAYERICON_SIZE, BM_LAYERICON_SIZE );

    // Draw the icon, with colors according to the router's layer pair
    wxMemoryDC iconDC;
    iconDC.SelectObject( *layerPairBitmap );
    wxBrush brush;
    wxPen   pen;
    int     buttonColor = -1;

    brush.SetStyle( wxBRUSHSTYLE_SOLID );
    brush.SetColour( aBgColor.WithAlpha( 1.0 ).ToColour() );
    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, BM_LAYERICON_SIZE, BM_LAYERICON_SIZE );

    for( unsigned ii = 0; ii < BM_LAYERICON_SIZE; ii++ )
    {
        for( unsigned jj = 0; jj < BM_LAYERICON_SIZE; jj++ )
        {
            if( s_BitmapLayerIcon[ii][jj] != buttonColor )
            {
                switch( s_BitmapLayerIcon[ii][jj] )
                {
                default:
                case 0: pen.SetColour( ICON_WHITE.ToColour() ); break;
                case 1: pen.SetColour( ICON_BLACK.ToColour() ); break;
                case 2: pen.SetColour( aTopColor.ToColour() ); break;
                case 3: pen.SetColour( aBottomColor.ToColour() ); break;
                }

                buttonColor = s_BitmapLayerIcon[ii][jj];
                iconDC.SetPen( pen );
            }

            iconDC.DrawPoint( jj, ii );
        }
    }

    // Deselect the bitmap from the DC in order to delete the MemoryDC safely without
    // deleting the bitmap
    iconDC.SelectObject( wxNullBitmap );

    // Scale the bitmap
    wxImage image = layerPairBitmap->ConvertToImage();

    // "NEAREST" causes less mixing of colors
    image.Rescale( aScale * image.GetWidth() / 4, aScale * image.GetHeight() / 4,
                   wxIMAGE_QUALITY_NEAREST );

    return std::make_unique<wxBitmap>( image );
}


std::unique_ptr<wxBitmap> LAYER_PRESENTATION::CreateLayerPairIcon( int aLeftLayer, int aRightLayer,
                                                                   int aScale ) const
{
    const COLOR4D bgColor = getLayerColor( LAYER_PCB_BACKGROUND );
    const COLOR4D topColor = getLayerColor( aLeftLayer );
    const COLOR4D bottomColor = getLayerColor( aRightLayer );

    return CreateLayerPairIcon( bgColor, topColor, bottomColor, aScale );
}
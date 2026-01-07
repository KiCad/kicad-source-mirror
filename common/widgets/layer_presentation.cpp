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

#include "widgets/layer_presentation.h"

#include <wx/bitmap.h>
#include <wx/bmpbndl.h>
#include <wx/dcmemory.h>
#include <wx/graphics.h>

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
    bmpDC.SelectObject( wxNullBitmap );
}


void LAYER_PRESENTATION::DrawColorSwatch( wxBitmap& aLayerbmp, int aLayer ) const
{
    const COLOR4D bgColor = getLayerColor( LAYER_PCB_BACKGROUND );
    const COLOR4D color = getLayerColor( aLayer );

    DrawColorSwatch( aLayerbmp, bgColor, color );
}


// Helper function to create a single bitmap at a specific size using vector graphics
static wxBitmap createLayerPairBitmapAtSize( const COLOR4D& aTopColor, const COLOR4D& aBottomColor, int aSize )
{
    wxBitmap bitmap( aSize, aSize );
    wxMemoryDC memDC;
    memDC.SelectObject( bitmap );

    wxGraphicsContext* gc = wxGraphicsContext::Create( memDC );
    if( !gc )
        return bitmap;

    gc->SetAntialiasMode( wxANTIALIAS_DEFAULT );

    int sepTopX = aSize - aSize / 3;
    int sepTopY = -1;
    int sepBotX = aSize / 3 - 1;
    int sepBotY = aSize;

    // Draw left quadrilateral (top layer)
    wxGraphicsPath topPath = gc->CreatePath();
    topPath.MoveToPoint( 0, 0 );
    topPath.AddLineToPoint( sepTopX, 0 );
    topPath.AddLineToPoint( sepBotX, aSize );
    topPath.AddLineToPoint( 0, aSize );
    topPath.CloseSubpath();

    wxBrush topBrush( aTopColor.WithAlpha( 1.0 ).ToColour() );
    gc->SetBrush( topBrush );
    gc->SetPen( *wxTRANSPARENT_PEN );
    gc->DrawPath( topPath );

    // Draw right quadrilateral (bottom layer)
    wxGraphicsPath bottomPath = gc->CreatePath();
    bottomPath.MoveToPoint( sepTopX, 0 );
    bottomPath.AddLineToPoint( aSize, 0 );
    bottomPath.AddLineToPoint( aSize, aSize );
    bottomPath.AddLineToPoint( sepBotX, aSize );
    bottomPath.CloseSubpath();

    wxBrush bottomBrush( aBottomColor.WithAlpha( 1.0 ).ToColour() );
    gc->SetBrush( bottomBrush );
    gc->DrawPath( bottomPath );

    int lineScale = std::max( 1, wxRound( aSize / 24.0 ) );

    // Draw separator line with white outline and black center
    wxPen whiteLine( *wxWHITE, 3 * lineScale );
    gc->SetPen( whiteLine );
    gc->StrokeLine( sepTopX, sepTopY, sepBotX, sepBotY );

    wxPen blackLine( *wxBLACK, 1 * lineScale );
    gc->SetPen( blackLine );
    gc->StrokeLine( sepTopX, sepTopY, sepBotX, sepBotY );

    delete gc;
    memDC.SelectObject( wxNullBitmap );

    return bitmap;
}


wxBitmapBundle LAYER_PRESENTATION::CreateLayerPairIcon( const COLOR4D& aTopColor, const COLOR4D& aBottomColor, int aDefSize )
{
    wxVector<wxBitmap> bitmaps;

    bitmaps.push_back( createLayerPairBitmapAtSize( aTopColor, aBottomColor, aDefSize ) );
    bitmaps.push_back( createLayerPairBitmapAtSize( aTopColor, aBottomColor, aDefSize * 1.3334 ) );
    bitmaps.push_back( createLayerPairBitmapAtSize( aTopColor, aBottomColor, aDefSize * 1.5 ) );
    bitmaps.push_back( createLayerPairBitmapAtSize( aTopColor, aBottomColor, aDefSize * 2 ) );
    bitmaps.push_back( createLayerPairBitmapAtSize( aTopColor, aBottomColor, aDefSize * 2.6667 ) );
    bitmaps.push_back( createLayerPairBitmapAtSize( aTopColor, aBottomColor, aDefSize * 3 ) );

    return wxBitmapBundle::FromBitmaps( bitmaps );
}


wxBitmapBundle LAYER_PRESENTATION::CreateLayerPairIcon( int aLeftLayer, int aRightLayer, int aDefSize ) const
{
    const COLOR4D topColor = getLayerColor( aLeftLayer );
    const COLOR4D bottomColor = getLayerColor( aRightLayer );

    return CreateLayerPairIcon( topColor, bottomColor, aDefSize );
}

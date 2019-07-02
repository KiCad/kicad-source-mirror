/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2019 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <preview_items/preview_utils.h>
#include <gal/graphics_abstraction_layer.h>
#include <base_units.h>
#include <pcb_painter.h>
#include <view/view.h>

double KIGFX::PREVIEW::PreviewOverlayDeemphAlpha( bool aDeemph )
{
    return aDeemph ? 0.5 : 1.0;
}


static wxString formatPreviewDimension( double aVal, EDA_UNITS_T aUnits )
{
    int precision = 4;

    // show a sane precision for the preview, which doesn't need to
    // be accurate down to the nanometre
    switch( aUnits )
    {
    case MILLIMETRES:
        precision = 3;  // 1um
        break;

    case INCHES:
        precision = 4;  // 0.1mil
        break;

    case DEGREES:
        precision = 1;  // 0.1deg
        break;

    case PERCENT:
        precision = 1;  // 0.1%
        break;

    case UNSCALED_UNITS:
        break;
    }

    const wxString fmtStr = wxString::Format( "%%.%df", precision );

    wxString str = wxString::Format( fmtStr, To_User_Unit( aUnits, aVal ) );

    const wxString symbol = GetAbbreviatedUnitsLabel( aUnits, false );

    if( symbol.size() )
        str << " " << symbol;

    return str;
}


wxString KIGFX::PREVIEW::DimensionLabel( const wxString& prefix,
            double aVal, EDA_UNITS_T aUnits )
{
    wxString str;

    if( prefix.size() )
        str << prefix << ": ";

    str << formatPreviewDimension( aVal, aUnits );
    return str;
}


void KIGFX::PREVIEW::SetConstantGlyphHeight( KIGFX::GAL& aGal, double aHeight )
{
    aHeight /= aGal.GetWorldScale();

    auto glyphSize = aGal.GetGlyphSize();
    glyphSize = glyphSize * ( aHeight / glyphSize.y );
    aGal.SetGlyphSize( glyphSize );
}


void KIGFX::PREVIEW::DrawTextNextToCursor( KIGFX::VIEW* aView,
        const VECTOR2D& aCursorPos, const VECTOR2D& aTextQuadrant,
        const std::vector<wxString>& aStrings )
{
    auto gal = aView->GetGAL();
    auto glyphSize = gal->GetGlyphSize();
    auto rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( aView->GetPainter()->GetSettings() );

    const auto lineSpace = glyphSize.y * 0.2;
    auto linePitch = glyphSize.y + lineSpace;

    // radius string goes on the right of the cursor centre line
    // with a small horizontal offset (enough to keep clear of a
    // system cursor if present)
    auto textPos = aCursorPos;

    // if the text goes above the cursor, shift it up
    if( aTextQuadrant.y > 0 )
    {
        textPos.y -= linePitch * ( aStrings.size() + 1 );
    }

    if( aTextQuadrant.x < 0 )
    {
        gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
        textPos.x += 15.0 / gal->GetWorldScale();
    }
    else
    {
        gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
        textPos.x -= 15.0 / gal->GetWorldScale();
    }

    gal->SetStrokeColor( rs->GetLayerColor( LAYER_AUX_ITEMS ).WithAlpha(
                            PreviewOverlayDeemphAlpha( true ) ) );
    gal->SetIsFill( false );

    // write strings top-to-bottom
    for( const auto& str : aStrings )
    {
        textPos.y += linePitch;
        gal->BitmapText( str, textPos, 0.0 );
    }
}

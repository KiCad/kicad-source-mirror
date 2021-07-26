/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 Kicad Developers, see AUTHORS.txt for contributors.
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
#include <painter.h>
#include <view/view.h>
#include <gal/hidpi_gl_canvas.h>

double KIGFX::PREVIEW::PreviewOverlayDeemphAlpha( bool aDeemph )
{
    return aDeemph ? 0.5 : 1.0;
}


wxString KIGFX::PREVIEW::DimensionLabel( const wxString& prefix, double aVal, EDA_UNITS aUnits,
                                         bool aIncludeUnits )
{
    wxString str;

    if( prefix.size() )
        str << prefix << ": ";

    wxString fmtStr;

    // show a sane precision for the preview, which doesn't need to be accurate down to the
    // nanometre
    switch( aUnits )
    {
    case EDA_UNITS::MILLIMETRES: fmtStr = wxT( "%.3f" ); break;  // 1um
    case EDA_UNITS::MILS:        fmtStr = wxT( "%.1f" ); break;  // 0.1mil
    case EDA_UNITS::INCHES:      fmtStr = wxT( "%.4f" ); break;  // 0.1mil
    case EDA_UNITS::DEGREES:     fmtStr = wxT( "%.1f" ); break;  // 0.1deg
    case EDA_UNITS::PERCENT:     fmtStr = wxT( "%.1f" ); break;  // 0.1%
    case EDA_UNITS::UNSCALED:    fmtStr = wxT( "%f" );   break;
    }

    str << wxString::Format( fmtStr, To_User_Unit( aUnits, aVal ) );

    if( aIncludeUnits )
        str << " " << GetAbbreviatedUnitsLabel( aUnits );

    return str;
}


KIGFX::PREVIEW::TEXT_DIMS KIGFX::PREVIEW::SetConstantGlyphHeight( KIGFX::GAL* aGal,
                                                                  int aRelativeSize )
{
    constexpr double hdpiSizes[] = { 8, 9, 11, 13, 15 };
    constexpr double sizes[] = { 10, 12, 14, 16, 18 };

    double height;
    double thicknessFactor;
    double shadowFactor;
    double linePitchFactor;

    HIDPI_GL_CANVAS* canvas = dynamic_cast<HIDPI_GL_CANVAS*>( aGal );

    if( canvas && canvas->GetScaleFactor() > 1 )
    {
        height = hdpiSizes[ 2 + aRelativeSize ];
        thicknessFactor = 0.15;
        shadowFactor = 0.10;
        linePitchFactor = 1.7;
    }
    else
    {
        height = sizes[ 2 + aRelativeSize ];
        thicknessFactor = 0.20;
        shadowFactor = 0.15;
        linePitchFactor = 1.9;
    }

    height /= aGal->GetWorldScale();

    VECTOR2D glyphSize = aGal->GetGlyphSize();
    glyphSize = glyphSize * ( height / glyphSize.y );
    aGal->SetGlyphSize( glyphSize );

    TEXT_DIMS textDims;

    textDims.StrokeWidth = glyphSize.x * thicknessFactor;
    textDims.ShadowWidth = glyphSize.x * shadowFactor;
    textDims.LinePitch = glyphSize.y * linePitchFactor;

    return textDims;
}


KIGFX::COLOR4D KIGFX::PREVIEW::GetShadowColor( const KIGFX::COLOR4D& aColor )
{
    if( aColor.GetBrightness() > 0.5 )
        return COLOR4D::BLACK;
    else
        return COLOR4D::WHITE;
}


void KIGFX::PREVIEW::DrawTextNextToCursor( KIGFX::VIEW* aView, const VECTOR2D& aCursorPos,
                                           const VECTOR2D& aTextQuadrant,
                                           const std::vector<wxString>& aStrings,
                                           bool aDrawingDropShadows )
{
    KIGFX::GAL*      gal = aView->GetGAL();
    RENDER_SETTINGS* rs = aView->GetPainter()->GetSettings();

    // constant text size on screen
    TEXT_DIMS        textDims = SetConstantGlyphHeight( gal );

    // radius string goes on the right of the cursor centre line with a small horizontal
    // offset (enough to keep clear of a system cursor if present)
    VECTOR2D         textPos = aCursorPos;

    bool             viewFlipped = gal->IsFlippedX();

    // if the text goes above the cursor, shift it up
    if( aTextQuadrant.y > 0 )
    {
        textPos.y -= textDims.LinePitch * ( aStrings.size() + 1 );
    }

    if( aTextQuadrant.x < 0 )
    {
        if( viewFlipped )
            gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
        else
            gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );

        textPos.x += 15.0 / gal->GetWorldScale();

        if( aDrawingDropShadows )
            textPos.x -= textDims.ShadowWidth;
    }
    else
    {
        if( viewFlipped )
            gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
        else
            gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );

        textPos.x -= 15.0 / gal->GetWorldScale();

        if( aDrawingDropShadows )
            textPos.x += textDims.ShadowWidth;
    }

    gal->SetIsFill( false );
    gal->SetStrokeColor( rs->GetLayerColor( LAYER_AUX_ITEMS ) );
    gal->SetLineWidth( textDims.StrokeWidth );
    gal->SetTextMirrored( viewFlipped ); // Prevent text flipping when view is flipped

    if( aDrawingDropShadows )
    {
        gal->SetStrokeColor( GetShadowColor( gal->GetStrokeColor() ) );
        gal->SetLineWidth( gal->GetLineWidth() + 2 * textDims.ShadowWidth );
    }

    // write strings top-to-bottom
    for( const wxString& str : aStrings )
    {
        textPos.y += textDims.LinePitch;
        gal->StrokeText( str, textPos, 0.0 );
    }
}

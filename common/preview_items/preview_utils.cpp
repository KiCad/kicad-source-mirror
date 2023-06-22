/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 Kicad Developers, see AUTHORS.txt for contributors.
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


wxString KIGFX::PREVIEW::DimensionLabel( const wxString& prefix, double aVal,
                                         const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
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

    str << wxString::Format( fmtStr, EDA_UNIT_UTILS::UI::ToUserUnit( aIuScale, aUnits, aVal ) );

    if( aIncludeUnits )
        str << EDA_UNIT_UTILS::GetText( aUnits );

    return str;
}


KIGFX::PREVIEW::TEXT_DIMS KIGFX::PREVIEW::GetConstantGlyphHeight( KIGFX::GAL* aGal,
                                                                  int aRelativeSize )
{
    constexpr double aspectRatio = 1.0;
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

    TEXT_DIMS textDims;

    textDims.GlyphSize = VECTOR2I( height * aspectRatio, height );
    textDims.StrokeWidth = height * thicknessFactor;
    textDims.ShadowWidth = height * shadowFactor;
    textDims.LinePitch = height * linePitchFactor;

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
    KIFONT::FONT*    font = KIFONT::FONT::GetFont();

    // constant text size on screen
    TEXT_DIMS        textDims = GetConstantGlyphHeight( gal );
    TEXT_ATTRIBUTES  textAttrs;

    // radius string goes on the right of the cursor centre line with a small horizontal
    // offset (enough to keep clear of a system cursor if present)
    VECTOR2D         textPos = aCursorPos;

    bool             viewFlipped = gal->IsFlippedX();

    // if the text goes above the cursor, shift it up
    if( aTextQuadrant.y > 0 )
        textPos.y -= textDims.LinePitch * ( aStrings.size() + 1 );

    if( aTextQuadrant.x < 0 )
    {
        if( viewFlipped )
            textAttrs.m_Halign = GR_TEXT_H_ALIGN_RIGHT;
        else
            textAttrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;

        textPos.x += 15.0 / gal->GetWorldScale();
    }
    else
    {
        if( viewFlipped )
            textAttrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
        else
            textAttrs.m_Halign = GR_TEXT_H_ALIGN_RIGHT;

        textPos.x -= 15.0 / gal->GetWorldScale();
    }

    // text is left (or right) aligned, so a shadow text need a small offset to be draw
    // around the basic text
    int shadowXoffset = aDrawingDropShadows ? textDims.ShadowWidth : 0;

    // Due to the fact a shadow text is drawn left or right aligned,
    // it needs an offset = shadowWidth/2 to be drawn at the same place as normal text
    // But for some reason we need to slightly modify this offset
    // for a better look for KiCad font (better alignment of shadow shape)
    const float adjust = 1.2f;      // Value chosen after tests
    shadowXoffset *= adjust;

    if( ( textAttrs.m_Halign == GR_TEXT_H_ALIGN_LEFT ) != viewFlipped )
        textPos.x -= shadowXoffset;
    else
        textPos.x += shadowXoffset;

    gal->SetStrokeColor( aView->GetPainter()->GetSettings()->GetLayerColor( LAYER_AUX_ITEMS ) );
    textAttrs.m_Mirrored = viewFlipped; // Prevent text flipping when view is flipped
    textAttrs.m_Size = textDims.GlyphSize;
    textAttrs.m_StrokeWidth = textDims.StrokeWidth;
    gal->SetIsFill( false );
    gal->SetIsStroke( true );

    if( aDrawingDropShadows )
    {
        textAttrs.m_StrokeWidth = textDims.StrokeWidth + ( 2 * textDims.ShadowWidth );
        gal->SetStrokeColor( GetShadowColor( gal->GetStrokeColor() ) );
    }

    // write strings top-to-bottom
    for( const wxString& str : aStrings )
    {
        textPos.y += textDims.LinePitch;
        font->Draw( gal, str, textPos, textAttrs );
    }
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jerry Jacobs
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

#ifndef GR_TEXT_H
#define GR_TEXT_H

#include <font/text_attributes.h>
#include <wx/gdicmn.h>

class wxDC;


namespace KIGFX
{
    class COLOR4D;
}

namespace KIFONT
{
    class METRICS;
}

class PLOTTER;

/**
 * Pen width should not allow characters to become cluttered up in their own fatness.  Normal
 * text is normally around 15% the fontsize, and bold text around 20%.  So we set a hard limit
 * at 25%, and a secondary limit for non-decorative text that must be readable at small sizes
 * at 18%.
 *
 * @param aPenSize the pen size to clamp.
 * @param aSize the char size (height or width, or its wxSize).
 * @param aBold true if text accept bold pen size.
 * @return the max pen size allowed.
 */
int ClampTextPenSize( int aPenSize, int aSize, bool aStrict = false );
float ClampTextPenSize( float aPenSize, int aSize, bool aStrict = false );
int ClampTextPenSize( int aPenSize, const VECTOR2I& aSize, bool aStrict = false );

/**
 * @param aTextSize the char size (height or width).
 * @return the "best" value for a pen size to draw/plot a bold text.
 */
int GetPenSizeForBold( int aTextSize );
int GetPenSizeForBold( const wxSize& aTextSize );

/**
 * @param aTextSize the char size (height or width).
 * @return the "best" value for a pen size to draw/plot a demibold text.
 */
int GetPenSizeForDemiBold( int aTextSize );
int GetPenSizeForDemiBold( const wxSize& aTextSize );

/**
 * @param aTextSize = the char size (height or width).
 * @return the "best" value for a pen size to draw/plot a non-bold text.
 */
int GetPenSizeForNormal( int aTextSize );
int GetPenSizeForNormal( const wxSize& aTextSize );

inline void InferBold( TEXT_ATTRIBUTES* aAttrs )
{
    int    penSize( aAttrs->m_StrokeWidth );
    wxSize textSize( aAttrs->m_Size.x, aAttrs->m_Size.y );

    aAttrs->m_Bold = abs( penSize - GetPenSizeForBold( textSize ) )
                   < abs( penSize - GetPenSizeForNormal( textSize ) );
}


/**
 * Return the margin for knocking out text.
 */
inline int GetKnockoutTextMargin( const VECTOR2I& aSize, int aThickness )
{
    return std::max( KiROUND( aThickness / 2.0 ), KiROUND( aSize.y / 9.0 ) );
}


/**
 * @return the X size of the graphic text.
 */
int GRTextWidth( const wxString& aText, KIFONT::FONT* aFont, const VECTOR2I& aSize,
                 int aThickness, bool aBold, bool aItalic, const KIFONT::METRICS& aFontMetrics );

/**
 * Print a graphic text through wxDC.
 *
 *  @param aDC is the current Device Context.
 *  @param aPos is the text position (according to h_justify, v_justify).
 *  @param aColor is the text color.
 *  @param aText is the text to draw.
 *  @param aOrient is the angle.
 *  @param aSize is the text size (size.x or size.y can be < 0 for mirrored texts).
 *  @param aH_justify is the horizontal justification (Left, center, right).
 *  @param aV_justify is the vertical justification (bottom, center, top).
 *  @param aWidth is the line width (pen width) (use default width if aWidth = 0).
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 *      Use a value min(aSize.x, aSize.y) / 5 for a bold text.
 *  @param aItalic is the true to simulate an italic font.
 *  @param aBold use true to use a bold font. Useful only with default width value (aWidth = 0).
 *  @param aFont is the font to use, or nullptr for the KiCad stroke font
 */
void GRPrintText( wxDC* aDC, const VECTOR2I& aPos, const KIGFX::COLOR4D& aColor,
                  const wxString& aText, const EDA_ANGLE& aOrient, const VECTOR2I& aSize,
                  enum GR_TEXT_H_ALIGN_T aH_justify, enum GR_TEXT_V_ALIGN_T aV_justify,
                  int aWidth, bool aItalic, bool aBold, KIFONT::FONT* aFont,
                  const KIFONT::METRICS& aFontMetrics );


#endif /* GR_TEXT_H */

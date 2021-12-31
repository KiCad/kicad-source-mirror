/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jerry Jacobs
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_item.h>
#include <font/text_attributes.h>

namespace KIGFX
{
    class COLOR4D;
}

/**
 * Minimum dimension in pixel for drawing/no drawing a text used in Pcbnew to decide to
 * draw (or not) some texts ( like net names on pads/tracks ).
 *
 * When a text height is smaller than MIN_TEXT_SIZE, it is not drawn by Pcbnew.
 */
#define MIN_TEXT_SIZE   5

/* Absolute minimum dimension in pixel to draw a text as text or a line
 * When a text height is smaller than MIN_DRAWABLE_TEXT_SIZE,
 * it is drawn, but like a line by the draw text function
*/
#define MIN_DRAWABLE_TEXT_SIZE 3

class PLOTTER;

/**
 * As a rule, pen width should not be >1/4em, otherwise the character will be cluttered up in
 * its own fatness.
 *
 * The pen width max is aSize/4 for bold texts, and aSize/6 for normal texts. The "best" pen
 * width is aSize/5 for bold texts so the clamp is consistent with bold option.
 *
 * @param aPenSize the pen size to clamp.
 * @param aSize the char size (height or width, or its wxSize).
 * @param aBold true if text accept bold pen size.
 * @return the max pen size allowed.
 */
int Clamp_Text_PenSize( int aPenSize, int aSize, bool aBold = true );
float Clamp_Text_PenSize( float aPenSize, int aSize, bool aBold = true );
int Clamp_Text_PenSize( int aPenSize, const VECTOR2I& aSize, bool aBold = true );

/**
 * @param aTextSize the char size (height or width).
 * @return the "best" value for a pen size to draw/plot a bold text.
 */
int GetPenSizeForBold( int aTextSize );
int GetPenSizeForBold( const wxSize& aTextSize );

/**
 * @param aTextSize = the char size (height or width).
 * @return the "best" value for a pen size to draw/plot a non-bold text.
 */
int GetPenSizeForNormal( int aTextSize );
int GetPenSizeForNormal( const wxSize& aTextSize );

/**
 * The full X size is GraphicTextWidth + the thickness of graphic lines.
 *
 * @return the X size of the graphic text.
 */
int GraphicTextWidth( const wxString& aText, KIFONT::FONT* aFont, const VECTOR2I& aSize,
                      bool italic, bool bold );

/**
 * Draw a graphic text (like footprint text)
 *
 *  @param aClipBox the clipping rect, or NULL if no clipping.
 *  @param aDC the current Device Context. NULL if draw within a 3D GL Canvas.
 *  @param aPos text position (according to h_justify, v_justify).
 *  @param aColor (COLOR4D) = text color.
 *  @param aText text to draw.
 *  @param aOrient angle.
 *  @param aSize text size (size.x or size.y can be < 0 for mirrored texts).
 *  @param aH_justify horizontal justification (Left, center, right).
 *  @param aV_justify vertical justification (bottom, center, top).
 *  @param aWidth line width (pen width) (default = 0). If width < 0 : draw segments in
 *                sketch mode, width = abs(width).  Use a value min(aSize.x, aSize.y) / 5
 *                for a bold text.
 *  @param aItalic true to simulate an italic font.
 *  @param aBold true to use a bold font.
 *  @param aFont is the font to use, or nullptr for the KiCad stroke font
 *  @param aCallback ( int x0, int y0, int xf, int yf, void* aData ) is a function called
 *                   (if non null) to draw each segment. used to draw 3D texts or for plotting.
 *                   NULL for normal drawings.
 *  @param aCallbackData is the auxiliary parameter aData for the callback function.
 *                       This can be nullptr if no auxiliary parameter is needed.
 *  @param aPlotter = a pointer to a PLOTTER instance, when this function is used to plot
 *                    the text. NULL to draw this text.
 */
void GRText( wxDC* aDC, const VECTOR2I& aPos, const KIGFX::COLOR4D& aColor, const wxString& aText,
             const EDA_ANGLE& aOrient, const VECTOR2I& aSize, enum GR_TEXT_H_ALIGN_T aH_justify,
             enum GR_TEXT_V_ALIGN_T aV_justify, int aWidth, bool aItalic, bool aBold,
             KIFONT::FONT* aFont,
             void (*aCallback)( int x0, int y0, int xf, int yf, void* aData ) = nullptr,
             void* aCallbackData = nullptr, PLOTTER* aPlotter = nullptr );


#endif /* GR_TEXT_H */

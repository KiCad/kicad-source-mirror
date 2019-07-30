/**
 * Functions to draw and plot text on screen
 * @file draw_graphic_text.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <plotter.h>
#include <eda_text.h>               // EDA_TEXT_HJUSTIFY_T and EDA_TEXT_VJUSTIFY_T
#include <trigo.h>
#include <macros.h>
#include <base_screen.h>
#include <gr_text.h>

#include <basic_gal.h>

/**
 * Function GetPensizeForBold
 * @return the "best" value for a pen size to draw/plot a bold text
 * @param aTextSize = the char size (height or width)
 */
int GetPenSizeForBold( int aTextSize )
{
    return KiROUND( aTextSize / 5.0 );
}


/**
 * Function  Clamp_Text_PenSize
 * Don't allow text to become cluttered up in its own fatness.  Bold fonts are generally around
 * aSize/5 in width, so we limit them to aSize/4, and normal text to aSize/6.
 * @param aPenSize = the pen size to clamp
 * @param aSize the char size (height or width)
 * @param aBold = true if text accept bold pen size
 * @return the max pen size allowed
 */
int Clamp_Text_PenSize( int aPenSize, int aSize, bool aBold )
{
    double scale    = aBold ? 4.0 : 6.0;
    int    maxWidth = KiROUND( (double) aSize / scale );

    return std::min( aPenSize, maxWidth );
}


float Clamp_Text_PenSize( float aPenSize, int aSize, bool aBold )
{
    float scale    = aBold ? 4.0 : 6.0;
    float maxWidth = (float) aSize / scale;

    return std::min( aPenSize, maxWidth );
}


int Clamp_Text_PenSize( int aPenSize, wxSize aSize, bool aBold )
{
    int size = std::min( std::abs( aSize.x ), std::abs( aSize.y ) );

    return Clamp_Text_PenSize( aPenSize, size, aBold );
}


int GraphicTextWidth( const wxString& aText, const wxSize& aSize, bool aItalic, bool aBold )
{
    basic_gal.SetFontItalic( aItalic );
    basic_gal.SetFontBold( aBold );
    basic_gal.SetGlyphSize( VECTOR2D( aSize ) );

    VECTOR2D tsize = basic_gal.GetTextLineSize( aText );

    return KiROUND( tsize.x );
}


/**
 * Function GRText
 * Draw a graphic text (like module texts)
 *  @param aDC = the current Device Context. NULL if draw within a 3D GL Canvas
 *  @param aPos = text position (according to h_justify, v_justify)
 *  @param aColor (COLOR4D) = text color
 *  @param aText = text to draw
 *  @param aOrient = angle in 0.1 degree
 *  @param aSize = text size (size.x or size.y can be < 0 for mirrored texts)
 *  @param aH_justify = horizontal justification (Left, center, right)
 *  @param aV_justify = vertical justification (bottom, center, top)
 *  @param aWidth = line width (pen width) (use default width if aWidth = 0)
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 *      Use a value min(aSize.x, aSize.y) / 5 for a bold text
 *  @param aItalic = true to simulate an italic font
 *  @param aBold = true to use a bold font. Useful only with default width value (aWidth = 0)
 *  @param aCallback( int x0, int y0, int xf, int yf, void* aData ) is a function called
 *                  (if non null) to draw each segment. used to draw 3D texts or for plotting.
 *                  NULL for normal drawings
 *  @param aCallbackData = is the auxiliary parameter aData for the callback function.
 *                         can be nullptr if no auxiliary parameter is needed
 *  @param aPlotter = a pointer to a PLOTTER instance, when this function is used to plot
 *                  the text. NULL to draw this text.
 */
void GRText( wxDC* aDC, const wxPoint& aPos, COLOR4D aColor, const wxString& aText,
             double aOrient, const wxSize& aSize, enum EDA_TEXT_HJUSTIFY_T aH_justify,
             enum EDA_TEXT_VJUSTIFY_T aV_justify, int aWidth, bool aItalic, bool aBold,
             void (* aCallback)( int x0, int y0, int xf, int yf, void* aData ),
             void* aCallbackData, PLOTTER* aPlotter )
{
    bool fill_mode = true;

    if( aWidth == 0 && aBold ) // Use default values if aWidth == 0
        aWidth = GetPenSizeForBold( std::min( aSize.x, aSize.y ) );

    if( aWidth < 0 )
    {
        aWidth = -aWidth;
        fill_mode = false;
    }

    basic_gal.SetIsFill( fill_mode );
    basic_gal.SetLineWidth( aWidth );

    EDA_TEXT dummy;
    dummy.SetItalic( aItalic );
    dummy.SetBold( aBold );
    dummy.SetHorizJustify( aH_justify );
    dummy.SetVertJustify( aV_justify );

    wxSize size = aSize;
    dummy.SetMirrored( size.x < 0 );

    if( size.x < 0 )
        size.x = - size.x;

    dummy.SetTextSize( size );

    basic_gal.SetTextAttributes( &dummy );
    basic_gal.SetPlotter( aPlotter );
    basic_gal.SetCallback( aCallback, aCallbackData );
    basic_gal.m_DC = aDC;
    basic_gal.m_Color = aColor;
    basic_gal.SetClipBox( nullptr );

    basic_gal.StrokeText( aText, VECTOR2D( aPos ), aOrient * M_PI/1800 );
}


void GRHaloText( wxDC * aDC, const wxPoint &aPos, const COLOR4D aBgColor, COLOR4D aColor1,
                 COLOR4D aColor2, const wxString &aText, double aOrient, const wxSize &aSize,
                 enum EDA_TEXT_HJUSTIFY_T aH_justify, enum EDA_TEXT_VJUSTIFY_T aV_justify,
                 int aWidth, bool aItalic, bool aBold,
                 void (*aCallback)( int x0, int y0, int xf, int yf, void* aData ),
                 void* aCallbackData, PLOTTER * aPlotter )
{
    // Swap color if contrast would be better
    // TODO: Maybe calculate contrast some way other than brightness
    if( aBgColor.GetBrightness() > 0.5 )
    {
        COLOR4D c = aColor1;
        aColor1 = aColor2;
        aColor2 = c;
    }

    // Draw the background
    GRText( aDC, aPos, aColor1, aText, aOrient, aSize, aH_justify, aV_justify, aWidth, aItalic,
            aBold, aCallback, aCallbackData, aPlotter );

    // Draw the text
    GRText( aDC, aPos, aColor2, aText, aOrient, aSize, aH_justify, aV_justify, aWidth/4, aItalic,
            aBold, aCallback, aCallbackData, aPlotter );
}

/**
 * Function PLOTTER::Text
 * same as GRText, but plot graphic text insteed of draw it
 *  @param aPos = text position (according to aH_justify, aV_justify)
 *  @param aColor (COLOR4D) = text color
 *  @param aText = text to draw
 *  @param aOrient = angle in 0.1 degree
 *  @param aSize = text size (size.x or size.y can be < 0 for mirrored texts)
 *  @param aH_justify = horizontal justification (Left, center, right)
 *  @param aV_justify = vertical justification (bottom, center, top)
 *  @param aWidth = line width (pen width) (default = 0)
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 *      Use a value min(aSize.x, aSize.y) / 5 for a bold text
 *  @param aItalic = true to simulate an italic font
 *  @param aBold = true to use a bold font Useful only with default width value (aWidth = 0)
 *  @param aMultilineAllowed = true to plot text as multiline, otherwise single line
 *  @param aData = a parameter used by some plotters in SetCurrentLineWidth(),
 * not directly used here.
 */
void PLOTTER::Text( const wxPoint&              aPos,
                    const COLOR4D               aColor,
                    const wxString&             aText,
                    double                      aOrient,
                    const wxSize&               aSize,
                    enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                    enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                    int                         aWidth,
                    bool                        aItalic,
                    bool                        aBold,
                    bool                        aMultilineAllowed,
                    void*                       aData )
{
    int textPensize = aWidth;

    if( textPensize == 0 && aBold ) // Use default values if aWidth == 0
        textPensize = GetPenSizeForBold( std::min( aSize.x, aSize.y ) );

    if( textPensize >= 0 )
        textPensize = Clamp_Text_PenSize( aWidth, aSize, aBold );
    else
        textPensize = -Clamp_Text_PenSize( -aWidth, aSize, aBold );

    SetCurrentLineWidth( textPensize, aData );

    SetColor( aColor );

    GRText( NULL, aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify, textPensize,
            aItalic, aBold, nullptr, nullptr, this );

    if( aWidth != textPensize )
        SetCurrentLineWidth( aWidth, aData );
}

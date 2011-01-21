/**
 * This file is part of the common libary
 * @file  drawtxt.h
 * @see   common.h
 */

#ifndef __INCLUDE__DRAWTXT_H__
#define __INCLUDE__DRAWTXT_H__ 1

#include "base_struct.h"

class EDA_DRAW_PANEL;
class PLOTTER;

/**
 * Function  Clamp_Text_PenSize
 *As a rule, pen width should not be >1/4em, otherwise the character
 * will be cluttered up in its own fatness
 * The pen width max is aSize/4 for bold texts, and aSize/6 for normal texts
 * The "best" pen width is aSize/5 for bold texts,
 * so the clamp is consistant with bold option.
 * @param aPenSize = the pen size to clamp
 * @param aSize the char size (height or width, od its wxSize)
 * @param aBold = true if text accept bold pen size
 * @return the max pen size allowed
 */
int Clamp_Text_PenSize( int aPenSize, int aSize, bool aBold = true );
int Clamp_Text_PenSize( int aPenSize, wxSize aSize, bool aBold = true );

/**
 * Function GetPensizeForBold
 * @return the "best" value for a pen size to draw/plot a bold text
 * @param aTextSize = the char size (height or width)
 */
int GetPenSizeForBold( int aTextSize );

/**
 * Function ReturnGraphicTextWidth
 * @return the X size of the graphic text
 * the full X size is ReturnGraphicTextWidth + the thickness of graphic lines
 */
int ReturnGraphicTextWidth( const wxString& aText, int size_h, bool italic, bool bold );

/**
 * Function NegableTextLength
 * Return the text length of a negable string, excluding the ~ markers */
int NegableTextLength( const wxString& aText );

/**
 * Function DrawGraphicText
 * Draw a graphic text (like module texts)
 *  @param aPanel = the current DrawPanel. NULL if draw within a 3D GL Canvas
 *  @param aDC = the current Device Context. NULL if draw within a 3D GL Canvas
 *  @param aPos = text position (according to h_justify, v_justify)
 *  @param aColor (enum EDA_Colors) = text color
 *  @param aText = text to draw
 *  @param aOrient = angle in 0.1 degree
 *  @param aSize = text size (size.x or size.y can be < 0 for mirrored texts)
 *  @param aH_justify = horizontal justification (Left, center, right)
 *  @param aV_justify = vertical justification (bottom, center, top)
 *  @param aWidth = line width (pen width) (default = 0)
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 *      Use a value min(aSize.x, aSize.y) / 5 for a bold text
 *  @param aItalic = true to simulate an italic font
 *  @param aBold = true to use a bold font
 *  @param aCallback() = function called (if non null) to draw each segment.
 *                  used to draw 3D texts or for plotting, NULL for normal drawings
 *  @param aPlotter = a pointer to a PLOTTER instance, when this function is used to plot
 *                  the text. NULL to draw this text.
 */
void DrawGraphicText( EDA_DRAW_PANEL * aPanel,
                      wxDC * aDC,
                      const wxPoint &aPos,
                      enum EDA_Colors aColor,
                      const wxString &aText,
                      int aOrient,
                      const wxSize &aSize,
                      enum GRTextHorizJustifyType aH_justify,
                      enum GRTextVertJustifyType aV_justify,
                      int aWidth,
                      bool aItalic,
                      bool aBold,
                      void (*aCallback)( int x0, int y0, int xf, int yf ) = NULL,
                      PLOTTER * aPlotter = NULL );


#endif /* __INCLUDE__DRAWTXT_H__ */

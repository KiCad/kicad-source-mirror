/**
 * This file is part of the common libary
 * @file  drawtxt.h
 * @see   common.h
 */


#ifndef __INCLUDE__DRAWTXT_H__
#define __INCLUDE__DRAWTXT_H__ 1

class WinEDA_DrawPanel;

/** Function NegableTextLength
 * Return the text length of a negable string, excluding the ~ markers */
int NegableTextLength( const wxString& aText );


/** Function DrawGraphicText
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
 *  @param aItalic = true to simulate an italic font
 *  @param aNegable = true to enable the ~ char for overbarring
 *  @param aCallback() = function called (if non null) to draw each segment.
 *                  used to draw 3D texts or for plotting, NULL for normal drawings
 */
void DrawGraphicText( WinEDA_DrawPanel * aPanel,
                      wxDC * aDC,
                      const wxPoint &aPos,
                      enum EDA_Colors aColor,
                      const wxString &aText,
                      int aOrient,
                      const wxSize &aSize,
                      enum GRTextHorizJustifyType aH_justify,
                      enum GRTextVertJustifyType aV_justify,
                      int aWidth = 0,
                      bool aItalic = false,
                      bool aNegable = false,
                      void (*aCallback)( int x0, int y0, int xf, int yf ) = NULL );

/** Function PlotGraphicText
 *  same as DrawGraphicText, but plot graphic text insteed of draw it
 *  @param aFormat_plot = plot format (PLOT_FORMAT_POST, PLOT_FORMAT_HPGL, PLOT_FORMAT_GERBER)
 *  @param aPos = text position (according to aH_justify, aV_justify)
 *  @param aColor (enum EDA_Colors) = text color
 *  @param aText = text to draw
 *  @param aOrient = angle in 0.1 degree
 *  @param aSize = text size (size.x or size.y can be < 0 for mirrored texts)
 *  @param aH_justify = horizontal justification (Left, center, right)
 *  @param aV_justify = vertical justification (bottom, center, top)
 *  @param aWidth = line width (pen width) (default = 0)
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 *  @param aItalic = true to simulate an italic font
 *  @param aNegable = true to enable the ~ char for overbarring
 */
void PlotGraphicText(            int                         aFormat_plot,
                                 const wxPoint&              aPos,
                                 enum EDA_Colors             aColor,
                                 const wxString&             aText,
                                 int                         aOrient,
                                 const wxSize&               aSize,
                                 enum GRTextHorizJustifyType aH_justify,
                                 enum GRTextVertJustifyType  aV_justify,
                                 int                         aWidth,
                                 bool                        aItalic = false,
                                 bool                        aNegable = false );


#endif /* __INCLUDE__DRAWTXT_H__ */

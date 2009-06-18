/**
 * Functions to draw and plot text on screen
 * @file drawtxt.cpp
 */
#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "plot_common.h"

#include "trigo.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "class_base_screen.h"

#ifndef DEFAULT_SIZE_TEXT
#   define DEFAULT_SIZE_TEXT 50
#endif

#define EDA_DRAWBASE
#include "hershey_fonts.h"

/* factor used to calculate actual size of shapes from hershey fonts (could be adjusted depending on the font name)
 * Its value is choosen in order to have letters like M, P .. vertical size equal to the vertical char size parameter
 * Of course some shapes can be bigger or smaller than the vertical char size parameter
 */
#define HERSHEY_SCALE_FACTOR 1 / 21.0
double s_HerscheyScaleFactor = HERSHEY_SCALE_FACTOR;



/** Function GetPensizeForBold
 * @return the "best" value for a pen size to draw/plot a bold text
 * @param aTextSize = the char size (height or width)
 */
int GetPenSizeForBold( int aTextSize )
{
    return wxRound( aTextSize / 5.0 );
}


/** Function  Clamp_Text_PenSize
 *As a rule, pen width should not be >1/4em, otherwise the character
 * will be cluttered up in its own fatness
 * so pen width max is aSize/4 for bold text, and aSize/6 for normal text
 * The "best" pen width is aSize/5 for bold texts,
 * so the clamp is consistant with bold option.
 * @param aPenSize = the pen size to clamp
 * @param aSize the char size (height or width)
 * @param aBold = true if text accept bold pen size
 * @return the max pen size allowed
 */
int Clamp_Text_PenSize( int aPenSize, int aSize, bool aBold )
{
    int penSize  = aPenSize;
    double scale = aBold ? 4.0 : 6.0;
    int maxWidth = wxRound( ABS( aSize ) / scale );

    if( penSize > maxWidth )
        penSize = maxWidth;
    return penSize;
}

int Clamp_Text_PenSize( int aPenSize, wxSize aSize, bool aBold )
{
    int size  = MIN( ABS( aSize.x ), ABS( aSize.y ) );
    return Clamp_Text_PenSize(aPenSize, size, aBold);;
}



/* Functions to draw / plot a string.
 *  texts have only one line.
 *  They can be in italic.
 *  Horizontal and Vertical justification are handled.
 *  Texts can be rotated
 *  substrings between ~ markers can be "negated" (i.e. with an over bar
 */

/** Function NegableTextLength
 * Return the text length of a negable string, excluding the ~ markers */
int NegableTextLength( const wxString& aText )
{
    int char_count = aText.length();

    /* Fix the character count, removing the ~ found */
    for( int i = char_count - 1; i >= 0; i-- )
    {
        if( aText[i] == '~' )
        {
            char_count--;
        }
    }

    return char_count;
}



/* Function GetHersheyShapeDescription()
 * return a pointer to the shape corresponding to unicode value AsciiCode
 * Note we use the same font for Bold and Normal texts
 * because kicad handles a variable pen size to do that
 * that gives better results in XOR draw mode.
 */
static const char* GetHersheyShapeDescription( int AsciiCode )
{
#if defined(KICAD_CYRILLIC)
    AsciiCode &= 0x7FF;
    if( AsciiCode > 0x40F && AsciiCode < 0x450 ) // big small Cyr
    {
        return hershey_cyrillic[AsciiCode - 0x410];
    }
    else if( AsciiCode == 0x401 )
    {
        return hershey_cyrillic[0x5];
    }
    else if( AsciiCode == 0x451 )
    {
        return hershey_cyrillic[0x25];
    }
#endif

    if ( AsciiCode > 0x7F )
        AsciiCode = '?';
    if( AsciiCode < 32 )
        AsciiCode = 32;                 /* Clamp control chars */
    AsciiCode -= 32;

    return hershey_simplex[AsciiCode];
}


int ReturnGraphicTextWidth( const wxString& aText, int aXSize, bool aItalic, bool aWidth )
{
    int tally = 0;
    int char_count = aText.length();

    for( int i = 0; i < char_count; i++ )
    {
        int AsciiCode = aText[i];

        if( AsciiCode == '~' ) /* Skip the negation marks */
        {
            continue;
        }

        const char* ptcar = GetHersheyShapeDescription( AsciiCode );
        /* Get metrics */
        int         xsta = *ptcar++ - 'R';
        int         xsto = *ptcar++ - 'R';
        tally += wxRound( aXSize * (xsto - xsta) * s_HerscheyScaleFactor );
    }

    /* Italic correction, 1/8em */
    if( aItalic )
    {
        tally += wxRound( aXSize * 0.125 );
    }
    return tally;
}


/* Helper function for drawing character polygons */
static void DrawGraphicTextPline(
    WinEDA_DrawPanel* aPanel,
    wxDC* aDC,
    EDA_Colors aColor,
    int aWidth,
    bool sketch_mode,
    int point_count,
    wxPoint* coord,
    void (* aCallback)( int x0, int y0, int xf, int yf ) )
{
    if( aCallback )
    {
        for( int ik = 0; ik < (point_count - 1); ik++ )
        {
            aCallback( coord[ik].x, coord[ik].y,
                       coord[ik + 1].x, coord[ik + 1].y );
        }
    }
    else if( sketch_mode )
    {
        for( int ik = 0; ik < (point_count - 1); ik++ )
            GRCSegm( &aPanel->m_ClipBox, aDC, coord[ik].x, coord[ik].y,
                     coord[ik + 1].x, coord[ik + 1].y, aWidth, aColor );
    }
    else
        GRPoly( &aPanel->m_ClipBox, aDC, point_count, coord, 0,
                aWidth, aColor, aColor );
}


/* Helper function for texts with over bar
 */
static int overbar_position( int size_v, int thickness )
{
    return wxRound( ( (double) size_v * 26 * s_HerscheyScaleFactor ) + ( (double) thickness * 1.5 ) );
}


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
 *  @param aWidth = line width (pen width) (use default width if aWidth = 0)
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 *      Use a value min(aSize.x, aSize.y) / 5 for a bold text
 *  @param aItalic = true to simulate an italic font
 *  @param aBold = true to use a bold font. Useful only with default width value (aWidth = 0)
 *  @param aCallback() = function called (if non null) to draw each segment.
 *                  used to draw 3D texts or for plotting, NULL for normal drawings
 */
/****************************************************************************************************/
void DrawGraphicText( WinEDA_DrawPanel* aPanel,
                     wxDC* aDC,
                     const wxPoint& aPos,
                     EDA_Colors aColor,
                     const wxString& aText,
                     int aOrient,
                     const wxSize& aSize,
                     enum GRTextHorizJustifyType aH_justify,
                     enum GRTextVertJustifyType aV_justify,
                     int aWidth,
                     bool aItalic,
                     bool aBold,
                     void (* aCallback)( int x0, int y0, int xf, int yf ) )
/****************************************************************************************************/
{
    int     char_count, AsciiCode;
    int     x0, y0;
    int     size_h, size_v;
    int     ptr;
    int     dx, dy;                         // Draw coordinate for segments to draw. also used in some other calculation
    wxPoint current_char_pos;               // Draw coordinates for the current char
    wxPoint overbar_pos;                    // Start point for the current overbar
    int     overbars;                       // Number of ~ seen
    int     overbar_italic_comp;            // Italic compensation for overbar


    #define        BUF_SIZE 100
    wxPoint coord[BUF_SIZE + 1];                // Buffer coordinate used to draw polylines (one char shape)
    bool    sketch_mode    = false;
    bool    italic_reverse = false;             // true for mirrored texts with m_Size.x < 0

    size_h = aSize.x;                           /* PLEASE NOTE: H is for HORIZONTAL not for HEIGHT */
    size_v = aSize.y;

    if( aWidth == 0 && aBold )       // Use default values if aWidth == 0
        aWidth = GetPenSizeForBold( MIN( aSize.x, aSize.y ) );

    if( aWidth < 0 )
    {
        aWidth = -aWidth;
        sketch_mode = true;
    }

#ifdef CLIP_PEN      // made by draw and plot functions
    aWidth = Clamp_Text_PenSize( aWidth, aSize, aBold );
#endif

    if( size_h < 0 )       // text is mirrored using size.x < 0 (mirror / Y axis)
        italic_reverse = true;

    char_count = NegableTextLength( aText );
    if( char_count == 0 )
        return;

    current_char_pos = aPos;

    dx = ReturnGraphicTextWidth( aText, size_h, aItalic, aWidth );
    dy = size_v;

    /* Do not draw the text if out of draw area! */
    if( aPanel )
    {
        int xm, ym, ll, xc, yc;
        ll = aPanel->GetScreen()->Scale( ABS( dx ) );

        xc = GRMapX( current_char_pos.x );
        yc = GRMapY( current_char_pos.y );

        x0 = aPanel->m_ClipBox.GetX() - ll;
        y0 = aPanel->m_ClipBox.GetY() - ll;
        xm = aPanel->m_ClipBox.GetRight() + ll;
        ym = aPanel->m_ClipBox.GetBottom() + ll;

        if( xc < x0 )
            return;
        if( yc < y0 )
            return;
        if( xc > xm )
            return;
        if( yc > ym )
            return;
    }


    /* Compute the position of the first letter of the text
     * this position is the position of the left bottom point of the letter
     * this is the same as the text position only for a left and bottom justified text
     * In others cases, this position must be calculated from the text position ans size
     */

    switch( aH_justify )
    {
    case GR_TEXT_HJUSTIFY_CENTER:
        current_char_pos.x -= dx / 2;
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        current_char_pos.x -= dx;
        break;

    case GR_TEXT_HJUSTIFY_LEFT:
        break;
    }

    switch( aV_justify )
    {
    case GR_TEXT_VJUSTIFY_CENTER:
        current_char_pos.y += dy / 2;
        break;

    case GR_TEXT_VJUSTIFY_TOP:
         current_char_pos.y += dy;
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        break;
    }

    // Note: if aPanel == NULL, we are using a GL Canvas that handle scaling
    if( aPanel && aPanel->GetScreen()->Scale( aSize.x ) == 0 )
        return;

    /* if a text size is too small, the text cannot be drawn, and it is drawn as a single graphic line */
    if( aPanel && ABS( ( aPanel->GetScreen()->Scale( aSize.x ) ) ) < 3 )
    {
        /* draw the text as a line always vertically centered */
        wxPoint end( current_char_pos.x + dx, current_char_pos.y );

        RotatePoint( &current_char_pos, aPos, aOrient );
        RotatePoint( &end, aPos, aOrient );

        if( aCallback )
            aCallback( current_char_pos.x, current_char_pos.y, end.x, end.y );
        else
            GRLine( &aPanel->m_ClipBox, aDC,
                    current_char_pos.x, current_char_pos.y, end.x, end.y, aWidth, aColor );

        return;
    }

    if( aItalic )
    {
        overbar_italic_comp = overbar_position( size_v, aWidth ) / 8;
        if( italic_reverse )
        {
            overbar_italic_comp = -overbar_italic_comp;
        }
    }
    else
    {
        overbar_italic_comp = 0;
    };

    overbars = 0;
    ptr = 0;   /* ptr = text index */
    while( ptr < char_count )
    {
        if( aText[ptr + overbars] == '~' )
        {
            /* Found an overbar, adjust the pointers */
            overbars++;

            if( overbars % 2 )
            {
                /* Starting the overbar */
                overbar_pos    = current_char_pos;
                overbar_pos.x += overbar_italic_comp;
                overbar_pos.y -= overbar_position( size_v, aWidth );
                RotatePoint( &overbar_pos, aPos, aOrient );
            }
            else
            {
                /* Ending the overbar */
                coord[0]       = overbar_pos;
                overbar_pos    = current_char_pos;
                overbar_pos.x += overbar_italic_comp;
                overbar_pos.y -= overbar_position( size_v, aWidth );
                RotatePoint( &overbar_pos, aPos, aOrient );
                coord[1] = overbar_pos;
                /* Plot the overbar segment */
                DrawGraphicTextPline( aPanel, aDC, aColor, aWidth,
                                      sketch_mode, 2, coord, aCallback );
            }
            continue; /* Skip ~ processing */
        }

        AsciiCode = aText.GetChar( ptr + overbars );

        const char* ptcar = GetHersheyShapeDescription( AsciiCode );
        /* Get metrics */
        int         xsta = *ptcar++ - 'R';
        int         xsto = *ptcar++ - 'R';
        int         point_count = 0;
        bool        endcar = false;
        while( !endcar )
        {
            int hc1, hc2;
            hc1 = *ptcar++;
            if( hc1 )
            {
                hc2 = *ptcar++;
            }
            else
            {
                /* End of character, insert a synthetic pen up */
                hc1    = ' ';
                hc2    = 'R';
                endcar = true;
            }
            hc1 -= 'R'; hc2 -= 'R'; /* Do the Hershey decode thing: coordinates values are coded as <value> + 'R' */

            /* Pen up request */
            if( hc1 == -50 && hc2 == 0 )
            {
                if( point_count )
                {
                    if( aWidth <= 1 )
                        aWidth = 0;
                    DrawGraphicTextPline( aPanel, aDC, aColor, aWidth,
                                          sketch_mode, point_count, coord, aCallback );
                }
                point_count = 0;
            }
            else
            {
                wxPoint currpoint;
                hc1 -= xsta; hc2 -= 11; /* Align the midpoint */
                hc1  = wxRound( hc1 * size_h * s_HerscheyScaleFactor );
                hc2  = wxRound( hc2 * size_v * s_HerscheyScaleFactor );

                // To simulate an italic font, add a x offset depending on the y offset
                if( aItalic )
                    hc1 -= wxRound( italic_reverse ? -hc2 / 8.0 : hc2 / 8.0 );
                currpoint.x = hc1 + current_char_pos.x;
                currpoint.y = hc2 + current_char_pos.y;

                RotatePoint( &currpoint, aPos, aOrient );
                coord[point_count] = currpoint;
                if( point_count < BUF_SIZE - 1 )
                    point_count++;
            }
        }

        /* end draw 1 char */

        ptr++;

        // Apply the advance width
        current_char_pos.x += wxRound( size_h * (xsto - xsta) * s_HerscheyScaleFactor );
    }

    if( overbars % 2 )
    {
        /* Close the last overbar */
        coord[0]       = overbar_pos;
        overbar_pos    = current_char_pos;
        overbar_pos.y -= overbar_position( size_v, aWidth );
        RotatePoint( &overbar_pos, aPos, aOrient );
        coord[1] = overbar_pos;
        /* Plot the overbar segment */
        DrawGraphicTextPline( aPanel, aDC, aColor, aWidth,
                              sketch_mode, 2, coord, aCallback );
    }
}


/* functions used to plot texts, using DrawGraphicText() with a call back function */
static void (*MovePenFct)( wxPoint pos, int state );    // a pointer to actual plot function (HPGL, PS, ..)
static bool s_Plotbegin;                                // Flag to init plot


/*
 * The call back function
 */
/****************************************************************/
static void s_Callback_plot( int x0, int y0, int xf, int yf )
/****************************************************************/
{
    static wxPoint PenLastPos;
    wxPoint        pstart;

    pstart.x = x0;
    pstart.y = y0;
    wxPoint pend;
    pend.x = xf;
    pend.y = yf;
    if( s_Plotbegin )       // First segment to plot
    {
        MovePenFct( pstart, 'U' );
        MovePenFct( pend, 'D' );
        s_Plotbegin = false;
    }
    else
    {
        if( PenLastPos == pstart )      // this is a next segment in a polyline
        {
            MovePenFct( pend, 'D' );
        }
        else                            // New segment to plot
        {
            MovePenFct( pstart, 'U' );
            MovePenFct( pend, 'D' );
        }
    }

    PenLastPos = pend;
}


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
 *      Use a value min(aSize.x, aSize.y) / 5 for a bold text
 *  @param aItalic = true to simulate an italic font
 *  @param aBold = true to use a bold font Useful only with default width value (aWidth = 0)
 */
/******************************************************************************************/
void PlotGraphicText( int                         aFormat_plot,
                      const wxPoint&              aPos,
                      enum EDA_Colors             aColor,
                      const wxString&             aText,
                      int                         aOrient,
                      const wxSize&               aSize,
                      enum GRTextHorizJustifyType aH_justify,
                      enum GRTextVertJustifyType  aV_justify,
                      int                         aWidth,
                      bool                        aItalic,
                      bool                        aBold )
/******************************************************************************************/
{
    if( aWidth == 0 && aBold )      // Use default values if aWidth == 0
        aWidth = GetPenSizeForBold( MIN( aSize.x, aSize.y ) );

#ifdef CLIP_PEN      // made by draw and plot functions
    if ( aWidth >= 0 )
        aWidth = Clamp_Text_PenSize( aWidth, aSize, aBold );
    else
        aWidth = - Clamp_Text_PenSize( -aWidth, aSize, aBold );
#endif

    // Initialise the actual function used to plot lines:
    switch( aFormat_plot )
    {
    case PLOT_FORMAT_POST:
        MovePenFct = LineTo_PS;
        SetCurrentLineWidthPS( aWidth );
        break;

    case PLOT_FORMAT_HPGL:
        MovePenFct = Move_Plume_HPGL;
        break;

    case PLOT_FORMAT_GERBER:
        MovePenFct = LineTo_GERBER;
        /* Gerber tool has to be set outside... */
        break;

    default:
        return;
    }

    if( aColor >= 0 && IsPostScript( aFormat_plot ) )
        SetColorMapPS( aColor );

    s_Plotbegin = true;
    DrawGraphicText( NULL, NULL, aPos, aColor, aText,
                     aOrient, aSize,
                     aH_justify, aV_justify,
                     aWidth, aItalic,
                     aBold,
                     s_Callback_plot );

    /* end text : pen UP ,no move */
    MovePenFct( wxPoint( 0, 0 ), 'Z' );
}

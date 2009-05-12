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
#include "grfonte.h"

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


/* Helper function for drawing character polygons */
static void DrawGraphicTextPline(
    WinEDA_DrawPanel* aPanel,
    wxDC* aDC,
    EDA_Colors aColor,
    int aWidth,
    bool sketch_mode,
    int point_count,
    wxPoint* coord,
    void (*aCallback)( int x0, int y0, int xf, int yf ) )
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


static int overbar_position( int size_v, int thickness )
{
    return size_v * 1.1 + thickness;
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
 *  @param aWidth = line width (pen width) (default = 0)
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 *  @param aItalic = true to simulate an italic font
 *  @param aNegable = true to enable the ~ char for overbarring
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
                     bool aNegable,
                     void (*aCallback)( int x0, int y0, int xf, int yf ) )
/****************************************************************************************************/
{
    int            char_count, AsciiCode;
    int            x0, y0;
    int            size_h, size_v, pitch;
    SH_CODE        f_cod, plume = 'U';
    const SH_CODE* ptcar;
    int            ptr;
    int            dx, dy;        // Draw coordinate for segments to draw. also used in some other calculation
    wxPoint        current_char_pos;        // Draw coordinates for the current char
    wxPoint        overbar_pos;            // Start point for the current overbar
    int            overbars;                // Number of ~ seen

    #define        BUF_SIZE 100
    wxPoint        coord[BUF_SIZE + 1];         // Buffer coordinate used to draw polylines (one char shape)
    bool           sketch_mode    = false;
    bool           italic_reverse = false;      // true for mirrored texts with m_Size.x < 0

    size_h = aSize.x;
    size_v = aSize.y;

    if( aWidth < 0 )
    {
        aWidth = -aWidth;
        sketch_mode = true;
    }
    int thickness = aWidth;
    if( aSize.x < 0 )       // text is mirrored using size.x < 0 (mirror / Y axis)
        italic_reverse = true;

    if( aNegable )
    {
        char_count = NegableTextLength( aText );
    }
    else
    {
        char_count = aText.Len();
    }
    if( char_count == 0 )
        return;

    pitch = (10 * size_h ) / 9;    // this is the pitch between chars
    if( pitch > 0 )
        pitch += thickness;
    else
        pitch -= thickness;

    current_char_pos = aPos;

    /* Do not draw the text if out of draw area! */
    if( aPanel )
    {
        int xm, ym, ll, xc, yc;
        int textsize = ABS( pitch );
        ll = aPanel->GetScreen()->Scale( textsize * char_count );

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
    dx = pitch * char_count;
    dy = size_v;                            /* dx, dy = draw offset between first letter and text center */

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
        current_char_pos.y += dy/2;
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
        wxPoint end( current_char_pos.x + dx, current_char_pos.y);

        RotatePoint( &current_char_pos, aPos, aOrient );
        RotatePoint( &end, aPos, aOrient );

        if( aCallback )
            aCallback( current_char_pos.x, current_char_pos.y, end.x, end.y );
        else
            GRLine( &aPanel->m_ClipBox, aDC,
                current_char_pos.x, current_char_pos.y, end.x, end.y , aWidth, aColor );

        return;
    }

    overbars = 0;
    ptr = 0;   /* ptr = text index */
    while( ptr < char_count )
    {
        if( aNegable )
        {
            if( aText[ptr + overbars] == '~' )
            {
                /* Found an overbar, adjust the pointers */
                overbars++;

                if( overbars % 2 )
                {
                    /* Starting the overbar */
                    overbar_pos = current_char_pos;
                    overbar_pos.y -= overbar_position( size_v, thickness );
                    RotatePoint( &overbar_pos, aPos, aOrient );
                }
                else
                {
                    /* Ending the overbar */
                    coord[0] = overbar_pos;
                    overbar_pos  = current_char_pos;
                    overbar_pos.y -= overbar_position( size_v, thickness );
                    RotatePoint( &overbar_pos, aPos, aOrient );
                    coord[1] = overbar_pos;
                    /* Plot the overbar segment */
                    DrawGraphicTextPline( aPanel, aDC, aColor, aWidth,
                                          sketch_mode, 2, coord, aCallback );
                }
                continue; /* Skip ~ processing */
            }
        }

        AsciiCode = aText.GetChar( ptr + overbars );

#if defined(wxUSE_UNICODE) && defined(KICAD_CYRILLIC)
        AsciiCode &= 0x7FF;
        if( AsciiCode > 0x40F && AsciiCode < 0x450 ) // big small Cyr
            AsciiCode = utf8_to_ascii[AsciiCode - 0x410] & 0xFF;
        else
            AsciiCode = AsciiCode & 0xFF;
#else
        AsciiCode &= 0xFF;
#endif
        ptcar = graphic_fonte_shape[AsciiCode];  /* ptcar pointe la description
                                                  *  du caractere a dessiner */

        int  point_count;
        bool endcar;
        for( point_count = 0, endcar = false; !endcar; ptcar++ )
        {
            f_cod = *ptcar;

            /* get code n de la forme selectionnee */
            switch( f_cod )
            {
            case 'X':
                endcar = true;    /* fin du caractere */
                break;

            case 'U':
                if( point_count && (plume == 'D' ) )
                {
                    if( aWidth <= 1 )
                        aWidth = 0;
                    DrawGraphicTextPline( aPanel, aDC, aColor, aWidth,
                                          sketch_mode, point_count, coord, aCallback );
                }
                plume = f_cod; point_count = 0;
                break;

            case 'D':
                plume = f_cod;
                break;

            default:
            {
                int y, k1, k2;
                wxPoint currpoint;
                y  = k1 = f_cod;        /* trace sur axe V */
                k1 = -( (k1 * size_v) / 9 );

                ptcar++;
                f_cod = *ptcar;

                k2 = f_cod;         /* trace sur axe H */
                k2 = (k2 * size_h) / 9;

                // To simulate an italic font, add a x offset depending on the y offset
                if( aItalic )
                    k2 -= italic_reverse ? -k1 / 8 : k1 / 8;
                currpoint.x = k2 + current_char_pos.x;
                currpoint.y = k1 + current_char_pos.y;

                RotatePoint( &currpoint, aPos, aOrient );
                coord[point_count] = currpoint;
               if( point_count < BUF_SIZE - 1 )
                    point_count++;
                break;
            }
            }

            /* end switch */
        }

        /* end draw 1 char */

        ptr++;
        current_char_pos.x += pitch;    // current_char_pos is now the next position
    }

    if( overbars % 2 )
    {
        /* Close the last overbar */
        coord[0] = overbar_pos;
        overbar_pos  = current_char_pos;
        overbar_pos.y  -= overbar_position( size_v, thickness );
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
/**********************/
static void s_Callback_plot( int x0,
                             int y0,
                             int xf,
                             int yf )
/**********************/
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
 *  @param aItalic = true to simulate an italic font
 *  @param aNegable = true to enable the ~ char for overbarring
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
                      bool                        aNegable )
/******************************************************************************************/
{
    // Initialise the actual function used to plot lines:
    switch( aFormat_plot )
    {
    case PLOT_FORMAT_POST:
        MovePenFct = LineTo_PS;
        break;

    case PLOT_FORMAT_HPGL:
        MovePenFct = Move_Plume_HPGL;
        break;

    case PLOT_FORMAT_GERBER:
        MovePenFct = LineTo_GERBER;
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
                     aWidth, aItalic, aNegable,
                     s_Callback_plot );

    /* end text : pen UP ,no move */
    MovePenFct( wxPoint( 0, 0 ), 'Z' );
}

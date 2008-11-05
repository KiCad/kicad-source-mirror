/*************************************************/
/* drawtxt.cpp : Function to draw and plot texts */
/*************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "plot_common.h"

#include "trigo.h"
#include "macros.h"

#ifndef DEFAULT_SIZE_TEXT
#define DEFAULT_SIZE_TEXT 50
#endif

#define EDA_DRAWBASE
#include "grfonte.h"

/* fonctions locales : */


/****************************************************************************/
void DrawGraphicText( WinEDA_DrawPanel* panel, wxDC* DC,
                      const wxPoint& Pos, int gcolor, const wxString& Text,
                      int orient, const wxSize& Size, int h_justify, int v_justify, int width )
/*****************************************************************************/

/* Draw a graphic text (like module texts)
 *  Text = text to draw
 *  Pos = text position (according to h_justify, v_justify)
 *  Size = text size (size.x or size.y can be < 0 for mirrored texts)
 *  orient = angle in 0.1 degree
 *  mode_color = GR_OR, GR_XOR..
 *  h_justify = horizontal justification (Left, center, right)
 *  v_justify = vertical justification (bottom, center, top)
 *  width = line width (pen width) (default = 0)
 *      if width < 0 : draw segments in sketch mode, width = abs(width)
 */
{
    int            ii, kk, nbchar, AsciiCode, endcar;
    int            k1, k2, x0, y0;
    int            zoom;
    int            size_h, size_v, espacement;
    SH_CODE        f_cod, plume = 'U';
    const SH_CODE* ptcar;
    int            ptr;
    int            ux0, uy0, dx, dy;    // Draw coordinate for segments to draw. also used in some other calculation
    int            cX, cY;              // Texte center
    int            ox, oy;              // Draw coordinates for the current char
    int            coord[100];          // Buffer coordinate used to draw polylines (char shapes)
    bool           sketch_mode = FALSE;

    zoom = panel->GetZoom();

    size_h = Size.x;
    size_v = Size.y;

    if( width < 0 )
    {
        width = -width;
        sketch_mode = TRUE;
    }
    kk = 0;
    ptr = 0;   /* ptr = text index */

    nbchar = Text.Len();
    if( nbchar == 0 )
        return;

    espacement = (10 * size_h ) / 9;    // this is the pitch between chars
    ox = cX = Pos.x;
    oy = cY = Pos.y;

    /* Do not draw the text if out of draw area! */
    if( panel )
    {
        int xm, ym, ll, xc, yc;
        int textsize = ABS( espacement );
        ll = (textsize * nbchar) / zoom;

        xc = GRMapX( cX );
        yc = GRMapY( cY );

        x0 = panel->m_ClipBox.GetX() - ll;
        y0 = panel->m_ClipBox.GetY() - ll;
        xm = panel->m_ClipBox.GetRight() + ll;
        ym = panel->m_ClipBox.GetBottom() + ll;

        if( xc < x0 )
            return;
        if( yc < y0 )
            return;
        if( xc > xm )
            return;
        if( yc > ym )
            return;
    }


    /* Compute the position ux0, uy0 of the first letter , next */
    dx = (espacement * nbchar) / 2;
    dy = size_v / 2;                        /* dx, dy = draw offset between first letter and text center */

    ux0 = uy0 = 0;                          /* Decalage du centre du texte / coord de ref */

    if( (orient == 0) || (orient == 1800) ) /* Horizontal Text */
    {
        switch( h_justify )
        {
        case GR_TEXT_HJUSTIFY_CENTER:
            break;

        case GR_TEXT_HJUSTIFY_RIGHT:
            ux0 = -dx;
            break;

        case GR_TEXT_HJUSTIFY_LEFT:
            ux0 = dx;
            break;
        }

        switch( v_justify )
        {
        case GR_TEXT_VJUSTIFY_CENTER:
            break;

        case GR_TEXT_VJUSTIFY_TOP:
            uy0 = dy;
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            uy0 = -dy;
            break;
        }
    }
    else    /* Vertical Text */
    {
        switch( h_justify )
        {
        case GR_TEXT_HJUSTIFY_CENTER:
            break;

        case GR_TEXT_HJUSTIFY_RIGHT:
            ux0 = -dy;
            break;

        case GR_TEXT_HJUSTIFY_LEFT:
            ux0 = dy;
            break;
        }

        switch( v_justify )
        {
        case GR_TEXT_VJUSTIFY_CENTER:
            break;

        case GR_TEXT_VJUSTIFY_TOP:
            uy0 = dx;
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            uy0 = -dx;
            break;
        }
    }

    cX += ux0;
    cY += uy0;

    ox = cX - dx;
    oy = cY + dy;

    if( (Size.x / zoom) == 0 )
        return;

    if( ABS( (Size.x / zoom) ) < 3 )    /* chars trop petits pour etre dessines */
    {                                   /* le texte est symbolise par une barre */
        dx = (espacement * nbchar) / 2;
        dy = size_v / 2;                /* Decalage du debut du texte / centre */

        ux0 = cX - dx;
        uy0 = cY;

        dx += cX;
        dy = cY;

        RotatePoint( &ux0, &uy0, cX, cY, orient );
        RotatePoint( &dx, &dy, cX, cY, orient );

        GRLine( &panel->m_ClipBox, DC, ux0, uy0, dx, dy, width, gcolor );

        return;
    }

#if 0
    dx  = (espacement * nbchar) / 2;
    dy  = size_v / 2;/* Decalage du debut du texte / centre */

    ux0 = cX - dx;
    uy0 = cY;

    dx += cX;
    dy = cY;

    RotatePoint( &ux0, &uy0, cX, cY, orient );
    RotatePoint( &dx, &dy, cX, cY, orient );

    DC->SetTextForeground( wxColour(
                              ColorRefs[gcolor].r,
                              ColorRefs[gcolor].g,
                              ColorRefs[gcolor].b ) );

    DC->DrawRotatedText( Text, GRMapX( ux0 ), GRMapY( uy0 ), (double) orient / 10.0 );
    return;
#endif

    while( kk++ < nbchar )
    {
        x0 = 0; y0 = 0;
#if defined(wxUSE_UNICODE) && defined(KICAD_CYRILLIC)
	AsciiCode = Text.GetChar(ptr) & 0x7FF;
	if ( AsciiCode > 0x40F && AsciiCode < 0x450 ) // big small Cyr
	    AsciiCode = utf8_to_ascii[AsciiCode - 0x410] & 0xFF;
	else
	    AsciiCode = AsciiCode & 0xFF;
#else
         AsciiCode = Text.GetChar( ptr ) & 255;
#endif
        ptcar = graphic_fonte_shape[AsciiCode];  /* ptcar pointe la description
                                                  *  du caractere a dessiner */

        for( ii = 0, endcar = FALSE; !endcar; ptcar++ )
        {
            f_cod = *ptcar;

            /* get code n de la forme selectionnee */
            switch( f_cod )
            {
            case 'X':
                endcar = TRUE;    /* fin du caractere */
                break;

            case 'U':
                if( ii && (plume == 'D' ) )
                {
                    if( width <= 1 )
                        GRPoly( &panel->m_ClipBox, DC, ii / 2, coord, 0, 0,
                                gcolor, gcolor );
                    else if( sketch_mode )
                    {
                        int ik, * coordptr;
                        coordptr = coord;
                        for( ik = 0; ik < (ii - 2); ik += 2, coordptr += 2 )
                            GRCSegm( &panel->m_ClipBox, DC, *coordptr, *(coordptr + 1),
                                     *(coordptr + 2), *(coordptr + 3), width, gcolor );
                    }
                    else
                        GRPoly( &panel->m_ClipBox, DC, ii / 2, coord, 0,
                                width, gcolor, gcolor );
                }
                plume = f_cod; ii = 0;
                break;

            case 'D':
                plume = f_cod;
                break;

            default:
                {
                    k1 = f_cod;     /* trace sur axe V */
                    k1 = -( (k1 * size_v) / 9 );

                    ptcar++;
                    f_cod = *ptcar;

                    k2    = f_cod;  /* trace sur axe H */
                    k2    = (k2 * size_h) / 9;
                    dx    = k2 + ox; dy = k1 + oy;

                    RotatePoint( &dx, &dy, cX, cY, orient );
                    coord[ii++] = dx;
                    coord[ii++] = dy;
                    break;
                }
            }

            /* end switch */
        }

        /* end boucle for = end trace de 1 caractere */

        ptr++; ox += espacement;
    }

    /* end trace du texte */
}


/******************************************************************************************/
void PlotGraphicText( int format_plot, const wxPoint& Pos, int gcolor,
                      const wxString& Text,
                      int orient, const wxSize& Size, int h_justify, int v_justify )
/******************************************************************************************/

/*
 *  id DrawGraphicText, for plot graphic text
 */
{
    int            kk, nbchar, end, AsciiCode;
    int            k1, k2, x0, y0, ox, oy;
    int            size_h, size_v, espacement;
    SH_CODE        f_cod, plume = 'U';
    const SH_CODE* ptcar;
    int            ptr;
    int            ux0, uy0, dx, dy;    // Coord de trace des segments de texte & variables de calcul */
    int            cX, cY;              // Centre du texte

    void           (*FctPlume)( wxPoint pos, int state );

    switch( format_plot )
    {
    case PLOT_FORMAT_POST:
        FctPlume = LineTo_PS;
        break;

    case PLOT_FORMAT_HPGL:
        FctPlume = Move_Plume_HPGL;
        break;

    case PLOT_FORMAT_GERBER:
    default:
        return;
    }

    if( gcolor >= 0 &&  IsPostScript( format_plot ) )
        SetColorMapPS( gcolor );

    size_h = Size.x;
    size_v = Size.y;
    if( size_h == 0 )
        size_h = DEFAULT_SIZE_TEXT;
    if( size_v == 0 )
        size_v = DEFAULT_SIZE_TEXT;

    kk = 0;
    ptr = 0; /* ptr = text index */

    /* calcul de la position du debut des textes: ox et oy */
    nbchar = Text.Len();

    espacement = (10 * size_h ) / 9;
    ox = cX = Pos.x;
    oy = cY = Pos.y;

    /* Calcul du cadrage du texte */
    dx = (espacement * nbchar) / 2;
    dy = size_v / 2;                        /* Decalage du debut du texte / centre */

    ux0 = uy0 = 0;                          /* Decalage du centre du texte / ccord de ref */

    if( (orient == 0) || (orient == 1800) ) /* Texte Horizontal */
    {
        switch( h_justify )
        {
        case GR_TEXT_HJUSTIFY_CENTER:
            break;

        case GR_TEXT_HJUSTIFY_RIGHT:
            ux0 = -dx;
            break;

        case GR_TEXT_HJUSTIFY_LEFT:
            ux0 = dx;
            break;
        }

        switch( v_justify )
        {
        case GR_TEXT_VJUSTIFY_CENTER:
            break;

        case GR_TEXT_VJUSTIFY_TOP:
            uy0 = dy;
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            uy0 = -dy;
            break;
        }
    }
    else    /* Texte Vertical */
    {
        switch( h_justify )
        {
        case GR_TEXT_HJUSTIFY_CENTER:
            break;

        case GR_TEXT_HJUSTIFY_RIGHT:
            ux0 = -dy;
            break;

        case GR_TEXT_HJUSTIFY_LEFT:
            ux0 = dy;
            break;
        }

        switch( v_justify )
        {
        case GR_TEXT_VJUSTIFY_CENTER:
            break;

        case GR_TEXT_VJUSTIFY_TOP:
            uy0 = dx;
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            uy0 = -dx;
            break;
        }
    }

    cX += ux0;
    cY += uy0;   /* cX, cY = coord du centre du texte */

    ox  = -dx;
    oy = +dy;    /* ox, oy = coord debut texte, relativement au centre */

    FctPlume( wxPoint( 0, 0 ), 'Z' );

    while( kk++ < nbchar )
    {
#if defined(wxUSE_UNICODE) && defined(KICAD_CYRILLIC)
	AsciiCode = Text.GetChar(ptr) & 0x7FF;
	if ( AsciiCode > 0x40F && AsciiCode < 0x450 ) // big small Cyr
	    AsciiCode = utf8_to_ascii[AsciiCode - 0x410] & 0xFF;
	else
	    AsciiCode = AsciiCode & 0xFF;
#else
        AsciiCode = Text.GetChar( ptr ) & 0xFF;
#endif
        ptcar = graphic_fonte_shape[AsciiCode];  /* ptcar pointe la description
                                                  *  du caractere a dessiner */

        for( end = 0; end == 0; ptcar++ )
        {
            f_cod = *ptcar;

            /* get code n de la forme selectionnee */
            switch( f_cod )
            {
            case 'X':
                end = 1;           /* fin du caractere */

            case 'U':
            case 'D':
                plume = f_cod; break;

            default:
                k1 = f_cod;      /* trace sur axe V */
                k1 = -(k1 * size_v) / 9;
                ptcar++;
                f_cod = *ptcar;

                k2    = f_cod;  /* trace sur axe H */
                k2    = (k2 * size_h) / 9;

                dx    = k2 + ox;
                dy = k1 + oy;

                RotatePoint( &dx, &dy, orient );
                FctPlume( wxPoint( cX + dx, cY + dy ), plume );

                x0 = k2;
                y0 = k1;
                break;
            }

            /* end switch */
        }

        /* end boucle for = end trace de 1 caractere */

        FctPlume( wxPoint( 0, 0 ), 'Z' );
        ptr++; ox += espacement;
    }

    /* end trace du texte */
    FctPlume( wxPoint( 0, 0 ), 'Z' );
}

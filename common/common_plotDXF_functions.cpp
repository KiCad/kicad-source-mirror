/******************************************/
/* Kicad: Common plot DXF Routines */
/******************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "plot_common.h"
#include "macros.h"
#include "kicad_string.h"

/***********************************************************************************/
void DXF_PLOTTER::set_viewport( wxPoint offset,
                                double aScale, int orient )
/***********************************************************************************/

/* Set the plot offset for the current plotting
 */
{
    wxASSERT( !output_file );
    plot_offset  = offset;
    plot_scale   = aScale;
    device_scale = 1;
    set_default_line_width( 0 );    /* No line width on DXF */
    plot_orient_options = 0;        /* No mirroring on DXF */
    current_color = BLACK;
}


/*****************************************************************/
void DXF_PLOTTER::start_plot( FILE* fout )
/*****************************************************************/
{
    wxASSERT( !output_file );
    output_file = fout;
    /* DXF HEADER - Boilerplate */
    fputs(
        "0\nSECTION\n2\nHEADER\n9\n$ANGBASE\n50\n0.0\n9\n$ANGDIR\n70\n0\n0\nENDSEC\n0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLTYPE\n70\n1\n0\nLTYPE\n2\nCONTINUOUS\n70\n0\n3\nSolid line\n72\n65\n73\n0\n40\n0.0\n0\nENDTAB\n",
        output_file );
    /* Layer table - one layer per color */
    fprintf( output_file, "0\nTABLE\n2\nLAYER\n70\n%d\n", NBCOLOR );
    for( int i = 0; i<NBCOLOR; i++ )
    {
        wxString cname = ColorRefs[i].m_Name;
        fprintf( output_file, "0\nLAYER\n2\n%s\n70\n0\n62\n%d\n6\nCONTINUOUS\n",
                 CONV_TO_UTF8( cname ), i + 1 );
    }

    /* End of layer table, begin entities */
    fputs( "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n", output_file );
}


/**********************************/
void DXF_PLOTTER::end_plot()
/**********************************/
{
    wxASSERT( output_file );
    /* DXF FOOTER */
    fputs( "0\nENDSEC\n0\nEOF\n", output_file );
    fclose( output_file );
    output_file = 0;
}


/******************************/
void DXF_PLOTTER::set_color( int color )
/******************************/

/*
 * color = color index in ColorRefs[]
 */
{
    wxASSERT( output_file );
    if( (color >= 0 && color_mode)
       || (color == BLACK)
       || (color == WHITE) )
    {
        current_color = color;
    }
}


/************************************************************/
void DXF_PLOTTER::rect( wxPoint p1, wxPoint p2, FILL_T fill, int width )
/************************************************************/
{
    wxASSERT( output_file );
    move_to( p1 );
    line_to( wxPoint( p1.x, p2.y ) );
    line_to( wxPoint( p2.x, p2.y ) );
    line_to( wxPoint( p2.x, p1.y ) );
    finish_to( wxPoint( p1.x, p1.y ) );
}


/************************************************************/
void DXF_PLOTTER::circle( wxPoint centre, int diameter, FILL_T fill, int width )
/************************************************************/
{
    wxASSERT( output_file );
    double rayon = user_to_device_size( diameter / 2 );
    user_to_device_coordinates( centre );
    if( rayon > 0 )
    {
        wxString cname = ColorRefs[current_color].m_Name;
        fprintf( output_file, "0\nCIRCLE\n8\n%s\n10\n%d.0\n20\n%d.0\n40\n%g\n",
                 CONV_TO_UTF8( cname ),
                 centre.x, centre.y, rayon );
    }
}


/*****************************************************/
void DXF_PLOTTER::poly( int nb, int* coord, FILL_T fill, int width )
/*****************************************************/

/* Trace un polygone (ferme si rempli) en format DXF
 * coord = tableau des coord des sommets
 * nb = nombre de coord ( 1 coord = 2 elements: X et Y du tableau )
 * fill : si != 0 polygone rempli
 */
{
    wxASSERT( output_file );
    if( nb <= 1 )
        return;

    move_to( wxPoint( coord[0], coord[1] ) );
    for( int ii = 1; ii < nb; ii++ )
        line_to( wxPoint( coord[ii * 2], coord[(ii * 2) + 1] ) );

    /* Fermeture eventuelle du polygone */
    if( fill )
    {
        int ii = (nb - 1) * 2;
        if( (coord[ii] != coord[0] ) || (coord[ii + 1] != coord[1]) )
            line_to( wxPoint( coord[0], coord[1] ) );
    }
    pen_finish();
}


/**********************************************/
void DXF_PLOTTER::pen_to( wxPoint pos, char plume )
/**********************************************/

/*
 * deplace la plume levee (plume = 'U') ou baissee (plume = 'D')
 * en position x,y
 * Unites en Unites DESSIN
 * Si plume = 'Z' lever de plume sans deplacement
 */
{
    wxASSERT( output_file );
    if( plume == 'Z' )
    {
        return;
    }
    user_to_device_coordinates( pos );

    if( pen_lastpos != pos && plume == 'D' )
    {
        /* DXF LINE */
        wxString cname = ColorRefs[current_color].m_Name;
        fprintf( output_file, "0\nLINE\n8\n%s\n10\n%d.0\n20\n%d.0\n11\n%d.0\n21\n%d.0\n",
                 CONV_TO_UTF8( cname ),
                 pen_lastpos.x, pen_lastpos.y, pos.x, pos.y );
    }
    pen_lastpos = pos;
}


void DXF_PLOTTER::set_dash( bool dashed )
{
    /* NOP for now */
    wxASSERT( output_file );
}


void DXF_PLOTTER::thick_segment( wxPoint start, wxPoint end, int width,
                                 GRTraceMode tracemode )

/** Function Plot a filled segment (track)
 * @param start = starting point
 * @param end = ending point
 * @param aWidth = segment width (thickness)
 * @param aPlotMode = FILLED, SKETCH ..
 */
{
    wxASSERT( output_file );

    if( tracemode == FILAIRE )  /* just a line is Ok */
    {
        move_to( start );
        finish_to( end );
    }
    else
        segment_as_oval( start, end, width, tracemode );
}


/********************************************************************/
void DXF_PLOTTER::arc( wxPoint centre, int StAngle, int EndAngle, int rayon,
                       FILL_T fill, int width )
/********************************************************************/

/* trace d'un arc de cercle:
 * centre = coord du centre
 * StAngle, EndAngle = angle de debut et fin
 * rayon = rayon de l'arc
 */
{
    wxASSERT( output_file );

    if( rayon <= 0 )
        return;

    user_to_device_coordinates( centre );
    rayon = user_to_device_size( rayon );

    /* DXF ARC */
    wxString cname = ColorRefs[current_color].m_Name;
    fprintf( output_file, "0\nARC\n8\n%s\n10\n%d.0\n20\n%d.0\n40\n%d.0\n50\n%d.0\n51\n%d.0\n",
             CONV_TO_UTF8( cname ),
             centre.x, centre.y, rayon,
             StAngle / 10, EndAngle / 10 );
}


/***********************************************************************************/
void DXF_PLOTTER::flash_pad_oval( wxPoint pos, wxSize size, int orient,
                                  GRTraceMode trace_mode )
/************************************************************************************/
/* Trace 1 pastille PAD_OVAL en position pos_X,Y , de dim size.x, size.y */
{
    wxASSERT( output_file );

    /* la pastille est ramenee a une pastille ovale avec size.y > size.x
     *  ( ovale vertical en orientation 0 ) */
    if( size.x > size.y )
    {
        EXCHG( size.x, size.y ); orient += 900;
        if( orient >= 3600 )
            orient -= 3600;
    }
    sketch_oval( pos, size, orient, -1 );
}


/*******************************************************************************/
void DXF_PLOTTER::flash_pad_circle( wxPoint pos, int diametre,
                                    GRTraceMode trace_mode )
/*******************************************************************************/
/* Trace 1 pastille RONDE (via,pad rond) en position pos */
{
    wxASSERT( output_file );
    circle( pos, diametre, NO_FILL );
}


/**************************************************************************/
void DXF_PLOTTER::flash_pad_rect( wxPoint pos, wxSize padsize,
                                  int orient, GRTraceMode trace_mode )
/**************************************************************************/

/*
 *  Trace 1 pad rectangulaire vertical ou horizontal ( Pad rectangulaire )
 *  donne par son centre et ses dimensions X et Y
 *  Units are user units
 */
{
    wxASSERT( output_file );
    wxSize size;
    int    ox, oy, fx, fy;

    size.x = padsize.x / 2;  size.y = padsize.y / 2;

    if( size.x < 0 )
        size.x = 0;
    if( size.y < 0 )
        size.y = 0;

    /* Si une des dimensions est nulle, le trace se reduit a 1 trait */
    if( size.x == 0 )
    {
        ox = pos.x; oy = pos.y - size.y;
        RotatePoint( &ox, &oy, pos.x, pos.y, orient );
        fx = pos.x; fy = pos.y + size.y;
        RotatePoint( &fx, &fy, pos.x, pos.y, orient );
        move_to( wxPoint( ox, oy ) );
        finish_to( wxPoint( fx, fy ) );
        return;
    }
    if( size.y == 0 )
    {
        ox = pos.x - size.x; oy = pos.y;
        RotatePoint( &ox, &oy, pos.x, pos.y, orient );
        fx = pos.x + size.x; fy = pos.y;
        RotatePoint( &fx, &fy, pos.x, pos.y, orient );
        move_to( wxPoint( ox, oy ) );
        finish_to( wxPoint( fx, fy ) );
        return;
    }

    ox = pos.x - size.x; oy = pos.y - size.y;
    RotatePoint( &ox, &oy, pos.x, pos.y, orient );
    move_to( wxPoint( ox, oy ) );

    fx = pos.x - size.x; fy = pos.y + size.y;
    RotatePoint( &fx, &fy, pos.x, pos.y, orient );
    line_to( wxPoint( fx, fy ) );

    fx = pos.x + size.x; fy = pos.y + size.y;
    RotatePoint( &fx, &fy, pos.x, pos.y, orient );
    line_to( wxPoint( fx, fy ) );

    fx = pos.x + size.x; fy = pos.y - size.y;
    RotatePoint( &fx, &fy, pos.x, pos.y, orient );
    line_to( wxPoint( fx, fy ) );

    finish_to( wxPoint( ox, oy ) );
}


/*******************************************************************/
void DXF_PLOTTER::flash_pad_trapez( wxPoint pos, wxSize size, wxSize delta,
                                    int orient, GRTraceMode trace_mode )
/*******************************************************************/

/*
 *  Trace 1 pad trapezoidal donne par :
 *  son centre pos.x,pos.y
 *  ses dimensions dimX et dimY
 *  les variations deltaX et deltaY
 *  son orientation orient et 0.1 degres
 *  le mode de trace (FILLED, SKETCH, FILAIRE)
 *  Le trace n'est fait que pour un trapeze, c.a.d que deltaX ou deltaY
 *  = 0.
 *
 *  les notation des sommets sont ( vis a vis de la table tracante )
 *      0 ------------- 3
 *        .			   .
 *          .		  .
 *           .		 .
 *            1 --- 2
 */
{
    wxASSERT( output_file );
    wxPoint polygone[4];    /* coord des sommets / centre du pad */
    wxPoint coord[4];       /* coord reelles des sommets du trapeze a tracer */
    int     moveX, moveY; /* variation de position plume selon axe X et Y , lors
                           *  du remplissage du trapeze */
    moveX = moveY = 0;

    size.x  /= 2;  size.y /= 2;
    delta.x /= 2; delta.y /= 2;

    polygone[0].x = -size.x - delta.y; polygone[0].y = +size.y + delta.x;
    polygone[1].x = -size.x + delta.y; polygone[1].y = -size.y - delta.x;
    polygone[2].x = +size.x - delta.y; polygone[2].y = -size.y + delta.x;
    polygone[3].x = +size.x + delta.y; polygone[3].y = +size.y - delta.x;

    for( int ii = 0; ii < 4; ii++ )
    {
        coord[ii].x = polygone[ii].x + pos.x;
        coord[ii].y = polygone[ii].y + pos.y;
        RotatePoint( &coord[ii], pos, orient );
    }

    // Plot edge:
    move_to( coord[0] );
    line_to( coord[1] );
    line_to( coord[2] );
    line_to( coord[3] );
    finish_to( coord[0] );
}

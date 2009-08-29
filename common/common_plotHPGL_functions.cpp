/******************************************/
/* Kicad: Common plot HPGL Routines */
/******************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "plot_common.h"
#include "macros.h"
#include "kicad_string.h"

/* From decimils to plu */
const double SCALE_HPGL = 0.102041;

/***********************************************************************************/
void HPGL_PLOTTER::set_viewport( wxPoint offset, double aScale, int orient )
/***********************************************************************************/

/* Set the plot offset for the current plotting
 */
{
    wxASSERT(!output_file);
    plot_offset = offset;
    plot_scale = aScale;
    device_scale = SCALE_HPGL;
    set_default_line_width(100); /* epaisseur du trait standard en 1/1000 pouce */
    plot_orient_options = orient;
}

/*****************************************************************/
void HPGL_PLOTTER::start_plot( FILE *fout )
/*****************************************************************/
{
    wxASSERT(!output_file);
    output_file = fout;
    fprintf( output_file, "IN;VS%d;PU;PA;SP%d;\n", pen_speed, pen_number );
}

/**********************************/
void HPGL_PLOTTER::end_plot()
/**********************************/
{
    wxASSERT(output_file);
    fputs( "PU;PA;SP0;\n", output_file );
    fclose( output_file );
    output_file = 0;
}

/************************************************************/
void HPGL_PLOTTER::rect( wxPoint p1, wxPoint p2, FILL_T fill, int width )
/************************************************************/
{
    wxASSERT(output_file);
    user_to_device_coordinates( p2 );
    move_to(p1);
    fprintf( output_file, "EA %d,%d;\n", p2.x, p2.y );
    pen_finish();
}

/************************************************************/
void HPGL_PLOTTER::circle( wxPoint centre, int diameter, FILL_T fill, int width )
/************************************************************/
{
    wxASSERT(output_file);
    double rayon = user_to_device_size(diameter / 2);

    if( rayon > 0 )
    {
	move_to(centre);
	fprintf( output_file, "CI %g;\n", rayon);
	pen_finish();
    }
}

/*****************************************************/
void HPGL_PLOTTER::poly( int nb, int* coord, FILL_T fill, int width )
/*****************************************************/

/* Trace un polygone (ferme si rempli) en format HPGL
  * coord = tableau des coord des sommets
  * nb = nombre de coord ( 1 coord = 2 elements: X et Y du tableau )
  * fill : si != 0 polygone rempli
 */
{
    wxASSERT(output_file);
    if( nb <= 1 )
        return;

    move_to( wxPoint( coord[0], coord[1] ) );
    for(int ii = 1; ii < nb; ii++ )
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

/***************************/
void HPGL_PLOTTER::pen_control( int plume )
/***************************/

/* leve (plume = 'U') ou baisse (plume = 'D') la plume
 */
{
    wxASSERT(output_file);
    switch (plume) {
    case 'U':
        if( pen_state != 'U' )
	{
            fputs( "PU;", output_file );
	    pen_state = 'U';
	}
	break;
    case 'D':
        if( pen_state != 'D' )
	{
            fputs( "PD;", output_file );
	    pen_state = 'D';
	}
	break;
    case 'Z':
	fputs( "PU;", output_file );
	pen_state = 'U';
	pen_lastpos.x = -1;
	pen_lastpos.y = -1;
	break;
    }
}

/**********************************************/
void HPGL_PLOTTER::pen_to( wxPoint pos, char plume )
/**********************************************/

/*
  * deplace la plume levee (plume = 'U') ou baissee (plume = 'D')
  * en position x,y
  * Unites en Unites DESSIN
  * Si plume = 'Z' lever de plume sans deplacement
 */
{
    wxASSERT(output_file);
    if( plume == 'Z' )
    {
        pen_control( 'Z' );
        return;
    }
    pen_control( plume );
    user_to_device_coordinates( pos );

    if (pen_lastpos != pos)
	fprintf( output_file, "PA %d,%d;\n", pos.x, pos.y );
    pen_lastpos = pos;
}

void HPGL_PLOTTER::set_dash( bool dashed )
{
    wxASSERT(output_file);
    if (dashed)
	fputs("LI 2;\n", stderr);
    else
	fputs("LI;\n", stderr);
}

void HPGL_PLOTTER::thick_segment( wxPoint start, wxPoint end, int width,
	GRTraceMode tracemode)
/** Function Plot a filled segment (track)
 * @param start = starting point
 * @param end = ending point
 * @param aWidth = segment width (thickness)
 * @param aPlotMode = FILLED, SKETCH ..
 */
{
    wxASSERT(output_file);
    wxPoint center;
    wxSize  size;

    if( (pen_diameter >= width) || (tracemode == FILAIRE) )  /* just a line is Ok */
    {
        move_to( start );
        finish_to( end );
    }
    else
	segment_as_oval(start, end, width, tracemode);
}

/********************************************************************/
void HPGL_PLOTTER::arc( wxPoint centre, int StAngle, int EndAngle, int rayon,
       FILL_T fill, int width )
/********************************************************************/

/* trace d'un arc de cercle:
  * centre = coord du centre
  * StAngle, EndAngle = angle de debut et fin
  * rayon = rayon de l'arc
  * commande
  *     PU;PA x,y;PD;AA start_arc_X, start_arc_Y, angle, NbSegm; PU;
  * ou	PU;PA x,y;PD;AA start_arc_X, start_arc_Y, angle; PU;
 */
{
    wxASSERT(output_file);
    wxPoint cmap;           /* point de depart */
    wxPoint cpos;           /* centre */
    float   angle;          /* angle de l'arc*/

    if( rayon <= 0 )
        return;

    cpos = centre;
    user_to_device_coordinates( cpos );

    if( plot_orient_options == PLOT_MIROIR )
	angle = (StAngle - EndAngle) / 10.0;
    else
    angle = (EndAngle - StAngle) / 10.0;
    /* Calcul des coord du point de depart : */
    cmap.x = (int) ( centre.x + ( rayon * cos( StAngle * M_PI / 1800 ) ) );
    cmap.y = (int) ( centre.y - ( rayon * sin( StAngle * M_PI / 1800 ) ) );
    user_to_device_coordinates( cmap );

    fprintf( output_file, "PU;PA %d,%d;PD;AA %d,%d, ", cmap.x, cmap.y, cpos.x, cpos.y );
    fprintf( output_file, "%f", angle );
    fprintf( output_file, ";PU;\n" );
    pen_finish();
}

/***********************************************************************************/
void HPGL_PLOTTER::flash_pad_oval( wxPoint pos, wxSize size, int orient,
	GRTraceMode trace_mode )
/************************************************************************************/
/* Trace 1 pastille PAD_OVAL en position pos_X,Y , de dim size.x, size.y */
{
    wxASSERT(output_file);
    int rayon, deltaxy, cx, cy;

    /* la pastille est ramenee a une pastille ovale avec size.y > size.x
     *  ( ovale vertical en orientation 0 ) */
    if( size.x > size.y )
    {
        EXCHG( size.x, size.y ); orient += 900;
        if( orient >= 3600 )
            orient -= 3600;
    }
    deltaxy = size.y - size.x; /* = distance entre centres de l'ovale */
    rayon   = size.x / 2;
    if( trace_mode == FILLED )
    {
        flash_pad_rect( pos, wxSize( size.x, deltaxy+pen_diameter ),
                                 orient, trace_mode );
        cx = 0; cy = deltaxy / 2;
        RotatePoint( &cx, &cy, orient );
        flash_pad_circle( wxPoint( cx + pos.x, cy + pos.y ), size.x, trace_mode );
        cx = 0; cy = -deltaxy / 2;
        RotatePoint( &cx, &cy, orient );
        flash_pad_circle( wxPoint( cx + pos.x, cy + pos.y ), size.x, trace_mode );
    }
    else    /* Trace en mode SKETCH */
    {
	sketch_oval(pos, size, orient, pen_diameter);
    }
}

/*******************************************************************************/
void HPGL_PLOTTER::flash_pad_circle(wxPoint pos, int diametre,
	    GRTraceMode trace_mode)
/*******************************************************************************/
/* Trace 1 pastille RONDE (via,pad rond) en position pos */
{
    wxASSERT(output_file);
    int  rayon, delta;

    user_to_device_coordinates( pos );

    delta = pen_diameter - pen_overlap;
    rayon = diametre / 2;
    if( trace_mode != FILAIRE )
    {
        rayon = (diametre - pen_diameter ) / 2;
    }

    if( rayon < 0 )
    {
        rayon = 0;
    }
    wxSize rsize( rayon, rayon );

    user_to_device_size( rsize );

    fprintf( output_file, "PA %d,%d;CI %d;\n", pos.x, pos.y, rsize.x );
    if( trace_mode == FILLED ) /* Trace en mode Remplissage */
    {
        if( delta > 0 )
        {
            while( (rayon -= delta ) >= 0 )
            {
                rsize.x = rsize.y = rayon;
                user_to_device_size( rsize );
                fprintf( output_file, "PA %d,%d; CI %d;\n", pos.x, pos.y, rsize.x );
            }
        }
    }
    pen_finish();
    return;
}

/**************************************************************************/
void HPGL_PLOTTER::flash_pad_rect(wxPoint pos, wxSize padsize,
	    int orient, GRTraceMode trace_mode)
/**************************************************************************/
/*
 *  Trace 1 pad rectangulaire vertical ou horizontal ( Pad rectangulaire )
 *  donne par son centre et ses dimensions X et Y
 *  Units are user units
 */
{
    wxASSERT(output_file);
    wxSize size;
    int    delta;
    int    ox, oy, fx, fy;

    size.x = padsize.x / 2;  size.y = padsize.y / 2;
    if( trace_mode != FILAIRE )
    {
	size.x = (padsize.x - (int) pen_diameter) / 2;
	size.y = (padsize.y - (int) pen_diameter) / 2;
    }

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

    if( trace_mode == FILLED )
    {
	/* Trace en mode Remplissage */
	delta = (int) (pen_diameter - pen_overlap);
	if( delta > 0 )
	    while( (size.x > 0) && (size.y > 0) )
	    {
		size.x -= delta; size.y -= delta;
		if( size.x < 0 )
		    size.x = 0;
		if( size.y < 0 )
		    size.y = 0;

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
    }
}

/*******************************************************************/
void HPGL_PLOTTER::flash_pad_trapez( wxPoint pos, wxSize size, wxSize delta,
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
    wxASSERT(output_file);
    wxPoint polygone[4];    /* coord des sommets / centre du pad */
    wxPoint coord[4];       /* coord reelles des sommets du trapeze a tracer */
    int     moveX, moveY; /* variation de position plume selon axe X et Y , lors
			   *  du remplissage du trapeze */
    moveX = moveY = pen_diameter;

    size.x  /= 2;  size.y /= 2;
    delta.x /= 2; delta.y /= 2;

    polygone[0].x = -size.x - delta.y; polygone[0].y = +size.y + delta.x;
    polygone[1].x = -size.x + delta.y; polygone[1].y = -size.y - delta.x;
    polygone[2].x = +size.x - delta.y; polygone[2].y = -size.y + delta.x;
    polygone[3].x = +size.x + delta.y; polygone[3].y = +size.y - delta.x;

    /* Trace du contour */
    polygone[0].x += moveX; polygone[0].y -= moveY;
    polygone[1].x += moveX; polygone[1].y += moveY;
    polygone[2].x -= moveX; polygone[2].y += moveY;
    polygone[3].x -= moveX; polygone[3].y -= moveY;

    for(int ii = 0; ii < 4; ii++ )
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

    if( trace_mode == FILLED )
    {
	int     jj;
	/* Fill the shape */
	moveX = moveY = pen_diameter - pen_overlap;
	/* calcul de jj = hauteur du remplissage */
	if( delta.y ) /* Trapeze horizontal */
	{
	    jj = size.y - (int) ( pen_diameter + (2 * pen_overlap) );
    }
    else
    {
	    jj = size.x - (int) ( pen_diameter + (2 * pen_overlap) );
	}

	/* Calcul de jj = nombre de segments a tracer pour le remplissage */
	jj = jj / (int) (pen_diameter - pen_overlap);

	/* Trace du contour */
	for( ; jj > 0; jj-- )
	{
	    polygone[0].x += moveX; polygone[0].y -= moveY;
	    polygone[1].x += moveX; polygone[1].y += moveY;
	    polygone[2].x -= moveX; polygone[2].y += moveY;
	    polygone[3].x -= moveX; polygone[3].y -= moveY;

	    /* Test de limitation de variation des dimensions :
	     *  si les sommets se "croisent", il ne faut plus modifier les
	     *  coordonnees correspondantes */
	    if( polygone[0].x > polygone[3].x )
	    {  /* croisement sur axe X des 2 sommets 0 et 3 */
		polygone[0].x = polygone[3].x = 0;
	    }
	    if( polygone[1].x > polygone[2].x )
	    {  /* croisement sur axe X des 2 sommets 1 et 2 */
		polygone[1].x = polygone[2].x = 0;
	    }
	    if( polygone[1].y > polygone[0].y )
	    {  /* croisement sur axe Y des 2 sommets 0 et 1 */
		polygone[0].y = polygone[1].y = 0;
	    }
	    if( polygone[2].y > polygone[3].y )
	    {  /* croisement sur axe Y des 2 sommets 2 et 3 */
		polygone[2].y = polygone[3].y = 0;
	    }

	    for(int ii = 0; ii < 4; ii++ )
	    {
		coord[ii].x = polygone[ii].x + pos.x;
		coord[ii].y = polygone[ii].y + pos.y;
		RotatePoint( &coord[ii], pos, orient );
	    }

	    move_to( coord[0] );
	    line_to( coord[1] );
	    line_to( coord[2] );
	    line_to( coord[3] );
	    finish_to( coord[0] );
	}
    }
}


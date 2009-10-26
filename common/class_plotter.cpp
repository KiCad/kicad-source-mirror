/******************************************
* class_plotter.cpp
* the class PLOTTER handle basic functions to plot schematic and boards
* with different plot formats.
* currently formats are:*
* HPGL
* POSTSCRIPT
* GERBER
* DXF
******************************************/

#include "fctsys.h"

//#include "gr_basic.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "common.h"
#include "plot_common.h"
#include "worksheet.h"
#include "macros.h"
#include "class_base_screen.h"
#include "drawtxt.h"

PLOTTER::PLOTTER( PlotFormat aPlotType )
{
    m_PlotType = aPlotType;
    plot_scale = 1;
    default_pen_width = 0;
    current_pen_width = -1;     /* To-be-set marker */
    pen_state = 'Z';            /* End-of-path idle */
    plot_orient_options = 0;    /* Mirror flag */
    output_file   = 0;
    color_mode    = false;      /* Start as a BW plot */
    negative_mode = false;
    sheet = NULL;
}


/********************************************************/
void PLOTTER::user_to_device_coordinates( wxPoint& pos )
/********************************************************/

/* modifie les coord pos.x et pos.y pour le trace selon l'orientation,
 * l'echelle, les offsets de trace */
{
    pos.x = (int) ( (pos.x - plot_offset.x) * plot_scale * device_scale );

    if( plot_orient_options == PLOT_MIROIR )
        pos.y = (int) ( (pos.y - plot_offset.y) * plot_scale * device_scale );
    else
        pos.y = (int) ( (paper_size.y - (pos.y - plot_offset.y) * plot_scale) * device_scale );
}


/********************************************************************/
void PLOTTER::arc( wxPoint centre, int StAngle, int EndAngle, int rayon,
                   FILL_T fill, int width )
/********************************************************************/
/* Generic arc rendered as a polyline */
{
    wxPoint   start, end;
    const int delta = 50;   /* increment (in 0.1 degrees) to draw circles */
    double    alpha;

    if( StAngle > EndAngle )
        EXCHG( StAngle, EndAngle );

    set_current_line_width( width );
    /* Please NOTE the different sign due to Y-axis flip */
    alpha   = StAngle / 1800.0 * M_PI;
    start.x = centre.x + (int) ( rayon * cos( -alpha ) );
    start.y = centre.y + (int) ( rayon * sin( -alpha ) );
    move_to( start );
    for( int ii = StAngle + delta; ii < EndAngle; ii += delta )
    {
        alpha = ii / 1800.0 * M_PI;
        end.x = centre.x + (int) ( rayon * cos( -alpha ) );
        end.y = centre.y + (int) ( rayon * sin( -alpha ) );
        line_to( end );
    }

    alpha = EndAngle / 1800.0 * M_PI;
    end.x = centre.x + (int) ( rayon * cos( -alpha ) );
    end.y = centre.y + (int) ( rayon * sin( -alpha ) );
    finish_to( end );
}


/************************************/
void PLOTTER::user_to_device_size( wxSize& size )
/************************************/
/* modifie les dimension size.x et size.y pour le trace selon l'echelle */
{
    size.x = (int) ( size.x * plot_scale * device_scale );
    size.y = (int) ( size.y * plot_scale * device_scale );
}


/************************************/
double PLOTTER::user_to_device_size( double size )
/************************************/
{
    return size * plot_scale * device_scale;
}


/************************************************************************************/
void PLOTTER::center_square( const wxPoint& position, int diametre, FILL_T fill )
/************************************************************************************/
{
    int rayon     = wxRound( diametre / 2.8284 );
    int coord[10] =
    {
        position.x + rayon, position.y + rayon,
        position.x + rayon, position.y - rayon,
        position.x - rayon, position.y - rayon,
        position.x - rayon, position.y + rayon,
        position.x + rayon, position.y + rayon
    };

    if( fill )
    {
        poly( 4, coord, fill );
    }
    else
    {
        poly( 5, coord, fill );
    }
}


/************************************************************************************/
void PLOTTER::center_lozenge( const wxPoint& position, int diametre, FILL_T fill )
/************************************************************************************/
{
    int rayon     = diametre / 2;
    int coord[10] =
    {
        position.x,         position.y + rayon,
        position.x + rayon, position.y,
        position.x,         position.y - rayon,
        position.x - rayon, position.y,
        position.x,         position.y + rayon,
    };

    if( fill )
    {
        poly( 4, coord, fill );
    }
    else
    {
        poly( 5, coord, fill );
    }
}


/************************************************************************************/
void PLOTTER::marker( const wxPoint& position, int diametre, int aShapeId )
/************************************************************************************/

/* Trace un motif de numero de forme aShapeId, aux coord x0, y0.
 *  x0, y0 = coordonnees tables
 *  diametre = diametre (coord table) du trou
 *  aShapeId = index ( permet de generer des formes caract )
 */
{
    int rayon = diametre / 2;

    int x0, y0;

    x0 = position.x; y0 = position.y;

    switch( aShapeId )
    {
    case 0:     /* vias : forme en X */
        move_to( wxPoint( x0 - rayon, y0 - rayon ) );
        line_to( wxPoint( x0 + rayon, y0 + rayon ) );
        move_to( wxPoint( x0 + rayon, y0 - rayon ) );
        finish_to( wxPoint( x0 - rayon, y0 + rayon ) );
        break;

    case 1:     /* Cercle */
        circle( position, diametre, NO_FILL );
        break;

    case 2:     /* forme en + */
        move_to( wxPoint( x0, y0 - rayon ) );
        line_to( wxPoint( x0, y0 + rayon ) );
        move_to( wxPoint( x0 + rayon, y0 ) );
        finish_to( wxPoint( x0 - rayon, y0 ) );
        break;

    case 3:     /* forme en X cercle */
        circle( position, diametre, NO_FILL );
        move_to( wxPoint( x0 - rayon, y0 - rayon ) );
        line_to( wxPoint( x0 + rayon, y0 + rayon ) );
        move_to( wxPoint( x0 + rayon, y0 - rayon ) );
        finish_to( wxPoint( x0 - rayon, y0 + rayon ) );
        break;

    case 4:     /* forme en cercle barre de - */
        circle( position, diametre, NO_FILL );
        move_to( wxPoint( x0 - rayon, y0 ) );
        finish_to( wxPoint( x0 + rayon, y0 ) );
        break;

    case 5:     /* forme en cercle barre de | */
        circle( position, diametre, NO_FILL );
        move_to( wxPoint( x0, y0 - rayon ) );
        finish_to( wxPoint( x0, y0 + rayon ) );
        break;

    case 6:     /* forme en carre */
        center_square( position, diametre, NO_FILL );
        break;

    case 7:     /* forme en losange */
        center_lozenge( position, diametre, NO_FILL );
        break;

    case 8:     /* forme en carre barre par un X*/
        center_square( position, diametre, NO_FILL );
        move_to( wxPoint( x0 - rayon, y0 - rayon ) );
        line_to( wxPoint( x0 + rayon, y0 + rayon ) );
        move_to( wxPoint( x0 + rayon, y0 - rayon ) );
        finish_to( wxPoint( x0 - rayon, y0 + rayon ) );
        break;

    case 9:     /* forme en losange barre par un +*/
        center_lozenge( position, diametre, NO_FILL );
        move_to( wxPoint( x0, y0 - rayon ) );
        line_to( wxPoint( x0, y0 + rayon ) );
        move_to( wxPoint( x0 + rayon, y0 ) );
        finish_to( wxPoint( x0 - rayon, y0 ) );
        break;

    case 10:     /* forme en carre barre par un '/' */
        center_square( position, diametre, NO_FILL );
        move_to( wxPoint( x0 - rayon, y0 - rayon ) );
        finish_to( wxPoint( x0 + rayon, y0 + rayon ) );
        break;

    case 11:     /* forme en losange barre par un |*/
        center_lozenge( position, diametre, NO_FILL );
        move_to( wxPoint( x0, y0 - rayon ) );
        finish_to( wxPoint( x0, y0 + rayon ) );
        break;

    case 12:     /* forme en losange barre par un -*/
        center_lozenge( position, diametre, NO_FILL );
        move_to( wxPoint( x0 - rayon, y0 ) );
        finish_to( wxPoint( x0 + rayon, y0 ) );
        break;

    default:
        circle( position, diametre, NO_FILL );
        break;
    }
}


/***************************************************************/
void PLOTTER::segment_as_oval( wxPoint start, wxPoint end, int width,
                               GRTraceMode tracemode )
/***************************************************************/
{
    /* Convert a thick segment and plot it as an oval */
    wxPoint center( (start.x + end.x) / 2, (start.y + end.y) / 2 );
    wxSize  size( end.x - start.x, end.y - start.y );
    int     orient;

    if( size.y == 0 )
        orient = 0;
    else if( size.x == 0 )
        orient = 900;
    else
        orient = -(int) ( atan2( (double) size.y, (double) size.x ) * 1800.0 / M_PI );
    size.x = (int) sqrt( ( (double) size.x * size.x ) + ( (double) size.y * size.y ) ) + width;
    size.y = width;

    flash_pad_oval( center, size, orient, tracemode );
}


/***************************************************************/
void PLOTTER::sketch_oval( wxPoint pos, wxSize size, int orient,
                           int width )
/***************************************************************/
{
    set_current_line_width( width );
    width = current_pen_width;
    int rayon, deltaxy, cx, cy;
    if( size.x > size.y )
    {
        EXCHG( size.x, size.y ); orient += 900;
        if( orient >= 3600 )
            orient -= 3600;
    }
    deltaxy = size.y - size.x; /* = distance entre centres de l'ovale */
    rayon   = (size.x - width) / 2;
    cx = -rayon; cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    move_to( wxPoint( cx + pos.x, cy + pos.y ) );
    cx = -rayon; cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    finish_to( wxPoint( cx + pos.x, cy + pos.y ) );

    cx = rayon; cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    move_to( wxPoint( cx + pos.x, cy + pos.y ) );
    cx = rayon; cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    finish_to( wxPoint( cx + pos.x, cy + pos.y ) );

    cx = 0; cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    arc( wxPoint( cx + pos.x, cy + pos.y ),
         orient + 1800, orient + 3600,
         rayon, NO_FILL );
    cx = 0; cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    arc( wxPoint( cx + pos.x, cy + pos.y ),
         orient, orient + 1800,
         rayon, NO_FILL );
}


/***************************************************************/
void PLOTTER::thick_segment( wxPoint start, wxPoint end, int width,
                             GRTraceMode tracemode )
/***************************************************************/

/* Plot 1 segment like a track segment
 */
{
    switch( tracemode )
    {
    case FILLED:
    case FILAIRE:
        set_current_line_width( tracemode==FILLED ? width : -1 );
        move_to( start );
        finish_to( end );
        break;

    case SKETCH:
        set_current_line_width( -1 );
        segment_as_oval( start, end, width, tracemode );
        break;
    }
}


void PLOTTER::thick_arc( wxPoint centre, int StAngle, int EndAngle, int rayon,
                         int width, GRTraceMode tracemode )
{
    switch( tracemode )
    {
    case FILAIRE:
        set_current_line_width( -1 );
        arc( centre, StAngle, EndAngle, rayon, NO_FILL, -1 );
        break;

    case FILLED:
        arc( centre, StAngle, EndAngle, rayon, NO_FILL, width );
        break;

    case SKETCH:
        set_current_line_width( -1 );
        arc( centre, StAngle, EndAngle, rayon - (width - current_pen_width) / 2, NO_FILL, -1 );
        arc( centre, StAngle, EndAngle, rayon + (width - current_pen_width) / 2, NO_FILL, -1 );
        break;
    }
}


void PLOTTER::thick_rect( wxPoint p1, wxPoint p2, int width,
                          GRTraceMode tracemode )
{
    switch( tracemode )
    {
    case FILAIRE:
        rect( p1, p2, NO_FILL, -1 );
        break;

    case FILLED:
        rect( p1, p2, NO_FILL, width );
        break;

    case SKETCH:
        set_current_line_width( -1 );
        p1.x -= (width - current_pen_width) / 2;
        p1.y -= (width - current_pen_width) / 2;
        p2.x += (width - current_pen_width) / 2;
        p2.y += (width - current_pen_width) / 2;
        rect( p1, p2, NO_FILL, -1 );
        p1.x += (width - current_pen_width);
        p1.y += (width - current_pen_width);
        p2.x -= (width - current_pen_width);
        p2.y -= (width - current_pen_width);
        rect( p1, p2, NO_FILL, -1 );
        break;
    }
}


void PLOTTER::thick_circle( wxPoint pos, int diametre, int width,
                            GRTraceMode tracemode )
{
    switch( tracemode )
    {
    case FILAIRE:
        circle( pos, diametre, NO_FILL, -1 );
        break;

    case FILLED:
        circle( pos, diametre, NO_FILL, width );
        break;

    case SKETCH:
        set_current_line_width( -1 );
        circle( pos, diametre - width + current_pen_width, NO_FILL, -1 );
        circle( pos, diametre + width - current_pen_width, NO_FILL, -1 );
        break;
    }
}


/*************************************************************************************/
void PLOTTER::set_paper_size( Ki_PageDescr* asheet )
/*************************************************************************************/
{
    wxASSERT( !output_file );
    sheet = asheet;

    // Sheets are in mils, plotter works with decimils
    paper_size.x = sheet->m_Size.x * 10;
    paper_size.y = sheet->m_Size.y * 10;
}

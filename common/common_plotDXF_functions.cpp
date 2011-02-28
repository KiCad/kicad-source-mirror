/***********************************/
/* Kicad: Common plot DXF Routines */
/***********************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "plot_common.h"
#include "macros.h"
#include "kicad_string.h"


/* Set the plot offset for the current plotting
 */
void DXF_PLOTTER::set_viewport( wxPoint aOffset, double aScale, bool aMirror )
{
    wxASSERT( !output_file );
    plot_offset  = aOffset;
    plot_scale   = aScale;
    device_scale = 1;
    set_default_line_width( 0 );    /* No line width on DXF */
    plotMirror = false;             /* No mirroring on DXF */
    current_color = BLACK;
}


bool DXF_PLOTTER::start_plot( FILE* fout )
{
    wxASSERT( !output_file );
    output_file = fout;
    /* DXF HEADER - Boilerplate */
    fputs( "0\nSECTION\n2\nHEADER\n9\n$ANGBASE\n50\n0.0\n9\n$ANGDIR\n70\n0\n0\nENDSEC\n0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLTYPE\n70\n1\n0\nLTYPE\n2\nCONTINUOUS\n70\n0\n3\nSolid line\n72\n65\n73\n0\n40\n0.0\n0\nENDTAB\n",
           output_file );
    /* Layer table - one layer per color */
    fprintf( output_file, "0\nTABLE\n2\nLAYER\n70\n%d\n", NBCOLOR );
    for( int i = 0; i<NBCOLOR; i++ )
    {
        wxString cname = ColorRefs[i].m_Name;
        fprintf( output_file, "0\nLAYER\n2\n%s\n70\n0\n62\n%d\n6\nCONTINUOUS\n",
                 TO_UTF8( cname ), i + 1 );
    }

    /* End of layer table, begin entities */
    fputs( "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n", output_file );

    return true;
}


bool DXF_PLOTTER::end_plot()
{
    wxASSERT( output_file );
    /* DXF FOOTER */
    fputs( "0\nENDSEC\n0\nEOF\n", output_file );
    fclose( output_file );
    output_file = NULL;

    return true;
}


/*
 * color = color index in ColorRefs[]
 */
void DXF_PLOTTER::set_color( int color )
{
    wxASSERT( output_file );
    if( ( color >= 0 && color_mode )
       || ( color == BLACK )
       || ( color == WHITE ) )
    {
        current_color = color;
    }
}


void DXF_PLOTTER::rect( wxPoint p1, wxPoint p2, FILL_T fill, int width )
{
    wxASSERT( output_file );
    move_to( p1 );
    line_to( wxPoint( p1.x, p2.y ) );
    line_to( wxPoint( p2.x, p2.y ) );
    line_to( wxPoint( p2.x, p1.y ) );
    finish_to( wxPoint( p1.x, p1.y ) );
}


void DXF_PLOTTER::circle( wxPoint centre, int diameter, FILL_T fill, int width )
{
    wxASSERT( output_file );
    double radius = user_to_device_size( diameter / 2 );
    user_to_device_coordinates( centre );
    if( radius > 0 )
    {
        wxString cname = ColorRefs[current_color].m_Name;
        if (!fill) {
          fprintf( output_file, "0\nCIRCLE\n8\n%s\n10\n%d.0\n20\n%d.0\n40\n%g\n",
                  TO_UTF8( cname ),
                  centre.x, centre.y, radius );
        }
        if (fill == FILLED_SHAPE) {
            int r = (int)(radius*0.5);
            fprintf( output_file, "0\nPOLYLINE\n");
            fprintf( output_file, "8\n%s\n66\n1\n70\n1\n", TO_UTF8( cname ));
            fprintf( output_file, "40\n%g\n41\n%g\n", radius,radius);
            fprintf( output_file, "0\nVERTEX\n8\n%s\n", TO_UTF8( cname ));
            fprintf( output_file, "10\n%d.0\n 20\n%d.0\n42\n1.0\n", centre.x-r,centre.y);
            fprintf( output_file, "0\nVERTEX\n8\n%s\n", TO_UTF8( cname ));
            fprintf( output_file, "10\n%d.0\n 20\n%d.0\n42\n1.0\n", centre.x+r,centre.y);
            fprintf( output_file, "0\nSEQEND\n");
    }
     }
}


/* Draw a polygon (closed if completed) in DXF format
 * coord = coord table tops
 * nb = number of coord (coord 1 = 2 elements: X and Y table)
 * fill: if != 0 filled polygon
 */
void DXF_PLOTTER::poly( int nb, int* coord, FILL_T fill, int width )
{
    wxASSERT( output_file );
    if( nb <= 1 )
        return;

    move_to( wxPoint( coord[0], coord[1] ) );
    for( int ii = 1; ii < nb; ii++ )
        line_to( wxPoint( coord[ii * 2], coord[(ii * 2) + 1] ) );

    /* Close polygon. */
    if( fill )
    {
        int ii = (nb - 1) * 2;
        if( ( coord[ii] != coord[0] ) || ( coord[ii + 1] != coord[1] ) )
            line_to( wxPoint( coord[0], coord[1] ) );
    }
    pen_finish();
}


/*
 * Move the pen up (pen = 'U') or down (feather = 'D') at position x, y
 * Unit to unit DRAWING
 * If pen = 'Z' without lifting pen displacement
 */
void DXF_PLOTTER::pen_to( wxPoint pos, char plume )
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
                 TO_UTF8( cname ),
                 pen_lastpos.x, pen_lastpos.y, pos.x, pos.y );
    }
    pen_lastpos = pos;
}


void DXF_PLOTTER::set_dash( bool dashed )
{
    /* NOP for now */
}


/**
 * Function thick_segment
 * Plot a filled segment (track)
 * @param aStart = starting point
 * @param aEnd = ending point
 * @param aWidth = segment width (thickness)
 * @param aPlotMode = FILLED, SKETCH ..
 */
void DXF_PLOTTER::thick_segment( wxPoint aStart, wxPoint aEnd, int aWidth,
                                 GRTraceMode aPlotMode )
{
    if( aPlotMode == FILAIRE )  /* just a line is Ok */
    {
        move_to( aStart );
        finish_to( aEnd );
    }
    else
        segment_as_oval( aStart, aEnd, aWidth, aPlotMode );
}


/* Plot an arc in DXF format.
 * center = center coord
 * StAngle, EndAngle = angle of beginning and end
 * Radius = radius of the arc
 */
void DXF_PLOTTER::arc( wxPoint centre, int StAngle, int EndAngle, int radius,
                       FILL_T fill, int width )
{
    wxASSERT( output_file );

    if( radius <= 0 )
        return;

    user_to_device_coordinates( centre );
    radius = wxRound( user_to_device_size( radius ) );

    /* DXF ARC */
    wxString cname = ColorRefs[current_color].m_Name;
    fprintf( output_file,
             "0\nARC\n8\n%s\n10\n%d.0\n20\n%d.0\n40\n%d.0\n50\n%d.0\n51\n%d.0\n",
             TO_UTF8( cname ),
             centre.x, centre.y, radius,
             StAngle / 10, EndAngle / 10 );
}


/* Plot oval pad at position. */
void DXF_PLOTTER::flash_pad_oval( wxPoint pos, wxSize size, int orient,
                                  GRTraceMode trace_mode )
{
    wxASSERT( output_file );

    /* The chip is reduced to an oval tablet with size.y > size.x
     * (Oval vertical orientation 0) */
    if( size.x > size.y )
    {
        EXCHG( size.x, size.y );
        orient += 900;
        if( orient >= 3600 )
            orient -= 3600;
    }
    sketch_oval( pos, size, orient, -1 );
}


/* Plot round pad or via. */
void DXF_PLOTTER::flash_pad_circle( wxPoint pos, int diametre,
                                    GRTraceMode trace_mode )
{
    wxASSERT( output_file );
    circle( pos, diametre, NO_FILL );
}


/*
 * Plot rectangular pad vertical or horizontal (rectangular Pad)
 */
void DXF_PLOTTER::flash_pad_rect( wxPoint pos, wxSize padsize,
                                  int orient, GRTraceMode trace_mode )
{
    wxASSERT( output_file );
    wxSize size;
    int    ox, oy, fx, fy;

    size.x = padsize.x / 2;  size.y = padsize.y / 2;

    if( size.x < 0 )
        size.x = 0;
    if( size.y < 0 )
        size.y = 0;

    /* If a dimension is zero, the trace is reduced to 1 line. */
    if( size.x == 0 )
    {
        ox = pos.x;
        oy = pos.y - size.y;
        RotatePoint( &ox, &oy, pos.x, pos.y, orient );
        fx = pos.x;
        fy = pos.y + size.y;
        RotatePoint( &fx, &fy, pos.x, pos.y, orient );
        move_to( wxPoint( ox, oy ) );
        finish_to( wxPoint( fx, fy ) );
        return;
    }
    if( size.y == 0 )
    {
        ox = pos.x - size.x;
        oy = pos.y;
        RotatePoint( &ox, &oy, pos.x, pos.y, orient );
        fx = pos.x + size.x;
        fy = pos.y;
        RotatePoint( &fx, &fy, pos.x, pos.y, orient );
        move_to( wxPoint( ox, oy ) );
        finish_to( wxPoint( fx, fy ) );
        return;
    }

    ox = pos.x - size.x;
    oy = pos.y - size.y;
    RotatePoint( &ox, &oy, pos.x, pos.y, orient );
    move_to( wxPoint( ox, oy ) );

    fx = pos.x - size.x;
    fy = pos.y + size.y;
    RotatePoint( &fx, &fy, pos.x, pos.y, orient );
    line_to( wxPoint( fx, fy ) );

    fx = pos.x + size.x;
    fy = pos.y + size.y;
    RotatePoint( &fx, &fy, pos.x, pos.y, orient );
    line_to( wxPoint( fx, fy ) );

    fx = pos.x + size.x;
    fy = pos.y - size.y;
    RotatePoint( &fx, &fy, pos.x, pos.y, orient );
    line_to( wxPoint( fx, fy ) );

    finish_to( wxPoint( ox, oy ) );
}


/*
 * Plot trapezoidal pad.
 * aPadPos is pad position, aCorners the corners position of the basic shape
 * Orientation aPadOrient in 0.1 degrees
 * Plot mode = FILLED, SKETCH (unused)
 */
void DXF_PLOTTER::flash_pad_trapez( wxPoint aPadPos, wxPoint aCorners[4],
                                     int aPadOrient, GRTraceMode aTrace_Mode )
{
    wxASSERT( output_file );
    wxPoint coord[4];       /* coord actual corners of a trapezoidal trace */

    for( int ii = 0; ii < 4; ii++ )
    {
        coord[ii] = aCorners[ii];
        RotatePoint( &coord[ii], aPadOrient );
        coord[ii] += aPadPos;
    }

    // Plot edge:
    move_to( coord[0] );
    line_to( coord[1] );
    line_to( coord[2] );
    line_to( coord[3] );
    finish_to( coord[0] );
}

/******************************************/
/* Kicad: Common plot Postscript Routines */
/******************************************/

#include "fctsys.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "common.h"
#include "plot_common.h"
#include "macros.h"
#include "kicad_string.h"


/* Set the plot offset for the current plotting */
void PS_PLOTTER::set_viewport( wxPoint offset, double aScale, int orient )
{
    wxASSERT( !output_file );
    plot_orient_options = orient;
    plot_offset  = offset;
    plot_scale   = aScale;
    device_scale = 1;               /* PS references in decimals */
    set_default_line_width( 100 );  /* default line width in 1/1000 inch */
}


/* Set the default line width (in 1/1000 inch) for the current plotting
 */
void PS_PLOTTER::set_default_line_width( int width )
{
    default_pen_width = width;   // line width in 1/1000 inch
    current_pen_width = -1;
}


/* Set the current line width (in 1/1000 inch) for the next plot
 */
void PS_PLOTTER::set_current_line_width( int width )
{
    wxASSERT( output_file );
    int pen_width;

    if( width >= 0 )
        pen_width = width;
    else
        pen_width = default_pen_width;

    if( pen_width != current_pen_width )
        fprintf( output_file, "%g setlinewidth\n",
                 user_to_device_size( pen_width ) );

    current_pen_width = pen_width;
}


/* Print the postscript set color command:
 * r g b setrgbcolor,
 * r, g, b  = color values (= 0 .. 1.0 )
 *
 * color = color index in ColorRefs[]
 */
void PS_PLOTTER::set_color( int color )
{
    wxASSERT( output_file );

    /* Return at invalid color index */
    if( color < 0 )
        return;

    if( color_mode )
    {
        if( negative_mode )
        {
            fprintf( output_file, "%.3g %.3g %.3g setrgbcolor\n",
                     (double) 1.0 - ColorRefs[color].m_Red / 255,
                     (double) 1.0 - ColorRefs[color].m_Green / 255,
                     (double) 1.0 - ColorRefs[color].m_Blue / 255 );
        }
        else
        {
            fprintf( output_file, "%.3g %.3g %.3g setrgbcolor\n",
                     (double) ColorRefs[color].m_Red / 255,
                     (double) ColorRefs[color].m_Green / 255,
                     (double) ColorRefs[color].m_Blue / 255 );
        }
    }
    else
    {
        /* B/W Mode - Use BLACK or WHITE for all items
         * note the 2 colors are used in B&W mode, mainly by Pcbnew to draw
         * holes in white on pads in black
         */
        int bwcolor = WHITE;
        if( color != WHITE )
            bwcolor = BLACK;
        if( negative_mode )
            fprintf( output_file, "%.3g %.3g %.3g setrgbcolor\n",
                     (double) 1.0 - ColorRefs[bwcolor].m_Red / 255,
                     (double) 1.0 - ColorRefs[bwcolor].m_Green / 255,
                     (double) 1.0 - ColorRefs[bwcolor].m_Blue / 255 );
        else
            fprintf( output_file, "%.3g %.3g %.3g setrgbcolor\n",
                     (double) ColorRefs[bwcolor].m_Red / 255,
                     (double) ColorRefs[bwcolor].m_Green / 255,
                     (double) ColorRefs[bwcolor].m_Blue / 255 );
    }
}


void PS_PLOTTER::set_dash( bool dashed )
{
    wxASSERT( output_file );
    if( dashed )
        fputs( "dashedline\n", stderr );
    else
        fputs( "solidline\n", stderr );
}


void PS_PLOTTER::rect( wxPoint p1, wxPoint p2, FILL_T fill, int width )
{
    user_to_device_coordinates( p1 );
    user_to_device_coordinates( p2 );

    set_current_line_width( width );
    fprintf( output_file, "%d %d %d %d rect%d\n", p1.x, p1.y,
             p2.x - p1.x, p2.y - p1.y, fill );
}


void PS_PLOTTER::circle( wxPoint pos, int diametre, FILL_T fill, int width )
{
    wxASSERT( output_file );
    user_to_device_coordinates( pos );
    double radius = user_to_device_size( diametre / 2.0 );

    if( radius < 1 )
        radius = 1;

    set_current_line_width( width );
    fprintf( output_file, "%d %d %g cir%d\n", pos.x, pos.y, radius, fill );
}


/* Plot an arc:
 * StAngle, EndAngle = start and end arc in 0.1 degree
 */
void PS_PLOTTER::arc( wxPoint centre, int StAngle, int EndAngle, int radius,
                      FILL_T fill, int width )
{
    wxASSERT( output_file );
    if( radius <= 0 )
        return;

    set_current_line_width( width );

    // Calculate start point.
    user_to_device_coordinates( centre );
    radius = wxRound( user_to_device_size( radius ) );
    if( plot_orient_options == PLOT_MIROIR )
        fprintf( output_file, "%d %d %d %g %g arc%d\n", centre.x, centre.y,
                 radius, (double) -EndAngle / 10, (double) -StAngle / 10,
                 fill );
    else
        fprintf( output_file, "%d %d %d %g %g arc%d\n", centre.x, centre.y,
                 radius, (double) StAngle / 10, (double) EndAngle / 10,
                 fill );
}


/** Function poly
 * @brief Draw a polygon ( a filled polygon if fill == 1 ) in POSTSCRIPT format
 * @param nb_segm = corner count
 * @param coord = corner list (a corner uses 2 int = X coordinate followed by Y
 *  coordinate
 * @param fill :if true : filled polygon
 * @param  width = line width
 */
void PS_PLOTTER::poly( int nb_segm, int* coord, FILL_T fill, int width )
{
    wxASSERT( output_file );
    wxPoint pos;

    if( nb_segm <= 1 )
        return;

    set_current_line_width( width );

    pos.x = coord[0];
    pos.y = coord[1];
    user_to_device_coordinates( pos );
    fprintf( output_file, "newpath\n%d %d moveto\n", pos.x, pos.y );

    for( int ii = 1; ii < nb_segm; ii++ )
    {
        pos.x = coord[2 * ii];
        pos.y = coord[2 * ii + 1];
        user_to_device_coordinates( pos );
        fprintf( output_file, "%d %d lineto\n", pos.x, pos.y );
    }

    // Close path
    fprintf( output_file, "poly%d\n", fill );
}


/* Routine to draw to a new position
 */
void PS_PLOTTER::pen_to( wxPoint pos, char plume )
{
    wxASSERT( output_file );
    if( plume == 'Z' )
    {
        if( pen_state != 'Z' )
        {
            fputs( "stroke\n", output_file );
            pen_state     = 'Z';
            pen_lastpos.x = -1;
            pen_lastpos.y = -1;
        }
        return;
    }

    user_to_device_coordinates( pos );
    if( pen_state == 'Z' )
    {
        fputs( "newpath\n", output_file );
    }
    if( pen_state != plume || pos != pen_lastpos )
        fprintf( output_file,
                 "%d %d %sto\n",
                 pos.x,
                 pos.y,
                 ( plume=='D' ) ? "line" : "move" );
    pen_state   = plume;
    pen_lastpos = pos;
}


/* The code within this function (and the CloseFilePS function)
 * creates postscript files whose contents comply with Adobe's
 * Document Structuring Convention, as documented by assorted
 * details described within the following URLs:
 *
 * http://en.wikipedia.org/wiki/Document_Structuring_Conventions
 * http://partners.adobe.com/public/developer/en/ps/5001.DSC_Spec.pdf
 *
 *
 * BBox is the boundary box (position and size of the "client rectangle"
 * for drawings (page - margins) in mils (0.001 inch)
 */
void PS_PLOTTER::start_plot( FILE* fout )
{
    wxASSERT( !output_file );
    wxString           msg;

    output_file = fout;
    static const char* PSMacro[] =
    {
        "/line {\n",
        "    newpath\n",
        "    moveto\n",
        "    lineto\n",
        "    stroke\n",
        "} bind def\n",
        "/cir0 { newpath 0 360 arc stroke } bind def\n",
        "/cir1 { newpath 0 360 arc gsave fill grestore stroke } bind def\n",
        "/cir2 { newpath 0 360 arc gsave fill grestore stroke } bind def\n",
        "/arc0 { newpath arc stroke } bind def\n",
        "/arc1 { newpath 4 index 4 index moveto arc closepath gsave fill ",
        "grestore stroke } bind def\n",
        "/arc2 { newpath 4 index 4 index moveto arc closepath gsave fill ",
        "grestore stroke } bind def\n",
        "/poly0 { stroke } bind def\n",
        "/poly1 { closepath gsave fill grestore stroke } bind def\n",
        "/poly2 { closepath gsave fill grestore stroke } bind def\n",
        "/rect0 { rectstroke } bind def\n",
        "/rect1 { rectfill } bind def\n",
        "/rect2 { rectfill } bind def\n",
        "/linemode0 { 0 setlinecap 0 setlinejoin 0 setlinewidth } bind def\n",
        "/linemode1 { 1 setlinecap 1 setlinejoin } bind def\n",
        "/dashedline { [50 50] 0 setdash } bind def\n",
        "/solidline { [] 0 setdash } bind def\n",
        "gsave\n",
        "0.0072 0.0072 scale\n",   // Configure postscript for decimals.
        "linemode1\n",
        NULL
    };

    const double       DECIMIL_TO_INCH = 0.0001;
    time_t             time1970 = time( NULL );

    fputs( "%!PS-Adobe-3.0\n", output_file );    // Print header

    fprintf( output_file, "%%%%Creator: %s\n", CONV_TO_UTF8( creator ) );

    // A "newline" character ("\n") is not included in the following string,
    // because it is provided by the ctime() function.
    fprintf( output_file, "%%%%CreationDate: %s", ctime( &time1970 ) );
    fprintf( output_file, "%%%%Title: %s\n", CONV_TO_UTF8( filename ) );
    fprintf( output_file, "%%%%Pages: 1\n" );
    fprintf( output_file, "%%%%PageOrder: Ascend\n" );

    // Print boundary box in 1/72 pixels per inch, box is in decimals
    const double CONV_SCALE = DECIMIL_TO_INCH * 72;

    // The coordinates of the lower left corner of the boundary
    // box need to be "rounded down", but the coordinates of its
    // upper right corner need to be "rounded up" instead.
    fprintf( output_file, "%%%%BoundingBox: 0 0 %d %d\n",
            (int) ceil( paper_size.y * CONV_SCALE ),
            (int) ceil( paper_size.x * CONV_SCALE ) );

    // Specify the size of the sheet and the name associated with that size.
    // (If the "User size" option has been selected for the sheet size,
    // identify the sheet size as "Custom" (rather than as "User"), but
    // otherwise use the name assigned by KiCad for each sheet size.)
    //
    // (The Document Structuring Convention also supports sheet weight,
    // sheet color, and sheet type properties being specified within a
    // %%DocumentMedia comment, but they are not being specified here;
    // a zero and two null strings are subsequently provided instead.)
    //
    // (NOTE: m_Size.y is *supposed* to be listed before m_Size.x;
    // the order in which they are specified is not wrong!)
    // Also note sheet->m_Size is given in mils, not in decimils and must be
    // sheet->m_Size * 10 in decimals
    if( sheet->m_Name.Cmp( wxT( "User" ) ) == 0 )
        fprintf( output_file, "%%%%DocumentMedia: Custom %d %d 0 () ()\n",
                 wxRound( sheet->m_Size.y * 10 * CONV_SCALE ),
                 wxRound( sheet->m_Size.x * 10 * CONV_SCALE ) );

    else  // ( if sheet->m_Name does not equal "User" )
        fprintf( output_file, "%%%%DocumentMedia: %s %d %d 0 () ()\n",
                 CONV_TO_UTF8( sheet->m_Name ),
                 wxRound( sheet->m_Size.y * 10 * CONV_SCALE ),
                 wxRound( sheet->m_Size.x * 10 * CONV_SCALE ) );

    fprintf( output_file, "%%%%Orientation: Landscape\n" );

    fprintf( output_file, "%%%%EndComments\n" );

    // Now specify various other details.

    // The following string has been specified here (rather than within
    // PSMacro[]) to highlight that it has been provided to ensure that the
    // contents of the postscript file comply with the details specified
    // within the Document Structuring Convention.
    fprintf( output_file, "%%%%Page: 1 1\n" );

    for( int ii = 0; PSMacro[ii] != NULL; ii++ )
    {
        fputs( PSMacro[ii], output_file );
    }

    // (If support for creating postscript files with a portrait orientation
    // is ever provided, determine whether it would be necessary to provide
    // an "else" command and then an appropriate "sprintf" command here.)
    fprintf( output_file, "%d 0 translate 90 rotate\n", paper_size.y );

    // Apply the scale adjustments
    if( plot_scale_adjX != 1.0 || plot_scale_adjY != 1.0 )
        fprintf( output_file, "%g %g scale\n",
                 plot_scale_adjX, plot_scale_adjY );

    // Set default line width ( g_Plot_DefaultPenWidth is in user units )
    fprintf( output_file, "%g setlinewidth\n",
             user_to_device_size( default_pen_width ) );
}


void PS_PLOTTER::end_plot()
{
    wxASSERT( output_file );
    fputs( "showpage\ngrestore\n%%EOF\n", output_file );
    fclose( output_file );
    output_file = 0;
}


/* Plot oval pad:
 * pos - Position of pad.
 * Dimensions dx, dy,
 * Orient Orient
 * The shape is drawn as a segment
 */
void PS_PLOTTER::flash_pad_oval( wxPoint pos, wxSize size, int orient,
                                 GRTraceMode modetrace )
{
    wxASSERT( output_file );
    int x0, y0, x1, y1, delta;

    // The pad is reduced to an oval by dy > dx
    if( size.x > size.y )
    {
        EXCHG( size.x, size.y );
        orient += 900;
        if( orient >= 3600 )
            orient -= 3600;
    }

    delta = size.y - size.x;
    x0    = 0;
    y0    = -delta / 2;
    x1    = 0;
    y1    = delta / 2;
    RotatePoint( &x0, &y0, orient );
    RotatePoint( &x1, &y1, orient );

    if( modetrace == FILLED )
        thick_segment( wxPoint( pos.x + x0, pos.y + y0 ),
                       wxPoint( pos.x + x1, pos.y + y1 ), size.x, modetrace );
    else
        sketch_oval( pos, size, orient, -1 );
}


/* Plot round pad or via.
 */
void PS_PLOTTER::flash_pad_circle( wxPoint pos, int diametre,
                                   GRTraceMode modetrace )
{
    wxASSERT( output_file );
    if( modetrace == FILLED )
    {
        set_current_line_width( 0 );
        circle( pos, diametre, FILLED_SHAPE );
    }
    else
    {
        set_current_line_width( -1 );
        int w = current_pen_width;
        circle( pos, diametre - 2 * w, NO_FILL );
    }
}


/* Plot rectangular pad in any orientation.
 */
void PS_PLOTTER::flash_pad_rect( wxPoint pos, wxSize size,
                                 int orient, GRTraceMode trace_mode )
{
    wxASSERT( output_file );

    set_current_line_width( -1 );
    int w = current_pen_width;
    size.x -= w;
    if( size.x < 1 )
        size.x = 1;
    size.y -= w;
    if( size.y < 1 )
        size.y = 1;

    int dx = size.x / 2;
    int dy = size.y / 2;

    int coord[10] =
    {
        pos.x - dx, pos.y + dy,
        pos.x - dx, pos.y - dy,
        pos.x + dx, pos.y - dy,
        pos.x + dx, pos.y + dy,
        0,          0
    };

    for( int ii = 0; ii < 4; ii++ )
    {
        RotatePoint( &coord[ii * 2], &coord[ii * 2 + 1], pos.x, pos.y, orient );
    }

    coord[8] = coord[0];
    coord[9] = coord[1];
    poly( 5, coord, ( trace_mode == FILLED ) ? FILLED_SHAPE : NO_FILL );
}


/* Plot trapezoidal pad.
 * Pos is pad center
 * Dimensions size.x and size.y
 * Changes delta.x and delta.y (1 of at least two must be zero)
 * Orientation east to 0.1 degrees
 * Plot mode (FILLED, SKETCH, WIRED)
 *
 * The evidence is that a trapezoid, ie that delta.x or delta.y = 0.
 *
 * The rating of the vertexes are (vis a vis the plotter)
 *
 * "       0 ------------- 3   "
 * "        .             .    "
 * "         .     O     .     "
 * "          .         .      "
 * "           1 ---- 2        "
 *
 *
 *   Example delta.y > 0, delta.x = 0
 * "           1 ---- 2        "
 * "          .         .      "
 * "         .     O     .     "
 * "        .             .    "
 * "       0 ------------- 3   "
 *
 *
 *   Example delta.y = 0, delta.x > 0
 * "       0                  "
 * "       . .                "
 * "       .     .            "
 * "       .           3      "
 * "       .           .      "
 * "       .     O     .      "
 * "       .           .      "
 * "       .           2      "
 * "       .     .            "
 * "       . .                "
 * "       1                  "
 */
void PS_PLOTTER::flash_pad_trapez( wxPoint centre, wxSize size, wxSize delta,
                                   int orient, GRTraceMode modetrace )
{
    wxASSERT( output_file );
    set_current_line_width( -1 );
    int w = current_pen_width;
    int dx, dy;
    int ddx, ddy;

    dx  = ( size.x - w ) / 2;
    dy  = ( size.y - w ) / 2;
    ddx = delta.x / 2;
    ddy = delta.y / 2;

    int coord[10] =
    {
        -dx - ddy, +dy + ddx,
        -dx + ddy, -dy - ddx,
        +dx - ddy, -dy + ddx,
        +dx + ddy, +dy - ddx,
        0,         0
    };

    for( int ii = 0; ii < 4; ii++ )
    {
        RotatePoint( &coord[ii * 2], &coord[ii * 2 + 1], orient );
        coord[ii * 2]     += centre.x;
        coord[ii * 2 + 1] += centre.y;
    }

    coord[8] = coord[0];
    coord[9] = coord[1];
    poly( 5, coord, ( modetrace == FILLED ) ? FILLED_SHAPE : NO_FILL );
}

/******************************************/
/* Kicad: Common plot GERBER Routines */
/******************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "common.h"
#include "plot_common.h"
#include "macros.h"
#include "kicad_string.h"

#include "build_version.h"


/**
 * Function set_viewport
 * Set the plot offset for the current plotting
 * @param aOffset = plot offset
 * @param aScale = coordinate scale (scale coefficient for coordinates)
 */
void GERBER_PLOTTER::set_viewport( wxPoint offset,
                                   double aScale, int orient )
{
    wxASSERT( !output_file );
    wxASSERT( orient == 0 );
    plot_orient_options = 0;
    plot_offset = offset;
    wxASSERT( aScale == 1 );
    plot_scale   = 1;
    device_scale = 1;
    set_default_line_width( 100 );  /* line thickness in 1 / 1000 inch */
}


/**
 * Function start_plot
 * Write GERBER header to file
 * initialize global variable g_Plot_PlotOutputFile
 * @param aFile: an opened file to write to
 */
bool GERBER_PLOTTER::start_plot( FILE* aFile )
{
    char Line[1024];

    wxASSERT( !output_file );
    final_file  = aFile;

    // Create a temporary filename to store gerber file
    // note tmpfile() does not work under Vista and W7 un user mode
    m_workFilename = filename + wxT(".tmp");
    work_file   = wxFopen( m_workFilename, wxT( "wt" ));
    output_file = work_file;
    wxASSERT( output_file );
    if( output_file == NULL )
        return false;

    DateAndTime( Line );
    wxString Title = creator + wxT( " " ) + GetBuildVersion();
    fprintf( output_file, "G04 (created by %s) date %s*\n",
             CONV_TO_UTF8( Title ), Line );

    // Specify linear interpol (G01), unit = INCH (G70), abs format (G90):
    fputs( "G01*\nG70*\nG90*\n", output_file );
    fputs( "%MOIN*%\n", output_file );     // set unites = INCHES

    /* Set gerber format to 3.4 */
    fputs( "G04 Gerber Fmt 3.4, Leading zero omitted, Abs format*\n%FSLAX34Y34*%\n",
           output_file );

    fputs( "G04 APERTURE LIST*\n", output_file );
    /* Select the default aperture */
    set_current_line_width( -1 );

    return true;
}


bool GERBER_PLOTTER::end_plot()
{
    char     line[1024];
    wxString msg;

    wxASSERT( output_file );
    /* Outfile is actually a temporary file! */
    fputs( "M02*\n", output_file );
    fflush( output_file );
//    rewind( work_file ); // work_file == output_file !!!
    fclose( work_file );
    work_file   = wxFopen( m_workFilename, wxT( "rt" ));
    wxASSERT( work_file );
    output_file = final_file;


    // Placement of apertures in RS274X
    while( fgets( line, 1024, work_file ) )
    {
        fputs( line, output_file );
        if( strcmp( strtok( line, "\n\r" ), "G04 APERTURE LIST*" ) == 0 )
        {
            write_aperture_list();
            fputs( "G04 APERTURE END LIST*\n", output_file );
        }
    }

    fclose( work_file );
    fclose( final_file );
    ::wxRemoveFile( m_workFilename );
    output_file = 0;

    return true;
}


/* Set the default line width (in 1/1000 inch) for the current plotting
 */
void GERBER_PLOTTER::set_default_line_width( int width )
{
    default_pen_width = width;
    current_aperture  = apertures.end();
}


/* Set the Current line width (in 1/1000 inch) for the next plot
 */
void GERBER_PLOTTER::set_current_line_width( int width )
{
    int pen_width;

    if( width > 0 )
        pen_width = width;
    else
        pen_width = default_pen_width;

    select_aperture( wxSize( pen_width, pen_width ), APERTURE::Plotting );
    current_pen_width = pen_width;
}


std::vector<APERTURE>::iterator GERBER_PLOTTER::get_aperture( const wxSize&           size,
                                                              APERTURE::Aperture_Type type )
{
    int last_D_code = 9;

    // Search an existing aperture
    std::vector<APERTURE>::iterator tool = apertures.begin();
    while( tool != apertures.end() )
    {
        last_D_code = tool->D_code;
        if( (tool->type == type)
           && (tool->size == size) )
            return tool;
        tool++;
    }

    // Allocate a new aperture
    APERTURE new_tool;
    new_tool.size   = size;
    new_tool.type   = type;
    new_tool.D_code = last_D_code + 1;
    apertures.push_back( new_tool );
    return apertures.end() - 1;
}


void GERBER_PLOTTER::select_aperture( const wxSize&           size,
                                      APERTURE::Aperture_Type type )
{
    wxASSERT( output_file );

    if( ( current_aperture == apertures.end() )
       || ( current_aperture->type != type )
       || ( current_aperture->size != size ) )
    {
        /* Pick an existing aperture or create a new one */
        current_aperture = get_aperture( size, type );
        fprintf( output_file, "G54D%d*\n", current_aperture->D_code );
    }
}


/*Generate list of D_CODES.
 * Returns the number of D_Codes generated in RS274X format.
 */
void GERBER_PLOTTER::write_aperture_list()
{
    wxASSERT( output_file );
    char cbuf[1024];

    /* Init : */
    for( std::vector<APERTURE>::iterator tool = apertures.begin();
         tool != apertures.end(); tool++ )
    {
        const float fscale = 0.0001f * plot_scale; // For 3.4 format
        char*       text;

        text = cbuf + sprintf( cbuf, "%%ADD%d", tool->D_code );

        switch( tool->type )
        {
        case APERTURE::Circle:
            sprintf( text, "C,%f*%%\n", tool->size.x * fscale );
            break;

        case APERTURE::Rect:
            sprintf( text, "R,%fX%f*%%\n", tool->size.x * fscale,
                     tool->size.y * fscale );
            break;

        case APERTURE::Plotting:
            sprintf( text, "C,%f*%%\n", tool->size.x * fscale );
            break;

        case APERTURE::Oval:
            sprintf( text, "O,%fX%f*%%\n", tool->size.x * fscale,
                     tool->size.y * fscale );
            break;
        }

        fputs( cbuf, output_file );
    }
}


void GERBER_PLOTTER::pen_to( wxPoint aPos, char plume )
{
    wxASSERT( output_file );
    user_to_device_coordinates( aPos );

    switch( plume )
    {
    case 'Z':
        break;

    case 'U':
        fprintf( output_file, "X%5.5dY%5.5dD02*\n", aPos.x, aPos.y );
        break;

    case 'D':
        fprintf( output_file, "X%5.5dY%5.5dD01*\n", aPos.x, aPos.y );
    }

    pen_state = plume;
}


void GERBER_PLOTTER::rect( wxPoint p1, wxPoint p2, FILL_T fill, int width )
{
    wxASSERT( output_file );
    int coord[10] =
    {
        p1.x, p1.y,
        p1.x, p2.y,
        p2.x, p2.y,
        p2.x, p1.y,
        p1.x, p1.y
    };
    poly( 5, coord, fill, width );
}


/**
 * Function circle
 * writes a non filled circle to output file
 * Plot one circle as segments (6 to 16 depending on its radius
 * @param aCentre = center coordinates
 * @param aDiameter = diameter of the circle
 * @param aWidth = line width
 */
void GERBER_PLOTTER::circle( wxPoint aCentre, int aDiameter, FILL_T fill,
                             int aWidth )
{
    wxASSERT( output_file );
    wxPoint   start, end;
    double    radius = aDiameter / 2;
    const int delta  = 3600 / 32; /* increment (in 0.1 degrees) to draw
                                   * circles */

    start.x = aCentre.x + wxRound( radius );
    start.y = aCentre.y;
    set_current_line_width( aWidth );
    move_to( start );
    for( int ii = delta; ii < 3600; ii += delta )
    {
        end.x = aCentre.x + (int) ( radius * fcosinus[ii] );
        end.y = aCentre.y + (int) ( radius * fsinus[ii] );
        line_to( end );
    }

    finish_to( start );
}


/**
 * Function PlotFilledPolygon_GERBER
 * writes a filled polyline to output file
 * @param aCornersCount = number of corners
 * @param aCoord = buffer of corners coordinates
 * @param aFill = plot option (NO_FILL, FILLED_SHAPE, FILLED_WITH_BG_BODYCOLOR)
 * @param aCoord = buffer of corners coordinates
 */
void GERBER_PLOTTER::poly( int aCornersCount, int* aCoord, FILL_T aFill,
                           int aWidth )
{
    wxASSERT( output_file );
    wxPoint pos, startpos;
    set_current_line_width( aWidth );

    if( aFill )
        fputs( "G36*\n", output_file );
    startpos.x = *aCoord++;
    startpos.y = *aCoord++;
    move_to( startpos );
    for( int ii = 1; ii < aCornersCount; ii++ )
    {
        pos.x = *aCoord++;
        pos.y = *aCoord++;
        line_to( pos );
    }

    if( aFill )
    {
        finish_to( startpos );
        fputs( "G37*\n", output_file );
    }
    else
    {
        pen_finish();
    }
}


/* Function flash_pad_circle
 * Plot a circular pad or via at the user position pos
 */
void GERBER_PLOTTER::flash_pad_circle( wxPoint pos, int diametre,
                                       GRTraceMode trace_mode )
{
    wxASSERT( output_file );
    wxSize size( diametre, diametre );

    switch( trace_mode )
    {
    case FILAIRE:
    case SKETCH:
        set_current_line_width( -1 );
        circle( pos, diametre - current_pen_width, NO_FILL );
        break;

    case FILLED:
        user_to_device_coordinates( pos );
        select_aperture( size, APERTURE::Circle );
        fprintf( output_file, "X%5.5dY%5.5dD03*\n", pos.x, pos.y );
        break;
    }
}


/* Plot oval pad at position pos:
 * Dimensions dx, dy,
 * Orient Orient
 * For a vertical or horizontal orientation, the shape is flashed
 * For any orientation the shape is drawn as a segment
 */
void GERBER_PLOTTER::flash_pad_oval( wxPoint pos, wxSize size, int orient,
                                     GRTraceMode trace_mode )
{
    wxASSERT( output_file );
    int x0, y0, x1, y1, delta;

    /* Plot a flashed shape. */
    if( ( orient == 0 || orient == 900 || orient == 1800 || orient == 2700 )
       && trace_mode == FILLED )
    {
        if( orient == 900 || orient == 2700 ) /* orientation turned 90 deg. */
            EXCHG( size.x, size.y );
        user_to_device_coordinates( pos );
        select_aperture( size, APERTURE::Oval );
        fprintf( output_file, "X%5.5dY%5.5dD03*\n", pos.x, pos.y );
    }
    else /* Plot pad as a segment. */
    {
        if( size.x > size.y )
        {
            EXCHG( size.x, size.y );
            if( orient < 2700 )
                orient += 900;
            else
                orient -= 2700;
        }
        if( trace_mode == FILLED )
        {
            /* The pad  is reduced to an oval with dy > dx */
            delta = size.y - size.x;
            x0    = 0;
            y0    = -delta / 2;
            x1    = 0;
            y1    = delta / 2;
            RotatePoint( &x0, &y0, orient );
            RotatePoint( &x1, &y1, orient );
            thick_segment( wxPoint( pos.x + x0, pos.y + y0 ),
                           wxPoint( pos.x + x1, pos.y + y1 ),
                           size.x, trace_mode );
        }
        else
            sketch_oval( pos, size, orient, -1 );
    }
}


/* Plot rectangular pad.
 * Gives its center, size, and orientation
 * For a vertical or horizontal shape, the shape is an aperture (Dcode) and
 * it is flashed.
 * For others shape the direction is plotted as a polygon.
 */
void GERBER_PLOTTER::flash_pad_rect( wxPoint pos, wxSize size,
                                     int orient, GRTraceMode trace_mode )

{
    wxASSERT( output_file );

    /* Plot as flashed. */
    switch( orient )
    {
    case 900:
    case 2700:        /* rotation of 90 degrees or 270 returns dimensions */
        EXCHG( size.x, size.y );

    // Pass through
    case 0:
    case 1800:
        switch( trace_mode )
        {
        case FILAIRE:
        case SKETCH:
            set_current_line_width( -1 );
            rect( wxPoint( pos.x - (size.x - current_pen_width) / 2,
                           pos.y - (size.y - current_pen_width) / 2 ),
                  wxPoint( pos.x + (size.x - current_pen_width) / 2,
                           pos.y + (size.y - current_pen_width) / 2 ),
                  NO_FILL );
            break;

        case FILLED:
            user_to_device_coordinates( pos );
            select_aperture( size, APERTURE::Rect );
            fprintf( output_file, "X%5.5dY%5.5dD03*\n", pos.x, pos.y );
            break;
        }

        break;

    default: /* plot pad shape as polygon */
    {
        wxPoint coord[4];
        // coord[0] is assumed the lower left
        // coord[1] is assumed the upper left
        // coord[2] is assumed the upper right
        // coord[3] is assumed the lower right

        /* Trace the outline. */
        coord[0].x = -size.x/2;   // lower left
        coord[0].y = size.y/2;
        coord[1].x = -size.x/2;   // upper left
        coord[1].y = -size.y/2;
        coord[2].x = size.x/2;    // upper right
        coord[2].y = -size.y/2;
        coord[3].x = size.x/2;    //lower right
        coord[3].y = size.y/2;

        flash_pad_trapez( pos, coord, orient, trace_mode );
    }
        break;
    }
}


/* Plot trapezoidal pad.
 * aPadPos is pad position, aCorners the corners positions of the basic shape
 * Orientation aPadOrient in 0.1 degrees
 * Plot mode  = FILLED or SKETCH
 */
 void GERBER_PLOTTER::flash_pad_trapez( wxPoint aPadPos,  wxPoint aCorners[4],
                                   int aPadOrient, GRTraceMode aTrace_Mode )

{
    wxPoint polygon[5];    // polygon corners list

     for( int ii = 0; ii < 4; ii++ )
        polygon[ii] = aCorners[ii];

   /* Draw the polygon and fill the interior as required. */
    for( int ii = 0; ii < 4; ii++ )
    {
        RotatePoint( &polygon[ii], aPadOrient );
        polygon[ii] += aPadPos;
    }
    // Close the polygon
    polygon[4] = polygon[0];

    set_current_line_width( -1 );
    poly( 5, &polygon[0].x, aTrace_Mode==FILLED ? FILLED_SHAPE : NO_FILL );
}

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


/***************************************************************************/
void Gerber_Plotter::set_viewport( wxPoint offset,
                                   double aScale, int orient )
/***************************************************************************/

/** function set_viewport
 * Set the plot offset for the current plotting
 * @param aOffset = plot offset
 * @param aScale = coordinate scale (scale coefficient for coordinates)
 */
{
    wxASSERT( !output_file );
    wxASSERT( orient == 0 );
    plot_orient_options = 0;
    plot_offset = offset;
    wxASSERT( aScale == 1 );
    plot_scale   = 1;
    device_scale = 1;
    set_default_line_width( 100 );            /* epaisseur du trait standard en 1/1000 pouce */
}


/******************************************************************/
void Gerber_Plotter::start_plot( FILE* aFile )
/*****************************************************************/

/** Function start_plot
 * Write GERBER header to file
 * initialize global variable g_Plot_PlotOutputFile
 * @param aFile: an opened file to write to
 */
{
    char Line[1024];

    wxASSERT( !output_file );
    final_file  = aFile;
    work_file   = tmpfile();
    output_file = work_file;
    DateAndTime( Line );
    wxString Title = creator + wxT( " " ) + GetBuildVersion();
    fprintf( output_file, "G04 (created by %s) date %s*\n", CONV_TO_UTF8( Title ), Line );

    // Specify linear interpol (G01), unit = INCH (G70), abs format (G90):
    fputs( "G01*\nG70*\nG90*\n", output_file );
    fputs( "%MOIN*%\n", output_file );     // set unites = INCHES

    /* Set gerber format to 3.4 */
    fputs( "G04 Gerber Fmt 3.4, Leading zero omitted, Abs format*\n%FSLAX34Y34*%\n",
           output_file );

    fputs( "G04 APERTURE LIST*\n", output_file );
    /* Select the default aperture */
    set_current_line_width( -1 );
}


/******************************************************************/
void Gerber_Plotter::end_plot()
/*****************************************************************/
{
    char     line[1024];
    wxString msg;

    wxASSERT( output_file );
    /* Outfile is actually a temporary file! */
    fputs( "M02*\n", output_file );
    fflush( output_file );
    rewind( work_file ); // work_file == output_file !!!
    output_file = final_file;


    // Placement des Apertures en RS274X
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
    output_file = 0;
}


/*************************************************************************************/
void Gerber_Plotter::set_default_line_width( int width )
/*************************************************************************************/

/* Set the default line width (in 1/1000 inch) for the current plotting
 */
{
    default_pen_width = width;      // epaisseur du trait standard en 1/1000 pouce
    current_aperture  = apertures.end();
}


/***************************************/
void Gerber_Plotter::set_current_line_width( int width )
/***************************************/

/* Set the Current line width (in 1/1000 inch) for the next plot
 */
{
    int pen_width;

    if( width > 0 )
        pen_width = width;
    else
        pen_width = default_pen_width;

    select_aperture( wxSize( pen_width, pen_width ), Aperture::Plotting );
    current_pen_width = pen_width;
}


/******************************************************/
vector<Aperture>::iterator Gerber_Plotter::get_aperture( const wxSize&           size,
                                                         Aperture::Aperture_Type type )
/******************************************************/
{
    int last_D_code = 9;

    // Search an existing aperture
    vector<Aperture>::iterator tool = apertures.begin();
    while( tool != apertures.end() )
    {
        last_D_code = tool->D_code;
        if( (tool->type == type)
           && (tool->size == size) )
            return tool;
        tool++;
    }

    // Allocate a new aperture
    Aperture new_tool;
    new_tool.size   = size;
    new_tool.type   = type;
    new_tool.D_code = last_D_code + 1;
    apertures.push_back( new_tool );
    return apertures.end() - 1;
}


/******************************************************/
void Gerber_Plotter::select_aperture( const wxSize& size, Aperture::Aperture_Type type )
/******************************************************/
{
    wxASSERT( output_file );
    if( ( current_aperture == apertures.end() )
       || (current_aperture->type != type)
       || (current_aperture->size != size) )
    {
        /* Pick an existing aperture or create a new one */
        current_aperture = get_aperture( size, type );
        fprintf( output_file, "G54D%d*\n", current_aperture->D_code );
    }
}


/******************************************************/
void Gerber_Plotter::write_aperture_list()
/******************************************************/

/* Genere la liste courante des D_CODES
 * Retourne le nombre de D_Codes utilises
 * Genere une sequence RS274X
 */
{
    wxASSERT( output_file );
    char cbuf[1024];

    /* Init : */
    for( vector<Aperture>::iterator tool = apertures.begin();
         tool != apertures.end(); tool++ )
    {
        const float fscale = 0.0001f * plot_scale; // For 3.4 format
        char*       text;

        text = cbuf + sprintf( cbuf, "%%ADD%d", tool->D_code );

        switch( tool->type )
        {
        case Aperture::Circle:
            sprintf( text, "C,%f*%%\n", tool->size.x * fscale );
            break;

        case Aperture::Rect:
            sprintf( text, "R,%fX%f*%%\n", tool->size.x * fscale,
                     tool->size.y * fscale );
            break;

        case Aperture::Plotting:
            sprintf( text, "C,%f*%%\n", tool->size.x * fscale );
            break;

        case Aperture::Oval:
            sprintf( text, "O,%fX%f*%%\n", tool->size.x * fscale,
                     tool->size.y * fscale );
            break;
        }

        fputs( cbuf, output_file );
    }
}


/**********************************************/
void Gerber_Plotter::pen_to( wxPoint aPos, char plume )
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


/**************************************************************************/
void Gerber_Plotter::rect( wxPoint p1, wxPoint p2, FILL_T fill, int width )
/**************************************************************************/
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


/*************************************************************************************/
void Gerber_Plotter::circle( wxPoint aCentre, int aDiameter, FILL_T fill, int aWidth )
/*************************************************************************************/

/** Function circle
 * writes a non filled circle to output file
 * Plot one circle as segments (6 to 16 depending on its radius
 * @param aCentre = centre coordintes
 * @param aDiameter = diameter of the circle
 * @param aWidth = line width
 */
{
    wxASSERT( output_file );
    wxPoint   start, end;
    double    radius = aDiameter / 2;
    const int delta  = 3600 / 32; /* increment (in 0.1 degrees) to draw circles */

    start.x = aCentre.x + radius;
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


/***************************************************************/
void Gerber_Plotter::poly( int aCornersCount, int* aCoord, FILL_T aFill, int aWidth )
/***************************************************************/

/** Function PlotFilledPolygon_GERBER
 * writes a filled polyline to output file
 * @param aCornersCount = numer of corners
 * @param aCoord = buffer of corners coordinates
 * @param aFill = plot option (NO_FILL, FILLED_SHAPE, FILLED_WITH_BG_BODYCOLOR)
 * @param aCoord = buffer of corners coordinates
 */
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
void Gerber_Plotter::flash_pad_circle( wxPoint pos, int diametre,
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
        select_aperture( size, Aperture::Circle );
        fprintf( output_file, "X%5.5dY%5.5dD03*\n", pos.x, pos.y );
        break;
    }
}


void Gerber_Plotter::flash_pad_oval( wxPoint pos, wxSize size, int orient,
                                     GRTraceMode trace_mode )

/* Trace 1 pastille PAD_OVAL en position pos_X,Y:
 *     dimensions dx, dy,
 *     orientation orient
 * Pour une orientation verticale ou horizontale, la forme est flashee
 * Pour une orientation quelconque la forme est tracee comme un segment
 */
{
    wxASSERT( output_file );
    int x0, y0, x1, y1, delta;

    /* Trace de la forme flashee */
    if( ( orient == 0 || orient == 900 || orient == 1800 || orient == 2700 )
       && trace_mode == FILLED )
    {
        if( orient == 900 || orient == 2700 ) /* orient tournee de 90 deg */
            EXCHG( size.x, size.y );
        user_to_device_coordinates( pos );
        select_aperture( size, Aperture::Oval );
        fprintf( output_file, "X%5.5dY%5.5dD03*\n", pos.x, pos.y );
    }
    else /* Forme tracee comme un segment */
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
            /* la pastille est ramenee a une pastille ovale avec dy > dx */
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


void Gerber_Plotter::flash_pad_rect( wxPoint pos, wxSize size,
                                     int orient, GRTraceMode trace_mode )

/* Plot 1 rectangular pad
 * donne par son centre, ses dimensions, et son orientation
 * For a vertical or horizontal shape, the shape is an aperture (Dcode) and it is flashed
 * For others orientations the shape is plotted as a polygon
 */
{
    wxASSERT( output_file );
    /* Trace de la forme flashee */
    switch( orient )
    {
    case 900:
    case 2700: /* la rotation de 90 ou 270 degres revient a permutter des dimensions */
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
            select_aperture( size, Aperture::Rect );
            fprintf( output_file, "X%5.5dY%5.5dD03*\n", pos.x, pos.y );
            break;
        }

        break;

    default: /* plot pad shape as polygon */
        flash_pad_trapez( pos, size, wxSize( 0, 0 ), orient, trace_mode );
        break;
    }
}


void Gerber_Plotter::flash_pad_trapez( wxPoint pos, wxSize size, wxSize delta,
                                       int orient, GRTraceMode trace_mode )

/* Trace 1 pad trapezoidal donne par :
 *    son centre pos.x,pos.y
 *    ses dimensions size.x et size.y
 *    les variations delta.x et delta.y ( 1 des deux au moins doit etre nulle)
 *    son orientation orient en 0.1 degres
 *    le mode de trace (FILLED, SKETCH, FILAIRE)
 *
 * Le trace n'est fait que pour un trapeze, c.a.d que delta.x ou delta.y
 *    = 0.
 *
 *   les notation des sommets sont ( vis a vis de la table tracante )
 *
 * "       0 ------------- 3   "
 * "        .             .    "
 * "         .     O     .     "
 * "          .         .      "
 * "           1 ---- 2        "
 *
 *
 *   exemple de Disposition pour delta.y > 0, delta.x = 0
 * "           1 ---- 2        "
 * "          .         .      "
 * "         .     O     .     "
 * "        .             .    "
 * "       0 ------------- 3   "
 *
 *
 *   exemple de Disposition pour delta.y = 0, delta.x > 0
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
{
    wxASSERT( output_file );
    int     ii, jj;
    int     dx, dy;
    wxPoint polygon[4]; /* polygon corners */
    int     coord[10];
    int     ddx, ddy;

    /* calcul des dimensions optimales du spot choisi = 1/4 plus petite dim */
    dx = size.x - abs( delta.y );
    dy = size.y - abs( delta.x );

    dx  = size.x / 2;
    dy  = size.y / 2;
    ddx = delta.x / 2;
    ddy = delta.y / 2;

    polygon[0].x = -dx - ddy;
    polygon[0].y = +dy + ddx;
    polygon[1].x = -dx + ddy;
    polygon[1].y = -dy - ddx;
    polygon[2].x = +dx - ddy;
    polygon[2].y = -dy + ddx;
    polygon[3].x = +dx + ddy;
    polygon[3].y = +dy - ddx;

    /* Dessin du polygone et Remplissage eventuel de l'interieur */

    for( ii = 0, jj = 0; ii < 4; ii++ )
    {
        RotatePoint( &polygon[ii].x, &polygon[ii].y, orient );
        coord[jj] = polygon[ii].x += pos.x;
        jj++;
        coord[jj] = polygon[ii].y += pos.y;
        jj++;
    }

    coord[8] = coord[0];
    coord[9] = coord[1];

    set_current_line_width( -1 );
    poly( 5, coord, trace_mode==FILLED ? FILLED_SHAPE : NO_FILL );
}

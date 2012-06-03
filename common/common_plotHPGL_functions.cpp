/**
 * @file common_plotHPGL_functions.cpp
 * @brief KiCad: Common plot HPGL Routines
 * Filled primitive are not supported, but some could be using HPGL/2
 * Since this plot engine is mostly intended for import in external programs,
 * sadly HPGL/2 isn't supported a lot... some of the primitives use overlapped
 * strokes to fill the shape
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <trigo.h>
#include <wxstruct.h>
#include <base_struct.h>
#include <plot_common.h>
#include <macros.h>
#include <kicad_string.h>

// HPGL scale factor (1 PLU = 1/40mm IIRC)
static const double PLUsPERDECIMIL = 0.102041;


void HPGL_PLOTTER::SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
				double aScale, bool aMirror )
{
    wxASSERT( !outputFile );
    plotOffset  = aOffset;
    plotScale = aScale;
    iuPerDeviceUnit = PLUsPERDECIMIL / aIusPerDecimil;
    /* Compute the paper size in IUs */
    paperSize = pageInfo.GetSizeMils();
    paperSize.x *= 10.0 * aIusPerDecimil;
    paperSize.y *= 10.0 * aIusPerDecimil;
    SetDefaultLineWidth( 0 ); // HPGL has pen sizes instead
    plotMirror = aMirror;
}

/**
 * At the start of the HPGL plot pen speed and number are requested
 */
bool HPGL_PLOTTER::StartPlot( FILE* fout )
{
    wxASSERT( !outputFile );
    outputFile = fout;
    fprintf( outputFile, "IN;VS%d;PU;PA;SP%d;\n", penSpeed, penNumber );
    return true;
}

/**
 * HPGL end of plot: pen return and release
 */
bool HPGL_PLOTTER::EndPlot()
{
    wxASSERT( outputFile );
    fputs( "PU;PA;SP0;\n", outputFile );
    fclose( outputFile );
    outputFile = NULL;
    return true;
}

/**
 * HPGL rectangle: fill not supported
 */
void HPGL_PLOTTER::Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill, int width )
{
    wxASSERT( outputFile );
    DPOINT p2dev = userToDeviceCoordinates( p2 );
    MoveTo( p1 );
    fprintf( outputFile, "EA %.0f,%.0f;\n", p2dev.x, p2dev.y );
    PenFinish();
}


/**
 * HPGL circle: fill not supported
 */
void HPGL_PLOTTER::Circle( const wxPoint& centre, int diameter, FILL_T fill,
                           int width )
{
    wxASSERT( outputFile );
    double radius = userToDeviceSize( diameter / 2 );

    if( radius > 0 )
    {
        MoveTo( centre );
        fprintf( outputFile, "CI %g;\n", radius );
        PenFinish();
    }
}


/** 
 * HPGL polygon: fill not supported (but closed, at least)
 */
void HPGL_PLOTTER::PlotPoly( const std::vector< wxPoint >& aCornerList, 
                             FILL_T aFill, int aWidth)
{
    if( aCornerList.size() <= 1 )
        return;

    MoveTo( aCornerList[0] );
    for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
        LineTo( aCornerList[ii] );

    // Close polygon if filled.
    if( aFill )
    {
        int ii = aCornerList.size() - 1;
        if( aCornerList[ii] != aCornerList[0] )
            LineTo( aCornerList[0] );
    }
    PenFinish();
}


/**
 * Pen control logic (remove redundant pen activations)
 */
void HPGL_PLOTTER::penControl( char plume )
{
    wxASSERT( outputFile );
    switch( plume )
    {
    case 'U':
        if( penState != 'U' )
        {
            fputs( "PU;", outputFile );
            penState = 'U';
        }
        break;

    case 'D':
        if( penState != 'D' )
        {
            fputs( "PD;", outputFile );
            penState = 'D';
        }
        break;

    case 'Z':
        fputs( "PU;", outputFile );
        penState = 'U';
        penLastpos.x = -1;
        penLastpos.y = -1;
        break;
    }
}


void HPGL_PLOTTER::PenTo( const wxPoint& pos, char plume )
{
    wxASSERT( outputFile );

    if( plume == 'Z' )
    {
        penControl( 'Z' );
        return;
    }

    penControl( plume );
    DPOINT pos_dev = userToDeviceCoordinates( pos );

    if( penLastpos != pos )
        fprintf( outputFile, "PA %.0f,%.0fd;\n", pos_dev.x, pos_dev.y );

    penLastpos = pos;
}


/**
 * HPGL supports dashed lines
 */
void HPGL_PLOTTER::SetDash( bool dashed )
{
    wxASSERT( outputFile );
    if( dashed )
        fputs( "LI 2;\n", stderr );
    else
        fputs( "LI;\n", stderr );
}


void HPGL_PLOTTER::ThickSegment( const wxPoint& start, const wxPoint& end, 
                                 int width, EDA_DRAW_MODE_T tracemode )
{
    wxASSERT( outputFile );
    wxPoint center;
    wxSize  size;

    // Suppress overlap if pen is too big or in line mode
    if( (penDiameter >= width) || (tracemode == LINE) ) 
    {
        MoveTo( start );
        FinishTo( end );
    }
    else
        segmentAsOval( start, end, width, tracemode );
}


/* Plot an arc:
 * Center = center coord
 * Stangl, endAngle = angle of beginning and end
 * Radius = radius of the arc
 * Command
 * PU PY x, y; PD start_arc_X AA, start_arc_Y, angle, NbSegm; PU;
 * Or PU PY x, y; PD start_arc_X AA, start_arc_Y, angle, PU;
 */
void HPGL_PLOTTER::Arc( const wxPoint& centre, int StAngle, int EndAngle, int radius,
                        FILL_T fill, int width )
{
    wxASSERT( outputFile );
    double angle;

    if( radius <= 0 )
        return;

    DPOINT centre_dev = userToDeviceCoordinates( centre );

    if( plotMirror )
        angle = (StAngle - EndAngle) / 10.0;
    else
        angle = (EndAngle - StAngle) / 10.0;

    // Calculate start point,
    wxPoint cmap;
    cmap.x = (int) ( centre.x + ( radius * cos( RAD2DEG( StAngle / 10.0 ) ) ) );
    cmap.y = (int) ( centre.y - ( radius * sin( RAD2DEG( StAngle / 10.0 ) ) ) );
    DPOINT cmap_dev = userToDeviceCoordinates( cmap );

    fprintf( outputFile,
             "PU;PA %.0f,%.0f;PD;AA %.0f,%.0f,",
             cmap_dev.x,
             cmap_dev.y,
             centre_dev.x,
             centre_dev.y );
    fprintf( outputFile, "%.0f", angle );
    fprintf( outputFile, ";PU;\n" );
    PenFinish();
}


/* Plot oval pad.
 */
void HPGL_PLOTTER::FlashPadOval( const wxPoint& pos, const wxSize& aSize, int orient,
                                 EDA_DRAW_MODE_T trace_mode )
{
    wxASSERT( outputFile );
    int deltaxy, cx, cy;
    wxSize size( aSize );

    /* The pad is reduced to an oval with size.y > size.x
     * (Oval vertical orientation 0)
     */
    if( size.x > size.y )
    {
        EXCHG( size.x, size.y ); orient += 900;
        if( orient >= 3600 )
            orient -= 3600;
    }
    deltaxy = size.y - size.x;     // distance between centers of the oval

    if( trace_mode == FILLED )
    {
        FlashPadRect( pos, wxSize( size.x, deltaxy + KiROUND( penDiameter ) ),
                      orient, trace_mode );
        cx = 0; cy = deltaxy / 2;
        RotatePoint( &cx, &cy, orient );
        FlashPadCircle( wxPoint( cx + pos.x, cy + pos.y ), size.x, trace_mode );
        cx = 0; cy = -deltaxy / 2;
        RotatePoint( &cx, &cy, orient );
        FlashPadCircle( wxPoint( cx + pos.x, cy + pos.y ), size.x, trace_mode );
    }
    else    // Plot in SKETCH mode.
    {
        sketchOval( pos, size, orient, KiROUND( penDiameter ) );
    }
}


/* Plot round pad or via.
 */
void HPGL_PLOTTER::FlashPadCircle( const wxPoint& pos, int diametre,
                                   EDA_DRAW_MODE_T trace_mode )
{
    wxASSERT( outputFile );
    DPOINT pos_dev = userToDeviceCoordinates( pos );

    int delta = KiROUND( penDiameter - penOverlap );
    int radius = diametre / 2;

    if( trace_mode != LINE )
    {
        radius = ( diametre - KiROUND( penDiameter ) ) / 2;
    }

    if( radius < 0 )
    {
        radius = 0;
    }

    double rsize = userToDeviceSize( radius );

    fprintf( outputFile, "PA %.0f,%.0fd;CI %.0f;\n", 
             pos_dev.x, pos_dev.y, rsize );

    if( trace_mode == FILLED )        // Plot in filled mode.
    {
        if( delta > 0 )
        {
            while( (radius -= delta ) >= 0 )
            {
                rsize = userToDeviceSize( radius );
                fprintf( outputFile, "PA %.0f,%.0f;CI %.0f;\n",
                         pos_dev.x, pos_dev.y, rsize );
            }
        }
    }
    PenFinish();
}


void HPGL_PLOTTER::FlashPadRect( const wxPoint& pos, const wxSize& padsize,
                                 int orient, EDA_DRAW_MODE_T trace_mode )
{
    wxASSERT( outputFile );
    wxSize size;
    int    delta;
    int    ox, oy, fx, fy;

    size.x = padsize.x / 2;
    size.y = padsize.y / 2;

    if( trace_mode != LINE )
    {
        size.x = (padsize.x - (int) penDiameter) / 2;
        size.y = (padsize.y - (int) penDiameter) / 2;
    }

    if( size.x < 0 )
        size.x = 0;
    if( size.y < 0 )
        size.y = 0;

    // If a dimension is zero, the trace is reduced to 1 line.
    if( size.x == 0 )
    {
        ox = pos.x;
        oy = pos.y - size.y;
        RotatePoint( &ox, &oy, pos.x, pos.y, orient );
        fx = pos.x;
        fy = pos.y + size.y;
        RotatePoint( &fx, &fy, pos.x, pos.y, orient );
        MoveTo( wxPoint( ox, oy ) );
        FinishTo( wxPoint( fx, fy ) );
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
        MoveTo( wxPoint( ox, oy ) );
        FinishTo( wxPoint( fx, fy ) );
        return;
    }

    ox = pos.x - size.x;
    oy = pos.y - size.y;
    RotatePoint( &ox, &oy, pos.x, pos.y, orient );
    MoveTo( wxPoint( ox, oy ) );

    fx = pos.x - size.x;
    fy = pos.y + size.y;
    RotatePoint( &fx, &fy, pos.x, pos.y, orient );
    LineTo( wxPoint( fx, fy ) );

    fx = pos.x + size.x;
    fy = pos.y + size.y;
    RotatePoint( &fx, &fy, pos.x, pos.y, orient );
    LineTo( wxPoint( fx, fy ) );

    fx = pos.x + size.x;
    fy = pos.y - size.y;
    RotatePoint( &fx, &fy, pos.x, pos.y, orient );
    LineTo( wxPoint( fx, fy ) );

    FinishTo( wxPoint( ox, oy ) );

    if( trace_mode == FILLED )
    {
        // Plot in filled mode.
        delta = (int) (penDiameter - penOverlap);

        if( delta > 0 )
            while( (size.x > 0) && (size.y > 0) )
            {
                size.x -= delta;
                size.y -= delta;

                if( size.x < 0 )
                    size.x = 0;
                if( size.y < 0 )
                    size.y = 0;

                ox = pos.x - size.x;
                oy = pos.y - size.y;
                RotatePoint( &ox, &oy, pos.x, pos.y, orient );
                MoveTo( wxPoint( ox, oy ) );

                fx = pos.x - size.x;
                fy = pos.y + size.y;
                RotatePoint( &fx, &fy, pos.x, pos.y, orient );
                LineTo( wxPoint( fx, fy ) );

                fx = pos.x + size.x;
                fy = pos.y + size.y;
                RotatePoint( &fx, &fy, pos.x, pos.y, orient );
                LineTo( wxPoint( fx, fy ) );

                fx = pos.x + size.x;
                fy = pos.y - size.y;
                RotatePoint( &fx, &fy, pos.x, pos.y, orient );
                LineTo( wxPoint( fx, fy ) );

                FinishTo( wxPoint( ox, oy ) );
            }

    }
}


void HPGL_PLOTTER::FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                   int aPadOrient, EDA_DRAW_MODE_T aTrace_Mode )
{
    wxASSERT( outputFile );
    wxPoint polygone[4];        // coordinates of corners relatives to the pad
    wxPoint coord[4];           // absolute coordinates of corners (coordinates in plotter space)
    int     move;

    move = KiROUND( penDiameter );

    for( int ii = 0; ii < 4; ii++ )
        polygone[ii] = aCorners[ii];

    // polygone[0] is assumed the lower left
    // polygone[1] is assumed the upper left
    // polygone[2] is assumed the upper right
    // polygone[3] is assumed the lower right

    // Plot the outline:
    for( int ii = 0; ii < 4; ii++ )
    {
        coord[ii] = polygone[ii];
        RotatePoint( &coord[ii], aPadOrient );
        coord[ii] += aPadPos;
    }
    MoveTo( coord[0] );
    LineTo( coord[1] );
    LineTo( coord[2] );
    LineTo( coord[3] );
    FinishTo( coord[0] );

    // Fill shape:
    if( aTrace_Mode == FILLED )
    {
        // TODO: replace this par the HPGL plot polygon.
        int jj;
        // Fill the shape
        move = KiROUND( penDiameter - penOverlap );
        // Calculate fill height.

        if( polygone[0].y == polygone[3].y ) // Horizontal
        {
            jj = polygone[3].y - (int) ( penDiameter + ( 2 * penOverlap ) );
        }
        else    // vertical
        {
            jj = polygone[3].x - (int) ( penDiameter + ( 2 * penOverlap ) );
        }

        // Calculation of dd = number of segments was traced to fill.
        jj = jj / (int) ( penDiameter - penOverlap );

        // Trace the outline.
        for( ; jj > 0; jj-- )
        {
            polygone[0].x += move;
            polygone[0].y -= move;
            polygone[1].x += move;
            polygone[1].y += move;
            polygone[2].x -= move;
            polygone[2].y += move;
            polygone[3].x -= move;
            polygone[3].y -= move;

            // Test for crossed vertexes.
            if( polygone[0].x > polygone[3].x ) /* X axis intersection on
                                                 *vertexes 0 and 3 */
            {
                polygone[0].x = polygone[3].x = 0;
            }
            if( polygone[1].x > polygone[2].x ) /*  X axis intersection on
                                                 *vertexes 1 and 2 */
            {
                polygone[1].x = polygone[2].x = 0;
            }
            if( polygone[1].y > polygone[0].y ) /* Y axis intersection on
                                                 *vertexes 0 and 1 */
            {
                polygone[0].y = polygone[1].y = 0;
            }
            if( polygone[2].y > polygone[3].y ) /* Y axis intersection on
                                                 *vertexes 2 and 3 */
            {
                polygone[2].y = polygone[3].y = 0;
            }

            for( int ii = 0; ii < 4; ii++ )
            {
                coord[ii] = polygone[ii];
                RotatePoint( &coord[ii], aPadOrient );
                coord[ii] += aPadPos;
            }

            MoveTo( coord[0] );
            LineTo( coord[1] );
            LineTo( coord[2] );
            LineTo( coord[3] );
            FinishTo( coord[0] );
        }
    }
}


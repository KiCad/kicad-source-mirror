/**
 * @file common_plotGERBER_functions.cpp
 * @brief Common GERBER plot routines.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <trigo.h>
#include <wxstruct.h>
#include <base_struct.h>
#include <common.h>
#include <plot_common.h>
#include <macros.h>
#include <kicad_string.h>

#include <build_version.h>


void GERBER_PLOTTER::SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
				  double aScale, bool aMirror )
{
    wxASSERT( !outputFile );
    wxASSERT( aMirror == false );
    m_plotMirror = false;
    plotOffset = aOffset;
    wxASSERT( aScale == 1 );
    plotScale = 1;
    m_IUsPerDecimil = aIusPerDecimil;
    iuPerDeviceUnit = 1.0 / aIusPerDecimil;
    /* We don't handle the filmbox, and it's more useful to keep the
     * origin at the origin */
    paperSize.x = 0;
    paperSize.y = 0;
    SetDefaultLineWidth( 100 * aIusPerDecimil ); // Arbitrary default
}

/**
 * Emit a D-Code record, using proper conversions
 * to format a leading zero omitted gerber coordinate
 * (for 4 decimal positions, see header generation in start_plot
 */
void GERBER_PLOTTER::emitDcode( const DPOINT& pt, int dcode )
{

    fprintf( outputFile, "X%dY%dD%02d*\n",
	    int( pt.x ), int( pt.y ), dcode );
}

/**
 * Function start_plot
 * Write GERBER header to file
 * initialize global variable g_Plot_PlotOutputFile
 */
bool GERBER_PLOTTER::StartPlot()
{
    wxASSERT( outputFile );

    finalFile = outputFile;     // the actual gerber file will be created later

    // Create a temporary filename to store gerber file
    // note tmpfile() does not work under Vista and W7 in user mode
    m_workFilename = filename + wxT(".tmp");
    workFile   = wxFopen( m_workFilename, wxT( "wt" ));
    outputFile = workFile;
    wxASSERT( outputFile );

    if( outputFile == NULL )
        return false;

    /* Set coordinate format to 3.4 absolute, leading zero omitted */
    fputs( "%FSLAX34Y34*%\n", outputFile );
    fputs( "G04 Gerber Fmt 3.4, Leading zero omitted, Abs format*\n", outputFile );

    wxString Title = creator + wxT( " " ) + GetBuildVersion();
    fprintf( outputFile, "G04 (created by %s) date %s*\n",
             TO_UTF8( Title ), TO_UTF8( DateAndTime() ) );

    /* Mass parameter: unit = INCHES */
    fputs( "%MOIN*%\n", outputFile );

    /* Specify linear interpol (G01), unit = INCH (G70), abs format (G90) */
    fputs( "G01*\nG70*\nG90*\n", outputFile );
    fputs( "G04 APERTURE LIST*\n", outputFile );
    /* Select the default aperture */
    SetCurrentLineWidth( -1 );

    return true;
}


bool GERBER_PLOTTER::EndPlot()
{
    char     line[1024];
    wxString msg;

    wxASSERT( outputFile );

    /* Outfile is actually a temporary file i.e. workFile */
    fputs( "M02*\n", outputFile );
    fflush( outputFile );

    fclose( workFile );
    workFile   = wxFopen( m_workFilename, wxT( "rt" ));
    wxASSERT( workFile );
    outputFile = finalFile;

    // Placement of apertures in RS274X
    while( fgets( line, 1024, workFile ) )
    {
        fputs( line, outputFile );

        if( strcmp( strtok( line, "\n\r" ), "G04 APERTURE LIST*" ) == 0 )
        {
            writeApertureList();
            fputs( "G04 APERTURE END LIST*\n", outputFile );
        }
    }

    fclose( workFile );
    fclose( finalFile );
    ::wxRemoveFile( m_workFilename );
    outputFile = 0;

    return true;
}


void GERBER_PLOTTER::SetDefaultLineWidth( int width )
{
    defaultPenWidth = width;
    currentAperture = apertures.end();
}


void GERBER_PLOTTER::SetCurrentLineWidth( int width )
{
    int pen_width;

    if( width > 0 )
        pen_width = width;
    else
        pen_width = defaultPenWidth;

    selectAperture( wxSize( pen_width, pen_width ), APERTURE::Plotting );
    currentPenWidth = pen_width;
}


std::vector<APERTURE>::iterator GERBER_PLOTTER::getAperture( const wxSize&           size,
                                                             APERTURE::APERTURE_TYPE type )
{
    int last_D_code = 9;

    // Search an existing aperture
    std::vector<APERTURE>::iterator tool = apertures.begin();

    while( tool != apertures.end() )
    {
        last_D_code = tool->DCode;

        if( (tool->Type == type) && (tool->Size == size) )
            return tool;

        tool++;
    }

    // Allocate a new aperture
    APERTURE new_tool;
    new_tool.Size  = size;
    new_tool.Type  = type;
    new_tool.DCode = last_D_code + 1;
    apertures.push_back( new_tool );
    return apertures.end() - 1;
}


void GERBER_PLOTTER::selectAperture( const wxSize&           size,
                                     APERTURE::APERTURE_TYPE type )
{
    wxASSERT( outputFile );

    if( ( currentAperture == apertures.end() )
       || ( currentAperture->Type != type )
       || ( currentAperture->Size != size ) )
    {
        // Pick an existing aperture or create a new one
        currentAperture = getAperture( size, type );
        fprintf( outputFile, "G54D%d*\n", currentAperture->DCode );
    }
}


/**
 * Generate the table of D codes
 */
void GERBER_PLOTTER::writeApertureList()
{
    wxASSERT( outputFile );
    char cbuf[1024];

    // Init
    for( std::vector<APERTURE>::iterator tool = apertures.begin();
         tool != apertures.end(); tool++ )
    {
        const double fscale = 0.0001f * plotScale
				* iuPerDeviceUnit ;
        char* text = cbuf + sprintf( cbuf, "%%ADD%d", tool->DCode );

        /* Please note: the Gerber specs for mass parameters say that
           exponential syntax is *not* allowed and the decimal point should
           also be always inserted. So the %g format is ruled out, but %f is fine
           (the # modifier forces the decimal point). Sadly the %f formatter
           can't remove trailing zeros but thats not a problem, since nothing
           forbid it (the file is only slightly longer) */

        switch( tool->Type )
        {
        case APERTURE::Circle:
            sprintf( text, "C,%#f*%%\n", tool->Size.x * fscale );
            break;

        case APERTURE::Rect:
            sprintf( text, "R,%#fX%#f*%%\n",
	             tool->Size.x * fscale,
                     tool->Size.y * fscale );
            break;

        case APERTURE::Plotting:
            sprintf( text, "C,%#f*%%\n", tool->Size.x * fscale );
            break;

        case APERTURE::Oval:
            sprintf( text, "O,%#fX%#f*%%\n",
	            tool->Size.x * fscale,
		    tool->Size.y * fscale );
            break;
        }

        fputs( cbuf, outputFile );
    }
}


void GERBER_PLOTTER::PenTo( const wxPoint& aPos, char plume )
{
    wxASSERT( outputFile );
    DPOINT pos_dev = userToDeviceCoordinates( aPos );

    switch( plume )
    {
    case 'Z':
        break;

    case 'U':
	emitDcode( pos_dev, 2 );
        break;

    case 'D':
	emitDcode( pos_dev, 1 );
    }

    penState = plume;
}


void GERBER_PLOTTER::Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill,
                           int width )
{
    std::vector< wxPoint > cornerList;

    // Build corners list
    cornerList.push_back( p1 );
    wxPoint corner(p1.x, p2.y);
    cornerList.push_back( corner );
    cornerList.push_back( p2 );
    corner.x = p2.x;
    corner.y = p1.y;
    cornerList.push_back( corner );
    cornerList.push_back( p1 );

    PlotPoly( cornerList, fill, width );
}


void GERBER_PLOTTER::Circle( const wxPoint& aCenter, int aDiameter, FILL_T aFill,
                             int aWidth )
{
    Arc( aCenter, 0, 3600, aDiameter / 2, aFill, aWidth );
}


void GERBER_PLOTTER::Arc( const wxPoint& aCenter, double aStAngle, double aEndAngle,
                          int aRadius, FILL_T aFill, int aWidth )
{
    wxASSERT( outputFile );
    wxPoint start, end;
    start.x = aCenter.x + KiROUND( cosdecideg( aRadius, aStAngle ) );
    start.y = aCenter.y - KiROUND( sindecideg( aRadius, aStAngle ) );
    SetCurrentLineWidth( aWidth );
    MoveTo( start );
    end.x = aCenter.x + KiROUND( cosdecideg( aRadius, aEndAngle ) );
    end.y = aCenter.y - KiROUND( sindecideg( aRadius, aEndAngle ) );
    DPOINT devEnd = userToDeviceCoordinates( end );
    DPOINT devCenter = userToDeviceCoordinates( aCenter )
        - userToDeviceCoordinates( start );
    fprintf( outputFile, "G75*\n" ); // Multiquadrant mode

    if( aStAngle < aEndAngle )
        fprintf( outputFile, "G03" );
    else
        fprintf( outputFile, "G02" );
    fprintf( outputFile, "X%dY%dI%dJ%dD01*\n", int( devEnd.x ), int( devEnd.y ),
             int( devCenter.x ), int( devCenter.y ) );
    fprintf( outputFile, "G74*\nG01*\n" ); // Back to single quadrant and linear interp.
}


/**
 * Gerber polygon: they can (and *should*) be filled with the
 * appropriate G36/G37 sequence (raster fills are deprecated)
 */
void GERBER_PLOTTER::PlotPoly( const std::vector< wxPoint >& aCornerList,
                               FILL_T aFill, int aWidth )
{
    if( aCornerList.size() <= 1 )
        return;

    // Gerber format does not know filled polygons with thick outline
    // Thereore, to plot a filled polygon with outline having a thickness,
    // one should plot outline as thick segments

    SetCurrentLineWidth( aWidth );

    if( aFill )
    {
        fputs( "G36*\n", outputFile );

        MoveTo( aCornerList[0] );

        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        FinishTo( aCornerList[0] );
        fputs( "G37*\n", outputFile );
    }

    if( aWidth > 0 )
    {
        MoveTo( aCornerList[0] );

        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        PenFinish();
    }
}

/**
 * Filled circular flashes are stored as apertures
 */
void GERBER_PLOTTER::FlashPadCircle( const wxPoint& pos, int diametre,
				     EDA_DRAW_MODE_T trace_mode )
{
    wxASSERT( outputFile );
    wxSize size( diametre, diametre );

    switch( trace_mode )
    {
    case LINE:
    case SKETCH:
        SetCurrentLineWidth( -1 );
        Circle( pos, diametre - currentPenWidth, NO_FILL );
        break;

    case FILLED:
	DPOINT pos_dev = userToDeviceCoordinates( pos );
        selectAperture( size, APERTURE::Circle );
	emitDcode( pos_dev, 3 );
        break;
    }
}


/**
 * Filled oval flashes are handled as aperture in the 90 degree positions only
 */
void GERBER_PLOTTER::FlashPadOval( const wxPoint& pos, const wxSize& aSize, double orient,
                                   EDA_DRAW_MODE_T trace_mode )
{
    wxASSERT( outputFile );
    int x0, y0, x1, y1, delta;
    wxSize size( aSize );

    /* Plot a flashed shape. */
    if( ( orient == 0 || orient == 900 || orient == 1800 || orient == 2700 )
       && trace_mode == FILLED )
    {
        if( orient == 900 || orient == 2700 ) /* orientation turned 90 deg. */
            EXCHG( size.x, size.y );

	DPOINT pos_dev = userToDeviceCoordinates( pos );
        selectAperture( size, APERTURE::Oval );
	emitDcode( pos_dev, 3 );
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
	    /* XXX to do: use an aperture macro to declare the rotated pad */
            /* The pad  is reduced to an oval with dy > dx */
            delta = size.y - size.x;
            x0    = 0;
            y0    = -delta / 2;
            x1    = 0;
            y1    = delta / 2;
            RotatePoint( &x0, &y0, orient );
            RotatePoint( &x1, &y1, orient );
            ThickSegment( wxPoint( pos.x + x0, pos.y + y0 ),
                           wxPoint( pos.x + x1, pos.y + y1 ),
                           size.x, trace_mode );
        }
        else
        {
            sketchOval( pos, size, orient, -1 );
        }
    }
}


/**
 * Filled rect flashes are handled as aperture in the 90 degree positions only
 */
void GERBER_PLOTTER::FlashPadRect( const wxPoint& pos, const wxSize& aSize,
                                   double orient, EDA_DRAW_MODE_T trace_mode )

{
    wxASSERT( outputFile );
    wxSize size( aSize );

    // Plot as an aperture flash
    switch( int( orient ) )
    {
    case 900:
    case 2700:        // rotation of 90 degrees or 270 swaps sizes
	EXCHG( size.x, size.y );

	// Pass through
    case 0:
    case 1800:
	switch( trace_mode )
	{
	case LINE:
	case SKETCH:
	    SetCurrentLineWidth( -1 );
	    Rect( wxPoint( pos.x - (size.x - currentPenWidth) / 2,
			pos.y - (size.y - currentPenWidth) / 2 ),
		    wxPoint( pos.x + (size.x - currentPenWidth) / 2,
			pos.y + (size.y - currentPenWidth) / 2 ),
		    NO_FILL );
	    break;

	case FILLED:
	    DPOINT pos_dev = userToDeviceCoordinates( pos );
	    selectAperture( size, APERTURE::Rect );
	    emitDcode( pos_dev, 3 );
	    break;
	}
	break;

    default: // plot pad shape as polygon
	{
	    // XXX to do: use an aperture macro to declare the rotated pad
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
	    coord[3].x = size.x/2;    // lower right
	    coord[3].y = size.y/2;

	    FlashPadTrapez( pos, coord, orient, trace_mode );
	}
	break;
    }
}


/**
 * Trapezoidal pad at the moment are *never* handled as aperture, since
 * they require aperture macros
 */
void GERBER_PLOTTER::FlashPadTrapez( const wxPoint& aPadPos,  const wxPoint* aCorners,
                                     double aPadOrient, EDA_DRAW_MODE_T aTrace_Mode )

{
    // XXX to do: use an aperture macro to declare the pad
    // polygon corners list
    std::vector< wxPoint > cornerList;

    for( int ii = 0; ii < 4; ii++ )
        cornerList.push_back( aCorners[ii] );

    // Draw the polygon and fill the interior as required
    for( unsigned ii = 0; ii < 4; ii++ )
    {
        RotatePoint( &cornerList[ii], aPadOrient );
        cornerList[ii] += aPadPos;
    }

    // Close the polygon
    cornerList.push_back( cornerList[0] );

    SetCurrentLineWidth( -1 );
    PlotPoly( cornerList, aTrace_Mode==FILLED ? FILLED_SHAPE : NO_FILL );
}

/**
 * Change the plot polarity and begin a new layer
 * Used to 'scratch off' silk screen away from solder mask
 */
void GERBER_PLOTTER::SetLayerPolarity( bool aPositive )
{
    if( aPositive )
        fprintf( outputFile, "%%LPD*%%\n" );
    else
        fprintf( outputFile, "%%LPC*%%\n" );
}


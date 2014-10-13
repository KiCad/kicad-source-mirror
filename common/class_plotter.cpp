/**
 * @file class_plotter.cpp
 * @brief KiCad: Base of all the plot routines
 * the class PLOTTER handle basic functions to plot schematic and boards
 * with different plot formats.
 *
 * There are currently engines for:
 * HPGL
 * POSTSCRIPT
 * GERBER
 * DXF
 * an SVG 'plot' is also provided along with the 'print' function by wx, but
 * is not handled here.
 */

#include <fctsys.h>

#include <trigo.h>
#include <wxstruct.h>
#include <base_struct.h>
#include <common.h>
#include <plot_common.h>
#include <macros.h>
#include <class_base_screen.h>
#include <drawtxt.h>

PLOTTER::PLOTTER( )
{
    plotScale = 1;
    defaultPenWidth = 0;
    currentPenWidth = -1;       // To-be-set marker
    penState = 'Z';             // End-of-path idle
    m_plotMirror = false;    		// Mirror flag
    m_mirrorIsHorizontal = true;
    m_yaxisReversed = false;
    outputFile = 0;
    colorMode = false;          // Starts as a BW plot
    negativeMode = false;
}

PLOTTER::~PLOTTER()
{
    // Emergency cleanup, but closing the file is
    // usually made in EndPlot().
    if( outputFile )
    {
        fclose( outputFile );
    }
}

/*
 * Open or create the plot file aFullFilename
 * return true if success, false if the file connot be created/opened
 *
 * Virtual because some plotters use ascii files, some others binary files (PDF)
 * The base class open the file in text mode
 */
bool PLOTTER::OpenFile( const wxString& aFullFilename )
{
    filename = aFullFilename;

    wxASSERT( !outputFile );

    // Open the file in text mode (not suitable for all plotters
    // but only for most of them
    outputFile = wxFopen( filename, wxT( "wt" ) );

    if( outputFile == NULL )
        return false ;

    return true;
}

/**
 * Modifies coordinates according to the orientation,
 * scale factor, and offsets trace. Also convert from a wxPoint to DPOINT,
 * since some output engines needs floating point coordinates.
 */
DPOINT PLOTTER::userToDeviceCoordinates( const wxPoint& aCoordinate )
{
    wxPoint pos = aCoordinate - plotOffset;

    double x = pos.x * plotScale;
    double y = ( paperSize.y - pos.y * plotScale );

    if( m_plotMirror )
    {
        if( m_mirrorIsHorizontal )
            x = ( paperSize.x - pos.x * plotScale );
        else
            y = pos.y * plotScale;
    }

    if( m_yaxisReversed )
        y = paperSize.y - y;

    x *= iuPerDeviceUnit;
    y *= iuPerDeviceUnit;

    return DPOINT( x, y );
}

/**
 * Modifies size according to the plotter scale factors
 * (wxSize version, returns a DPOINT)
 */
DPOINT PLOTTER::userToDeviceSize( const wxSize& size )
{
    return DPOINT( size.x * plotScale * iuPerDeviceUnit,
	           size.y * plotScale * iuPerDeviceUnit );
}

/**
 * Modifies size according to the plotter scale factors
 * (simple double version)
 */
double PLOTTER::userToDeviceSize( double size )
{
    return size * plotScale * iuPerDeviceUnit;
}


/**
 * Generic fallback: arc rendered as a polyline
 */
void PLOTTER::Arc( const wxPoint& centre, double StAngle, double EndAngle, int radius,
                   FILL_T fill, int width )
{
    wxPoint   start, end;
    const int delta = 50;   // increment (in 0.1 degrees) to draw circles

    if( StAngle > EndAngle )
        EXCHG( StAngle, EndAngle );

    SetCurrentLineWidth( width );
    /* Please NOTE the different sign due to Y-axis flip */
    start.x = centre.x + KiROUND( cosdecideg( radius, -StAngle ) );
    start.y = centre.y + KiROUND( sindecideg( radius, -StAngle ) );
    MoveTo( start );
    for( int ii = StAngle + delta; ii < EndAngle; ii += delta )
    {
        end.x = centre.x + KiROUND( cosdecideg( radius, -ii ) );
        end.y = centre.y + KiROUND( sindecideg( radius, -ii ) );
        LineTo( end );
    }

    end.x = centre.x + KiROUND( cosdecideg( radius, -EndAngle ) );
    end.y = centre.y + KiROUND( sindecideg( radius, -EndAngle ) );
    FinishTo( end );
}

/**
 * Fallback: if it doesn't handle bitmaps, we plot a rectangle
 */
void PLOTTER::PlotImage(const wxImage & aImage, const wxPoint& aPos,
                        double aScaleFactor )
{
    wxSize size( aImage.GetWidth() * aScaleFactor,
	         aImage.GetHeight() * aScaleFactor );

    wxPoint start = aPos;
    start.x -= size.x / 2;
    start.y -= size.y / 2;

    wxPoint end = start;
    end.x += size.x;
    end.y += size.y;

    Rect( start, end, NO_FILL );
}


/**
 * Plot a square centered on the position. Building block for markers
 */
void PLOTTER::markerSquare( const wxPoint& position, int radius )
{
    double r = KiROUND( radius / 1.4142 );
    std::vector< wxPoint > corner_list;
    wxPoint corner;
    corner.x = position.x + r;
    corner.y = position.y + r;
    corner_list.push_back( corner );
    corner.x = position.x + r;
    corner.y = position.y - r;
    corner_list.push_back( corner );
    corner.x = position.x - r;
    corner.y = position.y - r;
    corner_list.push_back( corner );
    corner.x = position.x - r;
    corner.y = position.y + r;
    corner_list.push_back( corner );
    corner.x = position.x + r;
    corner.y = position.y + r;
    corner_list.push_back( corner );

    PlotPoly( corner_list, NO_FILL, GetCurrentLineWidth() );
}

/**
 * Plot a circle centered on the position. Building block for markers
 */
void PLOTTER::markerCircle( const wxPoint& position, int radius )
{
    Circle( position, radius * 2, NO_FILL, GetCurrentLineWidth() );
}

/**
 * Plot a lozenge centered on the position. Building block for markers
 */
void PLOTTER::markerLozenge( const wxPoint& position, int radius )
{
    std::vector< wxPoint > corner_list;
    wxPoint corner;
    corner.x = position.x;
    corner.y = position.y + radius;
    corner_list.push_back( corner );
    corner.x = position.x + radius;
    corner.y = position.y,
    corner_list.push_back( corner );
    corner.x = position.x;
    corner.y = position.y - radius;
    corner_list.push_back( corner );
    corner.x = position.x - radius;
    corner.y = position.y;
    corner_list.push_back( corner );
    corner.x = position.x;
    corner.y = position.y + radius;
    corner_list.push_back( corner );

    PlotPoly( corner_list, NO_FILL, GetCurrentLineWidth() );
}

/**
 * Plot a - bar centered on the position. Building block for markers
 */
void PLOTTER::markerHBar( const wxPoint& pos, int radius )
{
    MoveTo( wxPoint( pos.x - radius, pos.y ) );
    FinishTo( wxPoint( pos.x + radius, pos.y ) );
}

/**
 * Plot a / bar centered on the position. Building block for markers
 */
void PLOTTER::markerSlash( const wxPoint& pos, int radius )
{
    MoveTo( wxPoint( pos.x - radius, pos.y - radius ) );
    FinishTo( wxPoint( pos.x + radius, pos.y + radius ) );
}

/**
 * Plot a \ bar centered on the position. Building block for markers
 */
void PLOTTER::markerBackSlash( const wxPoint& pos, int radius )
{
    MoveTo( wxPoint( pos.x + radius, pos.y - radius ) );
    FinishTo( wxPoint( pos.x - radius, pos.y + radius ) );
}

/**
 * Plot a | bar centered on the position. Building block for markers
 */
void PLOTTER::markerVBar( const wxPoint& pos, int radius )
{
    MoveTo( wxPoint( pos.x, pos.y - radius ) );
    FinishTo( wxPoint( pos.x, pos.y + radius ) );
}

/**
 * Draw a pattern shape number aShapeId, to coord x0, y0.
 * x0, y0 = coordinates tables
 * Diameter diameter = (coord table) hole
 * AShapeId = index (used to generate forms characters)
 */
void PLOTTER::Marker( const wxPoint& position, int diametre, unsigned aShapeId )
{
    int radius = diametre / 2;
    /* Marker are composed by a series of 'parts' superimposed; not every
       combination make sense, obviously. Since they are used in order I
       tried to keep the uglier/more complex constructions at the end.
       Also I avoided the |/ |\ -/ -\ construction because they're *very*
       ugly... if needed they could be added anyway... I'd like to see
       a board with more than 58 drilling/slotting tools!
       If Visual C++ supported the 0b literals they would be optimally
       and easily encoded as an integer array. We have to do with octal */
    static const unsigned char marker_patterns[MARKER_COUNT] = {
	// Bit order:  O Square Lozenge - | \ /
	// First choice: simple shapes
	0003,  // X
	0100,  // O
	0014,  // +
	0040,  // Sq
	0020,  // Lz
	// Two simple shapes
	0103,  // X O
	0017,  // X +
	0043,  // X Sq
	0023,  // X Lz
	0114,  // O +
	0140,  // O Sq
	0120,  // O Lz
	0054,  // + Sq
	0034,  // + Lz
	0060,  // Sq Lz
	// Three simple shapes
	0117,  // X O +
	0143,  // X O Sq
	0123,  // X O Lz
	0057,  // X + Sq
	0037,  // X + Lz
	0063,  // X Sq Lz
	0154,  // O + Sq
	0134,  // O + Lz
	0074,  // + Sq Lz
	// Four simple shapes
	0174,  // O Sq Lz +
	0163,  // X O Sq Lz
	0157,  // X O Sq +
	0137,  // X O Lz +
	0077,  // X Sq Lz +
	// This draws *everything *
	0177,  // X O Sq Lz +
	// Here we use the single bars... so the cross is forbidden
	0110,  // O -
	0104,  // O |
	0101,  // O /
	0050,  // Sq -
	0044,  // Sq |
	0041,  // Sq /
	0030,  // Lz -
	0024,  // Lz |
	0021,  // Lz /
	0150,  // O Sq -
	0144,  // O Sq |
	0141,  // O Sq /
	0130,  // O Lz -
	0124,  // O Lz |
	0121,  // O Lz /
	0070,  // Sq Lz -
	0064,  // Sq Lz |
	0061,  // Sq Lz /
	0170,  // O Sq Lz -
	0164,  // O Sq Lz |
	0161,  // O Sq Lz /
	// Last resort: the backlash component (easy to confound)
	0102,  // \ O
	0042,  // \ Sq
	0022,  // \ Lz
	0142,  // \ O Sq
	0122,  // \ O Lz
	0062,  // \ Sq Lz
	0162   // \ O Sq Lz
    };
    if( aShapeId >= MARKER_COUNT )
    {
        // Fallback shape
        markerCircle( position, radius );
    }
    else
    {
	// Decode the pattern and draw the corresponding parts
	unsigned char pat = marker_patterns[aShapeId];
	if( pat & 0001 )
	    markerSlash( position, radius );
	if( pat & 0002 )
	    markerBackSlash( position, radius );
	if( pat & 0004 )
	    markerVBar( position, radius );
	if( pat & 0010 )
	    markerHBar( position, radius );
	if( pat & 0020 )
	    markerLozenge( position, radius );
	if( pat & 0040 )
	    markerSquare( position, radius );
	if( pat & 0100 )
	    markerCircle( position, radius );
    }
}


/**
 * Convert a thick segment and plot it as an oval
 */
void PLOTTER::segmentAsOval( const wxPoint& start, const wxPoint& end, int width,
                             EDA_DRAW_MODE_T tracemode )
{
    wxPoint center( (start.x + end.x) / 2, (start.y + end.y) / 2 );
    wxSize  size( end.x - start.x, end.y - start.y );
    double  orient;

    if( size.y == 0 )
        orient = 0;
    else if( size.x == 0 )
        orient = 900;
    else
        orient = -ArcTangente( size.y, size.x );

    size.x = KiROUND( EuclideanNorm( size ) ) + width;
    size.y = width;

    FlashPadOval( center, size, orient, tracemode );
}


void PLOTTER::sketchOval( const wxPoint& pos, const wxSize& aSize, double orient,
                          int width )
{
    SetCurrentLineWidth( width );
    width = currentPenWidth;
    int radius, deltaxy, cx, cy;
    wxSize size( aSize );

    if( size.x > size.y )
    {
        EXCHG( size.x, size.y );
        orient = AddAngles( orient, 900 );
    }

    deltaxy = size.y - size.x;       /* distance between centers of the oval */
    radius   = ( size.x - width ) / 2;
    cx = -radius;
    cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    MoveTo( wxPoint( cx + pos.x, cy + pos.y ) );
    cx = -radius;
    cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    FinishTo( wxPoint( cx + pos.x, cy + pos.y ) );

    cx = radius;
    cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    MoveTo( wxPoint( cx + pos.x, cy + pos.y ) );
    cx = radius;
    cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    FinishTo( wxPoint( cx + pos.x, cy + pos.y ) );

    cx = 0;
    cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    Arc( wxPoint( cx + pos.x, cy + pos.y ),
         orient + 1800, orient + 3600,
         radius, NO_FILL );
    cx = 0;
    cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    Arc( wxPoint( cx + pos.x, cy + pos.y ),
         orient, orient + 1800,
         radius, NO_FILL );
}


/* Plot 1 segment like a track segment
 */
void PLOTTER::ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                            EDA_DRAW_MODE_T tracemode )
{
    switch( tracemode )
    {
    case FILLED:
    case LINE:
        SetCurrentLineWidth( tracemode==FILLED ? width : -1 );
        MoveTo( start );
        FinishTo( end );
        break;

    case SKETCH:
        SetCurrentLineWidth( -1 );
        segmentAsOval( start, end, width, tracemode );
        break;
    }
}


void PLOTTER::ThickArc( const wxPoint& centre, double StAngle, double EndAngle,
                        int radius, int width, EDA_DRAW_MODE_T tracemode )
{
    switch( tracemode )
    {
    case LINE:
        SetCurrentLineWidth( -1 );
        Arc( centre, StAngle, EndAngle, radius, NO_FILL, -1 );
        break;

    case FILLED:
        Arc( centre, StAngle, EndAngle, radius, NO_FILL, width );
        break;

    case SKETCH:
        SetCurrentLineWidth( -1 );
        Arc( centre, StAngle, EndAngle,
             radius - ( width - currentPenWidth ) / 2, NO_FILL, -1 );
        Arc( centre, StAngle, EndAngle,
             radius + ( width - currentPenWidth ) / 2, NO_FILL, -1 );
        break;
    }
}


void PLOTTER::ThickRect( const wxPoint& p1, const wxPoint& p2, int width,
                         EDA_DRAW_MODE_T tracemode )
{
    switch( tracemode )
    {
    case LINE:
        Rect( p1, p2, NO_FILL, -1 );
        break;

    case FILLED:
        Rect( p1, p2, NO_FILL, width );
        break;

    case SKETCH:
        SetCurrentLineWidth( -1 );
        wxPoint offsetp1( p1.x - (width - currentPenWidth) / 2,
                          p1.y - (width - currentPenWidth) / 2 );
        wxPoint offsetp2( p2.x + (width - currentPenWidth) / 2,
			  p2.y + (width - currentPenWidth) / 2 );
        Rect( offsetp1, offsetp2, NO_FILL, -1 );
        offsetp1.x += (width - currentPenWidth);
        offsetp1.y += (width - currentPenWidth);
        offsetp2.x -= (width - currentPenWidth);
        offsetp2.y -= (width - currentPenWidth);
        Rect( offsetp1, offsetp2, NO_FILL, -1 );
        break;
    }
}


void PLOTTER::ThickCircle( const wxPoint& pos, int diametre, int width,
                           EDA_DRAW_MODE_T tracemode )
{
    switch( tracemode )
    {
    case LINE:
        Circle( pos, diametre, NO_FILL, -1 );
        break;

    case FILLED:
        Circle( pos, diametre, NO_FILL, width );
        break;

    case SKETCH:
        SetCurrentLineWidth( -1 );
        Circle( pos, diametre - width + currentPenWidth, NO_FILL, -1 );
        Circle( pos, diametre + width - currentPenWidth, NO_FILL, -1 );
        break;
    }
}


void PLOTTER::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    wxASSERT( !outputFile );
    pageInfo = aPageSettings;
}


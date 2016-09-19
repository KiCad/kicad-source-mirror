/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file common_plotHPGL_functions.cpp
 * @brief KiCad: Common plot HPGL Routines
 * Since this plot engine is mostly intended for import in external programs,
 * sadly HPGL/2 isn't supported a lot... some of the primitives use overlapped
 * strokes to fill the shape
 */

/* Some HPGL commands:
 *  Note: the HPGL unit is 25 micrometers
 *  All commands MUST be terminated by a semi-colon or a linefeed.
 *  Spaces can NOT be substituted for required commas in the syntax of a command.
 *
 *
 *  AA (Arc Absolute): Angle is a floating point # (requires non integer value)
 *                   Draws an arc with the center at (X,Y).
 *                   A positive angle creates a counter-clockwise arc.
 *                   If the chord angle is specified,
 *                   this will be the number of degrees used for stepping around the arc.
 *                   If no value is given then a default value of five degrees is used.
 *  AA x, y, a {,b};
 *
 *  AR (Arc Relative):
 *  AR Dx, Dy, a {, b};
 *
 *  CA (Alternate Character Set):
 *  CA {n};
 *
 *  CI (Circle):
 *  CI r {,b};
 *
 *  CP (Character Plot):
 *  CP {h, v};
 *  h         [-127.9999 .. 127.9999]  Anzahl der Zeichen horizontal
 *  v         [-127.9999 .. 127.9999]  Anzahl der Zeichen vertikal
 *
 *  CS (Standard Character Set):
 *  CS {n};
 *
 *  DR (Relative Direction for Label Text):
 *  DR s, a;
 *
 *  DI (Absolute Direction for Label Text):
 *  DI {s, a};
 *
 *  DT (Define Terminator - this character becomes unavailable except to terminate a label string.
 *                        Default is ^C control-C):
 *  DT t;
 *
 *  EA (rEctangle Absolute - Unfilled, from current position to diagonal x,y):
 *  EA x, y;
 *
 *  ER (rEctangle Relative - Unfilled, from current position to diagonal x,y):
 *  ER x,y;
 *
 *  FT (Fill Type):
 *  FT {s {,l {a}}};
 *
 *  IM (Input Mask):
 *  IM {f};
 *
 *  IN (Initialize): This command instructs the controller to begin processing the HPGL plot file.
 *                 Without this, the commands in the file are received but never executed.
 *                 If multiple IN s are found during execution of the file,
 *                 the controller performs a Pause/Cancel operation.
 *                 All motion from the previous job, yet to be executed, is lost,
 *                 and the new information is executed.
 *  IN;
 *
 *  IP Input P1 and P2:
 *  IP {P1x, P1y {, P2x, P2y}};
 *
 *  IW (Input Window):
 *  IW {XUL, YUL, XOR, YOR};
 *
 *  LB (Label):
 *  LB c1 .. cn t;
 *
 *  PA (Plot Absolute): Moves to an absolute HPGL position and sets absolute mode for
 *                    future PU and PD commands. If no arguments follow the command,
 *                    only absolute mode is set.
 *  PA {x1, y1 {{PU|PD|,} ..., ..., xn, yn}};
 *  P1x, P1y, P2x, P2y  [Integer in ASCII]
 *
 *  PD (Pen Down): Executes <current pen> pen then moves to the requested position
 *               if one is specified. This position is dependent on whether absolute
 *               or relative mode is set. This command performs no motion in 3-D mode,
 *               but the outputs and feedrates are affected.
 *  PD {x, y};
 *
 *  PM          Polygon mode
 *      associated commands:
 *      PM2         End polygon mode
 *      FP          Fill polygon
 *      EP          Draw polygon outline
 *
 *  PR (Plot Relative):    Moves to the relative position specified and sets relative mode
 *                       for future PU and PD commands.
 *                       If no arguments follow the command, only relative mode is set.
 *  PR {Dx1, Dy1 {{PU|PD|,} ..., ..., Dxn, Dyn}};
 *
 *  PS (Paper Size):
 *  PS {n};
 *
 *  PT (Pen Thickness):     in mm
 *  PT {l};
 *
 *  PU (Pen Up):   Executes <current pen> pen then moves to the requested position
 *               if one is specified. This position is dependent on whether absolute
 *               or relative mode is set.
 *               This command performs no motion in 3-D mode, but the outputs
 *               and feedrates are affected.
 *  PU {x, y};
 *
 *  RA (Rectangle Absolute - Filled, from current position to diagonal x,y):
 *  RA x, y;
 *
 *  RO (Rotate Coordinate System):
 *  RO;
 *
 *  RR (Rectangle Relative - Filled, from current position to diagonal x,y):
 *  RR x, y;
 *
 *  SA (Select Alternate Set):
 *  SA;
 *
 *  SC (Scale):
 *  SC {Xmin, Xmax, Ymin, Ymax};
 *
 *  SI (Absolute Character Size):
 *  SI b, h;
 *  b         [-127.9999 .. 127.9999, keine 0]
 *  h         [-127.9999 .. 127.9999, keine 0]
 *
 *  SL (Character Slant):
 *  SL {a};
 *  a         [-3.5 .. -0.5, 0.5 .. 3.5]
*
 *  SP (Select Pen):   Selects a new pen or tool for use.
 *                   If no pen number or a value of zero is given,
 *                   the controller performs an EOF (end of file command).
 *                   Once an EOF is performed, no motion is executed,
 *                   until a new IN command is received.
 *  SP n;
 *
 *  SR (Relative Character Size):
 *  SR {b, h};
 *  b         [-127.9999 .. 127.9999, keine 0]
 *  h         [-127.9999 .. 127.9999, keine 0]
 *
 *  SS (Select Standard Set):
 *  SS;
 *
 *  TL (Tick Length):
 *  TL {tp {, tm}};
 *
 *  UC (User Defined Character):
 *  UC {i,} x1, y1, {i,} x2, y2, ... {i,} xn, yn;
 *
 *  VS (Velocity Select):
 *  VS {v {, n}};
 *  v         [1 .. 40]     in cm/s
 *  n         [1 .. 8]
 *
 *  XT (X Tick):
 *  XT;
 *
 *  YT (Y Tick):
 *  YT;
 */


#include <fctsys.h>
#include <gr_basic.h>
#include <trigo.h>
#include <wxstruct.h>
#include <base_struct.h>
#include <plot_common.h>
#include <macros.h>
#include <kicad_string.h>
#include <convert_basic_shapes_to_polygon.h>

// The hpgl command to close a polygon def, fill it and plot outline:
// PM 2; ends the polygon definition and closes it if not closed
// FP;   fills the polygon
// EP;   draws the polygon outline. It usually gives a better look to the filled polygon
static const char hpgl_end_polygon_cmd[] = "PM 2; FP; EP;\n";

// HPGL scale factor (1 PLU = 1/40mm = 25 micrometers)
static const double PLUsPERDECIMIL = 0.102041;

HPGL_PLOTTER::HPGL_PLOTTER()
{
    SetPenSpeed( 40 );      // Default pen speed = 40 cm/s; Pen speed is *always* in cm
    SetPenNumber( 1 );      // Default pen num = 1
    SetPenDiameter( 0.0 );
}

void HPGL_PLOTTER::SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                                double aScale, bool aMirror )
{
    wxASSERT( !outputFile );
    plotOffset  = aOffset;
    plotScale   = aScale;
    m_IUsPerDecimil = aIusPerDecimil;
    iuPerDeviceUnit = PLUsPERDECIMIL / aIusPerDecimil;
    /* Compute the paper size in IUs */
    paperSize   = pageInfo.GetSizeMils();
    paperSize.x *= 10.0 * aIusPerDecimil;
    paperSize.y *= 10.0 * aIusPerDecimil;
    SetDefaultLineWidth( 0 );    // HPGL has pen sizes instead
    m_plotMirror = aMirror;
}


/**
 * At the start of the HPGL plot pen speed and number are requested
 */
bool HPGL_PLOTTER::StartPlot()
{
    wxASSERT( outputFile );
    fprintf( outputFile, "IN;VS%d;PU;PA;SP%d;\n", penSpeed, penNumber );

    // Set HPGL Pen Thickness (in mm) (usefull in polygon fill command)
    double penThicknessMM = userToDeviceSize( penDiameter )/40;
    fprintf( outputFile, "PT %.1f;\n", penThicknessMM );

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


void HPGL_PLOTTER::SetPenDiameter( double diameter )
{
    penDiameter = diameter;
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


// HPGL circle
void HPGL_PLOTTER::Circle( const wxPoint& centre, int diameter, FILL_T fill,
                           int width )
{
    wxASSERT( outputFile );
    double radius = userToDeviceSize( diameter / 2 );
    SetCurrentLineWidth( width );

    if( fill == FILLED_SHAPE )
    {
        // Draw the filled area
        MoveTo( centre );
        fprintf( outputFile, "PM 0; CI %g;\n", radius );
        fprintf( outputFile, hpgl_end_polygon_cmd );   // Close, fill polygon and draw outlines
        PenFinish();
    }

    if( radius > 0 )
    {
        MoveTo( centre );
        fprintf( outputFile, "CI %g;\n", radius );
        PenFinish();
    }
}


/**
 * HPGL polygon:
 */

void HPGL_PLOTTER::PlotPoly( const std::vector<wxPoint>& aCornerList,
                             FILL_T aFill, int aWidth, void * aData )
{
    if( aCornerList.size() <= 1 )
        return;

    SetCurrentLineWidth( aWidth );
    MoveTo( aCornerList[0] );

    if( aFill == FILLED_SHAPE )
    {
        // Draw the filled area
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
        fprintf( outputFile, "PM 0;\n" );       // Start polygon

        for( unsigned ii = 1; ii < aCornerList.size(); ++ii )
            LineTo( aCornerList[ii] );

        int ii = aCornerList.size() - 1;

        if( aCornerList[ii] != aCornerList[0] )
            LineTo( aCornerList[0] );

        fprintf( outputFile, hpgl_end_polygon_cmd );   // Close, fill polygon and draw outlines
    }
    else
    {
        // Plot only the polygon outline.
        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        // Always close polygon if filled.
        if( aFill )
        {
            int ii = aCornerList.size() - 1;

            if( aCornerList[ii] != aCornerList[0] )
                LineTo( aCornerList[0] );
        }
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
        penState        = 'U';
        penLastpos.x    = -1;
        penLastpos.y    = -1;
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
        fprintf( outputFile, "PA %.0f,%.0f;\n", pos_dev.x, pos_dev.y );

    penLastpos = pos;
}


/**
 * HPGL supports dashed lines
 */
void HPGL_PLOTTER::SetDash( bool dashed )
{
    wxASSERT( outputFile );

    if( dashed )
        fputs( "LI 2;\n", outputFile );
    else
        fputs( "LI;\n", outputFile );
}


void HPGL_PLOTTER::ThickSegment( const wxPoint& start, const wxPoint& end,
                                 int width, EDA_DRAW_MODE_T tracemode, void* aData )
{
    wxASSERT( outputFile );
    wxPoint center;
    wxSize  size;

    // Suppress overlap if pen is too big
    if( penDiameter >= width )
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
void HPGL_PLOTTER::Arc( const wxPoint& centre, double StAngle, double EndAngle, int radius,
                        FILL_T fill, int width )
{
    wxASSERT( outputFile );
    double angle;

    if( radius <= 0 )
        return;

    DPOINT centre_dev = userToDeviceCoordinates( centre );

    if( m_plotMirror )
        angle = StAngle - EndAngle;
    else
        angle = EndAngle - StAngle;

    NORMALIZE_ANGLE_180( angle );
    angle /= 10;

    // Calculate arc start point:
    wxPoint cmap;
    cmap.x  = centre.x + KiROUND( cosdecideg( radius, StAngle ) );
    cmap.y  = centre.y - KiROUND( sindecideg( radius, StAngle ) );
    DPOINT  cmap_dev = userToDeviceCoordinates( cmap );

    fprintf( outputFile,
             "PU;PA %.0f,%.0f;PD;AA %.0f,%.0f,",
             cmap_dev.x, cmap_dev.y,
             centre_dev.x, centre_dev.y );
    fprintf( outputFile, "%.0f", angle );
    fprintf( outputFile, ";PU;\n" );
    PenFinish();
}


/* Plot oval pad.
 */
void HPGL_PLOTTER::FlashPadOval( const wxPoint& pos, const wxSize& aSize, double orient,
                                 EDA_DRAW_MODE_T trace_mode, void* aData )
{
    wxASSERT( outputFile );
    int     deltaxy, cx, cy;
    wxSize  size( aSize );

    /* The pad will be drawn as an oblong shape with size.y > size.x
     * (Oval vertical orientation 0)
     */
    if( size.x > size.y )
    {
        std::swap( size.x, size.y );
        orient = AddAngles( orient, 900 );
    }

    deltaxy = size.y - size.x;     // distance between centers of the oval

    if( trace_mode == FILLED )
    {
        FlashPadRect( pos, wxSize( size.x, deltaxy + KiROUND( penDiameter ) ),
                      orient, trace_mode, aData );
        cx = 0; cy = deltaxy / 2;
        RotatePoint( &cx, &cy, orient );
        FlashPadCircle( wxPoint( cx + pos.x, cy + pos.y ), size.x, trace_mode, aData );
        cx = 0; cy = -deltaxy / 2;
        RotatePoint( &cx, &cy, orient );
        FlashPadCircle( wxPoint( cx + pos.x, cy + pos.y ), size.x, trace_mode, aData );
    }
    else    // Plot in outline mode.
    {
        sketchOval( pos, size, orient, KiROUND( penDiameter ) );
    }
}


/* Plot round pad or via.
 */
void HPGL_PLOTTER::FlashPadCircle( const wxPoint& pos, int diametre,
                                   EDA_DRAW_MODE_T trace_mode, void* aData )
{
    wxASSERT( outputFile );
    DPOINT  pos_dev = userToDeviceCoordinates( pos );

    int     radius  = diametre / 2;

    if( trace_mode == FILLED )
    {
        // if filled mode, the pen diameter is removed from diameter
        // to keep the pad size
        radius -= KiROUND( penDiameter ) / 2;
    }

    if( radius < 0 )
        radius = 0;

    double rsize = userToDeviceSize( radius );

    if( trace_mode == FILLED )        // Plot in filled mode.
    {
        // A filled polygon uses always the current point to start the polygon.
        // Gives a correct current starting point for the circle
        MoveTo( wxPoint( pos.x+radius, pos.y ) );
        // Plot filled area and its outline
        fprintf( outputFile, "PM 0; PA %.0f,%.0f;CI %.0f;%s",
                         pos_dev.x, pos_dev.y, rsize, hpgl_end_polygon_cmd );
    }
    else
    {
        // Draw outline only:
        fprintf( outputFile, "PA %.0f,%.0f;CI %.0f;\n",
                     pos_dev.x, pos_dev.y, rsize );
    }

    PenFinish();
}


void HPGL_PLOTTER::FlashPadRect( const wxPoint& pos, const wxSize& padsize,
                                 double orient, EDA_DRAW_MODE_T trace_mode, void* aData )
{
    // Build rect polygon:
    std::vector<wxPoint> corners;

    int dx = padsize.x / 2;
    int dy = padsize.y / 2;

    if( trace_mode == FILLED )
    {
        // in filled mode, the pen diameter is removed from size
        // to compensate the extra size due to this pen size
        dx -= KiROUND( penDiameter ) / 2;
        dx = std::max( dx, 0);
        dy -= KiROUND( penDiameter ) / 2;
        dy = std::max( dy, 0);
    }


    corners.push_back( wxPoint( - dx, - dy ) );
    corners.push_back( wxPoint( - dx, + dy ) );
    corners.push_back( wxPoint( + dx, + dy ) );
    corners.push_back( wxPoint( + dx, - dy ) );


    for( unsigned ii = 0; ii < corners.size(); ii++ )
    {
        RotatePoint( &corners[ii], orient );
        corners[ii] += pos;
    }

    PlotPoly( corners, trace_mode == FILLED ? FILLED_SHAPE : NO_FILL );
}


void HPGL_PLOTTER::FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                        int aCornerRadius, double aOrient,
                                        EDA_DRAW_MODE_T aTraceMode, void* aData )
{
    SHAPE_POLY_SET outline;
    const int segmentToCircleCount = 32;

    wxSize size = aSize;

    if( aTraceMode == FILLED )
    {
        // in filled mode, the pen diameter is removed from size
        // to keep the pad size
        size.x -= KiROUND( penDiameter ) / 2;
        size.x = std::max( size.x, 0);
        size.y -= KiROUND( penDiameter ) / 2;
        size.y = std::max( size.y, 0);

        // keep aCornerRadius to a value < min size x,y < 2:
        aCornerRadius = std::min( aCornerRadius, std::min( size.x, size.y ) /2 );
    }

    TransformRoundRectToPolygon( outline, aPadPos, size, aOrient,
                                 aCornerRadius, segmentToCircleCount );

    // TransformRoundRectToPolygon creates only one convex polygon
    std::vector< wxPoint > cornerList;
    cornerList.reserve( segmentToCircleCount + 4 );
    SHAPE_LINE_CHAIN& poly = outline.Outline( 0 );

    for( int ii = 0; ii < poly.PointCount(); ++ii )
        cornerList.push_back( wxPoint( poly.Point( ii ).x, poly.Point( ii ).y ) );

    PlotPoly( cornerList, aTraceMode == FILLED ? FILLED_SHAPE : NO_FILL );
}

void HPGL_PLOTTER::FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                   SHAPE_POLY_SET* aPolygons,
                                   EDA_DRAW_MODE_T aTraceMode, void* aData )
{
    std::vector< wxPoint > cornerList;

    for( int cnt = 0; cnt < aPolygons->OutlineCount(); ++cnt )
    {
        SHAPE_LINE_CHAIN& poly = aPolygons->Outline( cnt );

        cornerList.clear();
        cornerList.reserve( poly.PointCount() );

        for( int ii = 1; ii < poly.PointCount(); ++ii )
            cornerList.push_back( wxPoint( poly.Point( ii ).x, poly.Point( ii ).y ) );

        PlotPoly( cornerList, aTraceMode == FILLED ? FILLED_SHAPE : NO_FILL );
    }
}


void HPGL_PLOTTER::FlashPadTrapez( const wxPoint& aPadPos, const wxPoint* aCorners,
                                   double aPadOrient, EDA_DRAW_MODE_T aTrace_Mode, void* aData )
{
    std::vector< wxPoint > cornerList;
    cornerList.reserve( 4 );

    for( int ii = 0; ii < 4; ii++ )
    {
        wxPoint coord( aCorners[ii] );
        RotatePoint( &coord, aPadOrient );
        coord += aPadPos;
        cornerList.push_back( coord );
    }

    PlotPoly( cornerList, aTrace_Mode == FILLED ? FILLED_SHAPE : NO_FILL );
}

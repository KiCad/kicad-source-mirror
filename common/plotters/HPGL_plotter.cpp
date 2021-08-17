/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file HPGL_plotter.cpp
 * @brief KiCad plotter for HPGL file format.
 * Since this plot engine is mostly intended for import in external programs,
 * sadly HPGL/2 isn't supported a lot... some of the primitives use overlapped
 * strokes to fill the shape.
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

#include <cstdio>

#include <string_utils.h>
#include <convert_basic_shapes_to_polygon.h>
#include <math/util.h>      // for KiROUND
#include <trigo.h>

#include "plotter_hpgl.h"


/// Compute the distance between two DPOINT points.
static double dpoint_dist( const DPOINT& a, const DPOINT& b );


// The hpgl command to close a polygon def, fill it and plot outline:
// PM 2; ends the polygon definition and closes it if not closed
// FP;   fills the polygon
// EP;   draws the polygon outline. It usually gives a better look to the filled polygon
static const char hpgl_end_polygon_cmd[] = "PM 2; FP; EP;\n";


// HPGL scale factor (1 Plotter Logical Unit = 1/40mm = 25 micrometers)
// PLUsPERDECIMIL = (25.4 / 10000) / 0.025
static const double PLUsPERDECIMIL = 0.1016;


HPGL_PLOTTER::HPGL_PLOTTER()
        : arcTargetChordLength( 0 ),
          arcMinChordDegrees( 5.0 ),
          dashType( PLOT_DASH_TYPE::SOLID ),
          useUserCoords( false ),
          fitUserCoords( false ),
          m_current_item( nullptr )
{
    SetPenSpeed( 40 );      // Default pen speed = 40 cm/s; Pen speed is *always* in cm
    SetPenNumber( 1 );      // Default pen num = 1
    SetPenDiameter( 0.0 );
}


void HPGL_PLOTTER::SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                                double aScale, bool aMirror )
{
    m_plotOffset      = aOffset;
    m_plotScale       = aScale;
    m_IUsPerDecimil   = aIusPerDecimil;
    m_iuPerDeviceUnit = PLUsPERDECIMIL / aIusPerDecimil;

    // Compute the paper size in IUs.
    m_paperSize   = m_pageInfo.GetSizeMils();
    m_paperSize.x *= 10.0 * aIusPerDecimil;
    m_paperSize.y *= 10.0 * aIusPerDecimil;
    m_plotMirror  = aMirror;
}


void HPGL_PLOTTER::SetTargetChordLength( double chord_len )
{
    arcTargetChordLength = userToDeviceSize( chord_len );
}


bool HPGL_PLOTTER::StartPlot()
{
    wxASSERT( m_outputFile );
    fprintf( m_outputFile, "IN;VS%d;PU;PA;SP%d;\n", penSpeed, penNumber );

    // Set HPGL Pen Thickness (in mm) (useful in polygon fill command)
    double penThicknessMM = userToDeviceSize( penDiameter )/40;
    fprintf( m_outputFile, "PT %.1f;\n", penThicknessMM );

    return true;
}


bool HPGL_PLOTTER::EndPlot()
{
    wxASSERT( m_outputFile );

    fputs( "PU;\n", m_outputFile );

    flushItem();
    sortItems( m_items );

    if( m_items.size() > 0 )
    {
        if( useUserCoords )
        {
            if( fitUserCoords )
            {
                BOX2D bbox = m_items.front().bbox;
                for( HPGL_ITEM const& item : m_items )
                {
                    bbox.Merge( item.bbox );
                }

                fprintf( m_outputFile, "SC%.0f,%.0f,%.0f,%.0f;\n", bbox.GetX(),
                         bbox.GetX() + bbox.GetWidth(), bbox.GetY(),
                         bbox.GetY() + bbox.GetHeight() );
            }
            else
            {
                DPOINT pagesize_dev( m_paperSize * m_iuPerDeviceUnit );
                fprintf( m_outputFile, "SC%.0f,%.0f,%.0f,%.0f;\n", 0., pagesize_dev.x, 0.,
                         pagesize_dev.y );
            }
        }

        DPOINT         loc          = m_items.begin()->loc_start;
        bool           pen_up       = true;
        PLOT_DASH_TYPE current_dash = PLOT_DASH_TYPE::SOLID;
        int            current_pen  = penNumber;

        for( HPGL_ITEM const& item : m_items )
        {
            if( item.loc_start != loc || pen_up )
            {
                if( !pen_up )
                {
                    fputs( "PU;", m_outputFile );
                    pen_up = true;
                }

                fprintf( m_outputFile, "PA %.0f,%.0f;", item.loc_start.x, item.loc_start.y );
            }

            if( item.dashType != current_dash )
            {
                current_dash = item.dashType;
                fputs( lineTypeCommand( item.dashType ), m_outputFile );
            }

            if( item.pen != current_pen )
            {
                if( !pen_up )
                {
                    fputs( "PU;", m_outputFile );
                    pen_up = true;
                }

                fprintf( m_outputFile, "SP%d;", item.pen );
                current_pen = item.pen;
            }

            if( pen_up && !item.lift_before )
            {
                fputs( "PD;", m_outputFile );
                pen_up = false;
            }
            else if( !pen_up && item.lift_before )
            {
                fputs( "PU;", m_outputFile );
                pen_up = true;
            }

            fputs( static_cast<const char*>( item.content.utf8_str() ), m_outputFile );

            if( !item.pen_returns )
            {
                // Assume commands drop the pen
                pen_up = false;
            }

            if( item.lift_after )
            {
                fputs( "PU;", m_outputFile );
                pen_up = true;
            }
            else
            {
                loc = item.loc_end;
            }

            fputs( "\n", m_outputFile );
        }
    }

    fputs( "PU;PA;SP0;\n", m_outputFile );
    fclose( m_outputFile );
    m_outputFile = nullptr;
    return true;
}


void HPGL_PLOTTER::SetPenDiameter( double diameter )
{
    penDiameter = diameter;
}


void HPGL_PLOTTER::Rect( const wxPoint& p1, const wxPoint& p2, FILL_TYPE fill, int width )
{
    wxASSERT( m_outputFile );

    DPOINT p1dev = userToDeviceCoordinates( p1 );
    DPOINT p2dev = userToDeviceCoordinates( p2 );

    MoveTo( p1 );

    if( fill == FILL_TYPE::FILLED_SHAPE )
    {
        startOrAppendItem( p1dev, wxString::Format( "RA %.0f,%.0f;", p2dev.x, p2dev.y ) );
    }

    startOrAppendItem( p1dev, wxString::Format( "EA %.0f,%.0f;", p2dev.x, p2dev.y ) );

    m_current_item->loc_end = m_current_item->loc_start;
    m_current_item->bbox.Merge( p2dev );
    PenFinish();
}


void HPGL_PLOTTER::Circle( const wxPoint& centre, int diameter, FILL_TYPE fill, int width )
{
    wxASSERT( m_outputFile );
    double radius = userToDeviceSize( diameter / 2 );
    DPOINT center_dev = userToDeviceCoordinates( centre );
    SetCurrentLineWidth( width );

    double const circumf             = 2.0 * M_PI * radius;
    double const target_chord_length = arcTargetChordLength;
    double       chord_degrees       = 360.0 * target_chord_length / circumf;

    if( chord_degrees < arcMinChordDegrees )
    {
        chord_degrees = arcMinChordDegrees;
    }
    else if( chord_degrees > 45 )
    {
        chord_degrees = 45;
    }

    if( fill == FILL_TYPE::FILLED_SHAPE )
    {
        // Draw the filled area
        MoveTo( centre );
        startOrAppendItem( center_dev, wxString::Format( "PM 0;CI %g,%g;%s", radius, chord_degrees,
                                                         hpgl_end_polygon_cmd ) );
        m_current_item->lift_before = true;
        m_current_item->pen_returns = true;
        m_current_item->bbox.Merge( BOX2D( center_dev - radius,
                                           VECTOR2D( 2 * radius, 2 * radius ) ) );
        PenFinish();
    }

    if( radius > 0 )
    {
        MoveTo( centre );
        startOrAppendItem( center_dev, wxString::Format( "CI %g,%g;", radius, chord_degrees ) );
        m_current_item->lift_before = true;
        m_current_item->pen_returns = true;
        m_current_item->bbox.Merge( BOX2D( center_dev - radius,
                                           VECTOR2D( 2 * radius, 2 * radius ) ) );
        PenFinish();
    }
}


void HPGL_PLOTTER::PlotPoly( const std::vector<wxPoint>& aCornerList, FILL_TYPE aFill,
                             int aWidth, void* aData )
{
    if( aCornerList.size() <= 1 )
        return;

    // Width less than zero is occasionally used to create background-only
    // polygons. Don't set that as the plotter line width, that'll cause
    // trouble. Also, later, skip plotting the outline if this is the case.
    if( aWidth > 0 )
    {
        SetCurrentLineWidth( aWidth );
    }

    MoveTo( aCornerList[0] );
    startItem( userToDeviceCoordinates( aCornerList[0] ) );

    if( aFill == FILL_TYPE::FILLED_SHAPE )
    {
        // Draw the filled area
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );

        m_current_item->content << wxString( "PM 0;\n" ); // Start polygon

        for( unsigned ii = 1; ii < aCornerList.size(); ++ii )
            LineTo( aCornerList[ii] );

        int ii = aCornerList.size() - 1;

        if( aCornerList[ii] != aCornerList[0] )
            LineTo( aCornerList[0] );

        m_current_item->content << hpgl_end_polygon_cmd; // Close, fill polygon and draw outlines
        m_current_item->pen_returns = true;
    }
    else if( aWidth > 0 )
    {
        // Plot only the polygon outline.
        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        // Always close polygon if filled.
        if( aFill != FILL_TYPE::NO_FILL )
        {
            int ii = aCornerList.size() - 1;

            if( aCornerList[ii] != aCornerList[0] )
                LineTo( aCornerList[0] );
        }
    }

    PenFinish();
}


void HPGL_PLOTTER::PenTo( const wxPoint& pos, char plume )
{
    wxASSERT( m_outputFile );

    if( plume == 'Z' )
    {
        m_penState = 'Z';
        flushItem();
        return;
    }

    DPOINT pos_dev = userToDeviceCoordinates( pos );
    DPOINT lastpos_dev = userToDeviceCoordinates( m_penLastpos );

    if( plume == 'U' )
    {
        m_penState = 'U';
        flushItem();
    }
    else if( plume == 'D' )
    {
        m_penState = 'D';
        startOrAppendItem( lastpos_dev, wxString::Format( "PA %.0f,%.0f;", pos_dev.x, pos_dev.y ) );
        m_current_item->loc_end = pos_dev;
        m_current_item->bbox.Merge( pos_dev );
    }

    m_penLastpos = pos;
}


void HPGL_PLOTTER::SetDash( PLOT_DASH_TYPE dashed )
{
    dashType = dashed;
    flushItem();
}


void HPGL_PLOTTER::ThickSegment( const wxPoint& start, const wxPoint& end,
                                 int width, OUTLINE_MODE tracemode, void* aData )
{
    wxASSERT( m_outputFile );

    // Suppress overlap if pen is too big
    if( penDiameter >= width )
    {
        MoveTo( start );
        FinishTo( end );
    }
    else
    {
        segmentAsOval( start, end, width, tracemode );
    }
}


void HPGL_PLOTTER::Arc( const wxPoint& centre, double StAngle, double EndAngle, int radius,
                        FILL_TYPE fill, int width )
{
    wxASSERT( m_outputFile );
    double angle;

    if( radius <= 0 )
        return;

    double const radius_dev          = userToDeviceSize( radius );
    double const circumf_dev         = 2.0 * M_PI * radius_dev;
    double const target_chord_length = arcTargetChordLength;
    double       chord_degrees       = 360.0 * target_chord_length / circumf_dev;

    if( chord_degrees < arcMinChordDegrees )
    {
        chord_degrees = arcMinChordDegrees;
    }
    else if( chord_degrees > 45 )
    {
        chord_degrees = 45;
    }

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

    startOrAppendItem( cmap_dev, wxString::Format( "AA %.0f,%.0f,%.0f,%g", centre_dev.x,
                                                   centre_dev.y, angle, chord_degrees ) );

    // TODO We could compute the final position and full bounding box instead...
    m_current_item->bbox.Merge( BOX2D( centre_dev - radius_dev,
                                       VECTOR2D( radius_dev * 2, radius_dev * 2 ) ) );
    m_current_item->lift_after = true;
    flushItem();
}


void HPGL_PLOTTER::FlashPadOval( const wxPoint& pos, const wxSize& aSize, double orient,
                                 OUTLINE_MODE trace_mode, void* aData )
{
    wxASSERT( m_outputFile );
    int     deltaxy, cx, cy;
    wxSize  size( aSize );

    // The pad will be drawn as an oblong shape with size.y > size.x (Oval vertical orientation 0).
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


void HPGL_PLOTTER::FlashPadCircle( const wxPoint& pos, int diametre,
                                   OUTLINE_MODE trace_mode, void* aData )
{
    wxASSERT( m_outputFile );
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
        startOrAppendItem( userToDeviceCoordinates( wxPoint( pos.x + radius, pos.y ) ),
                           wxString::Format( "PM 0; PA %.0f,%.0f;CI %.0f;%s",
                                             pos_dev.x, pos_dev.y, rsize, hpgl_end_polygon_cmd ) );
        m_current_item->lift_before = true;
        m_current_item->pen_returns = true;
    }
    else
    {
        // Draw outline only:
        startOrAppendItem( pos_dev, wxString::Format( "CI %.0f;", rsize ) );
        m_current_item->lift_before = true;
        m_current_item->pen_returns = true;
    }

    PenFinish();
}


void HPGL_PLOTTER::FlashPadRect( const wxPoint& pos, const wxSize& padsize,
                                 double orient, OUTLINE_MODE trace_mode, void* aData )
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


    corners.emplace_back( - dx, - dy );
    corners.emplace_back( - dx, + dy );
    corners.emplace_back( + dx, + dy );
    corners.emplace_back( + dx, - dy );

    // Close polygon
    corners.emplace_back( - dx, - dy );

    for( unsigned ii = 0; ii < corners.size(); ii++ )
    {
        RotatePoint( &corners[ii], orient );
        corners[ii] += pos;
    }

    PlotPoly( corners, trace_mode == FILLED ? FILL_TYPE::FILLED_SHAPE : FILL_TYPE::NO_FILL );
}


void HPGL_PLOTTER::FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                      int aCornerRadius, double aOrient,
                                      OUTLINE_MODE aTraceMode, void* aData )
{
    SHAPE_POLY_SET outline;

    wxSize size = aSize;

    if( aTraceMode == FILLED )
    {
        // In filled mode, the pen diameter is removed from size to keep the pad size.
        size.x -= KiROUND( penDiameter ) / 2;
        size.x = std::max( size.x, 0);
        size.y -= KiROUND( penDiameter ) / 2;
        size.y = std::max( size.y, 0);

        // keep aCornerRadius to a value < min size x,y < 2:
        aCornerRadius = std::min( aCornerRadius, std::min( size.x, size.y ) /2 );
    }

    TransformRoundChamferedRectToPolygon( outline, aPadPos, size, aOrient, aCornerRadius,
                                          0.0, 0, 0, GetPlotterArcHighDef(), ERROR_INSIDE );

    // TransformRoundRectToPolygon creates only one convex polygon
    std::vector<wxPoint> cornerList;
    SHAPE_LINE_CHAIN&    poly = outline.Outline( 0 );
    cornerList.reserve( poly.PointCount() );

    for( int ii = 0; ii < poly.PointCount(); ++ii )
        cornerList.emplace_back( poly.CPoint( ii ).x, poly.CPoint( ii ).y );

    if( cornerList.back() != cornerList.front() )
        cornerList.push_back( cornerList.front() );

    PlotPoly( cornerList, aTraceMode == FILLED ? FILL_TYPE::FILLED_SHAPE : FILL_TYPE::NO_FILL );
}


void HPGL_PLOTTER::FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize, double aOrient,
                                   SHAPE_POLY_SET* aPolygons, OUTLINE_MODE aTraceMode, void* aData )
{
    std::vector< wxPoint > cornerList;

    for( int cnt = 0; cnt < aPolygons->OutlineCount(); ++cnt )
    {
        SHAPE_LINE_CHAIN& poly = aPolygons->Outline( cnt );

        cornerList.clear();
        cornerList.reserve( poly.PointCount() );

        for( int ii = 0; ii < poly.PointCount(); ++ii )
            cornerList.emplace_back( poly.CPoint( ii ).x, poly.CPoint( ii ).y );

        if( cornerList.back() != cornerList.front() )
            cornerList.push_back( cornerList.front() );

        PlotPoly( cornerList, aTraceMode == FILLED ? FILL_TYPE::FILLED_SHAPE : FILL_TYPE::NO_FILL );
    }
}


void HPGL_PLOTTER::FlashPadTrapez( const wxPoint& aPadPos, const wxPoint* aCorners,
                                   double aPadOrient, OUTLINE_MODE aTraceMode, void* aData )
{
    std::vector< wxPoint > cornerList;
    cornerList.reserve( 5 );

    for( int ii = 0; ii < 4; ii++ )
    {
        wxPoint coord( aCorners[ii] );
        RotatePoint( &coord, aPadOrient );
        coord += aPadPos;
        cornerList.push_back( coord );
    }

    // Close polygon
    cornerList.push_back( cornerList.front() );

    PlotPoly( cornerList, aTraceMode == FILLED ? FILL_TYPE::FILLED_SHAPE : FILL_TYPE::NO_FILL );
}


void HPGL_PLOTTER::FlashRegularPolygon( const wxPoint& aShapePos, int aRadius, int aCornerCount,
                                        double aOrient, OUTLINE_MODE aTraceMode, void* aData )
{
    // Do nothing
    wxASSERT( 0 );
}


bool HPGL_PLOTTER::startItem( const DPOINT& location )
{
    return startOrAppendItem( location, wxEmptyString );
}


void HPGL_PLOTTER::flushItem()
{
    m_current_item = nullptr;
}


bool HPGL_PLOTTER::startOrAppendItem( const DPOINT& location, wxString const& content )
{
    if( m_current_item == nullptr )
    {
        HPGL_ITEM item;
        item.loc_start = location;
        item.loc_end = location;
        item.bbox = BOX2D( location );
        item.pen = penNumber;
        item.dashType = dashType;
        item.content = content;
        m_items.push_back( item );
        m_current_item = &m_items.back();
        return true;
    }
    else
    {
        m_current_item->content << content;
        return false;
    }
}


void HPGL_PLOTTER::sortItems( std::list<HPGL_ITEM>& items )
{
    if( items.size() < 2 )
    {
        return;
    }

    std::list<HPGL_ITEM> target;

    // Plot items are sorted to improve print time on mechanical plotters. This
    // means
    //  1) Avoid excess pen-switching - once a pen is selected, keep printing
    //      with it until no more items using that pen remain.
    //  2) Within the items for one pen, avoid bouncing back and forth around
    //      the page; items should be sequenced with nearby items.
    //
    // This is essentially a variant of the Traveling Salesman Problem where
    // the cities are themselves edges that must be traversed. This is of course
    // a famously NP-Hard problem and this particular variant has a monstrous
    // number of "cities". For now, we're using a naive nearest-neighbor search,
    // which is less than optimal but (usually!) better than nothing, very
    // simple to implement, and fast enough.
    //
    // Items are moved one at a time from `items` into `target`, searching
    // each time for the first one matching the above criteria. Then, all of
    // `target` is moved back into `items`.

    // Get the first one started
    HPGL_ITEM last_item = items.front();
    items.pop_front();
    target.emplace_back( last_item );

    while( !items.empty() )
    {
        auto best_it = items.begin();
        double best_dist = dpoint_dist( last_item.loc_end, best_it->loc_start );

        for( auto search_it = best_it; search_it != items.end(); search_it++ )
        {
            // Immediately forget an item as "best" if another one is a better pen match
            if( best_it->pen != last_item.pen && search_it->pen == last_item.pen )
            {
                best_it = search_it;
                continue;
            }

            double const dist = dpoint_dist( last_item.loc_end, search_it->loc_start );

            if( dist < best_dist )
            {
                best_it   = search_it;
                best_dist = dist;
                continue;
            }
        }

        target.emplace_back( *best_it );
        last_item = *best_it;
        items.erase( best_it );
    }

    items.splice( items.begin(), target );
}


const char* HPGL_PLOTTER::lineTypeCommand( PLOT_DASH_TYPE linetype )
{
    switch( linetype )
    {
    case PLOT_DASH_TYPE::DASH:
        return "LT -2 4 1;";
        break;
    case PLOT_DASH_TYPE::DOT:
        return "LT -1 2 1;";
        break;
    case PLOT_DASH_TYPE::DASHDOT:
        return "LT -4 6 1;";
        break;
    default:
        return "LT;";
        break;
    }
}


static double dpoint_dist( const DPOINT& a, const DPOINT& b )
{
    DPOINT diff = a - b;
    return sqrt( diff.x * diff.x + diff.y * diff.y );
}

/**
 * @file graphpcb.cpp
 * @brief PCB editor autorouting and "graphics" routines.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <trigo.h>
#include <pcbcommon.h>
#include <math_for_graphics.h>
#include <class_board.h>
#include <class_track.h>
#include <class_drawsegment.h>

#include <pcbnew.h>
#include <autorout.h>
#include <cell.h>

void TracePcbLine( int x0, int y0, int x1, int y1, LAYER_NUM layer, int color );

void TraceArc( int ux0, int uy0,
               int ux1, int uy1,
               double ArcAngle,
               int lg, LAYER_NUM layer, int color,
               int op_logic );


static void DrawSegmentQcq( int ux0, int uy0,
                            int ux1, int uy1,
                            int lg, LAYER_NUM layer, int color,
                            int op_logic );

static void TraceFilledCircle( int    cx, int cy, int radius,
                               LSET aLayerMask,
                               int    color,
                               int    op_logic );

static void TraceCircle( int ux0, int uy0, int ux1, int uy1, int lg, LAYER_NUM layer,
                         int color, int op_logic );

// Macro call to update cell.
#define OP_CELL( layer, dy, dx )                                        \
    {                                                                   \
        if( layer == UNDEFINED_LAYER )                                  \
        {                                                               \
            RoutingMatrix.WriteCell( dy, dx, BOTTOM, color );           \
            if( RoutingMatrix.m_RoutingLayersCount > 1 )                \
                RoutingMatrix.WriteCell( dy, dx, TOP, color );          \
        }                                                               \
        else                                                            \
        {                                                               \
            if( layer == g_Route_Layer_BOTTOM )                         \
                RoutingMatrix.WriteCell( dy, dx, BOTTOM, color );       \
            if( RoutingMatrix.m_RoutingLayersCount > 1 )                \
                if( layer == g_Route_Layer_TOP )                        \
                    RoutingMatrix.WriteCell( dy, dx, TOP, color );      \
        }                                                               \
    }

void PlacePad( D_PAD* aPad, int color, int marge, int op_logic )
{
    int     dx, dy;
    wxPoint shape_pos = aPad->ShapePos();

    dx = aPad->GetSize().x / 2;
    dx += marge;

    if( aPad->GetShape() == PAD_CIRCLE )
    {
        TraceFilledCircle( shape_pos.x, shape_pos.y, dx,
                           aPad->GetLayerSet(), color, op_logic );
        return;
    }

    dy = aPad->GetSize().y / 2;
    dy += marge;

    if( aPad->GetShape() == PAD_TRAPEZOID )
    {
        dx += abs( aPad->GetDelta().y ) / 2;
        dy += abs( aPad->GetDelta().x ) / 2;
    }

    // The pad is a rectangle ( horizontal or vertical )
    if( int( aPad->GetOrientation() ) % 900 == 0 )
    {
        // Orientation turned 90 deg.
        if( aPad->GetOrientation() == 900  ||  aPad->GetOrientation() == 2700 )
        {
            EXCHG( dx, dy );
        }

        TraceFilledRectangle( shape_pos.x - dx, shape_pos.y - dy,
                              shape_pos.x + dx, shape_pos.y + dy,
                              aPad->GetLayerSet(), color, op_logic );
    }
    else
    {
        TraceFilledRectangle( shape_pos.x - dx, shape_pos.y - dy,
                              shape_pos.x + dx, shape_pos.y + dy,
                              aPad->GetOrientation(),
                              aPad->GetLayerSet(), color, op_logic );
    }
}


/* Set to color the cells included in the circle
 * Parameters:
 * center: cx, cy.
 * radius: a value add to the radius or half the score pad
 * aLayerMask: layer occupied
 * color: mask write in cells
 * op_logic: type of writing in the cell (WRITE, OR)
 */
void TraceFilledCircle( int cx, int cy, int radius,
        LSET aLayerMask,  int color,  int op_logic )
{
    int   row, col;
    int   ux0, uy0, ux1, uy1;
    int   row_max, col_max, row_min, col_min;
    int   trace = 0;
    double fdistmin, fdistx, fdisty;
    int   tstwrite = 0;
    int   distmin;

    if( aLayerMask[g_Route_Layer_BOTTOM] )
        trace = 1;       // Trace on BOTTOM

    if( aLayerMask[g_Route_Layer_TOP] )
        if( RoutingMatrix.m_RoutingLayersCount > 1 )
            trace |= 2;  // Trace on TOP

    if( trace == 0 )
        return;

    RoutingMatrix.SetCellOperation( op_logic );

    cx -= RoutingMatrix.GetBrdCoordOrigin().x;
    cy -= RoutingMatrix.GetBrdCoordOrigin().y;

    distmin = radius;

    // Calculate the bounding rectangle of the circle.
    ux0 = cx - radius;
    uy0 = cy - radius;
    ux1 = cx + radius;
    uy1 = cy + radius;

    // Calculate limit coordinates of cells belonging to the rectangle.
    row_max = uy1 / RoutingMatrix.m_GridRouting;
    col_max = ux1 / RoutingMatrix.m_GridRouting;
    row_min = uy0 / RoutingMatrix.m_GridRouting;  // if (uy0 > row_min*Board.m_GridRouting) row_min++;
    col_min = ux0 / RoutingMatrix.m_GridRouting;  // if (ux0 > col_min*Board.m_GridRouting) col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= (RoutingMatrix.m_Nrows - 1) )
        row_max = RoutingMatrix.m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= (RoutingMatrix.m_Ncols - 1) )
        col_max = RoutingMatrix.m_Ncols - 1;

    // Calculate coordinate limits of cell belonging to the rectangle.
    if( row_min > row_max )
        row_max = row_min;

    if( col_min > col_max )
        col_max = col_min;

    fdistmin = (double) distmin * distmin;

    for( row = row_min; row <= row_max; row++ )
    {
        fdisty  = (double) ( cy - ( row * RoutingMatrix.m_GridRouting ) );
        fdisty *= fdisty;

        for( col = col_min; col <= col_max; col++ )
        {
            fdistx  = (double) ( cx - ( col * RoutingMatrix.m_GridRouting ) );
            fdistx *= fdistx;

            if( fdistmin <= ( fdistx + fdisty ) )
                continue;

            if( trace & 1 )
                RoutingMatrix.WriteCell( row, col, BOTTOM, color );

            if( trace & 2 )
                RoutingMatrix.WriteCell( row, col, TOP, color );

            tstwrite = 1;
        }
    }

    if( tstwrite )
        return;

    /* If no cell has been written, it affects the 4 neighboring diagonal
     * (Adverse event: pad off grid in the center of the 4 neighboring
     * diagonal) */
    distmin  = RoutingMatrix.m_GridRouting / 2 + 1;
    fdistmin = ( (double) distmin * distmin ) * 2; // Distance to center point diagonally

    for( row = row_min; row <= row_max; row++ )
    {
        fdisty  = (double) ( cy - ( row * RoutingMatrix.m_GridRouting ) );
        fdisty *= fdisty;

        for( col = col_min; col <= col_max; col++ )
        {
            fdistx  = (double) ( cx - ( col * RoutingMatrix.m_GridRouting ) );
            fdistx *= fdistx;

            if( fdistmin <= ( fdistx + fdisty ) )
                continue;

            if( trace & 1 )
                RoutingMatrix.WriteCell( row, col, BOTTOM, color );

            if( trace & 2 )
                RoutingMatrix.WriteCell( row, col, TOP, color );
        }
    }
}

void TraceSegmentPcb( DRAWSEGMENT* pt_segm, int color, int marge, int op_logic )
{
    int half_width = ( pt_segm->GetWidth() / 2 ) + marge;

    // Calculate the bounding rectangle of the segment (if H, V or Via)
    int ux0 = pt_segm->GetStart().x - RoutingMatrix.GetBrdCoordOrigin().x;
    int uy0 = pt_segm->GetStart().y - RoutingMatrix.GetBrdCoordOrigin().y;
    int ux1 = pt_segm->GetEnd().x - RoutingMatrix.GetBrdCoordOrigin().x;
    int uy1 = pt_segm->GetEnd().y - RoutingMatrix.GetBrdCoordOrigin().y;

    LAYER_NUM layer = pt_segm->GetLayer();

    if( color == VIA_IMPOSSIBLE )
        layer = UNDEFINED_LAYER;

    switch( pt_segm->GetShape() )
    {
    // The segment is here a straight line or a circle or an arc.:
    case S_CIRCLE:
        TraceCircle( ux0, uy0, ux1, uy1, half_width, layer, color, op_logic );
        break;

    case S_ARC:
        TraceArc( ux0, uy0, ux1, uy1, pt_segm->GetAngle(), half_width, layer, color, op_logic );
        break;

    // The segment is here a line segment.
    default:
        DrawSegmentQcq( ux0, uy0, ux1, uy1, half_width, layer, color, op_logic );
        break;
    }
}

void TraceSegmentPcb( TRACK* aTrack, int color, int marge, int op_logic )
{
    int half_width = ( aTrack->GetWidth() / 2 ) + marge;

    // Test if VIA (filled circle need to be drawn)
    if( aTrack->Type() == PCB_VIA_T )
    {
        LSET layer_mask;

        if( aTrack->IsOnLayer( g_Route_Layer_BOTTOM ) )
            layer_mask.set( g_Route_Layer_BOTTOM );

        if( aTrack->IsOnLayer( g_Route_Layer_TOP ) )
        {
            if( !layer_mask.any() )
                layer_mask = LSET( g_Route_Layer_TOP );
            else
                layer_mask.set();
        }

        if( color == VIA_IMPOSSIBLE )
            layer_mask.set();

        if( layer_mask.any() )
            TraceFilledCircle( aTrack->GetStart().x, aTrack->GetStart().y,
                               half_width, layer_mask, color, op_logic );
    }
    else
    {
        // Calculate the bounding rectangle of the segment
        int ux0 = aTrack->GetStart().x - RoutingMatrix.GetBrdCoordOrigin().x;
        int uy0 = aTrack->GetStart().y - RoutingMatrix.GetBrdCoordOrigin().y;
        int ux1 = aTrack->GetEnd().x - RoutingMatrix.GetBrdCoordOrigin().x;
        int uy1 = aTrack->GetEnd().y - RoutingMatrix.GetBrdCoordOrigin().y;

        // Ordinary track
        LAYER_ID layer = aTrack->GetLayer();

        if( color == VIA_IMPOSSIBLE )
            layer = UNDEFINED_LAYER;

        DrawSegmentQcq( ux0, uy0, ux1, uy1, half_width, layer, color, op_logic );
    }
}


/* Draws a line, if layer = -1 on all layers
 */
void TracePcbLine( int x0, int y0, int x1, int y1, LAYER_NUM layer, int color, int op_logic  )
{
    int  dx, dy, lim;
    int  cumul, inc, il, delta;

    RoutingMatrix.SetCellOperation( op_logic );

    if( x0 == x1 )  // Vertical.
    {
        if( y1 < y0 )
            EXCHG( y0, y1 );

        dy  = y0 / RoutingMatrix.m_GridRouting;
        lim = y1 / RoutingMatrix.m_GridRouting;
        dx  = x0 / RoutingMatrix.m_GridRouting;

        // Clipping limits of board.
        if( ( dx < 0 ) || ( dx >= RoutingMatrix.m_Ncols ) )
            return;

        if( dy < 0 )
            dy = 0;

        if( lim >= RoutingMatrix.m_Nrows )
            lim = RoutingMatrix.m_Nrows - 1;

        for( ; dy <= lim; dy++ )
        {
            OP_CELL( layer, dy, dx );
        }

        return;
    }

    if( y0 == y1 )  // Horizontal
    {
        if( x1 < x0 )
            EXCHG( x0, x1 );

        dx  = x0 / RoutingMatrix.m_GridRouting;
        lim = x1 / RoutingMatrix.m_GridRouting;
        dy  = y0 / RoutingMatrix.m_GridRouting;

        // Clipping limits of board.
        if( ( dy < 0 ) || ( dy >= RoutingMatrix.m_Nrows ) )
            return;

        if( dx < 0 )
            dx = 0;

        if( lim >= RoutingMatrix.m_Ncols )
            lim = RoutingMatrix.m_Ncols - 1;

        for( ; dx <= lim; dx++ )
        {
            OP_CELL( layer, dy, dx );
        }

        return;
    }

    // Here is some perspective: using the algorithm LUCAS.
    if( abs( x1 - x0 ) >= abs( y1 - y0 ) ) // segment slightly inclined/
    {
        if( x1 < x0 )
        {
            EXCHG( x1, x0 ); EXCHG( y1, y0 );
        }

        dx  = x0 / RoutingMatrix.m_GridRouting;
        lim = x1 / RoutingMatrix.m_GridRouting;
        dy  = y0 / RoutingMatrix.m_GridRouting;
        inc = 1;

        if( y1 < y0 )
            inc = -1;

        il    = lim - dx; cumul = il / 2;
        delta = abs( y1 - y0 ) / RoutingMatrix.m_GridRouting;

        for( ; dx <= lim; )
        {
            if( ( dx >= 0 ) && ( dy >= 0 ) &&
                ( dx < RoutingMatrix.m_Ncols ) &&
                ( dy < RoutingMatrix.m_Nrows ) )
            {
                OP_CELL( layer, dy, dx );
            }

            dx++;
            cumul += delta;

            if( cumul > il )
            {
                cumul -= il;
                dy    += inc;
            }
        }
    }
    else
    {
        if( y1 < y0 )
        {
            EXCHG( x1, x0 );
            EXCHG( y1, y0 );
        }

        dy  = y0 / RoutingMatrix.m_GridRouting;
        lim = y1 / RoutingMatrix.m_GridRouting;
        dx  = x0 / RoutingMatrix.m_GridRouting;
        inc = 1;

        if( x1 < x0 )
            inc = -1;

        il    = lim - dy;
        cumul = il / 2;
        delta = abs( x1 - x0 ) / RoutingMatrix.m_GridRouting;

        for( ; dy <= lim; )
        {
            if( ( dx >= 0 ) && ( dy >= 0 ) && ( dx < RoutingMatrix.m_Ncols ) && ( dy < RoutingMatrix.m_Nrows ) )
            {
                OP_CELL( layer, dy, dx );
            }

            dy++;
            cumul += delta;

            if( cumul > il )
            {
                cumul -= il;
                dx    += inc;
            }
        }
    }
}


void TraceFilledRectangle( int ux0, int uy0, int ux1, int uy1,
                           LSET aLayerMask, int color, int op_logic )
{
    int  row, col;
    int  row_min, row_max, col_min, col_max;
    int  trace = 0;

    if( aLayerMask[g_Route_Layer_BOTTOM] )
        trace = 1;     // Trace on BOTTOM

    if( aLayerMask[g_Route_Layer_TOP] && RoutingMatrix.m_RoutingLayersCount > 1 )
        trace |= 2;    // Trace on TOP

    if( trace == 0 )
        return;

    RoutingMatrix.SetCellOperation( op_logic );

    ux0 -= RoutingMatrix.GetBrdCoordOrigin().x;
    uy0 -= RoutingMatrix.GetBrdCoordOrigin().y;
    ux1 -= RoutingMatrix.GetBrdCoordOrigin().x;
    uy1 -= RoutingMatrix.GetBrdCoordOrigin().y;

    // Calculating limits coord cells belonging to the rectangle.
    row_max = uy1 / RoutingMatrix.m_GridRouting;
    col_max = ux1 / RoutingMatrix.m_GridRouting;
    row_min = uy0 / RoutingMatrix.m_GridRouting;

    if( uy0 > row_min * RoutingMatrix.m_GridRouting )
        row_min++;

    col_min = ux0 / RoutingMatrix.m_GridRouting;

    if( ux0 > col_min * RoutingMatrix.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( RoutingMatrix.m_Nrows - 1 ) )
        row_max = RoutingMatrix.m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( RoutingMatrix.m_Ncols - 1 ) )
        col_max = RoutingMatrix.m_Ncols - 1;

    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            if( trace & 1 )
                RoutingMatrix.WriteCell( row, col, BOTTOM, color );

            if( trace & 2 )
                RoutingMatrix.WriteCell( row, col, TOP, color );
        }
    }
}


void TraceFilledRectangle( int ux0, int uy0, int ux1, int uy1,
                           double angle, LSET aLayerMask, int color, int op_logic )
{
    int  row, col;
    int  cx, cy;    // Center of rectangle
    int  radius;     // Radius of the circle
    int  row_min, row_max, col_min, col_max;
    int  rotrow, rotcol;
    int  trace = 0;

    if( aLayerMask[g_Route_Layer_BOTTOM] )
        trace = 1;     // Trace on BOTTOM

    if( aLayerMask[g_Route_Layer_TOP] )
    {
        if( RoutingMatrix.m_RoutingLayersCount > 1 )
            trace |= 2;  // Trace on TOP
    }

    if( trace == 0 )
        return;

    RoutingMatrix.SetCellOperation( op_logic );

    ux0 -= RoutingMatrix.GetBrdCoordOrigin().x;
    uy0 -= RoutingMatrix.GetBrdCoordOrigin().y;
    ux1 -= RoutingMatrix.GetBrdCoordOrigin().x;
    uy1 -= RoutingMatrix.GetBrdCoordOrigin().y;

    cx    = (ux0 + ux1) / 2;
    cy    = (uy0 + uy1) / 2;
    radius = KiROUND( Distance( ux0, uy0, cx, cy ) );

    // Calculating coordinate limits belonging to the rectangle.
    row_max = ( cy + radius ) / RoutingMatrix.m_GridRouting;
    col_max = ( cx + radius ) / RoutingMatrix.m_GridRouting;
    row_min = ( cy - radius ) / RoutingMatrix.m_GridRouting;

    if( uy0 > row_min * RoutingMatrix.m_GridRouting )
        row_min++;

    col_min = ( cx - radius ) / RoutingMatrix.m_GridRouting;

    if( ux0 > col_min * RoutingMatrix.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( RoutingMatrix.m_Nrows - 1 ) )
        row_max = RoutingMatrix.m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( RoutingMatrix.m_Ncols - 1 ) )
        col_max = RoutingMatrix.m_Ncols - 1;

    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            rotrow = row * RoutingMatrix.m_GridRouting;
            rotcol = col * RoutingMatrix.m_GridRouting;
            RotatePoint( &rotcol, &rotrow, cx, cy, -angle );

            if( rotrow <= uy0 )
                continue;

            if( rotrow >= uy1 )
                continue;

            if( rotcol <= ux0 )
                continue;

            if( rotcol >= ux1 )
                continue;

            if( trace & 1 )
                RoutingMatrix.WriteCell( row, col, BOTTOM, color );

            if( trace & 2 )
                RoutingMatrix.WriteCell( row, col, TOP, color );
        }
    }
}


/* Fills all cells inside a segment
 * half-width = lg, org = ux0,uy0 end = ux1,uy1
 * coordinates are in PCB units
 */
void DrawSegmentQcq( int ux0, int uy0, int ux1, int uy1, int lg, LAYER_NUM layer,
                     int color, int op_logic )
{
    int  row, col;
    int  inc;
    int  row_max, col_max, row_min, col_min;
    int  demi_pas;

    int  cx, cy, dx, dy;

    RoutingMatrix.SetCellOperation( op_logic );

    // Make coordinate ux1 tj > ux0 to simplify calculations
    if( ux1 < ux0 )
    {
        EXCHG( ux1, ux0 );
        EXCHG( uy1, uy0 );
    }

    // Calculating the incrementing the Y axis
    inc = 1;

    if( uy1 < uy0 )
        inc = -1;

    demi_pas = RoutingMatrix.m_GridRouting / 2;

    col_min = ( ux0 - lg ) / RoutingMatrix.m_GridRouting;

    if( col_min < 0 )
        col_min = 0;

    col_max = ( ux1 + lg + demi_pas ) / RoutingMatrix.m_GridRouting;

    if( col_max > ( RoutingMatrix.m_Ncols - 1 ) )
        col_max = RoutingMatrix.m_Ncols - 1;

    if( inc > 0 )
    {
        row_min = ( uy0 - lg ) / RoutingMatrix.m_GridRouting;
        row_max = ( uy1 + lg + demi_pas ) / RoutingMatrix.m_GridRouting;
    }
    else
    {
        row_min = ( uy1 - lg ) / RoutingMatrix.m_GridRouting;
        row_max = ( uy0 + lg + demi_pas ) / RoutingMatrix.m_GridRouting;
    }

    if( row_min < 0 )
        row_min = 0;

    if( row_min > ( RoutingMatrix.m_Nrows - 1 ) )
        row_min = RoutingMatrix.m_Nrows - 1;

    if( row_max < 0 )
        row_max = 0;

    if( row_max > ( RoutingMatrix.m_Nrows - 1 ) )
        row_max = RoutingMatrix.m_Nrows - 1;

    dx = ux1 - ux0;
    dy = uy1 - uy0;

    double angle;
    if( dx )
    {
        angle = ArcTangente( dy, dx );
    }
    else
    {
        angle = 900;

        if( dy < 0 )
            angle = -900;
    }

    RotatePoint( &dx, &dy, angle );   // dx = length, dy = 0

    for( col = col_min; col <= col_max; col++ )
    {
        int cxr;
        cxr = ( col * RoutingMatrix.m_GridRouting ) - ux0;

        for( row = row_min; row <= row_max; row++ )
        {
            cy = (row * RoutingMatrix.m_GridRouting) - uy0;
            cx = cxr;
            RotatePoint( &cx, &cy, angle );

            if( abs( cy ) > lg )
                continue;             // The point is too far on the Y axis.

            /* This point a test is close to the segment: the position
             * along the X axis must be tested.
             */
            if( ( cx >= 0 ) && ( cx <= dx ) )
            {
                OP_CELL( layer, row, col );
                continue;
            }

            // Examination of extremities are rounded.
            if( ( cx < 0 ) && ( cx >= -lg ) )
            {
                if( ( ( cx * cx ) + ( cy * cy ) ) <= ( lg * lg ) )
                    OP_CELL( layer, row, col );

                continue;
            }

            if( ( cx > dx ) && ( cx <= ( dx + lg ) ) )
            {
                if( ( ( ( cx - dx ) * ( cx - dx ) ) + ( cy * cy ) ) <= ( lg * lg ) )
                    OP_CELL( layer, row, col );

                continue;
            }
        }
    }
}


/* Fills all cells of the routing matrix contained in the circle
 * half-width = lg, center = ux0, uy0, ux1,uy1 is a point on the circle.
 * coord are in PCB units.
 */
void TraceCircle( int ux0, int uy0, int ux1, int uy1, int lg, LAYER_NUM layer,
                  int color, int op_logic )
{
    int radius, nb_segm;
    int x0, y0,             // Starting point of the current segment trace.
        x1, y1;             // End point.
    int ii;
    int angle;

    radius = KiROUND( Distance( ux0, uy0, ux1, uy1 ) );

    x0 = x1 = radius;
    y0 = y1 = 0;

    if( lg < 1 )
        lg = 1;

    nb_segm = ( 2 * radius ) / lg;

    if( nb_segm < 5 )
        nb_segm = 5;

    if( nb_segm > 100 )
        nb_segm = 100;

    for( ii = 1; ii < nb_segm; ii++ )
    {
        angle = (3600 * ii) / nb_segm;
        x1    = KiROUND( cosdecideg( radius, angle ) );
        y1    = KiROUND( sindecideg( radius, angle ) );
        DrawSegmentQcq( x0 + ux0, y0 + uy0, x1 + ux0, y1 + uy0, lg, layer, color, op_logic );
        x0 = x1;
        y0 = y1;
    }

    DrawSegmentQcq( x1 + ux0, y1 + uy0, ux0 + radius, uy0, lg, layer, color, op_logic );
}


/* Fills all routing matrix cells contained in the arc
 * angle = ArcAngle, half-width lg
 * center = ux0,uy0, starting at ux1, uy1.  Coordinates are in
 * PCB units.
 */
void TraceArc( int ux0, int uy0, int ux1, int uy1, double ArcAngle, int lg,
               LAYER_NUM layer, int color, int op_logic )
{
    int radius, nb_segm;
    int x0, y0,             // Starting point of the current segment trace
        x1, y1;             // End point
    int ii;
    double angle, StAngle;


    radius = KiROUND( Distance( ux0, uy0, ux1, uy1 ) );

    x0 = ux1 - ux0;
    y0 = uy1 - uy0;
    StAngle = ArcTangente( uy1 - uy0, ux1 - ux0 );

    if( lg < 1 )
        lg = 1;

    nb_segm = ( 2 * radius ) / lg;
    nb_segm = ( nb_segm * std::abs( ArcAngle ) ) / 3600;

    if( nb_segm < 5 )
        nb_segm = 5;

    if( nb_segm > 100 )
        nb_segm = 100;

    for( ii = 1; ii <= nb_segm; ii++ )
    {
        angle  = ( ArcAngle * ii ) / nb_segm;
        angle += StAngle;

        NORMALIZE_ANGLE_POS( angle );

        x1 = KiROUND( cosdecideg( radius, angle ) );
        y1 = KiROUND( cosdecideg( radius, angle ) );
        DrawSegmentQcq( x0 + ux0, y0 + uy0, x1 + ux0, y1 + uy0, lg, layer, color, op_logic );
        x0 = x1;
        y0 = y1;
    }
}

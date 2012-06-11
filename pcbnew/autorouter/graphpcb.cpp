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

#include <class_board.h>
#include <class_track.h>

#include <pcbnew.h>
#include <autorout.h>
#include <cell.h>

void TracePcbLine( int x0, int y0, int x1, int y1, int layer, int color );

void TraceArc( int ux0, int uy0,
               int ux1, int uy1,
               int ArcAngle,
               int lg, int layer, int color,
               int op_logic );


static void DrawSegmentQcq( int ux0, int uy0,
                            int ux1, int uy1,
                            int lg, int layer, int color,
                            int op_logic );

static void TraceFilledCircle( BOARD* aPcb,
                               int    cx, int cy, int radius,
                               int    aLayerMask,
                               int    color,
                               int    op_logic );

static void TraceCircle( int ux0, int uy0, int ux1, int uy1, int lg, int layer,
                         int color, int op_logic );

// Macro call to update cell.
#define OP_CELL( layer, dy, dx )                                        \
    {                                                                   \
        if( layer < 0 )                                                 \
        {                                                               \
            RoutingMatrix.WriteCell( dy, dx, BOTTOM, color );           \
            if( Nb_Sides )                                              \
                RoutingMatrix.WriteCell( dy, dx, TOP, color );                        \
        }                                                               \
        else                                                            \
        {                                                               \
            if( layer == Route_Layer_BOTTOM )                           \
                RoutingMatrix.WriteCell( dy, dx, BOTTOM, color );       \
            if( Nb_Sides )                                              \
                if( layer == Route_Layer_TOP )                          \
                    RoutingMatrix.WriteCell( dy, dx, TOP, color );      \
        }                                                               \
    }

void PlacePad( BOARD* aPcb, D_PAD* aPad, int color, int marge, int op_logic )
{
    int     dx, dy;
    wxPoint shape_pos = aPad->ReturnShapePos();

    dx = aPad->GetSize().x / 2;
    dx += marge;

    if( aPad->GetShape() == PAD_CIRCLE )
    {
        TraceFilledCircle( aPcb, shape_pos.x, shape_pos.y, dx,
                           aPad->GetLayerMask(), color, op_logic );
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

        TraceFilledRectangle( aPcb, shape_pos.x - dx, shape_pos.y - dy,
                              shape_pos.x + dx, shape_pos.y + dy,
                              aPad->GetLayerMask(), color, op_logic );
    }
    else
    {
        TraceFilledRectangle( aPcb, shape_pos.x - dx, shape_pos.y - dy,
                              shape_pos.x + dx, shape_pos.y + dy,
                              (int) aPad->GetOrientation(),
                              aPad->GetLayerMask(), color, op_logic );
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
void TraceFilledCircle( BOARD* aPcb,
                        int    cx, int cy, int radius,
                        int    aLayerMask,
                        int    color,
                        int    op_logic )
{
    int   row, col;
    int   ux0, uy0, ux1, uy1;
    int   row_max, col_max, row_min, col_min;
    int   trace = 0;
    double fdistmin, fdistx, fdisty;
    int   tstwrite = 0;
    int   distmin;

    if( aLayerMask & GetLayerMask( Route_Layer_BOTTOM ) )
        trace = 1;       // Trace on BOTTOM

    if( aLayerMask & GetLayerMask( Route_Layer_TOP ) )
        if( Nb_Sides )
            trace |= 2;  // Trace on TOP

    if( trace == 0 )
        return;

    RoutingMatrix.SetCellOperation( op_logic );

    cx -= aPcb->GetBoundingBox().GetX();
    cy -= aPcb->GetBoundingBox().GetY();

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


void TraceSegmentPcb( BOARD* aPcb, TRACK* pt_segm, int color, int marge, int op_logic )
{
    int half_width;
    int ux0, uy0, ux1, uy1;

    half_width = ( pt_segm->m_Width / 2 ) + marge;

    // Calculate the bounding rectangle of the segment (if H, V or Via)
    ux0 = pt_segm->m_Start.x - aPcb->GetBoundingBox().GetX();
    uy0 = pt_segm->m_Start.y - aPcb->GetBoundingBox().GetY();
    ux1 = pt_segm->m_End.x - aPcb->GetBoundingBox().GetX();
    uy1 = pt_segm->m_End.y - aPcb->GetBoundingBox().GetY();

    // Test if VIA (filled circle was drawn)
    if( pt_segm->Type() == PCB_VIA_T )
    {
        int mask_layer = 0;

        if( pt_segm->IsOnLayer( Route_Layer_BOTTOM ) )
            mask_layer = 1 << Route_Layer_BOTTOM;

        if( pt_segm->IsOnLayer( Route_Layer_TOP ) )
        {
            if( mask_layer == 0 )
                mask_layer = 1 << Route_Layer_TOP;
            else
                mask_layer = -1;
        }

        if( color == VIA_IMPOSSIBLE )
            mask_layer = -1;

        if( mask_layer )
            TraceFilledCircle( aPcb, pt_segm->m_Start.x, pt_segm->m_Start.y,
                               half_width, mask_layer, color, op_logic );
        return;
    }

    int layer = pt_segm->GetLayer();

    if( color == VIA_IMPOSSIBLE )
        layer = -1;

    // The segment is here a straight line or a circle or an arc.:
    if( pt_segm->m_Shape == S_CIRCLE )
    {
        TraceCircle( ux0, uy0, ux1, uy1, half_width, layer, color, op_logic );
        return;
    }

    if( pt_segm->m_Shape == S_ARC )
    {
        TraceArc( ux0, uy0, ux1, uy1, pt_segm->m_Param, half_width, layer, color, op_logic );
        return;
    }

    // The segment is here a line segment.
    if( ( ux0 != ux1 ) && ( uy0 != uy1 ) ) // Segment tilts.
    {
        DrawSegmentQcq( ux0, uy0, ux1, uy1, half_width, layer, color, op_logic );
        return;
    }

    // The segment is horizontal or vertical.
    // F4EXB 051018-01
    DrawSegmentQcq( ux0, uy0, ux1, uy1, half_width, layer, color, op_logic );
}


/* Draws a line, if layer = -1 on all layers
 */
void TracePcbLine( int x0, int y0, int x1, int y1, int layer, int color, int op_logic  )
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
            if( ( dx >= 0 ) && ( dy >= 0 ) && ( dx < RoutingMatrix.m_Ncols ) && ( dy < RoutingMatrix.m_Nrows ) )
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


void TraceFilledRectangle( BOARD* aPcb, int ux0, int uy0, int ux1, int uy1,
                           int aLayerMask, int color, int op_logic )
{
    int  row, col;
    int  row_min, row_max, col_min, col_max;
    int  trace = 0;

    if( ( aLayerMask & GetLayerMask( Route_Layer_BOTTOM ) ) )
        trace = 1;     // Trace on BOTTOM

    if( ( aLayerMask & GetLayerMask( Route_Layer_TOP ) ) && Nb_Sides )
        trace |= 2;    // Trace on TOP

    if( trace == 0 )
        return;

    RoutingMatrix.SetCellOperation( op_logic );

    ux0 -= aPcb->GetBoundingBox().GetX();
    uy0 -= aPcb->GetBoundingBox().GetY();
    ux1 -= aPcb->GetBoundingBox().GetX();
    uy1 -= aPcb->GetBoundingBox().GetY();

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


void TraceFilledRectangle( BOARD* aPcb, int ux0, int uy0, int ux1, int uy1,
                           int angle, int aLayerMask, int color, int op_logic )
{
    int  row, col;
    int  cx, cy;    // Center of rectangle
    int  radius;     // Radius of the circle
    int  row_min, row_max, col_min, col_max;
    int  rotrow, rotcol;
    int  trace = 0;

    if( aLayerMask & GetLayerMask( Route_Layer_BOTTOM ) )
        trace = 1;     // Trace on BOTTOM

    if( aLayerMask & GetLayerMask( Route_Layer_TOP ) )
    {
        if( Nb_Sides )
            trace |= 2;  // Trace on TOP
    }

    if( trace == 0 )
        return;

    RoutingMatrix.SetCellOperation( op_logic );

    ux0 -= aPcb->GetBoundingBox().GetX();
    uy0 -= aPcb->GetBoundingBox().GetY();
    ux1 -= aPcb->GetBoundingBox().GetX();
    uy1 -= aPcb->GetBoundingBox().GetY();

    cx    = (ux0 + ux1) / 2;
    cy    = (uy0 + uy1) / 2;
    radius = (int) sqrt( (double) ( cx - ux0 ) * ( cx - ux0 )
                       + (double) ( cy - uy0 ) * ( cy - uy0 ) );

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
void DrawSegmentQcq( int ux0, int uy0, int ux1, int uy1, int lg, int layer,
                     int color, int op_logic )
{
    int  row, col;
    int  inc;
    int  row_max, col_max, row_min, col_min;
    int  demi_pas;

    int  angle;
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

    if( dx )
    {
        angle = (int) ( atan2( (double) dy, (double) dx ) * 1800 / M_PI );
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
void TraceCircle( int ux0, int uy0, int ux1, int uy1, int lg, int layer,
                  int color, int op_logic )
{
    int radius, nb_segm;
    int x0, y0,             // Starting point of the current segment trace.
        x1, y1;             // End point.
    int ii;
    int angle;

    radius = (int) hypot( (double) (ux1 - ux0), (double) (uy1 - uy0) );

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
        x1    = (int) ( radius * cos( DEG2RAD( (double)angle / 10.0 ) ) );
        y1    = (int) ( radius * sin( DEG2RAD( (double)angle / 10.0 ) ) );
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
void TraceArc( int ux0, int uy0, int ux1, int uy1, int ArcAngle, int lg,
               int layer, int color, int op_logic )
{
    int radius, nb_segm;
    int x0, y0,             // Starting point of the current segment trace
        x1, y1;             // End point
    int ii;
    int angle, StAngle;


    radius = (int) hypot( (double) (ux1 - ux0), (double) (uy1 - uy0) );

    x0 = ux1 - ux0;
    y0 = uy1 - uy0;
    StAngle = ArcTangente( uy1 - uy0, ux1 - ux0 );

    if( lg < 1 )
        lg = 1;

    nb_segm = ( 2 * radius ) / lg;
    nb_segm = ( nb_segm * abs( ArcAngle ) ) / 3600;

    if( nb_segm < 5 )
        nb_segm = 5;

    if( nb_segm > 100 )
        nb_segm = 100;

    for( ii = 1; ii <= nb_segm; ii++ )
    {
        angle  = ( ArcAngle * ii ) / nb_segm;
        angle += StAngle;

        while( angle >= 3600 )
            angle -= 3600;

        while( angle < 0 )
            angle += 3600;

        x1 = (int) ( radius * cos( DEG2RAD( (double)angle / 10.0 ) ) );
        y1 = (int) ( radius * sin( DEG2RAD( (double)angle / 10.0 ) ) );
        DrawSegmentQcq( x0 + ux0, y0 + uy0, x1 + ux0, y1 + uy0, lg, layer, color, op_logic );
        x0 = x1;
        y0 = y1;
    }
}

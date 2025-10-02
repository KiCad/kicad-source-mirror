/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "ar_matrix.h"
#include <lset.h>
#include <math/util.h>      // for KiROUND
#include <trigo.h>

#include <pcb_shape.h>
#include <pad.h>


AR_MATRIX::AR_MATRIX()
{
    m_BoardSide[0]       = nullptr;
    m_BoardSide[1]       = nullptr;
    m_DistSide[0]        = nullptr;
    m_DistSide[1]        = nullptr;
    m_opWriteCell        = nullptr;
    m_Nrows              = 0;
    m_Ncols              = 0;
    m_MemSize            = 0;
    m_RoutingLayersCount = 1;
    m_GridRouting        = 0;
    m_RouteCount         = 0;
    m_routeLayerBottom   = B_Cu;
    m_routeLayerTop      = PADSTACK::ALL_LAYERS;
}


AR_MATRIX::~AR_MATRIX()
{
}

// was: bool AR_MATRIX::ComputeMatrixSize(  BOARD* aPcb, bool aUseBoardEdgesOnly )
// aUseBoardEdgesOnly ? aPcb->GetBoardEdgesBoundingBox() : aPcb->GetBoundingBox();

bool AR_MATRIX::ComputeMatrixSize( const BOX2I& aBoundingBox )
{
    // The boundary box must have its start point on routing grid:
    m_BrdBox = aBoundingBox;

    m_BrdBox.SetX( m_BrdBox.GetX() - ( m_BrdBox.GetX() % m_GridRouting ) );
    m_BrdBox.SetY( m_BrdBox.GetY() - ( m_BrdBox.GetY() % m_GridRouting ) );

    // The boundary box must have its end point on routing grid:
    VECTOR2I end = m_BrdBox.GetEnd();

    end.x -= end.x % m_GridRouting;
    end.x += m_GridRouting;

    end.y -= end.y % m_GridRouting;
    end.y += m_GridRouting;

    m_BrdBox.SetEnd( end );

    m_Nrows = m_BrdBox.GetHeight() / m_GridRouting;
    m_Ncols = m_BrdBox.GetWidth() / m_GridRouting;

    // gives a small margin
    m_Ncols += 1;
    m_Nrows += 1;

    return true;
}


int AR_MATRIX::InitRoutingMatrix()
{
    if( m_Nrows <= 0 || m_Ncols <= 0 )
        return 0;

    // give a small margin for memory allocation:
    int ii = ( m_Nrows + 1 ) * ( m_Ncols + 1 );

    int side = AR_SIDE_BOTTOM;
    for( int jj = 0; jj < m_RoutingLayersCount; jj++ ) // m_RoutingLayersCount = 1 or 2
    {
        m_BoardSide[side] = nullptr;
        m_DistSide[side]  = nullptr;

        // allocate matrix & initialize everything to empty
        m_BoardSide[side] = new MATRIX_CELL[ ii * sizeof( MATRIX_CELL ) ];
        memset( m_BoardSide[side], 0, ii * sizeof( MATRIX_CELL ) );

        if( m_BoardSide[side] == nullptr )
            return -1;

        // allocate Distances
        m_DistSide[side] = new DIST_CELL[ ii * sizeof( DIST_CELL ) ];
        memset( m_DistSide[side], 0, ii * sizeof( DIST_CELL ) );

        if( m_DistSide[side] == nullptr )
            return -1;

        side = AR_SIDE_TOP;
    }

    m_MemSize = m_RouteCount * ii * ( sizeof( MATRIX_CELL ) + sizeof( DIST_CELL ) );

    return m_MemSize;
}


void AR_MATRIX::UnInitRoutingMatrix()
{
    int ii;

    for( ii = 0; ii < AR_MAX_ROUTING_LAYERS_COUNT; ii++ )
    {
        // de-allocate Distances matrix
        if( m_DistSide[ii] )
        {
            delete[] m_DistSide[ii];
            m_DistSide[ii] = nullptr;
        }

        // de-allocate cells matrix
        if( m_BoardSide[ii] )
        {
            delete[] m_BoardSide[ii];
            m_BoardSide[ii] = nullptr;
        }
    }

    m_Nrows = m_Ncols = 0;
}

// Initialize m_opWriteCell member to make the aLogicOp
void AR_MATRIX::SetCellOperation( AR_MATRIX::CELL_OP aLogicOp )
{
    switch( aLogicOp )
    {
    default:
    case WRITE_CELL:     m_opWriteCell = &AR_MATRIX::SetCell; break;
    case WRITE_OR_CELL:  m_opWriteCell = &AR_MATRIX::OrCell;  break;
    case WRITE_XOR_CELL: m_opWriteCell = &AR_MATRIX::XorCell; break;
    case WRITE_AND_CELL: m_opWriteCell = &AR_MATRIX::AndCell; break;
    case WRITE_ADD_CELL: m_opWriteCell = &AR_MATRIX::AddCell; break;
    }
}


/* return the value stored in a cell
 */
AR_MATRIX::MATRIX_CELL AR_MATRIX::GetCell( int aRow, int aCol, int aSide )
{
    MATRIX_CELL* p;

    p = m_BoardSide[aSide];
    return p[aRow * m_Ncols + aCol];
}


/* basic cell operation : WRITE operation
 */
void AR_MATRIX::SetCell( int aRow, int aCol, int aSide, MATRIX_CELL x )
{
    MATRIX_CELL* p;

    p = m_BoardSide[aSide];
    p[aRow * m_Ncols + aCol] = x;
}


/* basic cell operation : OR operation
 */
void AR_MATRIX::OrCell( int aRow, int aCol, int aSide, MATRIX_CELL x )
{
    MATRIX_CELL* p;

    p = m_BoardSide[aSide];
    p[aRow * m_Ncols + aCol] |= x;
}


/* basic cell operation : XOR operation
 */
void AR_MATRIX::XorCell( int aRow, int aCol, int aSide, MATRIX_CELL x )
{
    MATRIX_CELL* p;

    p = m_BoardSide[aSide];
    p[aRow * m_Ncols + aCol] ^= x;
}


/* basic cell operation : AND operation
 */
void AR_MATRIX::AndCell( int aRow, int aCol, int aSide, MATRIX_CELL x )
{
    MATRIX_CELL* p;

    p = m_BoardSide[aSide];
    p[aRow * m_Ncols + aCol] &= x;
}


/* basic cell operation : ADD operation
 */
void AR_MATRIX::AddCell( int aRow, int aCol, int aSide, MATRIX_CELL x )
{
    MATRIX_CELL* p;

    p = m_BoardSide[aSide];
    p[aRow * m_Ncols + aCol] += x;
}


// fetch distance cell
AR_MATRIX::DIST_CELL AR_MATRIX::GetDist( int aRow, int aCol, int aSide ) // fetch distance cell
{
    DIST_CELL* p;

    p = m_DistSide[aSide];
    return p[aRow * m_Ncols + aCol];
}


// store distance cell
void AR_MATRIX::SetDist( int aRow, int aCol, int aSide, DIST_CELL x )
{
    DIST_CELL* p;

    p = m_DistSide[aSide];
    p[aRow * m_Ncols + aCol] = x;
}


/*
** x is the direction to enter the cell of interest.
** y is the direction to exit the cell of interest.
** z is the direction to really exit the cell, if y=FROM_OTHERSIDE.
**
** return the distance of the trace through the cell of interest.
** the calculation is driven by the tables above.
*/


#define OP_CELL( layer, dy, dx )                             \
    {                                                        \
        if( layer == UNDEFINED_LAYER )                       \
        {                                                    \
            WriteCell( dy, dx, AR_SIDE_BOTTOM, color );      \
            if( m_RoutingLayersCount > 1 )                   \
                WriteCell( dy, dx, AR_SIDE_TOP, color );     \
        }                                                    \
        else                                                 \
        {                                                    \
            if( layer == m_routeLayerBottom )                \
                WriteCell( dy, dx, AR_SIDE_BOTTOM, color );  \
            if( m_RoutingLayersCount > 1 )                   \
                if( layer == m_routeLayerTop )               \
                    WriteCell( dy, dx, AR_SIDE_TOP, color ); \
        }                                                    \
    }

/* Fills all cells inside a segment
 * half-width = lg, org = ux0,uy0 end = ux1,uy1
 * coordinates are in PCB units
 */
void AR_MATRIX::drawSegmentQcq( int ux0, int uy0, int ux1, int uy1, int lg, int layer, int color,
                                AR_MATRIX::CELL_OP op_logic )
{
    int64_t row, col;
    int64_t inc;
    int64_t row_max, col_max, row_min, col_min;
    int64_t demi_pas;

    int cx, cy, dx, dy;

    SetCellOperation( op_logic );

    // Make coordinate ux1 tj > ux0 to simplify calculations
    if( ux1 < ux0 )
    {
        std::swap( ux1, ux0 );
        std::swap( uy1, uy0 );
    }

    // Calculating the incrementing the Y axis
    inc = 1;

    if( uy1 < uy0 )
        inc = -1;

    demi_pas = m_GridRouting / 2;

    col_min = ( ux0 - lg ) / m_GridRouting;

    if( col_min < 0 )
        col_min = 0;

    col_max = ( ux1 + lg + demi_pas ) / m_GridRouting;

    if( col_max > ( m_Ncols - 1 ) )
        col_max = m_Ncols - 1;

    if( inc > 0 )
    {
        row_min = ( uy0 - lg ) / m_GridRouting;
        row_max = ( uy1 + lg + demi_pas ) / m_GridRouting;
    }
    else
    {
        row_min = ( uy1 - lg ) / m_GridRouting;
        row_max = ( uy0 + lg + demi_pas ) / m_GridRouting;
    }

    if( row_min < 0 )
        row_min = 0;

    if( row_min > ( m_Nrows - 1 ) )
        row_min = m_Nrows - 1;

    if( row_max < 0 )
        row_max = 0;

    if( row_max > ( m_Nrows - 1 ) )
        row_max = m_Nrows - 1;

    dx = ux1 - ux0;
    dy = uy1 - uy0;

    EDA_ANGLE angle( VECTOR2I( dx, dy ) );

    RotatePoint( &dx, &dy, angle ); // dx = length, dy = 0

    for( col = col_min; col <= col_max; col++ )
    {
        int64_t cxr;
        cxr = ( col * m_GridRouting ) - ux0;

        for( row = row_min; row <= row_max; row++ )
        {
            cy = ( row * m_GridRouting ) - uy0;
            cx = cxr;
            RotatePoint( &cx, &cy, angle );

            if( abs( cy ) > lg )
                continue; // The point is too far on the Y axis.

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
void AR_MATRIX::traceCircle( int ux0, int uy0, int ux1, int uy1, int lg, int layer, int color,
                             AR_MATRIX::CELL_OP op_logic )
{
    int radius, nb_segm;
    int x0, y0;     // Starting point of the current segment trace.
    int x1, y1;     // End point.
    int ii;

    VECTOR2I pt1( ux0, uy0 );
    VECTOR2I pt2( ux1, uy1 );

    radius = pt1.Distance( pt2 );

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
        EDA_ANGLE angle = ( ANGLE_360 * ii ) / nb_segm;
        x1 = KiROUND( radius * angle.Cos() );
        y1 = KiROUND( radius * angle.Sin() );
        drawSegmentQcq( x0 + ux0, y0 + uy0, x1 + ux0, y1 + uy0, lg, layer, color, op_logic );
        x0 = x1;
        y0 = y1;
    }

    drawSegmentQcq( x1 + ux0, y1 + uy0, ux0 + radius, uy0, lg, layer, color, op_logic );
}


void AR_MATRIX::traceFilledCircle( int cx, int cy, int radius, const LSET& aLayerMask, int color,
                                   AR_MATRIX::CELL_OP op_logic )
{
    int    row, col;
    int    ux0, uy0, ux1, uy1;
    int    row_max, col_max, row_min, col_min;
    int    trace = 0;
    double fdistmin, fdistx, fdisty;
    int    tstwrite = 0;
    int    distmin;

    if( aLayerMask[m_routeLayerBottom] )
        trace = 1; // Trace on BOTTOM

    if( aLayerMask[m_routeLayerTop] )
        if( m_RoutingLayersCount > 1 )
            trace |= 2; // Trace on TOP

    if( trace == 0 )
        return;

    SetCellOperation( op_logic );

    cx -= GetBrdCoordOrigin().x;
    cy -= GetBrdCoordOrigin().y;

    distmin = radius;

    // Calculate the bounding rectangle of the circle.
    ux0 = cx - radius;
    uy0 = cy - radius;
    ux1 = cx + radius;
    uy1 = cy + radius;

    // Calculate limit coordinates of cells belonging to the rectangle.
    row_max = uy1 / m_GridRouting;
    col_max = ux1 / m_GridRouting;
    row_min = uy0 / m_GridRouting; // if (uy0 > row_min*Board.m_GridRouting) row_min++;
    col_min = ux0 / m_GridRouting; // if (ux0 > col_min*Board.m_GridRouting) col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( m_Nrows - 1 ) )
        row_max = m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( m_Ncols - 1 ) )
        col_max = m_Ncols - 1;

    // Calculate coordinate limits of cell belonging to the rectangle.
    if( row_min > row_max )
        row_max = row_min;

    if( col_min > col_max )
        col_max = col_min;

    fdistmin = (double) distmin * distmin;

    for( row = row_min; row <= row_max; row++ )
    {
        fdisty = (double) ( cy - ( row * m_GridRouting ) );
        fdisty *= fdisty;

        for( col = col_min; col <= col_max; col++ )
        {
            fdistx = (double) ( cx - ( col * m_GridRouting ) );
            fdistx *= fdistx;

            if( fdistmin <= ( fdistx + fdisty ) )
                continue;

            if( trace & 1 )
                WriteCell( row, col, AR_SIDE_BOTTOM, color );

            if( trace & 2 )
                WriteCell( row, col, AR_SIDE_TOP, color );

            tstwrite = 1;
        }
    }

    if( tstwrite )
        return;

    /* If no cell has been written, it affects the 4 neighboring diagonal
     * (Adverse event: pad off grid in the center of the 4 neighboring
     * diagonal) */
    distmin = m_GridRouting / 2 + 1;
    fdistmin = ( (double) distmin * distmin ) * 2; // Distance to center point diagonally

    for( row = row_min; row <= row_max; row++ )
    {
        fdisty = (double) ( cy - ( row * m_GridRouting ) );
        fdisty *= fdisty;

        for( col = col_min; col <= col_max; col++ )
        {
            fdistx = (double) ( cx - ( col * m_GridRouting ) );
            fdistx *= fdistx;

            if( fdistmin <= ( fdistx + fdisty ) )
                continue;

            if( trace & 1 )
                WriteCell( row, col, AR_SIDE_BOTTOM, color );

            if( trace & 2 )
                WriteCell( row, col, AR_SIDE_TOP, color );
        }
    }
}


/* Fills all routing matrix cells contained in the arc
 * angle = ArcAngle, half-width lg
 * center = ux0,uy0, starting at ux1, uy1.  Coordinates are in
 * PCB units.
 */
void AR_MATRIX::traceArc( int ux0, int uy0, int ux1, int uy1, const EDA_ANGLE& arcAngle, int lg,
                          int layer, int color, AR_MATRIX::CELL_OP op_logic )
{
    int       radius, nb_segm;
    int       x0, y0;     // Starting point of the current segment trace
    int       x1, y1;     // End point
    int       ii;
    EDA_ANGLE angle, startAngle;

    VECTOR2I pt1( ux0, uy0 );
    VECTOR2I pt2( ux1, uy1 );

    radius = pt1.Distance( pt2 );

    x0 = ux1 - ux0;
    y0 = uy1 - uy0;
    startAngle = EDA_ANGLE( VECTOR2I( ux1, uy1 ) - VECTOR2I( ux0, uy0 ) );

    if( lg < 1 )
        lg = 1;

    nb_segm = ( 2 * radius ) / lg;
    nb_segm = nb_segm * std::abs( arcAngle.AsDegrees() ) / 360.0;

    if( nb_segm < 5 )
        nb_segm = 5;

    if( nb_segm > 100 )
        nb_segm = 100;

    for( ii = 1; ii <= nb_segm; ii++ )
    {
        angle = arcAngle * ii / nb_segm;
        angle += startAngle;

        angle.Normalize();

        x1 = KiROUND( radius * angle.Cos() );
        y1 = KiROUND( radius * angle.Cos() );
        drawSegmentQcq( x0 + ux0, y0 + uy0, x1 + ux0, y1 + uy0, lg, layer, color, op_logic );
        x0 = x1;
        y0 = y1;
    }
}


void AR_MATRIX::TraceFilledRectangle( int ux0, int uy0, int ux1, int uy1, double angle,
                                      const LSET& aLayerMask, int color, AR_MATRIX::CELL_OP op_logic )
{
    int row, col;
    int cx, cy; // Center of rectangle
    int radius; // Radius of the circle
    int row_min, row_max, col_min, col_max;
    int rotrow, rotcol;
    int trace = 0;

    if( aLayerMask[m_routeLayerBottom] )
        trace = 1; // Trace on BOTTOM

    if( aLayerMask[m_routeLayerTop] )
    {
        if( m_RoutingLayersCount > 1 )
            trace |= 2; // Trace on TOP
    }

    if( trace == 0 )
        return;

    SetCellOperation( op_logic );

    ux0 -= GetBrdCoordOrigin().x;
    uy0 -= GetBrdCoordOrigin().y;
    ux1 -= GetBrdCoordOrigin().x;
    uy1 -= GetBrdCoordOrigin().y;

    cx = ( ux0 + ux1 ) / 2;
    cy = ( uy0 + uy1 ) / 2;
    VECTOR2I pt1( ux0, uy0 );
    VECTOR2I pt2( cx, cy );
    radius = pt1.Distance( pt2 );

    // Calculating coordinate limits belonging to the rectangle.
    row_max = ( cy + radius ) / m_GridRouting;
    col_max = ( cx + radius ) / m_GridRouting;
    row_min = ( cy - radius ) / m_GridRouting;

    if( uy0 > row_min * m_GridRouting )
        row_min++;

    col_min = ( cx - radius ) / m_GridRouting;

    if( ux0 > col_min * m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( m_Nrows - 1 ) )
        row_max = m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( m_Ncols - 1 ) )
        col_max = m_Ncols - 1;

    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            rotrow = row * m_GridRouting;
            rotcol = col * m_GridRouting;
            RotatePoint( &rotcol, &rotrow, cx, cy, -EDA_ANGLE( angle, TENTHS_OF_A_DEGREE_T ) );

            if( rotrow <= uy0 )
                continue;

            if( rotrow >= uy1 )
                continue;

            if( rotcol <= ux0 )
                continue;

            if( rotcol >= ux1 )
                continue;

            if( trace & 1 )
                WriteCell( row, col, AR_SIDE_BOTTOM, color );

            if( trace & 2 )
                WriteCell( row, col, AR_SIDE_TOP, color );
        }
    }
}


void AR_MATRIX::TraceFilledRectangle( int ux0, int uy0, int ux1, int uy1, const LSET& aLayerMask,
                                      int color, AR_MATRIX::CELL_OP op_logic )
{
    int row, col;
    int row_min, row_max, col_min, col_max;
    int trace = 0;

    if( aLayerMask[m_routeLayerBottom] )
        trace = 1; // Trace on BOTTOM

    if( aLayerMask[m_routeLayerTop] && m_RoutingLayersCount > 1 )
        trace |= 2; // Trace on TOP

    if( trace == 0 )
        return;

    SetCellOperation( op_logic );

    ux0 -= GetBrdCoordOrigin().x;
    uy0 -= GetBrdCoordOrigin().y;
    ux1 -= GetBrdCoordOrigin().x;
    uy1 -= GetBrdCoordOrigin().y;

    // Calculating limits coord cells belonging to the rectangle.
    row_max = uy1 / m_GridRouting;
    col_max = ux1 / m_GridRouting;
    row_min = uy0 / m_GridRouting;

    if( uy0 > row_min * m_GridRouting )
        row_min++;

    col_min = ux0 / m_GridRouting;

    if( ux0 > col_min * m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( m_Nrows - 1 ) )
        row_max = m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( m_Ncols - 1 ) )
        col_max = m_Ncols - 1;

    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            if( trace & 1 )
                WriteCell( row, col, AR_SIDE_BOTTOM, color );

            if( trace & 2 )
                WriteCell( row, col, AR_SIDE_TOP, color );
        }
    }
}


void AR_MATRIX::TracePcbShape( PCB_SHAPE* aShape, int aColor, int aMargin, AR_MATRIX::CELL_OP op_logic )
{
    int half_width = ( aShape->GetWidth() / 2 ) + aMargin;

    // Calculate the bounding rectangle of the segment (if H, V or Via)
    int layer = UNDEFINED_LAYER;    // Draw on all layers

    if( aShape->GetShape() == SHAPE_T::CIRCLE || aShape->GetShape() == SHAPE_T::SEGMENT )
    {
        int ux0 = aShape->GetStart().x - GetBrdCoordOrigin().x;
        int uy0 = aShape->GetStart().y - GetBrdCoordOrigin().y;
        int ux1 = aShape->GetEnd().x - GetBrdCoordOrigin().x;
        int uy1 = aShape->GetEnd().y - GetBrdCoordOrigin().y;

        if( aShape->GetShape() == SHAPE_T::CIRCLE )
            traceCircle( ux0, uy0, ux1, uy1, half_width, layer, aColor, op_logic );
        else
            drawSegmentQcq( ux0, uy0, ux1, uy1, half_width, layer, aColor, op_logic );
    }
    else if( aShape->GetShape() == SHAPE_T::ARC )
    {
        int ux0 = aShape->GetCenter().x - GetBrdCoordOrigin().x;
        int uy0 = aShape->GetCenter().y - GetBrdCoordOrigin().y;
        int ux1 = aShape->GetStart().x - GetBrdCoordOrigin().x;
        int uy1 = aShape->GetStart().y - GetBrdCoordOrigin().y;

        traceArc( ux0, uy0, ux1, uy1, aShape->GetArcAngle(), half_width, layer, aColor, op_logic );
    }
}


/**
 * Function CreateKeepOutRectangle
 * builds the cost map:
 * Cells ( in Dist map ) inside the rect x0,y0 a x1,y1 are
 *  incremented by value aKeepOut
 *  Cell outside this rectangle, but inside the rectangle
 *  x0,y0 -marge to x1,y1 + marge are incremented by a decreasing value
 *  (aKeepOut ... 0). The decreasing value depends on the distance to the first rectangle
 *  Therefore the cost is high in rect x0,y0 to x1,y1, and decrease outside this rectangle
 */
void AR_MATRIX::CreateKeepOutRectangle( int ux0, int uy0, int ux1, int uy1, int marge,
                                        int aKeepOut, const LSET& aLayerMask )
{
    int       row, col;
    int       row_min, row_max, col_min, col_max, pmarge;
    int       trace = 0;
    DIST_CELL data, LocalKeepOut;
    int       lgain, cgain;

    if( aLayerMask[m_routeLayerBottom] )
        trace = 1; // Trace on bottom layer.

    if( aLayerMask[m_routeLayerTop] && m_RoutingLayersCount )
        trace |= 2; // Trace on top layer.

    if( trace == 0 )
        return;

    ux0 -= m_BrdBox.GetX();
    uy0 -= m_BrdBox.GetY();
    ux1 -= m_BrdBox.GetX();
    uy1 -= m_BrdBox.GetY();

    ux0 -= marge;
    ux1 += marge;
    uy0 -= marge;
    uy1 += marge;

    pmarge = marge / m_GridRouting;

    if( pmarge < 1 )
        pmarge = 1;

    // Calculate the coordinate limits of the rectangle.
    row_max = uy1 / m_GridRouting;
    col_max = ux1 / m_GridRouting;
    row_min = uy0 / m_GridRouting;

    if( uy0 > row_min * m_GridRouting )
        row_min++;

    col_min = ux0 / m_GridRouting;

    if( ux0 > col_min * m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( m_Nrows - 1 ) )
        row_max = m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( m_Ncols - 1 ) )
        col_max = m_Ncols - 1;

    for( row = row_min; row <= row_max; row++ )
    {
        lgain = 256;

        if( row < pmarge )
            lgain = ( 256 * row ) / pmarge;
        else if( row > row_max - pmarge )
            lgain = ( 256 * ( row_max - row ) ) / pmarge;

        for( col = col_min; col <= col_max; col++ )
        {
            // RoutingMatrix Dist map contained the "cost" of the cell
            // at position (row, col)
            // in autoplace this is the cost of the cell, when
            // a footprint overlaps it, near a "master" footprint
            // this cost is high near the "master" footprint
            // and decrease with the distance
            cgain = 256;
            LocalKeepOut = aKeepOut;

            if( col < pmarge )
                cgain = ( 256 * col ) / pmarge;
            else if( col > col_max - pmarge )
                cgain = ( 256 * ( col_max - col ) ) / pmarge;

            cgain = ( cgain * lgain ) / 256;

            if( cgain != 256 )
                LocalKeepOut = ( LocalKeepOut * cgain ) / 256;

            if( trace & 1 )
            {
                data = GetDist( row, col, AR_SIDE_BOTTOM ) + LocalKeepOut;
                SetDist( row, col, AR_SIDE_BOTTOM, data );
            }

            if( trace & 2 )
            {
                data = GetDist( row, col, AR_SIDE_TOP );
                data = std::max( data, LocalKeepOut );
                SetDist( row, col, AR_SIDE_TOP, data );
            }
        }
    }
}


void AR_MATRIX::PlacePad( PAD* aPad, int color, int marge, AR_MATRIX::CELL_OP op_logic )
{
    int     dx, dy;
    VECTOR2I shape_pos = aPad->ShapePos( PADSTACK::ALL_LAYERS );

    // TODO(JE) padstacks
    dx = aPad->GetSize( PADSTACK::ALL_LAYERS ).x / 2;
    dx += marge;

    if( aPad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CIRCLE )
    {
        traceFilledCircle( shape_pos.x, shape_pos.y, dx, aPad->GetLayerSet(), color, op_logic );
        return;
    }

    dy = aPad->GetSize( PADSTACK::ALL_LAYERS ).y / 2;
    dy += marge;

    if( aPad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::TRAPEZOID )
    {
        dx += abs( aPad->GetDelta( PADSTACK::ALL_LAYERS ).y ) / 2;
        dy += abs( aPad->GetDelta( PADSTACK::ALL_LAYERS ).x ) / 2;
    }

    // The pad is a rectangle ( horizontal or vertical )
    if( aPad->GetOrientation().IsCardinal() )
    {
        // Orientation turned 90 deg.
        if( aPad->GetOrientation() == ANGLE_90 || aPad->GetOrientation() == ANGLE_270 )
        {
            std::swap( dx, dy );
        }

        TraceFilledRectangle( shape_pos.x - dx, shape_pos.y - dy, shape_pos.x + dx,
                shape_pos.y + dy, aPad->GetLayerSet(), color, op_logic );
    }
    else
    {
        TraceFilledRectangle( shape_pos.x - dx, shape_pos.y - dy, shape_pos.x + dx,
                              shape_pos.y + dy, aPad->GetOrientation().AsTenthsOfADegree(),
                              aPad->GetLayerSet(), color, op_logic );
    }
}

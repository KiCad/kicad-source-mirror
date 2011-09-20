/**
 * @file graphpcb.cpp
 * @brief PCB editor autorouting and "graphics" routines.
 */

#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"
#include "trigo.h"
#include "cell.h"
#include "ar_protos.h"

int  ToMatrixCoordinate( int aPhysicalCoordinate );

void TracePcbLine( int x0, int y0, int x1, int y1, int layer, int color );

void TraceArc( int ux0,
               int uy0,
               int ux1,
               int uy1,
               int ArcAngle,
               int lg,
               int layer,
               int color,
               int op_logic );


static void DrawSegmentQcq( int ux0,
                            int uy0,
                            int ux1,
                            int uy1,
                            int lg,
                            int layer,
                            int color,
                            int op_logic );

static void TraceFilledCircle( BOARD* Pcb,
                               int    cx,
                               int    cy,
                               int    radius,
                               int    aLayerMask,
                               int    color,
                               int    op_logic );

static void TraceCircle( int ux0, int uy0, int ux1, int uy1, int lg, int layer,
                         int color, int op_logic );

/* Macro call to update cell. */
#define OP_CELL( layer, dy, dx )                                        \
    {                                                                   \
        if( layer < 0 )                                                 \
        {                                                               \
            WriteCell( dy, dx, BOTTOM, color );                         \
            if( Nb_Sides )                                              \
                WriteCell( dy, dx, TOP, color );                        \
        }                                                               \
        else                                                            \
        {                                                               \
            if( layer == Route_Layer_BOTTOM )                           \
                WriteCell( dy, dx, BOTTOM, color );                     \
            if( Nb_Sides )                                              \
                if( layer == Route_Layer_TOP )                          \
                    WriteCell( dy, dx, TOP, color );                    \
        }                                                               \
    }


/**
 * Function ToMatrixCoordinate
 * compute the coordinate in the routing matrix from the real (board) value
 * @param aPhysicalCoordinate = value to convert
 * @return the coordinate relative to the matrix
 */
int ToMatrixCoordinate( int aPhysicalCoordinate )
{
    return aPhysicalCoordinate / Board.m_GridRouting;
}


void PlacePad( BOARD* Pcb, D_PAD* pt_pad, int color, int marge, int op_logic )
{
    int     dx, dy;
    wxPoint shape_pos = pt_pad->ReturnShapePos();

    dx = pt_pad->m_Size.x / 2;
    dx += marge;

    if( pt_pad->m_PadShape == PAD_CIRCLE )
    {
        TraceFilledCircle( Pcb, shape_pos.x, shape_pos.y, dx,
                           pt_pad->m_layerMask, color, op_logic );
        return;
    }


    dy = pt_pad->m_Size.y / 2;
    dy += marge;

    if( pt_pad->m_PadShape == PAD_TRAPEZOID )
    {
        dx += abs( pt_pad->m_DeltaSize.y ) / 2;
        dy += abs( pt_pad->m_DeltaSize.x ) / 2;
    }

    if( ( pt_pad->m_Orient % 900 ) == 0 ) /* The pad is a rectangle
                                           * horizontally or vertically. */
    {
        /* Orientation turned 90 deg. */
        if( ( pt_pad->m_Orient == 900 ) || ( pt_pad->m_Orient == 2700 ) )
        {
            EXCHG( dx, dy );
        }

        TraceFilledRectangle( Pcb, shape_pos.x - dx, shape_pos.y - dy,
                              shape_pos.x + dx, shape_pos.y + dy,
                              pt_pad->m_layerMask, color, op_logic );
    }
    else
    {
        TraceFilledRectangle( Pcb, shape_pos.x - dx, shape_pos.y - dy,
                              shape_pos.x + dx, shape_pos.y + dy,
                              (int) pt_pad->m_Orient,
                              pt_pad->m_layerMask, color, op_logic );
    }
}


/* Initialize a color value, the cells included in the board area of the
 * circle center cx, cy.
 * Parameters:
 * radius: a value add to the radius or half the score pad
 * aLayerMask: layer occupied
 * color: mask write in cells
 * op_logic: type of writing in the cell (WRITE, OR)
 */
void TraceFilledCircle( BOARD* Pcb,
                        int    cx,
                        int    cy,
                        int    radius,
                        int    aLayerMask,
                        int    color,
                        int    op_logic )
{
    int   row, col;
    int   ux0, uy0, ux1, uy1;
    int   row_max, col_max, row_min, col_min;
    int   trace = 0;
    float fdistmin, fdistx, fdisty;

    void  (* WriteCell)( int, int, int, MATRIX_CELL );
    int   tstwrite = 0;
    int   distmin;

    /* Determine occupied layer. */

    /* Single routing layer on bitmap and BOTTOM
     * Route_Layer_B = Route_Layer_A */

    if( aLayerMask & g_TabOneLayerMask[Route_Layer_BOTTOM] )
        trace = 1;       /* Trace on BOTTOM */

    if( aLayerMask & g_TabOneLayerMask[Route_Layer_TOP] )
        if( Nb_Sides )
            trace |= 2;  /* Trace on TOP */

    if( trace == 0 )
        return;

    switch( op_logic )
    {
    default:
    case WRITE_CELL:
        WriteCell = SetCell;
        break;

    case WRITE_OR_CELL:
        WriteCell = OrCell;
        break;

    case WRITE_XOR_CELL:
        WriteCell = XorCell;
        break;

    case WRITE_AND_CELL:
        WriteCell = AndCell;
        break;

    case WRITE_ADD_CELL:
        WriteCell = AddCell;
        break;
    }

    cx -= Pcb->m_BoundaryBox.m_Pos.x;
    cy -= Pcb->m_BoundaryBox.m_Pos.y;

    distmin = radius;

    /* Calculate the bounding rectangle of the circle. */
    ux0 = cx - radius;
    uy0 = cy - radius;
    ux1 = cx + radius;
    uy1 = cy + radius;

    /* Calculate limit coordinates of cells belonging to the rectangle. */
    row_max = uy1 / Board.m_GridRouting;
    col_max = ux1 / Board.m_GridRouting;
    row_min = uy0 / Board.m_GridRouting;  // if (uy0 > row_min*Board.m_GridRouting) row_min++;
    col_min = ux0 / Board.m_GridRouting;  // if (ux0 > col_min*Board.m_GridRouting) col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= (Nrows - 1) )
        row_max = Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= (Ncols - 1) )
        col_max = Ncols - 1;

    /* Calculate coordinate limits of cell belonging to the rectangle. */
    if( row_min > row_max )
        row_max = row_min;

    if( col_min > col_max )
        col_max = col_min;

    fdistmin = (float) distmin * distmin;

    for( row = row_min; row <= row_max; row++ )
    {
        fdisty  = (float) ( cy - ( row * Board.m_GridRouting ) );
        fdisty *= fdisty;

        for( col = col_min; col <= col_max; col++ )
        {
            fdistx  = (float) ( cx - ( col * Board.m_GridRouting ) );
            fdistx *= fdistx;

            if( fdistmin <= ( fdistx + fdisty ) )
                continue;

            if( trace & 1 )
                WriteCell( row, col, BOTTOM, color );

            if( trace & 2 )
                WriteCell( row, col, TOP, color );

            tstwrite = 1;
        }
    }

    if( tstwrite )
        return;

    /* If no cell has been written, it affects the 4 neighboring diagonal
     * (Adverse event: pad off grid in the center of the 4 neighboring
     * diagonal) */
    distmin  = Board.m_GridRouting / 2 + 1;
    fdistmin = ( (float) distmin * distmin ) * 2; /* Distance to center point diagonally */

    for( row = row_min; row <= row_max; row++ )
    {
        fdisty  = (float) ( cy - ( row * Board.m_GridRouting ) );
        fdisty *= fdisty;

        for( col = col_min; col <= col_max; col++ )
        {
            fdistx  = (float) ( cx - ( col * Board.m_GridRouting ) );
            fdistx *= fdistx;

            if( fdistmin <= ( fdistx + fdisty ) )
                continue;

            if( trace & 1 )
                WriteCell( row, col, BOTTOM, color );

            if( trace & 2 )
                WriteCell( row, col, TOP, color );
        }
    }
}


void TraceSegmentPcb( BOARD* Pcb, TRACK* pt_segm, int color, int marge, int op_logic )
{
    int half_width;
    int ux0, uy0, ux1, uy1;

    half_width = ( pt_segm->m_Width / 2 ) + marge;

    /* Calculate the bounding rectangle of the segment (if H, V or Via) */
    ux0 = pt_segm->m_Start.x - Pcb->m_BoundaryBox.m_Pos.x;
    uy0 = pt_segm->m_Start.y - Pcb->m_BoundaryBox.m_Pos.y;
    ux1 = pt_segm->m_End.x - Pcb->m_BoundaryBox.m_Pos.x;
    uy1 = pt_segm->m_End.y - Pcb->m_BoundaryBox.m_Pos.y;

    /* Test if VIA (filled circle was drawn) */
    if( pt_segm->Type() == TYPE_VIA )
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
            TraceFilledCircle( Pcb, pt_segm->m_Start.x, pt_segm->m_Start.y,
                               half_width, mask_layer, color, op_logic );
        return;
    }

    int layer = pt_segm->GetLayer();

    if( color == VIA_IMPOSSIBLE )
        layer = -1;

    /* The segment is here a straight line or a circle or an arc.: */
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

    /* The segment is here a line segment. */
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

    void (* WriteCell)( int, int, int, MATRIX_CELL );

    switch( op_logic )
    {
    default:
    case WRITE_CELL:
        WriteCell = SetCell; break;

    case WRITE_OR_CELL:
        WriteCell = OrCell; break;

    case WRITE_XOR_CELL:
        WriteCell = XorCell; break;

    case WRITE_AND_CELL:
        WriteCell = AndCell; break;

    case WRITE_ADD_CELL:
        WriteCell = AddCell; break;
    }

    if( x0 == x1 )  // Vertical.
    {
        if( y1 < y0 )
            EXCHG( y0, y1 );

        dy  = y0 / Board.m_GridRouting;
        lim = y1 / Board.m_GridRouting;
        dx  = x0 / Board.m_GridRouting;

        /* Clipping limits of board. */
        if( ( dx < 0 ) || ( dx >= Ncols ) )
            return;

        if( dy < 0 )
            dy = 0;

        if( lim >= Nrows )
            lim = Nrows - 1;

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

        dx  = x0 / Board.m_GridRouting;
        lim = x1 / Board.m_GridRouting;
        dy  = y0 / Board.m_GridRouting;

        /* Clipping limits of board. */
        if( ( dy < 0 ) || ( dy >= Nrows ) )
            return;

        if( dx < 0 )
            dx = 0;

        if( lim >= Ncols )
            lim = Ncols - 1;

        for( ; dx <= lim; dx++ )
        {
            OP_CELL( layer, dy, dx );
        }

        return;
    }

    /* Here is some perspective: using the algorithm LUCAS. */
    if( abs( x1 - x0 ) >= abs( y1 - y0 ) ) /* segment slightly inclined/ */
    {
        if( x1 < x0 )
        {
            EXCHG( x1, x0 ); EXCHG( y1, y0 );
        }

        dx  = x0 / Board.m_GridRouting;
        lim = x1 / Board.m_GridRouting;
        dy  = y0 / Board.m_GridRouting;
        inc = 1;

        if( y1 < y0 )
            inc = -1;

        il    = lim - dx; cumul = il / 2;
        delta = abs( y1 - y0 ) / Board.m_GridRouting;

        for( ; dx <= lim; )
        {
            if( ( dx >= 0 ) && ( dy >= 0 ) && ( dx < Ncols ) && ( dy < Nrows ) )
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

        dy  = y0 / Board.m_GridRouting;
        lim = y1 / Board.m_GridRouting;
        dx  = x0 / Board.m_GridRouting;
        inc = 1;

        if( x1 < x0 )
            inc = -1;

        il    = lim - dy; cumul = il / 2;
        delta = abs( x1 - x0 ) / Board.m_GridRouting;

        for( ; dy <= lim; )
        {
            if( ( dx >= 0 ) && ( dy >= 0 ) && ( dx < Ncols ) && ( dy < Nrows ) )
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


void TraceFilledRectangle( BOARD* Pcb, int ux0, int uy0, int ux1, int uy1,
                           int aLayerMask, int color, int op_logic )
{
    int  row, col;
    int  row_min, row_max, col_min, col_max;
    int  trace = 0;

    void (* WriteCell)( int, int, int, MATRIX_CELL );

    if( aLayerMask & g_TabOneLayerMask[Route_Layer_BOTTOM] )
        trace = 1;     /* Trace on BOTTOM */

    if( ( aLayerMask & g_TabOneLayerMask[Route_Layer_TOP] ) && Nb_Sides )
        trace |= 2;    /* Trace on TOP */

    if( trace == 0 )
        return;

    switch( op_logic )
    {
    default:
    case WRITE_CELL:
        WriteCell = SetCell;
        break;

    case WRITE_OR_CELL:
        WriteCell = OrCell;
        break;

    case WRITE_XOR_CELL:
        WriteCell = XorCell;
        break;

    case WRITE_AND_CELL:
        WriteCell = AndCell;
        break;

    case WRITE_ADD_CELL:
        WriteCell = AddCell;
        break;
    }

    ux0 -= Pcb->m_BoundaryBox.m_Pos.x;
    uy0 -= Pcb->m_BoundaryBox.m_Pos.y;
    ux1 -= Pcb->m_BoundaryBox.m_Pos.x;
    uy1 -= Pcb->m_BoundaryBox.m_Pos.y;

    /* Calculating limits coord cells belonging to the rectangle. */
    row_max = uy1 / Board.m_GridRouting;
    col_max = ux1 / Board.m_GridRouting;
    row_min = uy0 / Board.m_GridRouting;

    if( uy0 > row_min * Board.m_GridRouting )
        row_min++;

    col_min = ux0 / Board.m_GridRouting;

    if( ux0 > col_min * Board.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( Nrows - 1 ) )
        row_max = Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( Ncols - 1 ) )
        col_max = Ncols - 1;

    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            if( trace & 1 )
                WriteCell( row, col, BOTTOM, color );

            if( trace & 2 )
                WriteCell( row, col, TOP, color );
        }
    }
}


void TraceFilledRectangle( BOARD* Pcb, int ux0, int uy0, int ux1, int uy1,
                           int angle, int aLayerMask, int color, int op_logic )
{
    int  row, col;
    int  cx, cy;    /* Center of rectangle */
    int  radius;     /* Radius of the circle */
    int  row_min, row_max, col_min, col_max;
    int  rotrow, rotcol;
    int  trace = 0;

    void (* WriteCell)( int, int, int, MATRIX_CELL );

    if( aLayerMask & g_TabOneLayerMask[Route_Layer_BOTTOM] )
        trace = 1;     /* Trace on BOTTOM */

    if( aLayerMask & g_TabOneLayerMask[Route_Layer_TOP] )
    {
        if( Nb_Sides )
            trace |= 2;  /* Trace on TOP */
    }

    if( trace == 0 )
        return;

    switch( op_logic )
    {
    default:
    case WRITE_CELL:
        WriteCell = SetCell;
        break;

    case WRITE_OR_CELL:
        WriteCell = OrCell;
        break;

    case WRITE_XOR_CELL:
        WriteCell = XorCell;
        break;

    case WRITE_AND_CELL:
        WriteCell = AndCell;
        break;

    case WRITE_ADD_CELL:
        WriteCell = AddCell;
        break;
    }

    ux0 -= Pcb->m_BoundaryBox.m_Pos.x;
    uy0 -= Pcb->m_BoundaryBox.m_Pos.y;
    ux1 -= Pcb->m_BoundaryBox.m_Pos.x;
    uy1 -= Pcb->m_BoundaryBox.m_Pos.y;

    cx    = (ux0 + ux1) / 2;
    cy    = (uy0 + uy1) / 2;
    radius = (int) sqrt( (double) ( cx - ux0 ) * ( cx - ux0 )
                       + (double) ( cy - uy0 ) * ( cy - uy0 ) );

    /* Calculating coordinate limits belonging to the rectangle. */
    row_max = ( cy + radius ) / Board.m_GridRouting;
    col_max = ( cx + radius ) / Board.m_GridRouting;
    row_min = ( cy - radius ) / Board.m_GridRouting;

    if( uy0 > row_min * Board.m_GridRouting )
        row_min++;

    col_min = ( cx - radius ) / Board.m_GridRouting;

    if( ux0 > col_min * Board.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( Nrows - 1 ) )
        row_max = Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( Ncols - 1 ) )
        col_max = Ncols - 1;

    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            rotrow = row * Board.m_GridRouting;
            rotcol = col * Board.m_GridRouting;
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
                WriteCell( row, col, BOTTOM, color );

            if( trace & 2 )
                WriteCell( row, col, TOP, color );
        }
    }
}


/* Fills all cells inside a segment
 * half-width lg, org ux, ux end y0, y1
 * is set to color.
 * coordinates are in PCB units (0.1 mil) are relative to the Board
 */
void DrawSegmentQcq( int ux0, int uy0, int ux1, int uy1, int lg, int layer,
                     int color, int op_logic )
{
    int  row, col;
    int  inc;
    int  row_max, col_max, row_min, col_min;
    int  demi_pas;

    void (* WriteCell)( int, int, int, MATRIX_CELL );
    int  angle;
    int  cx, cy, dx, dy;

    switch( op_logic )
    {
    default:
    case WRITE_CELL:
        WriteCell = SetCell;
        break;

    case WRITE_OR_CELL:
        WriteCell = OrCell;
        break;

    case WRITE_XOR_CELL:
        WriteCell = XorCell;
        break;

    case WRITE_AND_CELL:
        WriteCell = AndCell;
        break;

    case WRITE_ADD_CELL:
        WriteCell = AddCell;
        break;
    }

    /* Make coordinate ux1 tj > ux0 to simplify calculations */
    if( ux1 < ux0 )
    {
        EXCHG( ux1, ux0 );
        EXCHG( uy1, uy0 );
    }

    /* Calculating the incrementing the Y axis */
    inc = 1;

    if( uy1 < uy0 )
        inc = -1;

    demi_pas = Board.m_GridRouting / 2;

    col_min = ( ux0 - lg ) / Board.m_GridRouting;

    if( col_min < 0 )
        col_min = 0;

    col_max = ( ux1 + lg + demi_pas ) / Board.m_GridRouting;

    if( col_max > ( Ncols - 1 ) )
        col_max = Ncols - 1;

    if( inc > 0 )
    {
        row_min = ( uy0 - lg ) / Board.m_GridRouting;
        row_max = ( uy1 + lg + demi_pas ) / Board.m_GridRouting;
    }
    else
    {
        row_min = ( uy1 - lg ) / Board.m_GridRouting;
        row_max = ( uy0 + lg + demi_pas ) / Board.m_GridRouting;
    }

    if( row_min < 0 )
        row_min = 0;

    if( row_min > ( Nrows - 1 ) )
        row_min = Nrows - 1;

    if( row_max < 0 )
        row_max = 0;

    if( row_max > ( Nrows - 1 ) )
        row_max = Nrows - 1;

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

    RotatePoint( &dx, &dy, angle );   /* dx = length, dy = 0 */

    for( col = col_min; col <= col_max; col++ )
    {
        int cxr;
        cxr = ( col * Board.m_GridRouting ) - ux0;

        for( row = row_min; row <= row_max; row++ )
        {
            cy = (row * Board.m_GridRouting) - uy0;
            cx = cxr;
            RotatePoint( &cx, &cy, angle );

            if( abs( cy ) > lg )
                continue;             /* The point is too far on the Y axis. */

            /* This point a test is close to the segment: the position
             * along the X axis must be tested.
             */
            if( ( cx >= 0 ) && ( cx <= dx ) )
            {
                OP_CELL( layer, row, col );
                continue;
            }

            /* Examination of extremities are rounded. */
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


/* Fills all cells BOARD contained in the circle
 * half-width lg center ux, ux through y0, y1 is set to color.
 * coord in PCB units (0.1 million) relating to the origin
 * pt_pcb-> m_PcbBox.m_Xmin, Y's board.
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


/* Fills all BOARD cells contained in the arc of "L" angle half-width lg
 * ux center, starting in ux y0, y1 is set to color.  Coordinates are in
 * PCB units (0.1 mil) relating to the origin pt_pcb-> Pcb_oX, Y's board.
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

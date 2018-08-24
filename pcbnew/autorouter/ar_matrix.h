/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
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


#ifndef __AR_MATRIX_H
#define __AR_MATRIX_H

#include <eda_rect.h>
#include <layers_id_colors_and_visibility.h>

class DRAWSEGMENT;
class TRACK;
class D_PAD;
class MODULE;

#define AR_MAX_ROUTING_LAYERS_COUNT 2

#define AR_SIDE_TOP 0
#define AR_SIDE_BOTTOM 1

/**
 * class AR_MATRIX
 * handle the matrix routing that describes the actual board
 */
class AR_MATRIX
{
public:
    typedef unsigned char MATRIX_CELL;
    typedef int           DIST_CELL;
    typedef char          DIR_CELL;

    MATRIX_CELL* m_BoardSide[AR_MAX_ROUTING_LAYERS_COUNT]; // the image map of 2 board sides
    DIST_CELL*   m_DistSide[AR_MAX_ROUTING_LAYERS_COUNT];  // the image map of 2 board sides:
                                                           // distance to cells
    DIR_CELL* m_DirSide[AR_MAX_ROUTING_LAYERS_COUNT];      // the image map of 2 board sides:
                                                           // pointers back to source
    bool     m_InitMatrixDone;
    int      m_RoutingLayersCount; // Number of layers for autorouting (0 or 1)
    int      m_GridRouting;        // Size of grid for autoplace/autoroute
    EDA_RECT m_BrdBox;             // Actual board bounding box
    int      m_Nrows, m_Ncols;     // Matrix size
    int      m_MemSize;            // Memory requirement, just for statistics
    int      m_RouteCount;         // Number of routes

    PCB_LAYER_ID m_routeLayerTop;
    PCB_LAYER_ID m_routeLayerBottom;

private:
    // a pointer to the current selected cell operation
    void ( AR_MATRIX::*m_opWriteCell )( int aRow, int aCol, int aSide, MATRIX_CELL aCell );

public:
    enum CELL_OP
    {
        WRITE_CELL = 0,
        WRITE_OR_CELL = 1,
        WRITE_XOR_CELL = 2,
        WRITE_AND_CELL = 3,
        WRITE_ADD_CELL = 4
    };

    AR_MATRIX();
    ~AR_MATRIX();

    void WriteCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell )
    {
        ( *this.*m_opWriteCell )( aRow, aCol, aSide, aCell );
    }

    /**
     * function GetBrdCoordOrigin
     * @return the board coordinate corresponding to the
     * routing matrix origin ( board coordinate offset )
     */
    wxPoint GetBrdCoordOrigin()
    {
        return m_BrdBox.GetOrigin();
    }

    /**
     * Function ComputeMatrixSize
     * calculates the number of rows and columns of dimensions of \a aPcb for routing and
     * automatic calculation of area.
     * @param aPcb = the physical board
     * @param aUseBoardEdgesOnly = true to use board edges only,
     *                           = false to use the full board bounding box (default)
     */
    bool ComputeMatrixSize( const EDA_RECT& aBoundingBox );

    /**
     * Function InitBoard
     * initializes the data structures.
     *
     * @return the amount of memory used or -1 if default.
     */
    int InitRoutingMatrix();

    void UnInitRoutingMatrix();

    // Initialize WriteCell to make the aLogicOp
    void SetCellOperation( CELL_OP aLogicOp );

    // functions to read/write one cell ( point on grid routing matrix:
    MATRIX_CELL GetCell( int aRow, int aCol, int aSide );
    void        SetCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell );
    void        OrCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell );
    void        XorCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell );
    void        AndCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell );
    void        AddCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell );
    DIST_CELL   GetDist( int aRow, int aCol, int aSide );
    void        SetDist( int aRow, int aCol, int aSide, DIST_CELL );
    int         GetDir( int aRow, int aCol, int aSide );
    void        SetDir( int aRow, int aCol, int aSide, int aDir );

    // calculate distance (with penalty) of a trace through a cell
    int CalcDist( int x, int y, int z, int side );

    // calculate approximate distance (manhattan distance)
    int GetApxDist( int r1, int c1, int r2, int c2 );


    void TraceSegmentPcb( DRAWSEGMENT* pt_segm, int color, int marge, AR_MATRIX::CELL_OP op_logic );
    void TraceSegmentPcb( TRACK* aTrack, int color, int marge, AR_MATRIX::CELL_OP op_logic );
    void CreateKeepOutRectangle(
            int ux0, int uy0, int ux1, int uy1, int marge, int aKeepOut, LSET aLayerMask );
    void PlacePad( D_PAD* aPad, int color, int marge, AR_MATRIX::CELL_OP op_logic );
    void TraceFilledRectangle( int ux0, int uy0, int ux1, int uy1, double angle, LSET aLayerMask,
            int color, AR_MATRIX::CELL_OP op_logic );
    void TraceFilledRectangle( int ux0, int uy0, int ux1, int uy1, LSET aLayerMask, int color,
            AR_MATRIX::CELL_OP op_logic );

private:

    void drawSegmentQcq( int ux0, int uy0, int ux1, int uy1, int lg, LAYER_NUM layer, int color,
            CELL_OP op_logic );

    void traceCircle( int ux0, int uy0, int ux1, int uy1, int lg, LAYER_NUM layer, int color,
            AR_MATRIX::CELL_OP op_logic );
    void traceFilledCircle(
            int cx, int cy, int radius, LSET aLayerMask, int color, AR_MATRIX::CELL_OP op_logic );
    void traceArc( int ux0, int uy0, int ux1, int uy1, double ArcAngle, int lg, LAYER_NUM layer,
            int color, AR_MATRIX::CELL_OP op_logic );
    void tracePcbLine( int x0, int y0, int x1, int y1, LAYER_NUM layer, int color,
            AR_MATRIX::CELL_OP op_logic );
};

#endif

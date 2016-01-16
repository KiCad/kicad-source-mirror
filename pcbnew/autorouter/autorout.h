/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
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

/**
 * @file autorout.h
 */

#ifndef AUTOROUT_H
#define AUTOROUT_H


#include <base_struct.h>
#include <layers_id_colors_and_visibility.h>


class BOARD;
class DRAWSEGMENT;


#define TOP     0
#define BOTTOM  1
#define EMPTY   0
#define ILLEGAL -1


/* Autorouter commands. */
enum AUTOPLACEROUTE_OPTIONS
{
    PLACE_ALL,
    PLACE_OUT_OF_BOARD,
    PLACE_INCREMENTAL,
    PLACE_1_MODULE,

    ROUTE_ALL,
    ROUTE_NET,
    ROUTE_MODULE,
    ROUTE_PAD
};

#define MAX_ROUTING_LAYERS_COUNT 2

#define FORCE_PADS 1  /* Force placement of pads for any Netcode */

/* search statistics */
extern int OpenNodes;   /* total number of nodes opened */
extern int ClosNodes;   /* total number of nodes closed */
extern int MoveNodes;   /* total number of nodes moved */
extern int MaxNodes;    /* maximum number of nodes opened at one time */


/* Structures useful to the generation of board as bitmap. */
typedef unsigned char MATRIX_CELL;
typedef int  DIST_CELL;
typedef char DIR_CELL;


/**
 * class MATRIX_ROUTING_HEAD
 * handle the matrix routing that describes the actual board
 */
class MATRIX_ROUTING_HEAD
{
public:
    MATRIX_CELL* m_BoardSide[MAX_ROUTING_LAYERS_COUNT]; // the image map of 2 board sides
    DIST_CELL*   m_DistSide[MAX_ROUTING_LAYERS_COUNT];  // the image map of 2 board sides:
                                                        // distance to cells
    DIR_CELL*    m_DirSide[MAX_ROUTING_LAYERS_COUNT];   // the image map of 2 board sides:
                                                        // pointers back to source
    bool         m_InitMatrixDone;
    int          m_RoutingLayersCount;          // Number of layers for autorouting (0 or 1)
    int          m_GridRouting;                 // Size of grid for autoplace/autoroute
    EDA_RECT     m_BrdBox;                      // Actual board bounding box
    int          m_Nrows, m_Ncols;              // Matrix size
    int          m_MemSize;                     // Memory requirement, just for statistics
    int          m_RouteCount;                  // Number of routes

private:
    // a pointer to the current selected cell operation
    void        (MATRIX_ROUTING_HEAD::* m_opWriteCell)( int aRow, int aCol,
                                                        int aSide, MATRIX_CELL aCell);

public:
    MATRIX_ROUTING_HEAD();
    ~MATRIX_ROUTING_HEAD();

    void WriteCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell)
    {
        (*this.*m_opWriteCell)( aRow, aCol, aSide, aCell );
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
    bool ComputeMatrixSize( BOARD* aPcb, bool aUseBoardEdgesOnly = false );

    /**
     * Function InitBoard
     * initializes the data structures.
     *
     * @return the amount of memory used or -1 if default.
     */
    int InitRoutingMatrix();

    void UnInitRoutingMatrix();

    // Initialize WriteCell to make the aLogicOp
    void SetCellOperation( int aLogicOp );

    // functions to read/write one cell ( point on grid routing matrix:
    MATRIX_CELL GetCell( int aRow, int aCol, int aSide);
    void SetCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell);
    void OrCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell);
    void XorCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell);
    void AndCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell);
    void AddCell( int aRow, int aCol, int aSide, MATRIX_CELL aCell);
    DIST_CELL GetDist( int aRow, int aCol, int aSide );
    void SetDist( int aRow, int aCol, int aSide, DIST_CELL );
    int GetDir( int aRow, int aCol, int aSide );
    void SetDir( int aRow, int aCol, int aSide, int aDir);

    // calculate distance (with penalty) of a trace through a cell
    int CalcDist(int x,int y,int z ,int side );

    // calculate approximate distance (manhattan distance)
    int GetApxDist( int r1, int c1, int r2, int c2 );
};

extern MATRIX_ROUTING_HEAD RoutingMatrix;        /* 2-sided board */


/* Constants used to trace the cells on the BOARD */
#define WRITE_CELL     0
#define WRITE_OR_CELL  1
#define WRITE_XOR_CELL 2
#define WRITE_AND_CELL 3
#define WRITE_ADD_CELL 4

// Functions:

class PCB_EDIT_FRAME;
class BOARD;
class D_PAD;
class RATSNEST_ITEM;
class TRACK;


/* Initialize a color value, the cells included in the board edge of the
 * pad surface by pt_pad, with the margin reserved for isolation and the
 * half width of the runway
 * Parameters:
 * Pt_pad: pointer to the description of the pad
 * color: mask write in cells
 * margin: add a value to the radius or half the score pad
 * op_logic: type of writing in the cell (WRITE, OR)
 */
void PlacePad( D_PAD* pt_pad, int type, int marge, int op_logic );

/* Draws a segment of track on the board. */
void TraceSegmentPcb( TRACK* pt_segm, int type, int marge, int op_logic );
void TraceSegmentPcb( DRAWSEGMENT* pt_segm, int type, int marge, int op_logic );

/* Uses the color value of all cells included in the board
 * coord of the rectangle ux0, uy0 (top right corner)
 * a ux1, uy1 (lower left corner) (coord PCB)
 * the rectangle is horizontal (or vertical)
 * masque_layer = mask layers;
 * op_logic = WRITE_CELL, WRITE_OR_CELL, WRITE_XOR_CELL, WRITE_AND_CELL
 */
void TraceFilledRectangle( int ux0, int uy0, int ux1, int uy1,
                           LSET side, int color, int op_logic);


/* Same as above, but the rectangle is inclined angle angle. */
void TraceFilledRectangle( int ux0, int uy0, int ux1, int uy1,
                           double angle, LSET masque_layer,
                           int color, int op_logic );

/* QUEUE.CPP */
void FreeQueue();
void InitQueue();
void GetQueue( int *, int *, int *, int *, int * );
bool  SetQueue( int, int, int, int, int, int, int );
void ReSetQueue( int, int, int, int, int, int, int );

/* WORK.CPP */
void InitWork();
void ReInitWork();
int SetWork( int, int, int , int, int, RATSNEST_ITEM *, int );
void GetWork( int *, int *, int *, int *, int *, RATSNEST_ITEM ** );
void SortWork(); /* order the work items; shortest first */

/* routing_matrix.cpp */
int Build_Work( BOARD * Pcb );
void PlaceCells( BOARD * Pcb, int net_code, int flag = 0 );


#endif  // AUTOROUT_H

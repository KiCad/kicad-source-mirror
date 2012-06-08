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


class BOARD;


#define TOP     0
#define BOTTOM  1
#define EMPTY   0
#define ILLEGAL -1


/* Autorouter commands. */
enum CommandOpt {
    PLACE_ALL,
    PLACE_OUT_OF_BOARD,
    PLACE_INCREMENTAL,
    PLACE_1_MODULE,

    ROUTE_ALL,
    ROUTE_NET,
    ROUTE_MODULE,
    ROUTE_PAD
};


#define ONE_SIDE  0
#define TWO_SIDES 1

#define MAX_SIDES_COUNT 2

extern int Nb_Sides;    /* Number of layers for autorouting (0 or 1) */

#define FORCE_PADS 1  /* Force placement of pads for any Netcode */

/* board dimensions */
extern int Nrows;
extern int Ncols;
extern int Ntotal;

/* search statistics */
extern int OpenNodes;   /* total number of nodes opened */
extern int ClosNodes;   /* total number of nodes closed */
extern int MoveNodes;   /* total number of nodes moved */
extern int MaxNodes;    /* maximum number of nodes opened at one time */


/* Structures useful to the generation of board as bitmap. */
typedef char MATRIX_CELL;
typedef int  DIST_CELL;
typedef char DIR_CELL;


/**
 * class MATRIX_ROUTING_HEAD
 */
class MATRIX_ROUTING_HEAD  /* header of blocks of MATRIX_CELL */
{
public:
    MATRIX_CELL* m_BoardSide[MAX_SIDES_COUNT];  /* ptr to block of memory: 2-sided board */
    DIST_CELL*   m_DistSide[MAX_SIDES_COUNT];   /* ptr to block of memory: path distance to
                                                 * cells */
    DIR_CELL*    m_DirSide[MAX_SIDES_COUNT];    /* header of blocks of chars:pointers back to
                                                 * source */
    bool         m_InitMatrixDone;
    int          m_Layers;
    int          m_GridRouting;                 // Size of grid for autoplace/autoroute
    EDA_RECT     m_BrdBox;                      // Actual board bounding box
    int          m_Nrows, m_Ncols;
    int          m_MemSize;

public:
    MATRIX_ROUTING_HEAD();
    ~MATRIX_ROUTING_HEAD();

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
void PlacePad( BOARD* Pcb, D_PAD* pt_pad, int type, int marge, int op_logic );

/* Draws a segment of track on the board. */
void TraceSegmentPcb( BOARD* Pcb, TRACK* pt_segm, int type, int marge, int op_logic );

/* Uses the color value of all cells included in the board
 * coord of the rectangle ux0, uy0 (top right corner)
 * a ux1, uy1 (lower left corner) (coord PCB)
 * the rectangle is horizontal (or vertical)
 * masque_layer = mask layers;
 * op_logic = WRITE_CELL, WRITE_OR_CELL, WRITE_XOR_CELL, WRITE_AND_CELL
 */
void TraceFilledRectangle( BOARD* Pcb, int ux0, int uy0, int ux1, int uy1,
                           int side, int color, int op_logic);


/* Same as above, but the rectangle is inclined angle angle. */
void TraceFilledRectangle( BOARD* Pcb, int ux0, int uy0, int ux1, int uy1,
                           int angle, int masque_layer, int color, int op_logic );

/* QUEUE.CPP */
void FreeQueue();
void InitQueue();
void GetQueue( int *, int *, int *, int *, int * );
int  SetQueue( int, int, int, int, int, int, int );
void ReSetQueue( int, int, int, int, int, int, int );

/* WORK.CPP */
void InitWork();
void ReInitWork();
int SetWork( int, int, int , int, int, RATSNEST_ITEM *, int );
void GetWork( int *, int *, int *, int *, int *, RATSNEST_ITEM ** );
void SortWork(); /* order the work items; shortest first */

/* DIST.CPP */
int GetApxDist( int r1, int c1, int r2, int c2 );
int CalcDist(int x,int y,int z ,int side );

/* BOARD.CPP */
int Build_Work( BOARD * Pcb );
void PlaceCells( BOARD * Pcb, int net_code, int flag = 0 );

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


#endif  // AUTOROUT_H

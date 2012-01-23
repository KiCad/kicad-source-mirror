/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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


extern int E_scale;     /* Scaling factor of distance tables. */

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
    bool         m_InitBoardDone;
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
     */
    bool ComputeMatrixSize( BOARD* aPcb );

    /**
     * Function InitBoard
     * initializes the data structures.
     *
     * @return the amount of memory used or -1 if default.
     */
    int InitBoard();

    void UnInitBoard();
};

extern MATRIX_ROUTING_HEAD Board;        /* 2-sided board */


/* Constants used to trace the cells on the BOARD */
#define WRITE_CELL     0
#define WRITE_OR_CELL  1
#define WRITE_XOR_CELL 2
#define WRITE_AND_CELL 3
#define WRITE_ADD_CELL 4


#endif  // AUTOROUT_H

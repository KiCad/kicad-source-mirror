/****************************************************/
/*					AUTOROUT.H						*/
/****************************************************/

#ifndef AUTOROUT_H
#define AUTOROUT_H


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

typedef char BoardCell;
typedef int  DistCell;

class BOARDHEAD  /* header of blocks of BoardCell */
{
public:
    BoardCell* m_BoardSide[2];  /* ptr to block of memory: 2-sided board */
    DistCell*  m_DistSide[2];   /* ptr to block of memory: path distance to
                                 * cells */
    char*      m_DirSide[2];    /* header of blocks of chars:pointers back to
                                 * source */
    bool       m_InitBoardDone;
    int        m_Layers;
    int        m_Nrows, m_Ncols;
    int        m_MemSize;

public:
    BOARDHEAD();
    ~BOARDHEAD();
    int     InitBoard();
    void    UnInitBoard();
};

extern BOARDHEAD Board;        /* 2-sided board */


/* Constants used to trace the cells on the BOARD */
#define WRITE_CELL     0
#define WRITE_OR_CELL  1
#define WRITE_XOR_CELL 2
#define WRITE_AND_CELL 3
#define WRITE_ADD_CELL 4


#include "ar_protos.h"


#endif  // AUTOROUT_H


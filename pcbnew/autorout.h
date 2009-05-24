/****************************************************/
/*					AUTOROUT.H						*/
/* declarations communes relative au routage	*/
/* et placement automatique des composants			*/
/****************************************************/

#ifndef AUTOROUT_H
#define AUTOROUT_H


#define TOP     0
#define BOTTOM  1
#define EMPTY   0
#define ILLEGAL -1


/***********************************************/
/* description d'un segment de chevelu general */
/***********************************************/

/* Commandes d'autoplacement / autorouage possibles */
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


/* Variables et structures d'autoroutage */

extern int E_scale;     /* facteur d'echelle des tables de distance */

#define ONE_SIDE  0
#define TWO_SIDES 1

extern int Nb_Sides;    /* Nombre de couches pour autoroutage (0 ou 1) */

/* Bits Flags de gestion de remplissage du BOARD */
#define FORCE_PADS 1  /* pour forcage placement pads quel que soit le netcode */

/* board dimensions */
extern int Nrows;
extern int Ncols;
extern int Ntotal;

/* search statistics */
extern int OpenNodes;   /* total number of nodes opened */
extern int ClosNodes;   /* total number of nodes closed */
extern int MoveNodes;   /* total number of nodes moved */
extern int MaxNodes;    /* maximum number of nodes opened at one time */

/* Structures utiles a la generation du board en Bit Map */


typedef char BoardCell;
typedef int  DistCell;

class BOARDHEAD  /* header of blocks of BoardCell */
{
public:
    BoardCell* m_BoardSide[2];  /* ptr to block of memory: 2-sided board */
    DistCell*  m_DistSide[2];   /* ptr to block of memory: path distance to cells */
    char*      m_DirSide[2];    /* header of blocks of chars:pointers back to source */
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


/* Constantes utilisees pour le trace des cellules sur le BOARD */
#define WRITE_CELL     0
#define WRITE_OR_CELL  1
#define WRITE_XOR_CELL 2
#define WRITE_AND_CELL 3
#define WRITE_ADD_CELL 4


#include "ar_protos.h"


#endif  // AUTOROUT_H


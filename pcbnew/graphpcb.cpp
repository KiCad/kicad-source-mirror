
/* EDITEUR de PCB: AUTOROUTAGE: routines "graphiques" */
/******************************************************/

/* Fichier BOARD.CC */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "zones.h"
#include "trigo.h"
#include "cell.h"

/* Exported functions */
int     ToMatrixCoordinate ( int aPhysicalCoordinate);
void    TraceLignePcb( int x0, int y0, int x1, int y1, int layer, int color );
void    TraceArc( int ux0, int uy0, int ux1, int uy1, int ArcAngle, int lg, int layer,
                  int color, int op_logique );


/* Local functions */
static void    DrawSegmentQcq( int ux0, int uy0, int ux1, int uy1, int lg, int layer,
                        int color, int op_logique );
static void    DrawHVSegment( int ux0, int uy0, int ux1, int uy1, int demi_largeur, int layer,
                       int color, int op_logique );

static void    TraceFilledCercle( BOARD* Pcb, int cx, int cy, int rayon, int masque_layer,
                           int color, int op_logique );
static void    TraceCercle( int ux0, int uy0, int ux1, int uy1, int lg, int layer,
                     int color, int op_logique );

/* Macro d'appel de mise a jour de cellules */
#define OP_CELL( layer, dy, dx ) { if( layer < 0 ) \
                                   { \
                                       WriteCell( dy, dx, BOTTOM, color ); \
                                       if( Nb_Sides ) \
                                           WriteCell( dy, dx, TOP, color );\
                                   } \
                                   else { \
                                       if( layer == Route_Layer_BOTTOM ) \
                                           WriteCell( dy, dx, BOTTOM, color );\
                                       if( Nb_Sides ) \
                                           if( layer == Route_Layer_TOP ) \
                                               WriteCell( dy, dx, TOP, color );\
                                   } }

int ToMatrixCoordinate ( int aPhysicalCoordinate)
/** Function ToMatrixCoordinate
 * compute the coordinate in the routing matrix from the real (board) value
 * @param aPhysicalCoordinate = value to convert
 * @return the coordinate relative to the matrix
 */
{
    return aPhysicalCoordinate / g_GridRoutingSize;
}

/******************************************************************************/
void Place_1_Pad_Board( BOARD* Pcb, D_PAD* pt_pad, int color, int marge, int op_logique )
/******************************************************************************/

/* Initialise a la valeur color, les cellules du Board comprises dans la
 *  surface du pad pointe par pt_pad, avec la marge reservee pour l'isolement
 *  et la demi largeur de la piste
 *  Parametres:
 *      pt_pad : pointeur sur la description du pad
 *      color : masque a ecrire dans les cellules
 *      marge : valeur a ajouter au rayon ou demi cote du pad
 *      op_logique: type d'ecriture dans la cellule ( WRITE, OR )
 */
{
    int     dx, dy;
    wxPoint shape_pos = pt_pad->ReturnShapePos();;

    dx = pt_pad->m_Size.x / 2; dx += marge;

    if( pt_pad->m_PadShape == PAD_CIRCLE )
    {
        TraceFilledCercle( Pcb, shape_pos.x, shape_pos.y, dx,
                           pt_pad->m_Masque_Layer, color, op_logique );
        return;
    }


    dy = pt_pad->m_Size.y / 2; dy += marge;

    if( pt_pad->m_PadShape == PAD_TRAPEZOID )
    {
        dx += abs( pt_pad->m_DeltaSize.y ) / 2;
        dy += abs( pt_pad->m_DeltaSize.x ) / 2;
    }

    if( (pt_pad->m_Orient % 900) == 0 )
    {               /* Le pad est un rectangle horizontal ou vertical */
        if( (pt_pad->m_Orient == 900 ) || (pt_pad->m_Orient == 2700) )
        {           /* orient tournee de 90 deg */
            EXCHG( dx, dy );
        }

        TraceFilledRectangle( Pcb, shape_pos.x - dx, shape_pos.y - dy,
                              shape_pos.x + dx, shape_pos.y + dy,
                              pt_pad->m_Masque_Layer, color, op_logique );
    }
    else
    {
        TraceFilledRectangle( Pcb, shape_pos.x - dx, shape_pos.y - dy,
                              shape_pos.x + dx, shape_pos.y + dy,
                              (int) pt_pad->m_Orient,
                              pt_pad->m_Masque_Layer, color, op_logique );
    }
}


/************************************************************************/
void TraceFilledCercle( BOARD* Pcb, int cx, int cy, int rayon, int masque_layer,
                        int color, int op_logique )
/************************************************************************/

/* Initialise a la valeur color, les cellules du Board comprises dans la
 *  surface du cercle de centre cx,cy.
 *  Parametres:
 *      rayon : valeur a ajouter au rayon ou demi cote du pad
 *      masque_layer = couches occupees
 *      color : masque a ecrire dans les cellules
 *      op_logique: type d'ecriture dans la cellule ( WRITE, OR )
 */
{
    int   row, col;
    int   ux0, uy0, ux1, uy1;
    int   row_max, col_max, row_min, col_min;
    int   trace = 0;
    float fdistmin, fdistx, fdisty;

    void  (*WriteCell)( int, int, int, BoardCell );
    int   tstwrite = 0;
    int   distmin;

    /* Determination des couches occupees : */

    /* routage sur 1 seule couche:
     *  sur bitmap BOTTOM et Route_Layer_B = Route_Layer_A*/

    if( masque_layer & g_TabOneLayerMask[Route_Layer_BOTTOM] )
        trace = 1;     /* Trace sur BOTTOM */

    if( masque_layer & g_TabOneLayerMask[Route_Layer_TOP] )
        if( Nb_Sides )
            trace |= 2;/* Trace sur TOP */

    if( trace == 0 )
        return;

    switch( op_logique )
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

    cx -= Pcb->m_BoundaryBox.m_Pos.x; cy -= Pcb->m_BoundaryBox.m_Pos.y;

    distmin = rayon;

    /* Calcul du rectangle d'encadrement du cercle*/
    ux0 = cx - rayon; uy0 = cy - rayon;
    ux1 = cx + rayon; uy1 = cy + rayon;

    /* Calcul des coord limites des cellules appartenant au rectangle */
    row_max = uy1 / g_GridRoutingSize;
    col_max = ux1 / g_GridRoutingSize;
    row_min = uy0 / g_GridRoutingSize;  // if (uy0 > row_min*g_GridRoutingSize ) row_min++;
    col_min = ux0 / g_GridRoutingSize;  // if (ux0 > col_min*g_GridRoutingSize ) col_min++;

    if( row_min < 0 )
        row_min = 0;
    if( row_max >= (Nrows - 1) )
        row_max = Nrows - 1;
    if( col_min < 0 )
        col_min = 0;
    if( col_max >= (Ncols - 1) )
        col_max = Ncols - 1;

    /* On s'assure que l'on place toujours au moins une cellule */
    if( row_min > row_max )
        row_max = row_min;
    if( col_min > col_max )
        col_max = col_min;

    fdistmin = (float) distmin * distmin;

    for( row = row_min; row <= row_max; row++ )
    {
        fdisty  = (float) ( cy - (row * g_GridRoutingSize) );
        fdisty *= fdisty;
        for( col = col_min; col <= col_max; col++ )
        {
            fdistx  = (float) ( cx - (col * g_GridRoutingSize) );
            fdistx *= fdistx;

            if( fdistmin <= (fdistx + fdisty) )
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

    /* Si aucune cellule n'a ete ecrite, on touche les 4 voisins diagonaux
     *  (cas defavorable: pad hors grille, au centre des 4 voisins diagonaux) */

    distmin  = g_GridRoutingSize / 2 + 1;
    fdistmin = ( (float) distmin * distmin ) * 2; /* = dist centre au point diagonal*/

    for( row = row_min; row <= row_max; row++ )
    {
        fdisty  = (float) ( cy - (row * g_GridRoutingSize) );
        fdisty *= fdisty;
        for( col = col_min; col <= col_max; col++ )
        {
            fdistx  = (float) ( cx - (col * g_GridRoutingSize) );
            fdistx *= fdistx;

            if( fdistmin <= (fdistx + fdisty) )
                continue;

            if( trace & 1 )
                WriteCell( row, col, BOTTOM, color );
            if( trace & 2 )
                WriteCell( row, col, TOP, color );
        }
    }
}


/******************************************************************************/
void TraceSegmentPcb( BOARD* Pcb, TRACK* pt_segm, int color, int marge, int op_logique )
/******************************************************************************/

/* trace un Segment de piste sur le BOARD de routage
 */
{
    int demi_pas, demi_largeur;
    int ux0, uy0, ux1, uy1;


    demi_pas     = g_GridRoutingSize / 2;
    demi_largeur = (pt_segm->m_Width / 2) + marge;
    /* Calcul du rectangle d'encadrement du segment ( si H,V ou Via) */
    ux0 = pt_segm->m_Start.x - Pcb->m_BoundaryBox.m_Pos.x;
    uy0 = pt_segm->m_Start.y - Pcb->m_BoundaryBox.m_Pos.y;
    ux1 = pt_segm->m_End.x - Pcb->m_BoundaryBox.m_Pos.x;
    uy1 = pt_segm->m_End.y - Pcb->m_BoundaryBox.m_Pos.y;

    /* Test si VIA (cercle plein a tracer) */
    if( pt_segm->Type() == TYPEVIA )
    {
		int mask_layer = 0;
		if ( pt_segm->IsOnLayer(Route_Layer_BOTTOM) )
			mask_layer = 1 << Route_Layer_BOTTOM;
		if ( pt_segm->IsOnLayer(Route_Layer_TOP) )
		{
			if ( mask_layer == 0 )
				mask_layer = 1 << Route_Layer_TOP;
			else mask_layer = -1;
		}

		if( color == VIA_IMPOSSIBLE )
			mask_layer = -1;

		if ( mask_layer )
			TraceFilledCercle( Pcb, pt_segm->m_Start.x, pt_segm->m_Start.y, demi_largeur,
                           mask_layer, color, op_logique );
        return;
    }

    int layer = pt_segm->GetLayer();
    if( color == VIA_IMPOSSIBLE )
        layer = -1;

    /* Le segment est ici un segment de droite ou un cercle ou un arc: */
    if( pt_segm->m_Shape == S_CIRCLE )
    {
        TraceCercle( ux0, uy0, ux1, uy1, demi_largeur, layer, color, op_logique );
        return;
    }

    if( pt_segm->m_Shape == S_ARC )
    {
        TraceArc( ux0, uy0, ux1, uy1, pt_segm->m_Param, demi_largeur, layer, color, op_logique );
        return;
    }

    /* Le segment est ici un segment de droite */
    if( (ux0 != ux1) && (uy0 != uy1) ) // segment incline
    {
        DrawSegmentQcq( ux0, uy0, ux1, uy1, demi_largeur, layer, color, op_logique );
        return;
    }

    // Ici le segment est HORIZONTAL ou VERTICAL
//	DrawHVSegment(ux0,uy0,ux1,uy1,demi_largeur,layer,color,op_logique);              //F4EXB 051018-01
    DrawSegmentQcq( ux0, uy0, ux1, uy1, demi_largeur, layer, color, op_logique );               //F4EXB 051018-01
    return;                                                                                     //F4EXB 051018-01
}


/******************************************************************************************/
void TraceLignePcb( int x0, int y0, int x1, int y1, int layer, int color, int op_logique  )
/******************************************************************************************/

/* trace une ligne; si layer = -1 sur toutes les couches
 */
{
    int  dx, dy, lim;
    int  cumul, inc, il, delta;

    void (*WriteCell)( int, int, int, BoardCell );

    switch( op_logique )
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

    if( x0 == x1 )  // ligne verticale
    {
        if( y1 < y0 )
            EXCHG( y0, y1 );
        dy = y0 / g_GridRoutingSize;
        lim = y1 / g_GridRoutingSize;
        dx = x0 / g_GridRoutingSize;
        /* Clipping aux limites du board */
        if( (dx < 0) || (dx >= Ncols) )
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

    if( y0 == y1 )  // ligne horizontale
    {
        if( x1 < x0 )
            EXCHG( x0, x1 );
        dx = x0 / g_GridRoutingSize;
        lim = x1 / g_GridRoutingSize;
        dy = y0 / g_GridRoutingSize;
        /* Clipping aux limites du board */
        if( (dy < 0) || (dy >= Nrows) )
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

    /* Ici l'angle est quelconque: on utilise l'algorithme de LUCAS */
    if( abs( x1 - x0 ) >= abs( y1 - y0 ) ) /* segment peu incline */
    {
        if( x1 < x0 )
        {
            EXCHG( x1, x0 ); EXCHG( y1, y0 );
        }

        dx  = x0 / g_GridRoutingSize;
        lim = x1 / g_GridRoutingSize;
        dy  = y0 / g_GridRoutingSize;
        inc = 1; if( y1 < y0 )
            inc = -1;
        il  = lim - dx; cumul = il / 2; delta = abs( y1 - y0 ) / g_GridRoutingSize;
        for( ; dx <= lim; )
        {
            if( (dx >= 0) && (dy >= 0)
               && (dx < Ncols) && (dy < Nrows) )
            {
                OP_CELL( layer, dy, dx );
            }
            dx++; cumul += delta;
            if( cumul > il )
            {
                cumul -= il; dy += inc;
            }
        }
    }
    else
    {
        if( y1 < y0 )
        {
            EXCHG( x1, x0 ); EXCHG( y1, y0 );
        }

        dy  = y0 / g_GridRoutingSize;
        lim = y1 / g_GridRoutingSize;
        dx  = x0 / g_GridRoutingSize;
        inc = 1; if( x1 < x0 )
            inc = -1;
        il  = lim - dy; cumul = il / 2; delta = abs( x1 - x0 ) / g_GridRoutingSize;
        for( ; dy <= lim; )
        {
            if( (dx >= 0) && (dy >= 0)
               && (dx < Ncols) && (dy < Nrows) )
            {
                OP_CELL( layer, dy, dx );
            }
            dy++; cumul += delta;
            if( cumul > il )
            {
                cumul -= il; dx += inc;
            }
        }
    }
}


/*****************************************************************/
void TraceFilledRectangle( BOARD* Pcb,
                           int ux0, int uy0, int ux1, int uy1,
                           int masque_layer, int color, int op_logique )
/*****************************************************************/

/*  Fonction Surchargee.
 *
 *   Met a la valeur color l'ensemble des cellules du board inscrites dans
 *  le rectangle de coord ux0,uy0 ( angle haut a gauche )
 *  a ux1,uy1 ( angle bas a droite )
 *  Le rectangle est horizontal ( ou vertical )
 *  Coordonnees PCB.
 */
{
    int  row, col;
    int  row_min, row_max, col_min, col_max;
    int  trace = 0;

    void (*WriteCell)( int, int, int, BoardCell );

    if( masque_layer & g_TabOneLayerMask[Route_Layer_BOTTOM] )
        trace = 1;     /* Trace sur BOTTOM */

    if( (masque_layer & g_TabOneLayerMask[Route_Layer_TOP] ) && Nb_Sides )
        trace |= 2;     /* Trace sur TOP */

    if( trace == 0 )
        return;

    switch( op_logique )
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

    ux0 -= Pcb->m_BoundaryBox.m_Pos.x; uy0 -= Pcb->m_BoundaryBox.m_Pos.y;
    ux1 -= Pcb->m_BoundaryBox.m_Pos.x; uy1 -= Pcb->m_BoundaryBox.m_Pos.y;

    /* Calcul des coord limites des cellules appartenant au rectangle */
    row_max = uy1 / g_GridRoutingSize;
    col_max = ux1 / g_GridRoutingSize;
    row_min = uy0 / g_GridRoutingSize; if( uy0 > row_min * g_GridRoutingSize )
        row_min++;
    col_min = ux0 / g_GridRoutingSize; if( ux0 > col_min * g_GridRoutingSize )
        col_min++;

    if( row_min < 0 )
        row_min = 0;
    if( row_max >= (Nrows - 1) )
        row_max = Nrows - 1;
    if( col_min < 0 )
        col_min = 0;
    if( col_max >= (Ncols - 1) )
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


/***********************************************************************************/
void TraceFilledRectangle( BOARD* Pcb, int ux0, int uy0, int ux1, int uy1, int angle,
                           int masque_layer, int color, int op_logique )
/***********************************************************************************/

/*  Fonction Surchargee.
 *
 *  Met a la valeur color l'ensemble des cellules du board inscrites dans
 *  le rectangle de coord ux0,uy0 ( angle haut a droite )
 *  a ux1,uy1 ( angle bas a gauche )
 *  Le rectangle est tourne de la valeur angle (en 0,1 degres)
 *  Coordonnees PCB.
 */
{
    int  row, col;
    int  cx, cy;    /* Centre du rectangle */
    int  rayon;     /* rayon du cercle exinscrit */
    int  row_min, row_max, col_min, col_max;
    int  rotrow, rotcol;
    int  trace = 0;

    void (*WriteCell)( int, int, int, BoardCell );

    if( masque_layer & g_TabOneLayerMask[Route_Layer_BOTTOM] )
        trace = 1;     /* Trace sur BOTTOM */

    if( masque_layer & g_TabOneLayerMask[Route_Layer_TOP] )
        if( Nb_Sides )
            trace |= 2;/* Trace sur TOP */

    if( trace == 0 )
        return;

    switch( op_logique )
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

    ux0 -= Pcb->m_BoundaryBox.m_Pos.x; uy0 -= Pcb->m_BoundaryBox.m_Pos.y;
    ux1 -= Pcb->m_BoundaryBox.m_Pos.x; uy1 -= Pcb->m_BoundaryBox.m_Pos.y;

    cx    = (ux0 + ux1) / 2; cy = (uy0 + uy1) / 2;
    rayon = (int) sqrt( (double) (cx - ux0) * (cx - ux0)
                       + (double) (cy - uy0) * (cy - uy0) );

    /* Calcul des coord limites des cellules appartenant au rectangle */
    row_max = (cy + rayon) / g_GridRoutingSize;
    col_max = (cx + rayon) / g_GridRoutingSize;
    row_min = (cy - rayon) / g_GridRoutingSize; if( uy0 > row_min * g_GridRoutingSize )
        row_min++;
    col_min = (cx - rayon) / g_GridRoutingSize; if( ux0 > col_min * g_GridRoutingSize )
        col_min++;

    if( row_min < 0 )
        row_min = 0;
    if( row_max >= (Nrows - 1) )
        row_max = Nrows - 1;
    if( col_min < 0 )
        col_min = 0;
    if( col_max >= (Ncols - 1) )
        col_max = Ncols - 1;

    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            rotrow = row * g_GridRoutingSize; rotcol = col * g_GridRoutingSize;
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


/*****************************************************************/
void DrawSegmentQcq( int ux0, int uy0, int ux1, int uy1, int lg, int layer,
                     int color, int op_logique )
/*****************************************************************/

/* Remplit toutes les cellules du BOARD contenues dans le segment
 *  de demi-largeur lg, org ux,y0 fin ux,y1 a la valeur color .
 *  coord en unites PCB (0.1 mil) relatives a l'origine pt_pcb->m_PcbBox.m_Xmin,Y du board.
 */
{
    int  row, col;
    int  inc;
    int  row_max, col_max, row_min, col_min;
    int  demi_pas;

    void (*WriteCell)( int, int, int, BoardCell );
    int  angle;
    int  cx, cy, dx, dy;

    switch( op_logique )
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

    /* on rend la coordonnee ux1 tj > ux0 , pour simplifier les calculs */
    if( ux1 < ux0 )
    {
        EXCHG( ux1, ux0 ); EXCHG( uy1, uy0 );
    }

    /* calcul de l'increment selon axe Y */
    inc = 1; if( uy1 < uy0 )
        inc = -1;

    /* Calcul de l'encadrement : */
    demi_pas = g_GridRoutingSize / 2;

    col_min = (ux0 - lg) / g_GridRoutingSize;
    if( col_min < 0 )
        col_min = 0;
    col_max = (ux1 + lg + demi_pas) / g_GridRoutingSize;
    if( col_max > (Ncols - 1) )
        col_max = Ncols - 1;

    if( inc > 0 )
    {
        row_min = (uy0 - lg) / g_GridRoutingSize;
        row_max = (uy1 + lg + demi_pas) / g_GridRoutingSize;
    }
    else
    {
        row_min = (uy1 - lg) / g_GridRoutingSize;
        row_max = (uy0 + lg + demi_pas) / g_GridRoutingSize;
    }

    if( row_min < 0 )
        row_min = 0;
    if( row_min > (Nrows - 1) )
        row_min = Nrows - 1;
    if( row_max < 0 )
        row_max = 0;
    if( row_max > (Nrows - 1) )
        row_max = Nrows - 1;


    dx = ux1 - ux0; dy = uy1 - uy0;
    if( dx )
        angle = (int) (atan2( (double)dy, (double)dx ) * 1800 / M_PI);
    else
    {
        angle = 900; if( dy < 0 )
            angle = -900;
    }

    RotatePoint( &dx, &dy, angle );   /* dx = longueur, dy = 0 */
    for( col = col_min; col <= col_max; col++ )
    {
        int cxr;
        cxr = (col * g_GridRoutingSize) - ux0;
        for( row = row_min; row <= row_max;  row++ )
        {
            cy = (row * g_GridRoutingSize) - uy0;
            cx = cxr;
            RotatePoint( &cx, &cy, angle );
            if( abs( cy ) > lg )
                continue;               /* Le point est trop loin sur l'axe Y */

            /* ici le point a tester est proche du segment: la position
             *  selon l'axe X doit etre testee */
            if( (cx >= 0) && (cx <= dx) )
            {
                OP_CELL( layer, row, col );
                continue;
            }
            /* examen des extremites qui sont arrondies */
            if( (cx < 0) && (cx >= -lg) )
            {
                if( ( (cx * cx) + (cy * cy) ) <= (lg * lg) )
                    OP_CELL( layer, row, col );
                continue;
            }
            if( (cx > dx) && ( cx <= (dx + lg) ) )
            {
                if( ( ( (cx - dx) * (cx - dx) ) + (cy * cy) ) <= (lg * lg) )
                    OP_CELL( layer, row, col );
                continue;
            }
        }
    }
}


/********************************************************************/
void DrawHVSegment( int ux0, int uy0, int ux1, int uy1, int demi_largeur, int layer,
                    int color, int op_logique )
/********************************************************************/

/* Draw a horizontal or vertical segment.
 *  same as DrawSegmentQcq, but faster
 */
{
    int  row, col;
    int  row_max, col_max, row_min, col_min;

    void (*WriteCell)( int, int, int, BoardCell );

    switch( op_logique )
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

    // Modif des coord pour que les coord de fin soient > coord de debut
    if( uy1 < uy0 )
        EXCHG( uy0, uy1 );              // ceci n'est vrai que parce que
    if( ux1 < ux0 )
        EXCHG( ux0, ux1 );              // dx ou dy ou les 2 sonts nuls

    // Le segment est assimile a un rectangle.
    // TODO: traiter correctement les extremites arrondies
//	if ( ux0 == ux1 )	// Vertical Segment
    {
        ux0 -= demi_largeur; ux1 += demi_largeur;
    }

//	else	// Horizontal segment
    {
        uy0 -= demi_largeur; uy1 += demi_largeur;
    }

    // Calcul des coord limites des cellules appartenant au rectangle
    row_max = uy1 / g_GridRoutingSize;
    col_max = ux1 / g_GridRoutingSize;

    row_min = uy0 / g_GridRoutingSize;
    if( uy0 > row_min * g_GridRoutingSize )
        row_min++;                                   // Traitement de l'arrondi par defaut

    col_min = ux0 / g_GridRoutingSize;
    if( ux0 > col_min * g_GridRoutingSize )
        col_min++;

    if( row_min < 0 )
        row_min = 0;
    if( row_max >= (Nrows - 1) )
        row_max = Nrows - 1;
    if( col_min < 0 )
        col_min = 0;
    if( col_max >= (Ncols - 1) )
        col_max = Ncols - 1;

    if( row_min > row_max )
        row_max = row_min;
    if( col_min > col_max )
        col_max = col_min;

    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            OP_CELL( layer, row, col );
        }
    }
}


/*****************************************************************/
void TraceCercle( int ux0, int uy0, int ux1, int uy1, int lg, int layer,
                  int color, int op_logique )
/*****************************************************************/

/* Remplit toutes les cellules du BOARD contenues dans le cercle
 *  de demi-largeur lg, centre ux,y0 passant par ux,y1 a la valeur color .
 *  coord en unites PCB (0.1 mil) relatives a l'origine pt_pcb->m_PcbBox.m_Xmin,Y du board.
 */
{
    int rayon, nb_segm;
    int x0, y0, // Point de depart du segment en cours de trace
        x1, y1; // point d'arrivee
    int ii;
    int angle;

    rayon = (int) hypot( (double) (ux1 - ux0), (double) (uy1 - uy0) );

    x0 = x1 = rayon; y0 = y1 = 0;
    if( lg < 1 )
        lg = 1;
    nb_segm = (2 * rayon) / lg;
    if( nb_segm < 5 )
        nb_segm = 5;
    if( nb_segm > 100 )
        nb_segm = 100;
    for( ii = 1; ii < nb_segm; ii++ )
    {
        angle = (3600 * ii) / nb_segm;
        x1    = (int) (rayon * fcosinus[angle]);
        y1    = (int) (rayon * fsinus[angle]);
        DrawSegmentQcq( x0 + ux0, y0 + uy0, x1 + ux0, y1 + uy0, lg, layer, color, op_logique );
        x0 = x1; y0 = y1;
    }

    DrawSegmentQcq( x1 + ux0, y1 + uy0, ux0 + rayon, uy0, lg, layer, color, op_logique );
}


/****************************************************************************/
void TraceArc( int ux0, int uy0, int ux1, int uy1, int ArcAngle, int lg, int layer,
               int color, int op_logique )
/****************************************************************************/

/* Remplit toutes les cellules du BOARD contenues dans l'arc de "longueur" angle
 *  de demi-largeur lg, centre ux,y0 commencant en ux,y1 a la valeur color .
 *  coord en unites PCB (0.1 mil) relatives a l'origine
 *  pt_pcb->m_PcbBox.m_Xmin,Y du board.
 */
{
    int rayon, nb_segm;
    int x0, y0, // Point de depart du segment en cours de trace
        x1, y1; // point d'arrivee
    int ii;
    int angle, StAngle;


    rayon = (int) hypot( (double) (ux1 - ux0), (double) (uy1 - uy0) );

    x0      = ux1 - ux0;
    y0      = uy1 - uy0;
    StAngle = ArcTangente( uy1 - uy0, ux1 - ux0 );
    if( lg < 1 )
        lg = 1;
    nb_segm = (2 * rayon) / lg;
    nb_segm = ( nb_segm * abs( ArcAngle ) ) / 3600;
    if( nb_segm < 5 )
        nb_segm = 5;
    if( nb_segm > 100 )
        nb_segm = 100;

    for( ii = 1; ii <= nb_segm; ii++ )
    {
        angle  = (ArcAngle * ii) / nb_segm;
        angle += StAngle;

        while( angle >= 3600 )
            angle -= 3600;

        while( angle < 0 )
            angle += 3600;

        x1 = (int) (rayon * fcosinus[angle]);
        y1 = (int) (rayon * fsinus[angle]);
        DrawSegmentQcq( x0 + ux0, y0 + uy0, x1 + ux0, y1 + uy0, lg, layer, color, op_logique );
        x0 = x1; y0 = y1;
    }

//	  DrawSegmentQcq(x1+ux0,y1+uy0, ux0+rayon, uy0,lg,layer, color, op_logique);
}

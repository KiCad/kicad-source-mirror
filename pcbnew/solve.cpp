/***************************************/
/* AUTOROUTAGE PCB : routine de calcul */
/***************************************/

/* fichier SOLVE.Cpp */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "protos.h"

#include <fcntl.h>
#include "cell.h"

/* Routines definies ici : */
static int  Route_1_Trace( WinEDA_PcbFrame* pcbframe, wxDC* DC, int two_sides, int row_source,
                           int col_source,
                           int row_target, int col_target, CHEVELU* pt_chevelu );
static int  Retrace( WinEDA_PcbFrame* pcbframe, wxDC* DC, int, int, int, int, int, int net_code );
static void OrCell_Trace( BOARD* pcb, int col, int row, int side, int orient, int current_net_code );
static void Place_Piste_en_Buffer( WinEDA_PcbFrame* pcbframe, wxDC* DC );

/* Variables locales : */
static int      segm_oX, segm_oY;
static int      segm_fX, segm_fY;   /* Origine et fin de la piste en cours de trace */
static CHEVELU* pt_cur_ch;
static int      Ncurrent;           /* measures of progress */


#define NOSUCCESS       0
#define STOP_FROM_ESC   -1
#define ERR_MEMORY      -2
#define SUCCESS         1
#define TRIVIAL_SUCCESS 2

/*
** visit neighboring cells like this (where [9] is on the other side):
**
**	+---+---+---+
**	| 1 | 2 | 3 |
**	+---+---+---+
**	| 4 |[9]| 5 |
**	+---+---+---+
**	| 6 | 7 | 8 |
**	+---+---+---+
*/

/* for visiting neighbors on the same side: increments/decrements des coord
 *  [][0] = row, []{1] = col a ajouter aux coord du point central pour
 *  obtenir les coord des 8 points voisins */
static int delta[8][2] = {
    {  1, -1 },     /* northwest	*/
    {  1, 0  },     /* north		*/
    {  1, 1  },     /* northeast	*/
    {  0, -1 },     /* west		*/
    {  0, 1  },     /* east		*/
    { -1, -1 },     /* southwest	*/
    { -1, 0  },     /* south		*/
    { -1, 1  }      /* southeast	*/
};

static int ndir[8] = { /* for building paths back to source */
    FROM_SOUTHEAST, FROM_SOUTH, FROM_SOUTHWEST,
    FROM_EAST,      FROM_WEST,
    FROM_NORTHEAST, FROM_NORTH, FROM_NORTHWEST
};

/* blocking masks for neighboring cells */
#define BLOCK_NORTHEAST     ( DIAG_NEtoSW | BENT_StoNE | BENT_WtoNE \
                              | ANGLE_NEtoSE | ANGLE_NWtoNE \
                              | SHARP_NtoNE | SHARP_EtoNE | HOLE )
#define BLOCK_SOUTHEAST     ( DIAG_SEtoNW | BENT_NtoSE | BENT_WtoSE \
                              | ANGLE_NEtoSE | ANGLE_SEtoSW \
                              | SHARP_EtoSE | SHARP_StoSE | HOLE )
#define BLOCK_SOUTHWEST     ( DIAG_NEtoSW | BENT_NtoSW | BENT_EtoSW \
                              | ANGLE_SEtoSW | ANGLE_SWtoNW \
                              | SHARP_StoSW | SHARP_WtoSW | HOLE )
#define BLOCK_NORTHWEST     ( DIAG_SEtoNW | BENT_EtoNW | BENT_StoNW \
                              | ANGLE_SWtoNW | ANGLE_NWtoNE \
                              | SHARP_WtoNW | SHARP_NtoNW | HOLE )
#define BLOCK_NORTH         ( LINE_VERTICAL | BENT_NtoSE | BENT_NtoSW \
                              | BENT_EtoNW | BENT_WtoNE \
                              | BENT_StoNE | BENT_StoNW \
                              | CORNER_NORTHEAST | CORNER_NORTHWEST \
                              | ANGLE_NEtoSE | ANGLE_SWtoNW | ANGLE_NWtoNE \
                              | DIAG_NEtoSW | DIAG_SEtoNW \
                              | SHARP_NtoNE | SHARP_NtoNW \
                              | SHARP_EtoNE | SHARP_WtoNW | HOLE )
#define BLOCK_EAST          ( LINE_HORIZONTAL | BENT_EtoSW | BENT_EtoNW \
                              | BENT_NtoSE | BENT_StoNE \
                              | BENT_WtoNE | BENT_WtoSE \
                              | CORNER_NORTHEAST | CORNER_SOUTHEAST \
                              | ANGLE_NEtoSE | ANGLE_SEtoSW | ANGLE_NWtoNE \
                              | DIAG_NEtoSW | DIAG_SEtoNW \
                              | SHARP_EtoNE | SHARP_EtoSE \
                              | SHARP_NtoNE | SHARP_StoSE | HOLE )
#define BLOCK_SOUTH         ( LINE_VERTICAL | BENT_StoNE | BENT_StoNW \
                              | BENT_EtoSW | BENT_WtoSE \
                              | BENT_NtoSE | BENT_NtoSW \
                              | CORNER_SOUTHEAST | CORNER_SOUTHWEST \
                              | ANGLE_NEtoSE | ANGLE_SWtoNW | ANGLE_SEtoSW \
                              | DIAG_NEtoSW | DIAG_SEtoNW \
                              | SHARP_StoSE | SHARP_StoSW \
                              | SHARP_EtoSE | SHARP_WtoSW | HOLE )
#define BLOCK_WEST          ( LINE_HORIZONTAL | BENT_WtoNE | BENT_WtoSE \
                              | BENT_NtoSW | BENT_StoNW \
                              | BENT_EtoSW | BENT_EtoNW \
                              | CORNER_SOUTHWEST | CORNER_NORTHWEST \
                              | ANGLE_SWtoNW | ANGLE_SEtoSW | ANGLE_NWtoNE \
                              | DIAG_NEtoSW | DIAG_SEtoNW \
                              | SHARP_WtoSW | SHARP_WtoNW \
                              | SHARP_NtoNW | SHARP_StoSW | HOLE )

struct block
{
    int  r1, c1;
    long b1;
    int  r2, c2;
    long b2;
};

static struct block blocking[8] =   /* blocking masks for diagonal traces */
{                                                                              {  0, -1,
                                                                                  BLOCK_NORTHEAST,
                                                                                  1,  0,
                                                                                  BLOCK_SOUTHWEST },
                                                                               {  0, 0,  0,
                                                                                  0,  0, 0               },
                                                                               {  1, 0,
                                                                                  BLOCK_SOUTHEAST,
                                                                                  0,  1,
                                                                                  BLOCK_NORTHWEST },
                                                                               {  0, 0,  0,
                                                                                  0,  0, 0               },
                                                                               {  0, 0,  0,
                                                                                  0,  0, 0               },
                                                                               {  0, -1,
                                                                                  BLOCK_SOUTHEAST,
                                                                                  -1, 0,
                                                                                  BLOCK_NORTHWEST },
                                                                               {  0, 0,  0,
                                                                                  0,  0, 0               },
                                                                               { -1, 0,
                                                                                 BLOCK_NORTHEAST,
                                                                                 0,  1,
                                                                                 BLOCK_SOUTHWEST } };

/* mask for hole-related blocking effects */
static struct
{
    long trace;
    int  present;
}           selfok2[8] = {
    { HOLE_NORTHWEST, 0 },
    { HOLE_NORTH,     0 },
    { HOLE_NORTHEAST, 0 },
    { HOLE_WEST,      0 },
    { HOLE_EAST,      0 },
    { HOLE_SOUTHWEST, 0 },
    { HOLE_SOUTH,     0 },
    { HOLE_SOUTHEAST, 0 }
};

static long newmask[8] = { /* patterns to mask out in neighbor cells */
    0,                                   CORNER_NORTHWEST | CORNER_NORTHEAST, 0,
    CORNER_NORTHWEST | CORNER_SOUTHWEST, CORNER_NORTHEAST | CORNER_SOUTHEAST,
    0,                                   CORNER_SOUTHWEST | CORNER_SOUTHEAST, 0
};


/* Macro d'affichage de l'activite du routeur; */
#define AFFICHE_ACTIVITE_ROUTE \
    msg.Printf( wxT( "%5.5d" ), OpenNodes ); \
    Affiche_1_Parametre( pcbframe, 24, wxT( "Open" ), msg, WHITE ); \
    msg.Printf( wxT( "%5.5d" ), ClosNodes ); \
    Affiche_1_Parametre( pcbframe, 32, wxT( "Closed" ), msg, WHITE ); \
    msg.Printf( wxT( "%5.5d" ), MoveNodes ); \
    Affiche_1_Parametre( pcbframe, 40, wxT( "Moved" ), msg, WHITE ); \
    msg.Printf( wxT( "%5.5d" ), MaxNodes ); \
    Affiche_1_Parametre( pcbframe, 48, wxT( "Max" ), msg, WHITE ); \
    msg.Printf( wxT( "%2.2d" ), (ClosNodes * 50) / (Nrows * Ncols) ); \
    Affiche_1_Parametre( pcbframe, 56, wxT( "%" ), msg, CYAN );


/********************************************************/
/* int WinEDA_PcbFrame::Solve(wxDC * DC, int two_sides) */
/********************************************************/

/* route all traces
 *     Return: 1 si OK
 *             -1 si Escape (arret en cours de routage) demande
 *             -2 si defaut alloc memoire
 */

int WinEDA_PcbFrame::Solve( wxDC* DC, int two_sides )
{
    int      current_net_code;
    int      row_source, col_source, row_target, col_target;
    int      success, nbsucces = 0, nbunsucces = 0;
    EQUIPOT* pt_equipot;
    bool     stop = FALSE;
    wxString msg;

    DrawPanel->m_AbortRequest = FALSE;
    DrawPanel->m_AbortEnable  = TRUE;

    Ncurrent = 0;
    MsgPanel->EraseMsgBox();
    msg.Printf( wxT( "%d  " ), m_Pcb->m_NbNoconnect );
    Affiche_1_Parametre( this, 72, wxT( "NoConn" ), msg, LIGHTCYAN );


    /* go until no more work to do */
    GetWork( &row_source, &col_source, &current_net_code,
             &row_target, &col_target, &pt_cur_ch ); // 1er chevelu a router

    for( ; row_source != ILLEGAL; GetWork( &row_source, &col_source,
                                           &current_net_code, &row_target, &col_target,
                                           &pt_cur_ch ) )
    {
        /* Tst demande d'arret de routage ( key ESCAPE actionnee ) */
        wxYield();
        if( DrawPanel->m_AbortRequest )
        {
            if( IsOK( this, _( "Abort routing?" ) ) )
            {
                success = STOP_FROM_ESC;
                stop    = TRUE;
                break;
            }
            else
                DrawPanel->m_AbortRequest = 0;
        }

        Ncurrent++;
        pt_equipot = m_Pcb->FindNet( current_net_code );
        if( pt_equipot )
        {
            msg.Printf( wxT( "[%8.8s]" ), pt_equipot->m_Netname.GetData() );
            Affiche_1_Parametre( this, 1, wxT( "Net route" ), msg, YELLOW );
            msg.Printf( wxT( "%d / %d" ), Ncurrent, Ntotal );
            Affiche_1_Parametre( this, 12, wxT( "Activity" ), msg, YELLOW );
        }

        pt_cur_ch = pt_cur_ch;
        segm_oX   = m_Pcb->m_BoundaryBox.m_Pos.x + (g_GridRoutingSize * col_source);
        segm_oY   = m_Pcb->m_BoundaryBox.m_Pos.y + (g_GridRoutingSize * row_source);
        segm_fX   = m_Pcb->m_BoundaryBox.m_Pos.x + (g_GridRoutingSize * col_target);
        segm_fY   = m_Pcb->m_BoundaryBox.m_Pos.y + (g_GridRoutingSize * row_target);

        /* Affiche Liaison */
        GRLine( &DrawPanel->m_ClipBox, DC, segm_oX, segm_oY, segm_fX, segm_fY, 0, WHITE | GR_XOR );
        pt_cur_ch->pad_start->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR | GR_SURBRILL );
        pt_cur_ch->pad_end->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR | GR_SURBRILL );

        success = Route_1_Trace( this, DC, two_sides, row_source, col_source,
                                 row_target, col_target, pt_cur_ch );

        switch( success )
        {
        case NOSUCCESS:
            pt_cur_ch->status |= CH_UNROUTABLE;
            nbunsucces++;
            break;

        case STOP_FROM_ESC:
            stop = TRUE;
            break;

        case ERR_MEMORY:
            stop = TRUE;
            break;

        default:
            nbsucces++;
            break;
        }

        msg.Printf( wxT( "%d  " ), nbsucces );
        Affiche_1_Parametre( this, 61, wxT( "Ok" ), msg, LIGHTGREEN );
        msg.Printf( wxT( "%d  " ), nbunsucces );
        Affiche_1_Parametre( this, 66, wxT( "Fail" ), msg, LIGHTRED );
        msg.Printf( wxT( "%d  " ), m_Pcb->m_NbNoconnect );
        Affiche_1_Parametre( this, 72, wxT( "NoConn" ), msg, LIGHTCYAN );

        /* Effacement des affichages de routage sur l'ecran */
        pt_cur_ch->pad_start->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_AND );
        pt_cur_ch->pad_end->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_AND );

        if( stop )
            break;
    }

    DrawPanel->m_AbortEnable = FALSE;

    return SUCCESS;
}


/**************************/
/* int Route_1_Trace(xxx) */
/**************************/

/* Route une piste du BOARD.
 *  Parametres:
 *      1 face / 2 faces ( 0 / 1)
 *      coord source (row,col)
 *      coord destination (row,col)
 *      net_code
 *      pointeur sur le chevelu de reference
 * 
 *  Retourne :
 *      SUCCESS si route trouvee
 *      TRIVIAL_SUCCESS si pads connectes par superposition ( pas de piste a tirer)
 *      NOSUCCESS si echec
 *      STOP_FROM_ESC si Escape demande
 *      ERR_MEMORY defaut alloc RAM
 */
static int Route_1_Trace( WinEDA_PcbFrame* pcbframe, wxDC* DC,
                          int two_sides, int row_source, int col_source,
                          int row_target, int col_target, CHEVELU* pt_chevelu )
{
    int        r, c, side, d, apx_dist, nr, nc;
    int        result, skip;
    int        i;
    LISTE_PAD* ptr;
    long       curcell, newcell, buddy, lastopen, lastclos, lastmove;
    int        newdist, olddir, _self;
    int        current_net_code;
    int        marge, via_marge;
    int        pad_masque_layer_s;  /* Masque des couches appartenant au pad de depart */
    int        pad_masque_layer_e;  /* Masque des couches appartenant au pad d'arrivee */
    int        masque_layer_TOP    = g_TabOneLayerMask[Route_Layer_TOP];
    int        masque_layer_BOTTOM = g_TabOneLayerMask[Route_Layer_BOTTOM];
    int        masque_layers; /* Masque des 2 couches de routage */
    int        tab_mask[2];/* permet le calcul du Masque de la couche en cours
                         *  de tst (side = TOP ou BOTTOM)*/
    int        start_mask_layer = 0;
    wxString   msg;

    result = NOSUCCESS;

    marge     = g_DesignSettings.m_TrackClearence + (g_DesignSettings.m_CurrentTrackWidth / 2);
    via_marge = g_DesignSettings.m_TrackClearence + (g_DesignSettings.m_CurrentViaSize / 2);

    /* clear direction flags */
    i = Nrows * Ncols * sizeof(char);
    memset( Board.m_DirSide[TOP], FROM_NOWHERE, i );
    memset( Board.m_DirSide[BOTTOM], FROM_NOWHERE, i );

    lastopen = lastclos = lastmove = 0;

    /* Init tab_masque[side] pour tests de fin de routage */
    tab_mask[TOP]    = masque_layer_TOP;
    tab_mask[BOTTOM] = masque_layer_BOTTOM;
    /* Init masque des couches actives */
    masque_layers = masque_layer_TOP | masque_layer_BOTTOM;

    pt_cur_ch = pt_chevelu;
    current_net_code   = pt_chevelu->GetNet();
    pad_masque_layer_s = pt_cur_ch->pad_start->m_Masque_Layer;
    pad_masque_layer_e = pt_cur_ch->pad_end->m_Masque_Layer;

    /* Test 1 Si routage possible c.a.d si les pads sont accessibles
     *      sur les couches de routage */

    if( (masque_layers & pad_masque_layer_s) == 0 )
        goto end_of_route;
    if( (masque_layers & pad_masque_layer_e) == 0 )
        goto end_of_route;

    /* Test 2 Si routage possible c.a.d si les pads sont accessibles
     *  sur la grille de routage ( 1 point de grille doit etre dans le pad)*/
    {
        int cX = (g_GridRoutingSize * col_source) + pcbframe->m_Pcb->m_BoundaryBox.m_Pos.x;
        int cY = (g_GridRoutingSize * row_source) + pcbframe->m_Pcb->m_BoundaryBox.m_Pos.y;
        int dx = pt_cur_ch->pad_start->m_Size.x / 2;
        int dy = pt_cur_ch->pad_start->m_Size.y / 2;
        int px = pt_cur_ch->pad_start->m_Pos.x;
        int py = pt_cur_ch->pad_start->m_Pos.y;

        if( ( (pt_cur_ch->pad_start->m_Orient / 900) & 1 ) != 0 )
            EXCHG( dx, dy );
        if( (abs( cX - px ) > dx ) || (abs( cY - py ) > dy) )
            goto end_of_route;

        cX = (g_GridRoutingSize * col_target) + pcbframe->m_Pcb->m_BoundaryBox.m_Pos.x;
        cY = (g_GridRoutingSize * row_target) + pcbframe->m_Pcb->m_BoundaryBox.m_Pos.y;
        dx = pt_cur_ch->pad_end->m_Size.x / 2;
        dy = pt_cur_ch->pad_end->m_Size.y / 2;
        px = pt_cur_ch->pad_end->m_Pos.x;
        py = pt_cur_ch->pad_end->m_Pos.y;
        if( ( (pt_cur_ch->pad_end->m_Orient / 900) & 1 ) != 0 )
            EXCHG( dx, dy );

        if( (abs( cX - px ) > dx ) || (abs( cY - py ) > dy) )
            goto end_of_route;
    }

    /* Test du cas trivial: connection directe par superposition des pads */
    if( (row_source == row_target) && (col_source == col_target)
       && ( pad_masque_layer_e & pad_masque_layer_s &
            g_TabAllCopperLayerMask[g_DesignSettings.m_CopperLayerCount - 1]) )
    {
        result = TRIVIAL_SUCCESS;
        goto end_of_route;
    }


    /* Placement du bit de suppression d'obstacle relative aux 2 pads a relier */
    pcbframe->Affiche_Message( wxT( "Gen Cells" ) );

    Place_1_Pad_Board( pcbframe->m_Pcb, pt_cur_ch->pad_start, CURRENT_PAD, marge, WRITE_OR_CELL );
    Place_1_Pad_Board( pcbframe->m_Pcb, pt_cur_ch->pad_end, CURRENT_PAD, marge, WRITE_OR_CELL );

    /* Regenere les barrieres restantes (qui peuvent empieter sur le placement
     *  des bits precedents) */
    ptr = (LISTE_PAD*) pcbframe->m_Pcb->m_Pads; i = pcbframe->m_Pcb->m_NbPads;
    for( ; i > 0; i--, ptr++ )
    {
        if( (pt_cur_ch->pad_start != *ptr) && (pt_cur_ch->pad_end != *ptr) )
        {
            Place_1_Pad_Board( pcbframe->m_Pcb, *ptr, ~CURRENT_PAD, marge, WRITE_AND_CELL );
        }
    }

    InitQueue(); /* initialize the search queue */
    apx_dist = GetApxDist( row_source, col_source, row_target, col_target );

    /* Init 1ere recherche */
    if( two_sides )   /* orientation preferentielle */
    {
        if( abs( row_target - row_source ) > abs( col_target - col_source ) )
        {
            if( pad_masque_layer_s & masque_layer_TOP )
            {
                start_mask_layer = 2;
                if( SetQueue( row_source, col_source, TOP, 0, apx_dist,
                              row_target, col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }
            if( pad_masque_layer_s & masque_layer_BOTTOM )
            {
                start_mask_layer |= 1;

                if( SetQueue( row_source, col_source, BOTTOM, 0, apx_dist,
                              row_target, col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }
        }
        else
        {
            if( pad_masque_layer_s & masque_layer_BOTTOM )
            {
                start_mask_layer = 1;
                if( SetQueue( row_source, col_source, BOTTOM, 0, apx_dist,
                              row_target, col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }
            if( pad_masque_layer_s & masque_layer_TOP )
            {
                start_mask_layer |= 2;

                if( SetQueue( row_source, col_source, TOP, 0, apx_dist,
                              row_target, col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }
        }
    }
    else if( pad_masque_layer_s & masque_layer_BOTTOM )
    {
        start_mask_layer = 1;

        if( SetQueue( row_source, col_source, BOTTOM, 0, apx_dist,
                      row_target, col_target ) == 0 )
        {
            return ERR_MEMORY;
        }
    }

    /* search until success or we exhaust all possibilities */
    GetQueue( &r, &c, &side, &d, &apx_dist );
    for( ; r != ILLEGAL; GetQueue( &r, &c, &side, &d, &apx_dist ) )
    {
        curcell = GetCell( r, c, side );
        if( curcell & CURRENT_PAD )
            curcell &= ~HOLE;
        if( (r == row_target) && (c == col_target)  /* success si layer OK */
           && ( tab_mask[side] & pad_masque_layer_e) )
        {
            /* Efface Liaison */
            GRSetDrawMode( DC, GR_XOR );
            GRLine( &pcbframe->DrawPanel->m_ClipBox,
                    DC,
                    segm_oX,
                    segm_oY,
                    segm_fX,
                    segm_fY,
                    0,
                    WHITE );

            /* Generation de la trace */
            if( Retrace( pcbframe, DC, row_source, col_source,
                         row_target, col_target, side, current_net_code ) )
            {
                result = SUCCESS;   /* Success : Route OK */
            }
            break;                  /* Fin du routage */
        }
        /* report every 300 new nodes or so */
        if( (OpenNodes - lastopen > 300) || (ClosNodes - lastclos > 300) ||
           (MoveNodes - lastmove > 300) )
        {
            lastopen = (OpenNodes / 300) * 300; lastclos = (ClosNodes / 300) * 300;
            lastmove = (MoveNodes / 300) * 300;

            if( pcbframe->DrawPanel->m_AbortRequest )
            {
                result = STOP_FROM_ESC; break;
            }
            AFFICHE_ACTIVITE_ROUTE;
        }

        _self = 0;
        if( curcell & HOLE )
        {
            _self = 5;
            /* set 'present' bits */
            for( i = 0; i < 8; i++ )
            {
                selfok2[i].present = 0;
                if( (curcell & selfok2[i].trace) )
                    selfok2[i].present = 1;
            }
        }

        for( i = 0; i < 8; i++ ) /* consider neighbors */
        {
            nr = r + delta[i][0]; nc = c + delta[i][1];

            /* off the edge? */
            if( nr < 0 || nr >= Nrows || nc < 0 || nc >= Ncols )
                continue;/* off the edge */

            if( _self == 5 && selfok2[i].present )
                continue;
            newcell = GetCell( nr, nc, side );
            if( newcell & CURRENT_PAD )
                newcell &= ~HOLE;

            /* check for non-target hole */
            if( newcell & HOLE )
            {
                if( nr != row_target || nc != col_target )
                    continue;
            }
            /* check for traces */
            else if( newcell & HOLE & ~(newmask[i]) )
                continue;

            /* check blocking on corner neighbors */
            if( delta[i][0] && delta[i][1] )
            {
                /* check first buddy */
                buddy = GetCell( r + blocking[i].r1, c + blocking[i].c1, side );
                if( buddy & CURRENT_PAD )
                    buddy &= ~HOLE;
                if( buddy & HOLE )
                    continue;

//				if (buddy & (blocking[i].b1)) continue;
                /* check second buddy */
                buddy = GetCell( r + blocking[i].r2, c + blocking[i].c2, side );
                if( buddy & CURRENT_PAD )
                    buddy &= ~HOLE;
                if( buddy & HOLE )
                    continue;

//				if (buddy & (blocking[i].b2)) continue;
            }

            olddir  = GetDir( r, c, side );
            newdist = d + CalcDist( ndir[i], olddir,
                                    (olddir == FROM_OTHERSIDE) ? GetDir( r,
                                                                         c,
                                                                         1 - side ) : 0, side );

            /* if (a) not visited yet, or (b) we have */
            /* found a better path, add it to queue */
            if( !GetDir( nr, nc, side ) )
            {
                SetDir( nr, nc, side, ndir[i] );
                SetDist( nr, nc, side, newdist );
                if( SetQueue( nr, nc, side, newdist,
                              GetApxDist( nr, nc, row_target, col_target ),
                              row_target, col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }
            else if( newdist < GetDist( nr, nc, side ) )
            {
                SetDir( nr, nc, side, ndir[i] );
                SetDist( nr, nc, side, newdist );
                ReSetQueue( nr, nc, side, newdist,
                            GetApxDist( nr, nc, row_target, col_target ),
                            row_target, col_target );
            }
        }

        /** etude de l'autre couche **/
        if( (two_sides) && !g_No_Via_Route )
        {
            olddir = GetDir( r, c, side );
            if( olddir == FROM_OTHERSIDE )
                continue;   /* useless move, so don't bother */
            if( curcell )   /* can't drill via if anything here */
                continue;
            /* check for holes or traces on other side */
            if( ( newcell = GetCell( r, c, 1 - side ) ) != 0 )
                continue;
            /* check for nearby holes or traces on both sides */
            for( skip = 0, i = 0; i < 8; i++ )
            {
                nr = r + delta[i][0]; nc = c + delta[i][1];

                if( nr < 0 || nr >= Nrows || nc < 0 || nc >= Ncols )
                    continue;/* off the edge !! */

                if( GetCell( nr, nc, side ) /* & blocking2[i]*/ )
                {
                    skip = 1; /* can't drill via here */
                    break;
                }

                if( GetCell( nr, nc, 1 - side ) /* & blocking2[i]*/ )
                {
                    skip = 1; /* can't drill via here */
                    break;
                }
            }

            if( skip )      /* neighboring hole or trace? */
                continue;   /* yes, can't drill via here */

            newdist = d + CalcDist( FROM_OTHERSIDE, olddir, 0, side );

            /*  if (a) not visited yet,
             *  or (b) we have found a better path,
             *  add it to queue */
            if( !GetDir( r, c, 1 - side ) )
            {
                SetDir( r, c, 1 - side, FROM_OTHERSIDE );
                SetDist( r, c, 1 - side, newdist );
                if( SetQueue( r, c, 1 - side, newdist, apx_dist, row_target,
                              col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }
            else if( newdist < GetDist( r, c, 1 - side ) )
            {
                SetDir( r, c, 1 - side, FROM_OTHERSIDE );
                SetDist( r, c, 1 - side, newdist );
                ReSetQueue( r, c, 1 - side, newdist, apx_dist, row_target, col_target );
            }
        }     /* Fin de l'exploration de l'autre couche */
    }

end_of_route:
    Place_1_Pad_Board( pcbframe->m_Pcb, pt_cur_ch->pad_start, ~CURRENT_PAD, marge, WRITE_AND_CELL );
    Place_1_Pad_Board( pcbframe->m_Pcb, pt_cur_ch->pad_end, ~CURRENT_PAD, marge, WRITE_AND_CELL );

    AFFICHE_ACTIVITE_ROUTE;
    return result;
}


static long bit[8][9] = { /* OT=Otherside */
    /* N, NE, E, SE, S, SW, W, NW, OT */
/* N */  { LINE_VERTICAL,    BENT_StoNE,   CORNER_SOUTHEAST, SHARP_StoSE,  0,
           SHARP_StoSW, CORNER_SOUTHWEST, BENT_StoNW, (HOLE | HOLE_SOUTH) },
/* NE */ { BENT_NtoSW,       DIAG_NEtoSW,  BENT_EtoSW,       ANGLE_SEtoSW, SHARP_StoSW,
           0, SHARP_WtoSW, ANGLE_SWtoNW, (HOLE | HOLE_SOUTHWEST) },
/* E */  { CORNER_NORTHWEST, BENT_WtoNE,   LINE_HORIZONTAL,  BENT_WtoSE,
           CORNER_SOUTHWEST, SHARP_WtoSW, 0, SHARP_WtoNW, (HOLE | HOLE_WEST) },
/* SE */ { SHARP_NtoNW,      ANGLE_NWtoNE, BENT_EtoNW,       DIAG_SEtoNW,  BENT_StoNW,
           ANGLE_SWtoNW, SHARP_WtoNW, 0, (HOLE | HOLE_NORTHWEST) },
/* S */  { 0,                SHARP_NtoNE,  CORNER_NORTHEAST, BENT_NtoSE,   LINE_VERTICAL,
           BENT_NtoSW, CORNER_NORTHWEST, SHARP_NtoNW, (HOLE | HOLE_NORTH) },
/* SW */ { SHARP_NtoNE,      0,            SHARP_EtoNE,      ANGLE_NEtoSE, BENT_StoNE,
           DIAG_NEtoSW,
           BENT_WtoNE, ANGLE_NWtoNE, (HOLE | HOLE_NORTHEAST) },
/* W */  { CORNER_NORTHEAST, SHARP_EtoNE,  0,                SHARP_EtoSE,  CORNER_SOUTHEAST,
           BENT_EtoSW, LINE_HORIZONTAL, BENT_EtoNW, (HOLE | HOLE_EAST) },
/* NW */ { BENT_NtoSE,       ANGLE_NEtoSE, SHARP_EtoSE,      0,            SHARP_StoSE,
           ANGLE_SEtoSW, BENT_WtoSE, DIAG_SEtoNW, (HOLE | HOLE_SOUTHEAST) }
};

/*****************************************************************/
/* int Retrace (COMMAND * Cmd, int row_source, int col_source	 */
/*				int row_target, int col_target, int target_side, */
/*				 		int current_net_code )					 */
/*****************************************************************/

/* work from target back to source, actually laying the traces
 *  Parametres:
 *      start on side target_side, aux coordonnees row_target, col_target.
 *      arrivee sur side masque_layer_start, coord row_source, col_source
 *  La recherche se fait en sens inverse du routage,
 *   c.a.d du point d'arrivee (target) vers le point de depart (source)
 *      du routeur.
 * 
 *  target_side = cote (TOP / BOTTOM) de depart
 *  mask_layer_source = masque des couches d'arrivee
 * 
 *  Retourne:
 *      0 si erreur
 *      > 0 si Ok
 */

static int Retrace( WinEDA_PcbFrame* pcbframe, wxDC* DC,
                    int row_source, int col_source,
                    int row_target, int col_target, int target_side,
                    int current_net_code )
{
    int  r0, c0, s0;
    int  r1, c1, s1;    /* row, col, side d'ou on vient */
    int  r2, c2, s2;    /* row, col, side ou on va */
    int  x, y = -1;
    long b;

    r1 = row_target;
    c1 = col_target;    /* start point is target ( end point is source )*/
    s1 = target_side;
    r0 = c0 = s0 = ILLEGAL;

    g_FirstTrackSegment = g_CurrentTrackSegment = NULL;
    g_TrackSegmentCount = 0;

    do {
        /* find where we came from to get here */
        r2 = r1; c2 = c1; s2 = s1;
        x  = GetDir( r1, c1, s1 );

        switch( x )
        {
        case FROM_NORTH:
            r2++;   break;

        case FROM_EAST:
            c2++;   break;

        case FROM_SOUTH:
            r2--;   break;

        case FROM_WEST:
            c2--;   break;

        case FROM_NORTHEAST:
            r2++;   c2++;   break;

        case FROM_SOUTHEAST:
            r2--;   c2++;   break;

        case FROM_SOUTHWEST:
            r2--;   c2--;   break;

        case FROM_NORTHWEST:
            r2++;   c2--;   break;

        case FROM_OTHERSIDE:
            s2 = 1 - s2;  break;

        default:
            DisplayError( pcbframe, wxT( "Retrace: internal error: no way back" ) );
            return 0;
        }

        if( r0 != ILLEGAL )
            y = GetDir( r0, c0, s0 );

        /* see if target or hole */
        if( ( (r1 == row_target) && (c1 == col_target) )
           || (s1 != s0) )
        {
            int p_dir;

            switch( x )
            {
            case FROM_NORTH:
                p_dir = HOLE_NORTH; break;

            case FROM_EAST:
                p_dir = HOLE_EAST; break;

            case FROM_SOUTH:
                p_dir = HOLE_SOUTH; break;

            case FROM_WEST:
                p_dir = HOLE_WEST;  break;

            case FROM_NORTHEAST:
                p_dir = HOLE_NORTHEAST; break;

            case FROM_SOUTHEAST:
                p_dir = HOLE_SOUTHEAST; break;

            case FROM_SOUTHWEST:
                p_dir = HOLE_SOUTHWEST; break;

            case FROM_NORTHWEST:
                p_dir = HOLE_NORTHWEST; break;

            case FROM_OTHERSIDE:
            default:
                DisplayError( pcbframe, wxT( "Retrace: error 1" ) );
                return 0;
            }

            OrCell_Trace( pcbframe->m_Pcb, r1, c1, s1, p_dir, current_net_code );
        }
        else
        {
            if( (y == FROM_NORTH || y == FROM_NORTHEAST
                 || y == FROM_EAST || y == FROM_SOUTHEAST
                 || y == FROM_SOUTH || y == FROM_SOUTHWEST
                 || y == FROM_WEST || y == FROM_NORTHWEST)
               && (x == FROM_NORTH || x == FROM_NORTHEAST
                   || x == FROM_EAST || x == FROM_SOUTHEAST
                   || x == FROM_SOUTH || x == FROM_SOUTHWEST
                   || x == FROM_WEST || x == FROM_NORTHWEST
                   || x == FROM_OTHERSIDE)
               && ( (b = bit[y - 1][x - 1]) != 0 ) )
            {
                OrCell_Trace( pcbframe->m_Pcb, r1, c1, s1, b, current_net_code );
                if( b & HOLE )
                    OrCell_Trace( pcbframe->m_Pcb, r2, c2, s2, HOLE, current_net_code );
            }
            else
            {
                DisplayError( pcbframe, wxT( "Retrace: error 2" ) );
                return 0;
            }
        }

        if( (r2 == row_source) && (c2 == col_source) )
        {     /* see if source */
            int p_dir;

            switch( x )
            {
            case FROM_NORTH:
                p_dir = HOLE_SOUTH; break;

            case FROM_EAST:
                p_dir = HOLE_WEST;  break;

            case FROM_SOUTH:
                p_dir = HOLE_NORTH; break;

            case FROM_WEST:
                p_dir = HOLE_EAST; break;

            case FROM_NORTHEAST:
                p_dir = HOLE_SOUTHWEST; break;

            case FROM_SOUTHEAST:
                p_dir = HOLE_NORTHWEST; break;

            case FROM_SOUTHWEST:
                p_dir = HOLE_NORTHEAST; break;

            case FROM_NORTHWEST:
                p_dir = HOLE_SOUTHEAST; break;

            case FROM_OTHERSIDE:
            default:
                DisplayError( pcbframe, wxT( "Retrace: error 3" ) );
                return 0;
            }

            OrCell_Trace( pcbframe->m_Pcb, r2, c2, s2, p_dir, current_net_code );
        }
        /* move to next cell */
        r0 = r1; c0 = c1; s0 = s1;
        r1 = r2; c1 = c2; s1 = s2;
    } while( !( (r2 == row_source) && (c2 == col_source) ) );

    Place_Piste_en_Buffer( pcbframe, DC );
    return 1;
}


/*****************************************************************************/
static void OrCell_Trace( BOARD* pcb, int col, int row,
                          int side, int orient, int current_net_code )
/*****************************************************************************/
/* appelle la routine OrCell et place la piste reelle sur le pcb */
{
    int    dx0, dy0, dx1, dy1;
    TRACK* NewTrack, * OldTrack;

    if( orient == HOLE )  /* Placement d'une VIA */
    {
        NewTrack = new SEGVIA( pcb );

        g_TrackSegmentCount++;
        NewTrack->Pback = g_CurrentTrackSegment;
        if( g_CurrentTrackSegment )
            g_CurrentTrackSegment->Pnext = NewTrack;
        else
            g_FirstTrackSegment = NewTrack;

        g_CurrentTrackSegment = NewTrack;

        g_CurrentTrackSegment->SetState( SEGM_AR, ON );
        g_CurrentTrackSegment->SetLayer( 0x0F );
        g_CurrentTrackSegment->m_Start.x = g_CurrentTrackSegment->m_End.x =
                                               pcb->m_BoundaryBox.m_Pos.x +
                                               (g_GridRoutingSize * row);
        g_CurrentTrackSegment->m_Start.y = g_CurrentTrackSegment->m_End.y =
                                               pcb->m_BoundaryBox.m_Pos.y +
                                               (g_GridRoutingSize * col);
        g_CurrentTrackSegment->m_Width   = g_DesignSettings.m_CurrentViaSize;
        g_CurrentTrackSegment->m_Shape   = g_DesignSettings.m_CurrentViaType;
        g_CurrentTrackSegment->SetNet( current_net_code );
    }
    else    /* Placement d'un segment standard */
    {
        NewTrack = new TRACK( pcb );

        g_TrackSegmentCount++;
        NewTrack->Pback = g_CurrentTrackSegment;
        if( g_CurrentTrackSegment )
            g_CurrentTrackSegment->Pnext = NewTrack;
        else
            g_FirstTrackSegment = NewTrack;

        g_CurrentTrackSegment = NewTrack;

        g_CurrentTrackSegment->SetLayer( Route_Layer_BOTTOM );
        if( side == TOP )
            g_CurrentTrackSegment->SetLayer( Route_Layer_TOP );

        g_CurrentTrackSegment->SetState( SEGM_AR, ON );
        g_CurrentTrackSegment->m_End.x   = pcb->m_BoundaryBox.m_Pos.x + (g_GridRoutingSize * row);
        g_CurrentTrackSegment->m_End.y   = pcb->m_BoundaryBox.m_Pos.y + (g_GridRoutingSize * col);
        g_CurrentTrackSegment->SetNet( current_net_code );

        if( g_CurrentTrackSegment->Pback == NULL ) /* Start Piste */
        {
            g_CurrentTrackSegment->m_Start.x = segm_fX;
            g_CurrentTrackSegment->m_Start.y = segm_fY;

            /* Replacement sur le centre du pad si hors grille */
            dx1 = g_CurrentTrackSegment->m_End.x - g_CurrentTrackSegment->m_Start.x;
            dy1 = g_CurrentTrackSegment->m_End.y - g_CurrentTrackSegment->m_Start.y;
            dx0 = pt_cur_ch->pad_end->m_Pos.x - g_CurrentTrackSegment->m_Start.x;
            dy0 = pt_cur_ch->pad_end->m_Pos.y - g_CurrentTrackSegment->m_Start.y;

            /* si aligne: modif du point origine */
            if( abs( dx0 * dy1 ) == abs( dx1 * dy0 ) ) /* Alignes ! */
            {
                g_CurrentTrackSegment->m_Start.x = pt_cur_ch->pad_end->m_Pos.x;
                g_CurrentTrackSegment->m_Start.y = pt_cur_ch->pad_end->m_Pos.y;
            }
            else /* Creation d'un segment suppl raccord */
            {
                NewTrack = g_CurrentTrackSegment->Copy();
                g_TrackSegmentCount++;
                NewTrack->Insert( pcb, g_CurrentTrackSegment );

                g_CurrentTrackSegment->m_Start.x = pt_cur_ch->pad_end->m_Pos.x;
                g_CurrentTrackSegment->m_Start.y = pt_cur_ch->pad_end->m_Pos.y;
                NewTrack->m_Start.x = g_CurrentTrackSegment->m_End.x;
                NewTrack->m_Start.y = g_CurrentTrackSegment->m_End.y;

                g_CurrentTrackSegment = NewTrack;
            }
        }
        else
        {
            if( g_CurrentTrackSegment->Pback )
            {
                g_CurrentTrackSegment->m_Start.x = ( (TRACK*) g_CurrentTrackSegment->Pback )->
                                                   m_End.x;
                g_CurrentTrackSegment->m_Start.y = ( (TRACK*) g_CurrentTrackSegment->Pback )->
                                                   m_End.y;
            }
        }
        g_CurrentTrackSegment->m_Width = g_DesignSettings.m_CurrentTrackWidth;

        if( (g_CurrentTrackSegment->m_Start.x != g_CurrentTrackSegment->m_End.x)
           || (g_CurrentTrackSegment->m_Start.y != g_CurrentTrackSegment->m_End.y) )
        {
            /* Reduction des segments alignes a 1 seul */
            OldTrack = (TRACK*) g_CurrentTrackSegment->Pback;
            if( OldTrack && (OldTrack->Type() != TYPEVIA) )
            {
                dx1 = g_CurrentTrackSegment->m_End.x - g_CurrentTrackSegment->m_Start.x;
                dy1 = g_CurrentTrackSegment->m_End.y - g_CurrentTrackSegment->m_Start.y;
                dx0 = OldTrack->m_End.x - OldTrack->m_Start.x;
                dy0 = OldTrack->m_End.y - OldTrack->m_Start.y;
                if( abs( dx0 * dy1 ) == abs( dx1 * dy0 ) )/* le dernier segment est en ligne*/
                {
                    OldTrack->m_End.x = g_CurrentTrackSegment->m_End.x;
                    OldTrack->m_End.y = g_CurrentTrackSegment->m_End.y;
                    delete g_CurrentTrackSegment;
                    g_CurrentTrackSegment = OldTrack;
                    g_CurrentTrackSegment->Pnext = NULL;
                    g_TrackSegmentCount--;
                }
            }
        }
    }
}


/*******************************************/
/* static void Place_Piste_en_Buffer() */
/*******************************************/

/* Insere la nouvelle piste creee dans la liste standard des pistes.
 *  Modifie les points de debut et fin de piste pour qu'ils soient relies
 *  au centre des pads corresponadants, meme hors grille
 */
static void Place_Piste_en_Buffer( WinEDA_PcbFrame* pcbframe, wxDC* DC )
{
    TRACK*            pt_track;
    int               dx0, dy0, dx1, dy1;
    int               marge, via_marge;
    WinEDA_DrawPanel* panel = pcbframe->DrawPanel;

    marge     = g_DesignSettings.m_TrackClearence + (g_DesignSettings.m_CurrentTrackWidth / 2);
    via_marge = g_DesignSettings.m_TrackClearence + (g_DesignSettings.m_CurrentViaSize / 2);

    /* tst point d'arrivee : doit etre sur pad start */

    dx1 = g_CurrentTrackSegment->m_End.x - g_CurrentTrackSegment->m_Start.x;
    dy1 = g_CurrentTrackSegment->m_End.y - g_CurrentTrackSegment->m_Start.y;
    /* Replacement sur le centre du pad si hors grille */

    dx0 = pt_cur_ch->pad_start->m_Pos.x - g_CurrentTrackSegment->m_Start.x;
    dy0 = pt_cur_ch->pad_start->m_Pos.y - g_CurrentTrackSegment->m_Start.y;

    /* si aligne: modif du point origine */
    if( abs( dx0 * dy1 ) == abs( dx1 * dy0 ) ) /* Alignes ! */
    {
        g_CurrentTrackSegment->m_End.x = pt_cur_ch->pad_start->m_Pos.x;
        g_CurrentTrackSegment->m_End.y = pt_cur_ch->pad_start->m_Pos.y;
    }
    else /* Creation d'un segment suppl raccord */
    {
        TRACK* NewTrack = g_CurrentTrackSegment->Copy();
        NewTrack->Insert( pcbframe->m_Pcb, g_CurrentTrackSegment );

        NewTrack->m_End.x   = pt_cur_ch->pad_start->m_Pos.x;
        NewTrack->m_End.y   = pt_cur_ch->pad_start->m_Pos.y;
        NewTrack->m_Start.x = g_CurrentTrackSegment->m_End.x;
        NewTrack->m_Start.y = g_CurrentTrackSegment->m_End.y;

        g_CurrentTrackSegment = NewTrack; g_TrackSegmentCount++;
    }


    g_FirstTrackSegment->start = Locate_Pad_Connecte( pcbframe->m_Pcb, g_FirstTrackSegment, START );
    if( g_FirstTrackSegment->start )
        g_FirstTrackSegment->SetState( BEGIN_ONPAD, ON );

    g_CurrentTrackSegment->end = Locate_Pad_Connecte( pcbframe->m_Pcb, g_CurrentTrackSegment, END );
    if( g_CurrentTrackSegment->end )
        g_CurrentTrackSegment->SetState( END_ONPAD, ON );

    /* recherche de la zone de rangement et insertion de la nouvelle piste */
    pt_track = g_FirstTrackSegment->GetBestInsertPoint( pcbframe->m_Pcb );
    g_FirstTrackSegment->Insert( pcbframe->m_Pcb, pt_track );

    Trace_Une_Piste( panel, DC, g_FirstTrackSegment, g_TrackSegmentCount, GR_OR );

    pcbframe->test_1_net_connexion( DC, g_FirstTrackSegment->GetNet() );

    /* Trace de la forme exacte de la piste en BOARD */
    for( pt_track = g_FirstTrackSegment; ; pt_track = (TRACK*) pt_track->Pnext )
    {
        TraceSegmentPcb( pcbframe->m_Pcb, pt_track, HOLE, marge, WRITE_CELL );
        TraceSegmentPcb( pcbframe->m_Pcb, pt_track, VIA_IMPOSSIBLE, via_marge, WRITE_OR_CELL );
        if( pt_track == g_CurrentTrackSegment )
            break;
    }

    ActiveScreen->SetModify();
}

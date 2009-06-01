/*************************************************/
/* Routines de placement automatique des MODULES */
/*************************************************/

/* Fichier autoplac.cpp */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "autorout.h"
#include "zones.h"
#include "cell.h"

#include "protos.h"


/************************************************************/
/* Menu et Routines de placement automatique des composants */
/************************************************************/

#define GAIN     16
#define PENALITE 500

/* Penalite pour orientation donnee par CntRot90 et CntRot180:
 *  gradue de 0 ( rotation interdite ) a 10 ( rotation a cout null )
 *  Le cout est ici donne en majoration
 */
static const float OrientPenality[11] = {
    2.0f,    /* CntRot = 0 en fait rotation interdite */
    1.9f,    /* CntRot = 1 */
    1.8f,    /* CntRot = 2 */
    1.7f,    /* CntRot = 3 */
    1.6f,    /* CntRot = 4 */
    1.5f,    /* CntRot = 5 */
    1.4f,    /* CntRot = 5 */
    1.3f,    /* CntRot = 7 */
    1.2f,    /* CntRot = 8 */
    1.1f,    /* CntRot = 9 */
    1.0f     /* CntRot = 10 rotation autorisee, penalite nulle */
};

/* Etat d'une cellule */
#define OUT_OF_BOARD      -2
#define OCCUPED_By_MODULE -1

/* variables locales */
static wxPoint CurrPosition;    // position courante du module en cours de placement
static bool    AutoPlaceShowAll = TRUE;

float          MinCout;

/* Fonctions locales */
static int      TstModuleOnBoard( BOARD* Pcb, MODULE* Module, bool TstOtherSide );
static void     Build_PlacedPads_List( BOARD* Pcb );
static int      Tri_PlaceModules( MODULE** pt_ref, MODULE** pt_compare );

static void     TracePenaliteRectangle( BOARD* Pcb, int ux0, int uy0, int ux1, int uy1,
                                        int marge, int Penalite, int masque_layer );
static MODULE*  PickModule( WinEDA_PcbFrame* pcbframe, wxDC* DC );


/********************************************************************************/
void WinEDA_PcbFrame::AutoPlaceModule( MODULE* Module, int place_mode, wxDC* DC )
/********************************************************************************/

/* Routine de Placement Automatique des composants dans le contour du PCB
 *  Les composants ayant le status FIXE ne sont pas bouges
 *  Si le menu appelant est le placement de 1 module, il sera replace
 */

{
    int      ii, activ;
    MODULE*  ThisModule = NULL;
    MODULE** BaseListeModules;
    wxPoint  PosOK;
    wxPoint  memopos;
    int      error;
    int      NbModules      = 0;
    int      NbTotalModules = 0;
    float    Pas;
    int      lay_tmp_TOP, lay_tmp_BOTTOM, OldPasRoute;

    if( GetBoard()->m_Modules == NULL )
        return;

    DrawPanel->m_AbortRequest = FALSE;
    DrawPanel->m_AbortEnable  = TRUE;

    switch( place_mode )
    {
    case  PLACE_1_MODULE:
        ThisModule = Module;
        if( ThisModule == NULL )
            return;
        ThisModule->m_ModuleStatus &= ~(MODULE_is_PLACED | MODULE_to_PLACE);
        break;

    case  PLACE_OUT_OF_BOARD:
        break;

    case  PLACE_ALL:
        if( !IsOK( this, _( "Footprints NOT LOCKED will be moved" ) ) )
            return;
        break;

    case  PLACE_INCREMENTAL:
        if( !IsOK( this, _( "Footprints NOT PLACED will be moved" ) ) )
            return;
        break;
    }

    memopos = CurrPosition;
    lay_tmp_BOTTOM = Route_Layer_BOTTOM;
    lay_tmp_TOP    = Route_Layer_TOP;
    OldPasRoute    = g_GridRoutingSize;

    g_GridRoutingSize = (int)GetScreen()->GetGrid().x;

    // Ensure g_GridRoutingSize has a reasonnable value:
    if( g_GridRoutingSize < 10 )
        g_GridRoutingSize = 10;                             // Min value = 1/1000 inch

    /* Compute module parmeters used in auto place */
    Module = GetBoard()->m_Modules;
    for( ; Module != NULL; Module = Module->Next() ) // remise a jour du rect d'encadrement
    {
        Module->Set_Rectangle_Encadrement();
        Module->SetRectangleExinscrit();
    }

    /* Generation du plan de placement */
    if( GenPlaceBoard() == 0 )
        return;

    /* Mise a jour des parametres modules utiles au placement */
    BaseListeModules = GenListeModules( GetBoard(), &NbTotalModules );
    MyFree( BaseListeModules );

    /* Placement des modules fixes sur le plan de placement */
    Module = GetBoard()->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->m_ModuleStatus &= ~MODULE_to_PLACE;

        switch( place_mode )
        {
        case  PLACE_1_MODULE:
            if( ThisModule == Module )
                Module->m_ModuleStatus |= MODULE_to_PLACE;
            break;

        case  PLACE_OUT_OF_BOARD:
            Module->m_ModuleStatus &= ~MODULE_is_PLACED;
            if( Module->m_ModuleStatus & MODULE_is_LOCKED )
                break;
            if( !GetBoard()->m_BoundaryBox.Inside( Module->m_Pos ) )
                Module->m_ModuleStatus |= MODULE_to_PLACE;
            break;

        case  PLACE_ALL:
            Module->m_ModuleStatus &= ~MODULE_is_PLACED;
            if( Module->m_ModuleStatus & MODULE_is_LOCKED )
                break;
            Module->m_ModuleStatus |= MODULE_to_PLACE;
            break;

        case  PLACE_INCREMENTAL:
            if( Module->m_ModuleStatus & MODULE_is_LOCKED )
            {
                Module->m_ModuleStatus &= ~MODULE_is_PLACED; break;
            }
            if( !(Module->m_ModuleStatus & MODULE_is_PLACED) )
                Module->m_ModuleStatus |= MODULE_to_PLACE;
            break;
        }


        if( Module->m_ModuleStatus & MODULE_to_PLACE )  // Erase from screen
        {
            NbModules++;
            Module->Draw( DrawPanel, DC, GR_XOR );
        }
        else
        {
            GenModuleOnBoard( Module );
        }
    }

    /* Placement des modules */
    activ = 0; Pas = 100.0;
    if( NbModules )
        Pas = 100.0 / (float) NbModules;
    while( ( Module = PickModule( this, DC ) ) != NULL )
    {
        float BestScore;
        DisplayActivity( (int) (activ * Pas), wxEmptyString ); activ++;

        /* Affichage du remplissage: surface de placement, obstacles, penalites */
        DrawInfoPlace( DC );

        /* Recherche du placement: orientation 0 */
        error     = RecherchePlacementModule( Module, DC );
        BestScore = MinCout;
        PosOK = CurrPosition;
        if( error == ESC )
            goto end_of_tst;

        /* Recherche du placement: orientation 180 */
        ii = Module->m_CntRot180 & 0x0F;
        if( ii != 0 )
        {
            int Angle_Rot_Module = 1800;
            Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            Module->SetRectangleExinscrit();
            error    = RecherchePlacementModule( Module, DC );
            MinCout *= OrientPenality[ii];
            if( BestScore > MinCout )   /* Cette orientation est meilleure */
            {
                PosOK     = CurrPosition;
                BestScore = MinCout;
            }
            else
            {
                Angle_Rot_Module = -1800;
                Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            }
            if( error == ESC )
                goto end_of_tst;
        }

        /* Recherche du placement: orientation 90 */
        ii = Module->m_CntRot90 & 0x0F;
        if( ii != 0 )
        {
            int Angle_Rot_Module = 900;
            Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            error    = RecherchePlacementModule( Module, DC );
            MinCout *= OrientPenality[ii];
            if( BestScore > MinCout )   /* Cette orientation est meilleure */
            {
                PosOK     = CurrPosition;
                BestScore = MinCout;
            }
            else
            {
                Angle_Rot_Module = -900;
                Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            }
            if( error == ESC )
                goto end_of_tst;
        }

        /* Recherche du placement: orientation -90 (ou 270 degres) */
        ii = (Module->m_CntRot90 >> 4 ) & 0x0F;
        if( ii != 0 )
        {
            int Angle_Rot_Module = 2700;
            Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            error    = RecherchePlacementModule( Module, DC );
            MinCout *= OrientPenality[ii];
            if( BestScore > MinCout )   /* Cette orientation est meilleure */
            {
                PosOK     = CurrPosition;
                BestScore = MinCout;
            }
            else
            {
                Angle_Rot_Module = -2700;
                Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            }
            if( error == ESC )
                goto end_of_tst;
        }

end_of_tst:

        if( error == ESC )
            break;

        /* placement du module */
        CurrPosition = GetScreen()->m_Curseur;
        GetScreen()->m_Curseur = PosOK;
        Place_Module( Module, DC );
        GetScreen()->m_Curseur = CurrPosition;

        Module->Set_Rectangle_Encadrement();
        Module->SetRectangleExinscrit();

        GenModuleOnBoard( Module );
        Module->m_ModuleStatus |= MODULE_is_PLACED;
        Module->m_ModuleStatus &= ~MODULE_to_PLACE;
    }

    CurrPosition = memopos;

    /* Liberation de la memoire */
    Board.UnInitBoard();

    Route_Layer_TOP    = lay_tmp_TOP;
    Route_Layer_BOTTOM = lay_tmp_BOTTOM;
    g_GridRoutingSize  = OldPasRoute;

    Module = GetBoard()->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->Set_Rectangle_Encadrement();
    }

    /* Recalcul de la liste des pads, detruite par les calculs precedents */
    GetBoard()->m_Status_Pcb = 0;
    GetBoard()->Build_Pads_Full_List();

    DrawPanel->ReDraw( DC, TRUE );

    DrawPanel->m_AbortEnable = FALSE;
}


/**********************************************/
void WinEDA_PcbFrame::DrawInfoPlace( wxDC* DC )
/**********************************************/

/* Affiche a l'ecran les infos de placement
 */
{
    int color, ii, jj;
    int ox, oy;
    BoardCell top_state, bottom_state;

    GRSetDrawMode( DC, GR_COPY );
    for( ii = 0; ii < Nrows; ii++ )
    {
        oy = GetBoard()->m_BoundaryBox.m_Pos.y + (ii * g_GridRoutingSize);

        for( jj = 0; jj < Ncols; jj++ )
        {
            ox = GetBoard()->m_BoundaryBox.m_Pos.x + (jj * g_GridRoutingSize);
            /* surface de placement : */
            color = BLACK;

            top_state    = GetCell( ii, jj, TOP );
            bottom_state = GetCell( ii, jj, BOTTOM );

            if( (top_state & CELL_is_ZONE) )
                color = BLUE;

            /* obstacles */
            if( (top_state & CELL_is_EDGE) || (bottom_state & CELL_is_EDGE) )
                color = WHITE;

            else if( top_state & (HOLE | CELL_is_MODULE) )
                color = LIGHTRED;
            else if( bottom_state & (HOLE | CELL_is_MODULE) )
                color = LIGHTGREEN;

            else /* Affichage du remplissage: Penalites */
            {
                if( GetDist( ii, jj, TOP ) || GetDist( ii, jj, BOTTOM ) )
                    color = DARKGRAY;
            }

            GRPutPixel( &DrawPanel->m_ClipBox, DC, ox, oy, color );
        }
    }
}


/***************************************/
int WinEDA_PcbFrame::GenPlaceBoard()
/***************************************/

/* Routine de generation du board ( cote composant + cote cuivre ) :
 *  Alloue la memoire necessaire pour representer en "bitmap" sur la grille
 *  courante:
 *  - la surface de placement des composant ( le board )
 *  - le bitmap des penalites
 *  et initialise les cellules du board a
 *   - HOLE pour les cellules occupees par un segment EDGE
 *   - CELL_is_ZONE pour les cellules internes au contour EDGE (s'il est ferme)
 *
 *  la surface de placement (board) donne les cellules internes au contour
 *  du pcb, et parmi celle-ci les cellules libres et les cellules deja occupees
 *
 *  le bitmap des penalites donnent les cellules occupes par les modules,
 *  augmentes d'une surface de penalite liee au nombre de pads du module
 *
 *  le bitmap des penalites est mis a 0
 *  l'occupation des cellules est laisse a 0
 */
{
    int             jj, ii;
    int             NbCells;
    EDA_BaseStruct* PtStruct;
    wxString        msg;

    Board.UnInitBoard();

    if( !SetBoardBoundaryBoxFromEdgesOnly() )
    {
        DisplayError( this, _( "No edge PCB, Unknown board size!" ), 30 );
        return 0;
    }

    /* The boundary box must have its start point on placing grid: */
    GetBoard()->m_BoundaryBox.m_Pos.x -= GetBoard()->m_BoundaryBox.m_Pos.x % g_GridRoutingSize;
    GetBoard()->m_BoundaryBox.m_Pos.y -= GetBoard()->m_BoundaryBox.m_Pos.y % g_GridRoutingSize;
    /* The boundary box must have its end point on placing grid: */
    wxPoint end = GetBoard()->m_BoundaryBox.GetEnd();
    end.x -= end.x % g_GridRoutingSize; end.x += g_GridRoutingSize;
    end.y -= end.y % g_GridRoutingSize; end.y += g_GridRoutingSize;
    GetBoard()->m_BoundaryBox.SetEnd( end );

    Nrows = GetBoard()->m_BoundaryBox.GetHeight() / g_GridRoutingSize;
    Ncols = GetBoard()->m_BoundaryBox.GetWidth() / g_GridRoutingSize;
    /* get a small margin for memory allocation: */
    Ncols  += 2; Nrows += 2;
    NbCells = Ncols * Nrows;

    MsgPanel->EraseMsgBox();
    msg.Printf( wxT( "%d" ), Ncols );
    Affiche_1_Parametre( this, 1, _( "Cols" ), msg, GREEN );
    msg.Printf( wxT( "%d" ), Nrows );
    Affiche_1_Parametre( this, 7, _( "Lines" ), msg, GREEN );
    msg.Printf( wxT( "%d" ), NbCells );
    Affiche_1_Parametre( this, 14, _( "Cells." ), msg, YELLOW );

    /* Choix du nombre de faces de placement */
    Nb_Sides = TWO_SIDES;

    Affiche_1_Parametre( this, 22, wxT( "S" ), ( Nb_Sides == TWO_SIDES ) ? wxT( "2" ) : wxT(
                             "1" ), WHITE );

    /* Creation du mapping du board */
    Board.InitBoard();

    /* Affichage de la memoire utilisee */
    msg.Printf( wxT( "%d" ), Board.m_MemSize / 1024 );
    Affiche_1_Parametre( this, 24, wxT( "Mem(Ko)" ), msg, CYAN );

    Route_Layer_BOTTOM = CMP_N;
    if( Nb_Sides == TWO_SIDES )
        Route_Layer_BOTTOM = COPPER_LAYER_N;
    Route_Layer_TOP = CMP_N;

    /* Place the edge layer segments */
    PtStruct = GetBoard()->m_Drawings;
    TRACK TmpSegm( NULL );

    TmpSegm.SetLayer( -1 );
    TmpSegm.SetNet( -1 );
    TmpSegm.m_Width   = g_GridRoutingSize / 2;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        DRAWSEGMENT* DrawSegm;

        switch( PtStruct->Type() )
        {
        case TYPE_DRAWSEGMENT:
            DrawSegm = (DRAWSEGMENT*) PtStruct;
            if( DrawSegm->GetLayer() != EDGE_N )
                break;

            TmpSegm.m_Start = DrawSegm->m_Start;
            TmpSegm.m_End   = DrawSegm->m_End;
            TmpSegm.m_Shape = DrawSegm->m_Shape;
            TmpSegm.m_Param = DrawSegm->m_Angle;

            TraceSegmentPcb( GetBoard(), &TmpSegm, HOLE | CELL_is_EDGE, g_GridRoutingSize, WRITE_CELL );
            break;

        case TYPE_TEXTE:
        default:
            break;
        }
    }

    /* Init du point d'accrochage de la zone */
    OrCell( Nrows / 2, Ncols / 2, BOTTOM, CELL_is_ZONE );

    /* Remplissage des cellules de la couche BOTTOM */
    ii = 1; jj = 1;

    while( ii )
    {
        msg.Printf( wxT( "%d" ), jj++ );
        Affiche_1_Parametre( this, 50, _( "Loop" ), msg, CYAN );
        ii = Propagation( this );
    }

    /* Init de la couche TOP */
    if( Board.m_BoardSide[TOP] )
        memcpy( Board.m_BoardSide[TOP], Board.m_BoardSide[BOTTOM], NbCells * sizeof(BoardCell) );

    return 1;
}


/******************************************************/
void WinEDA_PcbFrame::GenModuleOnBoard( MODULE* Module )
/******************************************************/

/* initialise sur le board de placement les cellules correspondantes au
 *  module Module
 */
{
    int    ox, oy, fx, fy, Penalite;
    int    marge = g_GridRoutingSize / 2;
    int    masque_layer;
    D_PAD* Pad;

    ox = Module->m_RealBoundaryBox.m_Pos.x - marge;
    fx = Module->m_RealBoundaryBox.GetRight() + marge;
    oy = Module->m_RealBoundaryBox.m_Pos.y - marge;
    fy = Module->m_RealBoundaryBox.GetBottom() + marge;

    if( ox < GetBoard()->m_BoundaryBox.m_Pos.x )
        ox = GetBoard()->m_BoundaryBox.m_Pos.x;
    if( ox > GetBoard()->m_BoundaryBox.GetRight() )
        ox = GetBoard()->m_BoundaryBox.GetRight();

    if( fx < GetBoard()->m_BoundaryBox.m_Pos.x )
        fx = GetBoard()->m_BoundaryBox.m_Pos.x;
    if( fx > GetBoard()->m_BoundaryBox.GetRight() )
        fx = GetBoard()->m_BoundaryBox.GetRight();

    if( oy < GetBoard()->m_BoundaryBox.m_Pos.y )
        oy = GetBoard()->m_BoundaryBox.m_Pos.y;
    if( oy > GetBoard()->m_BoundaryBox.GetBottom() )
        oy = GetBoard()->m_BoundaryBox.GetBottom();

    if( fy < GetBoard()->m_BoundaryBox.m_Pos.y )
        fy = GetBoard()->m_BoundaryBox.m_Pos.y;
    if( fy > GetBoard()->m_BoundaryBox.GetBottom() )
        fy = GetBoard()->m_BoundaryBox.GetBottom();

    masque_layer = 0;
    if( Module->GetLayer() == CMP_N )
        masque_layer = CMP_LAYER;
    if( Module->GetLayer() == COPPER_LAYER_N )
        masque_layer = CUIVRE_LAYER;

    TraceFilledRectangle( GetBoard(), ox, oy, fx, fy, masque_layer,
                          CELL_is_MODULE, WRITE_OR_CELL );

    /* Trace des pads et leur surface de securite */
    marge = g_DesignSettings.m_TrackClearence + g_DesignSettings.m_CurrentTrackWidth;

    for( Pad = Module->m_Pads; Pad != NULL; Pad = Pad->Next() )
    {
        Place_1_Pad_Board( GetBoard(), Pad, CELL_is_MODULE, marge, WRITE_OR_CELL );
    }

    /* Trace de la penalite */
    marge    = (g_GridRoutingSize * Module->m_PadNum ) / GAIN;
    Penalite = PENALITE;
    TracePenaliteRectangle( GetBoard(), ox, oy, fx, fy, marge, Penalite,
                            masque_layer );
}


/************************************************************************/
int WinEDA_PcbFrame::RecherchePlacementModule( MODULE* Module, wxDC* DC )
/************************************************************************/

/*
 *  Routine Principale de recherche de la position optimale du module
 *  Entree:
 *      Module pointe la struct MODULE du module a placer.
 *  Retourne:
 *      1 si placement impossible, 0 si OK
 *      et MinCout = variable externe = cout du meilleur placement
 */
{
    int     cx, cy;
    int     ox, oy, fx, fy;/* cadre d'occupation du module centre sur le curseur */
    int     error = 1;
    int     DisplayChevelu = 0;
    wxPoint LastPosOK;
    float   mincout, cout, Score;
    int     Penalite;
    bool    TstOtherSide;

    Module->DisplayInfo( this );

    Build_PlacedPads_List( GetBoard() );

    LastPosOK.x = GetBoard()->m_BoundaryBox.m_Pos.x;
    LastPosOK.y = GetBoard()->m_BoundaryBox.m_Pos.y;

    cx = Module->m_Pos.x; cy = Module->m_Pos.y;
    ox = Module->m_RealBoundaryBox.m_Pos.x - cx;
    fx = Module->m_RealBoundaryBox.m_Size.x + ox;
    oy = Module->m_RealBoundaryBox.m_Pos.y - cy;
    fy = Module->m_RealBoundaryBox.m_Size.y + oy;

    CurrPosition.x = GetBoard()->m_BoundaryBox.m_Pos.x - ox;
    CurrPosition.y = GetBoard()->m_BoundaryBox.m_Pos.y - oy;
    /* remise sur la grille de placement: */
    CurrPosition.x -= CurrPosition.x % g_GridRoutingSize;
    CurrPosition.y -= CurrPosition.y % g_GridRoutingSize;

    g_Offset_Module.x    = cx - CurrPosition.x;
    g_Offset_Module.y    = cy - CurrPosition.y;
    GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;

    /* tst des pastilles traversantes, qui pour un circuit imprime ayant des
     *  composants des 2 cotes, peuvent tomber sur un composant de cote oppose:
     *  s'il y a au moins 1 pastille apparaissant sur l'autre cote, ce cote
     *  est teste  */

    TstOtherSide = FALSE;
    if( Nb_Sides == TWO_SIDES )
    {
        D_PAD* Pad; int masque_otherlayer;
        masque_otherlayer = CUIVRE_LAYER;
        if( Module->GetLayer() == COPPER_LAYER_N )
            masque_otherlayer = CMP_LAYER;

        for( Pad = Module->m_Pads; Pad != NULL; Pad = Pad->Next() )
        {
            if( (Pad->m_Masque_Layer & masque_otherlayer) == 0 )
                continue;
            TstOtherSide = TRUE;
            break;
        }
    }


    DrawModuleOutlines( DrawPanel, DC, Module );

    mincout = -1.0;
    Affiche_Message( wxT( "Score ??, pos ??" ) );

    for( ; CurrPosition.x < GetBoard()->m_BoundaryBox.GetRight() - fx;
         CurrPosition.x += g_GridRoutingSize )
    {
        wxYield();
        if( DrawPanel->m_AbortRequest )
        {
            if( IsOK( this, _( "Ok to abort ?" ) ) )
                return ESC;
            else
                DrawPanel->m_AbortRequest = FALSE;
        }

        cx = Module->m_Pos.x; cy = Module->m_Pos.y;
        Module->m_RealBoundaryBox.m_Pos.x = ox + CurrPosition.x;
        Module->m_RealBoundaryBox.m_Pos.y = oy + CurrPosition.y;

        DrawModuleOutlines( DrawPanel, DC, Module );

        g_Offset_Module.x = cx - CurrPosition.x;
        CurrPosition.y    = GetBoard()->m_BoundaryBox.m_Pos.y - oy;
        /* remise sur la grille de placement: */
        CurrPosition.y -= CurrPosition.y % g_GridRoutingSize;

        DrawModuleOutlines( DrawPanel, DC, Module );

        for( ; CurrPosition.y < GetBoard()->m_BoundaryBox.GetBottom() - fy;
             CurrPosition.y += g_GridRoutingSize )
        {
            /* effacement des traces */
            DrawModuleOutlines( DrawPanel, DC, Module );
            if( DisplayChevelu )
                Compute_Ratsnest_PlaceModule( DC );
            DisplayChevelu = 0;
            Module->m_RealBoundaryBox.m_Pos.x = ox + CurrPosition.x;
            Module->m_RealBoundaryBox.m_Pos.y = oy + CurrPosition.y;

            g_Offset_Module.y = cy - CurrPosition.y;
            DrawModuleOutlines( DrawPanel, DC, Module );
            Penalite = TstModuleOnBoard( GetBoard(), Module, TstOtherSide );
            if( Penalite >= 0 ) /* c a d si le module peut etre place */
            {
                error = 0;
                build_ratsnest_module( DC, Module );
                cout = Compute_Ratsnest_PlaceModule( DC );
                DisplayChevelu = 1;
                Score = cout + (float) Penalite;

                if( (mincout >= Score ) || (mincout < 0 ) )
                {
                    LastPosOK = CurrPosition;
                    mincout   = Score;
                    wxString msg;
                    msg.Printf( wxT( "Score %d, pos %3.4f, %3.4f" ),
                                (int) mincout,
                                (float) LastPosOK.x / 10000, (float) LastPosOK.y / 10000 );
                    Affiche_Message( msg );
                }
            }
            if( DisplayChevelu )
                Compute_Ratsnest_PlaceModule( DC );
            DisplayChevelu = 0;
        }
    }

    DrawModuleOutlines( DrawPanel, DC, Module );  /* effacement du dernier trace */
    if( DisplayChevelu )
        Compute_Ratsnest_PlaceModule( DC );

    /* Regeneration des variables modifiees */
    Module->m_RealBoundaryBox.m_Pos.x = ox + cx;
    Module->m_RealBoundaryBox.m_Pos.y = oy + cy;
    CurrPosition = LastPosOK;

    GetBoard()->m_Status_Pcb &= ~(RATSNEST_ITEM_LOCAL_OK | LISTE_PAD_OK );

    MinCout = mincout;
    return error;
}


/**************************************************************************/
int TstRectangle( BOARD* Pcb, int ux0, int uy0, int ux1, int uy1, int side )
/**************************************************************************/

/* tst si la surface rectangulaire (ux,y0 .. ux,y1):
 *  - est sur une zone libre ( retourne OCCUPED_By_MODULE sinon)
 *  - est sur la surface utile du board ( retourne OUT_OF_BOARD sinon)
 *
 *  retourne 0 si OK
 */
{
    int          row, col;
    int          row_min, row_max, col_min, col_max;
    unsigned int data;

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
            data = GetCell( row, col, side );
            if( (data & CELL_is_ZONE) == 0 )    /* Cellule non autorisee */
                return OUT_OF_BOARD;
            if( data & CELL_is_MODULE )         /* Deja utilisee */
                return OCCUPED_By_MODULE;
        }
    }

    return 0;
}


/******************************************************************************/
unsigned int CalculePenaliteRectangle( BOARD* Pcb, int ux0, int uy0,
                                       int ux1, int uy1, int side )
/******************************************************************************/

/* calcule et retourne la penalite de la surface rectangulaire (ux,y0 .. ux,y1):
 *  ( somme des valeurs des cellules du plan des Distances )
 */
{
    int          row, col;
    int          row_min, row_max, col_min, col_max;
    unsigned int Penalite;

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

    Penalite = 0;
    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            Penalite += (int) GetDist( row, col, side );
        }
    }

    return Penalite;
}


/**********************************************************************/
int TstModuleOnBoard( BOARD* Pcb, MODULE* Module, bool TstOtherSide )
/**********************************************************************/

/*  Teste si le module peut etre place sur le board.
 *  retourne de diagnostic de TstRectangle().
 *  le module est connu par son rectangle d'encadrement
 */
{
    int ox, oy, fx, fy;
    int error, Penalite, marge, side, otherside;

    side = TOP; otherside = BOTTOM;
    if( Module->GetLayer() == COPPER_LAYER_N )
    {
        side = BOTTOM; otherside = TOP;
    }

    ox = Module->m_RealBoundaryBox.m_Pos.x;
    fx = Module->m_RealBoundaryBox.GetRight();
    oy = Module->m_RealBoundaryBox.m_Pos.y;
    fy = Module->m_RealBoundaryBox.GetBottom();

    error = TstRectangle( Pcb, ox, oy, fx, fy, side );
    if( error < 0 )
        return error;

    if( TstOtherSide )
    {
        error = TstRectangle( Pcb, ox, oy, fx, fy, otherside );
        if( error < 0 )
            return error;
    }

    marge = (g_GridRoutingSize * Module->m_PadNum ) / GAIN;

    Penalite = CalculePenaliteRectangle( Pcb, ox - marge, oy - marge,
                                         fx + marge, fy + marge, side );
    return Penalite;
}


/************************************************************/
float WinEDA_PcbFrame::Compute_Ratsnest_PlaceModule( wxDC* DC )
/************************************************************/

/* Routine affichant le chevelu du module en cours de deplacement, et
 *  evaluant le "cout" de la position.
 *  Le cout est la longueur des chevelus en distance de manhattan, avec
 *  penalite pour les inclinaisons se rapprochant de 45 degre
 */
{
    double    cout, icout;
    int      ox, oy;
    int      fx, fy;
    int      dx, dy;

    if( (GetBoard()->m_Status_Pcb & RATSNEST_ITEM_LOCAL_OK) == 0 )
        return -1;
    cout = 0;

    for( unsigned ii = 0; ii < GetBoard()->m_LocalRatsnest.size(); ii++ )
    {
        RATSNEST_ITEM* pt_local_chevelu = &GetBoard()->m_LocalRatsnest[ii];
        if( !(pt_local_chevelu->m_Status & LOCAL_RATSNEST_ITEM) )
        {
            ox = pt_local_chevelu->m_PadStart->GetPosition().x - g_Offset_Module.x;
            oy = pt_local_chevelu->m_PadStart->GetPosition().y - g_Offset_Module.y;
            fx = pt_local_chevelu->m_PadEnd->GetPosition().x;
            fy = pt_local_chevelu->m_PadEnd->GetPosition().y;

            if( AutoPlaceShowAll )
            {
                GRLine( &DrawPanel->m_ClipBox, DC, ox, oy, fx, fy,
                        0, g_DesignSettings.m_RatsnestColor | GR_XOR );
            }

            /* Evaluation du cout du chevelu: */
            dx = fx - ox;
            dy = fy - oy;

            dx = abs( dx );
            dy = abs( dy );

            if( dx < dy )
                EXCHG( dx, dy );/* dx >= dy */

            /* cout de la distance: */
            icout = (float) dx * dx;

            /* cout de l'inclinaison */
            icout += 3 * (float) dy * dy;
            icout  = sqrt( icout );
            cout  += icout; /* cout total = somme des couts de chaque chevelu */
        }
    }

    return (float)cout;
}


/********************************************/
void Build_PlacedPads_List( BOARD* aPcb )
/********************************************/

/*
 *  construction de la liste ( sous forme d'une liste de stucture )
 *  des caract utiles des pads du PCB pour Placement Automatique )
 *  Cette liste est restreinte a la liste des pads des modules deja places sur
 *  la carte.
 *
 *  parametres:
 *      adresse du buffer de classement = Pcb->ptr_pads;
 *
 *  Variables globales mise a jour:
 *  pointeur ptr_pads (adr de classement de la liste des pads)
 *  nb_pads = nombre utile de pastilles classes
 *  m_Status_Pcb |= LISTE_PAD_OK
 */
{
    aPcb->m_Pads.clear();

    aPcb->m_NbNodes = 0;

    // Initialisation du buffer et des variables de travail
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        if( module->m_ModuleStatus & MODULE_to_PLACE )
            continue;

        for( D_PAD* pad = module->m_Pads;  pad;  pad = pad->Next() )
        {
            aPcb->m_Pads.push_back( pad );
            pad->SetSubNet( 0 );
            pad->SetSubRatsnest( 0 );
            pad->SetParent( module );
            if( pad->GetNet() )
                aPcb->m_NbNodes++;
        }
    }

    aPcb->m_Status_Pcb |= LISTE_PAD_OK;
    aPcb->m_Status_Pcb &= ~(LISTE_RATSNEST_ITEM_OK | RATSNEST_ITEM_LOCAL_OK);
}


/*****************************************************************/
/* Construction de la zone de penalite ( rectangle ) d'un module */
/*****************************************************************/

/* les cellules ( du plan des Distances ) du rectangle x0,y0 a x1,y1 sont
 *  incrementees de la valeur Penalite
 *  celles qui sont externes au rectangle, mais internes au rectangle
 *  x0,y0 -marge a x1,y1 + marge sont incrementees d'une valeur
 *  (Penalite ... 0) decroissante en fonction de leur eloignement
 */
static void TracePenaliteRectangle( BOARD* Pcb, int ux0, int uy0, int ux1, int uy1,
                                    int marge, int Penalite, int masque_layer )
{
    int      row, col;
    int      row_min, row_max, col_min, col_max, pmarge;
    int      trace = 0;
    DistCell data, LocalPenalite;
    int      lgain, cgain;

    if( masque_layer & g_TabOneLayerMask[Route_Layer_BOTTOM] )
        trace = 1;     /* Trace sur BOTTOM */

    if( (masque_layer & g_TabOneLayerMask[Route_Layer_TOP] ) && Nb_Sides )
        trace |= 2;     /* Trace sur TOP */

    if( trace == 0 )
        return;

    ux0 -= Pcb->m_BoundaryBox.m_Pos.x; uy0 -= Pcb->m_BoundaryBox.m_Pos.y;
    ux1 -= Pcb->m_BoundaryBox.m_Pos.x; uy1 -= Pcb->m_BoundaryBox.m_Pos.y;

    ux0 -= marge; ux1 += marge;
    uy0 -= marge; uy1 += marge;

    pmarge = marge / g_GridRoutingSize; if( pmarge < 1 )
        pmarge = 1;

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
        lgain = 256;
        if( row < pmarge )
            lgain = (256 * row) / pmarge;
        else if( row > row_max - pmarge )
            lgain = ( 256 * (row_max - row) ) / pmarge;

        for( col = col_min; col <= col_max; col++ )
        {
            cgain = 256;
            LocalPenalite = Penalite;
            if( col < pmarge )
                cgain = (256 * col) / pmarge;
            else if( col > col_max - pmarge )
                cgain = ( 256 * (col_max - col) ) / pmarge;

            cgain = (cgain * lgain) / 256;
            if( cgain != 256 )
                LocalPenalite = (LocalPenalite * cgain) / 256;
            if( trace & 1 )
            {
                data = GetDist( row, col, BOTTOM ) + LocalPenalite;
                SetDist( row, col, BOTTOM, data );
            }
            if( trace & 2 )
            {
                data = GetDist( row, col, TOP );
                data = MAX( data, LocalPenalite );
                SetDist( row, col, TOP, data );
            }
        }
    }
}


/***************************************************/
/* Routines de tri de modules, utilisee par qsort: */
/***************************************************/

static int Tri_PlaceModules( MODULE** pt_ref, MODULE** pt_compare )
{
    float ff, ff1, ff2;

    ff1 = (*pt_ref)->m_Surface * (*pt_ref)->m_PadNum;
    ff2 = (*pt_compare)->m_Surface * (*pt_compare)->m_PadNum;
    ff  = ff1 - ff2;
    if( ff < 0 )
        return 1;
    if( ff > 0 )
        return -1;
    return 0;
}


static int Tri_RatsModules( MODULE** pt_ref, MODULE** pt_compare )
{
    float ff, ff1, ff2;

    ff1 = (*pt_ref)->m_Surface * (*pt_ref)->flag;
    ff2 = (*pt_compare)->m_Surface * (*pt_compare)->flag;
    ff  = ff1 - ff2;
    if( ff < 0 )
        return 1;
    if( ff > 0 )
        return -1;
    return 0;
}


/***************************************************************/
static MODULE* PickModule( WinEDA_PcbFrame* pcbframe, wxDC* DC )
/***************************************************************/

/* Recherche le "meilleur" module a placer
 *  les criteres de choix sont:
 *  - maximum de chevelus avec les modules deja places
 *  - taille max, et nombre de pads max
 */
{
    MODULE** BaseListeModules, ** pt_Dmod;
    MODULE*  Module = NULL, * AltModule = NULL;
    int      NbModules;

    BaseListeModules = GenListeModules( pcbframe->GetBoard(), &NbModules );
    if( BaseListeModules == NULL )
        return NULL;

    Build_PlacedPads_List( pcbframe->GetBoard() );

    /* Tri par surface decroissante des modules
     *  (on place les plus gros en 1er), surface ponderee par le nombre de pads */

    qsort( BaseListeModules, NbModules, sizeof(MODULE * *),
           ( int (*)( const void*, const void* ) )Tri_PlaceModules );

    for( pt_Dmod = BaseListeModules; *pt_Dmod != NULL; pt_Dmod++ )
    {
        (*pt_Dmod)->flag = 0;
        if( !( (*pt_Dmod)->m_ModuleStatus & MODULE_to_PLACE ) )
            continue;
        pcbframe->GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;
        (*pt_Dmod)->DisplayInfo( pcbframe );
        pcbframe->build_ratsnest_module( DC, *pt_Dmod );

        /* calcul du nombre de chevelus externes */
        for( unsigned ii = 0; ii < pcbframe->GetBoard()->m_LocalRatsnest.size(); ii++ )
        {
            if( (pcbframe->GetBoard()->m_LocalRatsnest[ii].m_Status & LOCAL_RATSNEST_ITEM) == 0 )
                (*pt_Dmod)->flag++;
        }
    }

    pcbframe->GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;

    qsort( BaseListeModules, NbModules, sizeof(MODULE * *),
           ( int (*)( const void*, const void* ) )Tri_RatsModules );


    /* Recherche du "meilleur" module */
    Module = NULL;
    for( pt_Dmod = BaseListeModules; *pt_Dmod != NULL; pt_Dmod++ )
    {
        if( !( (*pt_Dmod)->m_ModuleStatus & MODULE_to_PLACE ) )
            continue;
        AltModule = *pt_Dmod;
        if( (*pt_Dmod)->flag == 0 )
            continue;
        Module = *pt_Dmod; break;
    }

    MyFree( BaseListeModules );
    if( Module )
        return Module;
    else
        return AltModule;
}


/*******************************************************/
bool WinEDA_PcbFrame::SetBoardBoundaryBoxFromEdgesOnly()
/*******************************************************/

/* Determine le rectangle d'encadrement du pcb, selon les contours
 *  (couche EDGE) uniquement
 *  Sortie:
 *  GetBoard()->m_BoundaryBox mis a jour
 *  Retourne FALSE si pas de contour
 */
{
    int             rayon, cx, cy, d;
    int             xmax, ymax;
    BOARD_ITEM*     PtStruct;
    DRAWSEGMENT*    ptr;
    bool            succes = FALSE;

    if( GetBoard() == NULL )
        return FALSE;

    GetBoard()->m_BoundaryBox.m_Pos.x = GetBoard()->m_BoundaryBox.m_Pos.y = 0x7FFFFFFFl;
    xmax = ymax = -0x7FFFFFFFl;

    /* Analyse des Contours PCB */
    PtStruct = GetBoard()->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        if( PtStruct->Type() != TYPE_DRAWSEGMENT )
            continue;
        succes = TRUE;
        ptr    = (DRAWSEGMENT*) PtStruct;
        d = (ptr->m_Width / 2) + 1;
        if( ptr->m_Shape == S_CIRCLE )
        {
            cx     = ptr->m_Start.x; cy = ptr->m_Start.y;
            rayon  = (int) hypot( (double) (ptr->m_End.x - cx), (double) (ptr->m_End.y - cy) );
            rayon += d;
            GetBoard()->m_BoundaryBox.m_Pos.x = MIN( GetBoard()->m_BoundaryBox.m_Pos.x, cx - rayon );
            GetBoard()->m_BoundaryBox.m_Pos.y = MIN( GetBoard()->m_BoundaryBox.m_Pos.y, cy - rayon );
            xmax = MAX( xmax, cx + rayon );
            ymax = MAX( ymax, cy + rayon );
        }
        else
        {
            cx = MIN( ptr->m_Start.x, ptr->m_End.x );
            cy = MIN( ptr->m_Start.y, ptr->m_End.y );
            GetBoard()->m_BoundaryBox.m_Pos.x = MIN( GetBoard()->m_BoundaryBox.m_Pos.x, cx - d );
            GetBoard()->m_BoundaryBox.m_Pos.y = MIN( GetBoard()->m_BoundaryBox.m_Pos.y, cy - d );
            cx   = MAX( ptr->m_Start.x, ptr->m_End.x );
            cy   = MAX( ptr->m_Start.y, ptr->m_End.y );
            xmax = MAX( xmax, cx + d );
            ymax = MAX( ymax, cy + d );
        }
    }

    GetBoard()->m_BoundaryBox.SetWidth( xmax - GetBoard()->m_BoundaryBox.m_Pos.x );
    GetBoard()->m_BoundaryBox.SetHeight( ymax - GetBoard()->m_BoundaryBox.m_Pos.y );
    return succes;
}

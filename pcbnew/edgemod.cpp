/******************************************************/
/*	 Edition des contours d'un module: Routines		  */
/* d'effacement et d'edition  de segments et contours */
/*	appartenant aux modules							  */
/******************************************************/


/* fichier edgemod.cpp */

/* Routines d'edition des contours d'un module.
 *  La correction des Arcs de cercle n'est pas traitee ( mais
 *  les menus en routines sont prevus
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"


/* Routines Locales */

static void ShowEdgeModule( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void Exit_EditEdge_Module( WinEDA_DrawPanel* Panel, wxDC* DC );
static void Move_Segment( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

/* Variables locales : */
int            ArcValue = 900;
static wxPoint MoveVector;              // Move vector for move edge
static wxPoint CursorInitialPosition;   // Mouse cursor inital position for move command

/****************************************************************************/
void WinEDA_ModuleEditFrame::Start_Move_EdgeMod( EDGE_MODULE* Edge, wxDC* DC )
/****************************************************************************/

/* Routine de preparation du deplacement d'un element graphique type DRAWSEGMENT
 */
{
    if( Edge == NULL )
        return;
    Edge->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
    Edge->m_Flags |= IS_MOVED;
    MoveVector.x   = MoveVector.y = 0;
    CursorInitialPosition    = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = Move_Segment;
    DrawPanel->ForceCloseManageCurseur = Exit_EditEdge_Module;
    GetScreen()->SetCurItem( Edge );
    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
}


/*********************************************************************/
void WinEDA_ModuleEditFrame::Place_EdgeMod( EDGE_MODULE* Edge, wxDC* DC )
/*********************************************************************/

/*
 *  Routine de placement de l'element graphique type EDGE_MODULE en cours de deplacement
 */
{
    if( Edge == NULL )
        return;
    Edge->m_Start.x -= MoveVector.x;
    Edge->m_Start.y -= MoveVector.y;
    Edge->m_End.x   -= MoveVector.x;
    Edge->m_End.y   -= MoveVector.y;

    Edge->m_Start0.x -= MoveVector.x;
    Edge->m_Start0.y -= MoveVector.y;
    Edge->m_End0.x   -= MoveVector.x;
    Edge->m_End0.y   -= MoveVector.y;

    Edge->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
    Edge->m_Flags = 0;
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    GetScreen()->SetCurItem( NULL );
    GetScreen()->SetModify();
    MODULE* Module = (MODULE*) Edge->m_Parent;
    Module->Set_Rectangle_Encadrement();
}


/************************************************************************/
static void Move_Segment( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/************************************************************************/
/* redessin du contour du Segment Edge lors des deplacements de la souris */
{
    BASE_SCREEN* screen = panel->GetScreen();
    EDGE_MODULE* Edge   = (EDGE_MODULE*) screen->GetCurItem();

    if( Edge == NULL )
        return;

    MODULE* Module = (MODULE*) Edge->m_Parent;

    if( erase )
    {
        Edge->Draw( panel, DC, MoveVector, GR_XOR );
    }

    MoveVector.x = -(screen->m_Curseur.x - CursorInitialPosition.x);
    MoveVector.y = -(screen->m_Curseur.y - CursorInitialPosition.y);

    Edge->Draw( panel, DC, MoveVector, GR_XOR );

    Module->Set_Rectangle_Encadrement();
}


/************************************************************************/
static void ShowEdgeModule( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/************************************************************************/

/* Affiche le segment Edge Module en cours de trace
 */
{
    BASE_SCREEN* screen = panel->GetScreen();
    EDGE_MODULE* Edge   = (EDGE_MODULE*) screen->GetCurItem();

    if( Edge == NULL )
        return;

    MODULE* Module = (MODULE*) Edge->m_Parent;

    //	if( erase )
    {
        Edge->Draw( panel, DC, wxPoint( 0, 0 ), GR_XOR );
    }

    Edge->m_End = screen->m_Curseur;

    /* Mise a jour des coord relatives */
    Edge->m_End0.x = Edge->m_End.x - Module->m_Pos.x;
    Edge->m_End0.y = Edge->m_End.y - Module->m_Pos.y;
    RotatePoint( (int*) &Edge->m_End0.x,
                (int*) &Edge->m_End0.y, -Module->m_Orient );

    Edge->Draw( panel, DC, wxPoint( 0, 0 ), GR_XOR );

    Module->Set_Rectangle_Encadrement();
}


/***************************************************************************/
void WinEDA_ModuleEditFrame::Edit_Edge_Width( EDGE_MODULE* Edge, wxDC* DC )
/***************************************************************************/

/* change la largeur du EDGE_MODULE Edge, ou de tous si Edge == NULL
 */
{
    MODULE* Module = m_Pcb->m_Modules;

    SaveCopyInUndoList( Module );

    if( Edge == NULL )
    {
        Edge = (EDGE_MODULE*) Module->m_Drawings;
        for( ; Edge != NULL; Edge = (EDGE_MODULE*) Edge->Pnext )
        {
            if( Edge->m_StructType != TYPEEDGEMODULE )
                continue;
            Edge->m_Width = ModuleSegmentWidth;
        }
    }
    else
        Edge->m_Width = ModuleSegmentWidth;

    GetScreen()->SetModify();
    DrawPanel->Refresh( TRUE );
    Module->Set_Rectangle_Encadrement();
    Module->m_LastEdit_Time = time( NULL );
}


/***************************************************************************/
void WinEDA_ModuleEditFrame::Edit_Edge_Layer( EDGE_MODULE* Edge, wxDC* DC )
/***************************************************************************/

/* change la couche du EDGE_MODULE Edge, ou de tous si Edge == NULL
 */
{
    MODULE* Module    = m_Pcb->m_Modules;
    int     new_layer = SILKSCREEN_N_CMP;


    new_layer = SelectLayer( SILKSCREEN_N_CMP, LAYER_CUIVRE_N, LAST_NO_COPPER_LAYER );
    if( new_layer < 0 )
        return;

    SaveCopyInUndoList( Module );

    if( Edge == NULL )
    {
        Edge = (EDGE_MODULE*) Module->m_Drawings;
        for( ; Edge != NULL; Edge = (EDGE_MODULE*) Edge->Pnext )
        {
            if( Edge->m_StructType != TYPEEDGEMODULE )
                continue;
            Edge->m_Layer = new_layer;
        }
    }
    else
        Edge->m_Layer = new_layer;

    GetScreen()->SetModify();
    Module->Set_Rectangle_Encadrement();
    Module->m_LastEdit_Time = time( NULL );
    DrawPanel->Refresh( TRUE );
}


/*************************************************************************/
void WinEDA_ModuleEditFrame::Enter_Edge_Width( EDGE_MODULE* Edge, wxDC* DC )
/*************************************************************************/

/*
 *  Entre la nouvelle valeur pour ModuleSegmentWidth.
 *  change la largeur du EDGE_MODULE Edge si Edge != NULL
 */
{
    wxString buffer;
    long     ll;

    buffer << ModuleSegmentWidth;
    if( Get_Message( _( "New Width (1/10000\"):" ), buffer, this ) )
        return;

    if( buffer.ToLong( &ll ) )
        ModuleSegmentWidth = ll;
    else
    {
        DisplayError( this, _( "Incorrect number, no change" ) );
        return;
    }
    if( Edge )
    {
        MODULE* Module = m_Pcb->m_Modules;
        Module->DrawEdgesOnly( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
        Edge->m_Width = ModuleSegmentWidth;
        Module->DrawEdgesOnly( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
        Module->Set_Rectangle_Encadrement();
        GetScreen()->SetModify();
    }
}


/****************************************************************************/
void WinEDA_ModuleEditFrame::Delete_Edge_Module( EDGE_MODULE* Edge, wxDC* DC )
/****************************************************************************/
{
    if( Edge == NULL )
        return;
    if( Edge->m_StructType != TYPEEDGEMODULE )
    {
        DisplayError( this, wxT( "StructType error: TYPEEDGEMODULE expected" ) );
        return;
    }

    MODULE* Module = (MODULE*) Edge->m_Parent;
    Edge->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );

    /* suppression d'un segment */
    DeleteStructure( Edge );
    Module->m_LastEdit_Time = time( NULL );
    Module->Set_Rectangle_Encadrement();
    GetScreen()->SetModify();
}


/******************************************************************/
static void Exit_EditEdge_Module( WinEDA_DrawPanel* Panel, wxDC* DC )
/******************************************************************/
{
    EDGE_MODULE* Edge = (EDGE_MODULE*) Panel->GetScreen()->GetCurItem();

    if( Edge && (Edge->m_StructType == TYPEEDGEMODULE) )    /* error si non */
    {
        if( Edge->m_Flags & IS_NEW )                        /* effacement du nouveau contour */
        {
            MODULE* Module = (MODULE*) Edge->m_Parent;
            Edge->Draw( Panel, DC, MoveVector, GR_XOR );
            DeleteStructure( Edge );
            Module->Set_Rectangle_Encadrement();
        }
        else
        {
            Edge->Draw( Panel, DC, MoveVector, GR_XOR );
            Edge->m_Flags = 0;
            Edge->Draw( Panel, DC, wxPoint( 0, 0 ), GR_OR );
        }
    }
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    Panel->GetScreen()->SetCurItem( NULL );
}


/*************************************************************************/
EDGE_MODULE* WinEDA_ModuleEditFrame::Begin_Edge_Module( EDGE_MODULE* Edge,
                                                        wxDC* DC, int type_edge )
/*************************************************************************/

/* Fonction de debut de trace d'un nouveau contour.
 *  - Cree la place en memoire du nouveau contour
 *  - Prepare les coord des differents points
 *  - Met a jour la variable type_edge (= S_SEGMENT,S_ARC ...)
 */
{
    MODULE* Module = m_Pcb->m_Modules;
    int     angle  = 0;

    if( Module == NULL )
        return NULL;

    if( Edge == NULL )       /* debut reel du trace */
    {
        SaveCopyInUndoList( Module );
        Edge = new EDGE_MODULE( Module );
        MoveVector.x = MoveVector.y = 0;

        /* Chainage du nouvel element, en tete de liste Drawings */
        Edge->Pback = Module;
        Edge->Pnext = Module->m_Drawings;
        if( Module->m_Drawings )
            Module->m_Drawings->Pback = Edge;
        Module->m_Drawings = Edge;

        /* Mise a jour des caracteristiques du segment ou de l'arc */
        Edge->m_Flags = IS_NEW;
        Edge->m_Angle = angle;
        Edge->m_Shape = type_edge;
        if( Edge->m_Shape == S_ARC )
            Edge->m_Angle = ArcValue;
        Edge->m_Width = ModuleSegmentWidth;
        Edge->m_Layer = Module->m_Layer;
        if( Module->m_Layer == CMP_N )
            Edge->m_Layer = SILKSCREEN_N_CMP;
        if( Module->m_Layer == CUIVRE_N )
            Edge->m_Layer = SILKSCREEN_N_CU;
        /* Mise a jour du point de depart du segment ou de l'arc */
        Edge->m_Start = GetScreen()->m_Curseur;
        /* Mise a jour de la fin du segment , rectangle ou de l'arc*/
        Edge->m_End = Edge->m_Start;

        /* Mise a jour des coord relatives */
        Edge->m_Start0.x = Edge->m_Start.x - Module->m_Pos.x;
        Edge->m_Start0.y = Edge->m_Start.y - Module->m_Pos.y;
        RotatePoint( (int*) &(Edge->m_Start0.x),
                    (int*) &(Edge->m_Start0.y), -Module->m_Orient );
        Edge->m_End0 = Edge->m_Start0;
        Module->Set_Rectangle_Encadrement();

        DrawPanel->ManageCurseur = ShowEdgeModule;
        DrawPanel->ForceCloseManageCurseur = Exit_EditEdge_Module;
    }
    else    /* trace en cours : les coord du point d'arrivee ont ete mises
             *  a jour par la routine Montre_Position_New_Edge_Module*/
    {
        if( type_edge == S_SEGMENT )
        {
            if( (Edge->m_Start0.x) != (Edge->m_End0.x)
               || (Edge->m_Start0.y) != (Edge->m_End0.y) )
            {
                Edge->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
                EDGE_MODULE* newedge = new EDGE_MODULE( Module );
                newedge->Copy( Edge );
                newedge->AddToChain( Edge );
                Edge->m_Flags = 0;
                Edge = newedge;

                Edge->m_Flags = IS_NEW;
                Edge->m_Width = ModuleSegmentWidth;
                Edge->m_Start = GetScreen()->m_Curseur;
                Edge->m_End   = Edge->m_Start;

                /* Mise a jour des coord relatives */
                Edge->m_Start0.x = Edge->m_Start.x - Module->m_Pos.x;
                Edge->m_Start0.y = Edge->m_Start.y - Module->m_Pos.y;
                RotatePoint( (int*) &(Edge->m_Start0.x),
                            (int*) &(Edge->m_Start0.y), -Module->m_Orient );
                Edge->m_End0 = Edge->m_Start0;

                Module->Set_Rectangle_Encadrement();
                Module->m_LastEdit_Time = time( NULL );
                GetScreen()->SetModify();
            }
        }
        else
            DisplayError( this, wxT( "Begin_Edge() error" ) );
    }
    return Edge;
}


/*************************************************************************/
void WinEDA_ModuleEditFrame::End_Edge_Module( EDGE_MODULE* Edge, wxDC* DC )
/*************************************************************************/
{
    MODULE* Module = m_Pcb->m_Modules;

    /* test du dernier segment: si null: suppression */
    if( Edge )
    {
        if( (Edge->m_Start.x == Edge->m_End.x)
           && (Edge->m_Start.y == Edge->m_End.y) )
        {
            DeleteStructure( Edge );
        }
    }
    Edge->m_Flags = 0;
    Module->Set_Rectangle_Encadrement();
    Module->m_LastEdit_Time = time( NULL );
    GetScreen()->SetModify();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
}

/******************************************************/
/*	 Edition of footprint edges */
/******************************************************/


/* fichier edgemod.cpp */

/* Functions to edit graphic items used to draw footprint edges.
 *  Function to Arcs are not made (TODO..) but menus are ready to use
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

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

/* Function to initialise the move function params of a graphic item type DRAWSEGMENT
 */
{
    if( Edge == NULL )
        return;
    Edge->Draw( DrawPanel, DC, GR_XOR );
    Edge->m_Flags |= IS_MOVED;
    MoveVector.x   = MoveVector.y = 0;
    CursorInitialPosition    = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = Move_Segment;
    DrawPanel->ForceCloseManageCurseur = Exit_EditEdge_Module;
    SetCurItem( Edge );
    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
}


/*********************************************************************/
void WinEDA_ModuleEditFrame::Place_EdgeMod( EDGE_MODULE* Edge, wxDC* DC )
/*********************************************************************/

/*
 * Function to place a graphic item type EDGE_MODULE currently moved
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

    Edge->Draw( DrawPanel, DC, GR_OR );
    Edge->m_Flags = 0;
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    SetCurItem( NULL );
    GetScreen()->SetModify();
    MODULE* Module = (MODULE*) Edge->GetParent();
    Module->Set_Rectangle_Encadrement();
}


/************************************************************************/
static void Move_Segment( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/************************************************************************/
/* Move and redraw the current edited graphic item when mouse is moving */
{
    BASE_SCREEN* screen = panel->GetScreen();
    EDGE_MODULE* Edge   = (EDGE_MODULE*) screen->GetCurItem();

    if( Edge == NULL )
        return;

    MODULE* Module = (MODULE*) Edge->GetParent();

    if( erase )
    {
        Edge->Draw( panel, DC, GR_XOR, MoveVector );
    }

    MoveVector.x = -(screen->m_Curseur.x - CursorInitialPosition.x);
    MoveVector.y = -(screen->m_Curseur.y - CursorInitialPosition.y);

    Edge->Draw( panel, DC, GR_XOR, MoveVector );

    Module->Set_Rectangle_Encadrement();
}


/************************************************************************/
static void ShowEdgeModule( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/************************************************************************/

/* Redraw the current edited (moved) graphic item
 */
{
    BASE_SCREEN* screen = panel->GetScreen();
    EDGE_MODULE* Edge   = (EDGE_MODULE*) screen->GetCurItem();

    if( Edge == NULL )
        return;

    MODULE* Module = (MODULE*) Edge->GetParent();

    //	if( erase )
    {
        Edge->Draw( panel, DC, GR_XOR );
    }

    Edge->m_End = screen->m_Curseur;

    /* Mise a jour des coord relatives */
    Edge->m_End0.x = Edge->m_End.x - Module->m_Pos.x;
    Edge->m_End0.y = Edge->m_End.y - Module->m_Pos.y;
    RotatePoint( (int*) &Edge->m_End0.x,
                (int*) &Edge->m_End0.y, -Module->m_Orient );

    Edge->Draw( panel, DC, GR_XOR );

    Module->Set_Rectangle_Encadrement();
}


/***************************************************************************/
void WinEDA_ModuleEditFrame::Edit_Edge_Width( EDGE_MODULE* Edge )
/***************************************************************************/

/* Change the EDGE_MODULE Edge width,
 * if Edge == NULL change the width of the entire footprint edges
 * @param ModuleSegmentWidth (global) = new width
 * @param Edge = edge to edit, or NULL
 * @param DC = current Device Context
*/
{
    MODULE* Module = GetBoard()->m_Modules;

    SaveCopyInUndoList( Module );

    if( Edge == NULL )
    {
        Edge = (EDGE_MODULE*)(BOARD_ITEM*) Module->m_Drawings;
        for( ; Edge != NULL; Edge = Edge->Next() )
        {
            if( Edge->Type() != TYPE_EDGE_MODULE )
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
void WinEDA_ModuleEditFrame::Edit_Edge_Layer( EDGE_MODULE* Edge )
/***************************************************************************/

/* Change the EDGE_MODULE Edge layer,  (The new layer will be asked)
 * if Edge == NULL change the layer of the entire footprint edges
 * @param Edge = edge to edit, or NULL
 * @param DC = current Device Context
*/
{
    MODULE* Module    = GetBoard()->m_Modules;
    int     new_layer = SILKSCREEN_N_CMP;
    if( Edge != NULL )
        new_layer = Edge->GetLayer();


    /* Ask for the new layer */
    new_layer = SelectLayer( new_layer, FIRST_COPPER_LAYER, LAST_NO_COPPER_LAYER );
    if( new_layer < 0 )
        return;

    if ( new_layer >= FIRST_COPPER_LAYER && new_layer <= LAST_COPPER_LAYER )
    /* an edge is put on a copper layer, and it is very dangerous. a confirmation is requested */
    {
        if ( ! IsOK(this, _("The graphic item will be on a copper layer.It is very dangerous. Are you sure") ) )
            return;
    }

    SaveCopyInUndoList( Module );

    if( Edge == NULL )
    {
        Edge = (EDGE_MODULE*)(BOARD_ITEM*) Module->m_Drawings;
        for( ; Edge != NULL; Edge = Edge->Next() )
        {
            if( Edge->Type() != TYPE_EDGE_MODULE )
                continue;
            Edge->SetLayer( new_layer );
        }
    }
    else
        Edge->SetLayer( new_layer );

    GetScreen()->SetModify();
    Module->Set_Rectangle_Encadrement();
    Module->m_LastEdit_Time = time( NULL );
    DrawPanel->Refresh( TRUE );
}


/*************************************************************************/
void WinEDA_ModuleEditFrame::Enter_Edge_Width( EDGE_MODULE* Edge, wxDC* DC )
/*************************************************************************/

/*	Edition of the edge items width
 *  Ask for a new width and init ModuleSegmentWidth.
 *  Change the width of EDGE_MODULE Edge if Edge != NULL
 * @param Edge = edge to edit, or NULL
 * @param DC = current Device Context
 * @output ModuleSegmentWidth (global) = new width
 */
{
    wxString buffer;
    long     ll;

    buffer << ModuleSegmentWidth;
    if( Get_Message( _( "New Width (1/10000\"):" ), _("Edge Width"), buffer, this ) )
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
        MODULE* Module = GetBoard()->m_Modules;
        Module->DrawEdgesOnly( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
        Edge->m_Width = ModuleSegmentWidth;
        Module->DrawEdgesOnly( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
        Module->Set_Rectangle_Encadrement();
        GetScreen()->SetModify();
    }
}


/****************************************************************************/
void WinEDA_ModuleEditFrame::Delete_Edge_Module( EDGE_MODULE* Edge )
/****************************************************************************/
/** Function Delete_Edge_Module
 *  delete EDGE_MODULE Edge
 * @param Edge = edge to delete
 */
{
    if( Edge == NULL )
        return;
    if( Edge->Type() != TYPE_EDGE_MODULE )
    {
        DisplayError( this, wxT( "StructType error: TYPE_EDGE_MODULE expected" ) );
        return;
    }

    MODULE* Module = (MODULE*) Edge->GetParent();

    /* suppression d'un segment */
    Edge ->DeleteStructure();
    Module->m_LastEdit_Time = time( NULL );
    Module->Set_Rectangle_Encadrement();
    GetScreen()->SetModify();
}


/******************************************************************/
static void Exit_EditEdge_Module( WinEDA_DrawPanel* Panel, wxDC* DC )
/******************************************************************/
/* abort function in moving edge.
*/
{
    EDGE_MODULE* Edge = (EDGE_MODULE*) Panel->GetScreen()->GetCurItem();

    if( Edge && (Edge->Type() == TYPE_EDGE_MODULE) )    /* error si non */
    {
        if( Edge->m_Flags & IS_NEW )                        /* effacement du nouveau contour */
        {
            MODULE* Module = (MODULE*) Edge->GetParent();
            Edge->Draw( Panel, DC, GR_XOR, MoveVector );
            Edge ->DeleteStructure();
            Module->Set_Rectangle_Encadrement();
        }
        else
        {
            Edge->Draw( Panel, DC, GR_XOR, MoveVector );
            Edge->m_Flags = 0;
            Edge->Draw( Panel, DC, GR_OR );
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

/* Create a new edge item (line, arc ..).
 * @param  Edge = if NULL: create new edge else terminate edge and create a new edge
 * @param  DC = current Device Context
 * @param type_edge = S_SEGMENT,S_ARC ..
 * @return the new created edge.
 */
{
    MODULE* module = GetBoard()->m_Modules;
    int     angle  = 0;

    if( module == NULL )
        return NULL;

    if( Edge == NULL )       /* Start a new edge item */
    {
        SaveCopyInUndoList( module );

        Edge = new EDGE_MODULE( module );
        MoveVector.x = MoveVector.y = 0;

        /* Add the new item to the Drawings list head*/
        module->m_Drawings.PushFront( Edge );

        /* Mise a jour des caracteristiques du segment ou de l'arc */
        Edge->m_Flags = IS_NEW;
        Edge->m_Angle = angle;
        Edge->m_Shape = type_edge;

        if( Edge->m_Shape == S_ARC )
            Edge->m_Angle = ArcValue;

        Edge->m_Width = ModuleSegmentWidth;
        Edge->SetLayer( module->GetLayer() );

        if( module->GetLayer() == CMP_N )
            Edge->SetLayer( SILKSCREEN_N_CMP );
        if( module->GetLayer() == COPPER_LAYER_N )
            Edge->SetLayer( SILKSCREEN_N_CU );

        /* Initialise the starting point of the new segment or arc */
        Edge->m_Start = GetScreen()->m_Curseur;

        /* Initialise the ending point of the new segment or arc */
        Edge->m_End = Edge->m_Start;

        /* Initialise the relative coordinates */
        Edge->m_Start0.x = Edge->m_Start.x - module->m_Pos.x;
        Edge->m_Start0.y = Edge->m_Start.y - module->m_Pos.y;

        RotatePoint( (int*) &Edge->m_Start0.x,
                    (int*) &Edge->m_Start0.y, -module->m_Orient );

        Edge->m_End0 = Edge->m_Start0;
        module->Set_Rectangle_Encadrement();

        DrawPanel->ManageCurseur = ShowEdgeModule;
        DrawPanel->ForceCloseManageCurseur = Exit_EditEdge_Module;
    }

    /* trace en cours : les coord du point d'arrivee ont ete mises
     * a jour par la routine Montre_Position_New_Edge_Module
     */
    else
    {
        if( type_edge == S_SEGMENT )
        {
            if( Edge->m_Start0 != Edge->m_End0 )
            {
                Edge->Draw( DrawPanel, DC, GR_OR );

                EDGE_MODULE* newedge = new EDGE_MODULE( module );
                newedge->Copy( Edge );

                // insert _after_ Edge, which is the same as inserting _before_ Edge->Next()
                module->m_Drawings.Insert( newedge, Edge->Next() );

                Edge->m_Flags = 0;
                Edge = newedge;

                Edge->m_Flags = IS_NEW;
                Edge->m_Width = ModuleSegmentWidth;
                Edge->m_Start = GetScreen()->m_Curseur;
                Edge->m_End   = Edge->m_Start;

                /* Mise a jour des coord relatives */
                Edge->m_Start0.x = Edge->m_Start.x - module->m_Pos.x;
                Edge->m_Start0.y = Edge->m_Start.y - module->m_Pos.y;

                RotatePoint( (int*) &Edge->m_Start0.x,
                            (int*) &Edge->m_Start0.y, -module->m_Orient );

                Edge->m_End0 = Edge->m_Start0;

                module->Set_Rectangle_Encadrement();
                module->m_LastEdit_Time = time( NULL );
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
/* Terminate a move or create edge function
*/
{
    MODULE* Module = GetBoard()->m_Modules;

    /* If last segment length is 0: deletion */
    if( Edge )
    {
        if( (Edge->m_Start.x == Edge->m_End.x)
           && (Edge->m_Start.y == Edge->m_End.y) )
        {
            Edge ->DeleteStructure();
        }
    }
    Edge->m_Flags = 0;
    Module->Set_Rectangle_Encadrement();
    Module->m_LastEdit_Time = time( NULL );
    GetScreen()->SetModify();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
}

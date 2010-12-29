/****************************/
/* Footprint edges editing. */
/****************************/

/**
 * Functions to edit graphic items used to draw footprint edges.
 *
 * @todo - Arc functions not compete but menus are ready to use.
 */

#include "fctsys.h"
#include "trigo.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"


static void ShowEdgeModule( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void Exit_EditEdge_Module( WinEDA_DrawPanel* Panel, wxDC* DC );
static void Move_Segment( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

int            ArcValue = 900;
static wxPoint MoveVector;              // Move vector for move edge
static wxPoint CursorInitialPosition;   // Mouse cursor inital position for
                                        // move command


/* Function to initialise the move function params of a graphic item type
 * DRAWSEGMENT
 */
void WinEDA_ModuleEditFrame::Start_Move_EdgeMod( EDGE_MODULE* Edge, wxDC* DC )
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


/*
 * Function to place a graphic item type EDGE_MODULE currently moved
 */
void WinEDA_ModuleEditFrame::Place_EdgeMod( EDGE_MODULE* Edge )
{
    if( Edge == NULL )
        return;
    Edge->m_Start -= MoveVector;
    Edge->m_End   -= MoveVector;

    Edge->m_Start0 -= MoveVector;
    Edge->m_End0   -= MoveVector;

    Edge->m_Flags = 0;
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    SetCurItem( NULL );
    OnModify();
    MODULE* Module = (MODULE*) Edge->GetParent();
    Module->Set_Rectangle_Encadrement();
    DrawPanel->Refresh( );
}


/* Move and redraw the current edited graphic item when mouse is moving */
static void Move_Segment( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
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

    MoveVector = -(screen->m_Curseur - CursorInitialPosition);

    Edge->Draw( panel, DC, GR_XOR, MoveVector );

    Module->Set_Rectangle_Encadrement();
}


/* Redraw the current edited (moved) graphic item
 */
static void ShowEdgeModule( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
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

    /* Update relative coordinate. */
    Edge->m_End0 = Edge->m_End - Module->m_Pos;
    RotatePoint( &Edge->m_End0, -Module->m_Orient );

    Edge->Draw( panel, DC, GR_XOR );

    Module->Set_Rectangle_Encadrement();
}


/**
 * Function Edit_Edge_Width
 * changes the width of module perimeter lines, EDGE_MODULEs.
 * param ModuleSegmentWidth (global) = new width
 * @param aEdge = edge to edit, or NULL.  If aEdge == NULL change
 *               the width of all footprint's edges
 */
void WinEDA_ModuleEditFrame::Edit_Edge_Width( EDGE_MODULE* aEdge )
{
    MODULE* Module = GetBoard()->m_Modules;

    SaveCopyInUndoList( Module, UR_MODEDIT );

    if( aEdge == NULL )
    {
        aEdge = (EDGE_MODULE*) (BOARD_ITEM*) Module->m_Drawings;
        for( ; aEdge != NULL; aEdge = aEdge->Next() )
        {
            if( aEdge->Type() != TYPE_EDGE_MODULE )
                continue;
            aEdge->m_Width = g_ModuleSegmentWidth;
        }
    }
    else
        aEdge->m_Width = g_ModuleSegmentWidth;

    OnModify();
    Module->Set_Rectangle_Encadrement();
    Module->m_LastEdit_Time = time( NULL );
}


/* Change the EDGE_MODULE Edge layer,  (The new layer will be asked)
 * if Edge == NULL change the layer of the entire footprint edges
 * @param Edge = edge to edit, or NULL
 * @param DC = current Device Context
 */
void WinEDA_ModuleEditFrame::Edit_Edge_Layer( EDGE_MODULE* Edge )
{
    MODULE* Module    = GetBoard()->m_Modules;
    int     new_layer = SILKSCREEN_N_FRONT;

    if( Edge != NULL )
        new_layer = Edge->GetLayer();


    /* Ask for the new layer */
    new_layer = SelectLayer( new_layer,
                             FIRST_COPPER_LAYER,
                             LAST_NO_COPPER_LAYER );
    if( new_layer < 0 )
        return;

    if( IsValidCopperLayerIndex( new_layer ) )
    {
        /* an edge is put on a copper layer, and it is very dangerous. a
         *confirmation is requested */
        if( !IsOK( this,
                  _(
                      "The graphic item will be on a copper layer.  It is very dangerous. Are you sure?" ) ) )
            return;
    }

    SaveCopyInUndoList( Module, UR_MODEDIT );

    if( Edge == NULL )
    {
        Edge = (EDGE_MODULE*) (BOARD_ITEM*) Module->m_Drawings;
        for( ; Edge != NULL; Edge = Edge->Next() )
        {
            if( Edge->Type() != TYPE_EDGE_MODULE )
                continue;
            Edge->SetLayer( new_layer );
        }
    }
    else
        Edge->SetLayer( new_layer );

    OnModify();
    Module->Set_Rectangle_Encadrement();
    Module->m_LastEdit_Time = time( NULL );
}


/**
 * Function Enter_Edge_Width
 * Edition of width of module outlines
 * Ask for a new width.
 * Change the width of EDGE_MODULE aEdge if aEdge != NULL
 * @param aEdge = edge to edit, or NULL
 * changes g_ModuleSegmentWidth (global) = new width
 */
void WinEDA_ModuleEditFrame::Enter_Edge_Width( EDGE_MODULE* aEdge )
{
    wxString buffer;

    buffer = ReturnStringFromValue( g_UserUnit, g_ModuleSegmentWidth, GetScreen()->GetInternalUnits() );
    wxTextEntryDialog dlg( this, _( "New Width:" ), _( "Edge Width" ), buffer );
    if( dlg.ShowModal() != wxID_OK )
        return; // cancelled by user

    buffer = dlg.GetValue( );
    g_ModuleSegmentWidth = ReturnValueFromString( g_UserUnit, buffer, GetScreen()->GetInternalUnits() );

    if( aEdge )
    {
        MODULE* Module = GetBoard()->m_Modules;
        aEdge->m_Width = g_ModuleSegmentWidth;
        Module->Set_Rectangle_Encadrement();
        OnModify();
    }
}


/**
 * Function Delete_Edge_Module
 *  delete EDGE_MODULE Edge
 * @param Edge = edge to delete
 */
void WinEDA_ModuleEditFrame::Delete_Edge_Module( EDGE_MODULE* Edge )
{
    if( Edge == NULL )
        return;
    if( Edge->Type() != TYPE_EDGE_MODULE )
    {
        DisplayError( this,
                     wxT( "StructType error: TYPE_EDGE_MODULE expected" ) );
        return;
    }

    MODULE* Module = (MODULE*) Edge->GetParent();

    /* Delete segment. */
    Edge->DeleteStructure();
    Module->m_LastEdit_Time = time( NULL );
    Module->Set_Rectangle_Encadrement();
    OnModify();
}


/* abort function in moving edge.
 */
static void Exit_EditEdge_Module( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    EDGE_MODULE* Edge = (EDGE_MODULE*) Panel->GetScreen()->GetCurItem();

    if( Edge && ( Edge->Type() == TYPE_EDGE_MODULE ) )
    {
        if( Edge->m_Flags & IS_NEW )   /* Delete if new module. */
        {
            MODULE* Module = (MODULE*) Edge->GetParent();
            Edge->Draw( Panel, DC, GR_XOR, MoveVector );
            Edge->DeleteStructure();
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


/* Create a new edge item (line, arc ..).
 * @param  Edge = if NULL: create new edge else terminate edge and create a
 *                new edge
 * @param  DC = current Device Context
 * @param type_edge = S_SEGMENT,S_ARC ..
 * @return the new created edge.
 */
EDGE_MODULE* WinEDA_ModuleEditFrame::Begin_Edge_Module( EDGE_MODULE* Edge,
                                                        wxDC*        DC,
                                                        int          type_edge )
{
    MODULE* module = GetBoard()->m_Modules;
    int     angle  = 0;

    if( module == NULL )
        return NULL;

    if( Edge == NULL )       /* Start a new edge item */
    {
        SaveCopyInUndoList( module, UR_MODEDIT );

        Edge = new EDGE_MODULE( module );
        MoveVector.x = MoveVector.y = 0;

        /* Add the new item to the Drawings list head*/
        module->m_Drawings.PushFront( Edge );

        /* Update characteristics of the segment or arc. */
        Edge->m_Flags = IS_NEW;
        Edge->m_Angle = angle;
        Edge->m_Shape = type_edge;

        if( Edge->m_Shape == S_ARC )
            Edge->m_Angle = ArcValue;

        Edge->m_Width = g_ModuleSegmentWidth;
        Edge->SetLayer( module->GetLayer() );

        if( module->GetLayer() == LAYER_N_FRONT )
            Edge->SetLayer( SILKSCREEN_N_FRONT );
        if( module->GetLayer() == LAYER_N_BACK )
            Edge->SetLayer( SILKSCREEN_N_BACK );

        /* Initialise the starting point of the new segment or arc */
        Edge->m_Start = GetScreen()->m_Curseur;

        /* Initialise the ending point of the new segment or arc */
        Edge->m_End = Edge->m_Start;

        /* Initialise the relative coordinates */
        Edge->m_Start0 = Edge->m_Start - module->m_Pos;

        RotatePoint( &Edge->m_Start0, -module->m_Orient );

        Edge->m_End0 = Edge->m_Start0;
        module->Set_Rectangle_Encadrement();

        DrawPanel->ManageCurseur = ShowEdgeModule;
        DrawPanel->ForceCloseManageCurseur = Exit_EditEdge_Module;
    }
    /* Segment creation in progress.
     * The ending coordinate are updated by the function
     * Montre_Position_New_Edge_Module() called on move mouse event
     * during the segment craetion
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

                // insert _after_ Edge, which is the same as inserting _before_
                // Edge->Next()
                module->m_Drawings.Insert( newedge, Edge->Next() );
                Edge->m_Flags = 0;

                Edge = newedge;     // point now new item

                Edge->m_Flags = IS_NEW;
                Edge->m_Width = g_ModuleSegmentWidth;
                Edge->m_Start = GetScreen()->m_Curseur;
                Edge->m_End   = Edge->m_Start;

                /* Update relative coordinate. */
                Edge->m_Start0 = Edge->m_Start - module->m_Pos;

                RotatePoint( &Edge->m_Start0, -module->m_Orient );

                Edge->m_End0 = Edge->m_Start0;

                module->Set_Rectangle_Encadrement();
                module->m_LastEdit_Time = time( NULL );
                OnModify();
            }
        }
        else
            wxMessageBox( wxT( "Begin_Edge() error" ) );
    }
    return Edge;
}


/* Terminate a move or create edge function
 */
void WinEDA_ModuleEditFrame::End_Edge_Module( EDGE_MODULE* Edge )
{
    MODULE* Module = GetBoard()->m_Modules;

    if( Edge )
    {
        Edge->m_Flags = 0;
        /* If last segment length is 0: remove it */
        if( Edge->m_Start == Edge->m_End )
            Edge->DeleteStructure();
    }
    Module->Set_Rectangle_Encadrement();
    Module->m_LastEdit_Time = time( NULL );
    OnModify();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
}

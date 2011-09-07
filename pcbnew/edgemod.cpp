/*******************************/
/* Footprint outlines editing. */
/*******************************/

/**
 * Functions to edit graphic items used to draw footprint edges.
 *
 * @todo - Arc functions not compete but menus are ready to use.
 */

#include "fctsys.h"
#include "trigo.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"


static void ShowNewEdgeModule( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                               bool erase );
static void Abort_Move_ModuleOutline( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void ShowCurrentOutlineWhileMoving( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                           const wxPoint& aPosition, bool aErase );

int            ArcValue = 900;
static wxPoint MoveVector;              // Move vector for move edge
static wxPoint CursorInitialPosition;   // Mouse cursor initial position for move command


void FOOTPRINT_EDIT_FRAME::Start_Move_EdgeMod( EDGE_MODULE* Edge, wxDC* DC )
{
    if( Edge == NULL )
        return;

    Edge->Draw( DrawPanel, DC, GR_XOR );
    Edge->m_Flags |= IS_MOVED;
    MoveVector.x   = MoveVector.y = 0;
    CursorInitialPosition    = GetScreen()->GetCrossHairPosition();
    DrawPanel->SetMouseCapture( ShowCurrentOutlineWhileMoving, Abort_Move_ModuleOutline );
    SetCurItem( Edge );
    DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );
}


void FOOTPRINT_EDIT_FRAME::Place_EdgeMod( EDGE_MODULE* Edge )
{
    if( Edge == NULL )
        return;

    Edge->m_Start -= MoveVector;
    Edge->m_End   -= MoveVector;

    Edge->m_Start0 -= MoveVector;
    Edge->m_End0   -= MoveVector;

    Edge->m_Flags = 0;
    DrawPanel->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );
    OnModify();
    MODULE* Module = (MODULE*) Edge->GetParent();
    Module->CalculateBoundingBox();
    DrawPanel->Refresh( );
}


/* Redraw the current moved graphic item when mouse is moving
 * Use this function to show an existing outline, in move command
*/
static void ShowCurrentOutlineWhileMoving( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                           const wxPoint& aPosition, bool aErase )
{
    BASE_SCREEN* screen = aPanel->GetScreen();
    EDGE_MODULE* Edge   = (EDGE_MODULE*) screen->GetCurItem();

    if( Edge == NULL )
        return;

    MODULE* Module = (MODULE*) Edge->GetParent();

    if( aErase )
    {
        Edge->Draw( aPanel, aDC, GR_XOR, MoveVector );
    }

    MoveVector = -(screen->GetCrossHairPosition() - CursorInitialPosition);

    Edge->Draw( aPanel, aDC, GR_XOR, MoveVector );

    Module->CalculateBoundingBox();
}


/* Redraw the current graphic item during its creation
 * Use this function to show a new outline, in begin command
 */
static void ShowNewEdgeModule( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                               bool aErase )
{
    BASE_SCREEN* screen = aPanel->GetScreen();
    EDGE_MODULE* Edge   = (EDGE_MODULE*) screen->GetCurItem();

    if( Edge == NULL )
        return;

    MODULE* Module = (MODULE*) Edge->GetParent();

    //  if( erase )
    {
        Edge->Draw( aPanel, aDC, GR_XOR );
    }

    Edge->m_End = screen->GetCrossHairPosition();

    /* Update relative coordinate. */
    Edge->m_End0 = Edge->m_End - Module->m_Pos;
    RotatePoint( &Edge->m_End0, -Module->m_Orient );

    Edge->Draw( aPanel, aDC, GR_XOR );

    Module->CalculateBoundingBox();
}


void FOOTPRINT_EDIT_FRAME::Edit_Edge_Width( EDGE_MODULE* aEdge )
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
    {
        aEdge->m_Width = g_ModuleSegmentWidth;
    }

    OnModify();
    Module->CalculateBoundingBox();
    Module->m_LastEdit_Time = time( NULL );
}


void FOOTPRINT_EDIT_FRAME::Edit_Edge_Layer( EDGE_MODULE* Edge )
{
    MODULE* Module    = GetBoard()->m_Modules;
    int     new_layer = SILKSCREEN_N_FRONT;

    if( Edge != NULL )
        new_layer = Edge->GetLayer();

    /* Ask for the new layer */
    new_layer = SelectLayer( new_layer, FIRST_COPPER_LAYER, LAST_NO_COPPER_LAYER );

    if( new_layer < 0 )
        return;

    if( IsValidCopperLayerIndex( new_layer ) )
    {
        /* an edge is put on a copper layer, and it is very dangerous. a
         *confirmation is requested */
        if( !IsOK( this,
                   _( "The graphic item will be on a copper layer.  It is very dangerous. Are you sure?" ) ) )
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
    {
        Edge->SetLayer( new_layer );
    }

    OnModify();
    Module->CalculateBoundingBox();
    Module->m_LastEdit_Time = time( NULL );
}


void FOOTPRINT_EDIT_FRAME::Enter_Edge_Width( EDGE_MODULE* aEdge )
{
    wxString buffer;

    buffer = ReturnStringFromValue( g_UserUnit, g_ModuleSegmentWidth,
                                    GetScreen()->GetInternalUnits() );
    wxTextEntryDialog dlg( this, _( "New Width:" ), _( "Edge Width" ), buffer );

    if( dlg.ShowModal() != wxID_OK )
        return; // canceled by user

    buffer = dlg.GetValue( );
    g_ModuleSegmentWidth = ReturnValueFromString( g_UserUnit, buffer,
                                                  GetScreen()->GetInternalUnits() );

    if( aEdge )
    {
        MODULE* Module = GetBoard()->m_Modules;
        aEdge->m_Width = g_ModuleSegmentWidth;
        Module->CalculateBoundingBox();
        OnModify();
    }
}


void FOOTPRINT_EDIT_FRAME::Delete_Edge_Module( EDGE_MODULE* Edge )
{
    if( Edge == NULL )
        return;

    if( Edge->Type() != TYPE_EDGE_MODULE )
    {
        DisplayError( this, wxT( "StructType error: TYPE_EDGE_MODULE expected" ) );
        return;
    }

    MODULE* Module = (MODULE*) Edge->GetParent();

    /* Delete segment. */
    Edge->DeleteStructure();
    Module->m_LastEdit_Time = time( NULL );
    Module->CalculateBoundingBox();
    OnModify();
}


/* abort function in moving outline.
 */
static void Abort_Move_ModuleOutline( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    EDGE_MODULE* Edge = (EDGE_MODULE*) Panel->GetScreen()->GetCurItem();

    Panel->SetMouseCapture( NULL, NULL );

    if( Edge && ( Edge->Type() == TYPE_EDGE_MODULE ) )
    {
        if( Edge->IsNew() )   // On aborting, delete new outline.
        {
            MODULE* Module = (MODULE*) Edge->GetParent();
            Edge->Draw( Panel, DC, GR_XOR, MoveVector );
            Edge->DeleteStructure();
            Module->CalculateBoundingBox();
        }
        else   // On aborting, move existing outline to its initial position.
        {
            Edge->Draw( Panel, DC, GR_XOR, MoveVector );
            Edge->m_Flags = 0;
            Edge->Draw( Panel, DC, GR_OR );
        }
    }

    Panel->GetScreen()->SetCurItem( NULL );
}


EDGE_MODULE* FOOTPRINT_EDIT_FRAME::Begin_Edge_Module( EDGE_MODULE* Edge,
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

        /* Initialize the starting point of the new segment or arc */
        Edge->m_Start = GetScreen()->GetCrossHairPosition();

        /* Initialize the ending point of the new segment or arc */
        Edge->m_End = Edge->m_Start;

        /* Initialize the relative coordinates */
        Edge->m_Start0 = Edge->m_Start - module->m_Pos;

        RotatePoint( &Edge->m_Start0, -module->m_Orient );

        Edge->m_End0 = Edge->m_Start0;
        module->CalculateBoundingBox();
        DrawPanel->SetMouseCapture( ShowNewEdgeModule, Abort_Move_ModuleOutline );
    }
    /* Segment creation in progress.
     * The ending coordinate is updated by the function
     * ShowNewEdgeModule() called on move mouse event
     * during the segment creation
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

                // insert _after_ Edge, which is the same as inserting before Edge->Next()
                module->m_Drawings.Insert( newedge, Edge->Next() );
                Edge->m_Flags = 0;

                Edge = newedge;     // point now new item

                Edge->m_Flags = IS_NEW;
                Edge->m_Width = g_ModuleSegmentWidth;
                Edge->m_Start = GetScreen()->GetCrossHairPosition();
                Edge->m_End   = Edge->m_Start;

                /* Update relative coordinate. */
                Edge->m_Start0 = Edge->m_Start - module->m_Pos;

                RotatePoint( &Edge->m_Start0, -module->m_Orient );

                Edge->m_End0 = Edge->m_Start0;

                module->CalculateBoundingBox();
                module->m_LastEdit_Time = time( NULL );
                OnModify();
            }
        }
        else
        {
            wxMessageBox( wxT( "Begin_Edge() error" ) );
        }
    }

    return Edge;
}


void FOOTPRINT_EDIT_FRAME::End_Edge_Module( EDGE_MODULE* Edge )
{
    MODULE* Module = GetBoard()->m_Modules;

    if( Edge )
    {
        Edge->m_Flags = 0;

        /* If last segment length is 0: remove it */
        if( Edge->m_Start == Edge->m_End )
            Edge->DeleteStructure();
    }

    Module->CalculateBoundingBox();
    Module->m_LastEdit_Time = time( NULL );
    OnModify();
    DrawPanel->SetMouseCapture( NULL, NULL );
}

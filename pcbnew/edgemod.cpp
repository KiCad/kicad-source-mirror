/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file edgemod.cpp:
 * @brief Functions to edit graphic items used to draw footprint edges.
 *
 * @todo - Arc functions not compete but menus are ready to use.
 */

#include <fctsys.h>
#include <trigo.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>

#include <module_editor_frame.h>
#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>

#include <pcbnew.h>


static void ShowNewEdgeModule( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                               bool erase );
static void Abort_Move_ModuleOutline( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void ShowCurrentOutlineWhileMoving( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                           const wxPoint& aPosition, bool aErase );

static double  ArcValue = 900;
static wxPoint MoveVector;              // Move vector for move edge
static wxPoint CursorInitialPosition;   // Mouse cursor initial position for move command


void FOOTPRINT_EDIT_FRAME::Start_Move_EdgeMod( EDGE_MODULE* Edge, wxDC* DC )
{
    if( Edge == NULL )
        return;

    Edge->Draw( m_canvas, DC, GR_XOR );
    Edge->SetFlags( IS_MOVED );
    MoveVector.x   = MoveVector.y = 0;
    CursorInitialPosition    = GetScreen()->GetCrossHairPosition();
    m_canvas->SetMouseCapture( ShowCurrentOutlineWhileMoving, Abort_Move_ModuleOutline );
    SetCurItem( Edge );
    m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
}


void FOOTPRINT_EDIT_FRAME::Place_EdgeMod( EDGE_MODULE* aEdge )
{
    if( aEdge == NULL )
        return;

    aEdge->SetStart( aEdge->GetStart() - MoveVector );
    aEdge->SetEnd(   aEdge->GetEnd()   - MoveVector );

    aEdge->SetStart0( aEdge->GetStart0() - MoveVector );
    aEdge->SetEnd0(   aEdge->GetEnd0()   - MoveVector );

    aEdge->ClearFlags();
    m_canvas->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );
    OnModify();

    MODULE* module = (MODULE*) aEdge->GetParent();
    module->CalculateBoundingBox();

    m_canvas->Refresh( );
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

    MODULE* module = (MODULE*) Edge->GetParent();

    //  if( erase )
    {
        Edge->Draw( aPanel, aDC, GR_XOR );
    }

    Edge->SetEnd( screen->GetCrossHairPosition() );

    // Update relative coordinate.
    Edge->SetEnd0( Edge->GetEnd() - module->GetPosition() );

    wxPoint pt( Edge->GetEnd0() );

    RotatePoint( &pt, -module->GetOrientation() );

    Edge->SetEnd0( pt );

    Edge->Draw( aPanel, aDC, GR_XOR );

    module->CalculateBoundingBox();
}


void FOOTPRINT_EDIT_FRAME::Edit_Edge_Width( EDGE_MODULE* aEdge )
{
    MODULE* module = GetBoard()->m_Modules;

    SaveCopyInUndoList( module, UR_MODEDIT );

    if( aEdge == NULL )
    {
        aEdge = (EDGE_MODULE*) (BOARD_ITEM*) module->m_Drawings;

        for( ; aEdge != NULL; aEdge = aEdge->Next() )
        {
            if( aEdge->Type() != PCB_MODULE_EDGE_T )
                continue;

            aEdge->SetWidth( GetBoard()->GetDesignSettings().m_ModuleSegmentWidth );
        }
    }
    else
    {
        aEdge->SetWidth( GetBoard()->GetDesignSettings().m_ModuleSegmentWidth );
    }

    OnModify();
    module->CalculateBoundingBox();
    module->m_LastEdit_Time = time( NULL );
}


void FOOTPRINT_EDIT_FRAME::Edit_Edge_Layer( EDGE_MODULE* aEdge )
{
    MODULE* module    = GetBoard()->m_Modules;
    int     new_layer = SILKSCREEN_N_FRONT;

    if( aEdge )
        new_layer = aEdge->GetLayer();

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

    SaveCopyInUndoList( module, UR_MODEDIT );

    if( aEdge == NULL )
    {
        aEdge = (EDGE_MODULE*) (BOARD_ITEM*) module->m_Drawings;

        for( ; aEdge != NULL; aEdge = aEdge->Next() )
        {
            if( aEdge->Type() != PCB_MODULE_EDGE_T )
                continue;

            aEdge->SetLayer( new_layer );
        }
    }
    else
    {
        aEdge->SetLayer( new_layer );
    }

    OnModify();
    module->CalculateBoundingBox();
    module->m_LastEdit_Time = time( NULL );
}


void FOOTPRINT_EDIT_FRAME::Enter_Edge_Width( EDGE_MODULE* aEdge )
{
    wxString buffer;

    buffer = ReturnStringFromValue( g_UserUnit, GetBoard()->GetDesignSettings().m_ModuleSegmentWidth,
                                    GetScreen()->GetInternalUnits() );
    wxTextEntryDialog dlg( this, _( "New Width:" ), _( "Edge Width" ), buffer );

    if( dlg.ShowModal() != wxID_OK )
        return; // canceled by user

    buffer = dlg.GetValue( );
    GetBoard()->GetDesignSettings().m_ModuleSegmentWidth =
            ReturnValueFromString( g_UserUnit, buffer, GetScreen()->GetInternalUnits() );

    if( aEdge )
    {
        MODULE* module = GetBoard()->m_Modules;
        aEdge->SetWidth( GetBoard()->GetDesignSettings().m_ModuleSegmentWidth );
        module->CalculateBoundingBox();
        OnModify();
    }
}


void FOOTPRINT_EDIT_FRAME::Delete_Edge_Module( EDGE_MODULE* Edge )
{
    if( Edge == NULL )
        return;

    if( Edge->Type() != PCB_MODULE_EDGE_T )
    {
        DisplayError( this, wxT( "StructType error: PCB_MODULE_EDGE_T expected" ) );
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

    if( Edge && ( Edge->Type() == PCB_MODULE_EDGE_T ) )
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
            Edge->ClearFlags();
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

        // Add the new item to the Drawings list head
        module->m_Drawings.PushFront( Edge );

        // Update characteristics of the segment or arc.
        Edge->SetFlags( IS_NEW );
        Edge->SetAngle( angle );
        Edge->SetShape( type_edge );

        if( Edge->GetShape() == S_ARC )
            Edge->SetAngle( ArcValue );

        Edge->SetWidth( GetBoard()->GetDesignSettings().m_ModuleSegmentWidth );
        Edge->SetLayer( module->GetLayer() );

        if( module->GetLayer() == LAYER_N_FRONT )
            Edge->SetLayer( SILKSCREEN_N_FRONT );

        if( module->GetLayer() == LAYER_N_BACK )
            Edge->SetLayer( SILKSCREEN_N_BACK );

        // Initialize the starting point of the new segment or arc
        Edge->SetStart( GetScreen()->GetCrossHairPosition() );

        // Initialize the ending point of the new segment or arc
        Edge->SetEnd( Edge->GetStart() );

        // Initialize the relative coordinates
        Edge->SetStart0( Edge->GetStart() - module->GetPosition() );

        RotatePoint( &Edge->m_Start0, -module->m_Orient );

        Edge->m_End0 = Edge->m_Start0;
        module->CalculateBoundingBox();
        m_canvas->SetMouseCapture( ShowNewEdgeModule, Abort_Move_ModuleOutline );
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
                Edge->Draw( m_canvas, DC, GR_OR );

                EDGE_MODULE* newedge = new EDGE_MODULE( *Edge );

                // insert _after_ Edge, which is the same as inserting before Edge->Next()
                module->m_Drawings.Insert( newedge, Edge->Next() );
                Edge->ClearFlags();

                Edge = newedge;     // point now new item

                Edge->SetFlags( IS_NEW );
                Edge->SetWidth( GetBoard()->GetDesignSettings().m_ModuleSegmentWidth );
                Edge->SetStart( GetScreen()->GetCrossHairPosition() );
                Edge->SetEnd( Edge->GetStart() );

                // Update relative coordinate.
                Edge->SetStart0( Edge->GetStart() - module->GetPosition() );

                wxPoint pt( Edge->GetStart0() );

                RotatePoint( &pt, -module->GetOrientation() );

                Edge->SetStart0( pt );

                Edge->SetEnd0( Edge->GetStart0() );

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
        Edge->ClearFlags();

        /* If last segment length is 0: remove it */
        if( Edge->GetStart() == Edge->GetEnd() )
            Edge->DeleteStructure();
    }

    Module->CalculateBoundingBox();
    Module->m_LastEdit_Time = time( NULL );
    OnModify();
    m_canvas->SetMouseCapture( NULL, NULL );
}

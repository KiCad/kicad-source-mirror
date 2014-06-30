/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <base_units.h>

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


void FOOTPRINT_EDIT_FRAME::Start_Move_EdgeMod( EDGE_MODULE* aEdge, wxDC* DC )
{
    if( aEdge == NULL )
        return;

    aEdge->Draw( m_canvas, DC, GR_XOR );
    aEdge->SetFlags( IS_MOVED );
    MoveVector.x   = MoveVector.y = 0;
    CursorInitialPosition    = GetCrossHairPosition();
    m_canvas->SetMouseCapture( ShowCurrentOutlineWhileMoving, Abort_Move_ModuleOutline );
    SetCurItem( aEdge );
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
    EDGE_MODULE* edge   = (EDGE_MODULE*) screen->GetCurItem();

    if( edge == NULL )
        return;

    MODULE* module = (MODULE*) edge->GetParent();

    if( aErase )
    {
        edge->Draw( aPanel, aDC, GR_XOR, MoveVector );
    }

    MoveVector = -(aPanel->GetParent()->GetCrossHairPosition() - CursorInitialPosition);

    edge->Draw( aPanel, aDC, GR_XOR, MoveVector );

    module->CalculateBoundingBox();
}


/* Redraw the current graphic item during its creation
 * Use this function to show a new outline, in begin command
 */
static void ShowNewEdgeModule( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                               bool aErase )
{
    BASE_SCREEN* screen = aPanel->GetScreen();
    EDGE_MODULE* edge   = (EDGE_MODULE*) screen->GetCurItem();

    if( edge == NULL )
        return;

    MODULE* module = (MODULE*) edge->GetParent();

    //  if( erase )
    {
        edge->Draw( aPanel, aDC, GR_XOR );
    }

    edge->SetEnd( aPanel->GetParent()->GetCrossHairPosition() );

    // Update relative coordinate.
    edge->SetEnd0( edge->GetEnd() - module->GetPosition() );

    wxPoint pt( edge->GetEnd0() );

    RotatePoint( &pt, -module->GetOrientation() );

    edge->SetEnd0( pt );

    edge->Draw( aPanel, aDC, GR_XOR );

    module->CalculateBoundingBox();
}


void FOOTPRINT_EDIT_FRAME::Edit_Edge_Width( EDGE_MODULE* aEdge )
{
    MODULE* module = GetBoard()->m_Modules;

    SaveCopyInUndoList( module, UR_MODEDIT );

    if( aEdge == NULL )
    {
        aEdge = (EDGE_MODULE*) (BOARD_ITEM*) module->GraphicalItems();

        for( BOARD_ITEM *item = module->GraphicalItems(); item; item = item->Next() )
        {
            aEdge = dyn_cast<EDGE_MODULE*>( item );

            if( aEdge )
                aEdge->SetWidth( GetDesignSettings().m_ModuleSegmentWidth );
        }
    }
    else
    {
        aEdge->SetWidth( GetDesignSettings().m_ModuleSegmentWidth );
    }

    OnModify();
    module->CalculateBoundingBox();
    module->SetLastEditTime();
}


void FOOTPRINT_EDIT_FRAME::Edit_Edge_Layer( EDGE_MODULE* aEdge )
{
    // note: if aEdge == NULL, all outline segments will be modified

    MODULE*     module = GetBoard()->m_Modules;
    LAYER_ID    layer = F_SilkS;
    bool        modified = false;

    if( aEdge )
        layer = aEdge->GetLayer();

    // Ask for the new layer
    LAYER_ID new_layer = SelectLayer( layer, Edge_Cuts );

    if( layer < 0 )
        return;

    if( IsCopperLayer( new_layer ) )
    {
        /* an edge is put on a copper layer, and it is very dangerous. a
         *confirmation is requested */
        if( !IsOK( this,
                   _( "The graphic item will be on a copper layer. This is very dangerous. Are you sure?" ) ) )
            return;
    }

    if( !aEdge )
    {
        for( BOARD_ITEM *item = module->GraphicalItems() ; item != NULL;
                item = item->Next() )
        {
            aEdge = dyn_cast<EDGE_MODULE*>( item );

            if( aEdge && (aEdge->GetLayer() != new_layer) )
            {
                if( ! modified )    // save only once
                    SaveCopyInUndoList( module, UR_MODEDIT );
                aEdge->SetLayer( new_layer );
                modified = true;
            }
        }
    }
    else if( aEdge->GetLayer() != new_layer )
    {
        SaveCopyInUndoList( module, UR_MODEDIT );
        aEdge->SetLayer( new_layer );
        modified = true;
    }

    if( modified )
    {
        module->CalculateBoundingBox();
        module->SetLastEditTime();
    }
}


void FOOTPRINT_EDIT_FRAME::Enter_Edge_Width( EDGE_MODULE* aEdge )
{
    wxString buffer;

    buffer = StringFromValue( g_UserUnit, GetDesignSettings().m_ModuleSegmentWidth );
    wxTextEntryDialog dlg( this, _( "New Width:" ), _( "Edge Width" ), buffer );

    if( dlg.ShowModal() != wxID_OK )
        return; // canceled by user

    buffer = dlg.GetValue( );
    GetDesignSettings().m_ModuleSegmentWidth = ValueFromString( g_UserUnit, buffer );

    if( aEdge )
    {
        MODULE* module = GetBoard()->m_Modules;
        aEdge->SetWidth( GetDesignSettings().m_ModuleSegmentWidth );
        module->CalculateBoundingBox();
        OnModify();
    }
}


void FOOTPRINT_EDIT_FRAME::Delete_Edge_Module( EDGE_MODULE* aEdge )
{
    if( aEdge == NULL )
        return;

    if( aEdge->Type() != PCB_MODULE_EDGE_T )
    {
        DisplayError( this, wxT( "StructType error: PCB_MODULE_EDGE_T expected" ) );
        return;
    }

    MODULE* module = (MODULE*) aEdge->GetParent();

    // Delete segment.
    aEdge->DeleteStructure();
    module->SetLastEditTime();
    module->CalculateBoundingBox();
    OnModify();
}


/* abort function in moving outline.
 */
static void Abort_Move_ModuleOutline( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    EDGE_MODULE* edge = (EDGE_MODULE*) Panel->GetScreen()->GetCurItem();

    Panel->SetMouseCapture( NULL, NULL );

    if( edge && ( edge->Type() == PCB_MODULE_EDGE_T ) )
    {
        if( edge->IsNew() )   // On aborting, delete new outline.
        {
            MODULE* module = (MODULE*) edge->GetParent();
            edge->Draw( Panel, DC, GR_XOR, MoveVector );
            edge->DeleteStructure();
            module->CalculateBoundingBox();
        }
        else   // On aborting, move existing outline to its initial position.
        {
            edge->Draw( Panel, DC, GR_XOR, MoveVector );
            edge->ClearFlags();
            edge->Draw( Panel, DC, GR_OR );
        }
    }

    Panel->GetScreen()->SetCurItem( NULL );
}


EDGE_MODULE* FOOTPRINT_EDIT_FRAME::Begin_Edge_Module( EDGE_MODULE* aEdge,
                                                      wxDC*        DC,
                                                      STROKE_T     type_edge )
{
    MODULE* module = GetBoard()->m_Modules;

    if( module == NULL )
        return NULL;

    if( aEdge == NULL )       // Start a new edge item
    {
        SaveCopyInUndoList( module, UR_MODEDIT );

        aEdge = new EDGE_MODULE( module );
        MoveVector.x = MoveVector.y = 0;

        // Add the new item to the Drawings list head
        module->GraphicalItems().PushFront( aEdge );

        // Update characteristics of the segment or arc.
        aEdge->SetFlags( IS_NEW );
        aEdge->SetAngle( 0 );
        aEdge->SetShape( type_edge );

        if( aEdge->GetShape() == S_ARC )
            aEdge->SetAngle( ArcValue );

        aEdge->SetWidth( GetDesignSettings().m_ModuleSegmentWidth );
        aEdge->SetLayer( module->GetLayer() );

        // The default layer for an edge is the corresponding silk layer
        if( module->IsFlipped() )
            aEdge->SetLayer( B_SilkS );
        else
            aEdge->SetLayer( F_SilkS );

        // Initialize the starting point of the new segment or arc
        aEdge->SetStart( GetCrossHairPosition() );

        // Initialize the ending point of the new segment or arc
        aEdge->SetEnd( aEdge->GetStart() );

        // Initialize the relative coordinates
        aEdge->SetStart0( aEdge->GetStart() - module->GetPosition() );

        RotatePoint( &aEdge->m_Start0, -module->GetOrientation() );

        aEdge->m_End0 = aEdge->m_Start0;
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
            if( aEdge->m_Start0 != aEdge->m_End0 )
            {
                aEdge->Draw( m_canvas, DC, GR_OR );

                EDGE_MODULE* newedge = new EDGE_MODULE( *aEdge );

                // insert _after_ aEdge, which is the same as inserting before aEdge->Next()
                module->GraphicalItems().Insert( newedge, aEdge->Next() );
                aEdge->ClearFlags();

                aEdge = newedge;     // point now new item

                aEdge->SetFlags( IS_NEW );
                aEdge->SetWidth( GetDesignSettings().m_ModuleSegmentWidth );
                aEdge->SetStart( GetCrossHairPosition() );
                aEdge->SetEnd( aEdge->GetStart() );

                // Update relative coordinate.
                aEdge->SetStart0( aEdge->GetStart() - module->GetPosition() );

                wxPoint pt( aEdge->GetStart0() );

                RotatePoint( &pt, -module->GetOrientation() );

                aEdge->SetStart0( pt );

                aEdge->SetEnd0( aEdge->GetStart0() );

                module->CalculateBoundingBox();
                module->SetLastEditTime();
                OnModify();
            }
        }
        else
        {
            wxMessageBox( wxT( "Begin_Edge() error" ) );
        }
    }

    return aEdge;
}


void FOOTPRINT_EDIT_FRAME::End_Edge_Module( EDGE_MODULE* aEdge )
{
    MODULE* module = GetBoard()->m_Modules;

    if( aEdge )
    {
        aEdge->ClearFlags();

        // If last segment length is 0: remove it
        if( aEdge->GetStart() == aEdge->GetEnd() )
            aEdge->DeleteStructure();
    }

    module->CalculateBoundingBox();
    module->SetLastEditTime();
    OnModify();
    m_canvas->SetMouseCapture( NULL, NULL );
}

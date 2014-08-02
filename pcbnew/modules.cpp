/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/*
 * @file modules.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <trigo.h>
#include <macros.h>
#include <pcbcommon.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <protos.h>
#include <drag.h>


static void MoveFootprint( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                           const wxPoint& aPosition, bool aErase );
static void Abort_MoveOrCopyModule( EDA_DRAW_PANEL* Panel, wxDC* DC );


static MODULE*           s_ModuleInitialCopy = NULL;    /* Copy of module for
                                                         * abort/undo command
                                                         */
static PICKED_ITEMS_LIST s_PickedList;                  /* a picked list to
                                                         * save initial module
                                                         * and dragged tracks
                                                         */

/* Get a module name from user and return a pointer to the corresponding module
 */
MODULE* PCB_BASE_FRAME::GetModuleByName()
{
    wxString          moduleName;
    MODULE*           module = NULL;

    wxTextEntryDialog dlg( this, _( "Name:" ), _( "Search footprint" ), moduleName );

    if( dlg.ShowModal() != wxID_OK )
        return NULL;    //Aborted by user

    moduleName = dlg.GetValue();
    moduleName.Trim( true );
    moduleName.Trim( false );

    if( !moduleName.IsEmpty() )
    {
        module = GetBoard()->m_Modules;

        while( module )
        {
            if( module->GetReference().CmpNoCase( moduleName ) == 0 )
                break;

            module = module->Next();
        }
    }

    return module;
}


void PCB_EDIT_FRAME::StartMoveModule( MODULE* aModule, wxDC* aDC,
                                      bool aDragConnectedTracks )
{
    if( aModule == NULL )
        return;

    if( s_ModuleInitialCopy )
        delete s_ModuleInitialCopy;

    s_PickedList.ClearItemsList();  // Should be empty, but...

    // Creates a copy of the current module, for abort and undo commands
    s_ModuleInitialCopy = (MODULE*)aModule->Clone();
    s_ModuleInitialCopy->SetParent( GetBoard() );
    s_ModuleInitialCopy->ClearFlags();

    SetCurItem( aModule );
    GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;
    aModule->SetFlags( IS_MOVED );

    /* Show ratsnest. */
    if( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
        DrawGeneralRatsnest( aDC );

    EraseDragList();

    if( aDragConnectedTracks )
    {
        DRAG_LIST drglist( GetBoard() );
        drglist.BuildDragListe( aModule );

        ITEM_PICKER itemWrapper( NULL, UR_CHANGED );

        for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
        {
            TRACK* segm = g_DragSegmentList[ii].m_Track;
            itemWrapper.SetItem( segm );
            itemWrapper.SetLink( segm->Clone() );
            itemWrapper.GetLink()->SetState( IN_EDIT, false );
            s_PickedList.PushItem( itemWrapper );
        }

        UndrawAndMarkSegmentsToDrag( m_canvas, aDC );
    }

    GetBoard()->m_Status_Pcb |= DO_NOT_SHOW_GENERAL_RASTNEST;
    m_canvas->SetMouseCapture( MoveFootprint, Abort_MoveOrCopyModule );
    m_canvas->SetAutoPanRequest( true );

    // Erase the module.
    if( aDC )
    {
        aModule->SetFlags( DO_NOT_DRAW );
        m_canvas->RefreshDrawingRect( aModule->GetBoundingBox() );
        aModule->ClearFlags( DO_NOT_DRAW );
    }

    m_canvas->CallMouseCapture( aDC, wxDefaultPosition, false );
}


/* Called on a move or copy module command abort
 */
void Abort_MoveOrCopyModule( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    TRACK*               pt_segm;
    MODULE*              module;
    PCB_EDIT_FRAME*      pcbframe = (PCB_EDIT_FRAME*) Panel->GetParent();

    module = (MODULE*) pcbframe->GetScreen()->GetCurItem();
    pcbframe->GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;
    Panel->SetMouseCapture( NULL, NULL );

    if( module )
    {
        // Erase the current footprint on screen
        DrawModuleOutlines( Panel, DC, module );

        /* If a move command: return to old position
         * If a copy command, delete the new footprint
         */
        if( module->IsMoving() )
        {
            /* Restore old position for dragged tracks */
            for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
            {
                pt_segm = g_DragSegmentList[ii].m_Track;
                pt_segm->Draw( Panel, DC, GR_XOR );
                pt_segm->SetState( IN_EDIT, false );
                pt_segm->ClearFlags();
                g_DragSegmentList[ii].RestoreInitialValues();
                pt_segm->Draw( Panel, DC, GR_OR );
            }

            EraseDragList();
            module->ClearFlags( IS_MOVED );
        }

        if( module->IsNew() )  // Copy command: delete new footprint
        {
            module->DeleteStructure();
            module = NULL;
            pcbframe->GetBoard()->m_Status_Pcb = 0;
            pcbframe->GetBoard()->BuildListOfNets();
        }
    }

    /* Redraw the module. */
    if( module && s_ModuleInitialCopy )
    {
        if( s_ModuleInitialCopy->GetOrientation() != module->GetOrientation() )
            pcbframe->Rotate_Module( NULL, module, s_ModuleInitialCopy->GetOrientation(), false );

        if( s_ModuleInitialCopy->GetLayer() != module->GetLayer() )
            pcbframe->Change_Side_Module( module, NULL );

        module->Draw( Panel, DC, GR_OR );
    }

    pcbframe->SetCurItem( NULL );

    delete s_ModuleInitialCopy;
    s_ModuleInitialCopy = NULL;
    s_PickedList.ClearListAndDeleteItems();

    // Display ratsnest is allowed
    pcbframe->GetBoard()->m_Status_Pcb &= ~DO_NOT_SHOW_GENERAL_RASTNEST;

    if( pcbframe->GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
        pcbframe->DrawGeneralRatsnest( DC );

#ifdef __WXMAC__
    Panel->Refresh();
#endif
}


/* Redraw the footprint when moving the mouse.
 */
void MoveFootprint( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition, bool aErase )
{
    MODULE* module = (MODULE*) aPanel->GetScreen()->GetCurItem();

    if( module == NULL )
        return;

    /* Erase current footprint. */
    if( aErase )
    {
        DrawModuleOutlines( aPanel, aDC, module );
    }

    /* Redraw the module at the new position. */
    g_Offset_Module = module->GetPosition() - aPanel->GetParent()->GetCrossHairPosition();
    DrawModuleOutlines( aPanel, aDC, module );

    DrawSegmentWhileMovingFootprint( aPanel, aDC );
}


bool PCB_EDIT_FRAME::Delete_Module( MODULE* aModule, wxDC* aDC, bool aAskBeforeDeleting )
{
    wxString msg;

    if( aModule == NULL )
        return false;

    SetMsgPanel( aModule );

    /* Confirm module delete. */
    if( aAskBeforeDeleting )
    {
        msg.Printf( _( "Delete Module %s (value %s) ?" ),
                    GetChars( aModule->GetReference() ),
                    GetChars( aModule->GetValue() ) );

        if( !IsOK( this, msg ) )
        {
            return false;
        }
    }

    OnModify();

    /* Remove module from list, and put it in undo command list */
    m_Pcb->m_Modules.Remove( aModule );
    aModule->SetState( IS_DELETED, true );
    SaveCopyInUndoList( aModule, UR_DELETED );

    if( aDC && GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
        Compile_Ratsnest( aDC, true );

    // Redraw the full screen to ensure perfect display of board and ratsnest.
    if( aDC )
        m_canvas->Refresh();

    return true;
}


void PCB_EDIT_FRAME::Change_Side_Module( MODULE* Module, wxDC* DC )
{
    if( Module == NULL )
        return;

    if( ( Module->GetLayer() != F_Cu ) && ( Module->GetLayer() != B_Cu ) )
        return;

    OnModify();

    if( !Module->IsMoving() ) /* This is a simple flip, no other edition in progress */
    {
        GetBoard()->m_Status_Pcb &= ~( LISTE_RATSNEST_ITEM_OK | CONNEXION_OK );

        if( DC )
        {
            Module->SetFlags( DO_NOT_DRAW );
            m_canvas->RefreshDrawingRect( Module->GetBoundingBox() );
            Module->ClearFlags( DO_NOT_DRAW );
        }

        /* Show ratsnest if necessary. */
        if( DC && GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
            DrawGeneralRatsnest( DC );

        g_Offset_Module.x = 0;
        g_Offset_Module.y = 0;
    }
    else    // Module is being moved.
    {
        /* Erase footprint and draw outline if it has been already drawn. */
        if( DC )
        {
            DrawModuleOutlines( m_canvas, DC, Module );
            DrawSegmentWhileMovingFootprint( m_canvas, DC );
        }
    }

    /* Flip the module */
    Module->Flip( Module->GetPosition() );

    SetMsgPanel( Module );

    if( !Module->IsMoving() ) /* Inversion simple */
    {
        if( DC )
        {
            Module->Draw( m_canvas, DC, GR_OR );

            if( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
                Compile_Ratsnest( DC, true );
        }
    }
    else
    {
        if( DC )
        {
            DrawModuleOutlines( m_canvas, DC, Module );
            DrawSegmentWhileMovingFootprint( m_canvas, DC );
        }

        GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;
    }
}


void PCB_BASE_FRAME::PlaceModule( MODULE* aModule, wxDC* aDC, bool aDoNotRecreateRatsnest )
{
    wxPoint newpos;

    if( aModule == 0 )
        return;

    OnModify();
    GetBoard()->m_Status_Pcb &= ~( LISTE_RATSNEST_ITEM_OK | CONNEXION_OK);

    if( aModule->IsNew() )
    {
        SaveCopyInUndoList( aModule, UR_NEW );
    }
    else if( aModule->IsMoving() )
    {
        ITEM_PICKER picker( aModule, UR_CHANGED );
        picker.SetLink( s_ModuleInitialCopy );
        s_PickedList.PushItem( picker );
        s_ModuleInitialCopy = NULL;     // the picker is now owner of s_ModuleInitialCopy.
    }

    if( s_PickedList.GetCount() )
    {
        SaveCopyInUndoList( s_PickedList, UR_UNSPECIFIED );

        // Clear list, but DO NOT delete items, because they are owned by the saved undo
        // list and they therefore in use
        s_PickedList.ClearItemsList();
    }

    if( g_Show_Module_Ratsnest && ( GetBoard()->m_Status_Pcb & LISTE_PAD_OK ) && aDC )
        TraceModuleRatsNest( aDC );

    newpos = GetCrossHairPosition();
    aModule->SetPosition( newpos );
    aModule->ClearFlags();

    delete s_ModuleInitialCopy;
    s_ModuleInitialCopy = NULL;

    if( aDC )
        aModule->Draw( m_canvas, aDC, GR_OR );

    // Redraw dragged track segments, if any
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        TRACK * track = g_DragSegmentList[ii].m_Track;
        track->SetState( IN_EDIT, false );
        track->ClearFlags();

        if( aDC )
            track->Draw( m_canvas, aDC, GR_OR );
    }

    // Delete drag list
    EraseDragList();

    m_canvas->SetMouseCapture( NULL, NULL );

    if( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) && !aDoNotRecreateRatsnest )
        Compile_Ratsnest( aDC, true );

    if( aDC )
        m_canvas->Refresh();

    SetMsgPanel( aModule );
}


/*
 * Rotate the footprint angle degrees in the direction < 0.
 * If incremental == true, the rotation is made from the last orientation,
 * If the module is placed in the absolute orientation angle.
 * If DC == NULL, the component does not redraw.
 * Otherwise, it erases and redraws turns
 */
void PCB_BASE_FRAME::Rotate_Module( wxDC* DC, MODULE* module, double angle, bool incremental )
{
    if( module == NULL )
        return;

    OnModify();

    if( !module->IsMoving() ) /* This is a simple rotation, no other
                                           * edition in progress */
    {
        if( DC )                          // Erase footprint to screen
        {
            module->SetFlags( DO_NOT_DRAW );
            m_canvas->RefreshDrawingRect( module->GetBoundingBox() );
            module->ClearFlags( DO_NOT_DRAW );

            if( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
                DrawGeneralRatsnest( DC );
        }
    }
    else
    {
        if( DC )
        {
            DrawModuleOutlines( m_canvas, DC, module );
            DrawSegmentWhileMovingFootprint( m_canvas, DC );
        }
    }

    GetBoard()->m_Status_Pcb &= ~( LISTE_RATSNEST_ITEM_OK | CONNEXION_OK );

    if( incremental )
        module->SetOrientation( module->GetOrientation() + angle );
    else
        module->SetOrientation( angle );

    SetMsgPanel( module );

    if( DC )
    {
        if( !module->IsMoving() )
        {
            //  not beiing moved: redraw the module and update ratsnest
            module->Draw( m_canvas, DC, GR_OR );

            if( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
                Compile_Ratsnest( DC, true );
        }
        else
        {
            // Beiing moved: just redraw it
            DrawModuleOutlines( m_canvas, DC, module );
            DrawSegmentWhileMovingFootprint( m_canvas, DC );
        }

        if( module->GetFlags() == 0 )  // module not in edit: redraw full screen
            m_canvas->Refresh();
    }
}


/*************************************************/
/* Redraw in XOR mode the outlines of a module. */
/*************************************************/
void DrawModuleOutlines( EDA_DRAW_PANEL* panel, wxDC* DC, MODULE* module )
{
    int    pad_fill_tmp;
    D_PAD* pt_pad;

    if( module == NULL )
        return;

    module->DrawEdgesOnly( panel, DC, g_Offset_Module, GR_XOR );

    // Show pads in sketch mode to speedu up drawings
    pad_fill_tmp = DisplayOpt.DisplayPadFill;
    DisplayOpt.DisplayPadFill = true;

    pt_pad = module->Pads();

    for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
        pt_pad->Draw( panel, DC, GR_XOR, g_Offset_Module );

    DisplayOpt.DisplayPadFill = pad_fill_tmp;

    if( g_Show_Module_Ratsnest && panel )
    {
        PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) panel->GetParent();
        frame->build_ratsnest_module( module );
        frame->TraceModuleRatsNest( DC );
    }
}

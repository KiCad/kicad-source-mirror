/***************/
/* modules.cpp */
/***************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "trigo.h"

#include "protos.h"

#include "drag.h"


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
MODULE* WinEDA_BasePcbFrame::GetModuleByName()
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
            if( module->m_Reference->m_Text.CmpNoCase( moduleName ) == 0 )
                break;
            module = module->Next();
        }
    }
    return module;
}


void WinEDA_PcbFrame::StartMove_Module( MODULE* module, wxDC* DC )
{
    if( module == NULL )
        return;

    if( s_ModuleInitialCopy )
        delete s_ModuleInitialCopy;

    s_PickedList.ClearItemsList();  // Should be empty, but...
    // Creates a copy of the current module, for abort and undo commands
    s_ModuleInitialCopy = new MODULE( GetBoard() );
    s_ModuleInitialCopy->Copy( module );
    s_ModuleInitialCopy->m_Flags = 0;

    SetCurItem( module );
    GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;
    module->m_Flags |= IS_MOVED;

    /* Show ratsnest. */
    if( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
        DrawGeneralRatsnest( DC );

    EraseDragList();

    if( g_Drag_Pistes_On )
    {
        Build_Drag_Liste( DrawPanel, DC, module );
        ITEM_PICKER itemWrapper( NULL, UR_CHANGED );
        for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
        {
            TRACK* segm = g_DragSegmentList[ii].m_Segm;
            itemWrapper.m_PickedItem = segm;
            itemWrapper.m_Link = segm->Copy();
            itemWrapper.m_Link->SetState( IN_EDIT, OFF );
            s_PickedList.PushItem( itemWrapper );
        }
    }

    GetBoard()->m_Status_Pcb |= DO_NOT_SHOW_GENERAL_RASTNEST;
    DrawPanel->SetMouseCapture( Montre_Position_Empreinte, Abort_MoveOrCopyModule );
    DrawPanel->m_AutoPAN_Request = true;

    // Erase the module.
    if( DC )
    {
        int tmp = module->m_Flags;
        module->m_Flags |= DO_NOT_DRAW;
        DrawPanel->RefreshDrawingRect( module->GetBoundingBox() );
        module->m_Flags = tmp;
    }

    DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, FALSE );
}


/* Called on a move or copy module command abort
 */
void Abort_MoveOrCopyModule( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    TRACK*                pt_segm;
    MODULE*               module;
    WinEDA_PcbFrame*      pcbframe = (WinEDA_PcbFrame*) Panel->GetParent();

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
        if( module->m_Flags & IS_MOVED ) // Move command
        {
            if( g_Drag_Pistes_On )
            {
                /* Erase on screen dragged tracks */
                for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
                {
                    pt_segm = g_DragSegmentList[ii].m_Segm;
                    pt_segm->Draw( Panel, DC, GR_XOR );
                }
            }

            /* Go to old position for dragged tracks */
            for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
            {
                pt_segm = g_DragSegmentList[ii].m_Segm;
                pt_segm->SetState( IN_EDIT, OFF );
                g_DragSegmentList[ii].SetInitialValues();
                pt_segm->Draw( Panel, DC, GR_OR );
            }

            EraseDragList();
            module->m_Flags = 0;
        }

        if( (module->IsNew()) )  // Copy command: delete new footprint
        {
            module->DeleteStructure();
            module = NULL;
            pcbframe->GetBoard()->m_Status_Pcb = 0;
            pcbframe->GetBoard()->m_NetInfo->BuildListOfNets();
        }
    }

    /* Redraw the module. */
    if( module && s_ModuleInitialCopy )
    {
        if( s_ModuleInitialCopy->m_Orient != module->m_Orient )
            pcbframe->Rotate_Module( NULL,
                                     module,
                                     s_ModuleInitialCopy->m_Orient,
                                     FALSE );

        if( s_ModuleInitialCopy->GetLayer() != module->GetLayer() )
            pcbframe->Change_Side_Module( module, NULL );

        module->Draw( Panel, DC, GR_OR );
    }

    g_Drag_Pistes_On     = FALSE;
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


/**
 * Function Copie_Module
 * Copy an existing  footprint. The new footprint is added in module list
 * @param module = footprint to copy
 * @return a pointer on the new footprint (the copy of the existing footprint)
 */
MODULE* WinEDA_BasePcbFrame::Copie_Module( MODULE* module )
{
    MODULE* newmodule;

    if( module == NULL )
        return NULL;

    OnModify();

    /* Duplicate module */
    GetBoard()->m_Status_Pcb = 0;
    newmodule = new MODULE( GetBoard() );
    newmodule->Copy( module );

    GetBoard()->Add( newmodule, ADD_APPEND );

    newmodule->m_Flags = IS_NEW;

    GetBoard()->m_NetInfo->BuildListOfNets();

    newmodule->DisplayInfo( this );
    GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;
    return newmodule;
}


/* Redraw the footprint when moving the mouse.
 */
void Montre_Position_Empreinte( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                bool aErase )
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
    g_Offset_Module = module->m_Pos - aPanel->GetScreen()->GetCrossHairPosition();
    DrawModuleOutlines( aPanel, aDC, module );

    Dessine_Segments_Dragges( aPanel, aDC );
}


/**
 * Function Delete Module
 * Remove a footprint from m_Modules linked list and put it in undelete buffer
 * The ratsnest and pad list are recalculated
 * @param aModule = footprint to delete
 * @param aDC = currentDevice Context. if NULL: do not redraw new ratsnest
 * @param aAskBeforeDeleting : if true: ask for confirmation before deleting
 */
bool WinEDA_PcbFrame::Delete_Module( MODULE* aModule,
                                     wxDC*   aDC,
                                     bool    aAskBeforeDeleting )
{
    wxString msg;

    if( aModule == NULL )
        return FALSE;

    aModule->DisplayInfo( this );

    /* Confirm module delete. */
    if( aAskBeforeDeleting )
    {
        msg.Printf( _( "Delete Module %s (value %s) ?" ),
                   GetChars( aModule->m_Reference->m_Text ),
                   GetChars( aModule->m_Value->m_Text ) );
        if( !IsOK( this, msg ) )
        {
            return FALSE;
        }
    }

    OnModify();

    /* Remove module from list, and put it in undo command list */
    m_Pcb->m_Modules.Remove( aModule );
    aModule->SetState( IS_DELETED, ON );
    SaveCopyInUndoList( aModule, UR_DELETED );

    if( aDC && GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
        Compile_Ratsnest( aDC, true );

    // Redraw the full screen to ensure perfect display of board and ratsnest.
    if( aDC )
        DrawPanel->Refresh();

    return true;
}


/**
 * Function Change_Side_Module
 * Flip a footprint (switch layer from component or component to copper)
 * The mirroring is made from X axis
 * if a footprint is not on copper or component layer it is not flipped
 * (it could be on an adhesive layer, not supported at this time)
 * @param Module the footprint to flip
 * @param  DC Current Device Context. if NULL, no redraw
 */
void WinEDA_PcbFrame::Change_Side_Module( MODULE* Module, wxDC* DC )
{
    if( Module == NULL )
        return;
    if( ( Module->GetLayer() != LAYER_N_FRONT )
       && ( Module->GetLayer() != LAYER_N_BACK ) )
        return;

    OnModify();

    if( !( Module->m_Flags & IS_MOVED ) ) /* This is a simple flip, no other
                                           *edition in progress */
    {
        GetBoard()->m_Status_Pcb &= ~( LISTE_RATSNEST_ITEM_OK | CONNEXION_OK );
        if( DC )
        {
            int tmp = Module->m_Flags;
            Module->m_Flags |= DO_NOT_DRAW;
            DrawPanel->RefreshDrawingRect( Module->GetBoundingBox() );
            Module->m_Flags = tmp;
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
            DrawModuleOutlines( DrawPanel, DC, Module );
            Dessine_Segments_Dragges( DrawPanel, DC );
        }
    }

    /* Flip the module */
    Module->Flip( Module->m_Pos );

    Module->DisplayInfo( this );

    if( !( Module->m_Flags & IS_MOVED ) ) /* Inversion simple */
    {
        if( DC )
        {
            Module->Draw( DrawPanel, DC, GR_OR );
            if( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
                Compile_Ratsnest( DC, true );
        }
    }
    else
    {
        if( DC )
        {
            DrawModuleOutlines( DrawPanel, DC, Module );
            Dessine_Segments_Dragges( DrawPanel, DC );
        }
        GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;
    }
}


/* Place module at cursor position.
 *
 * DC (if NULL: no display screen has the output.
 * Update module coordinates with the new position.
 */
void WinEDA_BasePcbFrame::Place_Module( MODULE* module,
                                        wxDC*   DC,
                                        bool    aDoNotRecreateRatsnest )
{
    TRACK*  pt_segm;
    wxPoint newpos;

    if( module == 0 )
        return;

    OnModify();
    GetBoard()->m_Status_Pcb &= ~( LISTE_RATSNEST_ITEM_OK | CONNEXION_OK);

    if( module->IsNew() )
    {
        SaveCopyInUndoList( module, UR_NEW );
    }
    else if( (module->m_Flags & IS_MOVED ) )
    {
        ITEM_PICKER picker( module, UR_CHANGED );
        picker.m_Link = s_ModuleInitialCopy;
        s_PickedList.PushItem( picker );
        s_ModuleInitialCopy = NULL;     // the picker is now owner of
                                        // s_ModuleInitialCopy.
    }

    if( s_PickedList.GetCount() )
    {
        SaveCopyInUndoList( s_PickedList, UR_UNSPECIFIED );

        // Clear list, but DO NOT delete items,
        // because they are owned by the saved undo list and they therefore in
        // use
        s_PickedList.ClearItemsList();
    }

    if( g_Show_Module_Ratsnest && ( GetBoard()->m_Status_Pcb & LISTE_PAD_OK )
        && DC )
        trace_ratsnest_module( DC );

    newpos = GetScreen()->GetCrossHairPosition();
    module->SetPosition( newpos );
    module->m_Flags = 0;

    delete s_ModuleInitialCopy;
    s_ModuleInitialCopy = NULL;

    if( DC )
        module->Draw( DrawPanel, DC, GR_OR );

    if( g_DragSegmentList.size() )
    {
        /* Redraw dragged track segments */
        for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
        {
            pt_segm = g_DragSegmentList[ii].m_Segm;
            pt_segm->SetState( IN_EDIT, OFF );

            if( DC )
                pt_segm->Draw( DrawPanel, DC, GR_OR );
        }

        // Delete drag list
        EraseDragList();
    }

    g_Drag_Pistes_On = false;
    DrawPanel->SetMouseCapture( NULL, NULL );

    if( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) && !aDoNotRecreateRatsnest )
        Compile_Ratsnest( DC, true );

    if( DC )
        DrawPanel->Refresh();

    module->DisplayInfo( this );
}


/*
 * Rotate the footprint angle degrees in the direction < 0.
 * If incremental == true, the rotation is made from the last orientation,
 * If the module is placed in the absolute orientation angle.
 * If DC == NULL, the component does not redraw.
 * Otherwise, it erases and redraws turns
 */
void WinEDA_BasePcbFrame::Rotate_Module( wxDC* DC, MODULE* module,
                                         int angle, bool incremental )
{
    if( module == NULL )
        return;

    OnModify();

    if( !( module->m_Flags & IS_MOVED ) ) /* This is a simple rotation, no other
                                           * edition in progress */
    {
        if( DC )                          // Erase footprint to screen
        {
            int tmp = module->m_Flags;
            module->m_Flags |= DO_NOT_DRAW;
            DrawPanel->RefreshDrawingRect( module->GetBoundingBox() );
            module->m_Flags = tmp;

            if( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
                DrawGeneralRatsnest( DC );
        }
    }
    else
    {
        if( DC )
        {
            DrawModuleOutlines( DrawPanel, DC, module );
            Dessine_Segments_Dragges( DrawPanel, DC );
        }
    }

    GetBoard()->m_Status_Pcb &= ~( LISTE_RATSNEST_ITEM_OK | CONNEXION_OK );

    if( incremental )
        module->SetOrientation( module->m_Orient + angle );
    else
        module->SetOrientation( angle );

    module->DisplayInfo( this );

    if( DC )
    {
        if( !( module->m_Flags & IS_MOVED ) )
        {
            //  not beiing moved: redraw the module and update ratsnest
            module->Draw( DrawPanel, DC, GR_OR );
            if( GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
                Compile_Ratsnest( DC, true );
        }
        else
        {
            // Beiing moved: just redraw it
            DrawModuleOutlines( DrawPanel, DC, module );
            Dessine_Segments_Dragges( DrawPanel, DC );
        }

        if( module->m_Flags == 0 )  // module not in edit: redraw full screen
            DrawPanel->Refresh();
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

    if( g_Show_Pads_Module_in_Move )
    {
        pad_fill_tmp = DisplayOpt.DisplayPadFill;
        DisplayOpt.DisplayPadFill = true;

        pt_pad = module->m_Pads;
        for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
        {
            pt_pad->Draw( panel, DC, GR_XOR, g_Offset_Module );
        }

        DisplayOpt.DisplayPadFill = pad_fill_tmp;
    }

    if( g_Show_Module_Ratsnest && panel )
    {
        WinEDA_BasePcbFrame* frame = (WinEDA_BasePcbFrame*) panel->GetParent();
        frame->build_ratsnest_module( module );
        frame->trace_ratsnest_module( DC );
    }
}

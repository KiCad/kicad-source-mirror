/************************************************/
/* Module editor: Dialog box for editing module */
/*  properties and characteristics              */
/************************************************/

#include <fctsys.h>
#include <confirm.h>
#include <class_drawpanel.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <module_editor_frame.h>
#include <3d_viewer.h>

#include <class_module.h>
#include <class_pad.h>
#include <class_edge_mod.h>

#include <dialog_edit_module_for_BoardEditor.h>


/*
 * Show module property dialog.
 */
void PCB_EDIT_FRAME::InstallModuleOptionsFrame( MODULE* Module, wxDC* DC )
{
    if( Module == NULL )
        return;

#ifndef __WXMAC__
    DIALOG_MODULE_BOARD_EDITOR* dialog = new DIALOG_MODULE_BOARD_EDITOR( this, Module, DC );
#else
    // avoid Avoid "writes" in the dialog, creates errors with WxOverlay and NSView & Modal
    // Raising an Exception - Fixes #764678
    DIALOG_MODULE_BOARD_EDITOR* dialog = new DIALOG_MODULE_BOARD_EDITOR( this, Module, NULL );
#endif

    int retvalue = dialog->ShowModal(); /* retvalue =
                                         *  -1 if abort,
                                         *  0 if exchange module,
                                         *  1 for normal edition
                                         *  and 2 for a goto editor command
                                         */
    dialog->Destroy();

#ifdef __WXMAC__
    // If something edited, push a refresh request
    if (retvalue == 0 || retvalue == 1)
        m_canvas->Refresh();
#endif

    if( retvalue == 2 )
    {
        FOOTPRINT_EDIT_FRAME * editorFrame =
                FOOTPRINT_EDIT_FRAME::GetActiveFootprintEditor();
        if( editorFrame == NULL )
            editorFrame = new FOOTPRINT_EDIT_FRAME( this );

        editorFrame->Load_Module_From_BOARD( Module );
        SetCurItem( NULL );

        editorFrame->Show( true );
        editorFrame->Iconize( false );
    }
}


void FOOTPRINT_EDIT_FRAME::RemoveStruct( EDA_ITEM* Item )
{
    if( Item == NULL )
        return;

    switch( Item->Type() )
    {
    case PCB_PAD_T:
        DeletePad( (D_PAD*) Item, false );
        break;

    case PCB_MODULE_TEXT_T:
    {
        TEXTE_MODULE* text = (TEXTE_MODULE*) Item;

        switch( text->GetType() )
        {
        case TEXTE_MODULE::TEXT_is_REFERENCE:
            DisplayError( this, _( "Cannot delete REFERENCE!" ) );
            break;

        case TEXTE_MODULE::TEXT_is_VALUE:
            DisplayError( this, _( "Cannot delete VALUE!" ) );
            break;

        default:               
            DeleteTextModule( text );
        }
    }
    break;

    case PCB_MODULE_EDGE_T:
        Delete_Edge_Module( (EDGE_MODULE*) Item );
        m_canvas->Refresh();
        break;

    case PCB_MODULE_T:
        break;

    default:
    {
        wxString Line;
        Line.Printf( wxT( " RemoveStruct: item type %d unknown." ), Item->Type() );
        wxMessageBox( Line );
    }
    break;
    }
}

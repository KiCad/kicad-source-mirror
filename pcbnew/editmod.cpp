/************************************************/
/* Module editor: Dialog box for editing module */
/*  properties and characteristics              */
/************************************************/

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <module_editor_frame.h>
#include <trigo.h>
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

    DIALOG_MODULE_BOARD_EDITOR* dialog = new DIALOG_MODULE_BOARD_EDITOR( this, Module, DC );

    int retvalue = dialog->ShowModal(); /* retvalue =
                                         *  -1 if abort,
                                         *  0 if exchange module,
                                         *  1 for normal edition
                                         *  and 2 for a goto editor command
                                         */
    dialog->Destroy();

    if( retvalue == 2 )
    {
        if( m_ModuleEditFrame == NULL )
        {
            m_ModuleEditFrame = new FOOTPRINT_EDIT_FRAME( this,
                                                          _( "Module Editor" ),
                                                          wxPoint( -1, -1 ),
                                                          wxSize( 600, 400 ) );
        }

        m_ModuleEditFrame->Load_Module_From_BOARD( Module );
        SetCurItem( NULL );

        m_ModuleEditFrame->Show( true );
        m_ModuleEditFrame->Iconize( false );
    }
}


/*
 * Move the footprint anchor position to the current cursor position.
 */
void FOOTPRINT_EDIT_FRAME::Place_Ancre( MODULE* pt_mod )
{
    wxPoint   moveVector;
    EDA_ITEM* item;
    D_PAD*    pad;

    if( pt_mod == NULL )
        return;

    moveVector = pt_mod->m_Pos - GetScreen()->GetCrossHairPosition();

    pt_mod->m_Pos = GetScreen()->GetCrossHairPosition();

    /* Update the relative coordinates:
     * The coordinates are relative to the anchor point.
     * Calculate deltaX and deltaY from the anchor. */
    RotatePoint( &moveVector, -pt_mod->m_Orient );

    /* Update the pad coordinates. */
    pad = (D_PAD*) pt_mod->m_Pads;

    for( ; pad != NULL; pad = pad->Next() )
    {
        pad->m_Pos0 += moveVector;
    }

    /* Update the draw element coordinates. */
    item = pt_mod->m_Drawings;

    for( ; item != NULL; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            #undef STRUCT
            #define STRUCT ( (EDGE_MODULE*) item )
            STRUCT->m_Start0 += moveVector;
            STRUCT->m_End0   += moveVector;
            break;

        case PCB_MODULE_TEXT_T:
            #undef STRUCT
            #define STRUCT ( (TEXTE_MODULE*) item )
            STRUCT->SetPos0( STRUCT->GetPos0() + moveVector );
            break;

        default:
            break;
        }
    }

    pt_mod->CalculateBoundingBox();
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

        if( text->GetType() == TEXT_is_REFERENCE )
        {
            DisplayError( this, _( "Text is REFERENCE!" ) );
            break;
        }

        if( text->GetType() == TEXT_is_VALUE )
        {
            DisplayError( this, _( "Text is VALUE!" ) );
            break;
        }

        DeleteTextModule( text );
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
        Line.Printf( wxT( " Remove: draw item type %d unknown." ), Item->Type() );
        DisplayError( this, Line );
    }
    break;
    }
}

/************************************************/
/* Module editor: Dialog box for editing module	*/
/*  properties and characteristics				*/
/************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"
#include "trigo.h"
#include "3d_viewer.h"

#include "dialog_edit_module_for_BoardEditor.h"


/*
 * Show module property dialog.
 */
void WinEDA_PcbFrame::InstallModuleOptionsFrame( MODULE* Module, wxDC* DC )
{
    if( Module == NULL )
        return;

    DIALOG_MODULE_BOARD_EDITOR* dialog =
        new DIALOG_MODULE_BOARD_EDITOR( this, Module, DC );

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
            m_ModuleEditFrame = new WinEDA_ModuleEditFrame( this,
                                                           _( "Module Editor" ),
                                                           wxPoint( -1, -1 ),
                                                           wxSize( 600, 400 ) );
        }

        m_ModuleEditFrame->Load_Module_From_BOARD( Module );
        SetCurItem( NULL );

        m_ModuleEditFrame->Show( TRUE );
        m_ModuleEditFrame->Iconize( FALSE );
    }
}


/*
 * Position anchor under the cursor.
 */
void WinEDA_ModuleEditFrame::Place_Ancre( MODULE* pt_mod )
{
    wxPoint         moveVector;
    EDA_BaseStruct* PtStruct;
    D_PAD*          pt_pad;

    if( pt_mod == NULL )
        return;

    moveVector = pt_mod->m_Pos - GetScreen()->m_Curseur;

    pt_mod->m_Pos = GetScreen()->m_Curseur;

    /* Update the relative coordinates:
     * The coordinates are relative to the anchor point.
     * Calculate deltaX and deltaY from the anchor. */
    RotatePoint( &moveVector, -pt_mod->m_Orient );

    /* Update the pad coordinates. */
    pt_pad = (D_PAD*) pt_mod->m_Pads;
    for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
    {
        pt_pad->m_Pos0 += moveVector;
    }

    /* Update the draw element coordinates. */
    PtStruct = pt_mod->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_EDGE_MODULE:
                #undef STRUCT
                #define STRUCT ( (EDGE_MODULE*) PtStruct )
            STRUCT->m_Start0 += moveVector;
            STRUCT->m_End0   += moveVector;
            break;

        case TYPE_TEXTE_MODULE:
                #undef STRUCT
                #define STRUCT ( (TEXTE_MODULE*) PtStruct )
            STRUCT->m_Pos0 += moveVector;
            break;

        default:
            break;
        }
    }

    pt_mod->Set_Rectangle_Encadrement();
}


void WinEDA_ModuleEditFrame::RemoveStruct( EDA_BaseStruct* Item )
{
    if( Item == NULL )
        return;

    switch( Item->Type() )
    {
    case TYPE_PAD:
        DeletePad( (D_PAD*) Item, false );
        break;

    case TYPE_TEXTE_MODULE:
    {
        TEXTE_MODULE* text = (TEXTE_MODULE*) Item;
        if( text->m_Type == TEXT_is_REFERENCE )
        {
            DisplayError( this, _( "Text is REFERENCE!" ) );
            break;
        }
        if( text->m_Type == TEXT_is_VALUE )
        {
            DisplayError( this, _( "Text is VALUE!" ) );
            break;
        }
        DeleteTextModule( text );
    }
    break;

    case TYPE_EDGE_MODULE:
        Delete_Edge_Module( (EDGE_MODULE*) Item );
        DrawPanel->Refresh();
        break;

    case TYPE_MODULE:
        break;

    default:
    {
        wxString Line;
        Line.Printf( wxT( " Remove: draw item type %d unknown." ),
                     Item->Type() );
        DisplayError( this, Line );
    }
    break;
    }
}

/************************************************/
/* Module editor: Dialog box for editing module	*/
/*  properties and carateristics				*/
/************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "trigo.h"
#include "bitmaps.h"
#include "3d_struct.h"
#include "3d_viewer.h"

#include "dialog_edit_module.h"

#include "protos.h"

bool GoToEditor = FALSE;

/*******************************************************************/
void WinEDA_BasePcbFrame::InstallModuleOptionsFrame( MODULE* Module, wxDC * DC )
/*******************************************************************/

/* Fonction relai d'installation de la frame d'édition des proprietes
 *  du module*/
{
    WinEDA_ModulePropertiesFrame* frame =
        new WinEDA_ModulePropertiesFrame( this, Module, DC );

    frame->ShowModal(); frame->Destroy();

    if( GoToEditor && GetScreen()->GetCurItem() )
    {
        if( m_ModuleEditFrame == NULL )
        {
            m_ModuleEditFrame = new WinEDA_ModuleEditFrame( this,
                                                            _( "Module Editor" ),
                                                            wxPoint( -1, -1 ),
                                                            wxSize( 600, 400 ) );
        }

        m_ModuleEditFrame->Load_Module_From_BOARD( (MODULE*) GetScreen()->GetCurItem() );
        SetCurItem( NULL );

        GoToEditor = FALSE;
        m_ModuleEditFrame->Show( TRUE );
        m_ModuleEditFrame->Iconize( FALSE );
    }
}


/*******************************************************************/
void WinEDA_ModuleEditFrame::Place_Ancre( MODULE* pt_mod )
/*******************************************************************/

/*
 *  Repositionne l'ancre sous le curseur souris
 *  Le module doit etre d'abort selectionne
 */
{
    wxPoint         delta;
    EDA_BaseStruct* PtStruct;
    D_PAD*          pt_pad;

    if( pt_mod == NULL )
        return;

    delta = pt_mod->m_Pos - GetScreen()->m_Curseur;

    pt_mod->m_Pos = GetScreen()->m_Curseur;

    /* Mise a jour des coord relatives des elements:
     *  les coordonnees relatives sont relatives a l'ancre, pour orient 0.
     *  il faut donc recalculer deltaX et deltaY en orientation 0 */
    RotatePoint( &delta, -pt_mod->m_Orient );

    /* Mise a jour des coord relatives des pads */
    pt_pad = (D_PAD*) pt_mod->m_Pads;
    for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
    {
        pt_pad->m_Pos0 += delta;
    }

    /* Mise a jour des coord relatives contours .. */
    PtStruct = pt_mod->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_EDGE_MODULE:
                #undef STRUCT
                #define STRUCT ( (EDGE_MODULE*) PtStruct )
            STRUCT->m_Start0 += delta;
            STRUCT->m_End0   += delta;
            break;

        case TYPE_TEXTE_MODULE:
                #undef STRUCT
                #define STRUCT ( (TEXTE_MODULE*) PtStruct )
            STRUCT->m_Pos0 += delta;
            break;

        default:
            break;
        }
    }

    pt_mod->Set_Rectangle_Encadrement();
}


/**********************************************************************/
void WinEDA_ModuleEditFrame::RemoveStruct( EDA_BaseStruct* Item )
/**********************************************************************/
{
    if( Item == NULL )
        return;

    switch( Item->Type() )
    {
    case TYPE_PAD:
        DeletePad( (D_PAD*) Item );
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
        Line.Printf( wxT( " Remove: StructType %d Inattendu" ),
                     Item->Type() );
        DisplayError( this, Line );
    }
        break;
    }
}

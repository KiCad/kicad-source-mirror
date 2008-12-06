/**********************************************/
/* GERBVIEW : Routines d'initialisation globale */
/******* Fichier INITPCB.C ********************/
/**********************************************/


#include "fctsys.h"

#include "common.h"
#include "gerbview.h"

#include "protos.h"

/* Routines Locales */


/********************************************************/
bool WinEDA_GerberFrame::Clear_Pcb( bool query )
/********************************************************/

/* Realise les init des pointeurs et variables
 *  Si Item == NULL, il n'y aura pas de confirmation
 */
{
    int layer;

    if( m_Pcb == NULL )
        return FALSE;

    if( query )
    {
        if( m_Pcb->m_Drawings || m_Pcb->m_Track || m_Pcb->m_Zone )
        {
            if( !IsOK( this, _( "Current Data will be lost ?" ) ) )
                return FALSE;
        }
    }

    m_Pcb->m_Drawings.DeleteAll();

    m_Pcb->m_Track.DeleteAll();

    m_Pcb->m_Zone.DeleteAll();

    for( ; g_UnDeleteStackPtr != 0; )
    {
        g_UnDeleteStackPtr--;
        delete g_UnDeleteStack[ g_UnDeleteStackPtr];
    }

    /* init pointeurs  et variables */
    for( layer = 0; layer < 32; layer++ )
    {
        if( g_GERBER_List[layer] )
            g_GERBER_List[layer]->InitToolTable();
    }

    /* remise a 0 ou a une valeur initiale des variables de la structure */
    m_Pcb->m_BoundaryBox.SetOrigin( 0, 0 );
    m_Pcb->m_BoundaryBox.SetSize( 0, 0 );
    m_Pcb->m_Status_Pcb = 0;
    m_Pcb->m_NbLoclinks = 0;
    m_Pcb->m_NbLinks    = 0;
    m_Pcb->m_NbNodes     = 0;
    m_Pcb->m_NbNoconnect = 0;

    /* Init parametres de gestion des ecrans PAD et PCB */
    SetBaseScreen( ActiveScreen = ScreenPcb );
    GetScreen()->Init();

    return TRUE;
}


/*********************************************************/
void WinEDA_GerberFrame::Erase_Zones( bool query )
/*********************************************************/
{
    if( query && !IsOK( this, _( "Delete zones ?" ) ) )
        return;

    m_Pcb->m_Zone.DeleteAll();

    ScreenPcb->SetModify();
}


/************************************************************************/
void WinEDA_GerberFrame::Erase_Segments_Pcb( bool all_layers, bool query )
/************************************************************************/
{
    int  layer = GetScreen()->m_Active_Layer;

    if( all_layers )
        layer = -1;

    BOARD_ITEM*  next;
    for( BOARD_ITEM* item = m_Pcb->m_Drawings;  item;  item = next )
    {
        next = item->Next();

        switch( item->Type() )
        {
        case TYPE_DRAWSEGMENT:
        case TYPE_TEXTE:
        case TYPE_COTATION:
        case TYPE_MIRE:
            if( item->GetLayer() == layer  || layer < 0 )
                m_Pcb->Delete( item );
            break;

        default:
            DisplayError( this, wxT( "Type Draw inconnu/inattendu" ) );
            break;
        }
    }

    ScreenPcb->SetModify();
}


/******************************************************************/
void WinEDA_GerberFrame::Erase_Pistes( int masque_type, bool query )
/******************************************************************/

/* Efface les segments de piste, selon les autorisations affichees
 *  masque_type = masque des options de selection:
 *  SEGM_FIXE, SEGM_AR
 *  Si un des bits est a 1, il n'y a pas effacement du segment de meme bit a 1
 */
{
    TRACK*          pt_segm;
    BOARD_ITEM*     PtNext;

    if( query && !IsOK( this, _( "Delete Tracks?" ) ) )
        return;

    /* Marquage des pistes a effacer */
    for( pt_segm = m_Pcb->m_Track; pt_segm != NULL; pt_segm = (TRACK*) PtNext )
    {
        PtNext = pt_segm->Next();
        if( pt_segm->GetState( SEGM_FIXE | SEGM_AR ) & masque_type )
            continue;
        pt_segm->DeleteStructure();
    }

    ScreenPcb->SetModify();
}


/*****************************************************************/
void WinEDA_GerberFrame::Erase_Textes_Pcb( bool query )
/*****************************************************************/
{
    BOARD_ITEM* PtStruct;
    BOARD_ITEM* PtNext;

    if( query && !IsOK( this, _( "Delete Pcb Texts" ) ) )
        return;

    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtNext )
    {
        PtNext = PtStruct->Next();
        if( PtStruct->Type() == TYPE_TEXTE )
            PtStruct->DeleteStructure();
    }

    ScreenPcb->SetModify();
}


/*********************************************************/
void WinEDA_GerberFrame::Erase_Current_Layer( bool query )
/*********************************************************/
{
    int             layer = GetScreen()->m_Active_Layer;
    wxString        msg;

    msg.Printf( _( "Delete Layer %d" ), layer + 1 );
    if( query && !IsOK( this, msg ) )
        return;

    /* Delete tracks (spots and lines) */
    TRACK*     PtNext;
    for( TRACK* pt_segm = m_Pcb->m_Track; pt_segm != NULL; pt_segm = (TRACK*) PtNext )
    {
        PtNext = pt_segm->Next();
        if( pt_segm->GetLayer() != layer )
            continue;
        pt_segm->DeleteStructure();
    }

    /* Delete polygons */
    SEGZONE*     Nextzone;
    for( SEGZONE* zone = m_Pcb->m_Zone; zone != NULL; zone = Nextzone )
    {
        Nextzone = zone->Next();
        if( zone->GetLayer() != layer )
            continue;
        zone->DeleteStructure();
    }
    ScreenPcb->SetModify();
    ScreenPcb->SetRefreshReq();
}

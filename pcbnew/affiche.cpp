/**********************************************************/
/* Routines d'affichage de parametres et caracteristiques */
/**********************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "3d_struct.h"

#include "protos.h"

/* Routines locales */


/*******************************************************************/
void Affiche_Infos_Equipot( int netcode, WinEDA_BasePcbFrame* frame )
/*******************************************************************/
/* Affiche les infos relatives a une equipot: nb de pads, nets , connexions.. */
{
    int             nb_vias = 0, ii;
    EDA_BaseStruct* Struct;
    wxString        txt;
    MODULE*         module;
    D_PAD*          pad;
    EQUIPOT*        equipot;

    frame->MsgPanel->EraseMsgBox();

    equipot = frame->m_Pcb->FindNet( netcode );
    if( equipot )
        Affiche_1_Parametre( frame, 1, _( "Net Name" ), equipot->m_Netname, RED );
    else
        Affiche_1_Parametre( frame, 1, _( "No Net (not connected)" ), wxEmptyString, RED );

    txt.Printf( wxT( "%d" ), netcode );
    Affiche_1_Parametre( frame, 30, _( "Net Code" ), txt, RED );

    for( ii = 0, module = frame->m_Pcb->m_Modules; module != 0;
         module = (MODULE*) module->Pnext )
    {
        for( pad = module->m_Pads; pad != 0; pad = (D_PAD*) pad->Pnext )
        {
            if( pad->m_NetCode == netcode )
                ii++;
        }
    }

    txt.Printf( wxT( "%d" ), ii );
    Affiche_1_Parametre( frame, 40, _( "Pads" ), txt, DARKGREEN );

    for( ii = 0, Struct = frame->m_Pcb->m_Track; Struct != NULL; Struct = Struct->Pnext )
    {
        ii++;
        if( Struct->Type() == TYPEVIA )
            if( ( (SEGVIA*) Struct )->m_NetCode == netcode )
                nb_vias++;
    }

    txt.Printf( wxT( "%d" ), nb_vias );
    Affiche_1_Parametre( frame, 50, _( "Vias" ), txt, BLUE );
}

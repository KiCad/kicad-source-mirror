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
    double	    lengthnet = 0;

    frame->MsgPanel->EraseMsgBox();

    equipot = frame->m_Pcb->FindNet( netcode );
    if( equipot )
        Affiche_1_Parametre( frame, 1, _( "Net Name" ), equipot->GetNetname(), RED );
    else
        Affiche_1_Parametre( frame, 1, _( "No Net (not connected)" ), wxEmptyString, RED );

    txt.Printf( wxT( "%d" ), netcode );
    Affiche_1_Parametre( frame, 30, _( "Net Code" ), txt, RED );

    for( ii = 0, module = frame->m_Pcb->m_Modules; module != 0;
         module = module->Next() )
    {
        for( pad = module->m_Pads; pad != 0; pad = pad->Next() )
        {
            if( pad->GetNet() == netcode )
                ii++;
        }
    }

    txt.Printf( wxT( "%d" ), ii );
    Affiche_1_Parametre( frame, 40, _( "Pads" ), txt, DARKGREEN );

    for( ii = 0, Struct = frame->m_Pcb->m_Track; Struct != NULL; Struct = Struct->Next() )
    {
        ii++;
        if( Struct->Type() == TYPE_VIA )
            if( ( (SEGVIA*) Struct )->GetNet() == netcode )
                nb_vias++;
    if( Struct->Type() == TYPE_TRACK )
            if( ( (TRACK*) Struct )->GetNet() == netcode )
            lengthnet += ( (TRACK*) Struct )->GetLength();
    }

    txt.Printf( wxT( "%d" ), nb_vias );
    Affiche_1_Parametre( frame, 50, _( "Vias" ), txt, BLUE );

    valeur_param( (int) lengthnet, txt );
    Affiche_1_Parametre( frame, 60, _( "Net Length" ), txt, RED );
}

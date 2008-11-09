/**********************************************************/
/* Routines d'affichage de parametres et caracteristiques */
/**********************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"

#include "protos.h"

/* Routines locales */

/****************************************************************************/
void Affiche_Infos_PCB_Texte( WinEDA_BasePcbFrame* frame, TEXTE_PCB* pt_texte )
/****************************************************************************/

/* Affiche en bas d'ecran les caract du texte sur PCB
 *  Entree :
 *      pointeur de la description du texte
 */
{
    wxString Line;

    frame->MsgPanel->EraseMsgBox();

    if( pt_texte->Type() == TYPECOTATION )
        Affiche_1_Parametre( frame, 1, _( "COTATION" ), pt_texte->m_Text, DARKGREEN );

    else
        Affiche_1_Parametre( frame, 1, _( "PCB Text" ), pt_texte->m_Text, DARKGREEN );

    Line = _( "Layer " );
    Line << pt_texte->GetLayer() + 1;

    Affiche_1_Parametre( frame, 28, _( "Layer:" ), Line,
                         g_DesignSettings.m_LayerColor[pt_texte->GetLayer()] );

    Affiche_1_Parametre( frame, 36, _( "Mirror" ), wxEmptyString, GREEN );

    if( (pt_texte->m_Miroir & 1) )
        Affiche_1_Parametre( frame, -1, wxEmptyString, _( "No" ), DARKGREEN );
    else
        Affiche_1_Parametre( frame, -1, wxEmptyString, _( "Yes" ), DARKGREEN );


    Line.Printf( wxT( "%.1f" ), (float) pt_texte->m_Orient / 10 );
    Affiche_1_Parametre( frame, 43, _( "Orient" ), Line, DARKGREEN );

    valeur_param( pt_texte->m_Width, Line );
    Affiche_1_Parametre( frame, 50, _( "Width" ), Line, MAGENTA );

    valeur_param( pt_texte->m_Size.x, Line );
    Affiche_1_Parametre( frame, 60, _( "H Size" ), Line, RED );

    valeur_param( pt_texte->m_Size.y, Line );
    Affiche_1_Parametre( frame, 70, _( "V Size" ), Line, RED );
}


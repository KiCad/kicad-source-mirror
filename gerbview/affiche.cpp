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
void Affiche_Infos_PCB_Texte(WinEDA_BasePcbFrame * frame, TEXTE_PCB* pt_texte)
/****************************************************************************/
/* Affiche en bas d'ecran les caract du texte sur PCB
	Entree :
		pointeur de la description du texte
*/
{
wxString Line;

	frame->MsgPanel->EraseMsgBox();

	if( pt_texte->m_StructType == TYPECOTATION )
		Affiche_1_Parametre(frame, 1,_("COTATION"),pt_texte->m_Text, DARKGREEN);

	else
		Affiche_1_Parametre(frame, 1,_("PCB Text"),pt_texte->m_Text, DARKGREEN);

	Line = _("Layer "); Line << pt_texte->m_Layer + 1;
	Affiche_1_Parametre(frame, 28, _("Layer:"), Line, g_DesignSettings.m_LayerColor[pt_texte->m_Layer] );

	Affiche_1_Parametre(frame, 36, _("Mirror"),wxEmptyString,GREEN) ;
	if( (pt_texte->m_Miroir & 1) )
			Affiche_1_Parametre(frame, -1,wxEmptyString, _("No"), DARKGREEN) ;
	else	Affiche_1_Parametre(frame, -1,wxEmptyString, _("Yes"), DARKGREEN) ;
 

	Line.Printf( wxT("%.1f"),(float)pt_texte->m_Orient/10 );
	Affiche_1_Parametre(frame, 43,_("Orient"), Line, DARKGREEN) ;

	valeur_param(pt_texte->m_Width,Line) ;
	Affiche_1_Parametre(frame, 50,_("Width"), Line,MAGENTA) ;

	valeur_param(pt_texte->m_Size.x,Line) ;
	Affiche_1_Parametre(frame, 60,_("H Size"), Line,RED) ;

	valeur_param(pt_texte->m_Size.y,Line);
	Affiche_1_Parametre(frame, 70,_("V Size"), Line,RED) ;

}



/*********************************************************************/
void Affiche_Infos_Piste(WinEDA_BasePcbFrame * frame, TRACK * pt_piste)
/*********************************************************************/
/* Affiche les caract principales d'un segment de piste en bas d'ecran */
{
int d_index, ii = -1;
D_CODE * pt_D_code;
int layer = frame->GetScreen()->m_Active_Layer;
wxString msg;
	
	frame->MsgPanel->EraseMsgBox();

	d_index = pt_piste->m_NetCode;
	pt_D_code = ReturnToolDescr(layer, d_index, &ii);

	switch(pt_piste->m_StructType)
		{
		case TYPETRACK:
			if ( pt_piste->m_Shape < S_SPOT_CIRCLE ) msg = wxT("LINE");
			else  msg = wxT("FLASH");
			break;

		case TYPEZONE:
			msg = wxT("ZONE"); break;

		default:
			msg = wxT("????"); break;
		}
	Affiche_1_Parametre(frame, 1, _("Type"), msg, DARKCYAN);

	msg.Printf( wxT("%d"), ii+1);
	Affiche_1_Parametre(frame, 10, _("Tool"), msg, RED);

	if ( pt_D_code )
	{
		msg.Printf( wxT("D%d"), d_index);
		Affiche_1_Parametre(frame, 20, _("D CODE"),msg, BLUE);

		Affiche_1_Parametre(frame, 30, _("D type"),
					pt_D_code ? g_GERBER_Tool_Type[pt_D_code->m_Shape] : _("????"),
					BLUE);
	}

	msg.Printf( wxT("%d"),pt_piste->m_Layer + 1);
	Affiche_1_Parametre(frame, 40, _("Layer"), msg, BROWN) ;

	/* Affiche Epaisseur */
	valeur_param((unsigned)(pt_piste->m_Width), msg) ;
	Affiche_1_Parametre(frame, 50, _("Width"), msg, DARKCYAN) ;
}


	/**********************/
	/* CVPCB: autosel.cpp  */
	/**********************/

/* Routines de selection automatique des modules */

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"

#define QUOTE '\''

class AUTOMODULE
{
public:
	int m_Type;
	AUTOMODULE * Pnext;
	wxString m_Name;
	wxString m_LibName;
	wxString m_Library;

	AUTOMODULE() { m_Type = 0; Pnext = NULL; }
} ;


/* routines locales : */
static int auto_select(WinEDA_CvpcbFrame * frame, STORECMP * Cmp, AUTOMODULE * BaseListeMod);
static void auto_associe(WinEDA_CvpcbFrame * frame);


	/*************************************************************/
void WinEDA_CvpcbFrame::AssocieModule(wxCommandEvent& event)
/*************************************************************/
/* Fonction liee au boutton "Auto"
	Lance l'association automatique modules/composants
*/
{
	auto_associe(this);
}

/**************************************************/
static void auto_associe(WinEDA_CvpcbFrame * frame)
/**************************************************/
{
unsigned ii, j, k;
wxString EquivFileName, msg;
char Line[1024];
FILE *fichierstf ;	/* sert en lecture des differents fichiers *.STF */
AUTOMODULE * ItemModule,* NextMod;
AUTOMODULE * BaseListeMod = NULL;
STORECMP * Component;

int nb_correspondances = 0;

	if( nbcomp <= 0 ) return;

	/* recherche des equivalences a travers les fichiers possibles */
	for( ii= 0 ; ii < g_ListName_Equ.GetCount(); ii++)
		{
		/* Calcul du nom complet avec son chemin */
		EquivFileName = MakeFileName(g_RealLibDirBuffer,g_ListName_Equ[ii],g_EquivExtBuffer);

		if ( (fichierstf = wxFopen(EquivFileName, wxT("rt")))  == 0)
			{
			msg.Printf( _("Library: <%s> not found"),EquivFileName.GetData());
			DisplayError(frame, msg,10);
			continue ;
			}

		/* lecture fichier n */
		while ( fgets(Line,79,fichierstf) != 0 )
		{
			/* elimination des lignes vides */
			for (j = 0 ; j < 40 ; j++ )
			{
				if (Line[j] == 0 ) goto fin_de_while ;
				if (Line[j] == QUOTE ) break ;
			}

			ItemModule = new AUTOMODULE();
			ItemModule->Pnext = BaseListeMod;
			BaseListeMod = ItemModule;

			/* stockage du composant ( 'namecmp'  'namelib')
			name et namelib */
			for ( j++ ; j < 40 ; j++, k++)
			{
				if ( Line[j] == QUOTE) break ;
				ItemModule->m_Name.Append(Line[j]);
			}
			j++ ;
			for ( ; j < 80 ; ) if (Line[j++] == QUOTE) break ;
			for (; ; j++)
				{
				if (Line[j] == QUOTE) break ;
				ItemModule->m_LibName.Append(Line[j]);
				}
			nb_correspondances++ ;
			fin_de_while:;
			}
		fclose(fichierstf) ;

		/* Affichage Statistiques */
		msg.Printf(_("%d equivalences"),nb_correspondances);
		frame->SetStatusText(msg, 0);
		}

	Component = g_BaseListeCmp;
	for ( ii = 0; Component != NULL; Component = Component->Pnext, ii++ )
		{
		frame->m_ListCmp->SetSelection(ii,TRUE);
		if( Component->m_Module.IsEmpty() )
			auto_select(frame, Component, BaseListeMod);
		}

	/* Liberation memoire */
	for( ItemModule = BaseListeMod; ItemModule != NULL; ItemModule = NextMod)
		{
		NextMod = ItemModule->Pnext; delete ItemModule;
		}
	BaseListeMod = NULL;
}


/****************************************************************/
static int auto_select(WinEDA_CvpcbFrame * frame, STORECMP * Cmp,
			AUTOMODULE * BaseListeMod)
/****************************************************************/

/* associe automatiquement composant et Module
	Retourne;
		0 si OK
		1 si module specifie non trouve en liste librairie
		2 si pas de module specifie dans la liste des equivalences
*/
{
AUTOMODULE * ItemModule;
STOREMOD * Module;
wxString msg;

	/* examen de la liste des correspondances */
	ItemModule = BaseListeMod;
	for ( ; ItemModule != NULL; ItemModule = ItemModule->Pnext )
		{
		if ( ItemModule->m_Name.CmpNoCase(Cmp->m_Valeur) != 0) continue;

		/* Correspondance trouvee, recherche nom module dans la liste des
		modules disponibles en librairie */
		Module= g_BaseListePkg;
		for ( ;Module != NULL; Module = Module->Pnext )
		{

			if( ItemModule->m_LibName.CmpNoCase(Module->m_Module) == 0 )
			{ /* empreinte trouv‚e */
				frame->SetNewPkg(Module->m_Module);
				return(0);
			}
		}
		msg.Printf(
				  _("Component %s: Footprint %s not found in libraries"),
						Cmp->m_Valeur.GetData(), ItemModule->m_LibName.GetData());
		DisplayError(frame, msg, 10);
		return( 2 );
		}
	return(1);
}


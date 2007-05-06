	/****************************/
	/*	EESchema - database.cpp	*/
	/****************************/

/* Routine de selection d'un composant en librairie
*/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/* Routines locales */

/* Variables locales */


/********************************************************************************/
bool DataBaseGetName(WinEDA_DrawFrame * frame,
		wxString & Keys, wxString & BufName)
/********************************************************************************/
/*
	Routine de selection du nom d'un composant en librairie pour chargement,
	Keys pointe la liste des mots cles de filtrage
	Si Keys = "", recherche des composants qui correspondent
au masque BufName( avec * et ? )

 Retourne
	TRUE si composant selectionne
	FALSE si commande annulee
	place le nom du composant a charger, selectionne a partir d'une liste dans
	BufName
*/
{
int ii;
EDA_LibComponentStruct * LibEntry;
LibraryStruct * Lib;
WinEDAListBox * ListBox;
const wxChar ** ListNames;

	BufName.MakeUpper();
	Keys.MakeUpper();

	/* Examen de la liste des librairies pour comptage */
	for (Lib = g_LibraryList, ii = 0; Lib != NULL; Lib = Lib->m_Pnext)
		{
		LibEntry = (EDA_LibComponentStruct *) PQFirst(&Lib->m_Entries, FALSE);
		while( LibEntry )
			{
			if ( ! Keys.IsEmpty() )
				{
				if( KeyWordOk(Keys, LibEntry->m_KeyWord) ) ii++;
				}
			else
				{
				if( WildCompareString( BufName, LibEntry->m_Name.m_Text, FALSE ) ) ii++;
				}
			LibEntry = (EDA_LibComponentStruct *) PQNext(Lib->m_Entries, LibEntry, NULL);
			}
		}

	if ( ii == 0 )
		{
		DisplayError(frame, _("No Component found") );
		return 0;
		}

	ListNames = (const wxChar**)MyZMalloc( (ii+1) * sizeof(const wxChar*));
	for (Lib = g_LibraryList, ii = 0; Lib != NULL; Lib = Lib->m_Pnext)
		{
		/* Examen de la liste des elements */
		LibEntry = (EDA_LibComponentStruct *) PQFirst(&Lib->m_Entries, FALSE);
		while( LibEntry )
			{
			if ( ! Keys.IsEmpty())
				{
				if( KeyWordOk(Keys, LibEntry->m_KeyWord) )
					{
					ListNames[ii] = LibEntry->m_Name.m_Text.GetData(); ii++;
					}
				}
			else if( WildCompareString( BufName, LibEntry->m_Name.m_Text, FALSE ) ) 
				{
				ListNames[ii] = LibEntry->m_Name.m_Text; ii++;
				}
			LibEntry = (EDA_LibComponentStruct *) PQNext(Lib->m_Entries, LibEntry, NULL);
			}
		}

	ListBox = new WinEDAListBox(frame, _("Selection"), ListNames,
						wxEmptyString, DisplayCmpDoc, wxColour(200,200,255) );
	ListBox->MoveMouseToOrigin();
		
	/* Affichage de la liste selectionnee */
	if( ii )
		{
		ii = ListBox->ShowModal(); ListBox->Destroy();
		if ( ii < 0 ) ii = 0;
		else
			{
			BufName = ListNames[ii];
			ii = 1;
			}
		}


	free (ListNames);
	return (ii) ;
}


/**********************************/
void DisplayCmpDoc(wxString & Name)
/**********************************/
{
LibCmpEntry * CmpEntry;

	CmpEntry = FindLibPart(Name.GetData(), wxEmptyString, FIND_ALIAS);
	if ( CmpEntry == NULL ) return;

	Name = wxT("Descr: ") + CmpEntry->m_Doc;
	Name += wxT("\nKeyW: ") + CmpEntry->m_KeyWord;
}



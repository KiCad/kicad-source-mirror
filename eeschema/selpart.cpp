	/**************************/
	/* EESchema - selpart.cpp */
	/**************************/

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


/***************************************************************/
LibraryStruct * SelectLibraryFromList(WinEDA_DrawFrame * frame)
/***************************************************************/
/* Routine pour selectionner une librairie a partir d'une liste
*/
{
int ii, NumOfLibs = NumOfLibraries();
LibraryStruct *Lib = NULL;
static wxString OldLibName;
WinEDAListBox * ListBox;
wxString LibName;
const wxChar ** ListNames;

	if (NumOfLibs == 0)
	{
		DisplayError(frame, _("No libraries are loaded"));
		return(NULL) ;
	}

	ListNames = GetLibNames();
	ListBox = new WinEDAListBox(frame,
						_("Select Lib"), ListNames,  OldLibName, NULL,
						wxColour(150,255,255));
	ListBox->MoveMouseToOrigin();

	ii = ListBox->ShowModal(); ListBox->Destroy();

	if (ii >= 0)	/* Recherche de la librairie */
		{
		Lib = FindLibrary(ListNames[ii]);
		}

	free (ListNames);
	return(Lib);
}


/******************************************************************************************/
int DisplayComponentsNamesInLib( WinEDA_DrawFrame * frame,
				LibraryStruct *Library, wxString & Buffer, wxString & OldName)
/******************************************************************************************/
/* Routine de selection d'un composant en librairie, par affichage de la
	liste des composants de cette librairie
	Si Library == NULL, selection de librairie demandee
	sinon recherche uniquement dans library
	Retourne
		1 si composant selectionne
		0 si commande annulee
*/
{
int ii;
wxString msg;
EDA_LibComponentStruct *LibEntry;
WinEDAListBox * ListBox;
const wxChar ** ListNames;

	if(Library == NULL) Library = SelectLibraryFromList(frame);
	if(Library == NULL) return(0);

	PQCompFunc((PQCompFuncType) LibraryEntryCompare);
	LibEntry = (EDA_LibComponentStruct *) PQFirst(&Library->m_Entries, FALSE);

	ii = 0;
	while( LibEntry )
	{
		ii++;
		LibEntry = (EDA_LibComponentStruct *) PQNext(Library->m_Entries, LibEntry, NULL);
	}
	ListNames = (const wxChar**) MyZMalloc( (ii+1) * sizeof(wxChar*));

	msg.Printf( _("Select component (%d items)"), ii );

	ii = 0;
	LibEntry = (EDA_LibComponentStruct *) PQFirst(&Library->m_Entries, FALSE);
	while( LibEntry )
	{
		ListNames[ii++] = LibEntry->m_Name.m_Text.GetData();
		LibEntry = (EDA_LibComponentStruct *) PQNext(Library->m_Entries, LibEntry, NULL);
	}

//	Qsort(ListNames,StrNumICmp);

	ListBox = new WinEDAListBox(frame, msg,
							ListNames,  OldName, DisplayCmpDoc,
							wxColour(255,255,200));
	ListBox->MoveMouseToOrigin();
	
	ii = ListBox->ShowModal(); ListBox->Destroy();
	if ( ii >= 0 ) Buffer = ListNames[ii];

	free (ListNames);

	if ( ii < 0 ) return 0;
	return 1;
}

/************************************************************/
int GetNameOfPartToLoad(WinEDA_DrawFrame * frame,
			LibraryStruct *Library, wxString & BufName)
/************************************************************/
/*
	Routine de selection du nom d'un composant en librairie pour chargement,
	dans la librairie Library.
	Si Library == NULL, il y aura demande de selection d'une librairie
 Retourne
	1 si composant selectionne
	0 si commande annulee
	place le nom du composant a charger, selectionne a partir d'une liste dans
	BufName
*/
{
int ii;
static wxString OldCmpName;

	ii = DisplayComponentsNamesInLib(frame, Library, BufName, OldCmpName);
	if( ii <= 0 ) return 0;
	OldCmpName = BufName;
	return( 1 );
}

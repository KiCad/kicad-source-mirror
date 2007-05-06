	/**************************************************************/
	/*					libarch.cc								  */
	/* Module de generation du fichier d'archivage des composants */
	/**************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"

#include "protos.h"

/* fonctions exportees */

/* fonctions locales */
static int TriListEntry(EDA_LibComponentStruct **Objet1,
							EDA_LibComponentStruct **Objet2);

/* Variable locales */


/*******************************************************************/
bool LibArchive(wxWindow * frame, const wxString & ArchFullFileName)
/*******************************************************************/
/*
Creation du fichier librairie contenant tous les composants utilisés dans
le projet en cours
retourne  TRUE si fichier créé
*/
{
wxString DocFileName, msg;
char Line[256];
FILE *ArchiveFile, *DocFile;
EDA_BaseStruct ** ListStruct;
EDA_LibComponentStruct ** ListEntry, *Entry;
int ii, NbItems;
const wxChar * Text;


	/* Creation de la liste des elements */
	NbItems = GenListeCmp(NULL );	// Comptage des composants
	if ( NbItems == 0 ) return FALSE;

	ListStruct = (EDA_BaseStruct **)
			MyZMalloc( NbItems * sizeof(EDA_BaseStruct **) );
	if (ListStruct == NULL ) return FALSE;

	/* Calcul de la liste des composants */
	GenListeCmp(ListStruct);

	/* Calcul de la liste des Entrees de librairie
		et Remplacement des alias par les composants "Root" */
	ListEntry = (EDA_LibComponentStruct ** )
				MyZMalloc( NbItems * sizeof(EDA_LibComponentStruct *) );
	if (ListEntry == NULL ) return FALSE;

	for ( ii = 0; ii < NbItems; ii++ )
	{
		Text = ( (EDA_SchComponentStruct*)ListStruct[ii])->m_ChipName.GetData();
		Entry = FindLibPart(Text, wxEmptyString, FIND_ROOT);
		ListEntry[ii] = Entry;	// = NULL si Composant non trouvé en librairie
	}

	MyFree(ListStruct);

	qsort( ListEntry, NbItems, sizeof(EDA_LibComponentStruct *),
			(int(*)(const void*, const void*))TriListEntry);

	/* mise a jour extension fichier doc associe */
	DocFileName = ArchFullFileName;
	ChangeFileNameExt(DocFileName, wxT(".bck"));

	if ((ArchiveFile = wxFopen(ArchFullFileName, wxT("wt"))) == NULL)
	{
		MyFree(ListEntry);
		msg = _("Failed to create archive lib file ") + ArchFullFileName;
		DisplayError(frame, msg);
		return FALSE;
	}

	if ((DocFile = wxFopen(DocFileName, wxT("wt"))) == NULL)
	{
		msg = _("Failed to create doc lib file ") + DocFileName;
		DisplayError(frame, msg);
	}

	fprintf(ArchiveFile,"%s  %s\n#\n", LIBFILE_IDENT,DateAndTime(Line));
	if( DocFile)
		fprintf(DocFile,"%s  %s\n", DOCFILE_IDENT, DateAndTime(Line));

	/* Generation des elements */
	for ( ii = 0; ii < NbItems; ii++ )
	{
		if ( ListEntry[ii] == NULL )	// Composant non trouvé en librairie
		{
		continue;
			}
		if ( ii == 0 )
		{
			WriteOneLibEntry(frame, ArchiveFile, ListEntry[ii]);
			if( DocFile ) WriteOneDocLibEntry(DocFile, ListEntry[ii]);
		}
		else if ( ListEntry[ii-1] != ListEntry[ii] )
		{
			 WriteOneLibEntry(frame, ArchiveFile, ListEntry[ii]);
			 if( DocFile ) WriteOneDocLibEntry(DocFile, ListEntry[ii]);
		}
	}

	/* Generation fin de fichier */
	fprintf(ArchiveFile,"#\n#EndLibrary\n");
	fclose(ArchiveFile);

	if( DocFile )
	{
		fprintf(DocFile,"#\n#End Doc Library\n");
		fclose(DocFile);
	}

	MyFree(ListEntry);

	return TRUE;
}

/***********************************************************/
static int TriListEntry(EDA_LibComponentStruct **Objet1,
							EDA_LibComponentStruct **Objet2)
/***********************************************************/
/* Routine de comparaison pour le tri du Tableau par qsort()
	Les composants sont tries par LibName
*/
{
int ii;
const wxString * Text1, *Text2;

	if( (*Objet1 == NULL) && (*Objet2 == NULL ) ) return(0);
	if( *Objet1 == NULL) return(-1);
	if( *Objet2 == NULL) return(1);

	Text1 = &(*Objet1)->m_Name.m_Text;
	Text2 = &(*Objet2)->m_Name.m_Text;

	ii = Text1->CmpNoCase(* Text2);
	return(ii);
}



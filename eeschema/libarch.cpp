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

/* Imported functions */
int BuildComponentsListFromSchematic( ListComponent* List );

/* Local functions*/
static int TriListEntry(EDA_LibComponentStruct **Objet1, EDA_LibComponentStruct **Objet2);



/*******************************************************************/
bool LibArchive(wxWindow * frame, const wxString & ArchFullFileName)
/*******************************************************************/
/*
Creation du fichier librairie contenant tous les composants utilis�s dans
le projet en cours
retourne  TRUE si fichier cr��
*/
{
wxString DocFileName, msg;
char Line[256];
FILE *ArchiveFile, *DocFile;
ListComponent * List;
EDA_LibComponentStruct ** ListEntry, *Entry;
int ii, NbItems;
const wxChar * Text;


	/* Creation de la liste des elements */
	NbItems = BuildComponentsListFromSchematic(NULL );	// Comptage des composants
	if ( NbItems == 0 ) return FALSE;

	List = (ListComponent *) MyZMalloc( NbItems * sizeof( ListComponent ) );
	if (List == NULL ) return FALSE;

	/* Calcul de la liste des composants */
	BuildComponentsListFromSchematic(List);

	/* Calcul de la liste des Entrees de librairie
		et Remplacement des alias par les composants "Root" */
	ListEntry = (EDA_LibComponentStruct ** )
				MyZMalloc( NbItems * sizeof(EDA_LibComponentStruct *) );
	if (ListEntry == NULL ) return FALSE;

	for ( ii = 0; ii < NbItems; ii++ )
	{
		Text = List[ii].m_Comp->m_ChipName.GetData();
		Entry = FindLibPart(Text, wxEmptyString, FIND_ROOT);
		ListEntry[ii] = Entry;	// = NULL si Composant non trouv� en librairie
	}

	MyFree(List);

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
		if ( ListEntry[ii] == NULL )	// Composant non trouv� en librairie
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



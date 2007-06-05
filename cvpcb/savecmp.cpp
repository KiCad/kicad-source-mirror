	/**************/
	/* savecmp()  */
	/**************/

/* sauvegarde la liste des associations composants/empreintes */

#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "cvpcb.h"
#include "protos.h"

/* Chaines de caractere d'identification */
char EnteteCmpMod[] = {"Cmp-Mod V01"};

/****************************************************************************/
int WinEDA_CvpcbFrame::SaveComponentList(const wxString & NetlistFullFileName)
/*****************************************************************************/
/* Routine de sauvegarde du fichier des modules
	Retourne 1 si OK
			0 si ecriture non faite
*/
{
STORECMP * Cmp;
wxString FullFileName;
char Line[1024];
wxString Title = g_Main_Title + wxT(" ") + GetBuildVersion();

	/* calcul du nom du fichier */
	FullFileName = NetlistFullFileName;
	ChangeFileNameExt(FullFileName, g_ExtCmpBuffer);

	dest = wxFopen(FullFileName, wxT("wt") );
	if( dest == NULL ) return(0);	/* Erreur ecriture */

	fprintf(dest,"%s", EnteteCmpMod);
	fprintf(dest," Created by %s", CONV_TO_UTF8(Title));
	fprintf(dest," date = %s\n", DateAndTime(Line));

	for ( Cmp = g_BaseListeCmp ; Cmp != NULL ; Cmp = Cmp->Pnext )
		{
		fprintf(dest,"\nBeginCmp\n");
		fprintf(dest,"TimeStamp = %s;\n", CONV_TO_UTF8(Cmp->m_TimeStamp));
		fprintf(dest,"Reference = %s;\n", CONV_TO_UTF8(Cmp->m_Reference));
		fprintf(dest,"ValeurCmp = %s;\n", CONV_TO_UTF8(Cmp->m_Valeur));
		fprintf(dest,"IdModule  = %s;\n", CONV_TO_UTF8(Cmp->m_Module));
		fprintf(dest,"EndCmp\n");
		}
	fprintf(dest,"\nEndListe\n") ;
	fclose(dest) ;

	return(1) ;
}

/****************/
int loadcmp(void)
/***************/
/* recupere la liste des associations composants/empreintes
*/
{
wxString timestamp, valeur, ilib, namecmp;
bool read_cmp_data = FALSE, eof = FALSE;
STORECMP * Cmp;
char Line[1024], * ident, *data;
wxString FullFileName;

	/* calcul du nom du fichier */
	FullFileName = FFileName;
	ChangeFileNameExt(FullFileName, g_ExtCmpBuffer);

	source = wxFopen(FullFileName, wxT("rt") );
	if (source == NULL )
	{
		return(0) ;
	}

	/* Identification du Type de fichier CmpMod */
	if ( fgets(Line,79,source) == 0 ) return(0);
	if( strnicmp(Line, EnteteCmpMod, 11 ) != 0 ) /* old file version*/
	{
		fclose(source) ;
		DisplayError( NULL, wxT("Old version of Componaent file, recreate it!"));
		return(0) ;
	}
	
	/* lecture de la liste */
	while( ! eof && fgets(Line,79,source) != 0 )
	{
		if( strnicmp(Line, "EndListe", 8 ) == 0 ) break;

		/* Recherche du debut de description du composant */
		if( strnicmp(Line, "BeginCmp", 8 ) != 0 ) continue;
		timestamp.Empty();
		valeur.Empty();
		ilib.Empty();
		namecmp.Empty();
		read_cmp_data = TRUE;

		while( ! eof && read_cmp_data )
		{
			if( fgets(Line, 1024,source) == 0 )
			{
				eof = TRUE; break;
			}
			
			if( strnicmp(Line, "EndCmp", 6 ) == 0 )
			{
				read_cmp_data = TRUE; break;
			}

			ident = strtok ( Line,"=;\n\r");
			data = strtok ( NULL,";\n\r");
			if( strnicmp(ident, "TimeStamp", 9) == 0)
			{
				timestamp = CONV_FROM_UTF8(data);
				timestamp.Trim(TRUE);
				timestamp.Trim(FALSE);
				continue;
			}

			if( strnicmp(ident, "Reference", 9) == 0)
			{
				namecmp = CONV_FROM_UTF8(data);
				namecmp.Trim(TRUE);
				namecmp.Trim(FALSE);
				continue;
			}

			if( strnicmp(ident, "ValeurCmp", 9) == 0)
			{
				valeur = CONV_FROM_UTF8(data);
				valeur.Trim(TRUE);
				valeur.Trim(FALSE);
				continue;
			}

			if( strnicmp(ident, "IdModule", 8) == 0)
			{
				ilib = CONV_FROM_UTF8(data);
				ilib.Trim(TRUE);
				ilib.Trim(FALSE);
				continue;
			}
		} /* Fin lecture description de 1 composant */

		/* Recherche du composant correspondant en netliste et
			 mise a jour de ses parametres */
		for ( Cmp = g_BaseListeCmp ; Cmp != NULL ; Cmp = Cmp->Pnext )
		{
			if (selection_type == 1 )
			{
				if( timestamp != Cmp->m_TimeStamp )
					continue ;
			}
			else
				if( namecmp != Cmp->m_Reference ) continue;

			/* composant identifie , copie du nom du module correspondant */
			Cmp->m_Module= ilib;
		}
	}
	fclose(source) ;
	return(1) ;
}



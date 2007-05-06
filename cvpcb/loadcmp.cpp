		/***************************************************/
		/* Localisation des elements pointes par la souris */
		/***************************************************/

#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "cvpcb.h"
#include "protos.h"



/*****************************************************************/
MODULE * WinEDA_DisplayFrame::Get_Module(const wxString & CmpName)
/*****************************************************************/
/*
	Analyse les LIBRAIRIES pour trouver le module demande
	Si ce module est trouve, le copie en memoire, et le
	chaine en fin de liste des modules
		- Entree:
			name_cmp = nom du module
		- Retour:
			Pointeur sur le nouveau module.
*/
{
int LineNum, Found = 0;
unsigned ii;
char Line[1024], Name[255];
wxString libname;
MODULE * Module = NULL;

	for( ii= 0 ; ii < g_LibName_List.GetCount(); ii++)
	{
		/* Calcul du nom complet de la librairie */
		libname = MakeFileName(g_RealLibDirBuffer, g_LibName_List[ii], LibExtBuffer);

		if ((lib_module = wxFopen(libname, wxT("rt")))  == NULL ) 
		{
			continue ;
		}

		/* lecture entete chaine definie par ENTETE_LIBRAIRIE */
		LineNum = 0;
		GetLine(lib_module, Line, &LineNum) ;
		StrPurge(Line);
		if(strnicmp( Line,ENTETE_LIBRAIRIE, L_ENTETE_LIB) != 0)
		{
			DisplayError(this, _("This file is NOT a library file") );
			return(NULL);
		}

		/* Lecture de la liste des composants de la librairie */
		Found = 0;
		while( !Found && GetLine(lib_module,Line, &LineNum) )
		{
			if( strncmp( Line, "$MODULE",6) == 0 ) break;
			if( strnicmp( Line,"$INDEX",6) == 0 )
			{
				while( GetLine(lib_module,Line, &LineNum) )
				{
					if( strnicmp( Line,"$EndINDEX",9) == 0 ) break;
					StrPurge(Line);
					if( stricmp(Line, CONV_TO_UTF8(CmpName)) == 0 )
					{
						Found = 1; break; /* Trouve! */
					}
				}
			}
		}

		/* Lecture de la librairie */
		while( Found && GetLine(lib_module,Line, &LineNum) )
		{
			if( Line[0] != '$' ) continue;
			if( Line[1] != 'M' ) continue;
			if( strnicmp( Line, "$MODULE",7) != 0 ) continue;
			/* Lecture du nom du composant */
			sscanf(Line+7," %s",Name);
			if( stricmp(Name,CONV_TO_UTF8(CmpName)) == 0 )  /* composant localise */
			{
				Module = new MODULE(m_Pcb);
				Module->ReadDescr(lib_module, &LineNum);
				Module->SetPosition(wxPoint(0,0) );
				fclose(lib_module);
				return(Module) ;
			}
		}
		fclose(lib_module) ; lib_module = 0;
	}

	if( lib_module ) fclose(lib_module) ;

	wxString msg;
	msg.Printf(_("Module %s not found"),CmpName.GetData());
	DisplayError(this, msg);
	return(NULL) ;
}



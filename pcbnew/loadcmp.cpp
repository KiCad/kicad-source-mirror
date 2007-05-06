		/**********************************************/
		/* Footprints selection and loading functions */
		/**********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

class ModList
{
public:
	ModList * Next;
	wxString m_Name, m_Doc, m_KeyWord;

public:
	ModList(void)
	{
		Next = NULL;
	}
	~ModList(void)
	{
	}
};

/* Fonctions locales */
static void DisplayCmpDoc(wxString & Name);
static void ReadDocLib(const wxString & ModLibName );
/*****/

/* variables locales */
static ModList * MList;


/***************************************************************************/
void WinEDA_ModuleEditFrame::Load_Module_Module_From_BOARD( MODULE * Module )
/***************************************************************************/
{
MODULE * NewModule;

	if ( Module == NULL )
		{
		if (m_Parent->m_PcbFrame == NULL) return;
		if (m_Parent->m_PcbFrame->m_Pcb == NULL) return;
		if (m_Parent->m_PcbFrame->m_Pcb->m_Modules == NULL) return;

		Module = Select_1_Module_From_BOARD(m_Parent->m_PcbFrame->m_Pcb);
		}

	if ( Module == NULL ) return;

	m_CurrentScreen->m_CurrentItem = NULL;

	Clear_Pcb(NULL, TRUE);

	m_Pcb->m_Status_Pcb = 0;
	NewModule = new MODULE(m_Pcb);
	NewModule->Copy(Module);
	NewModule->m_Link = Module->m_TimeStamp;

	Module = NewModule;
	Module->m_Parent = m_Pcb;
	Module->Pback = m_Pcb->m_Modules; Module->Pnext = NULL;
	m_Pcb->m_Modules = Module;

	Module->m_Flags = 0;

	build_liste_pads();

	m_CurrentScreen->m_Curseur.x = m_CurrentScreen->m_Curseur.y = 0;
	Place_Module(Module, NULL);
	if( Module->m_Layer != CMP_N) Change_Side_Module(Module, NULL);
	Rotate_Module(NULL, Module, 0, FALSE);
	m_CurrentScreen->ClrModify();
	Zoom_Automatique(TRUE);
}

/****************************************************************************/
MODULE * WinEDA_BasePcbFrame::Load_Module_From_Library(const wxString & library,
			wxDC * DC)
/****************************************************************************/
/* Permet de charger un module directement a partir de la librairie */
{
MODULE * module;
wxPoint curspos = m_CurrentScreen->m_Curseur;
wxString ModuleName, keys;
static wxArrayString HistoryList;
bool AllowWildSeach = TRUE;

	/* Ask for a component name or key words */
	ModuleName = GetComponentName(this, HistoryList, _("Module name:"), NULL);
	ModuleName.MakeUpper();
	if( ModuleName.IsEmpty() )	/* Cancel command */
	{
		DrawPanel->MouseToCursorSchema();
		return NULL;
	}


	if( ModuleName[0] == '=' )	// Selection by keywords
	{
		AllowWildSeach = FALSE;
		keys = ModuleName.AfterFirst('=');
		ModuleName = Select_1_Module_From_List(this, library, wxEmptyString, keys);
		if( ModuleName.IsEmpty() )	/* Cancel command */
		{
			DrawPanel->MouseToCursorSchema();
			return NULL;
		}
	}

	else if( (ModuleName.Contains(wxT("?"))) || (ModuleName.Contains(wxT("*"))) ) // Selection wild card
	{
		AllowWildSeach = FALSE;
		ModuleName = Select_1_Module_From_List(this, library, ModuleName, wxEmptyString);
		if( ModuleName.IsEmpty() )
		{
			DrawPanel->MouseToCursorSchema();
			return NULL;	/* annulation de commande */
		}
	}

	module = Get_Librairie_Module(this, library, ModuleName, FALSE);

	if( (module == NULL) && AllowWildSeach )	/* Attemp to search with wildcard */
	{
		AllowWildSeach = FALSE;
		wxString wildname = wxChar('*') + ModuleName + wxChar('*');
		ModuleName = wildname;
		ModuleName = Select_1_Module_From_List(this, library, ModuleName, wxEmptyString);
		if( ModuleName.IsEmpty() )
		{
			DrawPanel->MouseToCursorSchema();
			return NULL;	/* annulation de commande */
		}
		else module = Get_Librairie_Module(this, library, ModuleName, TRUE);
	}

	m_CurrentScreen->m_Curseur = curspos;
	DrawPanel->MouseToCursorSchema();

	if( module )
	{
		AddHistoryComponentName(HistoryList, ModuleName);

		module->m_Flags = IS_NEW;
		module->m_Link = 0;
		module->m_TimeStamp = GetTimeStamp();
		m_Pcb->m_Status_Pcb = 0 ;
		module->SetPosition(curspos);
		build_liste_pads();

		module->Draw(DrawPanel, DC, wxPoint(0,0), GR_OR);
	}

	return module;
}

/*******************************************************************************/
MODULE * WinEDA_BasePcbFrame::Get_Librairie_Module(wxWindow * winaff,
		const wxString & library, const wxString & ModuleName, bool show_msg_err)
/*******************************************************************************/
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
int LineNum, Found= 0;
wxString fulllibname;
char Line[512];
wxString Name;
wxString ComponentName, msg;
MODULE * Module;
MODULE * NewModule;
FILE * lib_module = NULL;
unsigned ii;

	ComponentName = ModuleName;

	/* Calcul de l'adresse du dernier module: */
	Module = m_Pcb->m_Modules;
	if( Module ) while( Module->Pnext ) Module = (MODULE*) Module->Pnext;

	for( ii = 0; ii < g_LibName_List.GetCount(); ii++)
	{
		fulllibname = g_LibName_List[ii];

		/* Calcul du nom complet de la librairie */
		fulllibname = MakeFileName(g_RealLibDirBuffer,fulllibname,LibExtBuffer);

		if ((lib_module = wxFopen(fulllibname, wxT("rt")))  == NULL )
		{
			msg.Printf(_("Library <%s> not found"),fulllibname.GetData());
			Affiche_Message(msg);
			continue ;
		}

		msg.Printf(_("Scan Lib: %s"),fulllibname.GetData());
		Affiche_Message(msg);

		/* lecture entete chaine definie par ENTETE_LIBRAIRIE */
		LineNum = 0;
		GetLine(lib_module, Line, &LineNum) ;
		StrPurge(Line);
		if(strnicmp( Line,ENTETE_LIBRAIRIE, L_ENTETE_LIB) != 0)
		{
			DisplayError(winaff, _("File is Not a library") );
			return(NULL);
		}

		/* Lecture de la liste des composants de la librairie */
		Found = 0;
		while( !Found && GetLine(lib_module,Line, &LineNum) )
		{
			if( strnicmp( Line, "$MODULE",6) == 0 ) break;
			if( strnicmp( Line,"$INDEX",6) == 0 )
			{
				while( GetLine(lib_module,Line, &LineNum) )
				{
					if( strnicmp( Line,"$EndINDEX",9) == 0 ) break;
					StrPurge(Line);
					msg = CONV_FROM_UTF8(Line);
					if( msg.CmpNoCase(ComponentName) == 0 )
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
			Name = CONV_FROM_UTF8(Line+8);
			if( Name.CmpNoCase(ComponentName) == 0 )  /* composant localise */
			{
				NewModule = new MODULE(m_Pcb);
				NewModule->ReadDescr(lib_module, &LineNum);
				if( Module == NULL )	/* 1er Module */
				{
					m_Pcb->m_Modules = NewModule;
					NewModule->Pback = m_Pcb;
				}

				else
				{
					Module->Pnext = NewModule;
					NewModule->Pback = Module;
				}
				fclose(lib_module) ;
				Affiche_Message(wxEmptyString) ;
				return(NewModule) ;
			}
		}
		fclose(lib_module) ; lib_module = 0;
	}

	if( lib_module ) fclose(lib_module) ;

	if ( show_msg_err )
	{
		msg.Printf(_("Module <%s> not found"),ComponentName.GetData());
		DisplayError(winaff, msg);
	}
	return(NULL) ;
}

/***************************************************************/
wxString WinEDA_BasePcbFrame::Select_1_Module_From_List(
				wxWindow * active_window,
				const wxString & Library,
				const wxString & Mask, const wxString & KeyWord)
/***************************************************************/
/*
 Affiche la liste des modules des librairies
	Recherche dans la librairie Library ou generale si Library == NULL
	Mask = Filtre d'affichage( Mask = wxEmptyString pour listage non filtré )
	KeyWord = Liste de mots cles, Recherche limitee aux composants
		ayant ces mots cles ( KeyWord = wxEmptyString pour listage de tous les modules )

	retourne wxEmptyString si abort ou probleme
	ou le nom du module
*/
{
int LineNum;
unsigned ii, NbModules;
char Line[1024];
wxString FullLibName;
static wxString OldName;	/* Memorise le nom du dernier composant charge */
wxString CmpName;
FILE * lib_module;
wxString msg;
	
WinEDAListBox * ListBox = new WinEDAListBox(active_window, wxEmptyString,
			NULL, OldName, DisplayCmpDoc, wxColour(200, 200, 255) );

	wxBeginBusyCursor();

	/* Recherche des composants en librairies */
	NbModules = 0;
	for( ii = 0; ii < g_LibName_List.GetCount(); ii++)
	{
		/* Calcul du nom complet de la librairie */
		if( Library.IsEmpty() )
		{
			FullLibName = MakeFileName(g_RealLibDirBuffer,
						g_LibName_List[ii], LibExtBuffer);
		}
		else
			FullLibName = MakeFileName(g_RealLibDirBuffer,Library,LibExtBuffer);

		ReadDocLib(FullLibName );

		if( ! KeyWord.IsEmpty())	/* Inutile de lire la librairie si selection
						par mots cles, deja lus */
		{
			if( ! Library.IsEmpty() ) break;
			continue ;
		}

		if ((lib_module = wxFopen(FullLibName, wxT("rt")))  == NULL )
		{
			if( ! Library.IsEmpty() ) break;
			continue ;
		}

		msg = _("Library: "); msg << FullLibName;
		Affiche_Message(msg);

		/* lecture entete */
		LineNum = 0;
		GetLine(lib_module, Line, &LineNum, sizeof(Line) -1);

		if(strnicmp( Line,ENTETE_LIBRAIRIE, L_ENTETE_LIB) != 0)
		{
			DisplayError(this, wxT("This file is not an Eeschema libray file"), 20); continue;
		}

		/* Lecture de la librairie */
		while( GetLine(lib_module,Line, &LineNum, sizeof(Line) -1) )
		{
			if( Line[0] != '$' ) continue;
			if( strnicmp( Line, "$MODULE",6) == 0 ) break;
			if( strnicmp( Line,"$INDEX",6) == 0 )
			{
				while( GetLine(lib_module,Line, &LineNum) )
				{
					if( strnicmp( Line,"$EndINDEX",9) == 0 ) break;
                    strupper(Line);
					msg = CONV_FROM_UTF8(StrPurge(Line));
					if ( Mask.IsEmpty() )
					{
						ListBox->Append( msg );
						NbModules++;
					}
					else if ( WildCompareString(Mask, msg, FALSE) )
					{
						ListBox->Append( msg );
						NbModules++;
					}
				}
			} /* Fin Lecture INDEX */
		}  /* Fin lecture 1 Librairie */

		fclose(lib_module) ; lib_module = NULL;
		if( ! Library.IsEmpty() ) break;
	}

	/*  creation de la liste des modules si recherche par mots-cles */
	if( ! KeyWord.IsEmpty() )
	{
		ModList * ItemMod = MList;
		while( ItemMod != NULL )
		{
			if( KeyWordOk(KeyWord, ItemMod->m_KeyWord) )
			{
				NbModules++;
				ListBox->Append( ItemMod->m_Name );
			}
			ItemMod = ItemMod->Next;
		}
	}

	wxEndBusyCursor();

	msg.Printf( _("Modules (%d items)"), NbModules);
	ListBox->SetTitle(msg);
	ListBox->SortList();
	
	ii = ListBox->ShowModal();
	if ( ii >= 0 ) CmpName = ListBox->GetTextSelection();
	else CmpName.Empty();

	ListBox->Destroy();

	/* liberation mem de la liste des textes doc module */
	while( MList != NULL )
	{
		ModList * NewMod = MList->Next;
		delete MList;
		MList = NewMod;
	}

	if( CmpName != wxEmptyString ) OldName = CmpName;

	return(CmpName);
}

/******************************************/
static void DisplayCmpDoc(wxString &  Name)
/*******************************************/
/* Routine de recherche et d'affichage de la doc du composant Name
	La liste des doc est pointee par MList
*/
{
ModList * Mod = MList;

	if ( ! Mod )
	{
		Name.Empty(); return;
	}

	/* Recherche de la description */
	while ( Mod )
	{
		if( ! Mod->m_Name.IsEmpty() && (Mod->m_Name.CmpNoCase(Name) == 0) ) break;
		Mod = Mod->Next;
	}

	if ( Mod )
	{
		Name = ! Mod->m_Doc.IsEmpty() ? Mod->m_Doc  : wxT("No Doc");
		Name += wxT("\nKeyW: ");
		Name += ! Mod->m_KeyWord.IsEmpty() ? Mod->m_KeyWord : wxT("No Keyword");
	}

	else Name = wxEmptyString;
}

/***************************************************/
static void ReadDocLib(const wxString & ModLibName )
/***************************************************/
/* Routine de lecture du fichier Doc associe a la librairie ModLibName.
  	Cree en memoire la chaine liste des docs pointee par MList
	ModLibName = full file Name de la librairie Modules
*/
{
ModList * NewMod;
char Line[1024];
FILE * LibDoc;
wxString FullModLibName = ModLibName;

	ChangeFileNameExt(FullModLibName, EXT_DOC);

	if( (LibDoc = wxFopen(FullModLibName, wxT("rt"))) == NULL ) return;

	GetLine(LibDoc, Line, NULL, sizeof(Line) -1);
	if(strnicmp( Line,ENTETE_LIBDOC, L_ENTETE_LIB) != 0) return;

	/* Lecture de la librairie */
	while( GetLine(LibDoc,Line, NULL, sizeof(Line) -1) )
		{
		if( Line[0] != '$' ) continue;
		if( Line[1] == 'E' ) break;;
		if( Line[1] == 'M' )	/* Debut decription 1 module */
			{
			NewMod = new ModList();
			NewMod->Next = MList;
			MList = NewMod;
			while( GetLine(LibDoc,Line, NULL, sizeof(Line) -1) )
			{
				if( Line[0] ==  '$' )	/* $EndMODULE */
						break;
				switch( Line[0] )
				{
					case 'L':	/* LibName */
						NewMod->m_Name = CONV_FROM_UTF8(StrPurge(Line+3) );
						break;

					case 'K':	/* KeyWords */
						NewMod->m_KeyWord = CONV_FROM_UTF8(StrPurge(Line+3) );
						break;

					case 'C':	/* Doc */
						NewMod->m_Doc = CONV_FROM_UTF8(StrPurge(Line+3) );
						break;
				}
			}
		} /* lecture 1 descr module */
	}	/* Fin lecture librairie */
	fclose(LibDoc);
}

/********************************************************************/
MODULE * WinEDA_BasePcbFrame::Select_1_Module_From_BOARD(BOARD * Pcb)
/********************************************************************/
/* Affiche la liste des modules du PCB en cours
	Retourne un pointeur si module selectionne
	retourne NULL sinon
*/
{
int ii;
MODULE * Module;
static wxString OldName;	/* Memorise le nom du dernier composant charge */
wxString CmpName, msg;

WinEDAListBox * ListBox = new WinEDAListBox(this, wxEmptyString,
			NULL, wxEmptyString, NULL, wxColour(200, 200, 255) );

	/* Recherche des composants en BOARD */
	ii = 0;
	Module = Pcb->m_Modules;
	for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
	{
		ii++;
		ListBox->Append( Module->m_Reference->m_Text );
	}

	msg.Printf( _("Modules (%d items)"), ii);
	ListBox->SetTitle(msg);

	ListBox->SortList();

	ii = ListBox->ShowModal();
	if ( ii >= 0 ) CmpName = ListBox->GetTextSelection();
	else CmpName.Empty();

	ListBox->Destroy();

	if( CmpName == wxEmptyString ) return NULL;

	OldName = CmpName;

	// Recherche du pointeur sur le module
	Module = Pcb->m_Modules;
	for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
	{
		if ( CmpName.CmpNoCase(Module->m_Reference->m_Text) == 0 )
			break;
	}
	return Module;
}



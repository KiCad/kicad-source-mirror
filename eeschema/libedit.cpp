	/****************************/
	/*	EESchema - libedit.cpp	*/
	/****************************/

/* Routines de maintenanace des librairies:
	sauvegarde, modification de librairies.
	creation edition suppression de composants
*/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

#include "id.h"
#include "dialog_create_component.h"

/* Routines locales */

/* Variables locales */


/**********************************************/
void WinEDA_LibeditFrame::DisplayLibInfos()
/**********************************************/
/* Affiche dans la zone messages la librairie , et le composant edite */
{
wxString msg = wxT("Libedit: ");

	msg += CurrentLib ? CurrentLib->m_FullFileName : wxT("No Lib");
	SetTitle(msg);

	msg = _(" Part:   ");
	if ( CurrentLibEntry == NULL )
		{
		msg += _("None");
		}
	else
		{
			msg += CurrentLibEntry->m_Name.m_Text;
		if ( !CurrentAliasName.IsEmpty() )
			msg << wxT("  Alias ") << CurrentAliasName;
		}
wxChar UnitLetter[] = wxT("?ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	msg << wxT("   Unit ") << UnitLetter[CurrentUnit];

	if ( CurrentConvert > 1 ) msg += _("   Convert");
	else msg += _("   Normal");

	if ( CurrentLibEntry && (CurrentLibEntry->m_Options == ENTRY_POWER) )
		msg += _("  (Power Symbol)");

	SetStatusText(msg, 0);
}


/**************************************************/
void WinEDA_LibeditFrame::SelectActiveLibrary()
/**************************************************/
/* Routine to Select Current library
*/
{
LibraryStruct *Lib;

	Lib = SelectLibraryFromList(this);
	if( Lib )
		{
		CurrentLib = Lib;
		}
	DisplayLibInfos();
}

/*************************************************/
/* Routine to Load one selected library content. */
/*************************************************/
bool WinEDA_LibeditFrame::LoadOneLibraryPart()
{
int i;
wxString msg;
wxString CmpName;
EDA_LibComponentStruct *LibEntry = NULL;

	if( ScreenLib->IsModify() )
	{
		if( ! IsOK(this, _("Current Part not saved.\nContinue?") ) ) return FALSE;
	}

	if(CurrentLib == NULL) SelectActiveLibrary();
	if(CurrentLib == NULL) return FALSE;

	i = GetNameOfPartToLoad(this, CurrentLib, CmpName);
	if( i == 0) return FALSE;

	ScreenLib->ClrModify();
	CurrentDrawItem = NULL;
	// Effacement ancien composant affiché
	if( CurrentLibEntry)
	{
		delete CurrentLibEntry;
		CurrentLibEntry = NULL;
	}

	/* Chargement du composant */
	LibEntry = FindLibPart(CmpName.GetData(),CurrentLib->m_Name, FIND_ALIAS);

	if( LibEntry == NULL)
	{
		msg = _("Component \""); msg << CmpName << _("\" not found.");
		DisplayError(this, msg, 20);
		return FALSE;
	}

	LoadOneLibraryPartAux(LibEntry, CurrentLib);
	ReCreateHToolbar();
	Zoom_Automatique(FALSE);
	ReDrawPanel();
	return TRUE;
}

/**************************************************************************/
int WinEDA_LibeditFrame::LoadOneLibraryPartAux(EDA_LibComponentStruct *LibEntry,
								 LibraryStruct *Library, int noMsg)
/**************************************************************************/
/* Routine Pour Charger en memoire la copie de 1 libpart.
retourne
	0 si OK
	1 si err
	CurrentLibEntry pointe la copie ainsi creee
*/
{
wxString msg;
const wxChar * CmpName, *RootName = NULL;

	if( (LibEntry == NULL) || (Library == NULL) ) return(1);

	CmpName = LibEntry->m_Name.m_Text.GetData();
	CurrentAliasName.Empty();
	if( LibEntry->Type != ROOT)
		{
		RootName = ((EDA_LibCmpAliasStruct*)LibEntry)->m_RootName.GetData() ;
		if( !noMsg )
			{
			msg.Printf( wxT("\"<%s>\" is Alias of \"<%s>\""), CmpName, RootName);
			}

		LibEntry = FindLibPart(RootName,Library->m_Name,FIND_ROOT);

		if( LibEntry == NULL)
			{
			msg.Printf( wxT("Root Part \"<%s>\" not found."), RootName);
			DisplayError(this, msg, 20);
			return(1);
			}
		CurrentAliasName = CmpName;
		}

	if( CurrentLibEntry) delete CurrentLibEntry;

	CurrentLibEntry = CopyLibEntryStruct(this, LibEntry);
	CurrentUnit = 1; CurrentConvert = 1;
	DisplayLibInfos();

	BuildAliasData(Library, CurrentLibEntry);

	ScreenLib->ClrModify();
	g_AsDeMorgan = 0;

	if( LookForConvertPart(CurrentLibEntry) > 1 ) g_AsDeMorgan = 1;

	return(0);
}

/*********************************************************************/
void WinEDA_LibeditFrame::RedrawActiveWindow(wxDC * DC, bool EraseBg)
/*********************************************************************/
/* Routine generale d'affichage a l'ecran du "PartLib" en cours d'edition */
{
	if( m_CurrentScreen == NULL ) return;

	ActiveScreen = GetScreen();

	DC->SetBackground(*wxBLACK_BRUSH );
	DC->SetBackgroundMode(wxTRANSPARENT);
	GRResetPenAndBrush(DC);	// reinit de la brosse et plume courante

	DrawPanel->CursorOff(DC); // effacement curseur
	if(DrawPanel->ManageCurseur)
	{
		DrawPanel->ManageCurseur(DrawPanel, DC, FALSE); // effacement affichage lie au curseur
	}

	if (EraseBg ) DrawPanel->EraseScreen(DC);

	DrawPanel->DrawBackGround(DC);

	if( CurrentLibEntry)
		DrawLibEntry(DrawPanel, DC, CurrentLibEntry, 0, 0,
					CurrentUnit, CurrentConvert, GR_DEFAULT_DRAWMODE);

	DrawPanel->CursorOn(DC); // reaffichage curseur

	if(DrawPanel->ManageCurseur)
	{
		DrawPanel->ManageCurseur(DrawPanel, DC, FALSE); // reaffichage lie au curseur
	}

	m_CurrentScreen->ClrRefreshReq();
	DisplayLibInfos();
	Affiche_Status_Box();
}


/*************************************************/
void WinEDA_LibeditFrame::SaveActiveLibrary()
/*************************************************/
/* Sauvegarde en fichier la librairie pointee par CurrentLib
	une sauvegarde en .bak de l'ancien fichier est egalement cree
*/
{
wxString Name, msg;
int err;

	if(CurrentLib == NULL)
	{
		DisplayError(this, wxT("No Library specified")); return;
	}

	Name = MakeFileName(g_RealLibDirBuffer, CurrentLib->m_Name, g_LibExtBuffer);

	msg = _("Modify Library File \"") + Name + _("\"?");
	if( ! IsOK(this, msg) ) return;

	err = SaveOneLibrary(this, Name, CurrentLib);

	MsgPanel->EraseMsgBox();

	if ( err )
	{
		msg = _("Error while saving Library File \"") + Name + _("\".");
		Affiche_1_Parametre(this, 1, wxT(" *** ERROR : **"), msg,BLUE);
		DisplayError(this, msg);
	}
	else
	{
		msg = _("Library File \"") + Name + wxT("\" Ok");
		ChangeFileNameExt(Name,DOC_EXT);
		wxString msg1 = _("Document File \"") + Name + wxT("\" Ok");
		Affiche_1_Parametre(this, 1,msg, msg1,BLUE);
	}
}


/**************************************************************/
void WinEDA_LibeditFrame::DisplayCmpDoc(const wxString & Name)
/**************************************************************/
/*
Affiche la documentation du composant selectionne
Utilisée lors de l'affichage de la liste des composants en librairie
*/
{
LibCmpEntry * CmpEntry;

	if( CurrentLib == NULL ) return;
	MsgPanel->EraseMsgBox();
	CmpEntry = FindLibPart(Name.GetData(), CurrentLib->m_Name, FIND_ALIAS);
	if ( CmpEntry == NULL ) return;

	AfficheDoc(this, CmpEntry->m_Doc,CmpEntry->m_KeyWord);
}

/*********************************************/
void WinEDA_LibeditFrame::DeleteOnePart()
/*********************************************/
/* Routine de suppression d'un composant dans la librairie courante
	(effacement en memoire uniquement, le fichier n'est pas modifie)
	Le composant peut etre un alias, ou la definition de base.
	Si c'est un alias:
		il est supprime, et la liste des alias de la definition
		 de base est modifiee
	Si c'est le composant de base:
		Si la liste des alias est nulle, il est supprime
		Sinon le premier alias devient le composant de base, et les autres
		alias deviennent dependants de celui ci.
*/
{
wxString CmpName;
int NumOfParts;
EDA_LibComponentStruct *LibEntry;
WinEDAListBox * ListBox;
const wxChar ** ListNames;
wxString msg;

	CurrentDrawItem = NULL;

	if(CurrentLib == NULL)
	{
		SelectActiveLibrary();
		if(CurrentLib == NULL)
		{
			DisplayError(this, _("No Active Library"), 20); return;
		}
	}

	NumOfParts = 0;
	PQCompFunc((PQCompFuncType) LibraryEntryCompare);
	LibEntry = (EDA_LibComponentStruct *) PQFirst(&CurrentLib->m_Entries, FALSE);
	while( LibEntry != NULL )
	{
		NumOfParts++;
		LibEntry = (EDA_LibComponentStruct *)
			PQNext(CurrentLib->m_Entries, LibEntry, NULL);
	}

	ListNames = (const wxChar**) MyZMalloc((NumOfParts+1) * sizeof(wxChar*));
	LibEntry = (EDA_LibComponentStruct *) PQFirst(&CurrentLib->m_Entries, FALSE);
	msg.Printf( _("Select Component (%d items)"), NumOfParts);
	NumOfParts = 0;
	while( LibEntry != NULL )
	{
		ListNames[NumOfParts] = LibEntry->m_Name.m_Text.GetData();
		NumOfParts++;
		LibEntry = (EDA_LibComponentStruct *)
			PQNext(CurrentLib->m_Entries, LibEntry, NULL);
	}

	ListBox = new WinEDAListBox(this, msg,
						ListNames, wxEmptyString, NULL /*DisplayCmpDoc*/,
						wxColour(255,255,200));


	int ii = ListBox->ShowModal(); ListBox->Destroy();

	if( ii >= 0)
	{
		CmpName = ListNames[ii];
		LibEntry = FindLibPart(CmpName.GetData(), CurrentLib->m_Name, FIND_ALIAS);

		if( LibEntry == NULL )
			DisplayError(this, _("Component not found"), 20);

		else
		{
			msg = _("Delete component \"") + LibEntry->m_Name.m_Text +
				_("\" from library \"") + CurrentLib->m_Name + wxT("\"?");
			if( IsOK(this, msg) )
			{
				DeletePartInLib( CurrentLib, LibEntry );
			}
		}
	}

	free (ListNames);
}


/****************************************************/
void WinEDA_LibeditFrame::CreateNewLibraryPart()
/****************************************************/
/* Routine to create a new library component
	If an old component is currently in edit, it is deleted.
*/	
{
wxString msg;
EDA_LibComponentStruct * NewStruct;
int diag;

	if( CurrentLibEntry )
		if( ! IsOK(this, _("Delete old component?")) ) return;

	CurrentDrawItem = NULL;

	WinEDA_CreateCmpDialog Dialogbox(this);
	diag = Dialogbox.ShowModal();
	if ( diag != wxID_OK ) return;
	msg = Dialogbox.ReturnCmpName();
	if ( msg.IsEmpty() ) return;
	msg.MakeUpper(); msg.Replace(wxT(" "), wxT("_") );

	/* Test: y a t-il un composant deja de ce nom */
	if(CurrentLib)
	{
		 if( FindLibPart(msg.GetData(), CurrentLib->m_Name, FIND_ALIAS) )
		{
			wxString msg;
			msg << _("Component \"") << Dialogbox.ReturnCmpName() <<
					_("\" exists in library \"") << CurrentLib->m_Name << _("\".");
			DisplayError(this, msg);
			return;
		}
	}

	NewStruct =  new EDA_LibComponentStruct( msg);
	Dialogbox.SetComponentData(*NewStruct);
	if ( NewStruct->m_Prefix.m_Text.IsEmpty())
		NewStruct->m_Prefix.m_Text = wxT("U");
	NewStruct->m_Prefix.m_Text.MakeUpper();

	// Effacement ancien composant affiché
	if( CurrentLibEntry) delete CurrentLibEntry;
	CurrentLibEntry = NewStruct;
	CurrentUnit = 1;
	CurrentConvert = 1;
	ReCreateHToolbar();

	DisplayLibInfos();
}




/*******************************************************************/
void WinEDA_LibeditFrame::DeletePartInLib( LibraryStruct * Library,
			EDA_LibComponentStruct *Entry)
/*******************************************************************/
/* Suppression du composant Entry en librairie Library.
	(effacement en memoire uniquement, le fichier n'est pas modifie)
	Le composant peut etre un alias, ou la definition de base.
	Si c'est un alias:
		il est supprime, et la liste des alias de la definition
		 de base est modifiee
	Si c'est le composant de base:
		Si la liste des alias est nulle, il est supprime
		Sinon le premier alias devient le composant de base, et les autres
		alias deviennent dependants de celui ci.
*/
{
EDA_LibComponentStruct *RootEntry;
EDA_LibCmpAliasStruct * AliasEntry;

	if ( (Library == NULL) || (Entry == NULL) ) return;

	PQCompFunc((PQCompFuncType) LibraryEntryCompare);
	Library->m_Modified = 1;

	if( Entry->Type == ALIAS )
	{
		RootEntry = FindLibPart( ((EDA_LibCmpAliasStruct*)Entry)->m_RootName.GetData(),
									Library->m_Name,FIND_ROOT);
		/* Remove alias name from the root component alias list */
		if( RootEntry == NULL )
		{
			DisplayError(this, wxT("Warning: for Alias, root not found"), 30);
		}
		else
		{
			int index = wxNOT_FOUND;
			if( RootEntry->m_AliasList.GetCount() != 0)
			{
				index = RootEntry->m_AliasList.Index(Entry->m_Name.m_Text.GetData(), FALSE );
				if ( index != wxNOT_FOUND ) RootEntry->m_AliasList.RemoveAt(index);
			}
			if ( index == wxNOT_FOUND )
				DisplayError(this, wxT("Warning: Root for Alias as no alias list"), 30);
		}

		/* Effacement memoire pour cet alias */
		PQDelete( &Library->m_Entries, (void*) Entry );
		delete Entry;
		if( Library->m_NumOfParts > 0 ) CurrentLib->m_NumOfParts --;
		return;
	}

	/* Entry is a standard component (not an alias) */
	if( Entry->m_AliasList.GetCount() == 0) // Trivial case: no alias, we can safety delete e=this entry
	{
		PQDelete( &Library->m_Entries, Entry );
		delete Entry;
		if( Library->m_NumOfParts > 0 ) Library->m_NumOfParts --;
		return;
	}

	/* Entry is a component with alias
	We must change the first alias to a "root" component, and for all the aliases
	we must change the root component (which is deleted) by the first alias */
	wxString AliasName = Entry->m_AliasList[0];

	/* The root component is not really deleted, it is renamed with the first alias name */
	AliasEntry = (EDA_LibCmpAliasStruct*) FindLibPart(
								AliasName.GetData(), Library->m_Name, FIND_ALIAS);
	if( AliasEntry == NULL )
	{
		wxString msg;
		msg.Printf(wxT("Warning: Alias <%s> not found"), AliasName.GetData());
		DisplayError(this, msg, 30);
	}

	else
	{
		if( Library->m_NumOfParts > 0 ) Library->m_NumOfParts --;

		/* remove the root component from library */
		PQDelete( &Library->m_Entries, Entry );

		/* remove the first alias from library*/
		PQDelete( &Library->m_Entries, AliasEntry );

		/* remove the first alias name from alias list: */
		Entry->m_AliasList.RemoveAt(0);
		/* change the old name. New name for "root" is the name of the first alias */
		Entry->m_Name.m_Text = AliasName;
		Entry->m_Doc = AliasEntry->m_Doc;

		Entry->m_KeyWord = AliasEntry->m_KeyWord;

		FreeLibraryEntry((EDA_LibComponentStruct *)AliasEntry);

		/* root component (renamed) placed in library */
		PQInsert( &Library->m_Entries, Entry);
	}

	/* Change the "RootName", for other aliases */
	for( unsigned ii = 0; ii < Entry->m_AliasList.GetCount(); ii++ )
	{
		AliasName = Entry->m_AliasList[ii];

		AliasEntry = (EDA_LibCmpAliasStruct*) FindLibPart(
							AliasName.GetData(), Library->m_Name, FIND_ALIAS);
		if( AliasEntry == NULL )
		{	// Should not occurs. If happens, this is an error (or bug)
			wxString msg;
			msg.Printf( wxT("Warning: Alias <%s> not found"), AliasName.GetData());
			DisplayError(this, msg, 30);
			continue;
		}
		if( AliasEntry->Type != ALIAS )
		{	// Should not occurs. If happens, this is an error (or bug)
			wxString msg;
			msg.Printf( wxT("Warning: <%s> is not an Alias"), AliasName.GetData());
			DisplayError(this, msg, 30);
			continue;
		}
		AliasEntry->m_RootName = Entry->m_Name.m_Text;
	}
}


/***************************************************/
void WinEDA_LibeditFrame::SaveOnePartInMemory()
/***************************************************/
/* Routine de sauvegarde de la "partlib" courante dans la librairie courante
	Sauvegarde en memoire uniquement, et PAS sur fichier
	La routine efface l'ancien composant ( ou / et les alias ) a remplacer
	s'il existe, et sauve le nouveau et cree les alias correspondants.
*/
{
EDA_LibComponentStruct *Entry;
EDA_LibCmpAliasStruct *AliasEntry;
wxString msg;
bool NewCmp = TRUE;

	if(CurrentLibEntry == NULL)
	{
		DisplayError(this, _("No component to Save.") ); return;
	}

	if(CurrentLib == NULL) SelectActiveLibrary();

	if(CurrentLib == NULL)
	{
		DisplayError(this, _("No Library specified."), 20); return;
	}

	CurrentLib->m_Modified = 1;
	ScreenLib->ClrModify();

	PQCompFunc((PQCompFuncType) LibraryEntryCompare);

	if( (Entry = FindLibPart(CurrentLibEntry->m_Name.m_Text.GetData(),
				CurrentLib->m_Name, FIND_ROOT)) != NULL)
	{
		msg.Printf( _("Component \"%s\" exists. Change it?"),
					Entry->m_Name.m_Text.GetData());
		if( !IsOK(this, msg) ) return;
		NewCmp = FALSE;
	}

	/* Effacement des alias deja existants en librairie */
	for ( unsigned ii = 0; ii <  CurrentLibEntry->m_AliasList.GetCount(); ii += ALIAS_NEXT )
	{
		EDA_LibComponentStruct * LocalEntry;
		wxString aliasname = CurrentLibEntry->m_AliasList[ii+ALIAS_NAME];
		while( (LocalEntry = FindLibPart(aliasname.GetData(), CurrentLib->m_Name, FIND_ALIAS)) != NULL )
		{
			DeletePartInLib( CurrentLib, LocalEntry );
		}
	}

	if( !NewCmp )DeletePartInLib( CurrentLib, Entry );

	Entry = CopyLibEntryStruct(this, CurrentLibEntry);
	Entry->m_AliasList.Clear();
	PQInsert( &CurrentLib->m_Entries, (void*)Entry );
	CurrentLib->m_NumOfParts ++;

	/* Creation des nouveaux alias */
	for ( unsigned ii = 0; ii <  CurrentLibEntry->m_AliasList.GetCount(); ii += ALIAS_NEXT )
	{
		wxString aliasname = CurrentLibEntry->m_AliasList[ii+ALIAS_NAME];
		Entry->m_AliasList.Add(aliasname);
		AliasEntry = new EDA_LibCmpAliasStruct(aliasname.GetData(), Entry->m_Name.m_Text);
		AliasEntry->m_Doc = CurrentLibEntry->m_AliasList[ii+ALIAS_DOC];
		AliasEntry->m_KeyWord = CurrentLibEntry->m_AliasList[ii+ALIAS_KEYWORD];
		AliasEntry->m_DocFile = CurrentLibEntry->m_AliasList[ii+ALIAS_DOC_FILENAME];

		/* Placement en liste des composants de l'Alias */
		PQInsert( &CurrentLib->m_Entries, (void*) AliasEntry );
		CurrentLib->m_NumOfParts ++;
	}

	msg.Printf( _("Component %s saved in %s"),
			Entry->m_Name.m_Text.GetData(), CurrentLib->m_Name.GetData());
	Affiche_Message(msg);
}


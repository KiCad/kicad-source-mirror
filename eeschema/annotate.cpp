	/**************************************/
	/* annotate.cpp: component annotation */
	/**************************************/

#include "annotate_dialog.cpp"

#include "netlist.h"
#include "protos.h"

/* fonctions exportees */
int ListeComposants( CmpListStruct * BaseListeCmp, SCH_SCREEN *screen, int NumSheet);
int AnnotTriComposant(CmpListStruct *Objet1, CmpListStruct *Objet2);
void BreakReference( CmpListStruct * BaseListeCmp,int NbOfCmp );

/* fonctions locales */
static void ReAnnotateComponents( CmpListStruct * BaseListeCmp,int NbOfCmp );
static void ComputeReferenceNumber( CmpListStruct * BaseListeCmp,int NbOfCmp);
static int GetLastReferenceNumber(CmpListStruct *Objet, CmpListStruct * BaseListeCmp,
														int NbOfCmp);
static int ExistUnit(CmpListStruct *Objet, int Unit,
								 CmpListStruct * BaseListeCmp,int NbOfCmp);

/* Variable locales */
static bool AnnotProject = TRUE;


/**************************************/
void ReAnnotatePowerSymbolsOnly( void )
/**************************************/
/* Used to reannotate the power symbols, before testing erc or computing netlist
when a true component reannotation is not necessary

In order to avoid conflicts the ref number start with a 0:
	PWR with id 12 is named PWR12 in global annotation and PWR012 by the Power annotation
*/
{
	/* Build the screen list */
	EDA_ScreenList ScreenList(NULL);

	/* Update the sheet number, sheet count and date */
	ScreenList.UpdateSheetNumberAndDate();

	SCH_SCREEN* screen;
	int CmpNumber = 1;
	for ( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
	{
		EDA_BaseStruct *DrawList = screen->EEDrawList;
		for ( ; DrawList != NULL ; DrawList = DrawList->Pnext )
		{
			if ( DrawList->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
				continue;
			EDA_SchComponentStruct * DrawLibItem = (EDA_SchComponentStruct *) DrawList;
			EDA_LibComponentStruct * Entry =
				FindLibPart(DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT);
			if ( (Entry == NULL) || (Entry->m_Options != ENTRY_POWER) )
				continue;
			DrawLibItem->ClearAnnotation();
			DrawLibItem->m_RefIdNumber = CmpNumber;
			DrawLibItem->m_Field[REFERENCE].m_Text.RemoveLast();	// Remove the '?'
			DrawLibItem->m_Field[REFERENCE].m_Text << wxT("0") << CmpNumber;
			CmpNumber++;
		}
	}
}


/******************************************************************/
void InstallAnnotateFrame(WinEDA_SchematicFrame *parent, wxPoint & pos)
/******************************************************************/
{
	WinEDA_AnnotateFrame * frame = new WinEDA_AnnotateFrame(parent);
	frame->ShowModal(); frame->Destroy();
}


/******************************************************************/
void WinEDA_AnnotateFrame::AnnotateComponents(wxCommandEvent& event)
/******************************************************************/
/*
	Compute the annotation of the components for the whole projeect, or the current sheet only.
	All the components or the new ones only will be annotated.
*/
{
int NbSheet, ii , NbOfCmp;
SCH_SCREEN *screen;
CmpListStruct * BaseListeCmp;

	wxBusyCursor dummy;

	AnnotProject = (m_AnnotProjetCtrl->GetSelection() == 0) ? TRUE : FALSE;

	/* If it is an annotation for all the components, reset previous annotation: */
	if( m_AnnotNewCmpCtrl->GetSelection() == 0 ) DeleteAnnotation(event);
	if (m_Abort ) return;


	/* Build the screen list */
	EDA_ScreenList ScreenList(NULL);
	NbSheet = ScreenList.GetCount();

	/* Update the sheet number, sheet count and date */
	ScreenSch->SetModify();
	ScreenList.UpdateSheetNumberAndDate();

	/* First pass: Component counting */
	screen = (SCH_SCREEN*) m_Parent->m_CurrentScreen;
	if( AnnotProject == TRUE )
	{
		NbOfCmp = 0;
		for ( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
		{
			NbOfCmp += ListeComposants(NULL, screen, screen->m_SheetNumber);
		}
	}
	else NbOfCmp = ListeComposants(NULL, screen, screen->m_SheetNumber);

	if( NbOfCmp == 0 )
		return;

	ii = sizeof(CmpListStruct) * NbOfCmp;
	BaseListeCmp = (CmpListStruct *) MyZMalloc(ii);

	/* Second pass : Int data tables */
	screen = (SCH_SCREEN*) m_Parent->m_CurrentScreen;
	if( AnnotProject == TRUE )
	{
		ii = 0;
		for ( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
		{
			ii += ListeComposants(BaseListeCmp + ii,
									screen, screen->m_SheetNumber);
		}
	}
	else ii = ListeComposants(BaseListeCmp, screen, screen->m_SheetNumber);

	if( ii != NbOfCmp )
		DisplayError(this, wxT("Internal error in AnnotateComponents()"));

	/* Separation des Numeros de la reference: IC1 -> IC, et 1 dans .m_NumRef */
	BreakReference(BaseListeCmp, NbOfCmp);

	qsort( BaseListeCmp, NbOfCmp, sizeof(CmpListStruct),
				(int(*)(const void*, const void*))AnnotTriComposant);

	/* Recalcul des numeros de reference */
	ComputeReferenceNumber(BaseListeCmp, NbOfCmp);
	ReAnnotateComponents(BaseListeCmp, NbOfCmp);

	MyFree(BaseListeCmp); BaseListeCmp = NULL;

	/* Final control */
	CheckAnnotate(m_Parent, AnnotProject ? FALSE : TRUE);

	m_Parent->DrawPanel->Refresh(TRUE);	/* Refresh screen */
	Close();
}


/********************************************************************/
void WinEDA_AnnotateFrame::DeleteAnnotation(wxCommandEvent& event)
/********************************************************************/
/* Clear the current annotation for the whole project or only for the current sheet
	Update sheet number and number of sheets
*/
{
int NbSheet;
SCH_SCREEN * screen;
EDA_SchComponentStruct *DrawLibItem;

	if( !IsOK(this, _("Previous Annotation will be deleted. Continue ?") ) )
	{
		m_Abort = TRUE; return;
	}

	AnnotProject = (m_AnnotProjetCtrl->GetSelection() == 0) ? TRUE : FALSE;
	m_Abort = FALSE;

	/* Build the screen list */
	EDA_ScreenList ScreenList(NULL);
	NbSheet = ScreenList.GetCount();

	/* Update the sheet number, sheet count and date */
	ScreenList.UpdateSheetNumberAndDate();

	ScreenSch->SetModify();

	if( AnnotProject == TRUE ) screen = ScreenList.GetFirst();
	else screen = (SCH_SCREEN*) m_Parent->m_CurrentScreen;

	for ( ; screen != NULL; screen = ScreenList.GetNext() )
	{
		EDA_BaseStruct *DrawList = screen->EEDrawList;
		for ( ; DrawList != NULL ; DrawList = DrawList->Pnext )
		{
			if ( DrawList->m_StructType == DRAW_LIB_ITEM_STRUCT_TYPE )
			{
				DrawLibItem = (EDA_SchComponentStruct *) DrawList;
				DrawLibItem->ClearAnnotation();
			}
		}
		if( ! AnnotProject ) break;
	}

	m_Parent->DrawPanel->Refresh(TRUE);
	Close();
}


/************************************************************************************/
int ListeComposants(CmpListStruct * BaseListeCmp, SCH_SCREEN *screen, int NumSheet)
/***********************************************************************************/
/*	if BaseListeCmp == NULL : Components counting
	else update data table BaseListeCmp
*/
{
int NbrCmp = 0;
EDA_BaseStruct *DrawList = screen->EEDrawList;
EDA_SchComponentStruct *DrawLibItem;
EDA_LibComponentStruct *Entry;

	DrawList = screen->EEDrawList;

	while ( DrawList )
		{
		switch( DrawList->m_StructType )
			{
			case DRAW_SEGMENT_STRUCT_TYPE :
			case DRAW_JUNCTION_STRUCT_TYPE :
			case DRAW_TEXT_STRUCT_TYPE :
			case DRAW_LABEL_STRUCT_TYPE :
			case DRAW_GLOBAL_LABEL_STRUCT_TYPE :
				break;

			case DRAW_LIB_ITEM_STRUCT_TYPE :
				DrawLibItem = (EDA_SchComponentStruct *) DrawList;
  				Entry = FindLibPart(DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT);
 				if( Entry == NULL) break;
				if ( BaseListeCmp == NULL )	/* Items counting only */
				{
					NbrCmp++; break;
				}
				BaseListeCmp[NbrCmp].m_Cmp = DrawLibItem;
				BaseListeCmp[NbrCmp].m_NbParts = Entry->m_UnitCount;
				BaseListeCmp[NbrCmp].m_Unit = DrawLibItem->m_Multi;
				BaseListeCmp[NbrCmp].m_PartsLocked = Entry->m_UnitSelectionLocked;
				BaseListeCmp[NbrCmp].m_Sheet = NumSheet;
				BaseListeCmp[NbrCmp].m_IsNew = FALSE;
				BaseListeCmp[NbrCmp].m_TimeStamp = DrawLibItem->m_TimeStamp;
				if( DrawLibItem->m_Field[REFERENCE].m_Text.IsEmpty() )
					DrawLibItem->m_Field[REFERENCE].m_Text = wxT("DefRef?");
				strncpy( BaseListeCmp[NbrCmp].m_TextRef,
						CONV_TO_UTF8(DrawLibItem->m_Field[REFERENCE].m_Text), 32);

				BaseListeCmp[NbrCmp].m_NumRef = -1;

				if( DrawLibItem->m_Field[VALUE].m_Text.IsEmpty() )
					DrawLibItem->m_Field[VALUE].m_Text = wxT("~");
				strncpy( BaseListeCmp[NbrCmp].m_TextValue,
						CONV_TO_UTF8(DrawLibItem->m_Field[VALUE].m_Text), 32);
				NbrCmp++;
				break;

			case DRAW_PICK_ITEM_STRUCT_TYPE :
			case DRAW_POLYLINE_STRUCT_TYPE :
			case DRAW_BUSENTRY_STRUCT_TYPE :
			case DRAW_SHEET_STRUCT_TYPE :
			case DRAW_SHEETLABEL_STRUCT_TYPE :
			case DRAW_MARKER_STRUCT_TYPE :
			case DRAW_NOCONNECT_STRUCT_TYPE :
				break;

			default :
				break;
			}

		DrawList = DrawList->Pnext;
		}

return(NbrCmp);
}

/*****************************************************************/
int AnnotTriComposant(CmpListStruct *Objet1, CmpListStruct *Objet2)
/****************************************************************/
/* function used par qsort() for sorting the list
	Composants are sorted
		by reference
		if same reference: by value
			if same value: by unit number
				if same unit number, by sheet
					if same sheet, by time stamp
**/
{
int ii;

	ii = strnicmp( Objet1->m_TextRef, Objet2->m_TextRef, 32 );

	if ( ii == 0 ) ii = strnicmp( Objet1->m_TextValue, Objet2->m_TextValue, 32 );
	if ( ii == 0 ) ii = Objet1->m_Unit - Objet2->m_Unit;
	if ( ii == 0 ) ii = Objet1->m_Sheet - Objet2->m_Sheet;
	if ( ii == 0 ) ii = Objet1->m_TimeStamp - Objet2->m_TimeStamp;

	return(ii);
}


/********************************************************************/
static void ReAnnotateComponents( CmpListStruct * BaseListeCmp, int NbOfCmp)
/********************************************************************/
/* Update the reference component for the schematic project (or the current sheet)
*/
{
int ii;
char *Text;
EDA_SchComponentStruct *DrawLibItem;

	/* Reattribution des numeros */
	for ( ii = 0; ii < NbOfCmp ; ii++ )
	{
		Text = BaseListeCmp[ii].m_TextRef;
		DrawLibItem = BaseListeCmp[ii].m_Cmp;

		if ( BaseListeCmp[ii].m_NumRef < 0 ) strcat( Text, "?" );
		else sprintf( Text + strlen(Text),"%d",BaseListeCmp[ii].m_NumRef );

		DrawLibItem->m_Field[REFERENCE].m_Text = CONV_FROM_UTF8(Text);
		DrawLibItem->m_Multi = BaseListeCmp[ii].m_Unit;
		DrawLibItem->m_RefIdNumber = BaseListeCmp[ii].m_NumRef;
		if ( DrawLibItem->m_RefIdNumber < 0 ) DrawLibItem->m_RefIdNumber = 0;
	}
}



/**************************************************************/
void BreakReference( CmpListStruct * BaseListeCmp, int NbOfCmp)
/**************************************************************/

/* Modifie dans BaseListeCmp la reference des composants en supprimant la
	partie nombre de la partie texte.
Place le nombre dans .m_NumRef
Pour les composants multiples non encore annotes, met .m_Unit a sa valeur max
	Utilise:
		BaseListeCmp
		NbOfCmp
*/
{
int ii, ll;
char * Text;

	/* Separation des Numeros de la reference: IC1 -> IC, et 1 dans .m_NumRef */
	for ( ii = 0; ii < NbOfCmp ; ii++ )
	{
		BaseListeCmp[ii].m_NumRef = - 1;
		Text = BaseListeCmp[ii].m_TextRef;
		ll = strlen( Text ) - 1;
		if( Text[ll] == '?' )
		{
			BaseListeCmp[ii].m_IsNew = TRUE;
			if ( ! BaseListeCmp[ii].m_PartsLocked )
				BaseListeCmp[ii].m_Unit = 0x7FFFFFFF;
			Text[ll] = 0; continue;
		}

		if( isdigit(Text[ll]) == 0 )
		{
			BaseListeCmp[ii].m_IsNew = TRUE;
			if ( ! BaseListeCmp[ii].m_PartsLocked )
				BaseListeCmp[ii].m_Unit = 0x7FFFFFFF;
			continue;
		}

		while( ll >= 0 )
		{
			if( (Text[ll] <= ' ' ) || isdigit(Text[ll]) )
				ll--;
			else
			{
				if( isdigit(Text[ll+1]) )
					BaseListeCmp[ii].m_NumRef = atoi(& Text[ll+1]);
				Text[ll+1] = 0;
				break;
			}
		}
	}
}


/*****************************************************************************/
static void ComputeReferenceNumber( CmpListStruct * BaseListeCmp, int NbOfCmp)
/*****************************************************************************/
/* Compute the reference number for components without reference number
	Compute	.m_NumRef member
*/
{
int ii, jj, LastReferenceNumber, NumberOfUnits, Unit;
char * Text, * RefText, *ValText;
CmpListStruct * ObjRef, * ObjToTest;

	/* Components with an invisible reference (power...) always are re-annotated*/
	for ( ii = 0; ii < NbOfCmp ; ii++ )
	{
		Text = BaseListeCmp[ii].m_TextRef;
		if (*Text == '#')
		{
			BaseListeCmp[ii].m_IsNew = TRUE;
			BaseListeCmp[ii].m_NumRef = 0;
		}
	}

	ValText = RefText = ""; LastReferenceNumber = 1;
	for ( ii = 0; ii < NbOfCmp ; ii++ )
	{
		ObjRef = & BaseListeCmp[ii];
		if( BaseListeCmp[ii].m_Flag ) continue;

		Text = BaseListeCmp[ii].m_TextRef;
		if( strnicmp(RefText, Text, 32) != 0 ) /* Nouveau Identificateur */
		{
			RefText = BaseListeCmp[ii].m_TextRef;
			LastReferenceNumber = GetLastReferenceNumber(BaseListeCmp + ii, BaseListeCmp, NbOfCmp);
		}

		/* Annotation of mono-part components ( 1 part per package ) (trivial case)*/
		if( BaseListeCmp[ii].m_NbParts <= 1 )
		{
			if ( BaseListeCmp[ii].m_IsNew )
			{
				LastReferenceNumber++;
				BaseListeCmp[ii].m_NumRef = LastReferenceNumber;
			}
			BaseListeCmp[ii].m_Unit = 1;
			BaseListeCmp[ii].m_Flag = 1;
			BaseListeCmp[ii].m_IsNew = FALSE;
			continue;
		}

		/* Annotation of multi-part components ( n parts per package ) (complex case) */
		ValText = BaseListeCmp[ii].m_TextValue;
		NumberOfUnits = BaseListeCmp[ii].m_NbParts;

		if ( BaseListeCmp[ii].m_IsNew )
		{
			LastReferenceNumber++; BaseListeCmp[ii].m_NumRef = LastReferenceNumber;
			if ( ! BaseListeCmp[ii].m_PartsLocked )
				BaseListeCmp[ii].m_Unit = 1;
			BaseListeCmp[ii].m_Flag = 1;
		}

		for( Unit = 1; Unit <= NumberOfUnits; Unit++ )
		{
			if( BaseListeCmp[ii].m_Unit == Unit ) continue;
			jj = ExistUnit( BaseListeCmp + ii , Unit, BaseListeCmp, NbOfCmp );
			if ( jj >= 0 ) continue;	/* Unit exists for this reference */

			/* Search a component to annotate ( same prefix, same value) */
			for ( jj = ii+1; jj < NbOfCmp ; jj++ )
			{
				ObjToTest = &BaseListeCmp[jj];
				if( BaseListeCmp[jj].m_Flag ) continue;
				Text = BaseListeCmp[jj].m_TextRef;
				if( strnicmp(RefText, Text, 32) != 0 ) break;	// references are different
				Text = BaseListeCmp[jj].m_TextValue;
				if( strnicmp(ValText, Text, 32) != 0 ) break;	// values are different
				if ( ! BaseListeCmp[jj].m_IsNew )
				{
					//BaseListeCmp[jj].m_Flag = 1;
					continue;
				}
				/* Component without reference number found, annotate it if possible */
				if ( ! BaseListeCmp[jj].m_PartsLocked || (BaseListeCmp[jj].m_Unit == Unit) )
				{
					BaseListeCmp[jj].m_NumRef = BaseListeCmp[ii].m_NumRef;
					BaseListeCmp[jj].m_Unit = Unit;
					BaseListeCmp[jj].m_Flag = 1;
					BaseListeCmp[jj].m_IsNew = FALSE;
					break;
				}
			}
		}
	}
}


/*************************************************************************************************/
static int GetLastReferenceNumber(CmpListStruct *Objet, CmpListStruct * BaseListeCmp, int NbOfCmp)
/*************************************************************************************************/
/* Recherche le plus grand numero de reference dans les composants
	de meme prefixe de reference que celui pointe par Objet
	la liste des composants est supposee triee
*/
{
CmpListStruct * LastObjet = BaseListeCmp + NbOfCmp;
int LastNumber = 0;
const char * RefText;

	RefText = Objet->m_TextRef;
	for ( ; Objet < LastObjet; Objet++ )
	{
		if( strnicmp(RefText, Objet->m_TextRef, 32) != 0 ) /* Nouveau Identificateur */
			break;
		if( LastNumber < Objet->m_NumRef ) LastNumber = Objet->m_NumRef;
	}
	return(LastNumber);
}


/*****************************************************************/
static int ExistUnit(CmpListStruct *Objet, int Unit,
						CmpListStruct * BaseListeCmp, int NbOfCmp)
/****************************************************************/
/* Recherche dans la liste triee des composants, pour les composants
	multiples s'il existe pour le composant de reference Objet,
	une unite de numero Unit
		Retourne index dans BaseListeCmp si oui
		retourne -1 si non
*/
{
CmpListStruct * EndList = BaseListeCmp + NbOfCmp;
char * RefText, * ValText;
int NumRef, ii;
CmpListStruct *ItemToTest;

	RefText = Objet->m_TextRef;
	ValText = Objet->m_TextValue;
	NumRef = Objet->m_NumRef;
	for ( ItemToTest = BaseListeCmp, ii = 0; ItemToTest < EndList; ItemToTest++, ii++ )
	{
		if ( Objet == ItemToTest ) continue;
		if ( ItemToTest->m_IsNew ) continue;  /* non affecte */
		if ( ItemToTest->m_NumRef != NumRef ) continue;
		if( strnicmp(RefText, ItemToTest->m_TextRef, 32) != 0 ) /* Nouveau Identificateur */
			continue;
		if( ItemToTest->m_Unit == Unit)
		{
			return(ii);
		}
	}
	return(-1);
}

/******************************************************************/
int CheckAnnotate(WinEDA_SchematicFrame * frame, bool OneSheetOnly)
/******************************************************************/

/* Retourne le nombre de composants non annotés ou de meme référence (doublés)
	Si OneSheetOnly : recherche sur le schema courant
	sinon: recherche sur toute la hierarchie
*/
{
int NbSheet, ii, NumSheet = 1, error, NbOfCmp;
SCH_SCREEN *screen;
CmpListStruct * ListeCmp = NULL;
wxString Buff;
wxString msg, cmpref;

	/* build tje screen list */
	EDA_ScreenList ScreenList(NULL);
	NbSheet = ScreenList.GetCount();

	/* Update the sheet number, sheet count and date */
	ScreenSch->SetModify();
	ScreenList.UpdateSheetNumberAndDate();
	
	/* 1ere passe : Comptage du nombre de composants */
	screen = (SCH_SCREEN*) frame->m_CurrentScreen;
	if( ! OneSheetOnly )
	{
		NbOfCmp = 0;
		for ( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
		{
			NbOfCmp += ListeComposants(NULL, screen, NumSheet);
		}
	}
	else NbOfCmp = ListeComposants(NULL, screen, NumSheet);

	if( NbOfCmp == 0 )
	{
		wxBell();
		return(0);
	}

	ii = sizeof(CmpListStruct) * NbOfCmp;
	ListeCmp = (CmpListStruct *) MyZMalloc(ii);

	/* 2eme passe : Remplissage du tableau des caracteristiques */
	if( OneSheetOnly == 0 )
	{
		ii = 0; screen = ScreenSch;
		for ( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
		{
			ii += ListeComposants(ListeCmp + ii, screen, NumSheet);
		}
	}
	else
	{
		screen = (SCH_SCREEN*) frame->m_CurrentScreen;
		ListeComposants(ListeCmp, screen, NumSheet);
	}

	qsort( ListeCmp, NbOfCmp, sizeof(CmpListStruct),
				(int(*)(const void*, const void*))AnnotTriComposant);


	/* Separation des Numeros de la reference: IC1 -> IC, et 1 dans .m_NumRef */
	BreakReference(ListeCmp, NbOfCmp);

	/* comptage des elements non annotes */
	error = 0;
	for( ii = 0; ii < NbOfCmp-1 ; ii++ )
	{
		msg.Empty(); Buff.Empty();
		if ( ListeCmp[ii].m_IsNew )
		{
			if( ListeCmp[ii].m_NumRef >= 0 ) Buff << ListeCmp[ii].m_NumRef;
			else Buff = wxT("?");
			cmpref = CONV_FROM_UTF8(ListeCmp[ii].m_TextRef);
			msg.Printf( _("item not annotated: %s%s"), cmpref.GetData(), Buff.GetData());

			if( (ListeCmp[ii].m_Unit > 0) && (ListeCmp[ii].m_Unit < 0x7FFFFFFF) )
			{
				Buff.Printf( _("( unit %d)"), ListeCmp[ii].m_Unit);
				msg << Buff;
			}
			DisplayError(NULL, msg);
			error++; break;
		}

		if( MAX(ListeCmp[ii].m_NbParts, 1) < ListeCmp[ii].m_Unit  )	// Annotate error
		{
			if( ListeCmp[ii].m_NumRef >= 0 ) Buff << ListeCmp[ii].m_NumRef;
			else Buff = wxT("?");

			cmpref = CONV_FROM_UTF8(ListeCmp[ii].m_TextRef);
			msg.Printf( _("Error item %s%s"), cmpref.GetData(), Buff.GetData());

			Buff.Printf( _(" unit %d and no more than %d parts"),
										ListeCmp[ii].m_Unit, ListeCmp[ii].m_NbParts);
			msg << Buff;
			DisplayError(frame, msg);
			error++; break;
		}
	}

	if ( error ) return error;

	/* comptage des elements doublés (si tous sont annotés) */
	for( ii = 0; (ii < NbOfCmp-1) && (error < 4); ii++ )
	{
		msg.Empty(); Buff.Empty();
		if( (stricmp(ListeCmp[ii].m_TextRef,ListeCmp[ii+1].m_TextRef) != 0) ||
			( ListeCmp[ii].m_NumRef != ListeCmp[ii+1].m_NumRef ) )
			continue;
		/* Meme reference trouvée */

		/* Il y a erreur si meme unite */
		if( ListeCmp[ii].m_Unit == ListeCmp[ii+1].m_Unit )
		{

			if( ListeCmp[ii].m_NumRef >= 0 ) Buff << ListeCmp[ii].m_NumRef;
			else Buff = wxT("?");

			cmpref = CONV_FROM_UTF8(ListeCmp[ii].m_TextRef);
			msg.Printf( _("Multiple item %s%s"),
						cmpref.GetData(),Buff.GetData());

			if( (ListeCmp[ii].m_Unit > 0) && (ListeCmp[ii].m_Unit < 0x7FFFFFFF) )
			{
				Buff.Printf( _(" (unit %d)"), ListeCmp[ii].m_Unit);
				msg << Buff;
			}
			DisplayError(frame, msg);
			error++; continue;
		}

		/* Il y a erreur si unites differentes mais nombre de parts differentes
		par boitier (ex U3 ( 1 part) et U3B sont incompatibles) */
		if( ListeCmp[ii].m_NbParts != ListeCmp[ii+1].m_NbParts )
		{

			if( ListeCmp[ii].m_NumRef >= 0 ) Buff << ListeCmp[ii].m_NumRef;
			else Buff = wxT("?");

			cmpref = CONV_FROM_UTF8(ListeCmp[ii].m_TextRef);
			msg.Printf( _("Multiple item %s%s"), cmpref.GetData(), Buff.GetData());

			if( (ListeCmp[ii].m_Unit > 0) && (ListeCmp[ii].m_Unit < 0x7FFFFFFF) )
			{
				Buff.Printf( _(" (unit %d)"), ListeCmp[ii].m_Unit);
				msg << Buff;
			}

			DisplayError(frame, msg);
			error++;
		}

		/* Il y a erreur si unites differentes ET valeurs différentes */
		if( stricmp(ListeCmp[ii].m_TextValue,ListeCmp[ii+1].m_TextValue) != 0)
		{
			wxString nextcmpref, cmpvalue, nextcmpvalue;
			cmpref = CONV_FROM_UTF8(ListeCmp[ii].m_TextRef);
			nextcmpref = CONV_FROM_UTF8(ListeCmp[ii+1].m_TextRef);
			cmpvalue = CONV_FROM_UTF8(ListeCmp[ii].m_TextValue);
			nextcmpvalue = CONV_FROM_UTF8(ListeCmp[ii+1].m_TextValue);
			msg.Printf( _("Diff values for %s%d%c (%s) and %s%d%c (%s)"),
						cmpref.GetData(), ListeCmp[ii].m_NumRef, ListeCmp[ii].m_Unit+'A'-1,
						cmpvalue.GetData(),
						nextcmpref.GetData(), ListeCmp[ii+1].m_NumRef, ListeCmp[ii+1].m_Unit+'A'-1,
						nextcmpvalue.GetData());

			DisplayError(frame, msg);
			error++;
		}
	}

	MyFree(ListeCmp);
	return(error);
}



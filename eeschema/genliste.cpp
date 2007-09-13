	/***********************************************************/
	/*					genliste.cpp						   */
	/* Module de generation de listing de composants, labels.. */
	/***********************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"

#include "protos.h"

/* Structures pour memo et liste des elements */
typedef struct ListLabel
{
	int Type();
	void * m_Label;
	int m_SheetNumber;
} ListLabel;


/* fonctions locales */
static void GenereListeOfItems(wxWindow * frame, const wxString & FullFileName);
static int GenListeGLabels( ListLabel * List );
static int ListTriComposantByRef(EDA_SchComponentStruct **Objet1,
									EDA_SchComponentStruct **Objet2);
static int ListTriComposantByVal(EDA_SchComponentStruct **Objet1,
									EDA_SchComponentStruct **Objet2);
static int ListTriGLabelBySheet(ListLabel *Objet1, ListLabel *Objet2);
static int ListTriGLabelByVal(ListLabel *Objet1, ListLabel *Objet2);
static void DeleteSubCmp( EDA_BaseStruct ** List , int NbItems);

static int PrintListeCmpByRef( FILE *f, EDA_BaseStruct **List, int NbItems);
static int PrintListeCmpByVal( FILE *f, EDA_BaseStruct **List, int NbItems);

static int PrintListeGLabel( FILE *f, ListLabel *List, int NbItems);

// Constantes:
#define LISTCMP_BY_REF 1
#define LISTCMP_BY_VAL 2
#define LIST_SUBCMP 4
#define LIST_HPINS_BY_NAME 8
#define LIST_HPINS_BY_SHEET 0x10

/* Variable locales */
int ItemsToList = LISTCMP_BY_REF | LISTCMP_BY_VAL;

enum id_tools {
	ID_TOOLS_GEN_LIST = 1500,
	ID_EXIT_TOOLS,
	ID_SET_FILENAME_LIST
};

/* Classe de la frame de gestion de l'annotation */
class WinEDA_GenCmpListFrame: public wxDialog
{
public:
	WinEDA_DrawFrame * m_Parent;

	wxCheckBox * m_ListCmpbyRefItems;
	wxCheckBox * m_ListCmpbyValItems;
	wxCheckBox * m_ListSubCmpItems;
	wxCheckBox * m_GenListLabelsbyVal;
	wxCheckBox * m_GenListLabelsbySheet;
	wxString m_LibArchiveFileName;
	wxString m_ListFileName;

	// Constructor and destructor
	WinEDA_GenCmpListFrame(WinEDA_DrawFrame *parent, wxPoint& pos);
	~WinEDA_GenCmpListFrame() {};

	void GenList(wxCommandEvent& event);
	void GenListUpdateOpt();
	void ToolsExit(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(WinEDA_GenCmpListFrame, wxDialog)
	EVT_BUTTON(ID_EXIT_TOOLS, WinEDA_GenCmpListFrame::ToolsExit)
	EVT_BUTTON(ID_TOOLS_GEN_LIST, WinEDA_GenCmpListFrame::GenList)
END_EVENT_TABLE()


/***************************************************************/
void InstallToolsFrame(WinEDA_DrawFrame *parent, wxPoint & pos)
/***************************************************************/
{
	WinEDA_GenCmpListFrame * frame = new WinEDA_GenCmpListFrame(parent, pos);
	frame->ShowModal(); frame->Destroy();
}


/*****************************************************************************/
WinEDA_GenCmpListFrame::WinEDA_GenCmpListFrame(WinEDA_DrawFrame *parent,
		wxPoint& framepos):
		wxDialog(parent, -1, _("List of Material"), framepos, wxSize(360, 200),
			 DIALOG_STYLE)
/*****************************************************************************/
{
wxPoint pos;

	m_Parent = parent;
	SetFont(*g_DialogFont);

	if ( (framepos.x == -1) && (framepos.x == -1) ) Centre();

	/* Calcul du nom du fichier d'archivage */
	m_LibArchiveFileName = MakeFileName(wxEmptyString,ScreenSch->m_FileName,wxEmptyString);
	/* mise a jour extension  */
	ChangeFileNameExt(m_LibArchiveFileName, g_LibExtBuffer);

	/* Calcul du nom du fichier de listage */
	m_ListFileName = MakeFileName(wxEmptyString,ScreenSch->m_FileName,wxEmptyString);
	/* mise a jour extension  */
	ChangeFileNameExt(m_ListFileName, wxT(".lst"));

	pos.x = 5; pos.y = 15;
	new wxStaticBox(this, -1,_(" List items : "), pos, wxSize(230, 120));

	pos.x = 10; pos.y += 22;
	m_ListCmpbyRefItems = new wxCheckBox(this,-1, _("Components by Reference"), pos);
	if ( ItemsToList & LISTCMP_BY_REF ) m_ListCmpbyRefItems->SetValue(TRUE);
	pos.y += 17;
	m_ListCmpbyValItems = new wxCheckBox(this,-1, _("Components by Value"), pos);
	if ( ItemsToList & LISTCMP_BY_VAL ) m_ListCmpbyValItems->SetValue(TRUE);
	pos.y += 17;
	m_ListSubCmpItems = new  wxCheckBox(this,-1, _("Sub Components (i.e U2A, U2B..)"), pos);
	if ( ItemsToList & LIST_SUBCMP ) m_ListSubCmpItems->SetValue(TRUE);
	pos.y += 17;
	m_GenListLabelsbyVal = new wxCheckBox(this,-1, _("Hierachy Pins by name"), pos);
	if ( ItemsToList & LIST_HPINS_BY_NAME ) m_GenListLabelsbyVal->SetValue(TRUE);
	pos.y += 17;
	m_GenListLabelsbySheet = new wxCheckBox(this,-1, _("Hierachy Pins by Sheets"), pos);
	if ( ItemsToList & LIST_HPINS_BY_SHEET ) m_GenListLabelsbySheet->SetValue(TRUE);

	/* Creation des boutons de commande */
	pos.x = 260; pos.y = 40;
	wxButton * Button = new wxButton(this, ID_TOOLS_GEN_LIST,
						_("&Create List"), pos);
	Button->SetForegroundColour(*wxRED);

	pos.y += Button->GetSize().y + 5;
	Button = new wxButton(this,	ID_EXIT_TOOLS,
						_("&Exit"), pos);
	Button->SetForegroundColour(*wxBLUE);
}

// Fonctions de positionnement des variables d'option
void WinEDA_GenCmpListFrame::GenListUpdateOpt()
{
	ItemsToList = 0;
	if ( m_ListCmpbyRefItems->GetValue() ) ItemsToList |= LISTCMP_BY_REF;
	if ( m_ListCmpbyValItems->GetValue() ) ItemsToList |= LISTCMP_BY_VAL;
	if ( m_ListSubCmpItems->GetValue() ) ItemsToList |= LIST_SUBCMP;
	if ( m_GenListLabelsbyVal->GetValue() ) ItemsToList |= LIST_HPINS_BY_NAME;
	if ( m_GenListLabelsbySheet->GetValue() ) ItemsToList |= LIST_HPINS_BY_SHEET;
}

void WinEDA_GenCmpListFrame::GenList(wxCommandEvent& event)
{
#define EXT_LIST wxT(".lst")
wxString mask, filename;

	GenListUpdateOpt();

	mask = wxT("*"); mask += EXT_LIST;
	filename = EDA_FileSelector(_("List of material:"),
					wxEmptyString,				/* Chemin par defaut (ici dir courante) */
					m_ListFileName,	/* nom fichier par defaut, et resultat */
					EXT_LIST,		/* extension par defaut */
					mask,			/* Masque d'affichage */
					this,
					wxSAVE,
					TRUE
					);
	if ( filename.IsEmpty() ) return;
	else m_ListFileName = filename;

	GenereListeOfItems(this, m_ListFileName);
	Close();
}

void WinEDA_GenCmpListFrame::ToolsExit(wxCommandEvent& event)
{
	GenListUpdateOpt();
	Close();
}



/***************************************************************************/
static void GenereListeOfItems(wxWindow * frame, const wxString & FullFileName)
/***************************************************************************/
/*
	Routine principale pour la creation des listings ( composants et/ou labels
	globaux et "sheet labels" )
*/
{
FILE *f;
EDA_BaseStruct ** List;
ListLabel * ListOfLabels;
int NbItems;
char Line[1024];
wxString msg;
	
	/* Creation de la liste des elements */
	if ((f = wxFopen(FullFileName, wxT("wt"))) == NULL)
	{
		msg = _("Failed to open file "); msg << FullFileName;
		DisplayError(frame, msg);
		return;
	}

	NbItems = GenListeCmp(NULL );
	if ( NbItems )
	{
		List = (EDA_BaseStruct **)
				MyZMalloc( NbItems * sizeof(EDA_BaseStruct **) );
		if (List == NULL )
		{
			fclose(f);  return;
		}

		GenListeCmp(List);

		/* generation du fichier listing */
		DateAndTime(Line);
		fprintf( f, "%s  >> Creation date: %s\n", CONV_TO_UTF8(Main_Title), Line );

		/* Tri et impression de la liste des composants */

		qsort( List, NbItems, sizeof( EDA_BaseStruct * ),
				(int(*)(const void*, const void*))ListTriComposantByRef);

		if( (ItemsToList & LIST_SUBCMP) == 0 ) DeleteSubCmp(List, NbItems);

		if( (ItemsToList & LISTCMP_BY_REF) )
		{
			PrintListeCmpByRef(f, List, NbItems);
		}

		if( (ItemsToList & LISTCMP_BY_VAL) )
		{
			qsort( List, NbItems, sizeof( EDA_BaseStruct * ),
					(int(*)(const void*, const void*))ListTriComposantByVal);
			PrintListeCmpByVal(f, List, NbItems);
		}
		MyFree( List );
	}

	/***************************************/
	/* Generation liste des Labels globaux */
	/***************************************/

	NbItems = GenListeGLabels( NULL );
	if ( NbItems )
	{
		ListOfLabels = (ListLabel *) MyZMalloc( NbItems * sizeof(ListLabel) );
		if (ListOfLabels == NULL )
		{
			  fclose(f); return;
		}

		GenListeGLabels(ListOfLabels);

		/* Tri de la liste */
		if( (ItemsToList & LIST_HPINS_BY_SHEET) )
		{
			qsort( ListOfLabels, NbItems, sizeof( ListLabel ),
				(int(*)(const void*, const void*))ListTriGLabelBySheet);

			msg = _("\n#Glob labels ( order = Sheet Number )\n");
			fprintf( f, "%s", CONV_TO_UTF8(msg));
			PrintListeGLabel(f, ListOfLabels, NbItems);
		}

		if( (ItemsToList & LIST_HPINS_BY_NAME) )
		{
			qsort( ListOfLabels, NbItems, sizeof( ListLabel ),
				(int(*)(const void*, const void*))ListTriGLabelByVal);

			msg = _("\n#Glob labels ( order = Alphab. )\n");
			fprintf( f, "%s", CONV_TO_UTF8(msg));
			PrintListeGLabel(f, ListOfLabels, NbItems);
		}
		MyFree( ListOfLabels );
	}

	msg = _("\n#End List\n");
	fprintf( f, "%s", CONV_TO_UTF8(msg));
	fclose(f);
}



/****************************************/
int GenListeCmp( EDA_BaseStruct ** List )
/****************************************/
/* Routine de generation de la liste des elements utiles du dessin
	Si List == NULL: comptage des elements
	Sinon remplissage de la liste
	Initialise "FlagControlMulti" a SheetNumber pour la sortie des listes
	et m_Father comme pointeur sur la sheet d'appartenance
*/
{
int ii = 0;
EDA_BaseStruct *DrawList;
EDA_SchComponentStruct *DrawLibItem;
BASE_SCREEN * screen = ScreenSch;

	for( ; screen != NULL ; screen = (BASE_SCREEN*)screen->Pnext )
		{
		DrawList = screen->EEDrawList;
		while ( DrawList )
			{
			switch( DrawList->Type() )
				{

				case DRAW_LIB_ITEM_STRUCT_TYPE :
					ii++;
					DrawLibItem = (EDA_SchComponentStruct *) DrawList;
					DrawLibItem->m_FlagControlMulti = screen->m_SheetNumber;
					DrawLibItem->m_Parent = screen;
					if( List )
						{
						*List = DrawList; List++;
						}
					break;


				default: break;
				}
			DrawList = DrawList->Pnext;
			}
		}
	return ( ii );
}

/*********************************************/
static int GenListeGLabels( ListLabel * List )
/*********************************************/
/* Count the Glabels, or fill the list Listwith Glabel pointers 
	If List == NULL: Item count only
	Else fill list of Glabels
*/
{
int ii = 0;
EDA_BaseStruct *DrawList;
DrawSheetLabelStruct *SheetLabel;
BASE_SCREEN * screen = ScreenSch;

	for( ; screen != NULL ; screen = (BASE_SCREEN*)screen->Pnext )
	{
		DrawList = screen->EEDrawList;
		while ( DrawList )
		{
			switch( DrawList->Type() )
			{
				case DRAW_GLOBAL_LABEL_STRUCT_TYPE :
					if( List )
					{
						List->Type() = DRAW_TEXT_STRUCT_TYPE;
						List->m_SheetNumber = screen->m_SheetNumber;
						List->m_Label = DrawList; List++;
					}
					ii++;
					break;
				
				case DRAW_SHEET_STRUCT_TYPE :
				{
					#define Sheet ((DrawSheetStruct * ) DrawList)
					SheetLabel= Sheet->m_Label;
					while( SheetLabel != NULL )
					{
						if ( List )
						{
							List->Type() = DRAW_SHEETLABEL_STRUCT_TYPE;
							List->m_SheetNumber = screen->m_SheetNumber;
							List->m_Label = SheetLabel;
							List++;
						}
						ii++;
						SheetLabel = (DrawSheetLabelStruct*)(SheetLabel->Pnext);
					}
					break;
				}

				default: break;
			}
			DrawList = DrawList->Pnext;
		}
	}
	return ( ii );
}

/**********************************************************/
static int ListTriComposantByVal(EDA_SchComponentStruct **Objet1,
							EDA_SchComponentStruct **Objet2)
/**********************************************************/
 /* Routine de comparaison pour le tri du Tableau par qsort()
	Les composants sont tries
		par valeur
		si meme valeur: par reference
			si meme valeur: par numero d'unite

*/
{
int ii;
const wxString * Text1, *Text2;

	if( (*Objet1 == NULL) && (*Objet2 == NULL ) ) return(0);
	if( *Objet1 == NULL) return(-1);
	if( *Objet2 == NULL) return(1);

	Text1 = &(*Objet1)->m_Field[VALUE].m_Text;
	Text2 = &(*Objet2)->m_Field[VALUE].m_Text;
	ii = Text1->CmpNoCase(*Text2);

	if( ii == 0 )
	{
		Text1 = &(*Objet1)->m_Field[REFERENCE].m_Text;
		Text2 = &(*Objet2)->m_Field[REFERENCE].m_Text;
		ii = Text1->CmpNoCase(*Text2);
	}

	if ( ii == 0 )
	{
		ii = (*Objet1)->m_Multi - (*Objet2)->m_Multi;
	}

	return(ii);
}

/**********************************************************/
static int ListTriComposantByRef(EDA_SchComponentStruct **Objet1,
							EDA_SchComponentStruct **Objet2)
/**********************************************************/
 /* Routine de comparaison pour le tri du Tableau par qsort()
	Les composants sont tries
		par reference
		si meme referenece: par valeur
			si meme valeur: par numero d'unite

*/
{
int ii;
const wxString * Text1, *Text2;

	if( (*Objet1 == NULL) && (*Objet2 == NULL ) ) return(0);
	if( *Objet1 == NULL) return(-1);
	if( *Objet2 == NULL) return(1);

	Text1 = &(*Objet1)->m_Field[REFERENCE].m_Text;
	Text2 = &(*Objet2)->m_Field[REFERENCE].m_Text;
	ii = Text1->CmpNoCase(*Text2);

	if( ii == 0 )
	{
		Text1 = &(*Objet1)->m_Field[VALUE].m_Text;
		Text2 = &(*Objet2)->m_Field[VALUE].m_Text;
		ii = Text1->CmpNoCase(*Text2);
	}

	if ( ii == 0 )
	{
		ii = (*Objet1)->m_Multi - (*Objet2)->m_Multi;
	}

	return(ii);
}

/******************************************************************/
static int ListTriGLabelByVal(ListLabel *Objet1, ListLabel *Objet2)
/*******************************************************************/
/* Routine de comparaison pour le tri du Tableau par qsort()
	Les labels sont tries
		par comparaison ascii
		si meme valeur: par numero de sheet

*/
{
int ii;
const wxString * Text1, *Text2;

	if( Objet1->Type() == DRAW_SHEETLABEL_STRUCT_TYPE )
		Text1 = &((DrawSheetLabelStruct *)Objet1->m_Label)->m_Text;
	else
		Text1 = &((DrawTextStruct *)Objet1->m_Label)->m_Text;

	if( Objet2->Type() == DRAW_SHEETLABEL_STRUCT_TYPE )
		Text2 = &((DrawSheetLabelStruct *)Objet2->m_Label)->m_Text;
	else
		Text2 = &((DrawTextStruct *)Objet2->m_Label)->m_Text;
	ii = Text1->CmpNoCase(*Text2);

	if ( ii == 0 )
		{
		ii = Objet1->m_SheetNumber - Objet2->m_SheetNumber;
		}

	return(ii);
}

/*******************************************************************/
static int ListTriGLabelBySheet(ListLabel *Objet1, ListLabel *Objet2)
/*******************************************************************/
/* Routine de comparaison pour le tri du Tableau par qsort()
	Les labels sont tries
		par sheet number
		si meme valeur, par ordre alphabetique

*/
{
int ii;
const wxString * Text1, *Text2;

	ii = Objet1->m_SheetNumber - Objet2->m_SheetNumber;

	if ( ii == 0 )
	{
		if( Objet1->Type() == DRAW_SHEETLABEL_STRUCT_TYPE )
			Text1 = &((DrawSheetLabelStruct *)Objet1->m_Label)->m_Text;
		else
			Text1 = &((DrawTextStruct *)Objet1->m_Label)->m_Text;

		if( Objet2->Type() == DRAW_SHEETLABEL_STRUCT_TYPE )
			Text2 = &((DrawSheetLabelStruct *)Objet2->m_Label)->m_Text;
		else
			Text2 = &((DrawTextStruct *)Objet2->m_Label)->m_Text;
		ii = Text1->CmpNoCase(*Text2);
	}

	return(ii);
}



/**************************************************************/
static void DeleteSubCmp( EDA_BaseStruct ** List, int NbItems )
/**************************************************************/
/* Supprime les sous-composants, c'est a dire les descriptions redonnantes des
boitiers multiples
	La liste des composant doit etre triee par reference et par num d'unite
*/
{
int ii;
EDA_SchComponentStruct * LibItem;
const wxString * OldName = NULL;

	for( ii = 0; ii < NbItems ; ii++ )
	{
		LibItem = (EDA_SchComponentStruct *) List[ii];
		if ( LibItem == NULL ) continue;
		if( OldName )
		{
			if ( OldName->CmpNoCase( LibItem->m_Field[REFERENCE].m_Text ) == 0 )
			{
				List[ii] = NULL;
			}
		}
		OldName = &LibItem->m_Field[REFERENCE].m_Text;
	}
}


/**********************************************************************/
int PrintListeCmpByRef( FILE * f, EDA_BaseStruct ** List, int NbItems )
/**********************************************************************/
/* Impression de la liste des composants tries par reference
*/
{
int ii, Multi, Unit;
EDA_BaseStruct *DrawList;
EDA_SchComponentStruct *DrawLibItem;
EDA_LibComponentStruct *Entry;
char NameCmp[80];
wxString msg;
	
	msg = _("\n#Cmp ( order = Reference )");
	if ( (ItemsToList & LIST_SUBCMP) ) msg << _(" (with SubCmp)");
	fprintf( f, "%s\n", CONV_TO_UTF8(msg));

	for ( ii = 0; ii < NbItems; ii++ )
		{
		DrawList = List[ii];

		if( DrawList == NULL ) continue;
		if( DrawList->Type() != DRAW_LIB_ITEM_STRUCT_TYPE ) continue;

		DrawLibItem = (EDA_SchComponentStruct *) DrawList;
		if( DrawLibItem->m_Field[REFERENCE].m_Text[0] == '#' ) continue;

		Multi = 0; Unit = ' ';
		Entry = FindLibPart(DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT);
		if( Entry ) Multi = Entry->m_UnitCount;
		if( (Multi > 1 ) && ( ItemsToList & LIST_SUBCMP ) )
			 Unit = DrawLibItem->m_Multi + 'A' - 1;

		sprintf( NameCmp,"%s%c", CONV_TO_UTF8(DrawLibItem->m_Field[REFERENCE].m_Text),
			Unit);
		fprintf(f, "| %-10.10s %-12.12s",
					NameCmp,
					CONV_TO_UTF8(DrawLibItem->m_Field[VALUE].m_Text));

		if ( (ItemsToList & LIST_SUBCMP) )
			{
			BASE_SCREEN * screen = (BASE_SCREEN *)(DrawLibItem->m_Parent);
			wxString sheetname;
			if( screen->m_Parent )
				sheetname = ((DrawSheetStruct*)screen->m_Parent)->m_Field[VALUE].m_Text.GetData();
			else sheetname = _("Root");
			fprintf(f, "   (Sheet %.2d: \"%s\")", DrawLibItem->m_FlagControlMulti,
					CONV_TO_UTF8(sheetname));
			}

		fprintf(f,"\n");
		}
	msg = _("#End Cmp\n");
	fprintf(f, CONV_TO_UTF8(msg));
	return(0);
}

/***********************************************************************/
int PrintListeCmpByVal( FILE * f, EDA_BaseStruct ** List, int NbItems )
/**********************************************************************/
{
int ii, Multi;
wxChar Unit;
EDA_BaseStruct *DrawList;
EDA_SchComponentStruct *DrawLibItem;
EDA_LibComponentStruct *Entry;
wxString msg;
	
	msg = _("\n#Cmp ( order = Value )");
	if ( (ItemsToList & LIST_SUBCMP) ) msg <<  _(" (with SubCmp)");
	msg << wxT("\n");
	fprintf(f, CONV_TO_UTF8(msg));

	for ( ii = 0; ii < NbItems; ii++ )
		{
		DrawList = List[ii];

		if( DrawList == NULL ) continue;
		if( DrawList->Type() != DRAW_LIB_ITEM_STRUCT_TYPE ) continue;

		DrawLibItem = (EDA_SchComponentStruct *) DrawList;
		if( DrawLibItem->m_Field[REFERENCE].m_Text[0] == '#' ) continue;

		Multi = 0; Unit = ' ';
		Entry = FindLibPart(DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT);
		if( Entry ) Multi = Entry->m_UnitCount;
		if( (Multi > 1 ) && ( ItemsToList & LIST_SUBCMP ) )
			 Unit = DrawLibItem->m_Multi + 'A' - 1;
		msg = DrawLibItem->m_Field[REFERENCE].m_Text;
		msg.Append(Unit);

		fprintf(f, "| %-12.12s %-10.10s",
					CONV_TO_UTF8(DrawLibItem->m_Field[VALUE].m_Text),
					CONV_TO_UTF8(msg) );
		if ( (ItemsToList & LIST_SUBCMP) )
		{
			fprintf(f, "   (Sheet %.2d)", DrawLibItem->m_FlagControlMulti);
		}

		fprintf(f,"\n");
	}
	msg = _("#End Cmp\n");
	fprintf(f, CONV_TO_UTF8(msg));
	return(0);
}


/******************************************************************/
static int PrintListeGLabel( FILE *f, ListLabel *List, int NbItems)
/******************************************************************/
{
int ii, jj;
DrawGlobalLabelStruct *DrawTextItem;
DrawSheetLabelStruct * DrawSheetLabel;
ListLabel * LabelItem;
wxString msg;
	
	for ( ii = 0; ii < NbItems; ii++ )
	{
		LabelItem = & List[ii];

		switch( LabelItem->Type() )
		{
			case DRAW_GLOBAL_LABEL_STRUCT_TYPE :
				DrawTextItem = (DrawGlobalLabelStruct *)(LabelItem->m_Label);
				msg.Printf(
                        _("> %-28.28s Global        (Sheet %.2d) pos: %3.3f, %3.3f\n"),
							DrawTextItem->m_Text.GetData(),
							LabelItem->m_SheetNumber,
							(float)DrawTextItem->m_Pos.x / 1000,
							(float)DrawTextItem->m_Pos.y / 1000);
				
				fprintf(f, CONV_TO_UTF8(msg));
 				break;

			case DRAW_SHEETLABEL_STRUCT_TYPE :
			{
				DrawSheetLabel = (DrawSheetLabelStruct *) LabelItem->m_Label;
				jj = DrawSheetLabel->m_Shape;
				if ( jj < 0 ) jj = NET_TMAX; if ( jj > NET_TMAX ) jj = 4;
				wxString labtype = CONV_FROM_UTF8(SheetLabelType[jj]);
				msg.Printf(
                        _("> %-28.28s Sheet %-7.7s (Sheet %.2d) pos: %3.3f, %3.3f\n"),
							DrawSheetLabel->m_Text.GetData(),
							labtype.GetData(),
							LabelItem->m_SheetNumber,
							(float)DrawSheetLabel->m_Pos.x / 1000,
							(float)DrawSheetLabel->m_Pos.y / 1000);
				fprintf(f, CONV_TO_UTF8(msg));
			}
				break;

			default: break;
		}
	}
	msg = _("#End labels\n");
	fprintf(f, CONV_TO_UTF8(msg));
 	return(0);
}


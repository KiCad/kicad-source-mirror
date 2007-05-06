/*********************************************************************/
/*	EESchema - edition des librairies: Edition des champs ( Fields ) */
/*********************************************************************/

	/*	Fichier fieldedi.cpp	*/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

#include "wx/spinctrl.h"

static char *PanelText[] =
{
	"Ref",			/* Champ Reference of part, i.e. "IC21" */
	"Name",			/* Champ Value of part, i.e. "3.3K" */
	"Fld1",
	"Fld2",
	"Fld3",
	"Fld4",
	"Fld5",
	"Fld6",
	"Fld7",
	"Fld8",
	"Pcb",			/* Champ Name Module PCB, i.e. "16DIP300" */
	"Sheet"			/* Champ Name Schema component, i.e. "cnt16.sch" */
};


/* Routines locales */
static void MoveField(wxDC *DC, int flag);
static char * PrefixText(LibraryEntryStruct * LibEntry, int Unit);

/* Variables locales */

extern int CurrentUnit;
static wxPoint StartCursor;

/* Classe de la frame des propriétés d'un composant en librairie */

enum id_libedit {
	ID_LIBEDIT_NOTEBOOK = 3200,
	ID_PANEL_BASIC,
	ID_PANEL_ALIAS,
	ID_PANEL_REFERENCE,
	ID_PANEL_VALUE,
	ID_PANEL_FIELD1,
	ID_PANEL_FIELD2,
	ID_PANEL_FIELD3,
	ID_PANEL_FIELD4,
	ID_PANEL_FIELD5,
	ID_PANEL_FIELD6,
	ID_PANEL_FIELD7,
	ID_PANEL_FIELD8,
	ID_PANEL_MODULEPCB,
	ID_PANEL_SUBSCHEMATIC,
	ID_CLOSE_PART_PROPERTIES,
	ID_ACCEPT_PART_PROPERTIES,
	ID_ADD_ALIAS,
	ID_DELETE_ONE_ALIAS,
	ID_DELETE_ALL_ALIAS
};


	/************************************/
	/* class WinEDA_PartPropertiesFrame */
	/************************************/

class WinEDA_PartPropertiesFrame: public wxDialog
{
private:

	WinEDA_LibeditFrame * m_Parent;
	wxNotebook* m_NoteBook;
	wxListBox * PartAliasList;
	wxPanel * PanelBasic;
	wxPanel * PanelAlias;
	wxPanel * PanelDoc;
	wxPanel * PanelField[NUMBER_OF_FIELDS];

	wxCheckBox * AsConvertButt;

	wxCheckBox * ShowFieldText[NUMBER_OF_FIELDS];
	wxCheckBox * VorientFieldText[NUMBER_OF_FIELDS];
	wxCheckBox * ShowPinNumButt;
	wxCheckBox * ShowPinNameButt;
	wxCheckBox * PinsNameInsideButt;
	wxSpinCtrl * SelNumberOfUnits;
	wxSpinCtrl * m_SetSkew;

	WinEDA_GraphicTextCtrl * FieldTextCtrl[NUMBER_OF_FIELDS];
	WinEDA_PositionCtrl * FieldPosition[NUMBER_OF_FIELDS];
	int FieldFlags[NUMBER_OF_FIELDS];
	int FieldOrient[NUMBER_OF_FIELDS];
	WinEDA_EnterText * NewDoc;
	WinEDA_EnterText * NewDocfile;
	WinEDA_EnterText * NewKeywords;
	ListOfAliasStruct * AliasListCopy;


public:
	// Constructor and destructor
	WinEDA_PartPropertiesFrame(WinEDA_LibeditFrame *parent, wxPoint& pos);
	~WinEDA_PartPropertiesFrame(void)
		{
		if( AliasListCopy ) AliasListCopy->FreeList();
		};

private:
	void PartPropertiesAccept(wxCommandEvent& event);
	void DeleteAllAliasOfPart(void);
	void DeleteAliasOfPart(void);
	void AddAliasOfPart(void);
	bool ChangeNbUnitsPerPackage(int newUnit);
	bool SetUnsetConvert(void);

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(WinEDA_PartPropertiesFrame, wxDialog)
	EVT_BUTTON(ID_ACCEPT_PART_PROPERTIES, WinEDA_PartPropertiesFrame::PartPropertiesAccept)
	EVT_BUTTON(ID_CLOSE_PART_PROPERTIES, WinEDA_PartPropertiesFrame::Close)
	EVT_BUTTON(ID_ADD_ALIAS, WinEDA_PartPropertiesFrame::AddAliasOfPart)
	EVT_BUTTON(ID_DELETE_ONE_ALIAS, WinEDA_PartPropertiesFrame::DeleteAliasOfPart)
	EVT_BUTTON(ID_DELETE_ALL_ALIAS, WinEDA_PartPropertiesFrame::DeleteAllAliasOfPart)
END_EVENT_TABLE()


void InstallLibeditFrame(WinEDA_LibeditFrame * parent, wxPoint & pos)
{
	WinEDA_PartPropertiesFrame * frame = new WinEDA_PartPropertiesFrame(parent, pos);
	frame->ShowModal(); frame->Destroy();
}


WinEDA_PartPropertiesFrame::WinEDA_PartPropertiesFrame(WinEDA_LibeditFrame *parent, wxPoint& framepos):
		wxDialog(parent, -1, _("Componant properties"), framepos, wxSize(320, 300),
		wxDEFAULT_DIALOG_STYLE | FLOAT_ON_PARENT )
{
wxPoint pos;
wxLayoutConstraints* c;

	m_Parent = parent;

	SetAutoLayout(TRUE);

	Doc[0] = 0; Docfile[0] = 0; Keywords[0] = 0;
	AliasListCopy = NULL;

	for ( int ii = 0; ii < NUMBER_OF_FIELDS; ii++ )
		{
		FieldFlags[ii] = 0;
		}
	if ( CurrentLibEntry )
		{
		if ( ListAlias ) AliasListCopy = ListAlias->DupList();

		if( CurrentLibEntry->Doc )
			strncpy(Doc,CurrentLibEntry->Doc, 256);
		if( CurrentLibEntry->KeyWord )
			strncpy(Keywords,CurrentLibEntry->KeyWord, 256);
		if( CurrentLibEntry->DocFile )
			strncpy(Docfile,CurrentLibEntry->DocFile, 256);

		FieldFlags[REFERENCE] = CurrentLibEntry->m_Prefix.m_Attributs;
		FieldOrient[REFERENCE] = CurrentLibEntry->m_Prefix.m_Orient;

		FieldFlags[VALUE] = CurrentLibEntry->m_Name.m_Attributs;
		FieldOrient[VALUE] = CurrentLibEntry->m_Name.m_Orient;

		LibDrawField * Field = CurrentLibEntry->Fields;
		while ( Field )
			{
			FieldFlags[Field->m_FieldId] = Field->m_Attributs;
			FieldOrient[Field->m_FieldId] = Field->m_Orient;
			Field = (LibDrawField*)Field->Pnext;
			}
		}

	m_NoteBook = new wxNotebook(this, ID_LIBEDIT_NOTEBOOK);
	c = new wxLayoutConstraints;
	c->left.SameAs(this, wxLeft, 4);
	c->right.SameAs(this, wxRight, 4);
	c->top.SameAs(this, wxTop, 4);
	c->bottom.SameAs(this, wxBottom, 40);
	m_NoteBook->SetConstraints(c);
	m_NoteBook->SetAutoLayout(TRUE);

	/* Creation des boutons de commande */
	pos.x = 80; pos.y = 240;
	wxButton * Button = new wxButton(this, ID_CLOSE_PART_PROPERTIES,
						_("Close"), pos);
	c = new wxLayoutConstraints;
	c->left.SameAs(this, wxLeft, 20);
	c->height.AsIs();
	c->width.AsIs();
	c->bottom.SameAs(this, wxBottom, 5);
	Button->SetConstraints(c);

	pos.x += Button->GetDefaultSize().x + 10;
	Button = new wxButton(this, ID_ACCEPT_PART_PROPERTIES,
						_("Ok"), pos);
	c = new wxLayoutConstraints;
	c->right.SameAs(this, wxRight, 20);
	c->height.AsIs();
	c->width.AsIs();
	c->bottom.SameAs(this, wxBottom, 5);
	Button->SetConstraints(c);

	// Add panel Basic
	PanelBasic = new wxPanel(m_NoteBook, ID_PANEL_BASIC);
	c = new wxLayoutConstraints;
	c->left.SameAs(m_NoteBook, wxLeft);
	c->right.SameAs(m_NoteBook, wxRight);
	c->bottom.SameAs(m_NoteBook, wxBottom);
	PanelBasic->SetConstraints(c);
	m_NoteBook->AddPage(PanelBasic, _("Options"), TRUE);

	pos.x = 5; pos.y = 25;
	new wxStaticBox(PanelBasic, -1,_(" General : "), pos, wxSize(150, 120));

	pos.x = 10; pos.y += 22;
	AsConvertButt = new wxCheckBox(PanelBasic,-1, _("As Convert"), pos);
	if ( g_AsDeMorgan ) AsConvertButt->SetValue(TRUE);

	pos.y += 20;
	ShowPinNumButt = new  wxCheckBox(PanelBasic,-1, _("Show Pin Num"), pos);
	if ( CurrentLibEntry )
		{
		if ( CurrentLibEntry->DrawPinNum ) ShowPinNumButt->SetValue(TRUE);
		}
	else  ShowPinNumButt->SetValue(TRUE);

	pos.y += 20;
	ShowPinNameButt = new wxCheckBox(PanelBasic,-1, _("Show Pin Name"), pos);
	if ( CurrentLibEntry )
		{
		if( CurrentLibEntry->DrawPinName ) ShowPinNameButt->SetValue(TRUE);
		}
	else ShowPinNameButt->SetValue(TRUE);

	pos.y += 20;
	PinsNameInsideButt = new wxCheckBox(PanelBasic,-1, _("Pin Name Inside"), pos);
	if ( CurrentLibEntry )
		{
		if ( CurrentLibEntry->TextInside ) PinsNameInsideButt->SetValue(TRUE);
		}
	else PinsNameInsideButt->SetValue(TRUE);

	pos.y += 40;
	new wxStaticText(PanelBasic,-1,_("Number of Units:"), pos);
	pos.y += 15;
	wxString number;
	if ( CurrentLibEntry ) number.Printf("%d", CurrentLibEntry->NumOfUnits);
	else number = "1";
	SelNumberOfUnits = new wxSpinCtrl(PanelBasic,-1,number, pos,
				wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP,
				1, 16);

	pos.y -= 15; pos.x += 140;
	new wxStaticText(PanelBasic,-1,_("Skew:"), pos);
	pos.y += 15;
	if ( CurrentLibEntry && CurrentLibEntry->TextInside)
		number.Printf("%d", CurrentLibEntry->TextInside);
	else number = "40";
	m_SetSkew = new wxSpinCtrl(PanelBasic,-1,number, pos,
				wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP,
				1, 100);

	// Add Panel Documentation
	PanelDoc = new wxPanel(m_NoteBook, -1);
	c = new wxLayoutConstraints;
	c->left.SameAs(m_NoteBook, wxLeft);
	c->right.SameAs(m_NoteBook, wxRight);
	c->bottom.SameAs(m_NoteBook, wxBottom);
	PanelDoc->SetConstraints(c);
	m_NoteBook->AddPage(PanelDoc, _("Doc"), FALSE);
	pos.x = 5; pos.y = 40;
	NewDoc = new WinEDA_EnterText(PanelDoc,
				_("Doc:"), Doc,
				pos, wxSize(285,-1) );
	pos.y += 50;
	NewKeywoed = new WinEDA_EnterText(PanelDoc,
				_("Keywords:"), Keywords,
				pos, wxSize(285,-1) );
	pos.y += 50;
	NewDocFile = new WinEDA_EnterText(PanelDoc,
				_("DocFileName:"), Docfile,
				pos, wxSize(285,-1) );

	// Add Panel Alias List
	PanelAlias = new wxPanel(m_NoteBook, -1);
	c = new wxLayoutConstraints;
	c->left.SameAs(m_NoteBook, wxLeft);
	c->right.SameAs(m_NoteBook, wxRight);
	c->bottom.SameAs(m_NoteBook, wxBottom);
	PanelAlias->SetConstraints(c);
	m_NoteBook->AddPage(PanelAlias, _("Alias"), FALSE);

	pos.x = 200; pos.y = 70;
	new wxButton(PanelAlias, ID_ADD_ALIAS,
						_("Add"), pos);
	pos.y += Button->GetDefaultSize().y + 10;
	new wxButton(PanelAlias, ID_DELETE_ONE_ALIAS,
						_("Delete"), pos);
	pos.y += Button->GetDefaultSize().y + 10;
	new wxButton(PanelAlias, ID_DELETE_ALL_ALIAS,
						_("Delete All"), pos);

	pos.x = 5; pos.y = 30;
	PartAliasList = new wxListBox(PanelAlias,
							-1,
							pos, wxSize(160,170),
							0,NULL,
							wxLB_ALWAYS_SB|wxLB_SINGLE);
	wxStaticText * Msg = new wxStaticText(PanelAlias, -1, _("Alias"),
							wxPoint(pos.x,pos.y - 20) );
	Msg->SetForegroundColour(wxColour(200,0,0) );
	/* lecture des alias */
	ListOfAliasStruct * Alias = AliasListCopy;
	while( Alias )
		{
		PartAliasList->Append(Alias->m_Name.m_Text);
		Alias = Alias->Next;
		}

	// Add panel Fields
	for ( int ii = 0; ii < NUMBER_OF_FIELDS; ii++)
		{
		PanelField[ii] = new wxPanel(m_NoteBook, ID_PANEL_REFERENCE + ii);
		c = new wxLayoutConstraints;
		c->left.SameAs(m_NoteBook, wxLeft);
		c->right.SameAs(m_NoteBook, wxRight);
		c->bottom.SameAs(m_NoteBook, wxBottom);
		PanelField[ii]->SetConstraints(c);
		m_NoteBook->AddPage(PanelField[ii], PanelText[ii], FALSE);

		pos.x = 10; pos.y = 20;
		ShowFieldText[ii] = new wxCheckBox(PanelField[ii],-1,
				_("Show Text"), pos);
		if ( (FieldFlags[ii] & TEXT_NO_VISIBLE ) == 0 )
			ShowFieldText[ii]->SetValue(TRUE);

		pos.x += 150;
		VorientFieldText[ii] = new wxCheckBox(PanelField[ii],-1,
				_("Vertical"), pos);
		if ( FieldOrient[ii] ) VorientFieldText[ii]->SetValue(TRUE);

		pos.x = 10; pos.y += 50;
		wxPoint txtpos;
		switch ( ii )
			{
			case REFERENCE:
				if ( CurrentLibEntry )
					txtpos = CurrentLibEntry->m_Prefix.m_Pos;
				else txtpos = wxPoint(0,0);	  					  
				FieldTextCtrl[ii] = new WinEDA_GraphicTextCtrl( PanelField[ii],
							PanelText[ii],
							CurrentLibEntry ? CurrentLibEntry->m_Prefix.m_Text : (char*) "U",
							CurrentLibEntry ? CurrentLibEntry->m_Prefix.m_Size : 50,
							UnitMetric ,
							pos, 200, TRUE);
				FieldPosition[ii] = new WinEDA_PositionCtrl( PanelField[ii],
							txtpos,
							UnitMetric , wxPoint (pos.x + 150, pos.y + 32) );
				break;

			case VALUE:
				if ( CurrentLibEntry )
					txtpos = CurrentLibEntry->m_Name.m_Pos;
				else txtpos = wxPoint(0,0);	  					  
				FieldTextCtrl[ii] = new WinEDA_GraphicTextCtrl( PanelField[ii],
							PanelText[ii],
							CurrentLibEntry ? CurrentLibEntry->m_Name.m_Text : NULL,
							CurrentLibEntry ? CurrentLibEntry->m_Name.m_Size : 50,
							UnitMetric ,
							pos, 200, TRUE);
				FieldPosition[ii] = new WinEDA_PositionCtrl( PanelField[ii],
							txtpos,
							UnitMetric , wxPoint (pos.x + 150, pos.y + 32) );
				break;
			default:
				int fsize; char * ftext; wxPoint fpos;
				fsize = 50; ftext = NULL;
				LibDrawField * Field = NULL;
				fpos = wxPoint(0,0);
				//recherche du Field de FieldId correspondant, s'il existe
				if ( CurrentLibEntry )
					{
					Field = CurrentLibEntry->Fields;
					while ( Field )
						{
						if( Field->m_FieldId == ii )
							{
							fsize = Field->m_Size; ftext = Field->m_Text;
							fpos = Field->m_Pos;
							break;
							}
						Field = (LibDrawField*)Field->Pnext;
						}
					}

				FieldTextCtrl[ii] = new WinEDA_GraphicTextCtrl( PanelField[ii],
							PanelText[ii],
							ftext, fsize,
							UnitMetric ,
							pos, 200, TRUE);
				FieldPosition[ii] = new WinEDA_PositionCtrl( PanelField[ii],
							fpos,
							UnitMetric , wxPoint (pos.x + 150, pos.y + 32) );
				break;
			}
		}

	SetModal(TRUE);
}


/***************************************************************************/
/* WinEDA_PartPropertiesFrame::PartPropertiesAccept(wxCommandEvent& event) */
/***************************************************************************/

/* Met a jour les differents parametres pour le composant en cours d'édition
*/

void WinEDA_PartPropertiesFrame::PartPropertiesAccept(wxCommandEvent& event)
{
bool recreateTB = FALSE;

	if( CurrentLibEntry == NULL )
		{
		Close(); return;
		}

	m_Parent->CurrentScreen->SetModify();
	m_Parent->CurrentScreen->SetRefreshReq();

	if( ListAlias ) ListAlias->FreeList();
	if( AliasListCopy ) ListAlias = AliasListCopy->DupList();

	if( CurrentLibEntry->Doc && strcmp(Doc,CurrentLibEntry->Doc) )
		{
		free(CurrentLibEntry->Doc); CurrentLibEntry->Doc = NULL;
		}
	if( strlen(Doc) )CurrentLibEntry->Doc = strdup(Doc);

	if( CurrentLibEntry->KeyWord && strcmp(Keywords,CurrentLibEntry->KeyWord) )
		{
		free(CurrentLibEntry->KeyWord); CurrentLibEntry->KeyWord = NULL;
		}
	if( strlen(Keywords) ) CurrentLibEntry->KeyWord = strdup(Keywords);

	if( CurrentLibEntry->DocFile && strcmp(Docfile,CurrentLibEntry->DocFile) )
		{
		free(CurrentLibEntry->DocFile); CurrentLibEntry->DocFile = NULL;
		}
	if( strlen(Docfile) ) CurrentLibEntry->DocFile = strdup(Docfile);


	CurrentLibEntry->m_Prefix.SetText( (char*)FieldTextCtrl[REFERENCE]->GetText() );
	CurrentLibEntry->m_Name.SetText( (char*)FieldTextCtrl[VALUE]->GetText() );

	CurrentLibEntry->m_Prefix.m_Size = FieldTextCtrl[REFERENCE]->GetTextSize();
	CurrentLibEntry->m_Name.m_Size = FieldTextCtrl[VALUE]->GetTextSize();

	CurrentLibEntry->m_Prefix.m_Pos = FieldPosition[REFERENCE]->GetCoord();
	CurrentLibEntry->m_Name.m_Pos = FieldPosition[VALUE]->GetCoord();

	CurrentLibEntry->m_Prefix.m_Orient = VorientFieldText[REFERENCE]->GetValue() ? 1 : 0;
	CurrentLibEntry->m_Name.m_Orient = VorientFieldText[VALUE]->GetValue() ? 1 : 0;

	if ( ShowFieldText[REFERENCE]->GetValue() )
		CurrentLibEntry->m_Prefix.m_Attributs &= ~TEXT_NO_VISIBLE;
	else
		CurrentLibEntry->m_Prefix.m_Attributs |= TEXT_NO_VISIBLE;

	if ( ShowFieldText[VALUE]->GetValue() )
		CurrentLibEntry->m_Name.m_Attributs &= ~TEXT_NO_VISIBLE;
	else
		CurrentLibEntry->m_Name.m_Attributs |= TEXT_NO_VISIBLE;

	for ( int ii = FIELD1; ii < NUMBER_OF_FIELDS; ii++ )
		{
		LibDrawField * Field = CurrentLibEntry->Fields;
		LibDrawField * NextField, * previousField = NULL;
		while ( Field )
			{
			NextField = (LibDrawField*)Field->Pnext;
			if( Field->m_FieldId == ii )
				{
				Field->SetText((char*) FieldTextCtrl[ii]->GetText());
				Field->m_Size = FieldTextCtrl[ii]->GetTextSize();
				if ( ShowFieldText[ii]->GetValue() )
					Field->m_Attributs &= ~TEXT_NO_VISIBLE;
				else
					Field->m_Attributs |= TEXT_NO_VISIBLE;
				Field->m_Orient = VorientFieldText[ii]->GetValue() ? 1 : 0;
				Field->m_Pos = FieldPosition[Field->m_FieldId]->GetCoord();
				if( Field->m_Text == NULL )
					{
					delete Field;
					if ( previousField ) previousField->Pnext = NextField;
					else CurrentLibEntry->Fields = NextField;
					}
				break;
				}

			previousField = Field;
			Field = NextField;
			}
		if ( (Field == NULL) &&	strlen(FieldTextCtrl[ii]->GetText()) )
			{	// N'existe pas: a creer
			Field = new LibDrawField(ii);
			Field->SetText((char*) FieldTextCtrl[ii]->GetText());
			Field->m_Size = FieldTextCtrl[ii]->GetTextSize();
			Field->Pnext = CurrentLibEntry->Fields;
			if ( ShowFieldText[Field->m_FieldId]->GetValue() )
				Field->m_Attributs &= ~TEXT_NO_VISIBLE;
			else
				Field->m_Attributs |= TEXT_NO_VISIBLE;
			Field->m_Orient = VorientFieldText[Field->m_FieldId]->GetValue() ?1 : 0;
			Field->m_Pos = FieldPosition[Field->m_FieldId]->GetCoord();
			CurrentLibEntry->Fields = Field;
			}
		}


	int ii = SelNumberOfUnits->GetValue();
	if ( ChangeNbUnitsPerPackage(ii) ) recreateTB = TRUE;

	if ( AsConvertButt->GetValue() )
		{
		if ( ! g_AsDeMorgan )
			{
			g_AsDeMorgan = 1;
			if ( SetUnsetConvert() ) recreateTB = TRUE;
			}
		}
	else
		{
		if ( g_AsDeMorgan )
			{
			g_AsDeMorgan = 0;
			if ( SetUnsetConvert() ) recreateTB = TRUE;
			}
		}

	CurrentLibEntry->DrawPinNum = ShowPinNumButt->GetValue() ? 1 : 0;
	CurrentLibEntry->DrawPinName = ShowPinNameButt->GetValue() ? 1 : 0;

	if ( PinsNameInsideButt->GetValue() == FALSE)
		CurrentLibEntry->TextInside = 0;
	else
		CurrentLibEntry->TextInside = m_SetSkew->GetValue();

	if ( recreateTB) m_Parent->ReCreateHToolbar();

	Close();
}


	/***************************************************************/
	/* void WinEDA_PartPropertiesFrame::DeleteAllAliasOfPart(void) */
	/***************************************************************/

void WinEDA_PartPropertiesFrame::DeleteAllAliasOfPart(void)
{

	if(CurrentLibEntry && AliasListCopy )
		{
		if( confirmation(_("Ok to Delete Alias LIST")) == YES)
			{
			if( AliasListCopy ) AliasListCopy->FreeList();
			AliasListCopy = NULL;
			PartAliasList->Clear();
			}
		}

}

	/*********************************************************/
	/* void WinEDA_PartPropertiesFrame::AddAliasOfPart(void) */
	/*********************************************************/

void WinEDA_PartPropertiesFrame::AddAliasOfPart(void)
{
char Line[LINE_LEN];
int llen;
ListOfAliasStruct * NewAlias;

	if(CurrentLibEntry == NULL) return;

	*Line = 0;
	if( Get_Message(_("Alias:"),Line, this) != 0 ) return;

	ChangeSpaces(Line, '_');
	llen = strlen(Line) + 1;

	NewAlias = LocateAlias(AliasListCopy, Line);
	if ( NewAlias )
		{
		DisplayError(_("Already in use"), 10); return;
		}
	NewAlias = new ListOfAliasStruct(Line);
	NewAlias->Next = AliasListCopy;
	AliasListCopy = NewAlias;

	/* affichage des alias */
	PartAliasList->Clear();
	ListOfAliasStruct * Alias = AliasListCopy;
	while( Alias )
		{
		PartAliasList->Append(Alias->m_Name.m_Text);
		Alias = Alias->Next;
		}

}

	/********************************/
	/* void DeleteAliasOfPart(void) */
	/********************************/

void WinEDA_PartPropertiesFrame::DeleteAliasOfPart(void)
{
ListOfAliasStruct * Alias;
wxString aliasname;

	if(CurrentLibEntry == NULL) return;
	if ( AliasListCopy == NULL ) return;

	aliasname = PartAliasList->GetStringSelection();
	if ( aliasname == "" ) return;

	Alias = AliasListCopy;
	ListOfAliasStruct * Previous = NULL;
	while ( Alias )
		{
		if ( stricmp(aliasname.GetData(), Alias->m_Name.m_Text) == 0 )
			{
			if ( Previous ) Previous->Next = Alias->Next;
			else AliasListCopy = Alias->Next;
			delete Alias;
			break;
			}
		Previous = Alias;
		Alias = Alias->Next;
		}
	/* affichage des alias */
	PartAliasList->Clear();
	Alias = AliasListCopy;
	while( Alias )
		{
		PartAliasList->Append(Alias->m_Name.m_Text);
		Alias = Alias->Next;
		}

}


/************************************************************/
static void ExitMoveField(WinEDA_DrawFrame * frame, wxDC * DC)
/************************************************************/
{

	frame->CurrentScreen->ManageCurseur = NULL;
	frame->CurrentScreen->ForceCloseManageCurseur = NULL;
	if(CurrentDrawItem == NULL) return;

	wxPoint curpos;
	curpos = frame->CurrentScreen->Curseur;
	frame->CurrentScreen->Curseur = StartCursor;
	MoveField(DC, 1);
	frame->CurrentScreen->Curseur = curpos;
	CurrentDrawItem->m_Flags = 0;

	CurrentDrawItem = NULL;
}



	/*********************************************/
	/* static void StartMoveField(COMMAND * Cmd) */
	/*********************************************/
/* Initialise le deplacement d'un champ ( ref ou Name) */
void WinEDA_LibeditFrame::StartMoveField(wxDC * DC, LibDrawField *field)
{

	if( (CurrentLibEntry == NULL) || ( field == NULL ) ) return;
	CurrentDrawItem = field;
	CurrentDrawItem->m_Flags |= IS_MOVED;
	CurrentScreen->ManageCurseur = MoveField;
	CurrentScreen->ForceCloseManageCurseur = ExitMoveField;
	CurrentScreen->ManageCurseur(DC, 1);
	StartCursor = CurrentScreen->Curseur;
}

/*****************************************************************/
/* Routine d'affichage du texte 'Field' en cours de deplacement. */
/*	Routine normalement attachee au curseur						*/
/*****************************************************************/
static void MoveField(wxDC *DC, int flag)
{
int color;
LibDrawField *Field = (LibDrawField *)CurrentDrawItem;

	if( (CurrentLibEntry == NULL) || (Field == NULL) ) return;

	GRSetDrawMode(DC, XOR_MODE);

	switch (Field->m_FieldId)
		{
		case VALUE:
			color = ReturnLayerColor(LAYER_VALUEPART);
			break;

		case REFERENCE:
			color = ReturnLayerColor(LAYER_REFERENCEPART);
			break;

		default:
			color = ReturnLayerColor(LAYER_FIELDS);
			break;
		}

	if( Field->m_Attributs & TEXT_NO_VISIBLE ) color = DARKGRAY;
	if( flag >= 0 )
			Gr_E_texte(DC, Field->m_Pos.x, - Field->m_Pos.y,
					color, Field->m_Text,
					Field->m_Orient, Field->m_Size,
					GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER);

	Field->m_Pos.x = ActiveScreen->Curseur.x;
	Field->m_Pos.y = - ActiveScreen->Curseur.y;

	Gr_E_texte(DC, Field->m_Pos.x, - Field->m_Pos.y,
			color, Field->m_Text,
			Field->m_Orient, Field->m_Size,
			GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER);
}

	/********************************/
	/* static void PlaceField(void) */
	/********************************/

void WinEDA_LibeditFrame::PlaceField(wxDC * DC, LibDrawField *Field)
{
int color;

	if(Field == NULL ) return;

	GRSetDrawMode(DC, GR_DEFAULT_DRAWMODE);

	switch (Field->m_FieldId)
		{
		case REFERENCE:
			color = ReturnLayerColor(LAYER_REFERENCEPART);
			break;

		case VALUE:
			color = ReturnLayerColor(LAYER_VALUEPART);
			break;

		default:
			color = ReturnLayerColor(LAYER_FIELDS);
			break;
		}

	if( Field->m_Attributs & TEXT_NO_VISIBLE ) color = DARKGRAY;
	Gr_E_texte(DC, Field->m_Pos.x, - Field->m_Pos.y,
					color, Field->m_Text,
					Field->m_Orient, Field->m_Size,
					GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER);

	Field->m_Flags = 0;

	CurrentScreen->SetModify();
	CurrentScreen->ManageCurseur = NULL;
	CurrentScreen->ForceCloseManageCurseur = NULL;
	CurrentDrawItem = NULL;
}


	/****************************************/
	/* static void EditField(COMMAND * Cmd) */
	/****************************************/

void WinEDA_LibeditFrame::EditField(wxDC * DC, LibDrawField *Field)
{
char Text[LINE_LEN];
int color;
wxClientDC dc(DrawPanel);

	if( Field== NULL) return;

	switch (Field->m_FieldId)
		{
		case REFERENCE:
			color = ReturnLayerColor(LAYER_REFERENCEPART);
			break;

		case VALUE:
			color = ReturnLayerColor(LAYER_VALUEPART);
			break;

		default:
			color = ReturnLayerColor(LAYER_FIELDS);
			break;
		}

	*Text = 0;
	if( Field->m_Attributs & TEXT_NO_VISIBLE ) color = DARKGRAY;

	if (Field->m_Text) strcpy(Text,Field->m_Text);
	Get_Message(_("Text: "),Text, this);
	ChangeSpaces(Text, '_');

	GRSetDrawMode(&dc, XOR_MODE);
	Gr_E_texte(DC, Field->m_Pos.x, - Field->m_Pos.y,
					color, Field->m_Text,
					Field->m_Orient, Field->m_Size,
					GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER);

	Field->SetText(Text);

	if( Field->m_Flags == 0 ) GRSetDrawMode(&dc, GR_DEFAULT_DRAWMODE);

	Gr_E_texte(DC, Field->m_Pos.x, - Field->m_Pos.y,
					color, Field->m_Text,
					Field->m_Orient, Field->m_Size,
					GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER);

	CurrentScreen->SetModify();
}

/*******************************************************************/
void WinEDA_LibeditFrame::RotateField(wxDC * DC, LibDrawField *Field)
/*******************************************************************/
/*
 Routine de modification de l'orientation ( Horiz ou Vert. ) du champ.
	si un champ est en cours d'edition, modif de celui ci.
	sinon Modif du champ pointe par la souris
*/
{
int color;

	if( Field == NULL) return;

	CurrentScreen->SetModify();
	switch (Field->m_FieldId)
		{
		case REFERENCE:
			color = ReturnLayerColor(LAYER_REFERENCEPART);
			break;

		case VALUE:
			color = ReturnLayerColor(LAYER_VALUEPART);
			break;

		default:
			color = ReturnLayerColor(LAYER_FIELDS);
			break;
		}

	if( Field->m_Attributs & TEXT_NO_VISIBLE  ) color = DARKGRAY;

	GRSetDrawMode(DC, XOR_MODE);
	Gr_E_texte(DC, Field->m_Pos.x, - Field->m_Pos.y,
					color, Field->m_Text,
					Field->m_Orient, Field->m_Size,
					GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER);

	if( Field->m_Orient) Field->m_Orient = 0;
	else Field->m_Orient = 1;

	if( Field->m_Flags == 0 ) GRSetDrawMode(DC, GR_DEFAULT_DRAWMODE);

	Gr_E_texte(DC, Field->m_Pos.x, - Field->m_Pos.y,
					color, Field->m_Text,
					Field->m_Orient, Field->m_Size,
					GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER);
}


	/********************************************************/
	/* static int LocateField(LibraryEntryStruct *LibEntry) */
	/********************************************************/

/* Localise le champ (ref ou name) pointe par la souris
	 retourne:
		< 0: Pas de champ
		0: Ref
		1: Name(s)
		> 2 = Num Field
*/
LibDrawField * WinEDA_LibeditFrame::LocateField(LibraryEntryStruct *LibEntry)
{
int x0, y0, x1, y1;	/* Rectangle d'encadrement des textes a localiser */
int dx, dy;			/* Dimensions du texte */
LibDrawField *Field;

	/* Localisation du Nom */
	x0 = LibEntry->m_Name.m_Pos.x;
	y0 = - LibEntry->m_Name.m_Pos.y;
	dx = LibEntry->m_Name.m_Size * strlen(LibEntry->m_Name.m_Text),
	dy = LibEntry->m_Name.m_Size;
	if (LibEntry->m_Name.m_Orient) EXCHG(dx, dy);
	x0 -= dx/2; y0 -= dy/2;
	x1 = x0 + dx; y1 = y0 + dy;

	if( (CurrentScreen->Curseur.x >= x0) && ( CurrentScreen->Curseur.x <= x1) &&
		(CurrentScreen->Curseur.y >= y0) && ( CurrentScreen->Curseur.y <= y1) )
		return &LibEntry->m_Name;

	/* Localisation du Prefix */
	x0 = LibEntry->m_Prefix.m_Pos.x;
	y0 = - LibEntry->m_Prefix.m_Pos.y;
	dx = LibEntry->m_Prefix.m_Size * strlen(LibEntry->m_Prefix.m_Text),
	dy = LibEntry->m_Prefix.m_Size;
	if (LibEntry->m_Prefix.m_Orient) EXCHG(dx, dy);
	x0 -= dx/2; y0 -= dy/2;
	x1 = x0 + dx; y1 = y0 + dy;

	if( (CurrentScreen->Curseur.x >= x0) && ( CurrentScreen->Curseur.x <= x1) &&
		(CurrentScreen->Curseur.y >= y0) && ( CurrentScreen->Curseur.y <= y1) )
		return &LibEntry->m_Prefix;

	/* Localisation des autres fields */
	for (Field = LibEntry->Fields; Field != NULL;
						Field = (LibDrawField*)Field->Pnext)
		{
		if ( Field->m_Text == NULL) continue;
		x0 = Field->m_Pos.x; y0 = - Field->m_Pos.y;
		dx = Field->m_Size * strlen(Field->m_Text),
		dy = Field->m_Size;
		if (Field->m_Orient) EXCHG(dx, dy);
		x0 -= dx/2; y0 -= dy/2;
		x1 = x0 + dx; y1 = y0 + dy;
		if( (CurrentScreen->Curseur.x >= x0) && ( CurrentScreen->Curseur.x <= x1) &&
			(CurrentScreen->Curseur.y >= y0) && ( CurrentScreen->Curseur.y <= y1) )
			return(Field);
		}

	return NULL;
}

	/*********************************************************************/
	/* static char * PrefixText(LibraryEntryStruct * LibEntry, int Unit) */
	/*********************************************************************/

/* Calcule l'affichage complet du prefixe ( texte + '?' + ident unit )
	Retourne un pointeur sur le nouveau texte (allocation statique)
*/
static char * PrefixText(LibraryEntryStruct * LibEntry, int Unit)
{
static char Text[LINE_LEN];

	if (LibEntry->NumOfUnits > 1)
		sprintf(Text,"%s?%c",LibEntry->m_Prefix.m_Text,Unit + 'A' - 1);
	else sprintf(Text,"%s?",LibEntry->m_Prefix.m_Text);
	return( Text );
}



	/*****************************************************************/
	/* void WinEDA_PartPropertiesFrame::ChangeNbUnitsPerPackagevoid) */
	/*****************************************************************/

/* Routine de modification du nombre d'unites par package pour le
	composant courant;
*/
bool WinEDA_PartPropertiesFrame::ChangeNbUnitsPerPackage(int MaxUnit)
{
int OldNumUnits, ii, FlagDel = -1;
LibEDA_BaseStruct* DrawItem, * NextDrawItem;

	if( CurrentLibEntry == NULL ) return FALSE;

	/* Si pas de changement: termine */
	if ( CurrentLibEntry->NumOfUnits == MaxUnit ) return FALSE;

	OldNumUnits = CurrentLibEntry->NumOfUnits;
		if ( OldNumUnits < 1 ) OldNumUnits = 1;

	CurrentLibEntry->NumOfUnits = MaxUnit;


	/* Traitement des unites enlevees ou rajoutees */
	if(OldNumUnits > CurrentLibEntry->NumOfUnits )
		{
		DrawItem = CurrentLibEntry->Drawings;
		for ( ; DrawItem != NULL; DrawItem = NextDrawItem)
			{
			NextDrawItem = DrawItem->Pnext;
			if( DrawItem->m_Unit > MaxUnit )  /* Item a effacer */
				{
				if( FlagDel < 0 )
					{
					if( confirmation(_("Delete units")) == YES )
						{
						/* Si part selectee n'existe plus: selection 1ere unit */
						if( CurrentUnit > MaxUnit ) CurrentUnit = 1;
						FlagDel = 1;
						}
					else
						{
						FlagDel = 0;
						MaxUnit = OldNumUnits;
						CurrentLibEntry->NumOfUnits = MaxUnit;
						return FALSE;
						}
					}
				DeleteOneLibraryDrawStruct(NULL, CurrentLibEntry,
								DrawItem, 0);
				}
			}
		return TRUE;
		}

	if(OldNumUnits < CurrentLibEntry->NumOfUnits )
		{
		DrawItem = CurrentLibEntry->Drawings;
		for ( ; DrawItem != NULL; DrawItem = DrawItem->Pnext )
			{
			/* Duplication des items pour autres elements */
			if( DrawItem->m_Unit == 1 )
				{
				for ( ii = OldNumUnits +1; ii <= MaxUnit; ii ++ )
					{
					NextDrawItem = CopyDrawEntryStruct(DrawItem);
					NextDrawItem->Pnext = CurrentLibEntry->Drawings;
					CurrentLibEntry->Drawings = NextDrawItem;
					NextDrawItem->m_Unit = ii;
					}
				}
			}
		}
	return TRUE;
}


	/**********************************************************/
	/* void WinEDA_PartPropertiesFrame::SetUnsetConvert(void) */
	/**********************************************************/
/* crée ou efface (selon option AsConvert) les éléments
	de la représentation convertie d'un composant
*/
bool WinEDA_PartPropertiesFrame::SetUnsetConvert(void)
{
int FlagDel = 0;
LibEDA_BaseStruct* DrawItem = NULL, * NextDrawItem;

	if( g_AsDeMorgan )	/* Representation convertie a creer */
		{
		/* Traitement des elements a ajouter ( pins seulement ) */
		if( CurrentLibEntry ) DrawItem = CurrentLibEntry->Drawings;
		for ( ; DrawItem != NULL; DrawItem = DrawItem->Pnext )
			{
			/* Duplication des items pour autres elements */
			if( DrawItem->m_StructType != PIN_DRAW_TYPE ) continue;
			if( DrawItem->m_Convert == 1 )
				{
				if( FlagDel == 0 )
					{
					if( confirmation(_("Create pins for Convert items")) == YES )
						FlagDel = 1;
					else
						{
						if( confirmation(_("Part as \"De Morgan\" anymore")) == YES )
							return TRUE;

						g_AsDeMorgan = 0; return FALSE;
						}
					}
				NextDrawItem = CopyDrawEntryStruct(DrawItem);
				NextDrawItem->Pnext = CurrentLibEntry->Drawings;
				CurrentLibEntry->Drawings = NextDrawItem;
				NextDrawItem->m_Convert = 2;
				}
			}
		}

	else			   /* Representation convertie a supprimer */
		{
		/* Traitement des elements à supprimer */
		if( CurrentLibEntry ) DrawItem = CurrentLibEntry->Drawings;
		for ( ; DrawItem != NULL; DrawItem = NextDrawItem)
			{
			NextDrawItem = DrawItem->Pnext;
			if( DrawItem->m_Convert > 1 )  /* Item a effacer */
				{
				if( FlagDel == 0 )
					{
					if( confirmation(_("Delete Convert items")) == YES )
						{
						CurrentConvert = 1;
						FlagDel = 1;
						}
					else
						{
						g_AsDeMorgan = 1;
						return FALSE;
						}
					}
				ActiveScreen->SetModify();
				DeleteOneLibraryDrawStruct(NULL, CurrentLibEntry, DrawItem, 0);
				}
			}
		}
	return TRUE;
}


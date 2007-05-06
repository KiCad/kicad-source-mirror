	/*****************************************************************/
	/** options.cpp: options pour la visualisation des composants  **/
	/****************************************************************/

#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "cvpcb.h"
#include "protos.h"

enum {
	SET_OPTION = 8000,
	SET_EDGE_FORMAT,
	SET_TEXTE_FORMAT,
	PADFILL_OPT,
	PADNUM_OPT,
	EDGE_SELECT,
	TEXT_SELECT,
	ID_SAVE_CONFIG
	};


/********************************************/
/* Classes derivees pour la fenetre Options */
/********************************************/

class wxMyCheckBox: public wxCheckBox
{
private:
protected:
public:
	bool * BoolVar;

	// Constructor and destructor
	wxMyCheckBox(wxWindow *parent, int id, const wxString & Title,
				bool * RefVar, wxPoint& pos);
	~wxMyCheckBox(void) { };
};


	/************************************************************/
	/* classe derivee pour la fenetre de selection des options  */
	/*	   d'affichage du module                                */
	/************************************************************/

class wxOptionsBox: public wxDialog
{
private:
protected:
public:

	WinEDA_BasePcbFrame * m_Parent;
	wxMyCheckBox * IsShowPadFill;
	wxMyCheckBox * IsShowPadNum;
	wxRadioBox * EdgeRadioBox;
	wxRadioBox *TextRadioBox;

	// Constructor and destructor
	wxOptionsBox(WinEDA_BasePcbFrame * parent, wxPoint& pos);
	~wxOptionsBox(void);

	bool OnClose(void);
	void SetOptPadFill( wxCommandEvent& event);
	void SetOptPadNum( wxCommandEvent& event);
	void ReturnDisplayEdgeFormat(wxCommandEvent& event);
	void ReturnDisplayTexteFormat(wxCommandEvent& event);
	void SaveConfig(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(wxOptionsBox, wxDialog )
	EVT_CHECKBOX(PADFILL_OPT, wxOptionsBox::SetOptPadFill)
	EVT_CHECKBOX(PADNUM_OPT,  wxOptionsBox::SetOptPadNum)
	EVT_RADIOBOX(EDGE_SELECT, wxOptionsBox::ReturnDisplayEdgeFormat)
	EVT_RADIOBOX(TEXT_SELECT, wxOptionsBox::ReturnDisplayTexteFormat)
	EVT_BUTTON(ID_SAVE_CONFIG, wxOptionsBox::SaveConfig)
END_EVENT_TABLE()


/*********************************************************************/
void WinEDA_DisplayFrame::InstallOptionsDisplay(wxCommandEvent& event)
/*********************************************************************/
/* Creation de la fenetre d'options de la fenetre de visu */
{
wxPoint pos;

	GetPosition(&pos.x, &pos.y);
	pos.x += 10; if (pos.x < 0 ) pos.x = 0;
	pos.y += 50; if (pos.y < 0 ) pos.y = 0;

	wxOptionsBox * OptionWindow = new wxOptionsBox(this, pos);
	OptionWindow->ShowModal(); OptionWindow->Destroy();
}



	/********************************/
	/* Constructeur de wxMyCheckBox */
	/********************************/

wxMyCheckBox::wxMyCheckBox(wxWindow *parent, int id, const wxString & Title,
						bool * RefVar, wxPoint& pos):
			wxCheckBox(parent, id, Title, pos)
{
	BoolVar = RefVar;
	if( * BoolVar ) this->SetValue(TRUE);
	else this->SetValue(FALSE);
}


/******************************************************/
void wxOptionsBox::SetOptPadFill(wxCommandEvent& event)
/******************************************************/
{
	*IsShowPadFill->BoolVar == 0 ?
		* IsShowPadFill->BoolVar = 1 : * IsShowPadFill->BoolVar = 0;
	DisplayOpt.DisplayPadFill = m_Parent->m_DisplayPadFill = IsShowPadFill->BoolVar;
	m_Parent->ReDrawPanel();
}

/******************************************************/
void wxOptionsBox::SetOptPadNum(wxCommandEvent& event)
/******************************************************/
{
	*IsShowPadNum->BoolVar == 0 ?
		*IsShowPadNum->BoolVar = TRUE : *IsShowPadNum->BoolVar = FALSE;

    DisplayOpt.DisplayPadNum = m_Parent->m_DisplayPadNum = IsShowPadNum->BoolVar;
	m_Parent->ReDrawPanel();
}

	/********************************/
	/* Constructeur de wxOptionsBox */
	/********************************/

wxOptionsBox::wxOptionsBox(WinEDA_BasePcbFrame * parent, wxPoint& bpos):
			wxDialog(parent, -1,  _("Options"),	bpos, wxSize(220, 195),
					DIALOG_STYLE)
{
wxPoint pos;

	m_Parent = parent;

	SetFont(*g_DialogFont);

	pos.x = 100; pos.y = 15;
	new wxButton(this,	ID_SAVE_CONFIG,	 _("Save Cfg"), pos);

	pos.x = 10; pos.y = 10;
	IsShowPadFill = new wxMyCheckBox(this,
							PADFILL_OPT,
							 _("&Pad Fill"),
							&DisplayOpt.DisplayPadFill, pos);

	pos.y += 20;
	IsShowPadNum = new wxMyCheckBox(this,
							PADNUM_OPT,
							 _("Pad &Num"),
							&DisplayOpt.DisplayPadNum, pos);

	pos.y += 25;
wxString DrawOpt[] = { _("&Filaire"), _("&Filled"), _("&Sketch")};
	EdgeRadioBox = new wxRadioBox(this, EDGE_SELECT,
						 _("Edges:"),
						pos, wxSize(-1,-1),
						3,DrawOpt,1,wxRA_SPECIFY_COLS);

	EdgeRadioBox->SetSelection(DisplayOpt.DisplayModEdge);

	pos.x += 100;
	TextRadioBox = new wxRadioBox(this, TEXT_SELECT,
						 _("Texts:"),
						pos,wxSize(-1,-1),
						3, DrawOpt, 1,wxRA_SPECIFY_COLS);

	TextRadioBox->SetSelection(DisplayOpt.DisplayModText);

}

	/*****************************/
	/* Destructeur de OptionsBox */
	/*****************************/

wxOptionsBox::~wxOptionsBox(void)
{
}


	/**************************************/
	/* Fonctions de base de wxMyDialogBox */
	/**************************************/

/*******************************/
bool wxOptionsBox::OnClose(void)
/*******************************/
{
	Show(FALSE);
	return TRUE;
}

/****************************************************************/
void wxOptionsBox::ReturnDisplayEdgeFormat(wxCommandEvent& event)
/****************************************************************/
{
	DisplayOpt.DisplayModEdge = m_Parent->m_DisplayModEdge = EdgeRadioBox->GetSelection();
	m_Parent->ReDrawPanel();
}

/****************************************************************/
void wxOptionsBox::ReturnDisplayTexteFormat(wxCommandEvent& event)
/****************************************************************/
{
	DisplayOpt.DisplayModText = m_Parent->m_DisplayModText = TextRadioBox->GetSelection();
	m_Parent->ReDrawPanel();
}


/***************************************************/
void wxOptionsBox::SaveConfig(wxCommandEvent& event)
/***************************************************/
{
	Save_Config(this);
}


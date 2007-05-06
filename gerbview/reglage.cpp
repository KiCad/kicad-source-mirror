	/********************************************/
	/* PCBNEW - Gestion des Options et Reglages */
	/********************************************/

	/*	 Fichier reglage.cpp 	*/

/*
 Affichage et modifications des parametres de travail de PcbNew
 Parametres = dimensions des via, pistes, isolements, options...
*/


#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"

#include "protos.h"

/* Fonctions locales */

/* variables locales */

/***********/

enum {
	ID_SAVE_CFG = 1000,
	ID_EXIT_CFG
	};

/* Routines Locales */

	/*************************************************/
	/* classe derivee pour la frame de Configuration */
	/*************************************************/

class WinEDA_ConfigFrame: public wxDialog
{
private:

	WinEDA_GerberFrame * m_Parent;
	wxListBox * ListLibr;
	int LibModified;

	WinEDA_EnterText * TextDrillExt;
	WinEDA_EnterText * TextPhotoExt;
	WinEDA_EnterText * TextPenExt;

	// Constructor and destructor
public:
	WinEDA_ConfigFrame(WinEDA_GerberFrame *parent,const wxPoint& pos);
	~WinEDA_ConfigFrame(void) {};

private:
	void SaveCfg(wxCommandEvent & event);
	void OnQuit(wxCommandEvent & event);
	void OnCloseWindow(wxCloseEvent & event);

	DECLARE_EVENT_TABLE()

};
/* Construction de la table des evenements pour WinEDA_ConfigFrame */
BEGIN_EVENT_TABLE(WinEDA_ConfigFrame, wxDialog)
	EVT_BUTTON(ID_SAVE_CFG, WinEDA_ConfigFrame::SaveCfg)
	EVT_BUTTON(ID_EXIT_CFG, WinEDA_ConfigFrame::OnQuit)
	EVT_CLOSE(WinEDA_ConfigFrame::OnCloseWindow)
END_EVENT_TABLE()



	/*****************************************************************/
	/* void WinEDA_GerberFrame::InstallConfigFrame(const wxPoint & pos) */
	/*****************************************************************/

void WinEDA_GerberFrame::InstallConfigFrame(const wxPoint & pos)
{
WinEDA_ConfigFrame * CfgFrame = new WinEDA_ConfigFrame(this, pos);
	CfgFrame->ShowModal(); CfgFrame->Destroy();
}


	/************************************************************/
	/* Constructeur de WinEDA_ConfigFrame: la fenetre de config */
	/************************************************************/

WinEDA_ConfigFrame::WinEDA_ConfigFrame(WinEDA_GerberFrame *parent,
		const wxPoint& framepos):
		wxDialog(parent, -1, wxEmptyString, framepos, wxSize(300, 180),
		wxDEFAULT_DIALOG_STYLE|wxFRAME_FLOAT_ON_PARENT )
{
#define LEN_EXT 100
wxString title;

	m_Parent = parent;
	SetFont(* g_DialogFont);

	title = _("from ") + EDA_Appl->m_CurrentOptionFile;
	SetTitle(title);

	LibModified = FALSE;
	wxBoxSizer * MainBoxSizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(MainBoxSizer);
	wxBoxSizer * RightBoxSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer * LeftBoxSizer = new wxBoxSizer(wxVERTICAL);
	MainBoxSizer->Add(LeftBoxSizer, 0, wxGROW|wxALL, 5);
	MainBoxSizer->Add(RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
	
	/* Creation des boutons de commande */
	wxButton * Button = new wxButton(this,	ID_SAVE_CFG, _("Save Cfg"));
	Button->SetForegroundColour(*wxRED);
	RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	Button = new wxButton(this,	ID_EXIT_CFG, _("Exit"));
	Button->SetForegroundColour(*wxBLUE);
	RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	wxSize size;
	size.x = LEN_EXT; size.y = -1;
	TextDrillExt = new WinEDA_EnterText(this,
				_("Drill File Ext:"), g_DrillFilenameExt,
				LeftBoxSizer, size);

	TextPhotoExt = new WinEDA_EnterText(this,
				_("Gerber File Ext:"), g_PhotoFilenameExt,
				LeftBoxSizer, size);

	TextPenExt = new WinEDA_EnterText(this,
				_("DCode File Ext:"),  g_PenFilenameExt,
				LeftBoxSizer, size);
 
	GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
}


	/*****************************************************************/
	/* Fonctions de base de WinEDA_ConfigFrame: la fenetre de config */
	/*****************************************************************/

void WinEDA_ConfigFrame::OnCloseWindow(wxCloseEvent & event)
{
	g_DrillFilenameExt = TextDrillExt->GetValue();
	g_PhotoFilenameExt = TextPhotoExt->GetValue();
	g_PenFilenameExt = TextPenExt->GetValue();
	EndModal(0);
}

/******************************************************************/
void  WinEDA_ConfigFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
/******************************************************************/
{
    // true is to force the frame to close
    Close(true);
}


/******************************************************/
void WinEDA_ConfigFrame::SaveCfg(wxCommandEvent& event)
/******************************************************/
{
	m_Parent->Update_config();
}



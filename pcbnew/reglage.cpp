	/********************************************/
	/* PCBNEW - Gestion des Options et Reglages */
	/********************************************/

	/*	 Fichier reglage.cpp 	*/

/*
 Affichage et modifications des parametres de travail de PcbNew
 Parametres = dimensions des via, pistes, isolements, options...
*/


#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "pcbplot.h"

#include "protos.h"

enum {
	SAVE_CFG = 1000,
	DEL_LIB,
	ADD_LIB,
	INSERT_LIB,
	FORMAT_NETLIST
	};

/* Routines Locales */

	/*************************************************/
	/* classe derivee pour la frame de Configuration */
	/*************************************************/

class WinEDA_ConfigFrame: public wxDialog
{
public:

	WinEDA_PcbFrame * m_Parent;
	wxListBox * ListLibr;
	int LibModified;

	WinEDA_EnterText * m_TextLibDir;
	WinEDA_EnterText * m_TextHelpModulesFileName;

public:
	// Constructor and destructor
	WinEDA_ConfigFrame(WinEDA_PcbFrame *parent,const wxPoint& pos);
	~WinEDA_ConfigFrame(void) {};

private:
	void OnCloseWindow(wxCloseEvent & event);
	void SaveCfg(wxCommandEvent& event);
	void LibDelFct(wxCommandEvent& event);
	void LibInsertFct(wxCommandEvent& event);
	void SetNewOptions(void);

	DECLARE_EVENT_TABLE()

};
/* Construction de la table des evenements pour WinEDA_ConfigFrame */
BEGIN_EVENT_TABLE(WinEDA_ConfigFrame, wxDialog)
	EVT_BUTTON(SAVE_CFG, WinEDA_ConfigFrame::SaveCfg)
	EVT_BUTTON(DEL_LIB, WinEDA_ConfigFrame::LibDelFct)
	EVT_BUTTON(ADD_LIB, WinEDA_ConfigFrame::LibInsertFct)
	EVT_BUTTON(INSERT_LIB, WinEDA_ConfigFrame::LibInsertFct)
	EVT_CLOSE(WinEDA_ConfigFrame::OnCloseWindow)
END_EVENT_TABLE()



/*****************************************************************/
void WinEDA_PcbFrame::InstallConfigFrame(const wxPoint & pos)
/*****************************************************************/
{
WinEDA_ConfigFrame * CfgFrame = new WinEDA_ConfigFrame(this, pos);
	CfgFrame->ShowModal(); CfgFrame->Destroy();
}


	/************************************************************/
	/* Constructeur de WinEDA_ConfigFrame: la fenetre de config */
	/************************************************************/

#define X_SIZE 450
#define Y_SIZE 380
WinEDA_ConfigFrame::WinEDA_ConfigFrame(WinEDA_PcbFrame *parent,
		const wxPoint& framepos):
		wxDialog(parent, -1, "", framepos, wxSize(X_SIZE, Y_SIZE),
		DIALOG_STYLE )
{
wxPoint pos;
wxSize size;
wxString title;
wxButton * Button;
	
	m_Parent = parent;
	SetFont(*g_DialogFont);

	title = _("from ") + EDA_Appl->m_CurrentOptionFile;
	SetTitle(title);

	LibModified = FALSE;

	/* Creation des boutons de commande */
	pos.x = 10; pos.y = 5;
	Button = new wxButton(this, SAVE_CFG, _("Save Cfg"), pos);
	Button->SetForegroundColour(*wxRED);

	pos.x = 190;
	Button = new wxButton(this, DEL_LIB, _("Del"), pos);
	Button->SetForegroundColour(*wxRED);

	pos.x += Button->GetSize().x;
	Button = new wxButton(this, ADD_LIB, _("Add"), pos );
	Button->SetForegroundColour(wxColor(0,80,0));

	pos.x += Button->GetSize().x;
	Button = new wxButton(this, INSERT_LIB, _("Ins"), pos );
	Button->SetForegroundColour(*wxBLUE);

	pos.x = 190; pos.y += 35;
	wxStaticText * Msg = new wxStaticText(this, -1, _("Lib Modules:"), pos );
	pos.y += 15;
	ListLibr = new wxListBox(this,
							-1,
							pos, wxSize(X_SIZE - pos.x -10,190),
							0,NULL,
							wxLB_ALWAYS_SB|wxLB_SINGLE);
	Msg->SetForegroundColour(wxColour(200,0,0) );

	if ( g_LibName_List )
	{
		LibNameItem * libnamestruct = g_LibName_List->FirstLib;

		for ( ; libnamestruct != NULL; libnamestruct = libnamestruct->Pnext )
		{
			ListLibr->Append(libnamestruct->m_Name);
		}
	}


wxString text;
#define DELTA_VPOS 17
	size.x = 120; size.y = 90;
	pos.y = 100; pos.x = 10;
	new wxStaticBox(this, -1,_("Files ext:"), pos, size);

	pos.x += 5; pos.y += DELTA_VPOS;
	text = _("Board ext: ") + PcbExtBuffer;
	new wxStaticText(this, -1,text , pos);

	pos.x += 5; pos.y += DELTA_VPOS;
	text = _("Cmp ext: ") + NetCmpExtBuffer;
	new wxStaticText(this, -1,text , pos);

	pos.y += DELTA_VPOS;
	text = _("Lib ext: ") + LibExtBuffer;
	new wxStaticText(this, -1,text , pos);

	pos.y += DELTA_VPOS;
	text = _("Net ext: ") + NetExtBuffer;
	new wxStaticText(this, -1,text , pos);

	pos.x = 10; pos.y = 260;
	size.x = X_SIZE - pos.x - 10; size.y = -1;
	m_TextLibDir = new WinEDA_EnterText(this,
				_("Lib Modules Dir:"),  g_UserLibDirBuffer,
				pos, size);

	pos.y += m_TextLibDir->GetDimension().y + 25;
	wxString DocModuleFileName =
		EDA_Appl->m_EDA_CommonConfig->Read("module_doc_file", "pcbnew/footprints.pdf");
	m_TextHelpModulesFileName = new WinEDA_EnterText(this,
				_("Module Doc File:"),  DocModuleFileName,
				pos, size);

	size.x = X_SIZE;
	size.y = pos.y + m_TextHelpModulesFileName->GetDimension().y + 10;

	SetClientSize(size);
}


	/*****************************************************************/
	/* Fonctions de base de WinEDA_ConfigFrame: la fenetre de config */
	/*****************************************************************/

void WinEDA_ConfigFrame::OnCloseWindow(wxCloseEvent & event)
{
	SetNewOptions();
	EndModal(0);
}

/********************************************/
void WinEDA_ConfigFrame::SetNewOptions(void)
/********************************************/
{
	g_UserLibDirBuffer = m_TextLibDir->GetData();
	EDA_Appl->m_EDA_CommonConfig->Write("module_doc_file",
			m_TextHelpModulesFileName->GetData());
	SetRealLibraryPath("modules");
}


/******************************************************/
void WinEDA_ConfigFrame::SaveCfg(wxCommandEvent& event)
/******************************************************/
{
	SetNewOptions();
	m_Parent->Update_config(this);
}


/********************************************************/
void WinEDA_ConfigFrame::LibDelFct(wxCommandEvent& event)
/********************************************************/
{
int ii;
wxString LibNameSelected;

	ii = ListLibr->GetSelection();
	if ( ii < 0 ) return;

	LibNameSelected = ListLibr->GetStringSelection();

	g_LibName_List->RemoveLibName(LibNameSelected);

	ListLibr->Delete(ii);
	LibModified = TRUE;
}


/*************************************************************/
void WinEDA_ConfigFrame::LibInsertFct(wxCommandEvent& event)
/*************************************************************/
/* Insere ou ajoute une librairie a la liste existante:
	La nouvelle librairie est mise avant (insert) ou apres (ajout)
	la librairie sélectionnée
*/
{
int ii;
wxString fullfilename, ShortLibName;
wxString mask ="*";

	ii = ListLibr->GetSelection();
	if ( ii < 0 ) ii = 0;
	if( event.GetId() == ADD_LIB)
		{
		if( g_LibName_List->FirstLib != NULL ) ii ++;	/* Ajout apres selection */
		}

	SetNewOptions();
	mask += LibExtBuffer;
	g_RealLibDirBuffer.Replace("\\","/");

	fullfilename = EDA_FileSelector( _("library files:"),
				g_RealLibDirBuffer,			/* Chemin par defaut */
				"",					/* nom fichier par defaut */
				LibExtBuffer,		/* extension par defaut */
				mask,				/* Masque d'affichage */
				this,
				0,
				TRUE
				);

	if ( fullfilename == "" ) return;

	ShortLibName =
		MakeReducedFileName(fullfilename, g_RealLibDirBuffer, LibExtBuffer);

	//Addition ou insertion de la librarire
	LibModified = TRUE;
wxString SList[1];
	SList[0] =wxString(ShortLibName);
	if (  g_LibName_List->InsertLibName(SList[0], ii) )
		ListLibr->InsertItems(1,SList,ii);
	else DisplayError(this, _("Library exists! No Change"));
}


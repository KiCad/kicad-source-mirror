	/*****************************************************/
	/* Gestion de la configuration generale de EESCHEMA */
	/*****************************************************/
/* Gestion de la fenetre de selection des librarires actives, de leur chemin
et du format des netlistes générées
*/

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"
#include "netlist.h"

#include "id.h"


/* Fonctions Locales */

/***********/

enum {
	SAVE_CFG = 1000,
	DEL_LIB,
	ADD_LIB,
	INSERT_LIB,
	FORMAT_NETLIST
	};

/* Forward declarations */

/* Routines Locales */

	/*************************************************/
	/* classe derivee pour la frame de Configuration */
	/*************************************************/

class WinEDA_ConfigFrame: public wxDialog
{
private:
protected:
public:

	WinEDA_SchematicFrame * m_Parent;
	wxListBox * m_ListLibr;
	wxRadioBox *m_NetFormatBox;
	bool m_LibListChanged;
	WinEDA_EnterText * LibDirCtrl;

	// Constructor and destructor
	WinEDA_ConfigFrame(WinEDA_SchematicFrame *parent, const wxPoint& pos);
	~WinEDA_ConfigFrame(void) {};

	void OnCloseWindow(wxCloseEvent & event);

	void CreateListFormatsNetListes(const wxPoint & pos);

	void SaveCfg(wxCommandEvent& event);
	void LibDelFct(wxCommandEvent& event);
	void AddOrInsertLibrary(wxCommandEvent& event);
	void ReturnNetFormat(wxCommandEvent& event);
	void ChangeSetup(void);

	DECLARE_EVENT_TABLE()

};
/* Construction de la table des evenements pour WinEDA_ConfigFrame */
BEGIN_EVENT_TABLE(WinEDA_ConfigFrame, wxDialog)
	EVT_BUTTON(SAVE_CFG, WinEDA_ConfigFrame::SaveCfg)
	EVT_BUTTON(DEL_LIB, WinEDA_ConfigFrame::LibDelFct)
	EVT_BUTTON(ADD_LIB, WinEDA_ConfigFrame::AddOrInsertLibrary)
	EVT_BUTTON(INSERT_LIB, WinEDA_ConfigFrame::AddOrInsertLibrary)
	EVT_RADIOBOX(FORMAT_NETLIST, WinEDA_ConfigFrame::ReturnNetFormat)
	EVT_CLOSE(WinEDA_ConfigFrame::OnCloseWindow)
END_EVENT_TABLE()

/******************************************************************/
void WinEDA_SchematicFrame::InstallConfigFrame(const wxPoint & pos)
/******************************************************************/
{
	WinEDA_ConfigFrame * CfgFrame = new WinEDA_ConfigFrame(this, pos);
	CfgFrame->ShowModal(); CfgFrame->Destroy();
}


#define X_SIZE 500
#define Y_SIZE 360

/********************************************************************/
WinEDA_ConfigFrame::WinEDA_ConfigFrame(WinEDA_SchematicFrame *parent,
		const wxPoint& framepos):
		wxDialog(parent, -1, wxEmptyString, framepos, wxSize(X_SIZE, Y_SIZE),
		DIALOG_STYLE )
/*****************************************************************/
/* Constructeur de WinEDA_ConfigFrame: la fenetre de config des librairies
*/
{
wxPoint pos;
wxSize size;
int dimy;
wxString msg;
wxButton * Button;

	m_Parent = parent;
	m_LibListChanged = FALSE;
	SetFont(*g_DialogFont);

	msg = _("from ") + EDA_Appl->m_CurrentOptionFile;
	SetTitle(msg);

	/* Creation des boutons de commande */
	pos.x = 10; pos.y = 5;
	Button = new wxButton(this, SAVE_CFG, _("Save Cfg"), pos);
	Button->SetForegroundColour(*wxRED);

	pos.x = 230;
	Button = new wxButton(this, DEL_LIB, _("Del"), pos);
	Button->SetForegroundColour(*wxRED);

	pos.x += Button->GetSize().x;
	Button = new wxButton(this, ADD_LIB, _("Add"), pos );
	Button->SetForegroundColour(wxColor(0,80,0));

	pos.x += Button->GetSize().x;
	Button = new wxButton(this, INSERT_LIB, _("Ins"), pos );
	Button->SetForegroundColour(*wxBLUE);

	pos.x = 10; pos.y = 45;
	CreateListFormatsNetListes(pos);

	pos.x = 235; pos.y = 55;
	m_ListLibr = new wxListBox(this,
							-1, pos, wxSize(X_SIZE - pos.x -10,225),
							0,NULL,
							wxLB_ALWAYS_SB|wxLB_SINGLE);
	wxStaticText * Msg = new wxStaticText(this, -1, _("Libraries"),
							wxPoint(pos.x,  pos.y - 14) );
	Msg->SetForegroundColour(wxColour(200,0,0) );
	m_ListLibr->InsertItems(g_LibName_List, 0);

	// Affichage des extensions des différents fichiers:
	dimy = 17;
	pos.x = 10;
	pos.y = m_NetFormatBox->GetPosition().y + m_NetFormatBox->GetSize().y + 10;
	wxStaticBox * Box = new wxStaticBox(this, -1, _("Files ext:"), pos, wxSize(180,110) );
	pos.y += 20; pos.x += 10;
	msg = _("Cmp file Ext: ") + g_NetCmpExtBuffer;
	wxStaticText * text = new wxStaticText(this, -1, msg, pos);

	pos.y += dimy;
	msg = _("Net file Ext: ") + g_NetExtBuffer;
	text = new wxStaticText(this, -1, msg, pos);

	pos.y += dimy;
	msg = _("Library file Ext: ") + g_LibExtBuffer;
	text = new wxStaticText(this, -1, msg, pos);

	pos.y += dimy;
	msg = _("Symbol file Ext: ") + g_SymbolExtBuffer;
	text = new wxStaticText(this, -1, msg, pos);

	pos.y += dimy;
	msg = _("Schematic file Ext: ") + g_SchExtBuffer;
	text = new wxStaticText(this, -1, msg, pos);


	int posY = Box->GetPosition().y + Box->GetSize().y + 30;
	pos.x = 10; pos.y = MAX(310, posY);
	size.x = X_SIZE - pos.x -10; size.y = -1;
	LibDirCtrl = new WinEDA_EnterText(this,
				_("Library files path:"), g_UserLibDirBuffer,
				pos, size);
	pos.y += LibDirCtrl->GetDimension().y + 5;

	SetClientSize(wxSize(X_SIZE, pos.y) );
}


/***********************************************************/
void WinEDA_ConfigFrame::OnCloseWindow(wxCloseEvent & event)
/***********************************************************/
{
	ChangeSetup();
	if ( m_LibListChanged )
	{
		LoadLibraries(m_Parent);
		if ( m_Parent->m_Parent->ViewlibFrame )
			m_Parent->m_Parent->ViewlibFrame->ReCreateListLib();
	}
	EndModal(0);
}


/*******************************************/
void WinEDA_ConfigFrame::ChangeSetup(void)
/*******************************************/
{
	g_UserLibDirBuffer = LibDirCtrl->GetData();
	SetRealLibraryPath( wxT("library") );
}


/******************************************************/
void WinEDA_ConfigFrame::SaveCfg(wxCommandEvent& event)
/******************************************************/
{
	ChangeSetup();
	m_Parent->Save_Config(this);
}

/********************************************************/
void WinEDA_ConfigFrame::LibDelFct(wxCommandEvent& event)
/********************************************************/
{
int ii;

	ii = m_ListLibr->GetSelection();
	if ( ii < 0 ) return;

	g_LibName_List.RemoveAt(ii);
	m_ListLibr->Clear();
	m_ListLibr->InsertItems(g_LibName_List, 0);
	m_LibListChanged = TRUE;
}

/****************************************************************/
void WinEDA_ConfigFrame::AddOrInsertLibrary(wxCommandEvent& event)
/****************************************************************/
/* Insert or add a library to the existing library list:
	New library is put in list before (insert) or after (add)
	the selection
*/
{
int ii;
wxString FullLibName,ShortLibName, Mask;

	ii = m_ListLibr->GetSelection();
	if ( ii < 0 ) ii = 0;
	ChangeSetup();
	if( event.GetId() == ADD_LIB)
	{
		if( g_LibName_List.GetCount() != 0 ) ii ++;	/* Add after selection */
	}

	Mask = wxT("*") + g_LibExtBuffer;
	FullLibName = EDA_FileSelector( _("Library files:"),
				g_RealLibDirBuffer,		/* Chemin par defaut */
				wxEmptyString,					/* nom fichier par defaut */
				g_LibExtBuffer,		/* extension par defaut */
				Mask,				/* Masque d'affichage */
				this,
				wxFD_OPEN,
				TRUE
				);

	if ( FullLibName.IsEmpty() ) return;

	ShortLibName = MakeReducedFileName(FullLibName,g_RealLibDirBuffer,g_LibExtBuffer);

	//Add or insert new library name
	if (FindLibrary(ShortLibName) == NULL)
	{
		m_LibListChanged = TRUE;
		g_LibName_List.Insert(ShortLibName, ii);
		m_ListLibr->Clear();
		m_ListLibr->InsertItems(g_LibName_List, 0);
	}

	else DisplayError(this, _("Library already in use"));

}



/**************************************************************/
void WinEDA_ConfigFrame::ReturnNetFormat(wxCommandEvent& event)
/**************************************************************/
{
int ii;

	ii = m_NetFormatBox->GetSelection();
	if ( ii == 0 ) g_NetFormat = NET_TYPE_PCBNEW;
	if ( ii == 1 ) g_NetFormat = NET_TYPE_ORCADPCB2;
	else if ( ii == 2 ) g_NetFormat = NET_TYPE_CADSTAR;
	else if ( ii == 3 ) g_NetFormat = NET_TYPE_SPICE;
	else if ( g_NetFormat < NET_TYPE_CUSTOM1 ) g_NetFormat = NET_TYPE_CUSTOM1;
}


/***********************************************************************/
void WinEDA_ConfigFrame::CreateListFormatsNetListes(const wxPoint & pos)
/***********************************************************************/
/* Message de wxRadioBox de selection type netliste */

{
wxString Net_Select[] =
	{ wxT("&PcbNew"), wxT("&OrcadPcb2"), wxT("&CadStar"), wxT("&Spice"), wxT("Other")};

	m_NetFormatBox = new wxRadioBox(this, FORMAT_NETLIST,
						_("NetList Formats:"),
						pos, wxSize(-1,-1),
						5, Net_Select, 1, wxRA_SPECIFY_COLS);

	switch( g_NetFormat )
		{
		case NET_TYPE_NOT_INIT:
		case NET_TYPE_PCBNEW:
			m_NetFormatBox->SetSelection(0);
			break;

		case NET_TYPE_ORCADPCB2:
			m_NetFormatBox->SetSelection(1);
			break;

		case NET_TYPE_CADSTAR:
			m_NetFormatBox->SetSelection(2);
			break;

		case NET_TYPE_SPICE:
			m_NetFormatBox->SetSelection(3);
			break;

		default:
			m_NetFormatBox->SetSelection(4);
			break;
		}
}



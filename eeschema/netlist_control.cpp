	/**********************************/
	/* Dilaog box for netlist outputs */
	/**********************************/

#include "fctsys.h"

//#include "gr_basic.h"
#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "netlist.h"	/* Definitions generales liees au calcul de netliste */
#include "protos.h"


// ID for configuration:
#define CUSTOM_NETLIST_TITLE wxT("CustomNetlistTitle")
#define CUSTOM_NETLIST_COMMAND wxT("CustomNetlistCommand")

/* Routines locales */

/* Variable locales */

enum id_netlist {
    ID_CREATE_NETLIST = 1550,
	ID_CURRENT_FORMAT_IS_DEFAULT,
	ID_CLOSE_NETLIST,
	ID_RUN_SIMULATOR,
	ID_SETUP_PLUGIN,
	ID_NETLIST_NOTEBOOK
};

enum panel_netlist_index {
	PANELPCBNEW = 0,	// Create Netlist format Pcbnew
	PANELORCADPCB2,		// Create Netlis format OracdPcb2
	PANELCADSTAR,		// Create Netlis format OracdPcb2
	PANELSPICE,			// Create Netlis format Pspice
	PANELCUSTOMBASE		// Start auxiliary panels (custom netlists)
};

/* wxPanels for creating the NoteBook pages for each netlist format:
*/
class EDA_NoteBookPage: public wxPanel
{
public:
	int m_IdNetType;
	wxCheckBox * m_IsCurrentFormat;
	WinEDA_EnterText * m_CommandStringCtrl;
	WinEDA_EnterText * m_TitleStringCtrl;
	wxButton * m_ButtonCancel;
	wxBoxSizer * m_LeftBoxSizer;
	wxBoxSizer * m_RightBoxSizer;
	wxBoxSizer * m_LowBoxSizer;

	EDA_NoteBookPage(wxNotebook* parent, const wxString & title,
			int id_NetType, int idCheckBox, int idCreateFile);
	~EDA_NoteBookPage() {};
};


/*****************************************************************************/
EDA_NoteBookPage::EDA_NoteBookPage(wxNotebook* parent, const wxString & title,
		int id_NetType, int idCheckBox, int idCreateFile) :
		wxPanel(parent, -1 )
/*****************************************************************************/
/* Contructor to create a setup page for one netlist format.
	Used in Netlist format Dialog box creation
*/
{
	SetFont(*g_DialogFont);
	m_IdNetType = id_NetType;
	m_CommandStringCtrl = NULL;
	m_TitleStringCtrl = NULL;
	m_IsCurrentFormat = NULL;
	m_ButtonCancel = NULL;

	parent->AddPage(this, title, g_NetFormat == m_IdNetType);

	wxBoxSizer * MainBoxSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(MainBoxSizer);
	wxBoxSizer * UpperBoxSizer = new wxBoxSizer(wxHORIZONTAL);
	m_LowBoxSizer = new wxBoxSizer(wxVERTICAL);
	MainBoxSizer->Add(UpperBoxSizer, 0, wxGROW|wxALL, 5);
	MainBoxSizer->Add(m_LowBoxSizer, 0, wxGROW|wxALL, 5);

	m_LeftBoxSizer = new wxBoxSizer(wxVERTICAL);
	m_RightBoxSizer = new wxBoxSizer(wxVERTICAL);
	UpperBoxSizer->Add(m_LeftBoxSizer, 0, wxGROW|wxALL, 5);
	UpperBoxSizer->Add(m_RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

	if ( idCheckBox )
	{
		wxStaticText * text = new wxStaticText(this, -1, _("Options:"));
		m_LeftBoxSizer->Add(text, 0, wxGROW|wxALL, 5);

		m_IsCurrentFormat = new wxCheckBox(this, idCheckBox,
			_("Default format"));
		m_LeftBoxSizer->Add(m_IsCurrentFormat, 0, wxGROW|wxALL, 5);

		if ( g_NetFormat == m_IdNetType )
			m_IsCurrentFormat->SetValue(TRUE);

	}

	if ( idCreateFile )	// Create the 2 standard buttons: Create File ans Cancel
	{
		wxButton * Button;
		if ( idCreateFile == ID_SETUP_PLUGIN )
		{
			Button = new wxButton(this, idCreateFile,
								_("&Browse Plugin"));
		}
		else
		{
			Button = new wxButton(this, idCreateFile,
								_("&Netlist"));
		}
		Button->SetForegroundColour(*wxRED);
		m_RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

		m_ButtonCancel =
			Button = new wxButton(this,	ID_CLOSE_NETLIST,
							_("&Close"));
		Button->SetForegroundColour(*wxBLUE);
		m_RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);
	}
}

#define CUSTOMPANEL_COUNTMAX 8
/* Dialog frame for creating netlists */
class WinEDA_NetlistFrame: public wxDialog
{
public:
	WinEDA_SchematicFrame * m_Parent;
	wxNotebook* m_NoteBook;
	EDA_NoteBookPage * m_PanelNetType[4+CUSTOMPANEL_COUNTMAX];

	wxRadioBox * m_UseNetNamesInNetlist;

public:
	// Constructor and destructor
	WinEDA_NetlistFrame(WinEDA_SchematicFrame *parent, wxPoint& pos);
	~WinEDA_NetlistFrame() {};

private:
	void InstallCustomPages();
	void InstallPageSpice();
	void GenNetlist(wxCommandEvent& event);
	void RunSimulator(wxCommandEvent& event);
	void NetlistUpdateOpt();
	void NetlistExit(wxCommandEvent& event);
	void SelectNetlistType(wxCommandEvent& event);
	void SetupPlugin(wxCommandEvent& event);
	DECLARE_EVENT_TABLE()};

BEGIN_EVENT_TABLE(WinEDA_NetlistFrame, wxDialog)
	EVT_BUTTON(ID_CLOSE_NETLIST, WinEDA_NetlistFrame::NetlistExit)
	EVT_BUTTON(ID_CREATE_NETLIST, WinEDA_NetlistFrame::GenNetlist)
	EVT_BUTTON(ID_SETUP_PLUGIN, WinEDA_NetlistFrame::SetupPlugin)
	EVT_CHECKBOX(ID_CURRENT_FORMAT_IS_DEFAULT, WinEDA_NetlistFrame::SelectNetlistType)
	EVT_BUTTON(ID_RUN_SIMULATOR, WinEDA_NetlistFrame::RunSimulator)
END_EVENT_TABLE()


/****************************************************************/
void InstallNetlistFrame(WinEDA_SchematicFrame *parent, wxPoint & pos)
/****************************************************************/
/* Installator for the netlist generation dialog box
	*/
{
	WinEDA_NetlistFrame * frame = new WinEDA_NetlistFrame(parent, pos);
	frame->ShowModal(); frame->Destroy();
}

#define H_SIZE 370
#define V_SIZE 300

/*************************************************************************************/
WinEDA_NetlistFrame::WinEDA_NetlistFrame(WinEDA_SchematicFrame *parent, wxPoint& framepos):
		wxDialog(parent, -1, _("Netlist"), framepos, wxSize(H_SIZE, V_SIZE), DIALOG_STYLE)
//				DIALOG_STYLE|wxRESIZE_BORDER)
/*************************************************************************************/
/* Constructor for the netlist generation dialog box
*/
{
int ii;

	m_Parent = parent;
	SetFont(*g_DialogFont);
	if ( g_NetFormat == NET_TYPE_NOT_INIT )
			g_NetFormat = NET_TYPE_PCBNEW;

	for ( ii = 0; ii < PANELCUSTOMBASE+CUSTOMPANEL_COUNTMAX; ii ++ )
	{
		m_PanelNetType[ii] = NULL;
	}

	if ( (framepos.x == -1) && (framepos.x == -1) ) Centre();

    wxBoxSizer * GeneralBoxSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(GeneralBoxSizer);

	m_NoteBook = new wxNotebook(this, ID_NETLIST_NOTEBOOK,
   		wxDefaultPosition,wxSize(H_SIZE-6, V_SIZE - 28));
	m_NoteBook->SetFont(*g_DialogFont);
    GeneralBoxSizer->Add(m_NoteBook, 0, wxGROW|wxALL, 5);

	// Add panels

	// Add Panel FORMAT PCBNEW
	m_PanelNetType[PANELPCBNEW] = new EDA_NoteBookPage(m_NoteBook, wxT("Pcbnew"), NET_TYPE_PCBNEW,
		ID_CURRENT_FORMAT_IS_DEFAULT, ID_CREATE_NETLIST);

	// Add Panel FORMAT ORCADPCB2
	m_PanelNetType[PANELORCADPCB2] = new EDA_NoteBookPage(m_NoteBook, wxT("OrcadPCB2"), NET_TYPE_ORCADPCB2,
		ID_CURRENT_FORMAT_IS_DEFAULT, ID_CREATE_NETLIST);

	// Add Panel FORMAT CADSTAR
	m_PanelNetType[PANELCADSTAR] = new EDA_NoteBookPage(m_NoteBook, wxT("CadStar"), NET_TYPE_CADSTAR,
		ID_CURRENT_FORMAT_IS_DEFAULT, ID_CREATE_NETLIST);

	// Add Panel spice
	InstallPageSpice();
	
	// Add custom panels:
	InstallCustomPages();
	
	// Problem in wxMSV >= 2.7.1 : we must call GetSizer for  one notebook page
	// to have a proper sizer commutation of all pages 
	m_PanelNetType[PANELPCBNEW]->GetSizer()->Fit(this);
    m_PanelNetType[PANELPCBNEW]->GetSizer()->SetSizeHints(this);


	GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
}


/*************************************************/
void WinEDA_NetlistFrame::InstallPageSpice()
/*************************************************/
/* Create the spice page
*/
{
wxButton * Button;
EDA_NoteBookPage * page;
	
	page = m_PanelNetType[PANELSPICE] = new EDA_NoteBookPage(m_NoteBook, wxT("Spice"), NET_TYPE_SPICE, 0, 0);

	page->m_IsCurrentFormat = new wxCheckBox(page,ID_CURRENT_FORMAT_IS_DEFAULT,
					_("Default format"));
	page->m_IsCurrentFormat->SetValue( g_NetFormat == NET_TYPE_SPICE);
	page->m_LeftBoxSizer->Add(page->m_IsCurrentFormat, 0, wxGROW|wxALL, 5);
	
wxString netlist_opt[2] = { _("Use Net Names"), _("Use Net Numbers") };
	m_UseNetNamesInNetlist = new wxRadioBox(page,-1, _("Netlist Options:"),
			wxDefaultPosition, wxDefaultSize,
			2, netlist_opt, 1, wxRA_SPECIFY_COLS);
	if ( ! g_OptNetListUseNames ) m_UseNetNamesInNetlist->SetSelection(1);
	page->m_LeftBoxSizer->Add(m_UseNetNamesInNetlist, 0, wxGROW|wxALL, 5);

	page->m_CommandStringCtrl = new WinEDA_EnterText(page,
				_("Simulator command:"), g_SimulatorCommandLine,
				page->m_LowBoxSizer, wxSize(H_SIZE- 10, -1) );
	// Add buttons
	Button = new wxButton(page, ID_CREATE_NETLIST, _("Netlist") );
	Button->SetForegroundColour(*wxRED);
	page->m_RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	Button = new wxButton(page, ID_RUN_SIMULATOR, _("&Run Simulator"));
	Button->SetForegroundColour(wxColour(0,100,0));
	page->m_RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	Button = new wxButton(page, ID_CLOSE_NETLIST, _("&Close"));
	Button->SetForegroundColour(*wxBLUE);
	page->m_RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);
}

/*************************************************/
void WinEDA_NetlistFrame::InstallCustomPages()
/*************************************************/
/* create the pages for custom netlist format selection:
*/
{
int ii, CustomCount;
wxString title, previoustitle, msg;
EDA_NoteBookPage * CurrPage;

	CustomCount = CUSTOMPANEL_COUNTMAX;
	previoustitle = wxT("dummy_title");
	for ( ii = 0; ii < CustomCount; ii++ )
	{
		msg = CUSTOM_NETLIST_TITLE; msg << ii+1;
		title = m_Parent->m_Parent->m_EDA_Config->Read(msg);
		
		// Install the panel only if it is the first panel not initialised
		if ( (title.IsEmpty()) && ( previoustitle.IsEmpty() ) ) break;

		previoustitle = title;
		if ( title.IsEmpty() )
			CurrPage = m_PanelNetType[PANELCUSTOMBASE + ii] =
				new EDA_NoteBookPage(m_NoteBook,  _("Add Plugin"),
				NET_TYPE_CUSTOM1 + ii,
				ID_CURRENT_FORMAT_IS_DEFAULT , ID_SETUP_PLUGIN);
		else
			CurrPage = m_PanelNetType[PANELCUSTOMBASE + ii] =
				new EDA_NoteBookPage(m_NoteBook, title,
				NET_TYPE_CUSTOM1 + ii,
				ID_CURRENT_FORMAT_IS_DEFAULT , ID_CREATE_NETLIST);

		msg = CUSTOM_NETLIST_COMMAND; msg << ii+1;
		wxString Command = m_Parent->m_Parent->m_EDA_Config->Read(msg);
		CurrPage->m_CommandStringCtrl = new WinEDA_EnterText(CurrPage,
					_("Netlist command:"), Command,
					CurrPage->m_LowBoxSizer, wxSize(H_SIZE- 10, -1) );

		CurrPage->m_TitleStringCtrl = new WinEDA_EnterText(CurrPage,
					_("Title:"), title,
					CurrPage->m_LowBoxSizer, wxSize(H_SIZE- 10, -1) );
	}
}


/***********************************************************/
void WinEDA_NetlistFrame::SetupPlugin(wxCommandEvent& event)
/***********************************************************/
/* Browse the plugin files and set the m_CommandStringCtrl field
*/
{
wxString FullFileName, Mask, Path;
	Mask = wxT("*");
	Path =  EDA_Appl->m_BinDir;
	FullFileName = EDA_FileSelector( _("Plugin files:"),
					Path,		/* Chemin par defaut */
					FullFileName,		/* nom fichier par defaut */
					wxEmptyString,		  /* extension par defaut */
					Mask,				/* Masque d'affichage */
					this,
					wxFD_OPEN,
					TRUE
					);
	if ( FullFileName.IsEmpty() ) return;
		
EDA_NoteBookPage * CurrPage;
	CurrPage = (EDA_NoteBookPage *) m_NoteBook->GetCurrentPage();
	if ( CurrPage == NULL ) return;
		
	CurrPage->m_CommandStringCtrl->SetValue(FullFileName);
	
	/* Get a title for thgis page */
	wxString title = CurrPage->m_TitleStringCtrl->GetValue();
	if ( title.IsEmpty() )
		DisplayInfo(this, _("Now, you must choose a title for this netlist control page\nand close the dialog box"));
}


// Fonctions de positionnement des variables d'option
/*****************************************************************/
void WinEDA_NetlistFrame::SelectNetlistType(wxCommandEvent& event)
/*****************************************************************/
{
int ii;
EDA_NoteBookPage * CurrPage;

	for ( ii = 0; ii < PANELCUSTOMBASE+CUSTOMPANEL_COUNTMAX; ii++ )
		if ( m_PanelNetType[ii] )
			m_PanelNetType[ii]->m_IsCurrentFormat->SetValue(FALSE);

	CurrPage = (EDA_NoteBookPage *) m_NoteBook->GetCurrentPage();
	if ( CurrPage == NULL ) return;
		
	g_NetFormat = CurrPage->m_IdNetType;
	CurrPage->m_IsCurrentFormat->SetValue(TRUE);

}

/***********************************************/
void WinEDA_NetlistFrame::NetlistUpdateOpt()
/***********************************************/
{
int ii;
	
	g_SimulatorCommandLine =
		m_PanelNetType[PANELSPICE]->m_CommandStringCtrl->GetValue();
	g_NetFormat = NET_TYPE_PCBNEW;

	for ( ii = 0; ii < PANELCUSTOMBASE+CUSTOMPANEL_COUNTMAX; ii++ )
	{
		if ( m_PanelNetType[ii] == NULL ) break;
		if ( m_PanelNetType[ii]->m_IsCurrentFormat->GetValue() == TRUE )
			g_NetFormat = m_PanelNetType[ii]->m_IdNetType;
	}

	g_OptNetListUseNames = TRUE;	// Used for pspice, gnucap
	if ( m_UseNetNamesInNetlist->GetSelection() == 1 )
		g_OptNetListUseNames = FALSE;
}

/**********************************************************/
void WinEDA_NetlistFrame::GenNetlist(wxCommandEvent& event)
/**********************************************************/
{
wxString FullFileName, FileExt, Mask;
int netformat_tmp = g_NetFormat;

	NetlistUpdateOpt();

EDA_NoteBookPage * CurrPage;
	
	CurrPage = (EDA_NoteBookPage *) m_NoteBook->GetCurrentPage();
	g_NetFormat = CurrPage->m_IdNetType;
	
	/* Calcul du nom du fichier netlist */
	FullFileName = ScreenSch->m_FileName;

	switch ( g_NetFormat )
	{	
		case NET_TYPE_SPICE:
			FileExt = wxT(".cir");
			break;
		case NET_TYPE_CADSTAR:
			FileExt = wxT(".frp");
			break;
		default:
			FileExt = g_NetExtBuffer;
			break;
	}

	Mask = wxT("*") + FileExt + wxT("*");
	ChangeFileNameExt(FullFileName, FileExt);

	FullFileName = EDA_FileSelector( _("Netlist files:"),
					wxEmptyString,					/* Chemin par defaut */
					FullFileName,		/* nom fichier par defaut */
					FileExt,		  	/* extension par defaut */
					Mask,				/* Masque d'affichage */
					this,
					wxFD_SAVE,
					TRUE
					);
	if ( FullFileName.IsEmpty() ) return;

	m_Parent->MsgPanel->EraseMsgBox();

	ReAnnotatePowerSymbolsOnly();
	if( CheckAnnotate(m_Parent, 0) )
	{
		if( !IsOK( this, _("Must be Annotated, Continue ?"))  )
			return;
	}

	/* Cleanup the entire hierarchy */
	EDA_ScreenList ScreenList(NULL);
	for ( SCH_SCREEN * screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
	{
		bool ModifyWires;
		ModifyWires = screen->SchematicCleanUp(NULL);
		/* if wire list has changed, delete the Undo Redo list to avoid
		pointer problems with deleted data */
		if ( ModifyWires )
			screen->ClearUndoRedoList();
	}

	m_Parent->BuildNetListBase();
	if ( CurrPage->m_CommandStringCtrl)
		g_NetListerCommandLine = CurrPage->m_CommandStringCtrl->GetValue();
	else g_NetListerCommandLine.Empty();
		
	switch (g_NetFormat)
	{
		default:
			WriteNetList(m_Parent, FullFileName, TRUE);
			break;

		case NET_TYPE_CADSTAR:
		case NET_TYPE_ORCADPCB2:
			WriteNetList(m_Parent, FullFileName, FALSE);

		case NET_TYPE_SPICE:
			g_OptNetListUseNames = TRUE;	// Used for pspice, gnucap
			if ( m_UseNetNamesInNetlist->GetSelection() == 1 )
				g_OptNetListUseNames = FALSE;
			WriteNetList(m_Parent, FullFileName, g_OptNetListUseNames);
			break;
	}
	FreeTabNetList(g_TabObjNet, g_NbrObjNet );
	g_NetFormat = netformat_tmp;

	NetlistExit(event);
}

/***********************************************************/
void WinEDA_NetlistFrame::NetlistExit(wxCommandEvent& event)
/***********************************************************/
{
wxString msg, Command;
	
	NetlistUpdateOpt();
	// Update the new titles
	for ( int ii = 0; ii < CUSTOMPANEL_COUNTMAX; ii++ )
	{
		EDA_NoteBookPage * CurrPage = m_PanelNetType[ii + PANELCUSTOMBASE];
		if ( CurrPage == NULL ) break;
		msg = wxT("Custom"); msg << ii+1;
		if ( CurrPage->m_TitleStringCtrl )
		{
			wxString title = CurrPage->m_TitleStringCtrl->GetValue();
			if ( msg != title )	// Title has changed, Update config
			{
				msg = CUSTOM_NETLIST_TITLE; msg << ii+1;
				m_Parent->m_Parent->m_EDA_Config->Write(msg, title);
			}
		}
		
		if ( CurrPage->m_CommandStringCtrl )
		{
			Command = CurrPage->m_CommandStringCtrl->GetValue();
			msg = CUSTOM_NETLIST_COMMAND; msg << ii+1;
			m_Parent->m_Parent->m_EDA_Config->Write(msg, Command);
		}
	}
	Close();
}


/***********************************************************/
void WinEDA_NetlistFrame::RunSimulator(wxCommandEvent& event)
/***********************************************************/
{
wxString NetlistFullFileName, ExecFile, CommandLine;

	g_SimulatorCommandLine =
		m_PanelNetType[PANELSPICE]->m_CommandStringCtrl->GetValue();
	g_SimulatorCommandLine.Trim(FALSE);
	g_SimulatorCommandLine.Trim(TRUE);
	ExecFile = g_SimulatorCommandLine.BeforeFirst(' ');

	CommandLine = g_SimulatorCommandLine.AfterFirst(' ');

	/* Calcul du nom du fichier netlist */
	NetlistFullFileName = ScreenSch->m_FileName;
	ChangeFileNameExt(NetlistFullFileName, wxT(".cir"));
	AddDelimiterString(NetlistFullFileName);
	CommandLine += wxT(" ") + NetlistFullFileName;

	ExecuteFile(this, ExecFile, CommandLine);
}


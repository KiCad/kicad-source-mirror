///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_html_report_panel.h"

#include "dialog_export_netlist_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_EXPORT_NETLIST_BASE, DIALOG_SHIM )
	EVT_NOTEBOOK_PAGE_CHANGED( ID_CHANGE_NOTEBOOK_PAGE, DIALOG_EXPORT_NETLIST_BASE::_wxFB_OnNetlistTypeSelection )
	EVT_BUTTON( ID_ADD_PLUGIN, DIALOG_EXPORT_NETLIST_BASE::_wxFB_OnAddGenerator )
	EVT_BUTTON( ID_DEL_PLUGIN, DIALOG_EXPORT_NETLIST_BASE::_wxFB_OnDelGenerator )
END_EVENT_TABLE()

DIALOG_EXPORT_NETLIST_BASE::DIALOG_EXPORT_NETLIST_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextOutputPath = new wxStaticText( this, wxID_ANY, _("Output Path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOutputPath->Wrap( -1 );
	bSizer8->Add( m_staticTextOutputPath, 0, wxALL, 5 );

	m_outputPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputPath->SetMinSize( wxSize( 450,-1 ) );

	bSizer8->Add( m_outputPath, 0, wxALL, 5 );


	bUpperSizer->Add( bSizer8, 1, wxEXPAND, 5 );

	m_NoteBook = new wxNotebook( this, ID_CHANGE_NOTEBOOK_PAGE, wxDefaultPosition, wxDefaultSize, 0 );
	m_NoteBook->SetMinSize( wxSize( 540,-1 ) );


	bUpperSizer->Add( m_NoteBook, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerMsgPanel;
	bSizerMsgPanel = new wxBoxSizer( wxVERTICAL );

	m_MessagesBox = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bSizerMsgPanel->Add( m_MessagesBox, 1, wxEXPAND|wxTOP, 5 );


	bUpperSizer->Add( bSizerMsgPanel, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );

	m_buttonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAddGenerator = new wxButton( this, ID_ADD_PLUGIN, _("Add Exporter..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSizer->Add( m_buttonAddGenerator, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_buttonDelGenerator = new wxButton( this, ID_DEL_PLUGIN, _("Remove Exporter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSizer->Add( m_buttonDelGenerator, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton( this, wxID_OK );
	m_sdbSizer2->AddButton( m_sdbSizer2OK );
	m_sdbSizer2Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer2->AddButton( m_sdbSizer2Cancel );
	m_sdbSizer2->Realize();

	m_buttonSizer->Add( m_sdbSizer2, 1, wxEXPAND, 5 );


	bMainSizer->Add( m_buttonSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_EXPORT_NETLIST_BASE::~DIALOG_EXPORT_NETLIST_BASE()
{
}

BEGIN_EVENT_TABLE( NETLIST_DIALOG_ADD_GENERATOR_BASE, DIALOG_SHIM )
	EVT_BUTTON( wxID_BROWSE_PLUGINS, NETLIST_DIALOG_ADD_GENERATOR_BASE::_wxFB_OnBrowseGenerators )
END_EVENT_TABLE()

NETLIST_DIALOG_ADD_GENERATOR_BASE::NETLIST_DIALOG_ADD_GENERATOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerTop;
	bSizerTop = new wxBoxSizer( wxVERTICAL );

	m_staticTextName = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextName->Wrap( -1 );
	bSizerTop->Add( m_staticTextName, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_textCtrlName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerTop->Add( m_textCtrlName, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticTextCmd = new wxStaticText( this, wxID_ANY, _("Command line to run the exporter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCmd->Wrap( -1 );
	bSizerTop->Add( m_staticTextCmd, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_textCtrlCommand = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlCommand->SetMinSize( wxSize( 500,-1 ) );

	bSizerTop->Add( m_textCtrlCommand, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerTop, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_buttonGenerator = new wxButton( this, wxID_BROWSE_PLUGINS, _("Browse Scripts..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_buttonGenerator, 0, wxALL|wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerBottom->Add( m_sdbSizer, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerBottom, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );
}

NETLIST_DIALOG_ADD_GENERATOR_BASE::~NETLIST_DIALOG_ADD_GENERATOR_BASE()
{
}

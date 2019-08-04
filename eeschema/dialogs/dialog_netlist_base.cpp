///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_netlist_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( NETLIST_DIALOG_BASE, DIALOG_SHIM )
	EVT_NOTEBOOK_PAGE_CHANGED( ID_CHANGE_NOTEBOOK_PAGE, NETLIST_DIALOG_BASE::_wxFB_OnNetlistTypeSelection )
	EVT_BUTTON( ID_ADD_PLUGIN, NETLIST_DIALOG_BASE::_wxFB_OnAddGenerator )
	EVT_BUTTON( ID_DEL_PLUGIN, NETLIST_DIALOG_BASE::_wxFB_OnDelGenerator )
END_EVENT_TABLE()

NETLIST_DIALOG_BASE::NETLIST_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );
	
	m_NoteBook = new wxNotebook( this, ID_CHANGE_NOTEBOOK_PAGE, wxDefaultPosition, wxDefaultSize, 0 );
	m_NoteBook->SetMinSize( wxSize( 540,-1 ) );
	
	
	bUpperSizer->Add( m_NoteBook, 1, wxEXPAND | wxALL, 5 );
	
	
	bMainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_buttonSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonAddGenerator = new wxButton( this, ID_ADD_PLUGIN, _("Add Generator..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSizer->Add( m_buttonAddGenerator, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonDelGenerator = new wxButton( this, ID_DEL_PLUGIN, _("Remove Generator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSizer->Add( m_buttonDelGenerator, 0, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton( this, wxID_OK );
	m_sdbSizer2->AddButton( m_sdbSizer2OK );
	m_sdbSizer2Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer2->AddButton( m_sdbSizer2Cancel );
	m_sdbSizer2->Realize();
	
	m_buttonSizer->Add( m_sdbSizer2, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( m_buttonSizer, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	this->Centre( wxBOTH );
}

NETLIST_DIALOG_BASE::~NETLIST_DIALOG_BASE()
{
}

BEGIN_EVENT_TABLE( NETLIST_DIALOG_ADD_GENERATOR_BASE, DIALOG_SHIM )
	EVT_BUTTON( wxID_BROWSE_PLUGINS, NETLIST_DIALOG_ADD_GENERATOR_BASE::_wxFB_OnBrowseGenerators )
	EVT_BUTTON( wxID_CANCEL, NETLIST_DIALOG_ADD_GENERATOR_BASE::_wxFB_OnCancelClick )
	EVT_BUTTON( wxID_OK, NETLIST_DIALOG_ADD_GENERATOR_BASE::_wxFB_OnOKClick )
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
	
	m_staticTextCmd = new wxStaticText( this, wxID_ANY, _("Command line to run the generator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCmd->Wrap( -1 );
	bSizerTop->Add( m_staticTextCmd, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlCommand = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlCommand->SetMinSize( wxSize( 500,-1 ) );
	
	bSizerTop->Add( m_textCtrlCommand, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerMain->Add( bSizerTop, 1, wxEXPAND|wxALL, 5 );
	
	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonGenerator = new wxButton( this, wxID_BROWSE_PLUGINS, _("Browse Generators..."), wxDefaultPosition, wxDefaultSize, 0 );
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

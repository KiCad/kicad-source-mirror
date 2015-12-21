///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_get_component_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GET_COMPONENT_BASE::DIALOG_GET_COMPONENT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextName = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextName->Wrap( -1 );
	bSizerLeft->Add( m_staticTextName, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCmpNameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCmpNameCtrl->SetMaxLength( 0 ); 
	bSizerLeft->Add( m_textCmpNameCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextHistory = new wxStaticText( this, wxID_ANY, _("History list:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHistory->Wrap( -1 );
	bSizerLeft->Add( m_staticTextHistory, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_historyList = new wxListBox( this, ID_SEL_BY_LISTBOX, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_historyList->SetMinSize( wxSize( 200,100 ) );
	
	bSizerLeft->Add( m_historyList, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerUpper->Add( bSizerLeft, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_buttonKW = new wxButton( this, ID_ACCEPT_KEYWORD, _("Search by Keyword"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonKW, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonList = new wxButton( this, ID_LIST_ALL, _("List All"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonList, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonBrowse = new wxButton( this, ID_EXTRA_TOOL, _("Select by Browser"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonBrowse, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerUpper->Add( bSizerRight, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_historyList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::Accept ), NULL, this );
	m_buttonKW->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::Accept ), NULL, this );
	m_buttonList->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::Accept ), NULL, this );
	m_buttonBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::GetExtraSelection ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::OnCancel ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::Accept ), NULL, this );
}

DIALOG_GET_COMPONENT_BASE::~DIALOG_GET_COMPONENT_BASE()
{
	// Disconnect Events
	m_historyList->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::Accept ), NULL, this );
	m_buttonKW->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::Accept ), NULL, this );
	m_buttonList->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::Accept ), NULL, this );
	m_buttonBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::GetExtraSelection ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::OnCancel ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GET_COMPONENT_BASE::Accept ), NULL, this );
	
}

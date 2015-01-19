///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_list_selector_base.h"

///////////////////////////////////////////////////////////////////////////

EDA_LIST_DIALOG_BASE::EDA_LIST_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 400,400 ), wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	m_filterLabel = new wxStaticText( this, wxID_ANY, _("Filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filterLabel->Wrap( -1 );
	m_filterLabel->SetToolTip( _("Enter a string to filter items.\nOnly names containing this string will be listed") );
	
	bSizerMain->Add( m_filterLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_filterBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMain->Add( m_filterBox, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizerMain->Add( m_staticText2, 0, wxRIGHT|wxLEFT, 5 );
	
	m_listBox = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VRULES|wxALWAYS_SHOW_SB|wxVSCROLL );
	m_listBox->SetMinSize( wxSize( -1,200 ) );
	
	bSizerMain->Add( m_listBox, 3, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticTextMsg = new wxStaticText( this, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMsg->Wrap( -1 );
	bSizerMain->Add( m_staticTextMsg, 0, wxRIGHT|wxLEFT, 5 );
	
	m_messages = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_messages->SetMinSize( wxSize( -1,80 ) );
	
	bSizerMain->Add( m_messages, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( EDA_LIST_DIALOG_BASE::onClose ) );
	m_filterBox->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( EDA_LIST_DIALOG_BASE::textChangeInFilterBox ), NULL, this );
	m_listBox->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( EDA_LIST_DIALOG_BASE::onListItemActivated ), NULL, this );
	m_listBox->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( EDA_LIST_DIALOG_BASE::onListItemSelected ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_LIST_DIALOG_BASE::onCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_LIST_DIALOG_BASE::onOkClick ), NULL, this );
}

EDA_LIST_DIALOG_BASE::~EDA_LIST_DIALOG_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( EDA_LIST_DIALOG_BASE::onClose ) );
	m_filterBox->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( EDA_LIST_DIALOG_BASE::textChangeInFilterBox ), NULL, this );
	m_listBox->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( EDA_LIST_DIALOG_BASE::onListItemActivated ), NULL, this );
	m_listBox->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( EDA_LIST_DIALOG_BASE::onListItemSelected ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_LIST_DIALOG_BASE::onCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_LIST_DIALOG_BASE::onOkClick ), NULL, this );
	
}

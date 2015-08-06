///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 13 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_cleaning_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CLEANING_OPTIONS_BASE::DIALOG_CLEANING_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );
	
	m_cleanViasOpt = new wxCheckBox( this, wxID_ANY, _("&Delete redundant vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cleanViasOpt->SetToolTip( _("remove vias on pads with a through hole") );
	
	bSizerUpper->Add( m_cleanViasOpt, 0, wxALL, 5 );
	
	m_mergeSegmOpt = new wxCheckBox( this, wxID_ANY, _("&Merge overlapping segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mergeSegmOpt->SetToolTip( _("merge aligned track segments, and remove null segments") );
	
	bSizerUpper->Add( m_mergeSegmOpt, 0, wxALL, 5 );
	
	m_deleteUnconnectedOpt = new wxCheckBox( this, wxID_ANY, _("D&elete unconnected tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deleteUnconnectedOpt->SetToolTip( _("delete track segment having a dangling end") );
	
	bSizerUpper->Add( m_deleteUnconnectedOpt, 0, wxALL, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND|wxALL, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );
	
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
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CLEANING_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CLEANING_OPTIONS_BASE::OnOKClick ), NULL, this );
}

DIALOG_CLEANING_OPTIONS_BASE::~DIALOG_CLEANING_OPTIONS_BASE()
{
	// Disconnect Events
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CLEANING_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CLEANING_OPTIONS_BASE::OnOKClick ), NULL, this );
	
}

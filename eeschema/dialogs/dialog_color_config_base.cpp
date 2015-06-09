///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_color_config_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_COLOR_CONFIG_BASE::DIALOG_COLOR_CONFIG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_mainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	
	bmainSizer->Add( m_mainBoxSizer, 1, wxEXPAND, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bmainSizer->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bmainSizer->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( bmainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_sdbSizerApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COLOR_CONFIG_BASE::OnApplyClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COLOR_CONFIG_BASE::OnOkClick ), NULL, this );
}

DIALOG_COLOR_CONFIG_BASE::~DIALOG_COLOR_CONFIG_BASE()
{
	// Disconnect Events
	m_sdbSizerApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COLOR_CONFIG_BASE::OnApplyClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COLOR_CONFIG_BASE::OnOkClick ), NULL, this );
	
}

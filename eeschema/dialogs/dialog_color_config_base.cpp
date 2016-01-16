///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 28 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_color_config_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_COLOR_CONFIG_BASE::DIALOG_COLOR_CONFIG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_mainBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_pnlColors = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_mainBoxSizer->Add( m_pnlColors, 1, wxEXPAND | wxALL, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_mainBoxSizer->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	m_mainBoxSizer->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( m_mainBoxSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_COLOR_CONFIG_BASE::~DIALOG_COLOR_CONFIG_BASE()
{
}

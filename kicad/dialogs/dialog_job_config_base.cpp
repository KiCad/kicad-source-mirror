///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_job_config_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_JOB_CONFIG_BASE::DIALOG_JOB_CONFIG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, wxT("Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	m_mainSizer->Add( m_staticText1, 0, wxALL, 5 );

	m_jobOptionsPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_jobOptionsSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
	m_jobOptionsSizer->SetFlexibleDirection( wxBOTH );
	m_jobOptionsSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	m_jobOptionsPanel->SetSizer( m_jobOptionsSizer );
	m_jobOptionsPanel->Layout();
	m_jobOptionsSizer->Fit( m_jobOptionsPanel );
	m_mainSizer->Add( m_jobOptionsPanel, 1, wxEXPAND | wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1Save = new wxButton( this, wxID_SAVE );
	m_sdbSizer1->AddButton( m_sdbSizer1Save );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_mainSizer->Add( m_sdbSizer1, 0, wxBOTTOM|wxEXPAND, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_JOB_CONFIG_BASE::~DIALOG_JOB_CONFIG_BASE()
{
}

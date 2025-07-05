///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_startwizard_privacy_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_STARTWIZARD_PRIVACY_BASE::PANEL_STARTWIZARD_PRIVACY_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Updates") ), wxVERTICAL );

	m_stIntro = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Allow connections to the Internet to check for updated versions of KiCad and packages installed through the Plugin and Content Manager?"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stIntro->Wrap( 550 );
	sbSizer1->Add( m_stIntro, 0, wxALL|wxEXPAND, 5 );

	m_cbAutoUpdateKiCad = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Check for KiCad updates on startup"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_cbAutoUpdateKiCad, 0, wxALL, 5 );

	m_cbAutoUpdatePCM = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Check for package updates on startup"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_cbAutoUpdatePCM, 0, wxALL, 5 );


	bSizer8->Add( sbSizer1, 0, wxEXPAND, 5 );

	m_sizerDataCollection = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Data Collection") ), wxVERTICAL );

	m_stIntroDataCollection = new wxStaticText( m_sizerDataCollection->GetStaticBox(), wxID_ANY, _("KiCad can anonymously report crashes and other event data to the development team in order to help identify and fix bugs and improve performance.\n\nWhen data collection is enabled, KiCad will automatically send reports when crashes or other notable events occur. These reports contain technical details about the state of the KiCad software at the time of the report. No personally identifiable information (PII) is collected, and your design files such as schematics, PCBs, and libraries are never shared as part of this process."), wxDefaultPosition, wxDefaultSize, 0 );
	m_stIntroDataCollection->Wrap( 550 );
	m_sizerDataCollection->Add( m_stIntroDataCollection, 0, wxALL|wxEXPAND, 5 );

	m_cbDataCollection = new wxCheckBox( m_sizerDataCollection->GetStaticBox(), wxID_ANY, _("Enable anonymous data collection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerDataCollection->Add( m_cbDataCollection, 0, wxALL, 5 );


	bSizer8->Add( m_sizerDataCollection, 0, wxEXPAND|wxTOP, 5 );


	bPanelSizer->Add( bSizer8, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_STARTWIZARD_PRIVACY_BASE::~PANEL_STARTWIZARD_PRIVACY_BASE()
{
}

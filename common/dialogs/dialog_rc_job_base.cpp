///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_rc_job_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_RC_JOB_BASE::DIALOG_RC_JOB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textOutputPath = new wxStaticText( this, wxID_ANY, _("Output file:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizer1->Add( m_textOutputPath, 0, wxALIGN_CENTER, 5 );

	m_textCtrlOutputPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputPath->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlOutputPath, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticText18 = new wxStaticText( this, wxID_ANY, _("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer1->Add( m_staticText18, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceFormatChoices;
	m_choiceFormat = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFormatChoices, 0 );
	m_choiceFormat->SetSelection( 0 );
	fgSizer1->Add( m_choiceFormat, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText10 = new wxStaticText( this, wxID_ANY, _("Include:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizer1->Add( m_staticText10, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_cbViolationErrors = new wxCheckBox( this, wxID_ANY, _("Errors"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_cbViolationErrors, 0, wxALIGN_CENTER_VERTICAL, 10 );


	bSizer2->Add( 40, 0, 0, wxEXPAND, 5 );

	m_cbViolationWarnings = new wxCheckBox( this, wxID_ANY, _("Warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_cbViolationWarnings, 0, wxALIGN_CENTER_VERTICAL, 10 );


	fgSizer1->Add( bSizer2, 1, wxEXPAND|wxTOP|wxBOTTOM, 4 );


	bSizerMain->Add( fgSizer1, 1, wxALL|wxEXPAND, 10 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxVERTICAL );

	m_cbHaltOutput = new wxCheckBox( this, wxID_ANY, _("Report job failure when violations present"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_cbHaltOutput, 0, wxBOTTOM|wxLEFT, 5 );


	bSizerBottom->Add( 0, 3, 0, wxEXPAND, 5 );

	m_cbAllTrackViolations = new wxCheckBox( this, wxID_ANY, _("Report all errors for each track"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_cbAllTrackViolations, 0, wxBOTTOM|wxLEFT, 5 );


	bSizerBottom->Add( 0, 3, 0, wxEXPAND, 5 );

	m_cbSchParity = new wxCheckBox( this, wxID_ANY, _("Test for parity between PCB and schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_cbSchParity, 0, wxBOTTOM|wxLEFT, 5 );


	bSizerBottom->Add( 0, 3, 0, wxEXPAND, 5 );

	m_cbRefillZones = new wxCheckBox( this, wxID_ANY, _("Refill all zones before performing DRC"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_cbRefillZones, 0, wxBOTTOM|wxLEFT, 5 );


	bSizerMain->Add( bSizerBottom, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_choiceFormat->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RC_JOB_BASE::OnFormatChoice ), NULL, this );
}

DIALOG_RC_JOB_BASE::~DIALOG_RC_JOB_BASE()
{
	// Disconnect Events
	m_choiceFormat->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RC_JOB_BASE::OnFormatChoice ), NULL, this );

}

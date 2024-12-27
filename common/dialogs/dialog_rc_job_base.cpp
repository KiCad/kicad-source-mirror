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

	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	m_staticText9->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerMain->Add( m_staticText9, 0, wxALL, 5 );

	m_panel9 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText18 = new wxStaticText( m_panel9, wxID_ANY, _("Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer1->Add( m_staticText18, 0, wxALIGN_CENTER|wxALL, 5 );

	wxArrayString m_choiceFormatChoices;
	m_choiceFormat = new wxChoice( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFormatChoices, 0 );
	m_choiceFormat->SetSelection( 0 );
	fgSizer1->Add( m_choiceFormat, 0, wxALL, 5 );

	m_staticText10 = new wxStaticText( m_panel9, wxID_ANY, _("Violations"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizer1->Add( m_staticText10, 0, wxALIGN_CENTER|wxALL, 5 );

	m_panel6 = new wxPanel( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_cbViolationErrors = new wxCheckBox( m_panel6, wxID_ANY, _("Errors"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_cbViolationErrors, 0, wxALL, 5 );

	m_cbViolationWarnings = new wxCheckBox( m_panel6, wxID_ANY, _("Warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_cbViolationWarnings, 0, wxALL, 5 );


	m_panel6->SetSizer( bSizer2 );
	m_panel6->Layout();
	bSizer2->Fit( m_panel6 );
	fgSizer1->Add( m_panel6, 1, wxEXPAND | wxALL, 5 );

	m_textOutputPath = new wxStaticText( m_panel9, wxID_ANY, _("Output File"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizer1->Add( m_textOutputPath, 0, wxALIGN_CENTER|wxALL, 5 );

	m_textCtrlOutputPath = new wxTextCtrl( m_panel9, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputPath->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlOutputPath, 0, wxALL, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbHaltOutput = new wxCheckBox( m_panel9, wxID_ANY, _("Report job failure when violations present"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_cbHaltOutput, 1, wxALL, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbAllTrackViolations = new wxCheckBox( m_panel9, wxID_ANY, _("Report all errors for each track"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_cbAllTrackViolations, 0, wxALL, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbSchParity = new wxCheckBox( m_panel9, wxID_ANY, _("Test for parity between PCB and schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_cbSchParity, 0, wxALL, 5 );


	m_panel9->SetSizer( fgSizer1 );
	m_panel9->Layout();
	fgSizer1->Fit( m_panel9 );
	bSizerMain->Add( m_panel9, 1, wxEXPAND | wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_choiceFormat->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RC_JOB_BASE::OnFormatChoice ), NULL, this );
}

DIALOG_RC_JOB_BASE::~DIALOG_RC_JOB_BASE()
{
	// Disconnect Events
	m_choiceFormat->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RC_JOB_BASE::OnFormatChoice ), NULL, this );

}

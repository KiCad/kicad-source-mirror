///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_executecommand_job_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE::DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->AddGrowableRow( 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textCommand = new wxStaticText( this, wxID_ANY, _("Command:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textCommand->Wrap( -1 );
	fgSizer1->Add( m_textCommand, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textCtrlCommand = new wxTextCtrl( this, wxID_ANY, _("Enter the command line to run SPICE\nUsually '<path to SPICE binary> \"%I\"'\n%I will be replaced by the netlist filepath"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_textCtrlCommand->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_textCtrlCommand->SetMinSize( wxSize( 400,-1 ) );

	fgSizer1->Add( m_textCtrlCommand, 1, wxEXPAND|wxRIGHT, 5 );

	m_textOutputPath = new wxStaticText( this, wxID_ANY, _("Output path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizer1->Add( m_textOutputPath, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlOutputPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputPath->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlOutputPath, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	bSizerMain->Add( fgSizer1, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxVERTICAL );

	m_cbRecordOutput = new wxCheckBox( this, wxID_ANY, _("Record output messages"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_cbRecordOutput, 0, wxLEFT, 5 );

	m_cbIgnoreExitCode = new wxCheckBox( this, wxID_ANY, _("Ignore non-zero exit code"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_cbIgnoreExitCode, 0, wxLEFT|wxTOP, 5 );


	bSizerMain->Add( bSizerBottom, 0, wxALL|wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_cbRecordOutput->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE::OnRecordOutputClicked ), NULL, this );
}

DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE::~DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE()
{
	// Disconnect Events
	m_cbRecordOutput->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE::OnRecordOutputClicked ), NULL, this );

}

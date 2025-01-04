///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_copyfiles_job_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_COPYFILES_JOB_SETTINGS_BASE::DIALOG_COPYFILES_JOB_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textSource = new wxStaticText( this, wxID_ANY, _("Source:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSource->Wrap( -1 );
	fgSizer1->Add( m_textSource, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlSource = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlSource->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlSource, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_textDest = new wxStaticText( this, wxID_ANY, _("Destination:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textDest->Wrap( -1 );
	fgSizer1->Add( m_textDest, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlDest = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlDest->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlDest, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerMain->Add( fgSizer1, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxVERTICAL );

	m_cbGenerateError = new wxCheckBox( this, wxID_ANY, _("Generate error if no files copied"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_cbGenerateError, 0, wxLEFT, 5 );

	m_cbOverwrite = new wxCheckBox( this, wxID_ANY, _("Overwrite files in destination"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbOverwrite->SetValue(true);
	bSizerBottom->Add( m_cbOverwrite, 0, wxLEFT|wxTOP, 5 );


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
	m_cbGenerateError->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_COPYFILES_JOB_SETTINGS_BASE::OnRecordOutputClicked ), NULL, this );
}

DIALOG_COPYFILES_JOB_SETTINGS_BASE::~DIALOG_COPYFILES_JOB_SETTINGS_BASE()
{
	// Disconnect Events
	m_cbGenerateError->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_COPYFILES_JOB_SETTINGS_BASE::OnRecordOutputClicked ), NULL, this );

}

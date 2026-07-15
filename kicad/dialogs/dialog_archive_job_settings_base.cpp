///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_archive_job_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ARCHIVE_JOB_SETTINGS_BASE::DIALOG_ARCHIVE_JOB_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textDest = new wxStaticText( this, wxID_ANY, _("Archive file:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textDest->Wrap( -1 );
	fgSizer1->Add( m_textDest, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlDest = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlDest->SetToolTip( _("Text variables such as ${PROJECTNAME} are expanded. A relative path is resolved against the working directory.") );
	m_textCtrlDest->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlDest, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerMain->Add( fgSizer1, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxVERTICAL );

	m_cbIncludeExtras = new wxCheckBox( this, wxID_ANY, _("Include extra files (legacy and output files)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_cbIncludeExtras, 0, wxLEFT|wxTOP, 5 );


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
}

DIALOG_ARCHIVE_JOB_SETTINGS_BASE::~DIALOG_ARCHIVE_JOB_SETTINGS_BASE()
{
}

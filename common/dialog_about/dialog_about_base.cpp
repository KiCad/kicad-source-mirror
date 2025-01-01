///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_about_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ABOUT_BASE::DIALOG_ABOUT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerTitle;
	bSizerTitle = new wxBoxSizer( wxHORIZONTAL );


	bSizerTitle->Add( 0, 0, 1, wxEXPAND, 5 );

	m_bitmapApp = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerTitle->Add( m_bitmapApp, 1, wxALIGN_CENTER|wxALL, 5 );

	wxBoxSizer* b_apptitleSizer;
	b_apptitleSizer = new wxBoxSizer( wxVERTICAL );

	m_staticTextAppTitle = new wxStaticText( this, wxID_ANY, _("App Title"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticTextAppTitle->Wrap( -1 );
	m_staticTextAppTitle->SetFont( wxFont( 14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	b_apptitleSizer->Add( m_staticTextAppTitle, 0, wxALIGN_CENTER|wxALL, 5 );

	m_staticTextBuildVersion = new wxStaticText( this, wxID_ANY, _("Build Version Info"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticTextBuildVersion->Wrap( -1 );
	b_apptitleSizer->Add( m_staticTextBuildVersion, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextLibVersion = new wxStaticText( this, wxID_ANY, _("Lib Version Info"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticTextLibVersion->Wrap( -1 );
	b_apptitleSizer->Add( m_staticTextLibVersion, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxTOP, 5 );


	bSizerTitle->Add( b_apptitleSizer, 10, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );


	bSizer5->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btCopyVersionInfo = new wxButton( this, wxID_COPY, _("&Copy Version Info"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btCopyVersionInfo->SetToolTip( _("Copy KiCad version info to the clipboard") );

	bSizer5->Add( m_btCopyVersionInfo, 0, wxALL|wxEXPAND, 5 );

	m_btReportBug = new wxButton( this, wxID_COPY, _("&Report Bug"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btReportBug->SetToolTip( _("Report a problem with KiCad") );

	bSizer5->Add( m_btReportBug, 0, wxALL|wxEXPAND, 5 );

	m_btDonate = new wxButton( this, wxID_COPY, _("Donate"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btDonate->SetToolTip( _("Donate to KiCad") );

	bSizer5->Add( m_btDonate, 0, wxALL|wxEXPAND, 5 );


	bSizer5->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizerTitle->Add( bSizer5, 0, wxEXPAND, 10 );


	bSizerTitle->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerTitle, 0, wxEXPAND, 5 );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_notebook->SetMinSize( wxSize( 750,350 ) );


	bSizerMain->Add( m_notebook, 2, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_btOk = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );

	m_btOk->SetDefault();
	bSizerButtons->Add( m_btOk, 0, wxALL, 5 );


	bSizerMain->Add( bSizerButtons, 0, wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	// Connect Events
	m_btCopyVersionInfo->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ABOUT_BASE::onCopyVersionInfo ), NULL, this );
	m_btReportBug->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ABOUT_BASE::onReportBug ), NULL, this );
	m_btDonate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ABOUT_BASE::onDonateClick ), NULL, this );
	m_notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_ABOUT_BASE::OnNotebookPageChanged ), NULL, this );
}

DIALOG_ABOUT_BASE::~DIALOG_ABOUT_BASE()
{
	// Disconnect Events
	m_btCopyVersionInfo->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ABOUT_BASE::onCopyVersionInfo ), NULL, this );
	m_btReportBug->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ABOUT_BASE::onReportBug ), NULL, this );
	m_btDonate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ABOUT_BASE::onDonateClick ), NULL, this );
	m_notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_ABOUT_BASE::OnNotebookPageChanged ), NULL, this );

}

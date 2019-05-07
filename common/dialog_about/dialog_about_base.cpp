///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 28 2019)
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

	m_staticTextCopyright = new wxStaticText( this, wxID_ANY, _("Copyright Info"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticTextCopyright->Wrap( -1 );
	b_apptitleSizer->Add( m_staticTextCopyright, 0, wxALIGN_CENTER|wxALL, 1 );

	m_staticTextBuildVersion = new wxStaticText( this, wxID_ANY, _("Build Version Info"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticTextBuildVersion->Wrap( -1 );
	b_apptitleSizer->Add( m_staticTextBuildVersion, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextLibVersion = new wxStaticText( this, wxID_ANY, _("Lib Version Info"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticTextLibVersion->Wrap( -1 );
	b_apptitleSizer->Add( m_staticTextLibVersion, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxTOP, 5 );


	bSizerTitle->Add( b_apptitleSizer, 10, wxALL|wxEXPAND, 5 );


	bSizerTitle->Add( 0, 0, 2, wxEXPAND, 5 );


	bSizerMain->Add( bSizerTitle, 0, wxEXPAND, 5 );

	m_auiNotebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_SCROLL_BUTTONS|wxAUI_NB_TAB_FIXED_WIDTH );
	m_auiNotebook->SetMinSize( wxSize( 750,350 ) );


	bSizerMain->Add( m_auiNotebook, 2, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_btShowVersionInfo = new wxButton( this, wxID_COPY, _("&Show Version Info"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_btShowVersionInfo, 0, wxALL, 5 );

	m_btCopyVersionInfo = new wxButton( this, wxID_COPY, _("&Copy Version Info"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btCopyVersionInfo->SetToolTip( _("Copy KiCad version info to the clipboard") );

	bSizerButtons->Add( m_btCopyVersionInfo, 0, wxALL, 5 );

	m_btOk = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );

	m_btOk->SetDefault();
	bSizerButtons->Add( m_btOk, 0, wxALL, 5 );


	bSizerMain->Add( bSizerButtons, 0, wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	// Connect Events
	m_btShowVersionInfo->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ABOUT_BASE::onShowVersionInfo ), NULL, this );
	m_btCopyVersionInfo->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ABOUT_BASE::onCopyVersionInfo ), NULL, this );
}

DIALOG_ABOUT_BASE::~DIALOG_ABOUT_BASE()
{
	// Disconnect Events
	m_btShowVersionInfo->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ABOUT_BASE::onShowVersionInfo ), NULL, this );
	m_btCopyVersionInfo->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ABOUT_BASE::onCopyVersionInfo ), NULL, this );

}

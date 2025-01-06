///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_update_notice_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_UPDATE_NOTICE_BASE::DIALOG_UPDATE_NOTICE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 2, 10, 0 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_icon = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer4->Add( m_icon, 0, wxALL|wxALIGN_CENTER_VERTICAL, 10 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_messageLine1 = new wxStaticText( this, wxID_ANY, _("A new version of KiCad is available!"), wxDefaultPosition, wxDefaultSize, 0 );
	m_messageLine1->Wrap( -1 );
	m_messageLine1->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer4->Add( m_messageLine1, 0, wxALL, 5 );

	m_messageLine2 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_messageLine2->Wrap( -1 );
	bSizer4->Add( m_messageLine2, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	fgSizer4->Add( bSizer4, 1, wxEXPAND|wxRIGHT, 5 );


	fgSizer4->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizerMain->Add( fgSizer4, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_skipBtn = new wxButton( this, wxID_ANY, _("&Skip This Version"), wxDefaultPosition, wxDefaultSize, 0 );
	m_skipBtn->SetToolTip( _("Ignores the update notice for the announced new version. Additional update notes will be displayed for newer versions.") );

	bButtonSizer->Add( m_skipBtn, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );


	bButtonSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btnRemind = new wxButton( this, wxID_ANY, _("&Remind Me Later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnRemind->SetToolTip( _("Close this update notice. The update notice will be shown again when you relaunch KiCad.") );

	bButtonSizer->Add( m_btnRemind, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_btnDetailsPage = new wxButton( this, wxID_ANY, _("&View Details"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnDetailsPage->SetToolTip( _("Launch a web browser to the release announcements page.") );

	bButtonSizer->Add( m_btnDetailsPage, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );

	m_btnDownloadPage = new wxButton( this, wxID_ANY, _("Open &Downloads Page"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnDownloadPage->SetToolTip( _("Go to the platform appropriate downloads page.") );

	bButtonSizer->Add( m_btnDownloadPage, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );


	bSizerMain->Add( bButtonSizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_skipBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_NOTICE_BASE::OnSkipThisVersionClicked ), NULL, this );
	m_btnRemind->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_NOTICE_BASE::OnBtnRemindMeClicked ), NULL, this );
	m_btnDetailsPage->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_NOTICE_BASE::OnBtnDetailsPageClicked ), NULL, this );
	m_btnDownloadPage->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_NOTICE_BASE::OnBtnDownloadsPageClicked ), NULL, this );
}

DIALOG_UPDATE_NOTICE_BASE::~DIALOG_UPDATE_NOTICE_BASE()
{
	// Disconnect Events
	m_skipBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_NOTICE_BASE::OnSkipThisVersionClicked ), NULL, this );
	m_btnRemind->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_NOTICE_BASE::OnBtnRemindMeClicked ), NULL, this );
	m_btnDetailsPage->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_NOTICE_BASE::OnBtnDetailsPageClicked ), NULL, this );
	m_btnDownloadPage->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_NOTICE_BASE::OnBtnDownloadsPageClicked ), NULL, this );

}

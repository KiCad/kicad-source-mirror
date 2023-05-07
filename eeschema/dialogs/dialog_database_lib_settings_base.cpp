///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_infobar.h"

#include "dialog_database_lib_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DATABASE_LIB_SETTINGS_BASE::DIALOG_DATABASE_LIB_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	bmainSizer->Add( m_infoBar, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Connection") ), wxVERTICAL );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 0, 0 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rbDSN = new wxRadioButton( sbSizer4->GetStaticBox(), wxID_ANY, _("DSN:"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_rbDSN, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_txtDSN = new wxTextCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_txtDSN, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxALL|wxEXPAND, 5 );

	m_staticText2 = new wxStaticText( sbSizer4->GetStaticBox(), wxID_ANY, _("Username:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	gbSizer2->Add( m_staticText2, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_txtUser = new wxTextCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtUser->SetMinSize( wxSize( 150,-1 ) );

	gbSizer2->Add( m_txtUser, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_staticText3 = new wxStaticText( sbSizer4->GetStaticBox(), wxID_ANY, _("Password:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	gbSizer2->Add( m_staticText3, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_txtPassword = new wxTextCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtPassword->SetMinSize( wxSize( 150,-1 ) );

	gbSizer2->Add( m_txtPassword, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );


	gbSizer2->AddGrowableCol( 2 );

	sbSizer4->Add( gbSizer2, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_rbConnectionString = new wxRadioButton( sbSizer4->GetStaticBox(), wxID_ANY, _("Connection String:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_rbConnectionString, 0, wxALL, 5 );

	m_txtConnectionString = new wxTextCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtConnectionString->Enable( false );

	bSizer4->Add( m_txtConnectionString, 0, wxALL|wxEXPAND, 5 );


	sbSizer4->Add( bSizer4, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_btnTest = new wxButton( sbSizer4->GetStaticBox(), wxID_ANY, _("Test"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_btnTest, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_stConnectionTestStatus = new wxStaticText( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE );
	m_stConnectionTestStatus->Wrap( -1 );
	bSizer5->Add( m_stConnectionTestStatus, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	sbSizer4->Add( bSizer5, 0, wxALL|wxEXPAND, 5 );


	bupperSizer->Add( sbSizer4, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Libraries") ), wxHORIZONTAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_btnReloadConfig = new wxButton( sbSizer2->GetStaticBox(), wxID_ANY, _("Reload Configuration"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnReloadConfig->SetToolTip( _("Reload the database library configuration file") );

	bSizer6->Add( m_btnReloadConfig, 0, wxALL, 5 );

	m_stLibrariesStatus = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_stLibrariesStatus->Wrap( -1 );
	bSizer6->Add( m_stLibrariesStatus, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbSizer2->Add( bSizer6, 1, wxALL|wxEXPAND, 5 );


	bupperSizer->Add( sbSizer2, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Caching") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText5 = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("Cache size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	m_staticText5->SetToolTip( _("How many database queries to cache") );

	fgSizer1->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_spinCacheSize = new wxSpinCtrl( sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1024, 256 );
	m_spinCacheSize->SetToolTip( _("How many database queries to cache") );

	fgSizer1->Add( m_spinCacheSize, 0, wxALL, 5 );

	m_staticText6 = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("Cache timeout:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_staticText6->SetToolTip( _("Time in seconds that database queries will be cached for") );

	fgSizer1->Add( m_staticText6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_spinCacheTimeout = new wxSpinCtrl( sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 600, 10 );
	m_spinCacheTimeout->SetToolTip( _("Time in seconds that database queries will be cached for") );

	fgSizer1->Add( m_spinCacheTimeout, 0, wxALL, 5 );


	sbSizer3->Add( fgSizer1, 1, wxEXPAND, 5 );


	bupperSizer->Add( sbSizer3, 1, wxALL|wxEXPAND, 5 );


	bmainSizer->Add( bupperSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 6 );

	wxBoxSizer* m_buttonsSizer;
	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );


	m_buttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_buttonsSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	bmainSizer->Add( m_buttonsSizer, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( bmainSizer );
	this->Layout();

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnClose ) );
	m_rbDSN->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnDSNSelected ), NULL, this );
	m_rbConnectionString->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnConnectionStringSelected ), NULL, this );
	m_btnTest->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnBtnTest ), NULL, this );
	m_btnReloadConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnBtnReloadConfig ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnCloseClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnApplyClick ), NULL, this );
}

DIALOG_DATABASE_LIB_SETTINGS_BASE::~DIALOG_DATABASE_LIB_SETTINGS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnClose ) );
	m_rbDSN->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnDSNSelected ), NULL, this );
	m_rbConnectionString->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnConnectionStringSelected ), NULL, this );
	m_btnTest->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnBtnTest ), NULL, this );
	m_btnReloadConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnBtnReloadConfig ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnCloseClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DATABASE_LIB_SETTINGS_BASE::OnApplyClick ), NULL, this );

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_git_repository_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GIT_REPOSITORY_BASE::DIALOG_GIT_REPOSITORY_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Connection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizerMain->Add( m_staticText1, 0, wxLEFT|wxTOP, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	m_staticText3->Hide();

	fgSizer2->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_txtName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtName->Hide();

	fgSizer2->Add( m_txtName, 0, wxEXPAND|wxRIGHT, 5 );

	m_staticText4 = new wxStaticText( this, wxID_ANY, _("Location:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer2->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_txtURL = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_txtURL, 0, wxEXPAND|wxRIGHT, 5 );

	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Connection type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	m_staticText9->Hide();

	fgSizer2->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	wxString m_ConnTypeChoices[] = { _("HTTPS"), _("SSH"), _("Local") };
	int m_ConnTypeNChoices = sizeof( m_ConnTypeChoices ) / sizeof( wxString );
	m_ConnType = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ConnTypeNChoices, m_ConnTypeChoices, 0 );
	m_ConnType->SetSelection( 0 );
	m_ConnType->Hide();

	bSizer3->Add( m_ConnType, 0, wxRIGHT, 5 );


	bSizer3->Add( 0, 0, 0, wxEXPAND, 5 );


	fgSizer2->Add( bSizer3, 1, wxEXPAND, 5 );


	bSizerMain->Add( fgSizer2, 0, wxEXPAND, 5 );

	m_panelAuth = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_szAuth;
	m_szAuth = new wxBoxSizer( wxVERTICAL );

	m_staticText2 = new wxStaticText( m_panelAuth, wxID_ANY, _("Authentication"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_szAuth->Add( m_staticText2, 0, wxLEFT|wxTOP, 10 );

	m_staticline2 = new wxStaticLine( m_panelAuth, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_szAuth->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer* fgSizer21;
	fgSizer21 = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizer21->SetFlexibleDirection( wxBOTH );
	fgSizer21->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbCustom = new wxCheckBox( m_panelAuth, wxID_ANY, _("SSH private key:     "), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer21->Add( m_cbCustom, 0, wxALL, 5 );

	m_fpSSHKey = new wxFilePickerCtrl( m_panelAuth, wxID_ANY, wxEmptyString, _("Select SSH private key file"), _("*"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE|wxFLP_FILE_MUST_EXIST|wxFLP_OPEN );
	m_fpSSHKey->SetMinSize( wxSize( 250,-1 ) );

	fgSizer21->Add( m_fpSSHKey, 0, wxEXPAND|wxRIGHT, 5 );

	m_staticText11 = new wxStaticText( m_panelAuth, wxID_ANY, _("User name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer21->Add( m_staticText11, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_txtUsername = new wxTextCtrl( m_panelAuth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer21->Add( m_txtUsername, 0, wxEXPAND|wxRIGHT, 5 );

	m_labelPass1 = new wxStaticText( m_panelAuth, wxID_ANY, _("SSH key password:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelPass1->Wrap( -1 );
	fgSizer21->Add( m_labelPass1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_txtPassword = new wxTextCtrl( m_panelAuth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer21->Add( m_txtPassword, 0, wxEXPAND|wxRIGHT, 5 );


	m_szAuth->Add( fgSizer21, 1, wxBOTTOM|wxEXPAND, 5 );


	m_panelAuth->SetSizer( m_szAuth );
	m_panelAuth->Layout();
	m_szAuth->Fit( m_panelAuth );
	bSizerMain->Add( m_panelAuth, 0, wxEXPAND|wxTOP, 5 );


	bSizerMain->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizerHelp = new wxButton( this, wxID_HELP );
	m_sdbSizer->AddButton( m_sdbSizerHelp );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnClose ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnUpdateUI ) );
	m_txtURL->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnLocationExit ), NULL, this );
	m_ConnType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnSelectConnType ), NULL, this );
	m_cbCustom->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GIT_REPOSITORY_BASE::onCbCustom ), NULL, this );
	m_fpSSHKey->Connect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnFileUpdated ), NULL, this );
	m_sdbSizerHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnTestClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnOKClick ), NULL, this );
}

DIALOG_GIT_REPOSITORY_BASE::~DIALOG_GIT_REPOSITORY_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnClose ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnUpdateUI ) );
	m_txtURL->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnLocationExit ), NULL, this );
	m_ConnType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnSelectConnType ), NULL, this );
	m_cbCustom->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GIT_REPOSITORY_BASE::onCbCustom ), NULL, this );
	m_fpSSHKey->Disconnect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnFileUpdated ), NULL, this );
	m_sdbSizerHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnTestClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GIT_REPOSITORY_BASE::OnOKClick ), NULL, this );

}

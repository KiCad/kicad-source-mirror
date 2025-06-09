///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_git_repos_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GIT_REPOS_BASE::PANEL_GIT_REPOS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	m_enableGit = new wxCheckBox( this, wxID_ANY, _("Enable Git tracking"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_enableGit, 0, wxEXPAND|wxALL, 10 );

	m_gitSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpdate;
	bSizerUpdate = new wxBoxSizer( wxVERTICAL );

	m_staticText6 = new wxStaticText( this, wxID_ANY, _("Remote Tracking"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	bSizerUpdate->Add( m_staticText6, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerUpdate->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxGridBagSizer* gbUpdate;
	gbUpdate = new wxGridBagSizer( 4, 5 );
	gbUpdate->SetFlexibleDirection( wxBOTH );
	gbUpdate->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbUpdate->SetEmptyCellSize( wxSize( -1,2 ) );

	m_updateLabel = new wxStaticText( this, wxID_ANY, _("Update interval:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_updateLabel->Wrap( -1 );
	gbUpdate->Add( m_updateLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_updateInterval = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 60, 5 );
	m_updateInterval->SetToolTip( _("Number of minutes between remote update checks.  Zero disables automatic checks.") );

	gbUpdate->Add( m_updateInterval, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticText7 = new wxStaticText( this, wxID_ANY, _("minutes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	gbUpdate->Add( m_staticText7, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	gbUpdate->AddGrowableCol( 2 );

	bSizerUpdate->Add( gbUpdate, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	m_gitSizer->Add( bSizerUpdate, 0, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizerCommitData;
	bSizerCommitData = new wxBoxSizer( wxVERTICAL );

	m_staticText12 = new wxStaticText( this, wxID_ANY, _("Git Commit Data"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizerCommitData->Add( m_staticText12, 0, wxEXPAND|wxLEFT|wxTOP, 13 );

	m_staticline31 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerCommitData->Add( m_staticline31, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbDefault = new wxCheckBox( this, wxID_ANY, _("Use default values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbDefault->SetValue(true);
	fgSizer1->Add( m_cbDefault, 0, wxALL, 5 );


	fgSizer1->Add( 0, 0, 0, wxEXPAND, 5 );

	m_authorLabel = new wxStaticText( this, wxID_ANY, _("Author name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_authorLabel->Wrap( -1 );
	m_authorLabel->Enable( false );

	fgSizer1->Add( m_authorLabel, 0, wxTOP|wxLEFT, 5 );

	m_author = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_author->Enable( false );

	fgSizer1->Add( m_author, 1, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_authorEmailLabel = new wxStaticText( this, wxID_ANY, _("Author e-mail:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_authorEmailLabel->Wrap( -1 );
	m_authorEmailLabel->Enable( false );

	fgSizer1->Add( m_authorEmailLabel, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_authorEmail = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_authorEmail->Enable( false );

	fgSizer1->Add( m_authorEmail, 1, wxALL, 5 );


	bSizerCommitData->Add( fgSizer1, 0, wxEXPAND|wxALL, 5 );


	m_gitSizer->Add( bSizerCommitData, 1, wxEXPAND|wxTOP, 10 );


	bLeftSizer->Add( m_gitSizer, 0, wxEXPAND, 0 );


	bPanelSizer->Add( bLeftSizer, 0, wxRIGHT, 20 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_enableGit->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onEnableGitClick ), NULL, this );
	m_cbDefault->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onDefaultClick ), NULL, this );
}

PANEL_GIT_REPOS_BASE::~PANEL_GIT_REPOS_BASE()
{
	// Disconnect Events
	m_enableGit->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onEnableGitClick ), NULL, this );
	m_cbDefault->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onDefaultClick ), NULL, this );

}

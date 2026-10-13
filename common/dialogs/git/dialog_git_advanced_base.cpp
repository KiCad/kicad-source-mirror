///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_git_advanced_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GIT_ADVANCED_BASE::DIALOG_GIT_ADVANCED_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_topBar = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_staticText1;
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Repository:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	m_topBar->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4 );

	m_repoName = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_repoName->Wrap( -1 );
	m_repoName->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_topBar->Add( m_repoName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 16 );

	wxStaticText* m_staticText2;
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Branch:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_topBar->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4 );

	wxArrayString m_branchChoiceChoices;
	m_branchChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_branchChoiceChoices, 0 );
	m_branchChoice->SetSelection( 0 );
	m_topBar->Add( m_branchChoice, 0, wxALIGN_CENTER_VERTICAL, 0 );


	m_topBar->Add( 0, 0, 1, wxEXPAND, 0 );

	m_fetchButton = new wxButton( this, wxID_ANY, _("Fetch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_topBar->Add( m_fetchButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4 );

	m_pullButton = new wxButton( this, wxID_ANY, _("Pull"), wxDefaultPosition, wxDefaultSize, 0 );
	m_topBar->Add( m_pullButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4 );

	m_pushButton = new wxButton( this, wxID_ANY, _("Push"), wxDefaultPosition, wxDefaultSize, 0 );
	m_topBar->Add( m_pushButton, 0, wxALIGN_CENTER_VERTICAL, 0 );


	bMainSizer->Add( m_topBar, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_book = new WX_TREEBOOK( this, wxID_ANY );
	bMainSizer->Add( m_book, 1, wxEXPAND|wxALL, 4 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 4 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_GIT_ADVANCED_BASE::~DIALOG_GIT_ADVANCED_BASE()
{
}

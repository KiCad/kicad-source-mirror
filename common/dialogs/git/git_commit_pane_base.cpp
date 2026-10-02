///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "git_commit_pane_base.h"

///////////////////////////////////////////////////////////////////////////

GIT_COMMIT_PANE_BASE::GIT_COMMIT_PANE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_branchRow = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_commitToLabel;
	m_commitToLabel = new wxStaticText( this, wxID_ANY, _("Commit to:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_commitToLabel->Wrap( -1 );
	m_branchRow->Add( m_commitToLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4 );

	wxArrayString m_branchChoiceChoices;
	m_branchChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_branchChoiceChoices, 0 );
	m_branchChoice->SetSelection( 0 );
	m_branchRow->Add( m_branchChoice, 1, wxALIGN_CENTER_VERTICAL, 0 );


	bMainSizer->Add( m_branchRow, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 4 );

	m_summary = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bMainSizer->Add( m_summary, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 4 );

	m_summaryCount = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_summaryCount->Wrap( -1 );
	bMainSizer->Add( m_summaryCount, 0, wxALIGN_RIGHT|wxRIGHT, 4 );

	m_description = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_description->SetMinSize( wxSize( -1,60 ) );

	bMainSizer->Add( m_description, 0, wxEXPAND|wxLEFT|wxRIGHT, 4 );

	m_optionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Commit Options") ), wxVERTICAL );

	m_amendCheckbox = new wxCheckBox( m_optionsSizer->GetStaticBox(), wxID_ANY, _("Amend last commit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_optionsSizer->Add( m_amendCheckbox, 0, wxLEFT|wxRIGHT|wxTOP, 4 );

	m_signOffCheckbox = new wxCheckBox( m_optionsSizer->GetStaticBox(), wxID_ANY, _("Sign off (add Signed-off-by)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_optionsSizer->Add( m_signOffCheckbox, 0, wxLEFT|wxRIGHT|wxTOP, 4 );

	wxBoxSizer* bNewBranchSizer;
	bNewBranchSizer = new wxBoxSizer( wxHORIZONTAL );

	m_newBranchCheckbox = new wxCheckBox( m_optionsSizer->GetStaticBox(), wxID_ANY, _("Create a new branch"), wxDefaultPosition, wxDefaultSize, 0 );
	bNewBranchSizer->Add( m_newBranchCheckbox, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4 );

	m_newBranchName = new wxTextCtrl( m_optionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bNewBranchSizer->Add( m_newBranchName, 1, wxALIGN_CENTER_VERTICAL, 0 );


	m_optionsSizer->Add( bNewBranchSizer, 0, wxEXPAND|wxALL, 4 );


	bMainSizer->Add( m_optionsSizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 4 );

	m_changesRow = new wxBoxSizer( wxHORIZONTAL );

	m_changesLabel = new wxStaticText( this, wxID_ANY, _("Changes to Commit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_changesLabel->Wrap( -1 );
	m_changesRow->Add( m_changesLabel, 0, wxALIGN_CENTER_VERTICAL, 0 );


	m_changesRow->Add( 0, 0, 1, wxEXPAND, 0 );

	m_showDiffButton = new wxButton( this, wxID_ANY, _("Show Diff"), wxDefaultPosition, wxDefaultSize, 0 );
	m_changesRow->Add( m_showDiffButton, 0, wxALIGN_CENTER_VERTICAL, 0 );


	bMainSizer->Add( m_changesRow, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 4 );

	m_emptyLabel = new wxStaticText( this, wxID_ANY, _("No changes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_emptyLabel->Wrap( -1 );
	bMainSizer->Add( m_emptyLabel, 0, wxALL, 8 );

	m_checklist = new FILE_CHECKLIST_CTRL( this, wxID_ANY );
	bMainSizer->Add( m_checklist, 1, wxALL|wxEXPAND, 4 );

	wxBoxSizer* bCommitSizer;
	bCommitSizer = new wxBoxSizer( wxHORIZONTAL );

	m_commitOnlyButton = new wxButton( this, wxID_ANY, _("Commit"), wxDefaultPosition, wxDefaultSize, 0 );
	bCommitSizer->Add( m_commitOnlyButton, 1, wxALL, 4 );

	m_commitButton = new wxButton( this, wxID_ANY, _("Commit and Push"), wxDefaultPosition, wxDefaultSize, 0 );
	bCommitSizer->Add( m_commitButton, 1, wxALL, 4 );


	bMainSizer->Add( bCommitSizer, 0, wxEXPAND, 0 );

	wxBoxSizer* bActionSizer;
	bActionSizer = new wxBoxSizer( wxHORIZONTAL );

	m_pullButton = new wxButton( this, wxID_ANY, _("Get Changes"), wxDefaultPosition, wxDefaultSize, 0 );
	bActionSizer->Add( m_pullButton, 1, wxALL, 4 );

	m_advancedButton = new wxButton( this, wxID_ANY, _("Advanced..."), wxDefaultPosition, wxDefaultSize, 0 );
	bActionSizer->Add( m_advancedButton, 1, wxALL, 4 );


	bMainSizer->Add( bActionSizer, 0, wxEXPAND, 0 );


	this->SetSizer( bMainSizer );
	this->Layout();
}

GIT_COMMIT_PANE_BASE::~GIT_COMMIT_PANE_BASE()
{
}

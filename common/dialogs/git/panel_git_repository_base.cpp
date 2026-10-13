///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_git_repository_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GIT_REPOSITORY_BASE::PANEL_GIT_REPOSITORY_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bContent;
	bContent = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* m_grid;
	m_grid = new wxFlexGridSizer( 0, 2, 8, 12 );
	m_grid->AddGrowableCol( 1 );
	m_grid->SetFlexibleDirection( wxBOTH );
	m_grid->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_staticText10;
	m_staticText10 = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	m_grid->Add( m_staticText10, 0, wxALIGN_CENTER_VERTICAL, 0 );

	m_repoUserName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_grid->Add( m_repoUserName, 1, wxEXPAND, 0 );

	wxStaticText* m_staticText11;
	m_staticText11 = new wxStaticText( this, wxID_ANY, _("Email:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	m_grid->Add( m_staticText11, 0, wxALIGN_CENTER_VERTICAL, 0 );

	m_repoUserEmail = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_grid->Add( m_repoUserEmail, 1, wxEXPAND, 0 );


	bContent->Add( m_grid, 0, wxEXPAND|wxALL, 4 );

	m_identityNote = new wxStaticText( this, wxID_ANY, _("Name and email apply to this project only. Preferences holds the default used when these are empty."), wxDefaultPosition, wxDefaultSize, 0 );
	m_identityNote->Wrap( -1 );
	bContent->Add( m_identityNote, 0, wxALL, 4 );


	bMainSizer->Add( bContent, 1, wxEXPAND|wxALL, 8 );

	wxBoxSizer* bBottomRow;
	bBottomRow = new wxBoxSizer( wxHORIZONTAL );

	m_saveButton = new wxButton( this, wxID_ANY, _("Save"), wxDefaultPosition, wxDefaultSize, 0 );
	bBottomRow->Add( m_saveButton, 0, wxRIGHT, 4 );


	bMainSizer->Add( bBottomRow, 0, wxLEFT|wxRIGHT|wxBOTTOM, 4 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_GIT_REPOSITORY_BASE::~PANEL_GIT_REPOSITORY_BASE()
{
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_git_branches_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GIT_BRANCHES_BASE::PANEL_GIT_BRANCHES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_branchSearch = new wxSearchCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_branchSearch->ShowSearchButton( true );
	#endif
	m_branchSearch->ShowCancelButton( false );
	bMainSizer->Add( m_branchSearch, 0, wxEXPAND|wxALL, 4 );

	m_branchesSplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_branchesSplitter->SetSashGravity( 0.6 );
	m_branchesSplitter->SetMinimumPaneSize( 200 );

	m_branchListPanel = new wxPanel( m_branchesSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bbranchListPanelSizer;
	bbranchListPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_branchesList = new wxListCtrl( m_branchListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bbranchListPanelSizer->Add( m_branchesList, 1, wxEXPAND, 0 );

	m_branchCount = new wxStaticText( m_branchListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_branchCount->Wrap( -1 );
	bbranchListPanelSizer->Add( m_branchCount, 0, wxLEFT|wxTOP, 4 );


	m_branchListPanel->SetSizer( bbranchListPanelSizer );
	m_branchListPanel->Layout();
	bbranchListPanelSizer->Fit( m_branchListPanel );
	m_branchDetail = new wxPanel( m_branchesSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bbranchDetailSizer;
	bbranchDetailSizer = new wxBoxSizer( wxVERTICAL );

	m_branchTitle = new wxStaticText( m_branchDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_branchTitle->Wrap( -1 );
	m_branchTitle->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bbranchDetailSizer->Add( m_branchTitle, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_branchCommit = new wxStaticText( m_branchDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_branchCommit->Wrap( -1 );
	bbranchDetailSizer->Add( m_branchCommit, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_branchAuthor = new wxStaticText( m_branchDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_branchAuthor->Wrap( -1 );
	bbranchDetailSizer->Add( m_branchAuthor, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	wxStaticText* m_staticText4;
	m_staticText4 = new wxStaticText( m_branchDetail, wxID_ANY, _("Message:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	bbranchDetailSizer->Add( m_staticText4, 0, wxLEFT|wxRIGHT|wxTOP, 8 );

	m_branchMessage = new wxTextCtrl( m_branchDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_branchMessage->SetMinSize( wxSize( -1,90 ) );

	bbranchDetailSizer->Add( m_branchMessage, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_branchSync = new wxStaticText( m_branchDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_branchSync->Wrap( -1 );
	bbranchDetailSizer->Add( m_branchSync, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );


	bbranchDetailSizer->Add( 0, 0, 1, wxEXPAND, 0 );

	wxBoxSizer* bButtons;
	bButtons = new wxBoxSizer( wxHORIZONTAL );

	m_newBranchButton = new wxButton( m_branchDetail, wxID_ANY, _("New Branch..."), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_newBranchButton, 1, wxRIGHT, 4 );

	m_deleteBranchButton = new wxButton( m_branchDetail, wxID_ANY, _("Delete Branch"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_deleteBranchButton, 1, 0, 0 );


	bbranchDetailSizer->Add( bButtons, 0, wxEXPAND|wxALL, 8 );


	m_branchDetail->SetSizer( bbranchDetailSizer );
	m_branchDetail->Layout();
	bbranchDetailSizer->Fit( m_branchDetail );
	m_branchesSplitter->SplitVertically( m_branchListPanel, m_branchDetail, -1 );
	bMainSizer->Add( m_branchesSplitter, 1, wxEXPAND|wxALL, 4 );

	wxBoxSizer* bBottomRow;
	bBottomRow = new wxBoxSizer( wxHORIZONTAL );

	m_forcePushButton = new wxButton( this, wxID_ANY, _("Force Push"), wxDefaultPosition, wxDefaultSize, 0 );
	bBottomRow->Add( m_forcePushButton, 0, wxRIGHT, 4 );


	bMainSizer->Add( bBottomRow, 0, wxLEFT|wxRIGHT|wxBOTTOM, 4 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_GIT_BRANCHES_BASE::~PANEL_GIT_BRANCHES_BASE()
{
}

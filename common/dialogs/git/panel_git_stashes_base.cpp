///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_git_stashes_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GIT_STASHES_BASE::PANEL_GIT_STASHES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_stashesSplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_stashesSplitter->SetSashGravity( 0.6 );
	m_stashesSplitter->SetMinimumPaneSize( 200 );

	m_stashListPanel = new wxPanel( m_stashesSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bstashListPanelSizer;
	bstashListPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_stashesList = new wxListCtrl( m_stashListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bstashListPanelSizer->Add( m_stashesList, 1, wxEXPAND, 0 );

	m_stashCount = new wxStaticText( m_stashListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_stashCount->Wrap( -1 );
	bstashListPanelSizer->Add( m_stashCount, 0, wxLEFT|wxTOP, 4 );


	m_stashListPanel->SetSizer( bstashListPanelSizer );
	m_stashListPanel->Layout();
	bstashListPanelSizer->Fit( m_stashListPanel );
	m_stashDetail = new wxPanel( m_stashesSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bstashDetailSizer;
	bstashDetailSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticText* m_staticText6;
	m_staticText6 = new wxStaticText( m_stashDetail, wxID_ANY, _("Stash Details"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_staticText6->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bstashDetailSizer->Add( m_staticText6, 0, wxLEFT|wxRIGHT|wxTOP, 8 );

	m_stashWhen = new wxStaticText( m_stashDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_stashWhen->Wrap( -1 );
	bstashDetailSizer->Add( m_stashWhen, 0, wxLEFT|wxRIGHT|wxTOP, 8 );

	m_stashBranch = new wxStaticText( m_stashDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_stashBranch->Wrap( -1 );
	bstashDetailSizer->Add( m_stashBranch, 0, wxLEFT|wxRIGHT|wxTOP, 8 );

	m_stashCommit = new wxStaticText( m_stashDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_stashCommit->Wrap( -1 );
	bstashDetailSizer->Add( m_stashCommit, 0, wxLEFT|wxRIGHT|wxTOP, 8 );

	wxStaticText* m_staticText7;
	m_staticText7 = new wxStaticText( m_stashDetail, wxID_ANY, _("Message:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	bstashDetailSizer->Add( m_staticText7, 0, wxLEFT|wxRIGHT|wxTOP, 8 );

	m_stashMessage = new wxTextCtrl( m_stashDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_stashMessage->SetMinSize( wxSize( -1,60 ) );

	bstashDetailSizer->Add( m_stashMessage, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	wxStaticText* m_staticText8;
	m_staticText8 = new wxStaticText( m_stashDetail, wxID_ANY, _("Changes:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	bstashDetailSizer->Add( m_staticText8, 0, wxLEFT|wxRIGHT|wxTOP, 8 );

	m_stashFiles = new wxListCtrl( m_stashDetail, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL );
	bstashDetailSizer->Add( m_stashFiles, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	wxBoxSizer* bRestore;
	bRestore = new wxBoxSizer( wxHORIZONTAL );

	m_applyStashButton = new wxButton( m_stashDetail, wxID_ANY, _("Apply Stash"), wxDefaultPosition, wxDefaultSize, 0 );
	bRestore->Add( m_applyStashButton, 1, wxRIGHT, 4 );

	m_popStashButton = new wxButton( m_stashDetail, wxID_ANY, _("Pop Stash"), wxDefaultPosition, wxDefaultSize, 0 );
	bRestore->Add( m_popStashButton, 1, 0, 0 );


	bstashDetailSizer->Add( bRestore, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_dropStashButton = new wxButton( m_stashDetail, wxID_ANY, _("Drop Stash"), wxDefaultPosition, wxDefaultSize, 0 );
	bstashDetailSizer->Add( m_dropStashButton, 0, wxEXPAND|wxALL, 8 );


	m_stashDetail->SetSizer( bstashDetailSizer );
	m_stashDetail->Layout();
	bstashDetailSizer->Fit( m_stashDetail );
	m_stashesSplitter->SplitVertically( m_stashListPanel, m_stashDetail, -1 );
	bMainSizer->Add( m_stashesSplitter, 1, wxEXPAND|wxALL, 4 );

	wxBoxSizer* bBottomRow;
	bBottomRow = new wxBoxSizer( wxHORIZONTAL );

	m_stashSaveButton = new wxButton( this, wxID_ANY, _("Stash Changes..."), wxDefaultPosition, wxDefaultSize, 0 );
	bBottomRow->Add( m_stashSaveButton, 0, wxRIGHT, 4 );


	bMainSizer->Add( bBottomRow, 0, wxLEFT|wxRIGHT|wxBOTTOM, 4 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_GIT_STASHES_BASE::~PANEL_GIT_STASHES_BASE()
{
}

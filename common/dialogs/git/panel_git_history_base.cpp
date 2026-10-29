///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_git_history_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GIT_HISTORY_BASE::PANEL_GIT_HISTORY_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bFilters;
	bFilters = new wxBoxSizer( wxHORIZONTAL );

	m_historySearch = new wxSearchCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_historySearch->ShowSearchButton( true );
	#endif
	m_historySearch->ShowCancelButton( false );
	bFilters->Add( m_historySearch, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8 );

	wxArrayString m_historyBranchChoices;
	m_historyBranch = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_historyBranchChoices, 0 );
	m_historyBranch->SetSelection( 0 );
	bFilters->Add( m_historyBranch, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8 );

	m_historyShowRefs = new wxCheckBox( this, wxID_ANY, _("Show refs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_historyShowRefs->SetValue(true);
	bFilters->Add( m_historyShowRefs, 0, wxALIGN_CENTER_VERTICAL, 0 );


	bMainSizer->Add( bFilters, 0, wxEXPAND|wxALL, 4 );

	m_historyPathRow = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_staticText3;
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Showing commits that changed"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	m_historyPathRow->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 6 );

	m_historyPathLabel = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE );
	m_historyPathLabel->Wrap( -1 );
	m_historyPathLabel->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_historyPathRow->Add( m_historyPathLabel, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 12 );

	m_showAllButton = new wxButton( this, wxID_ANY, _("Show All Commits"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	m_historyPathRow->Add( m_showAllButton, 0, wxALIGN_CENTER_VERTICAL, 0 );


	bMainSizer->Add( m_historyPathRow, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4 );

	m_historySplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_historySplitter->SetSashGravity( 0.6 );
	m_historySplitter->SetMinimumPaneSize( 200 );

	m_historyListPanel = new wxPanel( m_historySplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bhistoryListPanelSizer;
	bhistoryListPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_historyList = new wxDataViewListCtrl( m_historyListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bhistoryListPanelSizer->Add( m_historyList, 1, wxEXPAND, 0 );


	m_historyListPanel->SetSizer( bhistoryListPanelSizer );
	m_historyListPanel->Layout();
	bhistoryListPanelSizer->Fit( m_historyListPanel );
	m_commitDetail = new wxPanel( m_historySplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bcommitDetailSizer;
	bcommitDetailSizer = new wxBoxSizer( wxVERTICAL );

	m_commitTitle = new wxStaticText( m_commitDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_commitTitle->Wrap( -1 );
	m_commitTitle->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bcommitDetailSizer->Add( m_commitTitle, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	wxBoxSizer* bAuthorRow;
	bAuthorRow = new wxBoxSizer( wxHORIZONTAL );

	m_commitAvatar = new wxStaticBitmap( m_commitDetail, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bAuthorRow->Add( m_commitAvatar, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 6 );

	m_commitAuthor = new wxStaticText( m_commitDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_commitAuthor->Wrap( -1 );
	bAuthorRow->Add( m_commitAuthor, 1, wxALIGN_CENTER_VERTICAL, 0 );


	bcommitDetailSizer->Add( bAuthorRow, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_commitMessage = new wxTextCtrl( m_commitDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_commitMessage->SetMinSize( wxSize( -1,90 ) );

	bcommitDetailSizer->Add( m_commitMessage, 0, wxEXPAND|wxALL, 8 );

	m_commitStats = new wxStaticText( m_commitDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_commitStats->Wrap( -1 );
	bcommitDetailSizer->Add( m_commitStats, 0, wxLEFT|wxRIGHT, 8 );

	wxBoxSizer* bStatsRow;
	bStatsRow = new wxBoxSizer( wxHORIZONTAL );

	m_commitFileCount = new wxStaticText( m_commitDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_commitFileCount->Wrap( -1 );
	bStatsRow->Add( m_commitFileCount, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 12 );

	m_commitAdded = new wxStaticText( m_commitDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_commitAdded->Wrap( -1 );
	bStatsRow->Add( m_commitAdded, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8 );

	m_commitRemoved = new wxStaticText( m_commitDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_commitRemoved->Wrap( -1 );
	bStatsRow->Add( m_commitRemoved, 0, wxALIGN_CENTER_VERTICAL, 0 );


	bcommitDetailSizer->Add( bStatsRow, 0, wxEXPAND|wxLEFT|wxRIGHT, 8 );

	m_detailSplitter = new wxSplitterWindow( m_commitDetail, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_detailSplitter->SetMinimumPaneSize( 60 );

	m_commitFilesPanel = new wxPanel( m_detailSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bcommitFilesPanelSizer;
	bcommitFilesPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_commitFiles = new wxListCtrl( m_commitFilesPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bcommitFilesPanelSizer->Add( m_commitFiles, 1, wxEXPAND, 0 );


	m_commitFilesPanel->SetSizer( bcommitFilesPanelSizer );
	m_commitFilesPanel->Layout();
	bcommitFilesPanelSizer->Fit( m_commitFilesPanel );
	m_detailSplitter->Initialize( m_commitFilesPanel );
	bcommitDetailSizer->Add( m_detailSplitter, 1, wxEXPAND|wxALL, 8 );


	m_commitDetail->SetSizer( bcommitDetailSizer );
	m_commitDetail->Layout();
	bcommitDetailSizer->Fit( m_commitDetail );
	m_historySplitter->SplitVertically( m_historyListPanel, m_commitDetail, -1 );
	bMainSizer->Add( m_historySplitter, 1, wxEXPAND|wxALL, 4 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_GIT_HISTORY_BASE::~PANEL_GIT_HISTORY_BASE()
{
}

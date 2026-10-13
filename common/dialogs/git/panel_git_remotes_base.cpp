///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_git_remotes_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GIT_REMOTES_BASE::PANEL_GIT_REMOTES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_remotesSplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_remotesSplitter->SetSashGravity( 0.6 );
	m_remotesSplitter->SetMinimumPaneSize( 200 );

	m_remoteListPanel = new wxPanel( m_remotesSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bremoteListPanelSizer;
	bremoteListPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_remotesList = new wxListCtrl( m_remoteListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bremoteListPanelSizer->Add( m_remotesList, 1, wxEXPAND, 0 );

	m_remoteCount = new wxStaticText( m_remoteListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_remoteCount->Wrap( -1 );
	bremoteListPanelSizer->Add( m_remoteCount, 0, wxLEFT|wxTOP, 4 );


	m_remoteListPanel->SetSizer( bremoteListPanelSizer );
	m_remoteListPanel->Layout();
	bremoteListPanelSizer->Fit( m_remoteListPanel );
	m_remoteDetail = new wxPanel( m_remotesSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bremoteDetailSizer;
	bremoteDetailSizer = new wxBoxSizer( wxVERTICAL );

	m_remoteTitle = new wxStaticText( m_remoteDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_remoteTitle->Wrap( -1 );
	m_remoteTitle->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bremoteDetailSizer->Add( m_remoteTitle, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	wxStaticText* m_staticText5;
	m_staticText5 = new wxStaticText( m_remoteDetail, wxID_ANY, _("URL:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bremoteDetailSizer->Add( m_staticText5, 0, wxLEFT|wxRIGHT|wxTOP, 8 );

	m_remoteUrl = new wxTextCtrl( m_remoteDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bremoteDetailSizer->Add( m_remoteUrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_remoteFetched = new wxStaticText( m_remoteDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_remoteFetched->Wrap( -1 );
	bremoteDetailSizer->Add( m_remoteFetched, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );


	bremoteDetailSizer->Add( 0, 0, 1, wxEXPAND, 0 );

	wxBoxSizer* bButtons;
	bButtons = new wxBoxSizer( wxHORIZONTAL );

	m_editUrlButton = new wxButton( m_remoteDetail, wxID_ANY, _("Edit URL..."), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_editUrlButton, 1, wxRIGHT, 4 );

	m_removeRemoteButton = new wxButton( m_remoteDetail, wxID_ANY, _("Remove Remote"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_removeRemoteButton, 1, 0, 0 );


	bremoteDetailSizer->Add( bButtons, 0, wxEXPAND|wxALL, 8 );


	m_remoteDetail->SetSizer( bremoteDetailSizer );
	m_remoteDetail->Layout();
	bremoteDetailSizer->Fit( m_remoteDetail );
	m_remotesSplitter->SplitVertically( m_remoteListPanel, m_remoteDetail, -1 );
	bMainSizer->Add( m_remotesSplitter, 1, wxEXPAND|wxALL, 4 );

	wxBoxSizer* bBottomRow;
	bBottomRow = new wxBoxSizer( wxHORIZONTAL );

	m_addRemoteButton = new wxButton( this, wxID_ANY, _("Add Remote..."), wxDefaultPosition, wxDefaultSize, 0 );
	bBottomRow->Add( m_addRemoteButton, 0, wxRIGHT, 4 );


	bMainSizer->Add( bBottomRow, 0, wxLEFT|wxRIGHT|wxBOTTOM, 4 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_GIT_REMOTES_BASE::~PANEL_GIT_REMOTES_BASE()
{
}

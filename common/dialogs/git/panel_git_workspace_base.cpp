///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialogs/git/panel_mdw_versions.h"

#include "panel_git_workspace_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GIT_WORKSPACE_BASE::PANEL_GIT_WORKSPACE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_search = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bMainSizer->Add( m_search, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 4 );

	m_workspaceSplitter = new WX_SPLITTER_WINDOW( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_workspaceSplitter->SetSashGravity( 0.6 );
	m_workspaceSplitter->SetMinimumPaneSize( 200 );

	m_itemListPanel = new wxPanel( m_workspaceSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bitemListPanelSizer;
	bitemListPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_itemsList = new wxListCtrl( m_itemListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bitemListPanelSizer->Add( m_itemsList, 1, wxEXPAND, 0 );

	wxBoxSizer* bStatusSizer;
	bStatusSizer = new wxBoxSizer( wxHORIZONTAL );

	m_itemCount = new wxStaticText( m_itemListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_itemCount->Wrap( -1 );
	bStatusSizer->Add( m_itemCount, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 4 );

	m_moreLink = new wxHyperlinkCtrl( m_itemListPanel, wxID_ANY, _("Show more"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_moreLink->Hide();

	bStatusSizer->Add( m_moreLink, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 4 );

	m_retryLink = new wxHyperlinkCtrl( m_itemListPanel, wxID_ANY, _("Retry"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_retryLink->Hide();

	bStatusSizer->Add( m_retryLink, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 4 );


	bitemListPanelSizer->Add( bStatusSizer, 0, wxEXPAND, 0 );


	m_itemListPanel->SetSizer( bitemListPanelSizer );
	m_itemListPanel->Layout();
	bitemListPanelSizer->Fit( m_itemListPanel );
	m_itemDetail = new wxPanel( m_workspaceSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bitemDetailSizer;
	bitemDetailSizer = new wxBoxSizer( wxVERTICAL );

	m_itemTitle = new wxStaticText( m_itemDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_itemTitle->Wrap( -1 );
	m_itemTitle->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bitemDetailSizer->Add( m_itemTitle, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_itemFolder = new wxStaticText( m_itemDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_itemFolder->Wrap( -1 );
	bitemDetailSizer->Add( m_itemFolder, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_itemEditing = new wxStaticText( m_itemDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_itemEditing->Wrap( -1 );
	bitemDetailSizer->Add( m_itemEditing, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_itemLastChange = new wxStaticText( m_itemDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_itemLastChange->Wrap( -1 );
	bitemDetailSizer->Add( m_itemLastChange, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_versions = new PANEL_MDW_VERSIONS( m_itemDetail, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_versions->SetMinSize( wxSize( -1,120 ) );

	bitemDetailSizer->Add( m_versions, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	wxBoxSizer* bButtons;
	bButtons = new wxBoxSizer( wxHORIZONTAL );


	bButtons->Add( 0, 0, 1, wxEXPAND, 0 );

	m_checkOutButton = new wxButton( m_itemDetail, wxID_ANY, _("Check Out"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_checkOutButton, 0, wxRIGHT, 4 );

	m_checkInButton = new wxButton( m_itemDetail, wxID_ANY, _("Check In..."), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_checkInButton, 0, wxRIGHT, 4 );

	m_editingButton = new wxButton( m_itemDetail, wxID_ANY, _("Start Editing"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_editingButton, 0, wxRIGHT, 4 );


	bitemDetailSizer->Add( bButtons, 0, wxEXPAND|wxALL, 8 );


	m_itemDetail->SetSizer( bitemDetailSizer );
	m_itemDetail->Layout();
	bitemDetailSizer->Fit( m_itemDetail );
	m_workspaceSplitter->SplitVertically( m_itemListPanel, m_itemDetail, -1 );
	bMainSizer->Add( m_workspaceSplitter, 1, wxEXPAND|wxALL, 4 );

	wxBoxSizer* bBottomRow;
	bBottomRow = new wxBoxSizer( wxHORIZONTAL );

	m_connectLibraryButton = new wxButton( this, wxID_ANY, _("Connect Team Library..."), wxDefaultPosition, wxDefaultSize, 0 );
	bBottomRow->Add( m_connectLibraryButton, 0, wxRIGHT, 4 );


	bMainSizer->Add( bBottomRow, 0, wxLEFT|wxRIGHT|wxBOTTOM, 4 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_GIT_WORKSPACE_BASE::~PANEL_GIT_WORKSPACE_BASE()
{
}

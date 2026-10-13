///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_git_tags_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GIT_TAGS_BASE::PANEL_GIT_TAGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_tagsSplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_tagsSplitter->SetSashGravity( 0.6 );
	m_tagsSplitter->SetMinimumPaneSize( 200 );

	m_tagListPanel = new wxPanel( m_tagsSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* btagListPanelSizer;
	btagListPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_tagsList = new wxListCtrl( m_tagListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	btagListPanelSizer->Add( m_tagsList, 1, wxEXPAND, 0 );

	m_tagCount = new wxStaticText( m_tagListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_tagCount->Wrap( -1 );
	btagListPanelSizer->Add( m_tagCount, 0, wxLEFT|wxTOP, 4 );


	m_tagListPanel->SetSizer( btagListPanelSizer );
	m_tagListPanel->Layout();
	btagListPanelSizer->Fit( m_tagListPanel );
	m_tagDetail = new wxPanel( m_tagsSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* btagDetailSizer;
	btagDetailSizer = new wxBoxSizer( wxVERTICAL );

	m_tagTitle = new wxStaticText( m_tagDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_tagTitle->Wrap( -1 );
	m_tagTitle->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	btagDetailSizer->Add( m_tagTitle, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_tagCommit = new wxStaticText( m_tagDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_tagCommit->Wrap( -1 );
	btagDetailSizer->Add( m_tagCommit, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_tagAuthor = new wxStaticText( m_tagDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_tagAuthor->Wrap( -1 );
	btagDetailSizer->Add( m_tagAuthor, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	m_tagWhen = new wxStaticText( m_tagDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_tagWhen->Wrap( -1 );
	btagDetailSizer->Add( m_tagWhen, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	wxStaticText* m_staticText9;
	m_staticText9 = new wxStaticText( m_tagDetail, wxID_ANY, _("Message:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	btagDetailSizer->Add( m_staticText9, 0, wxLEFT|wxRIGHT|wxTOP, 8 );

	m_tagMessage = new wxTextCtrl( m_tagDetail, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_tagMessage->SetMinSize( wxSize( -1,90 ) );

	btagDetailSizer->Add( m_tagMessage, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );

	wxBoxSizer* bButtons;
	bButtons = new wxBoxSizer( wxHORIZONTAL );


	bButtons->Add( 0, 0, 1, wxEXPAND, 0 );

	m_checkoutTagButton = new wxButton( m_tagDetail, wxID_ANY, _("Check Out"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_checkoutTagButton, 0, wxRIGHT, 4 );

	m_deleteTagButton = new wxButton( m_tagDetail, wxID_ANY, _("Delete Tag"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_deleteTagButton, 0, 0, 0 );


	btagDetailSizer->Add( bButtons, 0, wxEXPAND|wxALL, 8 );


	m_tagDetail->SetSizer( btagDetailSizer );
	m_tagDetail->Layout();
	btagDetailSizer->Fit( m_tagDetail );
	m_tagsSplitter->SplitVertically( m_tagListPanel, m_tagDetail, -1 );
	bMainSizer->Add( m_tagsSplitter, 1, wxEXPAND|wxALL, 4 );

	wxBoxSizer* bBottomRow;
	bBottomRow = new wxBoxSizer( wxHORIZONTAL );

	m_createTagButton = new wxButton( this, wxID_ANY, _("Create Tag..."), wxDefaultPosition, wxDefaultSize, 0 );
	bBottomRow->Add( m_createTagButton, 0, wxRIGHT, 4 );


	bMainSizer->Add( bBottomRow, 0, wxLEFT|wxRIGHT|wxBOTTOM, 4 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_GIT_TAGS_BASE::~PANEL_GIT_TAGS_BASE()
{
}

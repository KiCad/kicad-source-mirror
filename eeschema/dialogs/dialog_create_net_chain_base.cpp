///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_create_net_chain_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CREATE_NET_CHAIN_BASE::DIALOG_CREATE_NET_CHAIN_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 700,650 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_headerLabel = new wxStaticText( this, wxID_ANY, _(" Select a detected net chain path and assign a name to create a net chain."), wxDefaultPosition, wxDefaultSize, 0 );
	m_headerLabel->Wrap( -1 );
	bMainSizer->Add( m_headerLabel, 0, wxLEFT|wxRIGHT|wxTOP, 10 );

	m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_splitter->SetSashGravity( 0.75 );
	m_splitter->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_CREATE_NET_CHAIN_BASE::m_splitterOnIdle ), NULL, this );
	m_splitter->SetMinimumPaneSize( 60 );

	m_gridPanel = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bGridSizer;
	bGridSizer = new wxBoxSizer( wxVERTICAL );

	m_filterInput = new wxTextCtrl( m_gridPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bGridSizer->Add( m_filterInput, 0, wxBOTTOM|wxEXPAND, 5 );

	m_chainsGrid = new WX_GRID( m_gridPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_chainsGrid->CreateGrid( 0, 3 );
	m_chainsGrid->EnableEditing( false );
	m_chainsGrid->EnableGridLines( true );
	m_chainsGrid->EnableDragGridSize( false );
	m_chainsGrid->SetMargins( 0, 0 );

	// Columns
	m_chainsGrid->SetColSize( 0, 200 );
	m_chainsGrid->SetColSize( 1, 60 );
	m_chainsGrid->SetColSize( 2, 300 );
	m_chainsGrid->EnableDragColMove( false );
	m_chainsGrid->EnableDragColSize( true );
	m_chainsGrid->SetColLabelValue( 0, _("Suggested Name") );
	m_chainsGrid->SetColLabelValue( 1, _("Nets") );
	m_chainsGrid->SetColLabelValue( 2, _("Preview") );
	m_chainsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_chainsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_chainsGrid->EnableDragRowSize( false );
	m_chainsGrid->SetRowLabelSize( 0 );
	m_chainsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_chainsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_chainsGrid->SetMinSize( wxSize( -1,120 ) );

	bGridSizer->Add( m_chainsGrid, 1, wxEXPAND, 5 );


	m_gridPanel->SetSizer( bGridSizer );
	m_gridPanel->Layout();
	bGridSizer->Fit( m_gridPanel );
	m_membersPanel = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bMembersSizer;
	bMembersSizer = new wxBoxSizer( wxVERTICAL );

	m_membersLabel = new wxStaticText( m_membersPanel, wxID_ANY, _("Member Nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_membersLabel->Wrap( -1 );
	bMembersSizer->Add( m_membersLabel, 0, wxLEFT|wxTOP, 5 );

	m_membersListBox = new wxListBox( m_membersPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE );
	bMembersSizer->Add( m_membersListBox, 1, wxALL|wxEXPAND, 5 );


	m_membersPanel->SetSizer( bMembersSizer );
	m_membersPanel->Layout();
	bMembersSizer->Fit( m_membersPanel );
	m_splitter->SplitHorizontally( m_gridPanel, m_membersPanel, -100 );
	bMainSizer->Add( m_splitter, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	m_separator = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_separator, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	m_manualLabel = new wxStaticText( this, wxID_ANY, _("Manual endpoint selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_manualLabel->Wrap( -1 );
	m_manualLabel->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Helvetica") ) );

	bMainSizer->Add( m_manualLabel, 0, wxLEFT|wxRIGHT|wxTOP, 10 );

	wxBoxSizer* bEndpointSizer;
	bEndpointSizer = new wxBoxSizer( wxHORIZONTAL );

	m_fromLabel = new wxStaticText( this, wxID_ANY, _("From:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fromLabel->Wrap( -1 );
	bEndpointSizer->Add( m_fromLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP, 5 );

	m_fromComponent = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_SORT );
	bEndpointSizer->Add( m_fromComponent, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP, 5 );

	m_toLabel = new wxStaticText( this, wxID_ANY, _("To:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toLabel->Wrap( -1 );
	bEndpointSizer->Add( m_toLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_toComponent = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_SORT );
	bEndpointSizer->Add( m_toComponent, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP, 5 );

	m_findPathButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_findPathButton->SetToolTip( _("Find net chain path between the two components") );

	bEndpointSizer->Add( m_findPathButton, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 0 );


	bMainSizer->Add( bEndpointSizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	wxBoxSizer* bNameSizer;
	bNameSizer = new wxBoxSizer( wxHORIZONTAL );

	m_nameLabel = new wxStaticText( this, wxID_ANY, _("Chain Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nameLabel->Wrap( -1 );
	bNameSizer->Add( m_nameLabel, 0, wxRIGHT|wxTOP, 5 );

	m_nameInput = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bNameSizer->Add( m_nameInput, 1, wxEXPAND|wxRIGHT, 5 );


	bMainSizer->Add( bNameSizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_refreshButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_refreshButton->SetToolTip( _("Re-detect potential net chains from the schematic") );

	bButtonsSizer->Add( m_refreshButton, 0, wxRIGHT, 5 );


	bButtonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bButtonsSizer->Add( m_sdbSizer, 0, wxALIGN_CENTER_VERTICAL, 0 );


	bMainSizer->Add( bButtonsSizer, 0, wxALL|wxEXPAND, 10 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_filterInput->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_NET_CHAIN_BASE::OnFilterChanged ), NULL, this );
	m_chainsGrid->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_CREATE_NET_CHAIN_BASE::OnChainSelected ), NULL, this );
	m_findPathButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CREATE_NET_CHAIN_BASE::OnFindPathClicked ), NULL, this );
	m_refreshButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CREATE_NET_CHAIN_BASE::OnRefreshClicked ), NULL, this );
}

DIALOG_CREATE_NET_CHAIN_BASE::~DIALOG_CREATE_NET_CHAIN_BASE()
{
	// Disconnect Events
	m_filterInput->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_NET_CHAIN_BASE::OnFilterChanged ), NULL, this );
	m_chainsGrid->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_CREATE_NET_CHAIN_BASE::OnChainSelected ), NULL, this );
	m_findPathButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CREATE_NET_CHAIN_BASE::OnFindPathClicked ), NULL, this );
	m_refreshButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CREATE_NET_CHAIN_BASE::OnRefreshClicked ), NULL, this );

}

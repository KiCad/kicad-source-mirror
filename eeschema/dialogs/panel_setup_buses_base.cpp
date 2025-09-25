///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_setup_buses_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_BUSES_BASE::PANEL_SETUP_BUSES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	bMargins = new wxBoxSizer( wxHORIZONTAL );

	bLeftCol = new wxBoxSizer( wxVERTICAL );

	m_busesLabel = new wxStaticText( this, wxID_ANY, _("Bus Definitions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_busesLabel->Wrap( -1 );
	bLeftCol->Add( m_busesLabel, 0, wxLEFT|wxTOP, 8 );


	bLeftCol->Add( 0, 3, 0, wxEXPAND, 5 );

	m_aliasesGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_aliasesGrid->CreateGrid( 0, 1 );
	m_aliasesGrid->EnableEditing( true );
	m_aliasesGrid->EnableGridLines( true );
	m_aliasesGrid->EnableDragGridSize( false );
	m_aliasesGrid->SetMargins( 0, 0 );

	// Columns
	m_aliasesGrid->SetColSize( 0, 300 );
	m_aliasesGrid->EnableDragColMove( false );
	m_aliasesGrid->EnableDragColSize( true );
	m_aliasesGrid->SetColLabelValue( 0, _("Alias") );
	m_aliasesGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_aliasesGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_aliasesGrid->EnableDragRowSize( true );
	m_aliasesGrid->SetRowLabelSize( 0 );
	m_aliasesGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_aliasesGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_aliasesGrid->SetMinSize( wxSize( -1,180 ) );

	bLeftCol->Add( m_aliasesGrid, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	m_addAlias = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer10->Add( m_addAlias, 0, wxBOTTOM, 5 );


	bSizer10->Add( 20, 0, 0, wxEXPAND, 5 );

	m_deleteAlias = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer10->Add( m_deleteAlias, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer10->Add( 15, 0, 1, 0, 5 );

	m_source = new wxStaticText( this, wxID_ANY, _("(source)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_source->Wrap( -1 );
	bSizer10->Add( m_source, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bLeftCol->Add( bSizer10, 0, wxEXPAND|wxTOP, 3 );


	bMargins->Add( bLeftCol, 1, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bRightColumn;
	bRightColumn = new wxBoxSizer( wxVERTICAL );

	m_membersBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	membersPanel = new wxPanel( m_membersBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	m_membersLabel = new wxStaticText( membersPanel, wxID_ANY, _("Members of '%s'"), wxDefaultPosition, wxDefaultSize, 0 );
	m_membersLabel->Wrap( -1 );
	bSizer7->Add( m_membersLabel, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 8 );


	bSizer7->Add( 0, 3, 0, wxEXPAND, 5 );

	m_membersGrid = new WX_GRID( membersPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_membersGrid->CreateGrid( 0, 1 );
	m_membersGrid->EnableEditing( true );
	m_membersGrid->EnableGridLines( true );
	m_membersGrid->EnableDragGridSize( false );
	m_membersGrid->SetMargins( 0, 0 );

	// Columns
	m_membersGrid->SetColSize( 0, 300 );
	m_membersGrid->EnableDragColMove( false );
	m_membersGrid->EnableDragColSize( true );
	m_membersGrid->SetColLabelValue( 0, _("Net or Nested Bus Name") );
	m_membersGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_membersGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_membersGrid->EnableDragRowSize( true );
	m_membersGrid->SetRowLabelSize( 0 );
	m_membersGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_membersGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_membersGrid->SetMinSize( wxSize( -1,180 ) );

	bSizer7->Add( m_membersGrid, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer101;
	bSizer101 = new wxBoxSizer( wxHORIZONTAL );

	m_addMember = new STD_BITMAP_BUTTON( membersPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer101->Add( m_addMember, 0, wxBOTTOM, 5 );


	bSizer101->Add( 20, 0, 0, wxEXPAND, 5 );

	m_removeMember = new STD_BITMAP_BUTTON( membersPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer101->Add( m_removeMember, 0, wxBOTTOM|wxLEFT, 5 );


	bSizer7->Add( bSizer101, 0, wxEXPAND|wxTOP, 3 );


	membersPanel->SetSizer( bSizer7 );
	membersPanel->Layout();
	bSizer7->Fit( membersPanel );
	m_membersBook->AddPage( membersPanel, _("a page"), false );
	emptyPanel = new wxPanel( m_membersBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	emptyPanel->Enable( false );

	m_membersBook->AddPage( emptyPanel, _("a page"), false );

	bRightColumn->Add( m_membersBook, 1, wxEXPAND, 5 );


	bMargins->Add( bRightColumn, 1, wxEXPAND|wxLEFT, 5 );


	bPanelSizer->Add( bMargins, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BUSES_BASE::OnUpdateUI ) );
	m_aliasesGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_BUSES_BASE::OnSizeGrid ), NULL, this );
	m_addAlias->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BUSES_BASE::OnAddAlias ), NULL, this );
	m_deleteAlias->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BUSES_BASE::OnDeleteAlias ), NULL, this );
	m_membersGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_BUSES_BASE::OnSizeGrid ), NULL, this );
	m_addMember->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BUSES_BASE::OnAddMember ), NULL, this );
	m_removeMember->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BUSES_BASE::OnRemoveMember ), NULL, this );
}

PANEL_SETUP_BUSES_BASE::~PANEL_SETUP_BUSES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BUSES_BASE::OnUpdateUI ) );
	m_aliasesGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_BUSES_BASE::OnSizeGrid ), NULL, this );
	m_addAlias->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BUSES_BASE::OnAddAlias ), NULL, this );
	m_deleteAlias->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BUSES_BASE::OnDeleteAlias ), NULL, this );
	m_membersGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_BUSES_BASE::OnSizeGrid ), NULL, this );
	m_addMember->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BUSES_BASE::OnAddMember ), NULL, this );
	m_removeMember->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BUSES_BASE::OnRemoveMember ), NULL, this );

}

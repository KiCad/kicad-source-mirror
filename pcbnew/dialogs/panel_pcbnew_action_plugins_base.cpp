///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 11 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_pcbnew_action_plugins_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PCBNEW_ACTION_PLUGINS_BASE::PANEL_PCBNEW_ACTION_PLUGINS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bGridSizer;
	bGridSizer = new wxBoxSizer( wxVERTICAL );
	
	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE );
	
	// Grid
	m_grid->CreateGrid( 3, 6 );
	m_grid->EnableEditing( false );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );
	
	// Columns
	m_grid->AutoSizeColumns();
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelValue( 0, wxT("Icon") );
	m_grid->SetColLabelValue( 1, wxT("Show button") );
	m_grid->SetColLabelValue( 2, wxT("Name") );
	m_grid->SetColLabelValue( 3, wxT("Category") );
	m_grid->SetColLabelValue( 4, wxT("Description") );
	m_grid->SetColLabelValue( 5, wxT("Path") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	bGridSizer->Add( m_grid, 1, wxALL|wxEXPAND, 5 );
	
	
	bPanelSizer->Add( bGridSizer, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT, 0 );
	
	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_moveUpButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_moveUpButton->SetMinSize( wxSize( 32,32 ) );
	
	bButtonsSizer->Add( m_moveUpButton, 0, wxALIGN_TOP|wxALL, 5 );
	
	m_moveDownButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_moveDownButton->SetMinSize( wxSize( 32,32 ) );
	
	bButtonsSizer->Add( m_moveDownButton, 0, wxALL, 5 );
	
	m_reloadButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_reloadButton->SetMinSize( wxSize( 32,32 ) );
	
	bButtonsSizer->Add( m_reloadButton, 0, wxALL, 5 );
	
	
	bPanelSizer->Add( bButtonsSizer, 0, wxALIGN_RIGHT|wxALIGN_TOP, 0 );
	
	
	this->SetSizer( bPanelSizer );
	this->Layout();
	
	// Connect Events
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnGridCellClick ), NULL, this );
	m_moveUpButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnMoveUpButtonClick ), NULL, this );
	m_moveDownButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnMoveDownButtonClick ), NULL, this );
	m_reloadButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnReloadButtonClick ), NULL, this );
}

PANEL_PCBNEW_ACTION_PLUGINS_BASE::~PANEL_PCBNEW_ACTION_PLUGINS_BASE()
{
	// Disconnect Events
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnGridCellClick ), NULL, this );
	m_moveUpButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnMoveUpButtonClick ), NULL, this );
	m_moveDownButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnMoveDownButtonClick ), NULL, this );
	m_reloadButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnReloadButtonClick ), NULL, this );
	
}

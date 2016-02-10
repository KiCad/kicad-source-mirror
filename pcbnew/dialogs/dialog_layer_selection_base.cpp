///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jan  1 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_layer_selection_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LAYER_SELECTION_BASE::DIALOG_LAYER_SELECTION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	m_leftGridLayers = new wxGrid( this, ID_LEFT_LIST, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_leftGridLayers->CreateGrid( 1, 3 );
	m_leftGridLayers->EnableEditing( false );
	m_leftGridLayers->EnableGridLines( true );
	m_leftGridLayers->EnableDragGridSize( false );
	m_leftGridLayers->SetMargins( 0, 3 );
	
	// Columns
	m_leftGridLayers->EnableDragColMove( false );
	m_leftGridLayers->EnableDragColSize( false );
	m_leftGridLayers->SetColLabelSize( 0 );
	m_leftGridLayers->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_leftGridLayers->EnableDragRowSize( false );
	m_leftGridLayers->SetRowLabelSize( 0 );
	m_leftGridLayers->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	m_leftGridLayers->SetLabelBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	// Cell Defaults
	m_leftGridLayers->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_MENU ) );
	m_leftGridLayers->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerUpper->Add( m_leftGridLayers, 1, wxALL|wxEXPAND, 5 );
	
	m_rightGridLayers = new wxGrid( this, ID_RIGHT_LIST, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_rightGridLayers->CreateGrid( 1, 3 );
	m_rightGridLayers->EnableEditing( false );
	m_rightGridLayers->EnableGridLines( true );
	m_rightGridLayers->EnableDragGridSize( false );
	m_rightGridLayers->SetMargins( 0, 3 );
	
	// Columns
	m_rightGridLayers->EnableDragColMove( false );
	m_rightGridLayers->EnableDragColSize( false );
	m_rightGridLayers->SetColLabelSize( 0 );
	m_rightGridLayers->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_rightGridLayers->EnableDragRowSize( false );
	m_rightGridLayers->SetRowLabelSize( 0 );
	m_rightGridLayers->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_rightGridLayers->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_MENU ) );
	m_rightGridLayers->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerUpper->Add( m_rightGridLayers, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_leftGridLayers->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LAYER_SELECTION_BASE::OnLeftGridCellClick ), NULL, this );
	m_leftGridLayers->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_LAYER_SELECTION_BASE::OnLeftButtonReleased ), NULL, this );
	m_rightGridLayers->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LAYER_SELECTION_BASE::OnRightGridCellClick ), NULL, this );
	m_rightGridLayers->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_LAYER_SELECTION_BASE::OnLeftButtonReleased ), NULL, this );
}

DIALOG_LAYER_SELECTION_BASE::~DIALOG_LAYER_SELECTION_BASE()
{
	// Disconnect Events
	m_leftGridLayers->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LAYER_SELECTION_BASE::OnLeftGridCellClick ), NULL, this );
	m_leftGridLayers->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_LAYER_SELECTION_BASE::OnLeftButtonReleased ), NULL, this );
	m_rightGridLayers->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LAYER_SELECTION_BASE::OnRightGridCellClick ), NULL, this );
	m_rightGridLayers->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_LAYER_SELECTION_BASE::OnLeftButtonReleased ), NULL, this );
	
}

DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextTopLayer = new wxStaticText( this, wxID_ANY, _("Top/Front Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTopLayer->Wrap( -1 );
	bSizerLeft->Add( m_staticTextTopLayer, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_leftGridLayers = new wxGrid( this, ID_LEFT_LIST, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_leftGridLayers->CreateGrid( 1, 3 );
	m_leftGridLayers->EnableEditing( false );
	m_leftGridLayers->EnableGridLines( true );
	m_leftGridLayers->EnableDragGridSize( false );
	m_leftGridLayers->SetMargins( 0, 3 );
	
	// Columns
	m_leftGridLayers->EnableDragColMove( false );
	m_leftGridLayers->EnableDragColSize( false );
	m_leftGridLayers->SetColLabelSize( 0 );
	m_leftGridLayers->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_leftGridLayers->EnableDragRowSize( false );
	m_leftGridLayers->SetRowLabelSize( 0 );
	m_leftGridLayers->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	m_leftGridLayers->SetLabelBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	// Cell Defaults
	m_leftGridLayers->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_MENU ) );
	m_leftGridLayers->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerLeft->Add( m_leftGridLayers, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizerUpper->Add( bSizerLeft, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextBottomLayer = new wxStaticText( this, wxID_ANY, _("Bottom/Back Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBottomLayer->Wrap( -1 );
	bSizerRight->Add( m_staticTextBottomLayer, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_rightGridLayers = new wxGrid( this, ID_RIGHT_LIST, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_rightGridLayers->CreateGrid( 1, 3 );
	m_rightGridLayers->EnableEditing( false );
	m_rightGridLayers->EnableGridLines( true );
	m_rightGridLayers->EnableDragGridSize( false );
	m_rightGridLayers->SetMargins( 0, 3 );
	
	// Columns
	m_rightGridLayers->EnableDragColMove( false );
	m_rightGridLayers->EnableDragColSize( false );
	m_rightGridLayers->SetColLabelSize( 0 );
	m_rightGridLayers->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_rightGridLayers->EnableDragRowSize( false );
	m_rightGridLayers->SetRowLabelSize( 0 );
	m_rightGridLayers->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_rightGridLayers->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_MENU ) );
	m_rightGridLayers->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerRight->Add( m_rightGridLayers, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizerUpper->Add( bSizerRight, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_leftGridLayers->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnLeftGridCellClick ), NULL, this );
	m_rightGridLayers->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnRightGridCellClick ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnOKClick ), NULL, this );
}

DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::~DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE()
{
	// Disconnect Events
	m_leftGridLayers->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnLeftGridCellClick ), NULL, this );
	m_rightGridLayers->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnRightGridCellClick ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnOKClick ), NULL, this );
	
}

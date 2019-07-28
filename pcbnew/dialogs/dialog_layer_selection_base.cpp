///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_layer_selection_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LAYER_SELECTION_BASE::DIALOG_LAYER_SELECTION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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
	m_leftGridLayers->EnableGridLines( false );
	m_leftGridLayers->EnableDragGridSize( false );
	m_leftGridLayers->SetMargins( 5, 3 );

	// Columns
	m_leftGridLayers->SetColSize( 0, 24 );
	m_leftGridLayers->SetColSize( 1, 20 );
	m_leftGridLayers->SetColSize( 2, 72 );
	m_leftGridLayers->EnableDragColMove( false );
	m_leftGridLayers->EnableDragColSize( false );
	m_leftGridLayers->SetColLabelSize( 0 );
	m_leftGridLayers->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_leftGridLayers->EnableDragRowSize( false );
	m_leftGridLayers->SetRowLabelSize( 0 );
	m_leftGridLayers->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_leftGridLayers->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerUpper->Add( m_leftGridLayers, 1, wxALL|wxEXPAND, 5 );

	m_rightGridLayers = new wxGrid( this, ID_RIGHT_LIST, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_rightGridLayers->CreateGrid( 1, 3 );
	m_rightGridLayers->EnableEditing( false );
	m_rightGridLayers->EnableGridLines( false );
	m_rightGridLayers->EnableDragGridSize( false );
	m_rightGridLayers->SetMargins( 5, 3 );

	// Columns
	m_rightGridLayers->SetColSize( 0, 24 );
	m_rightGridLayers->SetColSize( 1, 20 );
	m_rightGridLayers->SetColSize( 2, 72 );
	m_rightGridLayers->EnableDragColMove( false );
	m_rightGridLayers->EnableDragColSize( false );
	m_rightGridLayers->SetColLabelSize( 0 );
	m_rightGridLayers->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_rightGridLayers->EnableDragRowSize( false );
	m_rightGridLayers->SetRowLabelSize( 0 );
	m_rightGridLayers->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_rightGridLayers->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerUpper->Add( m_rightGridLayers, 1, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND|wxALL, 10 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

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

DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );

	m_staticTextTopLayer = new wxStaticText( this, wxID_ANY, _("Top/Front layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTopLayer->Wrap( -1 );
	bSizerLeft->Add( m_staticTextTopLayer, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_leftGridLayers = new wxGrid( this, ID_LEFT_LIST, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_leftGridLayers->CreateGrid( 1, 3 );
	m_leftGridLayers->EnableEditing( false );
	m_leftGridLayers->EnableGridLines( false );
	m_leftGridLayers->EnableDragGridSize( false );
	m_leftGridLayers->SetMargins( 3, 3 );

	// Columns
	m_leftGridLayers->SetColSize( 0, 24 );
	m_leftGridLayers->SetColSize( 1, 20 );
	m_leftGridLayers->SetColSize( 2, 72 );
	m_leftGridLayers->EnableDragColMove( false );
	m_leftGridLayers->EnableDragColSize( false );
	m_leftGridLayers->SetColLabelSize( 0 );
	m_leftGridLayers->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_leftGridLayers->EnableDragRowSize( false );
	m_leftGridLayers->SetRowLabelSize( 0 );
	m_leftGridLayers->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_leftGridLayers->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizerLeft->Add( m_leftGridLayers, 0, wxALL|wxEXPAND, 5 );


	bSizerUpper->Add( bSizerLeft, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	m_staticTextBottomLayer = new wxStaticText( this, wxID_ANY, _("Bottom/Back layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBottomLayer->Wrap( -1 );
	bSizerRight->Add( m_staticTextBottomLayer, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_rightGridLayers = new wxGrid( this, ID_RIGHT_LIST, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_rightGridLayers->CreateGrid( 1, 3 );
	m_rightGridLayers->EnableEditing( false );
	m_rightGridLayers->EnableGridLines( false );
	m_rightGridLayers->EnableDragGridSize( false );
	m_rightGridLayers->SetMargins( 3, 3 );

	// Columns
	m_rightGridLayers->SetColSize( 0, 24 );
	m_rightGridLayers->SetColSize( 1, 20 );
	m_rightGridLayers->SetColSize( 2, 72 );
	m_rightGridLayers->EnableDragColMove( false );
	m_rightGridLayers->EnableDragColSize( false );
	m_rightGridLayers->SetColLabelSize( 0 );
	m_rightGridLayers->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_rightGridLayers->EnableDragRowSize( false );
	m_rightGridLayers->SetRowLabelSize( 0 );
	m_rightGridLayers->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_rightGridLayers->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizerRight->Add( m_rightGridLayers, 0, wxALL|wxEXPAND, 5 );


	bSizerUpper->Add( bSizerRight, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxALL|wxEXPAND, 5 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_RIGHT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_leftGridLayers->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnLeftGridCellClick ), NULL, this );
	m_rightGridLayers->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnRightGridCellClick ), NULL, this );
}

DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::~DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE()
{
	// Disconnect Events
	m_leftGridLayers->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnLeftGridCellClick ), NULL, this );
	m_rightGridLayers->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::OnRightGridCellClick ), NULL, this );

}

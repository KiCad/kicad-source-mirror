///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_layer_selection_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LAYER_SELECTION_BASE::DIALOG_LAYER_SELECTION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	m_leftGridLayers = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

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
	m_leftGridLayers->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizerUpper->Add( m_leftGridLayers, 1, wxALL|wxEXPAND, 5 );

	m_rightGridLayers = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

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
	m_rightGridLayers->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizerUpper->Add( m_rightGridLayers, 1, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND|wxALL, 10 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LAYER_SELECTION_BASE::OnMouseMove ) );
	m_leftGridLayers->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LAYER_SELECTION_BASE::OnLeftGridCellClick ), NULL, this );
	m_leftGridLayers->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_LAYER_SELECTION_BASE::OnLeftButtonReleased ), NULL, this );
	m_rightGridLayers->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LAYER_SELECTION_BASE::OnRightGridCellClick ), NULL, this );
	m_rightGridLayers->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_LAYER_SELECTION_BASE::OnLeftButtonReleased ), NULL, this );
}

DIALOG_LAYER_SELECTION_BASE::~DIALOG_LAYER_SELECTION_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LAYER_SELECTION_BASE::OnMouseMove ) );
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

	m_leftGridLayers = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

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


	bSizerUpper->Add( bSizerLeft, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	m_staticTextBottomLayer = new wxStaticText( this, wxID_ANY, _("Bottom/Back layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBottomLayer->Wrap( -1 );
	bSizerRight->Add( m_staticTextBottomLayer, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_rightGridLayers = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

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


	bSizerUpper->Add( bSizerRight, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	m_addToPresetsButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer7->Add( m_addToPresetsButton, 0, wxALL, 5 );


	bSizerUpper->Add( bSizer7, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticBoxSizer* m_presetsSizer;
	m_presetsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Copper Layer Pair Presets") ), wxVERTICAL );

	m_presetsGrid = new WX_GRID( m_presetsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_presetsGrid->CreateGrid( 0, 4 );
	m_presetsGrid->EnableEditing( true );
	m_presetsGrid->EnableGridLines( true );
	m_presetsGrid->EnableDragGridSize( false );
	m_presetsGrid->SetMargins( 5, 3 );

	// Columns
	m_presetsGrid->SetColSize( 0, 48 );
	m_presetsGrid->SetColSize( 1, 24 );
	m_presetsGrid->SetColSize( 2, 80 );
	m_presetsGrid->SetColSize( 3, 120 );
	m_presetsGrid->EnableDragColMove( false );
	m_presetsGrid->EnableDragColSize( false );
	m_presetsGrid->SetColLabelValue( 0, _("Enabled") );
	m_presetsGrid->SetColLabelValue( 1, wxEmptyString );
	m_presetsGrid->SetColLabelValue( 2, _("Layers") );
	m_presetsGrid->SetColLabelValue( 3, _("Label") );
	m_presetsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_presetsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_presetsGrid->EnableDragRowSize( false );
	m_presetsGrid->SetRowLabelSize( 0 );
	m_presetsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_presetsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_presetsSizer->Add( m_presetsGrid, 1, wxALL|wxEXPAND, 5 );

	m_deleteRowButton = new STD_BITMAP_BUTTON( m_presetsSizer->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_presetsSizer->Add( m_deleteRowButton, 0, wxBOTTOM|wxLEFT|wxTOP, 5 );


	bSizerUpper->Add( m_presetsSizer, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxALL|wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE::~DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE()
{
}

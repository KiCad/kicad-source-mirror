///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_fp_properties_3d_model_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_FP_PROPERTIES_3D_MODEL_BASE::PANEL_FP_PROPERTIES_3D_MODEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	bSizerMain3D = new wxBoxSizer( wxVERTICAL );

	m_splitter1 = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_splitter1->SetSashGravity( 0.5 );
	m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::m_splitter1OnIdle ), NULL, this );
	m_splitter1->SetMinimumPaneSize( 112 );

	m_upperPanel = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_modelsGrid = new WX_GRID( m_upperPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE );

	// Grid
	m_modelsGrid->CreateGrid( 2, 3 );
	m_modelsGrid->EnableEditing( true );
	m_modelsGrid->EnableGridLines( false );
	m_modelsGrid->EnableDragGridSize( false );
	m_modelsGrid->SetMargins( 0, 0 );

	// Columns
	m_modelsGrid->SetColSize( 0, 20 );
	m_modelsGrid->SetColSize( 1, 120 );
	m_modelsGrid->SetColSize( 2, 48 );
	m_modelsGrid->EnableDragColMove( false );
	m_modelsGrid->EnableDragColSize( false );
	m_modelsGrid->SetColLabelValue( 0, wxEmptyString );
	m_modelsGrid->SetColLabelValue( 1, _("3D Model(s)") );
	m_modelsGrid->SetColLabelValue( 2, _("Show") );
	m_modelsGrid->SetColLabelSize( 22 );
	m_modelsGrid->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Rows
	m_modelsGrid->EnableDragRowSize( false );
	m_modelsGrid->SetRowLabelSize( 0 );
	m_modelsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_modelsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizer4->Add( m_modelsGrid, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer3DButtons;
	bSizer3DButtons = new wxBoxSizer( wxHORIZONTAL );

	m_button3DShapeAdd = new STD_BITMAP_BUTTON( m_upperPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizer3DButtons->Add( m_button3DShapeAdd, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_button3DShapeBrowse = new STD_BITMAP_BUTTON( m_upperPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizer3DButtons->Add( m_button3DShapeBrowse, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bSizer3DButtons->Add( 20, 0, 0, 0, 5 );

	m_button3DShapeRemove = new STD_BITMAP_BUTTON( m_upperPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizer3DButtons->Add( m_button3DShapeRemove, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizer3DButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonConfig3DPaths = new wxButton( m_upperPanel, wxID_ANY, _("Configure Paths..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonConfig3DPaths, 0, wxALL, 5 );


	bSizer4->Add( bSizer3DButtons, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );


	m_upperPanel->SetSizer( bSizer4 );
	m_upperPanel->Layout();
	bSizer4->Fit( m_upperPanel );
	m_lowerPanel = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_LowerSizer3D = new wxBoxSizer( wxHORIZONTAL );


	m_lowerPanel->SetSizer( m_LowerSizer3D );
	m_lowerPanel->Layout();
	m_LowerSizer3D->Fit( m_lowerPanel );
	m_splitter1->SplitHorizontally( m_upperPanel, m_lowerPanel, 112 );
	bSizerMain3D->Add( m_splitter1, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerMain3D );
	this->Layout();

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::OnUpdateUI ) );
	m_modelsGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::On3DModelCellChanged ), NULL, this );
	m_modelsGrid->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::On3DModelSelected ), NULL, this );
	m_button3DShapeAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::OnAdd3DRow ), NULL, this );
	m_button3DShapeBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::OnAdd3DModel ), NULL, this );
	m_button3DShapeRemove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::OnRemove3DModel ), NULL, this );
	m_buttonConfig3DPaths->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::Cfg3DPath ), NULL, this );
}

PANEL_FP_PROPERTIES_3D_MODEL_BASE::~PANEL_FP_PROPERTIES_3D_MODEL_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::OnUpdateUI ) );
	m_modelsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::On3DModelCellChanged ), NULL, this );
	m_modelsGrid->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::On3DModelSelected ), NULL, this );
	m_button3DShapeAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::OnAdd3DRow ), NULL, this );
	m_button3DShapeBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::OnAdd3DModel ), NULL, this );
	m_button3DShapeRemove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::OnRemove3DModel ), NULL, this );
	m_buttonConfig3DPaths->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::Cfg3DPath ), NULL, this );

}

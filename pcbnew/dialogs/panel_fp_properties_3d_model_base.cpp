///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_fp_properties_3d_model_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_FP_PROPERTIES_3D_MODEL_BASE::PANEL_FP_PROPERTIES_3D_MODEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	bSizerMain3D = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );

	m_modelsGrid = new WX_GRID( sbSizer3->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE );

	// Grid
	m_modelsGrid->CreateGrid( 3, 3 );
	m_modelsGrid->EnableEditing( true );
	m_modelsGrid->EnableGridLines( false );
	m_modelsGrid->EnableDragGridSize( false );
	m_modelsGrid->SetMargins( 0, 0 );

	// Columns
	m_modelsGrid->SetColSize( 0, 20 );
	m_modelsGrid->SetColSize( 1, 650 );
	m_modelsGrid->SetColSize( 2, 65 );
	m_modelsGrid->EnableDragColMove( false );
	m_modelsGrid->EnableDragColSize( false );
	m_modelsGrid->SetColLabelSize( 22 );
	m_modelsGrid->SetColLabelValue( 0, wxEmptyString );
	m_modelsGrid->SetColLabelValue( 1, _("3D Model(s)") );
	m_modelsGrid->SetColLabelValue( 2, _("Show") );
	m_modelsGrid->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Rows
	m_modelsGrid->EnableDragRowSize( false );
	m_modelsGrid->SetRowLabelSize( 0 );
	m_modelsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_modelsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbSizer3->Add( m_modelsGrid, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizer3DButtons;
	bSizer3DButtons = new wxBoxSizer( wxHORIZONTAL );

	m_button3DShapeAdd = new wxBitmapButton( sbSizer3->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizer3DButtons->Add( m_button3DShapeAdd, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_button3DShapeBrowse = new wxBitmapButton( sbSizer3->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizer3DButtons->Add( m_button3DShapeBrowse, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bSizer3DButtons->Add( 20, 0, 0, 0, 5 );

	m_button3DShapeRemove = new wxBitmapButton( sbSizer3->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizer3DButtons->Add( m_button3DShapeRemove, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizer3DButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonConfig3DPaths = new wxButton( sbSizer3->GetStaticBox(), wxID_ANY, _("Configure Paths..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonConfig3DPaths, 0, wxALL, 5 );


	sbSizer3->Add( bSizer3DButtons, 0, wxEXPAND, 5 );


	bSizerMain3D->Add( sbSizer3, 4, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	bLowerSizer3D = new wxBoxSizer( wxHORIZONTAL );


	bSizerMain3D->Add( bLowerSizer3D, 11, wxEXPAND, 5 );


	this->SetSizer( bSizerMain3D );
	this->Layout();
	bSizerMain3D->Fit( this );

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

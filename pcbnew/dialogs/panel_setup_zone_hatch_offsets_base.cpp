///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_setup_zone_hatch_offsets_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_ZONE_HATCH_OFFSETS_BASE::PANEL_SETUP_ZONE_HATCH_OFFSETS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_staticTextLabel = new wxStaticText( this, wxID_ANY, _("Zone Hatched Fill Offsets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLabel->Wrap( -1 );
	mainSizer->Add( m_staticTextLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );


	mainSizer->Add( 0, 2, 0, 0, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM, 5 );

	m_layerOffsetsGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_layerOffsetsGrid->CreateGrid( 0, 3 );
	m_layerOffsetsGrid->EnableEditing( true );
	m_layerOffsetsGrid->EnableGridLines( true );
	m_layerOffsetsGrid->EnableDragGridSize( false );
	m_layerOffsetsGrid->SetMargins( 0, 0 );

	// Columns
	m_layerOffsetsGrid->SetColSize( 0, 160 );
	m_layerOffsetsGrid->SetColSize( 1, 120 );
	m_layerOffsetsGrid->SetColSize( 2, 120 );
	m_layerOffsetsGrid->EnableDragColMove( false );
	m_layerOffsetsGrid->EnableDragColSize( true );
	m_layerOffsetsGrid->SetColLabelValue( 0, _("Layer") );
	m_layerOffsetsGrid->SetColLabelValue( 1, _("X Offset") );
	m_layerOffsetsGrid->SetColLabelValue( 2, _("Y Offset") );
	m_layerOffsetsGrid->SetColLabelSize( 22 );
	m_layerOffsetsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_layerOffsetsGrid->EnableDragRowSize( true );
	m_layerOffsetsGrid->SetRowLabelSize( 0 );
	m_layerOffsetsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_layerOffsetsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	mainSizer->Add( m_layerOffsetsGrid, 0, wxALL, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

PANEL_SETUP_ZONE_HATCH_OFFSETS_BASE::~PANEL_SETUP_ZONE_HATCH_OFFSETS_BASE()
{
}

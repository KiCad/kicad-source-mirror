///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_fp_editor_graphics_defaults_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_FP_EDITOR_GRAPHICS_DEFAULTS_BASE::PANEL_FP_EDITOR_GRAPHICS_DEFAULTS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* defaultPropertiesSizer;
	defaultPropertiesSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticText* defaultPropertiesLabel;
	defaultPropertiesLabel = new wxStaticText( this, wxID_ANY, _("Default Properties for New Graphic Items"), wxDefaultPosition, wxDefaultSize, 0 );
	defaultPropertiesLabel->Wrap( -1 );
	defaultPropertiesSizer->Add( defaultPropertiesLabel, 0, wxEXPAND|wxRIGHT|wxLEFT, 8 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	defaultPropertiesSizer->Add( m_staticline1, 0, wxBOTTOM|wxEXPAND|wxTOP, 2 );


	defaultPropertiesSizer->Add( 0, 7, 0, wxEXPAND, 5 );

	m_graphicsGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	// Grid
	m_graphicsGrid->CreateGrid( 6, 5 );
	m_graphicsGrid->EnableEditing( true );
	m_graphicsGrid->EnableGridLines( true );
	m_graphicsGrid->EnableDragGridSize( false );
	m_graphicsGrid->SetMargins( 0, 0 );

	// Columns
	m_graphicsGrid->SetColSize( 0, 110 );
	m_graphicsGrid->SetColSize( 1, 100 );
	m_graphicsGrid->SetColSize( 2, 100 );
	m_graphicsGrid->SetColSize( 3, 100 );
	m_graphicsGrid->SetColSize( 4, 60 );
	m_graphicsGrid->EnableDragColMove( false );
	m_graphicsGrid->EnableDragColSize( true );
	m_graphicsGrid->SetColLabelValue( 0, _("Line Thickness") );
	m_graphicsGrid->SetColLabelValue( 1, _("Text Width") );
	m_graphicsGrid->SetColLabelValue( 2, _("Text Height") );
	m_graphicsGrid->SetColLabelValue( 3, _("Text Thickness") );
	m_graphicsGrid->SetColLabelValue( 4, _("Italic") );
	m_graphicsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_graphicsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_graphicsGrid->EnableDragRowSize( false );
	m_graphicsGrid->SetRowLabelValue( 0, _("Silk Layers") );
	m_graphicsGrid->SetRowLabelValue( 1, _("Copper Layers") );
	m_graphicsGrid->SetRowLabelValue( 2, _("Edge Cuts") );
	m_graphicsGrid->SetRowLabelValue( 3, _("Courtyards") );
	m_graphicsGrid->SetRowLabelValue( 4, _("Fab Layers") );
	m_graphicsGrid->SetRowLabelValue( 5, _("Other Layers") );
	m_graphicsGrid->SetRowLabelSize( 125 );
	m_graphicsGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_graphicsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	defaultPropertiesSizer->Add( m_graphicsGrid, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 1 );


	bSizerMargins->Add( defaultPropertiesSizer, 0, wxEXPAND|wxTOP, 5 );


	bSizerMargins->Add( 0, 20, 0, wxEXPAND, 5 );


	bSizerMain->Add( bSizerMargins, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
}

PANEL_FP_EDITOR_GRAPHICS_DEFAULTS_BASE::~PANEL_FP_EDITOR_GRAPHICS_DEFAULTS_BASE()
{
}

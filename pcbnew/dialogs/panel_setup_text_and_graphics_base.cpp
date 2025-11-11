///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_setup_text_and_graphics_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_TEXT_AND_GRAPHICS_BASE::PANEL_SETUP_TEXT_AND_GRAPHICS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_gridSizer = new wxBoxSizer( wxVERTICAL );

	m_staticTextDefProp = new wxStaticText( this, wxID_ANY, _("Default Properties for New Graphics and Text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefProp->Wrap( -1 );
	m_gridSizer->Add( m_staticTextDefProp, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 13 );


	m_gridSizer->Add( 0, 2, 0, 0, 5 );

	m_staticline11 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_gridSizer->Add( m_staticline11, 0, wxEXPAND|wxBOTTOM, 5 );


	m_gridSizer->Add( 0, 3, 0, wxEXPAND, 5 );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	// Grid
	m_grid->CreateGrid( 6, 6 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 140 );
	m_grid->SetColSize( 1, 140 );
	m_grid->SetColSize( 2, 140 );
	m_grid->SetColSize( 3, 140 );
	m_grid->SetColSize( 4, 80 );
	m_grid->SetColSize( 5, 120 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelValue( 0, _("Line Thickness") );
	m_grid->SetColLabelValue( 1, _("Text Width") );
	m_grid->SetColLabelValue( 2, _("Text Height") );
	m_grid->SetColLabelValue( 3, _("Text Thickness") );
	m_grid->SetColLabelValue( 4, _("Italic") );
	m_grid->SetColLabelValue( 5, _("Keep Upright") );
	m_grid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelValue( 0, _("Silk Layers") );
	m_grid->SetRowLabelValue( 1, _("Copper Layers") );
	m_grid->SetRowLabelValue( 2, _("Edge Cuts") );
	m_grid->SetRowLabelValue( 3, _("Courtyards") );
	m_grid->SetRowLabelValue( 4, _("Fab Layers") );
	m_grid->SetRowLabelValue( 5, _("Other Layers") );
	m_grid->SetRowLabelSize( 132 );
	m_grid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bMargins->Add( m_grid, 0, wxBOTTOM, 15 );


	m_gridSizer->Add( bMargins, 1, wxEXPAND|wxLEFT, 5 );


	m_mainSizer->Add( m_gridSizer, 0, wxEXPAND, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );
}

PANEL_SETUP_TEXT_AND_GRAPHICS_BASE::~PANEL_SETUP_TEXT_AND_GRAPHICS_BASE()
{
}

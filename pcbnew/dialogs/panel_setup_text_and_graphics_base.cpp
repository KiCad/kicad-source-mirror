///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_setup_text_and_graphics_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_TEXT_AND_GRAPHICS_BASE::PANEL_SETUP_TEXT_AND_GRAPHICS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_gridSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Default properties for new graphic items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	m_gridSizer->Add( m_staticText1, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	// Grid
	m_grid->CreateGrid( 5, 6 );
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
	m_grid->SetColLabelSize( 24 );
	m_grid->SetColLabelValue( 0, _("Line Thickness") );
	m_grid->SetColLabelValue( 1, _("Text Width") );
	m_grid->SetColLabelValue( 2, _("Text Height") );
	m_grid->SetColLabelValue( 3, _("Text Thickness") );
	m_grid->SetColLabelValue( 4, _("Italic") );
	m_grid->SetColLabelValue( 5, _("Keep Upright") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 132 );
	m_grid->SetRowLabelValue( 0, _("Silk Layers") );
	m_grid->SetRowLabelValue( 1, _("Copper Layers") );
	m_grid->SetRowLabelValue( 2, _("Edge Cuts") );
	m_grid->SetRowLabelValue( 3, _("Courtyards") );
	m_grid->SetRowLabelValue( 4, _("Other Layers") );
	m_grid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetToolTip( _("Net Class parameters") );

	m_gridSizer->Add( m_grid, 0, wxBOTTOM|wxLEFT, 20 );


	m_gridSizer->Add( 0, 0, 0, wxEXPAND|wxTOP, 5 );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Default properties for new dimension objects:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_gridSizer->Add( m_staticText2, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_dimensionUnitsLabel = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dimensionUnitsLabel->Wrap( -1 );
	fgSizer1->Add( m_dimensionUnitsLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_dimensionUnitsChoices[] = { _("Inches"), _("Mils"), _("Millimeters") };
	int m_dimensionUnitsNChoices = sizeof( m_dimensionUnitsChoices ) / sizeof( wxString );
	m_dimensionUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dimensionUnitsNChoices, m_dimensionUnitsChoices, 0 );
	m_dimensionUnits->SetSelection( 0 );
	fgSizer1->Add( m_dimensionUnits, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_dimensionPrecisionLabel = new wxStaticText( this, wxID_ANY, _("Precision:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dimensionPrecisionLabel->Wrap( -1 );
	fgSizer1->Add( m_dimensionPrecisionLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_dimensionPrecisionChoices[] = { _("0.01 mm / 1 mil"), _("0.001 mm / 0.1 mil"), _("0.0001mm / 0.01 mil") };
	int m_dimensionPrecisionNChoices = sizeof( m_dimensionPrecisionChoices ) / sizeof( wxString );
	m_dimensionPrecision = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dimensionPrecisionNChoices, m_dimensionPrecisionChoices, 0 );
	m_dimensionPrecision->SetSelection( 0 );
	fgSizer1->Add( m_dimensionPrecision, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	m_gridSizer->Add( fgSizer1, 1, wxEXPAND|wxBOTTOM|wxLEFT, 20 );


	mainSizer->Add( m_gridSizer, 0, wxRIGHT|wxLEFT, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

PANEL_SETUP_TEXT_AND_GRAPHICS_BASE::~PANEL_SETUP_TEXT_AND_GRAPHICS_BASE()
{
}

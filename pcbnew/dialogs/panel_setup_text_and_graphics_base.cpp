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

	m_staticTextDefProp = new wxStaticText( this, wxID_ANY, _("Default properties for new graphic items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefProp->Wrap( -1 );
	m_gridSizer->Add( m_staticTextDefProp, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

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
	m_grid->SetRowLabelValue( 4, _("Fab Layers") );
	m_grid->SetRowLabelValue( 5, _("Other Layers") );
	m_grid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_gridSizer->Add( m_grid, 0, wxEXPAND|wxBOTTOM|wxLEFT, 5 );


	m_gridSizer->Add( 0, 0, 0, wxEXPAND|wxTOP, 5 );

	m_staticTextDefPropDim = new wxStaticText( this, wxID_ANY, _("Default properties for new dimension objects:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefPropDim->Wrap( -1 );
	m_gridSizer->Add( m_staticTextDefPropDim, 0, wxALL, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxHORIZONTAL );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

	m_lblDimensionUnits = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblDimensionUnits->Wrap( -1 );
	gbSizer1->Add( m_lblDimensionUnits, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_dimensionUnitsChoices[] = { _("Inches"), _("Mils"), _("Millimeters"), _("Automatic") };
	int m_dimensionUnitsNChoices = sizeof( m_dimensionUnitsChoices ) / sizeof( wxString );
	m_dimensionUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dimensionUnitsNChoices, m_dimensionUnitsChoices, 0 );
	m_dimensionUnits->SetSelection( 0 );
	m_dimensionUnits->SetToolTip( _("Default units for dimensions (\"automatic\" to follow the chosen UI units)") );

	gbSizer1->Add( m_dimensionUnits, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( 100, 0, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblTextPositionMode = new wxStaticText( this, wxID_ANY, _("Text position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextPositionMode->Wrap( -1 );
	gbSizer1->Add( m_lblTextPositionMode, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_dimensionTextPositionModeChoices[] = { _("Outside"), _("Inline") };
	int m_dimensionTextPositionModeNChoices = sizeof( m_dimensionTextPositionModeChoices ) / sizeof( wxString );
	m_dimensionTextPositionMode = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dimensionTextPositionModeNChoices, m_dimensionTextPositionModeChoices, 0 );
	m_dimensionTextPositionMode->SetSelection( 0 );
	m_dimensionTextPositionMode->SetToolTip( _("Where to position the dimension text relative to the dimension line") );

	gbSizer1->Add( m_dimensionTextPositionMode, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblDimensionUnitsFormat = new wxStaticText( this, wxID_ANY, _("Units format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblDimensionUnitsFormat->Wrap( -1 );
	gbSizer1->Add( m_lblDimensionUnitsFormat, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_dimensionUnitsFormatChoices[] = { _("1234"), _("1234 mm"), _("1234 (mm)") };
	int m_dimensionUnitsFormatNChoices = sizeof( m_dimensionUnitsFormatChoices ) / sizeof( wxString );
	m_dimensionUnitsFormat = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dimensionUnitsFormatNChoices, m_dimensionUnitsFormatChoices, 0 );
	m_dimensionUnitsFormat->SetSelection( 1 );
	gbSizer1->Add( m_dimensionUnitsFormat, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_dimensionTextKeepAligned = new wxCheckBox( this, wxID_ANY, _("Keep text aligned"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dimensionTextKeepAligned->SetToolTip( _("When checked, dimension text will be kept aligned with dimension lines") );

	gbSizer1->Add( m_dimensionTextKeepAligned, wxGBPosition( 1, 3 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_lblDimensionPrecision = new wxStaticText( this, wxID_ANY, _("Precision:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblDimensionPrecision->Wrap( -1 );
	gbSizer1->Add( m_lblDimensionPrecision, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_dimensionPrecisionChoices[] = { _("0"), _("0.0"), _("0.00"), _("0.000"), _("0.0000"), _("0.00000") };
	int m_dimensionPrecisionNChoices = sizeof( m_dimensionPrecisionChoices ) / sizeof( wxString );
	m_dimensionPrecision = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dimensionPrecisionNChoices, m_dimensionPrecisionChoices, 0 );
	m_dimensionPrecision->SetSelection( 4 );
	m_dimensionPrecision->SetToolTip( _("How many digits of precision to show") );

	gbSizer1->Add( m_dimensionPrecision, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblArrowLength = new wxStaticText( this, wxID_ANY, _("Arrow length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblArrowLength->Wrap( -1 );
	gbSizer1->Add( m_lblArrowLength, wxGBPosition( 2, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_dimensionArrowLength = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_dimensionArrowLength, wxGBPosition( 2, 4 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_arrowLengthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_arrowLengthUnits->Wrap( -1 );
	gbSizer1->Add( m_arrowLengthUnits, wxGBPosition( 2, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_dimensionSuppressZeroes = new wxCheckBox( this, wxID_ANY, _("Suppress trailing zeroes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dimensionSuppressZeroes->SetToolTip( _("When checked, \"1.2300\" will be rendered as \"1.23\" even if precision is set to show more digits") );

	gbSizer1->Add( m_dimensionSuppressZeroes, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_lblExtensionOffset = new wxStaticText( this, wxID_ANY, _("Extension line offset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblExtensionOffset->Wrap( -1 );
	gbSizer1->Add( m_lblExtensionOffset, wxGBPosition( 3, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_dimensionExtensionOffset = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_dimensionExtensionOffset, wxGBPosition( 3, 4 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_dimensionExtensionOffsetUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dimensionExtensionOffsetUnits->Wrap( -1 );
	gbSizer1->Add( m_dimensionExtensionOffsetUnits, wxGBPosition( 3, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_gridSizer->Add( gbSizer1, 1, wxBOTTOM|wxEXPAND|wxLEFT, 20 );


	mainSizer->Add( m_gridSizer, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

PANEL_SETUP_TEXT_AND_GRAPHICS_BASE::~PANEL_SETUP_TEXT_AND_GRAPHICS_BASE()
{
}

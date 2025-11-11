///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_dimensions_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_DIMENSIONS_BASE::PANEL_SETUP_DIMENSIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_staticTextDefPropDim = new wxStaticText( this, wxID_ANY, _("Default Properties for New Dimension Objects"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefPropDim->Wrap( -1 );
	mainSizer->Add( m_staticTextDefPropDim, 0, wxTOP|wxRIGHT|wxLEFT, 13 );


	mainSizer->Add( 0, 2, 0, 0, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 5 );
	gbSizer1->SetFlexibleDirection( wxVERTICAL );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblDimensionUnits = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblDimensionUnits->Wrap( -1 );
	gbSizer1->Add( m_lblDimensionUnits, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	wxString m_dimensionUnitsChoices[] = { _("Inches"), _("Mils"), _("Millimeters"), _("Automatic") };
	int m_dimensionUnitsNChoices = sizeof( m_dimensionUnitsChoices ) / sizeof( wxString );
	m_dimensionUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dimensionUnitsNChoices, m_dimensionUnitsChoices, 0 );
	m_dimensionUnits->SetSelection( 0 );
	m_dimensionUnits->SetToolTip( _("Default units for dimensions (\"automatic\" to follow the chosen UI units)") );

	gbSizer1->Add( m_dimensionUnits, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


	gbSizer1->Add( 0, 0, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblTextPositionMode = new wxStaticText( this, wxID_ANY, _("Text position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblTextPositionMode->Wrap( -1 );
	gbSizer1->Add( m_lblTextPositionMode, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 4 );

	wxString m_dimensionTextPositionModeChoices[] = { _("Outside"), _("Inline") };
	int m_dimensionTextPositionModeNChoices = sizeof( m_dimensionTextPositionModeChoices ) / sizeof( wxString );
	m_dimensionTextPositionMode = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dimensionTextPositionModeNChoices, m_dimensionTextPositionModeChoices, 0 );
	m_dimensionTextPositionMode->SetSelection( 0 );
	m_dimensionTextPositionMode->SetToolTip( _("Where to position the dimension text relative to the dimension line") );

	gbSizer1->Add( m_dimensionTextPositionMode, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_lblDimensionUnitsFormat = new wxStaticText( this, wxID_ANY, _("Units format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblDimensionUnitsFormat->Wrap( -1 );
	gbSizer1->Add( m_lblDimensionUnitsFormat, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	wxString m_dimensionUnitsFormatChoices[] = { _("1234"), _("1234 mm"), _("1234 (mm)") };
	int m_dimensionUnitsFormatNChoices = sizeof( m_dimensionUnitsFormatChoices ) / sizeof( wxString );
	m_dimensionUnitsFormat = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dimensionUnitsFormatNChoices, m_dimensionUnitsFormatChoices, 0 );
	m_dimensionUnitsFormat->SetSelection( 1 );
	gbSizer1->Add( m_dimensionUnitsFormat, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_dimensionTextKeepAligned = new wxCheckBox( this, wxID_ANY, _("Keep text aligned"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dimensionTextKeepAligned->SetToolTip( _("When checked, dimension text will be kept aligned with dimension lines") );

	gbSizer1->Add( m_dimensionTextKeepAligned, wxGBPosition( 1, 3 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND, 5 );

	m_lblDimensionPrecision = new wxStaticText( this, wxID_ANY, _("Precision:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblDimensionPrecision->Wrap( -1 );
	gbSizer1->Add( m_lblDimensionPrecision, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	wxString m_dimensionPrecisionChoices[] = { _("0"), _("0.0"), _("0.00"), _("0.000"), _("0.0000"), _("0.00000") };
	int m_dimensionPrecisionNChoices = sizeof( m_dimensionPrecisionChoices ) / sizeof( wxString );
	m_dimensionPrecision = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dimensionPrecisionNChoices, m_dimensionPrecisionChoices, 0 );
	m_dimensionPrecision->SetSelection( 4 );
	m_dimensionPrecision->SetToolTip( _("How many digits of precision to show") );

	gbSizer1->Add( m_dimensionPrecision, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_lblArrowLength = new wxStaticText( this, wxID_ANY, _("Arrow length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblArrowLength->Wrap( -1 );
	gbSizer1->Add( m_lblArrowLength, wxGBPosition( 2, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_dimensionArrowLength = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_dimensionArrowLength, wxGBPosition( 2, 4 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_arrowLengthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_arrowLengthUnits->Wrap( -1 );
	gbSizer1->Add( m_arrowLengthUnits, wxGBPosition( 2, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_dimensionSuppressZeroes = new wxCheckBox( this, wxID_ANY, _("Suppress trailing zeroes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dimensionSuppressZeroes->SetToolTip( _("When checked, \"1.2300\" will be rendered as \"1.23\" even if precision is set to show more digits") );

	gbSizer1->Add( m_dimensionSuppressZeroes, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_lblExtensionOffset = new wxStaticText( this, wxID_ANY, _("Extension line offset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblExtensionOffset->Wrap( -1 );
	gbSizer1->Add( m_lblExtensionOffset, wxGBPosition( 3, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_dimensionExtensionOffset = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_dimensionExtensionOffset, wxGBPosition( 3, 4 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_dimensionExtensionOffsetUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dimensionExtensionOffsetUnits->Wrap( -1 );
	gbSizer1->Add( m_dimensionExtensionOffsetUnits, wxGBPosition( 3, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	mainSizer->Add( gbSizer1, 1, wxEXPAND|wxALL, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

PANEL_SETUP_DIMENSIONS_BASE::~PANEL_SETUP_DIMENSIONS_BASE()
{
}

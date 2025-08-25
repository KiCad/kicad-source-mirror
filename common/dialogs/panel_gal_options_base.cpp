///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_gal_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GAL_OPTIONS_BASE::PANEL_GAL_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Grid Display"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	mainSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizerGridStyle;
	bSizerGridStyle = new wxBoxSizer( wxHORIZONTAL );

	m_gridStyleLabel = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gridStyleLabel->Wrap( -1 );
	bSizerGridStyle->Add( m_gridStyleLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_rbDots = new wxRadioButton( this, wxID_ANY, _("Dots"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizerGridStyle->Add( m_rbDots, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rbLines = new wxRadioButton( this, wxID_ANY, _("Lines"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerGridStyle->Add( m_rbLines, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rbCrosses = new wxRadioButton( this, wxID_ANY, _("Small crosses"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerGridStyle->Add( m_rbCrosses, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	mainSizer->Add( bSizerGridStyle, 0, wxEXPAND|wxALL, 5 );

	wxGridBagSizer* gbGridSettings;
	gbGridSettings = new wxGridBagSizer( 5, 5 );
	gbGridSettings->SetFlexibleDirection( wxHORIZONTAL );
	gbGridSettings->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	l_gridLineWidth = new wxStaticText( this, wxID_ANY, _("Grid thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridLineWidth->Wrap( -1 );
	gbGridSettings->Add( l_gridLineWidth, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxArrayString m_gridLineWidthChoices;
	m_gridLineWidth = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_gridLineWidthChoices, 0 );
	m_gridLineWidth->SetSelection( 0 );
	gbGridSettings->Add( m_gridLineWidth, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	l_gridLineWidthUnits = new wxStaticText( this, wxID_ANY, _("pixels"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridLineWidthUnits->Wrap( -1 );
	gbGridSettings->Add( l_gridLineWidthUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	l_gridMinSpacing = new wxStaticText( this, wxID_ANY, _("Minimum grid spacing:"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridMinSpacing->Wrap( -1 );
	gbGridSettings->Add( l_gridMinSpacing, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_gridMinSpacing = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 5, 200, 10 );
	gbGridSettings->Add( m_gridMinSpacing, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	l_gridMinSpacingUnits = new wxStaticText( this, wxID_ANY, _("pixels"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridMinSpacingUnits->Wrap( -1 );
	gbGridSettings->Add( l_gridMinSpacingUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	l_gridSnapOptions = new wxStaticText( this, wxID_ANY, _("Snap to grid:"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridSnapOptions->Wrap( -1 );
	gbGridSettings->Add( l_gridSnapOptions, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_gridSnapOptionsChoices[] = { _("Always"), _("When grid shown"), _("Never") };
	int m_gridSnapOptionsNChoices = sizeof( m_gridSnapOptionsChoices ) / sizeof( wxString );
	m_gridSnapOptions = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_gridSnapOptionsNChoices, m_gridSnapOptionsChoices, 0 );
	m_gridSnapOptions->SetSelection( 0 );
	gbGridSettings->Add( m_gridSnapOptions, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	gbGridSettings->AddGrowableCol( 1 );

	mainSizer->Add( gbGridSettings, 0, wxEXPAND|wxALL, 5 );


	mainSizer->Add( 0, 5, 0, 0, 5 );

	m_stGridLabel = new wxStaticText( this, wxID_ANY, _("Cursor"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stGridLabel->Wrap( -1 );
	mainSizer->Add( m_stGridLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 1, 3, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rbSmallCrosshairs = new wxRadioButton( this, wxID_ANY, _("Small crosshairs"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizer1->Add( m_rbSmallCrosshairs, 0, wxTOP|wxLEFT, 5 );

	m_rbFullWindowCrosshairs = new wxRadioButton( this, wxID_ANY, _("Full window crosshairs"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_rbFullWindowCrosshairs, 0, wxLEFT, 5 );

	m_rb45DegreeCrosshairs = new wxRadioButton( this, wxID_ANY, _("45 degree crosshairs"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_rb45DegreeCrosshairs, 0, wxLEFT, 5 );


	fgSizer1->Add( 0, 8, 0, wxEXPAND, 5 );

	m_forceCursorDisplay = new wxCheckBox( this, wxID_ANY, _("Always show crosshairs"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_forceCursorDisplay, 0, wxLEFT, 5 );


	mainSizer->Add( fgSizer1, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

PANEL_GAL_OPTIONS_BASE::~PANEL_GAL_OPTIONS_BASE()
{
}

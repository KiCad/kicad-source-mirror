///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "gal_options_panel_base.h"

///////////////////////////////////////////////////////////////////////////

GAL_OPTIONS_PANEL_BASE::GAL_OPTIONS_PANEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* sLeftSizer;
	sLeftSizer = new wxBoxSizer( wxVERTICAL );

	wxString m_renderingEngineChoices[] = { _("Accelerated graphics"), _("Fallback graphics") };
	int m_renderingEngineNChoices = sizeof( m_renderingEngineChoices ) / sizeof( wxString );
	m_renderingEngine = new wxRadioBox( this, wxID_ANY, _("Rendering Engine"), wxDefaultPosition, wxDefaultSize, m_renderingEngineNChoices, m_renderingEngineChoices, 1, wxRA_SPECIFY_COLS );
	m_renderingEngine->SetSelection( 0 );
	sLeftSizer->Add( m_renderingEngine, 0, wxALL, 5 );


	mainSizer->Add( sLeftSizer, 0, wxEXPAND, 5 );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Grid Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	mainSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxString m_gridStyleChoices[] = { _("Dots"), _("Lines"), _("Small crosses") };
	int m_gridStyleNChoices = sizeof( m_gridStyleChoices ) / sizeof( wxString );
	m_gridStyle = new wxRadioBox( this, wxID_ANY, _("Grid Style"), wxDefaultPosition, wxDefaultSize, m_gridStyleNChoices, m_gridStyleChoices, 1, wxRA_SPECIFY_COLS );
	m_gridStyle->SetSelection( 0 );
	mainSizer->Add( m_gridStyle, 0, wxALL, 5 );

	wxFlexGridSizer* fgGridSettingsGrid;
	fgGridSettingsGrid = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgGridSettingsGrid->AddGrowableCol( 1 );
	fgGridSettingsGrid->SetFlexibleDirection( wxBOTH );
	fgGridSettingsGrid->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	l_gridLineWidth = new wxStaticText( this, wxID_ANY, _("Grid thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridLineWidth->Wrap( -1 );
	fgGridSettingsGrid->Add( l_gridLineWidth, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_gridLineWidthChoices;
	m_gridLineWidth = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_gridLineWidthChoices, 0 );
	m_gridLineWidth->SetSelection( 0 );
	fgGridSettingsGrid->Add( m_gridLineWidth, 0, wxALL|wxEXPAND, 5 );

	l_gridLineWidthUnits = new wxStaticText( this, wxID_ANY, _("px"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridLineWidthUnits->Wrap( -1 );
	fgGridSettingsGrid->Add( l_gridLineWidthUnits, 0, wxALL, 5 );

	l_gridMinSpacing = new wxStaticText( this, wxID_ANY, _("Min grid spacing:"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridMinSpacing->Wrap( -1 );
	fgGridSettingsGrid->Add( l_gridMinSpacing, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_gridMinSpacing = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 5, 200, 10 );
	fgGridSettingsGrid->Add( m_gridMinSpacing, 0, wxALL|wxEXPAND, 5 );

	l_gridMinSpacingUnits = new wxStaticText( this, wxID_ANY, _("px"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridMinSpacingUnits->Wrap( -1 );
	fgGridSettingsGrid->Add( l_gridMinSpacingUnits, 0, wxALL, 5 );

	l_gridSnapOptions = new wxStaticText( this, wxID_ANY, _("Snap to grid:"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridSnapOptions->Wrap( -1 );
	fgGridSettingsGrid->Add( l_gridSnapOptions, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_gridSnapOptionsChoices[] = { _("Always"), _("When grid shown"), _("Never") };
	int m_gridSnapOptionsNChoices = sizeof( m_gridSnapOptionsChoices ) / sizeof( wxString );
	m_gridSnapOptions = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_gridSnapOptionsNChoices, m_gridSnapOptionsChoices, 0 );
	m_gridSnapOptions->SetSelection( 0 );
	fgGridSettingsGrid->Add( m_gridSnapOptions, 0, wxALL|wxEXPAND, 5 );


	fgGridSettingsGrid->Add( 0, 0, 1, wxEXPAND, 5 );


	mainSizer->Add( fgGridSettingsGrid, 0, wxEXPAND, 5 );


	mainSizer->Add( 0, 15, 0, 0, 5 );

	m_stGridLabel = new wxStaticText( this, wxID_ANY, _("Cursor Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stGridLabel->Wrap( -1 );
	mainSizer->Add( m_stGridLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* sCursorSettings;
	sCursorSettings = new wxBoxSizer( wxVERTICAL );

	wxString m_cursorShapeChoices[] = { _("Small crosshair"), _("Full window crosshair") };
	int m_cursorShapeNChoices = sizeof( m_cursorShapeChoices ) / sizeof( wxString );
	m_cursorShape = new wxRadioBox( this, wxID_ANY, _("Cursor Shape"), wxDefaultPosition, wxDefaultSize, m_cursorShapeNChoices, m_cursorShapeChoices, 1, wxRA_SPECIFY_COLS );
	m_cursorShape->SetSelection( 0 );
	m_cursorShape->SetToolTip( _("Cursor shape for drawing, placement and movement tools") );

	sCursorSettings->Add( m_cursorShape, 0, wxALL, 5 );

	m_forceCursorDisplay = new wxCheckBox( this, wxID_ANY, _("Always show crosshairs"), wxDefaultPosition, wxDefaultSize, 0 );
	sCursorSettings->Add( m_forceCursorDisplay, 0, wxALL, 5 );


	mainSizer->Add( sCursorSettings, 1, wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
}

GAL_OPTIONS_PANEL_BASE::~GAL_OPTIONS_PANEL_BASE()
{
}

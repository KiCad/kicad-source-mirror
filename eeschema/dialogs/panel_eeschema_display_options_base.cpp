///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/font_choice.h"

#include "panel_eeschema_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE::PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	m_galOptionsSizer = new wxBoxSizer( wxVERTICAL );


	bSizer9->Add( m_galOptionsSizer, 0, wxEXPAND, 0 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );


	bSizer8->Add( 0, 7, 0, wxEXPAND, 5 );

	m_crossprobeLabel = new wxStaticText( this, wxID_ANY, _("Cross-probing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_crossprobeLabel->Wrap( -1 );
	bSizer8->Add( m_crossprobeLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer8->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bCrossProbingSizer;
	bCrossProbingSizer = new wxBoxSizer( wxVERTICAL );

	m_checkCrossProbeOnSelection = new wxCheckBox( this, wxID_ANY, _("Select/highlight objects corresponding to PCB selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeOnSelection->SetValue(true);
	m_checkCrossProbeOnSelection->SetToolTip( _("Highlight symbols corresponding to selected footprints") );

	bCrossProbingSizer->Add( m_checkCrossProbeOnSelection, 0, wxALL, 5 );

	m_checkCrossProbeCenter = new wxCheckBox( this, wxID_ANY, _("Center view on cross-probed items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeCenter->SetValue(true);
	m_checkCrossProbeCenter->SetToolTip( _("Ensures that cross-probed symbols are visible in the current view") );

	bCrossProbingSizer->Add( m_checkCrossProbeCenter, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeZoom = new wxCheckBox( this, wxID_ANY, _("Zoom to fit cross-probed items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeZoom->SetValue(true);
	bCrossProbingSizer->Add( m_checkCrossProbeZoom, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeAutoHighlight = new wxCheckBox( this, wxID_ANY, _("Highlight cross-probed nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeAutoHighlight->SetValue(true);
	m_checkCrossProbeAutoHighlight->SetToolTip( _("Highlight nets when they are highlighted in the PCB editor") );

	bCrossProbingSizer->Add( m_checkCrossProbeAutoHighlight, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeFlash = new wxCheckBox( this, wxID_ANY, _("Flash cross-probed selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeFlash->SetToolTip( _("Temporarily flash the newly cross-probed selection 3 times") );
	bCrossProbingSizer->Add( m_checkCrossProbeFlash, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer8->Add( bCrossProbingSizer, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bSizer9->Add( bSizer8, 0, wxEXPAND, 5 );


	bPanelSizer->Add( bSizer9, 0, wxEXPAND|wxRIGHT, 5 );


	bPanelSizer->Add( 20, 0, 0, 0, 5 );

	wxBoxSizer* bRightColumn;
	bRightColumn = new wxBoxSizer( wxVERTICAL );

	m_appearanceLabel = new wxStaticText( this, wxID_ANY, _("Appearance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_appearanceLabel->Wrap( -1 );
	bRightColumn->Add( m_appearanceLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightColumn->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bAppearanceSizer;
	bAppearanceSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerDefaultFont;
	bSizerDefaultFont = new wxBoxSizer( wxHORIZONTAL );

	m_defaultFontLabel = new wxStaticText( this, wxID_ANY, _("Default font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_defaultFontLabel->Wrap( -1 );
	bSizerDefaultFont->Add( m_defaultFontLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_defaultFontCtrlChoices[] = { _("KiCad Font") };
	int m_defaultFontCtrlNChoices = sizeof( m_defaultFontCtrlChoices ) / sizeof( wxString );
	m_defaultFontCtrl = new FONT_CHOICE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_defaultFontCtrlNChoices, m_defaultFontCtrlChoices, 0 );
	m_defaultFontCtrl->SetSelection( 0 );
	bSizerDefaultFont->Add( m_defaultFontCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bAppearanceSizer->Add( bSizerDefaultFont, 1, wxEXPAND|wxTOP, 5 );

	m_checkShowHiddenPins = new wxCheckBox( this, wxID_ANY, _("S&how hidden pins"), wxDefaultPosition, wxDefaultSize, 0 );
	bAppearanceSizer->Add( m_checkShowHiddenPins, 0, wxEXPAND|wxALL, 5 );

	m_checkShowHiddenFields = new wxCheckBox( this, wxID_ANY, _("Show hidden fields"), wxDefaultPosition, wxDefaultSize, 0 );
	bAppearanceSizer->Add( m_checkShowHiddenFields, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_checkShowDirectiveLabels = new wxCheckBox( this, wxID_ANY, _("Show directive labels"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowDirectiveLabels->SetValue(true);
	bAppearanceSizer->Add( m_checkShowDirectiveLabels, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkShowERCErrors = new wxCheckBox( this, wxID_ANY, _("Show ERC errors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowERCErrors->SetValue(true);
	bAppearanceSizer->Add( m_checkShowERCErrors, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkShowERCWarnings = new wxCheckBox( this, wxID_ANY, _("Show ERC warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowERCWarnings->SetValue(true);
	bAppearanceSizer->Add( m_checkShowERCWarnings, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkShowERCExclusions = new wxCheckBox( this, wxID_ANY, _("Show ERC exclusions"), wxDefaultPosition, wxDefaultSize, 0 );
	bAppearanceSizer->Add( m_checkShowERCExclusions, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbMarkSimExclusions = new wxCheckBox( this, wxID_ANY, _("Mark items which are excluded from simulation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbMarkSimExclusions->SetValue(true);
	bAppearanceSizer->Add( m_cbMarkSimExclusions, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkShowOPVoltages = new wxCheckBox( this, wxID_ANY, _("Show OP voltages"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowOPVoltages->SetValue(true);
	bAppearanceSizer->Add( m_checkShowOPVoltages, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkShowOPCurrents = new wxCheckBox( this, wxID_ANY, _("Show OP currents"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowOPCurrents->SetValue(true);
	bAppearanceSizer->Add( m_checkShowOPCurrents, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkShowPinAltModeIcons = new wxCheckBox( this, wxID_ANY, _("Show pin alternate mode indicator icons"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowPinAltModeIcons->SetValue(true);
	bAppearanceSizer->Add( m_checkShowPinAltModeIcons, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_checkPageLimits = new wxCheckBox( this, wxID_ANY, _("Show page limi&ts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkPageLimits->SetValue(true);
	bAppearanceSizer->Add( m_checkPageLimits, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRightColumn->Add( bAppearanceSizer, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bRightColumn->Add( 0, 7, 0, wxEXPAND, 5 );

	m_selectionLabel = new wxStaticText( this, wxID_ANY, _("Selection && Highlighting"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selectionLabel->Wrap( -1 );
	bRightColumn->Add( m_selectionLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightColumn->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSelectionSizer;
	bSelectionSizer = new wxBoxSizer( wxVERTICAL );

	m_checkSelDrawChildItems = new wxCheckBox( this, wxID_ANY, _("Draw selected child items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkSelDrawChildItems->SetValue(true);
	bSelectionSizer->Add( m_checkSelDrawChildItems, 0, wxEXPAND|wxALL, 5 );

	m_checkSelFillShapes = new wxCheckBox( this, wxID_ANY, _("Fill selected shapes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSelectionSizer->Add( m_checkSelFillShapes, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 2, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,10 ) );

	m_selWidthLabel = new wxStaticText( this, wxID_ANY, _("Selection thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_selWidthLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	m_selWidthCtrl = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT|wxSP_ARROW_KEYS, 0, 50, 3.000000, 1 );
	m_selWidthCtrl->SetDigits( 0 );
	gbSizer1->Add( m_selWidthCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

        m_highlightColorNote = new wxStaticText( this, wxID_ANY, _("(selection color can be edited in the \"Colors\" page)"), wxDefaultPosition, wxDefaultSize, 0 );
        m_highlightColorNote->Wrap( -1 );
        gbSizer1->Add( m_highlightColorNote, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxRIGHT|wxLEFT, 5 );

        m_collisionMarkerWidthLabel = new wxStaticText( this, wxID_ANY, _("Net collision marker width:"), wxDefaultPosition, wxDefaultSize, 0 );
        m_collisionMarkerWidthLabel->Wrap( -1 );
        gbSizer1->Add( m_collisionMarkerWidthLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

        m_collisionMarkerWidthCtrl = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT|wxSP_ARROW_KEYS, 1, 50, 4.000000, 1 );
        m_collisionMarkerWidthCtrl->SetDigits( 0 );
        gbSizer1->Add( m_collisionMarkerWidthCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

        m_highlightWidthLabel = new wxStaticText( this, wxID_ANY, _("Highlight thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
        m_highlightWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_highlightWidthLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_highlightWidthCtrl = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT|wxSP_ARROW_KEYS, 0, 50, 2.000000, 1 );
	m_highlightWidthCtrl->SetDigits( 0 );
	gbSizer1->Add( m_highlightWidthCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


	bSelectionSizer->Add( gbSizer1, 0, wxEXPAND|wxRIGHT, 5 );

	m_highlightNetclassColors = new wxCheckBox( this, wxID_ANY, _("Highlight netclass colors"), wxDefaultPosition, wxDefaultSize, 0 );
	bSelectionSizer->Add( m_highlightNetclassColors, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxGridBagSizer* gbSizer11;
	gbSizer11 = new wxGridBagSizer( 2, 5 );
	gbSizer11->SetFlexibleDirection( wxBOTH );
	gbSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	m_colorHighlightLabel = new wxStaticText( this, wxID_ANY, _("Color highlight thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_colorHighlightLabel->Wrap( -1 );
	gbSizer11->Add( m_colorHighlightLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	m_colHighlightThickness = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT|wxSP_ARROW_KEYS, 0, 50, 15.000000, 1 );
	m_colHighlightThickness->SetDigits( 0 );
	gbSizer11->Add( m_colHighlightThickness, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_colHighlightLabel2 = new wxStaticText( this, wxID_ANY, _("Color highlight opacity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_colHighlightLabel2->Wrap( -1 );
	gbSizer11->Add( m_colHighlightLabel2, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_colHighlightTransparency = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT|wxSP_ARROW_KEYS, 0, 100, 60.000000, 1 );
	m_colHighlightTransparency->SetDigits( 0 );
	gbSizer11->Add( m_colHighlightTransparency, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	bSelectionSizer->Add( gbSizer11, 0, wxEXPAND, 5 );


	bRightColumn->Add( bSelectionSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bPanelSizer->Add( bRightColumn, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE::~PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE()
{
}

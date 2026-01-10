///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"
#include "widgets/wx_infobar.h"

#include "panel_zone_properties_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_ZONE_PROPERTIES_BASE::PANEL_ZONE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_copperZoneInfoBar = new WX_INFOBAR( this );
	m_copperZoneInfoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_copperZoneInfoBar->SetEffectDuration( 500 );
	bSizer8->Add( m_copperZoneInfoBar, 0, wxEXPAND, 5 );

	wxBoxSizer* bPropertiesSizer;
	bPropertiesSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer8;
	gbSizer8 = new wxGridBagSizer( 3, 5 );
	gbSizer8->SetFlexibleDirection( wxBOTH );
	gbSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_zoneNameLabel = new wxStaticText( this, wxID_ANY, _("Zone name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_zoneNameLabel->Wrap( -1 );
	gbSizer8->Add( m_zoneNameLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcZoneName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer8->Add( m_tcZoneName, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_netLabel = new wxStaticText( this, wxID_ANY, _("Net name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_netLabel->Wrap( -1 );
	gbSizer8->Add( m_netLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_netSelector = new NET_SELECTOR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer8->Add( m_netSelector, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	gbSizer8->AddGrowableCol( 1 );

	bPropertiesSizer->Add( gbSizer8, 0, wxEXPAND|wxALL, 5 );

	m_cbLocked = new wxCheckBox( this, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	bPropertiesSizer->Add( m_cbLocked, 0, wxLEFT|wxRIGHT, 5 );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_clearancesPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bClearancesSizer;
	bClearancesSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizerClearances;
	gbSizerClearances = new wxGridBagSizer( 1, 1 );
	gbSizerClearances->SetFlexibleDirection( wxBOTH );
	gbSizerClearances->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizerClearances->SetEmptyCellSize( wxSize( -1,10 ) );

	m_clearanceLabel = new wxStaticText( m_clearancesPanel, wxID_ANY, _("Clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceLabel->Wrap( -1 );
	m_clearanceLabel->SetToolTip( _("Copper clearance for this zone (set to 0 to use the netclass clearance)") );

	gbSizerClearances->Add( m_clearanceLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_clearanceCtrl = new wxTextCtrl( m_clearancesPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerClearances->Add( m_clearanceCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALL, 4 );

	m_clearanceUnits = new wxStaticText( m_clearancesPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceUnits->Wrap( -1 );
	gbSizerClearances->Add( m_clearanceUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_minWidthLabel = new wxStaticText( m_clearancesPanel, wxID_ANY, _("Minimum width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minWidthLabel->Wrap( -1 );
	m_minWidthLabel->SetToolTip( _("Minimum thickness of filled areas.") );

	gbSizerClearances->Add( m_minWidthLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_minWidthCtrl = new wxTextCtrl( m_clearancesPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerClearances->Add( m_minWidthCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 4 );

	m_minWidthUnits = new wxStaticText( m_clearancesPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minWidthUnits->Wrap( -1 );
	gbSizerClearances->Add( m_minWidthUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_connectionLabel = new wxStaticText( m_clearancesPanel, wxID_ANY, _("Pad connections:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_connectionLabel->Wrap( -1 );
	m_connectionLabel->SetToolTip( _("Default pad connection type to zone.\nThis setting can be overridden by local  pad settings") );

	gbSizerClearances->Add( m_connectionLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	wxString m_PadInZoneOptChoices[] = { _("Solid"), _("Thermal reliefs"), _("Reliefs for PTH"), _("None") };
	int m_PadInZoneOptNChoices = sizeof( m_PadInZoneOptChoices ) / sizeof( wxString );
	m_PadInZoneOpt = new wxChoice( m_clearancesPanel, ID_M_PADINZONEOPT, wxDefaultPosition, wxDefaultSize, m_PadInZoneOptNChoices, m_PadInZoneOptChoices, 0 );
	m_PadInZoneOpt->SetSelection( 0 );
	gbSizerClearances->Add( m_PadInZoneOpt, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_antipadLabel = new wxStaticText( m_clearancesPanel, wxID_ANY, _("Thermal relief gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_antipadLabel->Wrap( -1 );
	m_antipadLabel->SetToolTip( _("The distance that will be kept clear between the filled area of the zone and a pad connected by thermal relief spokes.") );

	gbSizerClearances->Add( m_antipadLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 4 );

	m_antipadCtrl = new wxTextCtrl( m_clearancesPanel, wxID_ANTIPAD_SIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_antipadCtrl->SetToolTip( _("Clearance between pads in the same net and filled areas.") );

	gbSizerClearances->Add( m_antipadCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 4 );

	m_antipadUnits = new wxStaticText( m_clearancesPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_antipadUnits->Wrap( -1 );
	gbSizerClearances->Add( m_antipadUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_spokeWidthLabel = new wxStaticText( m_clearancesPanel, wxID_ANY, _("Thermal spoke width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthLabel->Wrap( -1 );
	gbSizerClearances->Add( m_spokeWidthLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 4 );

	m_spokeWidthCtrl = new wxTextCtrl( m_clearancesPanel, wxID_COPPER_BRIDGE_VALUE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthCtrl->SetToolTip( _("Width of copper in thermal reliefs.") );

	gbSizerClearances->Add( m_spokeWidthCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALL, 4 );

	m_spokeWidthUnits = new wxStaticText( m_clearancesPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthUnits->Wrap( -1 );
	gbSizerClearances->Add( m_spokeWidthUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	gbSizerClearances->AddGrowableCol( 2 );

	bClearancesSizer->Add( gbSizerClearances, 0, wxALL, 5 );


	m_clearancesPanel->SetSizer( bClearancesSizer );
	m_clearancesPanel->Layout();
	bClearancesSizer->Fit( m_clearancesPanel );
	m_notebook->AddPage( m_clearancesPanel, _("Clearances && Pad Connections"), false );
	m_displayOverridesPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bDisplayOverridesSizer;
	bDisplayOverridesSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizerDisplayOverrides;
	gbSizerDisplayOverrides = new wxGridBagSizer( 5, 5 );
	gbSizerDisplayOverrides->SetFlexibleDirection( wxBOTH );
	gbSizerDisplayOverrides->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextStyle = new wxStaticText( m_displayOverridesPanel, wxID_ANY, _("Outline display:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyle->Wrap( -1 );
	gbSizerDisplayOverrides->Add( m_staticTextStyle, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	wxString m_OutlineDisplayCtrlChoices[] = { _("Line"), _("Hatched"), _("Fully hatched") };
	int m_OutlineDisplayCtrlNChoices = sizeof( m_OutlineDisplayCtrlChoices ) / sizeof( wxString );
	m_OutlineDisplayCtrl = new wxChoice( m_displayOverridesPanel, ID_M_OUTLINEAPPEARANCECTRL, wxDefaultPosition, wxDefaultSize, m_OutlineDisplayCtrlNChoices, m_OutlineDisplayCtrlChoices, 0 );
	m_OutlineDisplayCtrl->SetSelection( 0 );
	gbSizerDisplayOverrides->Add( m_OutlineDisplayCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_stBorderHatchPitchText = new wxStaticText( m_displayOverridesPanel, wxID_ANY, _("Outline hatch pitch:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stBorderHatchPitchText->Wrap( -1 );
	gbSizerDisplayOverrides->Add( m_stBorderHatchPitchText, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_outlineHatchPitchCtrl = new wxTextCtrl( m_displayOverridesPanel, ID_M_CORNERSMOOTHINGCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerDisplayOverrides->Add( m_outlineHatchPitchCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_outlineHatchUnits = new wxStaticText( m_displayOverridesPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_outlineHatchUnits->Wrap( -1 );
	gbSizerDisplayOverrides->Add( m_outlineHatchUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	gbSizerDisplayOverrides->AddGrowableCol( 2 );

	bDisplayOverridesSizer->Add( gbSizerDisplayOverrides, 0, wxALL, 5 );


	m_displayOverridesPanel->SetSizer( bDisplayOverridesSizer );
	m_displayOverridesPanel->Layout();
	bDisplayOverridesSizer->Fit( m_displayOverridesPanel );
	m_notebook->AddPage( m_displayOverridesPanel, _("Display Overrides"), false );
	m_hatchedFillPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bHatchedFillSizer;
	bHatchedFillSizer = new wxBoxSizer( wxHORIZONTAL );

	wxGridBagSizer* gbSizerHatchedFill;
	gbSizerHatchedFill = new wxGridBagSizer( 0, 0 );
	gbSizerHatchedFill->SetFlexibleDirection( wxBOTH );
	gbSizerHatchedFill->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizerHatchedFill->SetEmptyCellSize( wxSize( -1,10 ) );

	m_cbHatched = new wxCheckBox( m_hatchedFillPanel, wxID_ANY, _("Hatched fill"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbHatched->SetValue(true);
	gbSizerHatchedFill->Add( m_cbHatched, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_staticTextGrindOrient = new wxStaticText( m_hatchedFillPanel, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrindOrient->Wrap( -1 );
	gbSizerHatchedFill->Add( m_staticTextGrindOrient, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );

	m_tcGridStyleOrientation = new wxTextCtrl( m_hatchedFillPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerHatchedFill->Add( m_tcGridStyleOrientation, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticTextRotUnits = new wxStaticText( m_hatchedFillPanel, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotUnits->Wrap( -1 );
	gbSizerHatchedFill->Add( m_staticTextRotUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextStyleThickness = new wxStaticText( m_hatchedFillPanel, wxID_ANY, _("Hatch width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyleThickness->Wrap( -1 );
	gbSizerHatchedFill->Add( m_staticTextStyleThickness, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_tcGridStyleThickness = new wxTextCtrl( m_hatchedFillPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerHatchedFill->Add( m_tcGridStyleThickness, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_GridStyleThicknessUnits = new wxStaticText( m_hatchedFillPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_GridStyleThicknessUnits->Wrap( -1 );
	gbSizerHatchedFill->Add( m_GridStyleThicknessUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextGridGap = new wxStaticText( m_hatchedFillPanel, wxID_ANY, _("Hatch gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridGap->Wrap( -1 );
	gbSizerHatchedFill->Add( m_staticTextGridGap, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_tcGridStyleGap = new wxTextCtrl( m_hatchedFillPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerHatchedFill->Add( m_tcGridStyleGap, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_GridStyleGapUnits = new wxStaticText( m_hatchedFillPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_GridStyleGapUnits->Wrap( -1 );
	gbSizerHatchedFill->Add( m_GridStyleGapUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextGridSmoothingLevel = new wxStaticText( m_hatchedFillPanel, wxID_ANY, _("Smoothing effort:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridSmoothingLevel->Wrap( -1 );
	m_staticTextGridSmoothingLevel->SetToolTip( _("Value of smoothing effort\n0 = no smoothing\n1 = chamfer\n2 = round corners\n3 = round corners (finer shape)") );

	gbSizerHatchedFill->Add( m_staticTextGridSmoothingLevel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_spinCtrlSmoothLevel = new wxSpinCtrl( m_hatchedFillPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 3, 0 );
	gbSizerHatchedFill->Add( m_spinCtrlSmoothLevel, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextGridSmootingVal = new wxStaticText( m_hatchedFillPanel, wxID_ANY, _("Smoothing amount:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridSmootingVal->Wrap( -1 );
	m_staticTextGridSmootingVal->SetToolTip( _("Ratio between smoothed corners size and the gap between lines\n0 = no smoothing\n1.0 = max radius/chamfer size (half gap value)") );

	gbSizerHatchedFill->Add( m_staticTextGridSmootingVal, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_spinCtrlSmoothValue = new wxSpinCtrlDouble( m_hatchedFillPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1, 0.1, 0.1 );
	m_spinCtrlSmoothValue->SetDigits( 2 );
	gbSizerHatchedFill->Add( m_spinCtrlSmoothValue, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	gbSizerHatchedFill->AddGrowableCol( 2 );

	bHatchedFillSizer->Add( gbSizerHatchedFill, 0, wxALL, 5 );

	wxBoxSizer* bSizerOffsetOverrides;
	bSizerOffsetOverrides = new wxBoxSizer( wxVERTICAL );

	m_offsetOverridesLabel = new wxStaticText( m_hatchedFillPanel, wxID_ANY, _("Hatch offset overrides:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetOverridesLabel->Wrap( -1 );
	bSizerOffsetOverrides->Add( m_offsetOverridesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerOffsetOverrides->Add( 0, 2, 0, 0, 5 );

	m_layerSpecificOverrides = new WX_GRID( m_hatchedFillPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_layerSpecificOverrides->CreateGrid( 0, 3 );
	m_layerSpecificOverrides->EnableEditing( true );
	m_layerSpecificOverrides->EnableGridLines( true );
	m_layerSpecificOverrides->EnableDragGridSize( false );
	m_layerSpecificOverrides->SetMargins( 0, 0 );

	// Columns
	m_layerSpecificOverrides->SetColSize( 0, 90 );
	m_layerSpecificOverrides->SetColSize( 1, 90 );
	m_layerSpecificOverrides->SetColSize( 2, 90 );
	m_layerSpecificOverrides->EnableDragColMove( false );
	m_layerSpecificOverrides->EnableDragColSize( true );
	m_layerSpecificOverrides->SetColLabelValue( 0, _("Layer") );
	m_layerSpecificOverrides->SetColLabelValue( 1, _("X Offset") );
	m_layerSpecificOverrides->SetColLabelValue( 2, _("Y Offset") );
	m_layerSpecificOverrides->SetColLabelSize( 22 );
	m_layerSpecificOverrides->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_layerSpecificOverrides->EnableDragRowSize( true );
	m_layerSpecificOverrides->SetRowLabelSize( 0 );
	m_layerSpecificOverrides->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_layerSpecificOverrides->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerOffsetOverrides->Add( m_layerSpecificOverrides, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_bpAddCustomLayer = new STD_BITMAP_BUTTON( m_hatchedFillPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_bpAddCustomLayer, 0, wxLEFT|wxRIGHT, 5 );


	bButtonSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDeleteCustomLayer = new STD_BITMAP_BUTTON( m_hatchedFillPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_bpDeleteCustomLayer, 0, wxRIGHT, 5 );


	bSizerOffsetOverrides->Add( bButtonSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bHatchedFillSizer->Add( bSizerOffsetOverrides, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_hatchedFillPanel->SetSizer( bHatchedFillSizer );
	m_hatchedFillPanel->Layout();
	bHatchedFillSizer->Fit( m_hatchedFillPanel );
	m_notebook->AddPage( m_hatchedFillPanel, _("Hatched Fill"), true );

	bPropertiesSizer->Add( m_notebook, 1, wxEXPAND|wxALL, 5 );

	gbSizerGeneralProps = new wxGridBagSizer( 5, 5 );
	gbSizerGeneralProps->SetFlexibleDirection( wxHORIZONTAL );
	gbSizerGeneralProps->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cornerSmoothingLabel = new wxStaticText( this, wxID_ANY, _("Corner smoothing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerSmoothingLabel->Wrap( -1 );
	gbSizerGeneralProps->Add( m_cornerSmoothingLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_cornerSmoothingChoiceChoices[] = { _("None"), _("Chamfer"), _("Fillet") };
	int m_cornerSmoothingChoiceNChoices = sizeof( m_cornerSmoothingChoiceChoices ) / sizeof( wxString );
	m_cornerSmoothingChoice = new wxChoice( this, ID_CORNER_SMOOTHING, wxDefaultPosition, wxDefaultSize, m_cornerSmoothingChoiceNChoices, m_cornerSmoothingChoiceChoices, 0 );
	m_cornerSmoothingChoice->SetSelection( 1 );
	gbSizerGeneralProps->Add( m_cornerSmoothingChoice, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 2 );

	m_cornerRadiusLabel = new wxStaticText( this, wxID_ANY, _("Chamfer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusLabel->Wrap( -1 );
	gbSizerGeneralProps->Add( m_cornerRadiusLabel, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_cornerRadiusCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerGeneralProps->Add( m_cornerRadiusCtrl, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_cornerRadiusUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusUnits->Wrap( -1 );
	gbSizerGeneralProps->Add( m_cornerRadiusUnits, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_removeIslandsLabel = new wxStaticText( this, wxID_ANY, _("Remove islands:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeIslandsLabel->Wrap( -1 );
	m_removeIslandsLabel->SetToolTip( _("Choose what to do with unconnected copper islands") );

	gbSizerGeneralProps->Add( m_removeIslandsLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_cbRemoveIslandsChoices[] = { _("Always"), _("Never"), _("Below area limit") };
	int m_cbRemoveIslandsNChoices = sizeof( m_cbRemoveIslandsChoices ) / sizeof( wxString );
	m_cbRemoveIslands = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbRemoveIslandsNChoices, m_cbRemoveIslandsChoices, 0 );
	m_cbRemoveIslands->SetSelection( 0 );
	gbSizerGeneralProps->Add( m_cbRemoveIslands, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 2 );

	m_islandThresholdLabel = new wxStaticText( this, wxID_ANY, _("Area limit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_islandThresholdLabel->Wrap( -1 );
	m_islandThresholdLabel->SetToolTip( _("Isolated islands smaller than this will be removed") );

	gbSizerGeneralProps->Add( m_islandThresholdLabel, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 10 );

	m_tcIslandThreshold = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerGeneralProps->Add( m_tcIslandThreshold, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );

	m_islandThresholdUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_islandThresholdUnits->Wrap( -1 );
	gbSizerGeneralProps->Add( m_islandThresholdUnits, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );


	gbSizerGeneralProps->AddGrowableCol( 1 );
	gbSizerGeneralProps->AddGrowableCol( 3 );

	bPropertiesSizer->Add( gbSizerGeneralProps, 0, wxALL|wxEXPAND, 10 );


	bSizer8->Add( bPropertiesSizer, 1, wxEXPAND, 5 );


	bMainSizer->Add( bSizer8, 1, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_tcZoneName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::OnZoneNameChanged ), NULL, this );
	m_cbHatched->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::onHatched ), NULL, this );
	m_bpAddCustomLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::OnAddLayerItem ), NULL, this );
	m_bpDeleteCustomLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::OnDeleteLayerItem ), NULL, this );
	m_cornerSmoothingChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::OnCornerSmoothingSelection ), NULL, this );
	m_cbRemoveIslands->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::OnRemoveIslandsSelection ), NULL, this );
}

PANEL_ZONE_PROPERTIES_BASE::~PANEL_ZONE_PROPERTIES_BASE()
{
	// Disconnect Events
	m_tcZoneName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::OnZoneNameChanged ), NULL, this );
	m_cbHatched->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::onHatched ), NULL, this );
	m_bpAddCustomLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::OnAddLayerItem ), NULL, this );
	m_bpDeleteCustomLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::OnDeleteLayerItem ), NULL, this );
	m_cornerSmoothingChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::OnCornerSmoothingSelection ), NULL, this );
	m_cbRemoveIslands->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ZONE_PROPERTIES_BASE::OnRemoveIslandsSelection ), NULL, this );

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_copper_zones_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_COPPER_ZONE_BASE::DIALOG_COPPER_ZONE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_MainBoxSizer = new wxBoxSizer( wxVERTICAL );

	m_copperZoneInfo = new wxInfoBar( this );
	m_copperZoneInfo->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_copperZoneInfo->SetEffectDuration( 500 );
	m_MainBoxSizer->Add( m_copperZoneInfo, 0, wxBOTTOM|wxEXPAND, 5 );

	wxBoxSizer* m_OptionsBoxSizer;
	m_OptionsBoxSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layer") ), wxVERTICAL );

	sbSizer2->SetMinSize( wxSize( 180,-1 ) );
	m_layers = new wxDataViewListCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER|wxBORDER_SIMPLE );
	m_layers->SetMinSize( wxSize( 120,-1 ) );

	sbSizer2->Add( m_layers, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_OptionsBoxSizer->Add( sbSizer2, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Net") ), wxVERTICAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bFilteringSizer;
	bFilteringSizer = new wxBoxSizer( wxHORIZONTAL );

	m_ShowNetNameFilter = new wxTextCtrl( sbSizer3->GetStaticBox(), ID_TEXTCTRL_NETNAMES_FILTER, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_ShowNetNameFilter->SetToolTip( _("Pattern to filter net names in filtered list.\nOnly net names matching this pattern are displayed.") );

	bFilteringSizer->Add( m_ShowNetNameFilter, 5, wxALIGN_CENTER|wxBOTTOM, 5 );


	bFilteringSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_hideAutoGenNetNamesOpt = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Hide auto-generated net names"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hideAutoGenNetNamesOpt->SetValue(true);
	bFilteringSizer->Add( m_hideAutoGenNetNamesOpt, 0, wxALIGN_CENTER|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bFilteringSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sortByPadsOpt = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Sort nets by pad count"), wxDefaultPosition, wxDefaultSize, 0 );
	bFilteringSizer->Add( m_sortByPadsOpt, 0, wxALIGN_CENTER|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer8->Add( bFilteringSizer, 1, wxEXPAND, 20 );


	sbSizer3->Add( bSizer8, 1, wxEXPAND, 5 );

	m_ListNetNameSelection = new wxListBox( sbSizer3->GetStaticBox(), ID_NETNAME_SELECTION, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	sbSizer3->Add( m_ListNetNameSelection, 8, wxBOTTOM|wxEXPAND, 5 );


	m_OptionsBoxSizer->Add( sbSizer3, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_MainBoxSizer->Add( m_OptionsBoxSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbGeneral;
	sbGeneral = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("General") ), wxHORIZONTAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_zoneNameLabel = new wxStaticText( sbGeneral->GetStaticBox(), wxID_ANY, _("Zone name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_zoneNameLabel->Wrap( -1 );
	m_zoneNameLabel->SetToolTip( _("A unique name for this zone to identify it for DRC") );

	fgSizer1->Add( m_zoneNameLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_tcZoneName = new wxTextCtrl( sbGeneral->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_tcZoneName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_staticTextPriorityLevel = new wxStaticText( sbGeneral->GetStaticBox(), wxID_ANY, _("Zone priority level:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPriorityLevel->Wrap( -1 );
	m_staticTextPriorityLevel->SetToolTip( _("Zones are filled by priority level, level 3 has higher priority than level 2.\nWhen a zone is inside another zone:\n* If its priority is higher, its outlines are removed from the other zone.\n* If its priority is equal, a DRC error is set.") );

	fgSizer1->Add( m_staticTextPriorityLevel, 0, wxBOTTOM, 5 );

	m_PriorityLevelCtrl = new wxSpinCtrl( sbGeneral->GetStaticBox(), ID_M_PRIORITYLEVELCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
	fgSizer1->Add( m_PriorityLevelCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbGeneral->Add( fgSizer1, 1, wxEXPAND, 5 );


	bSizer7->Add( sbGeneral, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxStaticBoxSizer* m_ExportableSetupSizer;
	m_ExportableSetupSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Shape") ), wxHORIZONTAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_constrainOutline = new wxCheckBox( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Constrain outline to H, V and 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_constrainOutline, wxGBPosition( 0, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT, 5 );

	m_cbLocked = new wxCheckBox( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbLocked, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT, 5 );

	m_staticTextStyle = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Outline display:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyle->Wrap( -1 );
	gbSizer1->Add( m_staticTextStyle, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	wxString m_OutlineDisplayCtrlChoices[] = { _("Line"), _("Hatched"), _("Fully hatched") };
	int m_OutlineDisplayCtrlNChoices = sizeof( m_OutlineDisplayCtrlChoices ) / sizeof( wxString );
	m_OutlineDisplayCtrl = new wxChoice( m_ExportableSetupSizer->GetStaticBox(), ID_M_OUTLINEAPPEARANCECTRL, wxDefaultPosition, wxDefaultSize, m_OutlineDisplayCtrlNChoices, m_OutlineDisplayCtrlChoices, 0 );
	m_OutlineDisplayCtrl->SetSelection( 0 );
	gbSizer1->Add( m_OutlineDisplayCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticline1 = new wxStaticLine( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	gbSizer1->Add( m_staticline1, wxGBPosition( 3, 0 ), wxGBSpan( 1, 3 ), wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextSmoothing = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Corner smoothing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSmoothing->Wrap( -1 );
	gbSizer1->Add( m_staticTextSmoothing, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxString m_cornerSmoothingChoiceChoices[] = { _("None"), _("Chamfer"), _("Fillet") };
	int m_cornerSmoothingChoiceNChoices = sizeof( m_cornerSmoothingChoiceChoices ) / sizeof( wxString );
	m_cornerSmoothingChoice = new wxChoice( m_ExportableSetupSizer->GetStaticBox(), ID_CORNER_SMOOTHING, wxDefaultPosition, wxDefaultSize, m_cornerSmoothingChoiceNChoices, m_cornerSmoothingChoiceChoices, 0 );
	m_cornerSmoothingChoice->SetSelection( 0 );
	gbSizer1->Add( m_cornerSmoothingChoice, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALL, 5 );

	m_cornerRadiusLabel = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Chamfer distance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusLabel->Wrap( -1 );
	gbSizer1->Add( m_cornerRadiusLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_cornerRadiusCtrl = new wxTextCtrl( m_ExportableSetupSizer->GetStaticBox(), ID_M_CORNERSMOOTHINGCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cornerRadiusCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cornerRadiusUnits = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusUnits->Wrap( -1 );
	gbSizer1->Add( m_cornerRadiusUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	gbSizer1->AddGrowableCol( 0 );

	m_ExportableSetupSizer->Add( gbSizer1, 1, wxEXPAND, 5 );


	bSizer7->Add( m_ExportableSetupSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	bSizerMiddle->Add( bSizer7, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Electrical Properties") ), wxVERTICAL );

	wxGridBagSizer* gbSizerSettings;
	gbSizerSettings = new wxGridBagSizer( 0, 0 );
	gbSizerSettings->SetFlexibleDirection( wxBOTH );
	gbSizerSettings->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_clearanceLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceLabel->Wrap( -1 );
	m_clearanceLabel->SetToolTip( _("Copper clearance for this zone (set to 0 to use the netclass clearance)") );

	gbSizerSettings->Add( m_clearanceLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_clearanceCtrl = new wxTextCtrl( sbSizer5->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerSettings->Add( m_clearanceCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_clearanceUnits = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceUnits->Wrap( -1 );
	gbSizerSettings->Add( m_clearanceUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_minWidthLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Minimum width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minWidthLabel->Wrap( -1 );
	m_minWidthLabel->SetToolTip( _("Minimum thickness of filled areas.") );

	gbSizerSettings->Add( m_minWidthLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_minWidthCtrl = new wxTextCtrl( sbSizer5->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerSettings->Add( m_minWidthCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_minWidthUnits = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minWidthUnits->Wrap( -1 );
	gbSizerSettings->Add( m_minWidthUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticline2 = new wxStaticLine( sbSizer5->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	gbSizerSettings->Add( m_staticline2, wxGBPosition( 2, 0 ), wxGBSpan( 1, 3 ), wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_connectionLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Pad connections:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_connectionLabel->Wrap( -1 );
	m_connectionLabel->SetToolTip( _("Default pad connection type to zone.\nThis setting can be overridden by local  pad settings") );

	gbSizerSettings->Add( m_connectionLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	wxString m_PadInZoneOptChoices[] = { _("Solid"), _("Thermal reliefs"), _("Reliefs for PTH"), _("None") };
	int m_PadInZoneOptNChoices = sizeof( m_PadInZoneOptChoices ) / sizeof( wxString );
	m_PadInZoneOpt = new wxChoice( sbSizer5->GetStaticBox(), ID_M_PADINZONEOPT, wxDefaultPosition, wxDefaultSize, m_PadInZoneOptNChoices, m_PadInZoneOptChoices, 0 );
	m_PadInZoneOpt->SetSelection( 0 );
	gbSizerSettings->Add( m_PadInZoneOpt, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALL, 5 );

	m_antipadLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Thermal relief gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_antipadLabel->Wrap( -1 );
	m_antipadLabel->SetToolTip( _("The distance that will be kept clear between the filled area of the zone and a pad connected by thermal relief spokes.") );

	gbSizerSettings->Add( m_antipadLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_antipadCtrl = new wxTextCtrl( sbSizer5->GetStaticBox(), wxID_ANTIPAD_SIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_antipadCtrl->SetToolTip( _("Clearance between pads in the same net and filled areas.") );

	gbSizerSettings->Add( m_antipadCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_antipadUnits = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_antipadUnits->Wrap( -1 );
	gbSizerSettings->Add( m_antipadUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_spokeWidthLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Thermal spoke width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthLabel->Wrap( -1 );
	gbSizerSettings->Add( m_spokeWidthLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_spokeWidthCtrl = new wxTextCtrl( sbSizer5->GetStaticBox(), wxID_COPPER_BRIDGE_VALUE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthCtrl->SetToolTip( _("Width of copper in thermal reliefs.") );

	gbSizerSettings->Add( m_spokeWidthCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALL, 5 );

	m_spokeWidthUnits = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthUnits->Wrap( -1 );
	gbSizerSettings->Add( m_spokeWidthUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	gbSizerSettings->AddGrowableCol( 1 );

	sbSizer5->Add( gbSizerSettings, 0, wxEXPAND, 5 );


	bSizerMiddle->Add( sbSizer5, 0, wxEXPAND|wxTOP|wxRIGHT, 10 );

	wxStaticBoxSizer* sbSizerZoneStyle;
	sbSizerZoneStyle = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fill") ), wxVERTICAL );

	wxGridBagSizer* gbSizer3;
	gbSizer3 = new wxGridBagSizer( 0, 0 );
	gbSizer3->SetFlexibleDirection( wxBOTH );
	gbSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer3->SetEmptyCellSize( wxSize( -1,10 ) );

	m_staticTextGridFillType = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Fill type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridFillType->Wrap( -1 );
	gbSizer3->Add( m_staticTextGridFillType, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_GridStyleCtrlChoices[] = { _("Solid fill"), _("Hatch pattern") };
	int m_GridStyleCtrlNChoices = sizeof( m_GridStyleCtrlChoices ) / sizeof( wxString );
	m_GridStyleCtrl = new wxChoice( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_GridStyleCtrlNChoices, m_GridStyleCtrlChoices, 0 );
	m_GridStyleCtrl->SetSelection( 0 );
	gbSizer3->Add( m_GridStyleCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextGrindOrient = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrindOrient->Wrap( -1 );
	gbSizer3->Add( m_staticTextGrindOrient, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_tcGridStyleOrientation = new wxTextCtrl( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer3->Add( m_tcGridStyleOrientation, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticTextRotUnits = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotUnits->Wrap( -1 );
	gbSizer3->Add( m_staticTextRotUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextStyleThickness = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Hatch width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyleThickness->Wrap( -1 );
	gbSizer3->Add( m_staticTextStyleThickness, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_tcGridStyleThickness = new wxTextCtrl( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer3->Add( m_tcGridStyleThickness, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_GridStyleThicknessUnits = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_GridStyleThicknessUnits->Wrap( -1 );
	gbSizer3->Add( m_GridStyleThicknessUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextGridGap = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Hatch gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridGap->Wrap( -1 );
	gbSizer3->Add( m_staticTextGridGap, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_tcGridStyleGap = new wxTextCtrl( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer3->Add( m_tcGridStyleGap, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_GridStyleGapUnits = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_GridStyleGapUnits->Wrap( -1 );
	gbSizer3->Add( m_GridStyleGapUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextGridSmoothingLevel = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Smoothing effort:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridSmoothingLevel->Wrap( -1 );
	m_staticTextGridSmoothingLevel->SetToolTip( _("Value of smoothing effort\n0 = no smoothing\n1 = chamfer\n2 = round corners\n3 = round corners (finer shape)") );

	gbSizer3->Add( m_staticTextGridSmoothingLevel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_spinCtrlSmoothLevel = new wxSpinCtrl( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 3, 0 );
	gbSizer3->Add( m_spinCtrlSmoothLevel, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextGridSmootingVal = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Smoothing amount:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridSmootingVal->Wrap( -1 );
	m_staticTextGridSmootingVal->SetToolTip( _("Ratio between smoothed corners size and the gap between lines\n0 = no smoothing\n1.0 = max radius/chamfer size (half gap value)") );

	gbSizer3->Add( m_staticTextGridSmootingVal, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_spinCtrlSmoothValue = new wxSpinCtrlDouble( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1, 0.100000, 0.1 );
	m_spinCtrlSmoothValue->SetDigits( 0 );
	gbSizer3->Add( m_spinCtrlSmoothValue, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticline5 = new wxStaticLine( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	gbSizer3->Add( m_staticline5, wxGBPosition( 6, 0 ), wxGBSpan( 1, 3 ), wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_staticText40 = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Remove islands:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText40->Wrap( -1 );
	m_staticText40->SetToolTip( _("Choose what to do with unconnected copper islands") );

	gbSizer3->Add( m_staticText40, wxGBPosition( 7, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	wxString m_cbRemoveIslandsChoices[] = { _("Always"), _("Never"), _("Below area limit") };
	int m_cbRemoveIslandsNChoices = sizeof( m_cbRemoveIslandsChoices ) / sizeof( wxString );
	m_cbRemoveIslands = new wxChoice( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbRemoveIslandsNChoices, m_cbRemoveIslandsChoices, 0 );
	m_cbRemoveIslands->SetSelection( 0 );
	gbSizer3->Add( m_cbRemoveIslands, wxGBPosition( 7, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_islandThresholdLabel = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Minimum island size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_islandThresholdLabel->Wrap( -1 );
	m_islandThresholdLabel->Enable( false );
	m_islandThresholdLabel->SetToolTip( _("Isolated islands smaller than this will be removed") );

	gbSizer3->Add( m_islandThresholdLabel, wxGBPosition( 8, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_tcIslandThreshold = new wxTextCtrl( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_tcIslandThreshold->Enable( false );

	gbSizer3->Add( m_tcIslandThreshold, wxGBPosition( 8, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_islandThresholdUnits = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_islandThresholdUnits->Wrap( -1 );
	m_islandThresholdUnits->Enable( false );

	gbSizer3->Add( m_islandThresholdUnits, wxGBPosition( 8, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	gbSizer3->AddGrowableCol( 0 );

	sbSizerZoneStyle->Add( gbSizer3, 1, wxEXPAND|wxBOTTOM, 5 );


	bSizerMiddle->Add( sbSizerZoneStyle, 0, wxEXPAND|wxTOP|wxRIGHT, 10 );


	m_MainBoxSizer->Add( bSizerMiddle, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerbottom;
	bSizerbottom = new wxBoxSizer( wxHORIZONTAL );

	m_ExportSetupButton = new wxButton( this, wxID_BUTTON_EXPORT, _("Export Settings to Other Zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExportSetupButton->SetToolTip( _("Export this zone setup (excluding layer and net selection) to all other copper zones.") );

	bSizerbottom->Add( m_ExportSetupButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerbottom->Add( m_sdbSizer, 1, wxALL|wxEXPAND, 5 );


	m_MainBoxSizer->Add( bSizerbottom, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( m_MainBoxSizer );
	this->Layout();
	m_MainBoxSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_COPPER_ZONE_BASE::OnClose ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_COPPER_ZONE_BASE::OnUpdateUI ) );
	m_layers->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_COPPER_ZONE_BASE::OnLayerSelection ), NULL, this );
	m_ShowNetNameFilter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnShowNetNameFilterChange ), NULL, this );
	m_ShowNetNameFilter->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnShowNetNameFilterChange ), NULL, this );
	m_hideAutoGenNetNamesOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnNetSortingOptionSelected ), NULL, this );
	m_sortByPadsOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnNetSortingOptionSelected ), NULL, this );
	m_ListNetNameSelection->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnNetSelectionUpdated ), NULL, this );
	m_GridStyleCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnStyleSelection ), NULL, this );
	m_cbRemoveIslands->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnRemoveIslandsSelection ), NULL, this );
	m_ExportSetupButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::ExportSetupToOtherCopperZones ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnButtonCancelClick ), NULL, this );
}

DIALOG_COPPER_ZONE_BASE::~DIALOG_COPPER_ZONE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_COPPER_ZONE_BASE::OnClose ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_COPPER_ZONE_BASE::OnUpdateUI ) );
	m_layers->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_COPPER_ZONE_BASE::OnLayerSelection ), NULL, this );
	m_ShowNetNameFilter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnShowNetNameFilterChange ), NULL, this );
	m_ShowNetNameFilter->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnShowNetNameFilterChange ), NULL, this );
	m_hideAutoGenNetNamesOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnNetSortingOptionSelected ), NULL, this );
	m_sortByPadsOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnNetSortingOptionSelected ), NULL, this );
	m_ListNetNameSelection->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnNetSelectionUpdated ), NULL, this );
	m_GridStyleCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnStyleSelection ), NULL, this );
	m_cbRemoveIslands->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnRemoveIslandsSelection ), NULL, this );
	m_ExportSetupButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::ExportSetupToOtherCopperZones ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnButtonCancelClick ), NULL, this );

}

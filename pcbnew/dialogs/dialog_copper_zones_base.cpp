///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
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

	wxBoxSizer* m_OptionsBoxSizer;
	m_OptionsBoxSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layer") ), wxVERTICAL );

	m_layers = new wxDataViewListCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER|wxBORDER_SIMPLE );
	m_layers->SetMinSize( wxSize( 120,-1 ) );

	sbSizer2->Add( m_layers, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_OptionsBoxSizer->Add( sbSizer2, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Net") ), wxHORIZONTAL );

	m_ListNetNameSelection = new wxListBox( sbSizer3->GetStaticBox(), ID_NETNAME_SELECTION, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	sbSizer3->Add( m_ListNetNameSelection, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bFilteringSizer;
	bFilteringSizer = new wxBoxSizer( wxVERTICAL );

	m_staticTextDisplay = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("Hide nets matching:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDisplay->Wrap( -1 );
	bFilteringSizer->Add( m_staticTextDisplay, 0, wxRIGHT|wxLEFT, 5 );

	m_DoNotShowNetNameFilter = new wxTextCtrl( sbSizer3->GetStaticBox(), ID_TEXTCTRL_NETNAMES_FILTER, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_DoNotShowNetNameFilter->SetToolTip( _("Pattern to filter net names in filtered list.\nNet names matching this pattern are not displayed.") );
	m_DoNotShowNetNameFilter->SetMinSize( wxSize( 180,-1 ) );

	bFilteringSizer->Add( m_DoNotShowNetNameFilter, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticTextVFilter = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("Show nets matching:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVFilter->Wrap( -1 );
	bFilteringSizer->Add( m_staticTextVFilter, 0, wxRIGHT|wxLEFT, 5 );

	m_ShowNetNameFilter = new wxTextCtrl( sbSizer3->GetStaticBox(), ID_TEXTCTRL_NETNAMES_FILTER, _("*"), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_ShowNetNameFilter->SetToolTip( _("Pattern to filter net names in filtered list.\nOnly net names matching this pattern are displayed.") );

	bFilteringSizer->Add( m_ShowNetNameFilter, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_buttonRunFilter = new wxButton( sbSizer3->GetStaticBox(), wxID_APPLY_FILTERS, _("Apply Filters"), wxDefaultPosition, wxDefaultSize, 0 );
	bFilteringSizer->Add( m_buttonRunFilter, 0, wxALL|wxEXPAND, 5 );


	bFilteringSizer->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_showAllNetsOpt = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Show all nets"), wxDefaultPosition, wxDefaultSize, 0 );
	bFilteringSizer->Add( m_showAllNetsOpt, 0, wxALL, 5 );


	bFilteringSizer->Add( 0, 0, 0, wxEXPAND|wxTOP, 5 );

	m_sortByPadsOpt = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Sort nets by pad count"), wxDefaultPosition, wxDefaultSize, 0 );
	bFilteringSizer->Add( m_sortByPadsOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_bNoNetWarning = new wxBoxSizer( wxHORIZONTAL );

	m_bitmapNoNetWarning = new wxStaticBitmap( sbSizer3->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bNoNetWarning->Add( m_bitmapNoNetWarning, 0, wxTOP|wxBOTTOM|wxLEFT, 8 );

	m_staticText18 = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("No net will result\nin an unconnected \ncopper island."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	m_bNoNetWarning->Add( m_staticText18, 0, wxALL, 5 );


	bFilteringSizer->Add( m_bNoNetWarning, 1, wxEXPAND|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxTOP, 20 );


	sbSizer3->Add( bFilteringSizer, 0, wxEXPAND, 20 );


	m_OptionsBoxSizer->Add( sbSizer3, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_MainBoxSizer->Add( m_OptionsBoxSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* m_ExportableSetupSizer;
	m_ExportableSetupSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Shape") ), wxHORIZONTAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_constrainOutline = new wxCheckBox( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Constrain outline to H, V and 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_constrainOutline, wxGBPosition( 0, 0 ), wxGBSpan( 1, 3 ), wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextPriorityLevel = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Zone priority level:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPriorityLevel->Wrap( -1 );
	m_staticTextPriorityLevel->SetToolTip( _("Zones are filled by priority level, level 3 has higher priority than level 2.\nWhen a zone is inside another zone:\n* If its priority is higher, its outlines are removed from the other zone.\n* If its priority is equal, a DRC error is set.") );

	gbSizer1->Add( m_staticTextPriorityLevel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_PriorityLevelCtrl = new wxSpinCtrl( m_ExportableSetupSizer->GetStaticBox(), ID_M_PRIORITYLEVELCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
	gbSizer1->Add( m_PriorityLevelCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALL, 5 );

	m_staticTextStyle = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Outline display:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyle->Wrap( -1 );
	gbSizer1->Add( m_staticTextStyle, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxString m_OutlineAppearanceCtrlChoices[] = { _("Line"), _("Hatched"), _("Fully hatched") };
	int m_OutlineAppearanceCtrlNChoices = sizeof( m_OutlineAppearanceCtrlChoices ) / sizeof( wxString );
	m_OutlineAppearanceCtrl = new wxChoice( m_ExportableSetupSizer->GetStaticBox(), ID_M_OUTLINEAPPEARANCECTRL, wxDefaultPosition, wxDefaultSize, m_OutlineAppearanceCtrlNChoices, m_OutlineAppearanceCtrlChoices, 0 );
	m_OutlineAppearanceCtrl->SetSelection( 0 );
	gbSizer1->Add( m_OutlineAppearanceCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

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

	m_cornerRadiusUnits = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusUnits->Wrap( -1 );
	gbSizer1->Add( m_cornerRadiusUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	gbSizer1->AddGrowableCol( 1 );

	m_ExportableSetupSizer->Add( gbSizer1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerMiddle->Add( m_ExportableSetupSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Electrical Properties") ), wxVERTICAL );

	wxGridBagSizer* gbSizerSettings;
	gbSizerSettings = new wxGridBagSizer( 0, 0 );
	gbSizerSettings->SetFlexibleDirection( wxBOTH );
	gbSizerSettings->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_clearanceLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceLabel->Wrap( -1 );
	gbSizerSettings->Add( m_clearanceLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_clearanceCtrl = new wxTextCtrl( sbSizer5->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerSettings->Add( m_clearanceCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_clearanceUnits = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceUnits->Wrap( -1 );
	gbSizerSettings->Add( m_clearanceUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_minWidthLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Minimum width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minWidthLabel->Wrap( -1 );
	m_minWidthLabel->SetToolTip( _("Minimum thickness of filled areas.") );

	gbSizerSettings->Add( m_minWidthLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_minWidthCtrl = new wxTextCtrl( sbSizer5->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerSettings->Add( m_minWidthCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_minWidthUnits = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minWidthUnits->Wrap( -1 );
	gbSizerSettings->Add( m_minWidthUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_connectionLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Pad connections:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_connectionLabel->Wrap( -1 );
	m_connectionLabel->SetToolTip( _("Default pad connection type to zone.\nThis setting can be overridden by local  pad settings") );

	gbSizerSettings->Add( m_connectionLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_PadInZoneOptChoices[] = { _("Solid"), _("Thermal reliefs"), _("Reliefs for PTH"), _("None") };
	int m_PadInZoneOptNChoices = sizeof( m_PadInZoneOptChoices ) / sizeof( wxString );
	m_PadInZoneOpt = new wxChoice( sbSizer5->GetStaticBox(), ID_M_PADINZONEOPT, wxDefaultPosition, wxDefaultSize, m_PadInZoneOptNChoices, m_PadInZoneOptChoices, 0 );
	m_PadInZoneOpt->SetSelection( 0 );
	gbSizerSettings->Add( m_PadInZoneOpt, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALL, 5 );

	m_antipadLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Thermal clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_antipadLabel->Wrap( -1 );
	gbSizerSettings->Add( m_antipadLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_antipadCtrl = new wxTextCtrl( sbSizer5->GetStaticBox(), wxID_ANTIPAD_SIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_antipadCtrl->SetToolTip( _("Clearance between pads in the same net and filled areas.") );

	gbSizerSettings->Add( m_antipadCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_antipadUnits = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_antipadUnits->Wrap( -1 );
	gbSizerSettings->Add( m_antipadUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_spokeWidthLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Thermal spoke width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthLabel->Wrap( -1 );
	gbSizerSettings->Add( m_spokeWidthLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_spokeWidthCtrl = new wxTextCtrl( sbSizer5->GetStaticBox(), wxID_COPPER_BRIDGE_VALUE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthCtrl->SetToolTip( _("Width of copper in thermal reliefs.") );

	gbSizerSettings->Add( m_spokeWidthCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALL, 5 );

	m_spokeWidthUnits = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthUnits->Wrap( -1 );
	gbSizerSettings->Add( m_spokeWidthUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	gbSizerSettings->AddGrowableCol( 1 );

	sbSizer5->Add( gbSizerSettings, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerMiddle->Add( sbSizer5, 1, wxEXPAND|wxTOP|wxRIGHT, 10 );

	wxStaticBoxSizer* sbSizerZoneStyle;
	sbSizerZoneStyle = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fill") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerZoneStyle;
	fgSizerZoneStyle = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerZoneStyle->AddGrowableCol( 1 );
	fgSizerZoneStyle->SetFlexibleDirection( wxBOTH );
	fgSizerZoneStyle->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextGridFillType = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Fill type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridFillType->Wrap( -1 );
	fgSizerZoneStyle->Add( m_staticTextGridFillType, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_GridStyleCtrlChoices[] = { _("Solid shape"), _("Hatch pattern") };
	int m_GridStyleCtrlNChoices = sizeof( m_GridStyleCtrlChoices ) / sizeof( wxString );
	m_GridStyleCtrl = new wxChoice( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_GridStyleCtrlNChoices, m_GridStyleCtrlChoices, 0 );
	m_GridStyleCtrl->SetSelection( 0 );
	fgSizerZoneStyle->Add( m_GridStyleCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	fgSizerZoneStyle->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextGrindOrient = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrindOrient->Wrap( -1 );
	fgSizerZoneStyle->Add( m_staticTextGrindOrient, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_tcGridStyleOrientation = new wxTextCtrl( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerZoneStyle->Add( m_tcGridStyleOrientation, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_staticTextRotUnits = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotUnits->Wrap( -1 );
	fgSizerZoneStyle->Add( m_staticTextRotUnits, 0, wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextStyleThickness = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Hatch width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyleThickness->Wrap( -1 );
	fgSizerZoneStyle->Add( m_staticTextStyleThickness, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_tcGridStyleThickness = new wxTextCtrl( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerZoneStyle->Add( m_tcGridStyleThickness, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_GridStyleThicknessUnits = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_GridStyleThicknessUnits->Wrap( -1 );
	fgSizerZoneStyle->Add( m_GridStyleThicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextGridGap = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Hatch gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridGap->Wrap( -1 );
	fgSizerZoneStyle->Add( m_staticTextGridGap, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_tcGridStyleGap = new wxTextCtrl( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerZoneStyle->Add( m_tcGridStyleGap, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_GridStyleGapUnits = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_GridStyleGapUnits->Wrap( -1 );
	fgSizerZoneStyle->Add( m_GridStyleGapUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextGridSmoothingLevel = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Smoothing effort:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridSmoothingLevel->Wrap( -1 );
	m_staticTextGridSmoothingLevel->SetToolTip( _("Value of smoothing effort\n0 = no smoothing\n1 = chamfer\n2 = round corners\n3 = round corners (finer shape)") );

	fgSizerZoneStyle->Add( m_staticTextGridSmoothingLevel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlSmoothLevel = new wxSpinCtrl( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 3, 0 );
	fgSizerZoneStyle->Add( m_spinCtrlSmoothLevel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	fgSizerZoneStyle->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextGridSmootingVal = new wxStaticText( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, _("Smooth value (0..1):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridSmootingVal->Wrap( -1 );
	m_staticTextGridSmootingVal->SetToolTip( _("Ratio between smoothed corners size and the gap between lines\n0 = no smoothing\n1.0 = max radius/chamfer size (half gap value)") );

	fgSizerZoneStyle->Add( m_staticTextGridSmootingVal, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlSmoothValue = new wxSpinCtrlDouble( sbSizerZoneStyle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1, 0.100000, 0.1 );
	m_spinCtrlSmoothValue->SetDigits( 0 );
	fgSizerZoneStyle->Add( m_spinCtrlSmoothValue, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	fgSizerZoneStyle->Add( 0, 0, 1, wxEXPAND, 5 );


	sbSizerZoneStyle->Add( fgSizerZoneStyle, 1, wxEXPAND, 5 );


	bSizerMiddle->Add( sbSizerZoneStyle, 1, wxEXPAND|wxTOP|wxRIGHT, 10 );


	m_MainBoxSizer->Add( bSizerMiddle, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerbottom;
	bSizerbottom = new wxBoxSizer( wxHORIZONTAL );

	m_ExportSetupButton = new wxButton( this, wxID_BUTTON_EXPORT, _("Export Settings to Other Zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExportSetupButton->SetToolTip( _("Export this zone setup (excluding layer and net selection) to all other copper zones.") );

	bSizerbottom->Add( m_ExportSetupButton, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 10 );

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
	m_DoNotShowNetNameFilter->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnRunFiltersButtonClick ), NULL, this );
	m_ShowNetNameFilter->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnRunFiltersButtonClick ), NULL, this );
	m_buttonRunFilter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnRunFiltersButtonClick ), NULL, this );
	m_showAllNetsOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnNetSortingOptionSelected ), NULL, this );
	m_sortByPadsOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnNetSortingOptionSelected ), NULL, this );
	m_GridStyleCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnStyleSelection ), NULL, this );
	m_ExportSetupButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::ExportSetupToOtherCopperZones ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnButtonCancelClick ), NULL, this );
}

DIALOG_COPPER_ZONE_BASE::~DIALOG_COPPER_ZONE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_COPPER_ZONE_BASE::OnClose ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_COPPER_ZONE_BASE::OnUpdateUI ) );
	m_layers->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_COPPER_ZONE_BASE::OnLayerSelection ), NULL, this );
	m_DoNotShowNetNameFilter->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnRunFiltersButtonClick ), NULL, this );
	m_ShowNetNameFilter->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnRunFiltersButtonClick ), NULL, this );
	m_buttonRunFilter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnRunFiltersButtonClick ), NULL, this );
	m_showAllNetsOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnNetSortingOptionSelected ), NULL, this );
	m_sortByPadsOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnNetSortingOptionSelected ), NULL, this );
	m_GridStyleCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnStyleSelection ), NULL, this );
	m_ExportSetupButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::ExportSetupToOtherCopperZones ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnButtonCancelClick ), NULL, this );

}

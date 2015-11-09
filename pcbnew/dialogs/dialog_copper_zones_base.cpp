///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_copper_zones_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_COPPER_ZONE_BASE, DIALOG_SHIM )
	EVT_CLOSE( DIALOG_COPPER_ZONE_BASE::_wxFB_OnClose )
	EVT_CHOICE( ID_M_NETDISPLAYOPTION, DIALOG_COPPER_ZONE_BASE::_wxFB_OnNetSortingOptionSelected )
	EVT_TEXT_ENTER( ID_TEXTCTRL_NETNAMES_FILTER, DIALOG_COPPER_ZONE_BASE::_wxFB_OnRunFiltersButtonClick )
	EVT_TEXT_ENTER( ID_TEXTCTRL_NETNAMES_FILTER, DIALOG_COPPER_ZONE_BASE::_wxFB_OnRunFiltersButtonClick )
	EVT_BUTTON( wxID_APPLY_FILTERS, DIALOG_COPPER_ZONE_BASE::_wxFB_OnRunFiltersButtonClick )
	EVT_CHOICE( ID_CORNER_SMOOTHING, DIALOG_COPPER_ZONE_BASE::_wxFB_OnCornerSmoothingModeChoice )
	EVT_CHOICE( ID_M_PADINZONEOPT, DIALOG_COPPER_ZONE_BASE::_wxFB_OnPadsInZoneClick )
	EVT_BUTTON( wxID_BUTTON_EXPORT, DIALOG_COPPER_ZONE_BASE::_wxFB_ExportSetupToOtherCopperZones )
	EVT_BUTTON( wxID_CANCEL, DIALOG_COPPER_ZONE_BASE::_wxFB_OnButtonCancelClick )
	EVT_BUTTON( wxID_OK, DIALOG_COPPER_ZONE_BASE::_wxFB_OnButtonOkClick )
END_EVENT_TABLE()

DIALOG_COPPER_ZONE_BASE::DIALOG_COPPER_ZONE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_MainBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_OptionsBoxSizer;
	m_OptionsBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* m_layerSizer;
	m_layerSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText17 = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	m_layerSizer->Add( m_staticText17, 0, wxTOP|wxRIGHT, 5 );
	
	m_LayerSelectionCtrl = new wxListView( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_ALIGN_LEFT|wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL );
	m_layerSizer->Add( m_LayerSelectionCtrl, 1, wxRIGHT, 5 );
	
	
	m_OptionsBoxSizer->Add( m_layerSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerNets;
	bSizerNets = new wxBoxSizer( wxVERTICAL );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Net:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizerNets->Add( m_staticText2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ListNetNameSelection = new wxListBox( this, ID_NETNAME_SELECTION, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizerNets->Add( m_ListNetNameSelection, 1, wxEXPAND, 5 );
	
	
	m_OptionsBoxSizer->Add( bSizerNets, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_NetSortOptSizer;
	m_NetSortOptSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Net Filtering") ), wxVERTICAL );
	
	m_staticText16 = new wxStaticText( m_NetSortOptSizer->GetStaticBox(), wxID_ANY, _("Display:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	m_NetSortOptSizer->Add( m_staticText16, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_NetDisplayOptionChoices[] = { _("Show all (alphabetical)"), _("Show all (pad count)"), _("Filtered (alphabetical)"), _("Filtered (pad count)") };
	int m_NetDisplayOptionNChoices = sizeof( m_NetDisplayOptionChoices ) / sizeof( wxString );
	m_NetDisplayOption = new wxChoice( m_NetSortOptSizer->GetStaticBox(), ID_M_NETDISPLAYOPTION, wxDefaultPosition, wxDefaultSize, m_NetDisplayOptionNChoices, m_NetDisplayOptionChoices, 0 );
	m_NetDisplayOption->SetSelection( 0 );
	m_NetSortOptSizer->Add( m_NetDisplayOption, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_staticText5 = new wxStaticText( m_NetSortOptSizer->GetStaticBox(), wxID_ANY, _("Hidden net filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	m_NetSortOptSizer->Add( m_staticText5, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DoNotShowNetNameFilter = new wxTextCtrl( m_NetSortOptSizer->GetStaticBox(), ID_TEXTCTRL_NETNAMES_FILTER, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_DoNotShowNetNameFilter->SetMaxLength( 0 ); 
	m_DoNotShowNetNameFilter->SetToolTip( _("Pattern to filter net names in filtered list.\nNet names matching this pattern are not displayed.") );
	
	m_NetSortOptSizer->Add( m_DoNotShowNetNameFilter, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText51 = new wxStaticText( m_NetSortOptSizer->GetStaticBox(), wxID_ANY, _("Visible net filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText51->Wrap( -1 );
	m_NetSortOptSizer->Add( m_staticText51, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ShowNetNameFilter = new wxTextCtrl( m_NetSortOptSizer->GetStaticBox(), ID_TEXTCTRL_NETNAMES_FILTER, _("*"), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_ShowNetNameFilter->SetMaxLength( 0 ); 
	m_ShowNetNameFilter->SetToolTip( _("Pattern to filter net names in filtered list.\nOnly net names matching this pattern are displayed.") );
	
	m_NetSortOptSizer->Add( m_ShowNetNameFilter, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_buttonRunFilter = new wxButton( m_NetSortOptSizer->GetStaticBox(), wxID_APPLY_FILTERS, _("Apply Filters"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetSortOptSizer->Add( m_buttonRunFilter, 0, wxALL|wxEXPAND, 5 );
	
	
	m_OptionsBoxSizer->Add( m_NetSortOptSizer, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_MainBoxSizer->Add( m_OptionsBoxSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* m_ExportableSetupSizer;
	m_ExportableSetupSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Settings") ), wxHORIZONTAL );
	
	wxBoxSizer* bSizerSettings;
	bSizerSettings = new wxBoxSizer( wxVERTICAL );
	
	m_ClearanceValueTitle = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ClearanceValueTitle->Wrap( -1 );
	bSizerSettings->Add( m_ClearanceValueTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ZoneClearanceCtrl = new wxTextCtrl( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ZoneClearanceCtrl->SetMaxLength( 0 ); 
	bSizerSettings->Add( m_ZoneClearanceCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_MinThicknessValueTitle = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Minimum width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MinThicknessValueTitle->Wrap( -1 );
	m_MinThicknessValueTitle->SetToolTip( _("Minimun thickness of filled areas.") );
	
	bSizerSettings->Add( m_MinThicknessValueTitle, 0, wxRIGHT|wxLEFT, 5 );
	
	m_ZoneMinThicknessCtrl = new wxTextCtrl( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ZoneMinThicknessCtrl->SetMaxLength( 0 ); 
	bSizerSettings->Add( m_ZoneMinThicknessCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText151 = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Corner smoothing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText151->Wrap( -1 );
	bSizerSettings->Add( m_staticText151, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_cornerSmoothingChoiceChoices[] = { _("None"), _("Chamfer"), _("Fillet") };
	int m_cornerSmoothingChoiceNChoices = sizeof( m_cornerSmoothingChoiceChoices ) / sizeof( wxString );
	m_cornerSmoothingChoice = new wxChoice( m_ExportableSetupSizer->GetStaticBox(), ID_CORNER_SMOOTHING, wxDefaultPosition, wxDefaultSize, m_cornerSmoothingChoiceNChoices, m_cornerSmoothingChoiceChoices, 0 );
	m_cornerSmoothingChoice->SetSelection( 0 );
	bSizerSettings->Add( m_cornerSmoothingChoice, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_cornerSmoothingTitle = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Chamfer distance (mm):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerSmoothingTitle->Wrap( -1 );
	bSizerSettings->Add( m_cornerSmoothingTitle, 0, wxRIGHT|wxLEFT, 5 );
	
	m_cornerSmoothingCtrl = new wxTextCtrl( m_ExportableSetupSizer->GetStaticBox(), ID_M_CORNERSMOOTHINGCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerSmoothingCtrl->SetMaxLength( 0 ); 
	bSizerSettings->Add( m_cornerSmoothingCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	m_ExportableSetupSizer->Add( bSizerSettings, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerPadsConnection;
	bSizerPadsConnection = new wxBoxSizer( wxVERTICAL );
	
	m_staticText13 = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Pad connection:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	bSizerPadsConnection->Add( m_staticText13, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_PadInZoneOptChoices[] = { _("Solid"), _("Thermal relief"), _("THT thermal"), _("None") };
	int m_PadInZoneOptNChoices = sizeof( m_PadInZoneOptChoices ) / sizeof( wxString );
	m_PadInZoneOpt = new wxChoice( m_ExportableSetupSizer->GetStaticBox(), ID_M_PADINZONEOPT, wxDefaultPosition, wxDefaultSize, m_PadInZoneOptNChoices, m_PadInZoneOptChoices, 0 );
	m_PadInZoneOpt->SetSelection( 0 );
	bSizerPadsConnection->Add( m_PadInZoneOpt, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	wxStaticBoxSizer* m_ThermalShapesParamsSizer;
	m_ThermalShapesParamsSizer = new wxStaticBoxSizer( new wxStaticBox( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Thermal Reliefs") ), wxVERTICAL );
	
	m_AntipadSizeText = new wxStaticText( m_ThermalShapesParamsSizer->GetStaticBox(), wxID_ANY, _("Antipad clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AntipadSizeText->Wrap( -1 );
	m_ThermalShapesParamsSizer->Add( m_AntipadSizeText, 0, wxTOP|wxRIGHT, 5 );
	
	m_AntipadSizeValue = new wxTextCtrl( m_ThermalShapesParamsSizer->GetStaticBox(), wxID_ANTIPAD_SIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_AntipadSizeValue->SetMaxLength( 0 ); 
	m_AntipadSizeValue->SetToolTip( _("Clearance between pads in the same net and filled areas.") );
	
	m_ThermalShapesParamsSizer->Add( m_AntipadSizeValue, 0, wxEXPAND|wxBOTTOM, 5 );
	
	m_CopperBridgeWidthText = new wxStaticText( m_ThermalShapesParamsSizer->GetStaticBox(), wxID_ANY, _("Spoke width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CopperBridgeWidthText->Wrap( -1 );
	m_ThermalShapesParamsSizer->Add( m_CopperBridgeWidthText, 0, wxTOP|wxRIGHT, 5 );
	
	m_CopperWidthValue = new wxTextCtrl( m_ThermalShapesParamsSizer->GetStaticBox(), wxID_COPPER_BRIDGE_VALUE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CopperWidthValue->SetMaxLength( 0 ); 
	m_CopperWidthValue->SetToolTip( _("Width of copper in thermal reliefs.") );
	
	m_ThermalShapesParamsSizer->Add( m_CopperWidthValue, 0, wxEXPAND|wxBOTTOM, 5 );
	
	
	bSizerPadsConnection->Add( m_ThermalShapesParamsSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	m_ExportableSetupSizer->Add( bSizerPadsConnection, 0, wxEXPAND, 5 );
	
	wxBoxSizer* m_MiddleBox;
	m_MiddleBox = new wxBoxSizer( wxVERTICAL );
	
	m_staticText171 = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Priority level:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText171->Wrap( -1 );
	m_staticText171->SetToolTip( _("On each copper layer, zones are filled by priority order.\nSo when a zone is inside an other zone:\n* If its priority is highter: its outlines are removed from the other layer.\n* If its priority is equal: a DRC error is set.") );
	
	m_MiddleBox->Add( m_staticText171, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PriorityLevelCtrl = new wxSpinCtrl( m_ExportableSetupSizer->GetStaticBox(), ID_M_PRIORITYLEVELCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
	m_MiddleBox->Add( m_PriorityLevelCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText11 = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Fill mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	m_MiddleBox->Add( m_staticText11, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_FillModeCtrlChoices[] = { _("Polygon"), _("Segment") };
	int m_FillModeCtrlNChoices = sizeof( m_FillModeCtrlChoices ) / sizeof( wxString );
	m_FillModeCtrl = new wxChoice( m_ExportableSetupSizer->GetStaticBox(), ID_M_FILLMODECTRL, wxDefaultPosition, wxDefaultSize, m_FillModeCtrlNChoices, m_FillModeCtrlChoices, 0 );
	m_FillModeCtrl->SetSelection( 0 );
	m_MiddleBox->Add( m_FillModeCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_staticText12 = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Segments / 360 deg:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	m_MiddleBox->Add( m_staticText12, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_ArcApproximationOptChoices[] = { _("16"), _("32") };
	int m_ArcApproximationOptNChoices = sizeof( m_ArcApproximationOptChoices ) / sizeof( wxString );
	m_ArcApproximationOpt = new wxChoice( m_ExportableSetupSizer->GetStaticBox(), ID_M_ARCAPPROXIMATIONOPT, wxDefaultPosition, wxDefaultSize, m_ArcApproximationOptNChoices, m_ArcApproximationOptChoices, 0 );
	m_ArcApproximationOpt->SetSelection( 0 );
	m_MiddleBox->Add( m_ArcApproximationOpt, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	m_ExportableSetupSizer->Add( m_MiddleBox, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerLowerRight;
	bSizerLowerRight = new wxBoxSizer( wxVERTICAL );
	
	m_staticText14 = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Outline slope:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	bSizerLowerRight->Add( m_staticText14, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_OrientEdgesOptChoices[] = { _("Arbitrary"), _("H, V, and 45 deg only") };
	int m_OrientEdgesOptNChoices = sizeof( m_OrientEdgesOptChoices ) / sizeof( wxString );
	m_OrientEdgesOpt = new wxChoice( m_ExportableSetupSizer->GetStaticBox(), ID_M_ORIENTEDGESOPT, wxDefaultPosition, wxDefaultSize, m_OrientEdgesOptNChoices, m_OrientEdgesOptChoices, 0 );
	m_OrientEdgesOpt->SetSelection( 0 );
	bSizerLowerRight->Add( m_OrientEdgesOpt, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_staticText15 = new wxStaticText( m_ExportableSetupSizer->GetStaticBox(), wxID_ANY, _("Outline style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	bSizerLowerRight->Add( m_staticText15, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_OutlineAppearanceCtrlChoices[] = { _("Line"), _("Hatched"), _("Fully hatched") };
	int m_OutlineAppearanceCtrlNChoices = sizeof( m_OutlineAppearanceCtrlChoices ) / sizeof( wxString );
	m_OutlineAppearanceCtrl = new wxChoice( m_ExportableSetupSizer->GetStaticBox(), ID_M_OUTLINEAPPEARANCECTRL, wxDefaultPosition, wxDefaultSize, m_OutlineAppearanceCtrlNChoices, m_OutlineAppearanceCtrlChoices, 0 );
	m_OutlineAppearanceCtrl->SetSelection( 0 );
	bSizerLowerRight->Add( m_OutlineAppearanceCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	m_ExportableSetupSizer->Add( bSizerLowerRight, 0, wxEXPAND, 5 );
	
	
	m_MainBoxSizer->Add( m_ExportableSetupSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerbottom;
	bSizerbottom = new wxBoxSizer( wxHORIZONTAL );
	
	m_ExportSetupButton = new wxButton( this, wxID_BUTTON_EXPORT, _("Export Settings to Other Zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExportSetupButton->SetToolTip( _("Export this zone setup (excluding layer and net selection) to all other copper zones.") );
	
	bSizerbottom->Add( m_ExportSetupButton, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerbottom->Add( m_sdbSizer, 1, wxEXPAND, 5 );
	
	
	m_MainBoxSizer->Add( bSizerbottom, 0, wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( m_MainBoxSizer );
	this->Layout();
}

DIALOG_COPPER_ZONE_BASE::~DIALOG_COPPER_ZONE_BASE()
{
}

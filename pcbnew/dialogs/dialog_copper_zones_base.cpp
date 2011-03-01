///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 17 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_copper_zones_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( dialog_copper_zone_base, wxDialog )
	EVT_CLOSE( dialog_copper_zone_base::_wxFB_OnClose )
	EVT_SIZE( dialog_copper_zone_base::_wxFB_OnSize )
	EVT_CHOICE( ID_M_NETDISPLAYOPTION, dialog_copper_zone_base::_wxFB_OnNetSortingOptionSelected )
	EVT_TEXT_ENTER( ID_TEXTCTRL_NETNAMES_FILTER, dialog_copper_zone_base::_wxFB_OnRunFiltersButtonClick )
	EVT_TEXT_ENTER( ID_TEXTCTRL_NETNAMES_FILTER, dialog_copper_zone_base::_wxFB_OnRunFiltersButtonClick )
	EVT_BUTTON( wxID_APPLY_FILTERS, dialog_copper_zone_base::_wxFB_OnRunFiltersButtonClick )
	EVT_CHOICE( ID_CORNER_SMOOTHING, dialog_copper_zone_base::_wxFB_OnCornerSmoothingModeChoice )
	EVT_CHOICE( ID_M_PADINZONEOPT, dialog_copper_zone_base::_wxFB_OnPadsInZoneClick )
	EVT_BUTTON( wxID_BUTTON_EXPORT, dialog_copper_zone_base::_wxFB_ExportSetupToOtherCopperZones )
	EVT_BUTTON( wxID_OK, dialog_copper_zone_base::_wxFB_OnButtonOkClick )
	EVT_BUTTON( wxID_CANCEL, dialog_copper_zone_base::_wxFB_OnButtonCancelClick )
END_EVENT_TABLE()

dialog_copper_zone_base::dialog_copper_zone_base( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_MainBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_OptionsBoxSizer;
	m_OptionsBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_layerSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText17 = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	m_layerSizer->Add( m_staticText17, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_OptionsBoxSizer->Add( m_layerSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Net:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizer7->Add( m_staticText2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ListNetNameSelection = new wxListBox( this, ID_NETNAME_SELECTION, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizer7->Add( m_ListNetNameSelection, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_OptionsBoxSizer->Add( bSizer7, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_NetSortOptSizer;
	m_NetSortOptSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Net Filtering") ), wxVERTICAL );
	
	m_staticText16 = new wxStaticText( this, wxID_ANY, _("Display:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	m_NetSortOptSizer->Add( m_staticText16, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_NetDisplayOptionChoices[] = { _("Show all (alphabetical)"), _("Show all (advanced)"), _("Filtered (alphabetical)"), _("Filtered (advanced)") };
	int m_NetDisplayOptionNChoices = sizeof( m_NetDisplayOptionChoices ) / sizeof( wxString );
	m_NetDisplayOption = new wxChoice( this, ID_M_NETDISPLAYOPTION, wxDefaultPosition, wxDefaultSize, m_NetDisplayOptionNChoices, m_NetDisplayOptionChoices, 0 );
	m_NetDisplayOption->SetSelection( 0 );
	m_NetSortOptSizer->Add( m_NetDisplayOption, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Hidden net filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	m_NetSortOptSizer->Add( m_staticText5, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DoNotShowNetNameFilter = new wxTextCtrl( this, ID_TEXTCTRL_NETNAMES_FILTER, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_DoNotShowNetNameFilter->SetToolTip( _("Pattern to filter net names in filtered list.\nNet names matching this pattern are not displayed.") );
	
	m_NetSortOptSizer->Add( m_DoNotShowNetNameFilter, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText51 = new wxStaticText( this, wxID_ANY, _("Visible net filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText51->Wrap( -1 );
	m_NetSortOptSizer->Add( m_staticText51, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ShowNetNameFilter = new wxTextCtrl( this, ID_TEXTCTRL_NETNAMES_FILTER, _("*"), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_ShowNetNameFilter->SetToolTip( _("Pattern to filter net names in filtered list.\nOnly net names matching this pattern are displayed.") );
	
	m_NetSortOptSizer->Add( m_ShowNetNameFilter, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_buttonRunFilter = new wxButton( this, wxID_APPLY_FILTERS, _("Apply Filters"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetSortOptSizer->Add( m_buttonRunFilter, 0, wxALL|wxEXPAND, 5 );
	
	m_OptionsBoxSizer->Add( m_NetSortOptSizer, 0, wxALL, 5 );
	
	m_MainBoxSizer->Add( m_OptionsBoxSizer, 1, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_ExportableSetupSizer;
	m_ExportableSetupSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Settings") ), wxHORIZONTAL );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	m_ClearanceValueTitle = new wxStaticText( this, wxID_ANY, _("Clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ClearanceValueTitle->Wrap( -1 );
	bSizer9->Add( m_ClearanceValueTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ZoneClearanceCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_ZoneClearanceCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_MinThicknessValueTitle = new wxStaticText( this, wxID_ANY, _("Minimum width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MinThicknessValueTitle->Wrap( -1 );
	bSizer9->Add( m_MinThicknessValueTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ZoneMinThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ZoneMinThicknessCtrl->SetToolTip( _("Minimun thickness of filled areas.") );
	
	bSizer9->Add( m_ZoneMinThicknessCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText151 = new wxStaticText( this, wxID_ANY, _("Corner smoothing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText151->Wrap( -1 );
	bSizer9->Add( m_staticText151, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_cornerSmoothingChoiceChoices[] = { _("None"), _("Chamfer"), _("Fillet") };
	int m_cornerSmoothingChoiceNChoices = sizeof( m_cornerSmoothingChoiceChoices ) / sizeof( wxString );
	m_cornerSmoothingChoice = new wxChoice( this, ID_CORNER_SMOOTHING, wxDefaultPosition, wxDefaultSize, m_cornerSmoothingChoiceNChoices, m_cornerSmoothingChoiceChoices, 0 );
	m_cornerSmoothingChoice->SetSelection( 0 );
	bSizer9->Add( m_cornerSmoothingChoice, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_cornerSmoothingTitle = new wxStaticText( this, wxID_ANY, _("Chamfer distance (mm):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerSmoothingTitle->Wrap( -1 );
	bSizer9->Add( m_cornerSmoothingTitle, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_cornerSmoothingCtrl = new wxTextCtrl( this, ID_M_CORNERSMOOTHINGCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_cornerSmoothingCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_ExportableSetupSizer->Add( bSizer9, 0, wxEXPAND, 5 );
	
	wxBoxSizer* m_LeftBox;
	m_LeftBox = new wxBoxSizer( wxVERTICAL );
	
	m_staticText13 = new wxStaticText( this, wxID_ANY, _("Pad connection:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	m_LeftBox->Add( m_staticText13, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_PadInZoneOptChoices[] = { _("Solid"), _("Thermal relief"), _("None") };
	int m_PadInZoneOptNChoices = sizeof( m_PadInZoneOptChoices ) / sizeof( wxString );
	m_PadInZoneOpt = new wxChoice( this, ID_M_PADINZONEOPT, wxDefaultPosition, wxDefaultSize, m_PadInZoneOptNChoices, m_PadInZoneOptChoices, 0 );
	m_PadInZoneOpt->SetSelection( 0 );
	m_LeftBox->Add( m_PadInZoneOpt, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	wxStaticBoxSizer* m_ThermalShapesParamsSizer;
	m_ThermalShapesParamsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Thermal Reliefs") ), wxVERTICAL );
	
	m_AntipadSizeText = new wxStaticText( this, wxID_ANY, _("Antipad clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AntipadSizeText->Wrap( -1 );
	m_ThermalShapesParamsSizer->Add( m_AntipadSizeText, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_AntipadSizeValue = new wxTextCtrl( this, wxID_ANTIPAD_SIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_AntipadSizeValue->SetToolTip( _("Clearance between pads in the same net and filled areas.") );
	
	m_ThermalShapesParamsSizer->Add( m_AntipadSizeValue, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_CopperBridgeWidthText = new wxStaticText( this, wxID_ANY, _("Spoke width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CopperBridgeWidthText->Wrap( -1 );
	m_ThermalShapesParamsSizer->Add( m_CopperBridgeWidthText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_CopperWidthValue = new wxTextCtrl( this, wxID_COPPER_BRIDGE_VALUE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CopperWidthValue->SetToolTip( _("Width of copper in thermal reliefs.") );
	
	m_ThermalShapesParamsSizer->Add( m_CopperWidthValue, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_LeftBox->Add( m_ThermalShapesParamsSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_ExportableSetupSizer->Add( m_LeftBox, 0, wxEXPAND, 5 );
	
	wxBoxSizer* m_MiddleBox;
	m_MiddleBox = new wxBoxSizer( wxVERTICAL );
	
	m_staticText11 = new wxStaticText( this, wxID_ANY, _("Fill mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	m_MiddleBox->Add( m_staticText11, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_FillModeCtrlChoices[] = { _("Polygon"), _("Segment") };
	int m_FillModeCtrlNChoices = sizeof( m_FillModeCtrlChoices ) / sizeof( wxString );
	m_FillModeCtrl = new wxChoice( this, ID_M_FILLMODECTRL, wxDefaultPosition, wxDefaultSize, m_FillModeCtrlNChoices, m_FillModeCtrlChoices, 0 );
	m_FillModeCtrl->SetSelection( 0 );
	m_MiddleBox->Add( m_FillModeCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_staticText12 = new wxStaticText( this, wxID_ANY, _("Segments / 360 deg:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	m_MiddleBox->Add( m_staticText12, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_ArcApproximationOptChoices[] = { _("16"), _("32") };
	int m_ArcApproximationOptNChoices = sizeof( m_ArcApproximationOptChoices ) / sizeof( wxString );
	m_ArcApproximationOpt = new wxChoice( this, ID_M_ARCAPPROXIMATIONOPT, wxDefaultPosition, wxDefaultSize, m_ArcApproximationOptNChoices, m_ArcApproximationOptChoices, 0 );
	m_ArcApproximationOpt->SetSelection( 0 );
	m_MiddleBox->Add( m_ArcApproximationOpt, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_ExportableSetupSizer->Add( m_MiddleBox, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer81;
	bSizer81 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText14 = new wxStaticText( this, wxID_ANY, _("Outline slope:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	bSizer81->Add( m_staticText14, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_OrientEdgesOptChoices[] = { _("Arbitrary"), _("H, V, and 45 deg only") };
	int m_OrientEdgesOptNChoices = sizeof( m_OrientEdgesOptChoices ) / sizeof( wxString );
	m_OrientEdgesOpt = new wxChoice( this, ID_M_ORIENTEDGESOPT, wxDefaultPosition, wxDefaultSize, m_OrientEdgesOptNChoices, m_OrientEdgesOptChoices, 0 );
	m_OrientEdgesOpt->SetSelection( 0 );
	bSizer81->Add( m_OrientEdgesOpt, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_staticText15 = new wxStaticText( this, wxID_ANY, _("Outline style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	bSizer81->Add( m_staticText15, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_OutlineAppearanceCtrlChoices[] = { _("Line"), _("Hatched"), _("Fully hatched") };
	int m_OutlineAppearanceCtrlNChoices = sizeof( m_OutlineAppearanceCtrlChoices ) / sizeof( wxString );
	m_OutlineAppearanceCtrl = new wxChoice( this, ID_M_OUTLINEAPPEARANCECTRL, wxDefaultPosition, wxDefaultSize, m_OutlineAppearanceCtrlNChoices, m_OutlineAppearanceCtrlChoices, 0 );
	m_OutlineAppearanceCtrl->SetSelection( 0 );
	bSizer81->Add( m_OutlineAppearanceCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_ExportableSetupSizer->Add( bSizer81, 0, wxEXPAND, 5 );
	
	m_MainBoxSizer->Add( m_ExportableSetupSizer, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );
	
	m_ExportSetupButton = new wxButton( this, wxID_BUTTON_EXPORT, _("Export Settings to Other Zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExportSetupButton->SetToolTip( _("Export this zone setup (excluding layer and net selection) to all other copper zones.") );
	
	bSizer10->Add( m_ExportSetupButton, 0, wxALL|wxEXPAND, 5 );
	
	m_OkButton = new wxButton( this, wxID_OK, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OkButton->SetDefault(); 
	bSizer10->Add( m_OkButton, 0, wxALL|wxEXPAND, 5 );
	
	m_ButtonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer10->Add( m_ButtonCancel, 0, wxALL|wxEXPAND, 5 );
	
	m_MainBoxSizer->Add( bSizer10, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	this->SetSizer( m_MainBoxSizer );
	this->Layout();
}

dialog_copper_zone_base::~dialog_copper_zone_base()
{
}

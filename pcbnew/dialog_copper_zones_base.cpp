///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_copper_zones_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( dialog_copper_zone_base, wxDialog )
	EVT_INIT_DIALOG( dialog_copper_zone_base::_wxFB_OnInitDialog )
	EVT_RADIOBOX( wxID_PADS_IN_ZONE_OPTIONS, dialog_copper_zone_base::_wxFB_OnPadsInZoneClick )
	EVT_BUTTON( wxID_BUTTON_EXPORT, dialog_copper_zone_base::_wxFB_ExportSetupToOtherCopperZones )
	EVT_BUTTON( wxID_OK, dialog_copper_zone_base::_wxFB_OnButtonOkClick )
	EVT_BUTTON( wxID_CANCEL, dialog_copper_zone_base::_wxFB_OnButtonCancelClick )
	EVT_RADIOBOX( ID_NET_SORTING_OPTION, dialog_copper_zone_base::_wxFB_OnNetSortingOptionSelected )
END_EVENT_TABLE()

dialog_copper_zone_base::dialog_copper_zone_base( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* m_MainBoxSize;
	m_MainBoxSize = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_OptionsBoxSizer;
	m_OptionsBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* m_ExportableSetupSizer;
	m_ExportableSetupSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Zone Setup:") ), wxHORIZONTAL );
	
	wxBoxSizer* m_LeftBoxSizer;
	m_LeftBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* m_FillOptionsBox;
	m_FillOptionsBox = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Zone Fill Options:") ), wxVERTICAL );
	
	wxString m_GridCtrlChoices[] = { _("0.00000"), _("0.00000"), _("0.00000"), _("0.00000"), _("No grid (For tests only!)") };
	int m_GridCtrlNChoices = sizeof( m_GridCtrlChoices ) / sizeof( wxString );
	m_GridCtrl = new wxRadioBox( this,  ID_RADIOBOX_GRID_SELECTION, _("Grid Size for Filling:"), wxDefaultPosition, wxDefaultSize, m_GridCtrlNChoices, m_GridCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_GridCtrl->SetSelection( 4 );
	m_FillOptionsBox->Add( m_GridCtrl, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_PadInZoneOptChoices[] = { _("Include pads"), _("Thermal relief"), _("Exclude pads") };
	int m_PadInZoneOptNChoices = sizeof( m_PadInZoneOptChoices ) / sizeof( wxString );
	m_PadInZoneOpt = new wxRadioBox( this, wxID_PADS_IN_ZONE_OPTIONS, _("Pad in Zone:"), wxDefaultPosition, wxDefaultSize, m_PadInZoneOptNChoices, m_PadInZoneOptChoices, 1, wxRA_SPECIFY_COLS );
	m_PadInZoneOpt->SetSelection( 1 );
	m_FillOptionsBox->Add( m_PadInZoneOpt, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_ThermalShapesParamsSizer;
	m_ThermalShapesParamsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Thermal Reliefs Parameters") ), wxVERTICAL );
	
	m_AntipadSizeText = new wxStaticText( this, wxID_ANY, _("Antipad Size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AntipadSizeText->Wrap( -1 );
	m_ThermalShapesParamsSizer->Add( m_AntipadSizeText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_AntipadSizeValue = new wxTextCtrl( this, wxID_ANTIPAD_SIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_AntipadSizeValue->SetToolTip( _("Define the gap around the pad") );
	
	m_ThermalShapesParamsSizer->Add( m_AntipadSizeValue, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_CopperBridgeWidthText = new wxStaticText( this, wxID_ANY, _("Copper Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CopperBridgeWidthText->Wrap( -1 );
	m_ThermalShapesParamsSizer->Add( m_CopperBridgeWidthText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_CopperWidthValue = new wxTextCtrl( this, wxID_COPPER_BRIDGE_VALUE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CopperWidthValue->SetToolTip( _("Define the tickness of copper in thermal reliefs") );
	
	m_ThermalShapesParamsSizer->Add( m_CopperWidthValue, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_FillOptionsBox->Add( m_ThermalShapesParamsSizer, 0, wxEXPAND, 5 );
	
	m_LeftBoxSizer->Add( m_FillOptionsBox, 1, wxEXPAND, 5 );
	
	m_ExportableSetupSizer->Add( m_LeftBoxSizer, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_ExportableSetupSizer->Add( 5, 5, 0, wxEXPAND, 5 );
	
	wxBoxSizer* m_MiddleBox;
	m_MiddleBox = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_MiddleBoxSizer;
	m_MiddleBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* m_OutilinesBoxOpt;
	m_OutilinesBoxOpt = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Outlines Options:") ), wxVERTICAL );
	
	wxString m_OrientEdgesOptChoices[] = { _("Any"), _("H , V and 45 deg") };
	int m_OrientEdgesOptNChoices = sizeof( m_OrientEdgesOptChoices ) / sizeof( wxString );
	m_OrientEdgesOpt = new wxRadioBox( this, wxID_ANY, _("Zone edges orient:"), wxDefaultPosition, wxDefaultSize, m_OrientEdgesOptNChoices, m_OrientEdgesOptChoices, 1, wxRA_SPECIFY_COLS );
	m_OrientEdgesOpt->SetSelection( 0 );
	m_OutilinesBoxOpt->Add( m_OrientEdgesOpt, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OutlineAppearanceCtrlChoices[] = { _("Line"), _("Hatched outline"), _("Full hatched") };
	int m_OutlineAppearanceCtrlNChoices = sizeof( m_OutlineAppearanceCtrlChoices ) / sizeof( wxString );
	m_OutlineAppearanceCtrl = new wxRadioBox( this, ID_RADIOBOX_OUTLINES_OPTION, _("Outlines Appearance"), wxDefaultPosition, wxDefaultSize, m_OutlineAppearanceCtrlNChoices, m_OutlineAppearanceCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_OutlineAppearanceCtrl->SetSelection( 1 );
	m_OutlineAppearanceCtrl->SetToolTip( _("Choose how a zone outline is displayed\n- Single line\n- Short hatching\n- Full zone area hatched") );
	
	m_OutilinesBoxOpt->Add( m_OutlineAppearanceCtrl, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_ArcApproximationOptChoices[] = { _("16 segments / 360 deg"), _("32 segments / 360 deg") };
	int m_ArcApproximationOptNChoices = sizeof( m_ArcApproximationOptChoices ) / sizeof( wxString );
	m_ArcApproximationOpt = new wxRadioBox( this, wxID_ARC_APPROX, _("Arcs Approximation:"), wxDefaultPosition, wxDefaultSize, m_ArcApproximationOptNChoices, m_ArcApproximationOptChoices, 1, wxRA_SPECIFY_COLS );
	m_ArcApproximationOpt->SetSelection( 1 );
	m_ArcApproximationOpt->SetToolTip( _("Number of segments to approximate a circle in filling calculations.\n16 segment is faster to calculate and when redraw screen.\n32 segment give a better quality") );
	
	m_OutilinesBoxOpt->Add( m_ArcApproximationOpt, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_OthersOptionsSizer;
	m_OthersOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Others Options:") ), wxVERTICAL );
	
	m_ShowFilledAreasInSketchOpt = new wxCheckBox( this, wxID_ANY, _("Show filled areas in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_ShowFilledAreasInSketchOpt->SetToolTip( _("If enabled, filled areas in is this zone will be displayed as non filled polygons.\nIf disabled, filled areas in is this zone will be displayed as \"solid\" areas (normal mode).") );
	
	m_OthersOptionsSizer->Add( m_ShowFilledAreasInSketchOpt, 0, wxALL, 5 );
	
	m_ClearanceValueTitle = new wxStaticText( this, wxID_ANY, _("Zone clearance value (mm):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ClearanceValueTitle->Wrap( -1 );
	m_OthersOptionsSizer->Add( m_ClearanceValueTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ZoneClearanceCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OthersOptionsSizer->Add( m_ZoneClearanceCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_OutilinesBoxOpt->Add( m_OthersOptionsSizer, 1, wxEXPAND, 5 );
	
	m_MiddleBoxSizer->Add( m_OutilinesBoxOpt, 1, wxEXPAND, 5 );
	
	m_ExportSetupButton = new wxButton( this, wxID_BUTTON_EXPORT, _("Export to others zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExportSetupButton->SetToolTip( _("Export this zone setup to all others copper zones") );
	
	m_MiddleBoxSizer->Add( m_ExportSetupButton, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_MiddleBox->Add( m_MiddleBoxSizer, 0, 0, 5 );
	
	m_ExportableSetupSizer->Add( m_MiddleBox, 1, wxEXPAND, 5 );
	
	m_OptionsBoxSizer->Add( m_ExportableSetupSizer, 1, wxEXPAND, 5 );
	
	
	m_OptionsBoxSizer->Add( 0, 0, 0, wxEXPAND, 5 );
	
	wxBoxSizer* m_RightBoxSizer;
	m_RightBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_OkButton = new wxButton( this, wxID_OK, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OkButton->SetDefault(); 
	m_RightBoxSizer->Add( m_OkButton, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_ButtonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RightBoxSizer->Add( m_ButtonCancel, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_RightBoxSizer->Add( 5, 20, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_NetSortOptSizer;
	m_NetSortOptSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Nets Display Options:") ), wxVERTICAL );
	
	wxString m_NetSortingOptionChoices[] = { _("Alphabetic"), _("Advanced") };
	int m_NetSortingOptionNChoices = sizeof( m_NetSortingOptionChoices ) / sizeof( wxString );
	m_NetSortingOption = new wxRadioBox( this, ID_NET_SORTING_OPTION, _("Net sorting:"), wxDefaultPosition, wxDefaultSize, m_NetSortingOptionNChoices, m_NetSortingOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_NetSortingOption->SetSelection( 1 );
	m_NetSortingOption->SetToolTip( _("Nets can be sorted:\nBy alphabetic order\nBy number of pads in the net (advanced)") );
	
	m_NetSortOptSizer->Add( m_NetSortingOption, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Filter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	m_NetSortOptSizer->Add( m_staticText5, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_NetNameFilter = new wxTextCtrl( this, ID_TEXTCTRL_NETNAMES_FILTER, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_NetSortOptSizer->Add( m_NetNameFilter, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_RightBoxSizer->Add( m_NetSortOptSizer, 1, wxEXPAND, 5 );
	
	m_OptionsBoxSizer->Add( m_RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_MainBoxSize->Add( m_OptionsBoxSizer, 0, 0, 5 );
	
	wxBoxSizer* m_NetAndLayersLiastBoxSizer;
	m_NetAndLayersLiastBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Net:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_NetAndLayersLiastBoxSizer->Add( m_staticText2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ListNetNameSelection = new wxListBox( this, ID_NETNAME_SELECTION, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_NetAndLayersLiastBoxSizer->Add( m_ListNetNameSelection, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	m_NetAndLayersLiastBoxSizer->Add( m_staticText3, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_LayerSelectionCtrl = new wxListBox( this, ID_LAYER_CHOICE, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_NetAndLayersLiastBoxSizer->Add( m_LayerSelectionCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_MainBoxSize->Add( m_NetAndLayersLiastBoxSizer, 0, wxEXPAND, 5 );
	
	this->SetSizer( m_MainBoxSize );
	this->Layout();
}

dialog_copper_zone_base::~dialog_copper_zone_base()
{
}

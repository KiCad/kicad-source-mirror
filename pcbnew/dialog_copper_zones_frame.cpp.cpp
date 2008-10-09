///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_copper_zones_frame.cpp.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( dialog_copper_zone_frame, wxDialog )
	EVT_INIT_DIALOG( dialog_copper_zone_frame::_wxFB_OnInitDialog )
	EVT_BUTTON( wxID_OK, dialog_copper_zone_frame::_wxFB_OnOkButtonClick )
	EVT_BUTTON( wxID_CANCEL, dialog_copper_zone_frame::_wxFB_OnButtonCancleClick )
	EVT_RADIOBOX( ID_NET_SORTING_OPTION, dialog_copper_zone_frame::_wxFB_OnNetSortingOptionSelected )
END_EVENT_TABLE()

dialog_copper_zone_frame::dialog_copper_zone_frame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* m_MainBoxSize;
	m_MainBoxSize = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_OptionsBoxSizer;
	m_OptionsBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* m_LeftBoxSizer;
	m_LeftBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_GridCtrlChoices[] = { _("0.00000"), _("0.00000"), _("0.00000"), _("0.00000"), _("|No Grid (For tests only!)") };
	int m_GridCtrlNChoices = sizeof( m_GridCtrlChoices ) / sizeof( wxString );
	m_GridCtrl = new wxRadioBox( this,  ID_RADIOBOX_GRID_SELECTION, _("Grid Size for Filling:"), wxDefaultPosition, wxDefaultSize, m_GridCtrlNChoices, m_GridCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_GridCtrl->SetSelection( 4 );
	m_LeftBoxSizer->Add( m_GridCtrl, 0, wxALL, 5 );
	
	m_ClearanceValueTitle = new wxStaticText( this, wxID_ANY, _("Zone clearance value (mm):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ClearanceValueTitle->Wrap( -1 );
	m_LeftBoxSizer->Add( m_ClearanceValueTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ZoneClearanceCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LeftBoxSizer->Add( m_ZoneClearanceCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxString m_OutlineAppearanceCtrlChoices[] = { _("Line"), _("Hatched Outline"), _("Full Hatched") };
	int m_OutlineAppearanceCtrlNChoices = sizeof( m_OutlineAppearanceCtrlChoices ) / sizeof( wxString );
	m_OutlineAppearanceCtrl = new wxRadioBox( this, ID_RADIOBOX_OUTLINES_OPTION, _("Outlines Appearance"), wxDefaultPosition, wxDefaultSize, m_OutlineAppearanceCtrlNChoices, m_OutlineAppearanceCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_OutlineAppearanceCtrl->SetSelection( 0 );
	m_LeftBoxSizer->Add( m_OutlineAppearanceCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_OptionsBoxSizer->Add( m_LeftBoxSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* m_MiddleBoxSizer;
	m_MiddleBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_FillOptChoices[] = { _("Include Pads"), _("Thermal"), _("Exclude Pads") };
	int m_FillOptNChoices = sizeof( m_FillOptChoices ) / sizeof( wxString );
	m_FillOpt = new wxRadioBox( this, wxID_ANY, _("Pad options:"), wxDefaultPosition, wxDefaultSize, m_FillOptNChoices, m_FillOptChoices, 1, wxRA_SPECIFY_COLS );
	m_FillOpt->SetSelection( 1 );
	m_MiddleBoxSizer->Add( m_FillOpt, 0, wxALL|wxEXPAND, 5 );
	
	
	m_MiddleBoxSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxString m_OrientEdgesOptChoices[] = { _("Any"), _("H , V and 45 deg") };
	int m_OrientEdgesOptNChoices = sizeof( m_OrientEdgesOptChoices ) / sizeof( wxString );
	m_OrientEdgesOpt = new wxRadioBox( this, wxID_ANY, _("Zone edges orient:"), wxDefaultPosition, wxDefaultSize, m_OrientEdgesOptNChoices, m_OrientEdgesOptChoices, 1, wxRA_SPECIFY_COLS );
	m_OrientEdgesOpt->SetSelection( 0 );
	m_MiddleBoxSizer->Add( m_OrientEdgesOpt, 0, wxALL|wxEXPAND, 5 );
	
	m_OptionsBoxSizer->Add( m_MiddleBoxSizer, 1, wxEXPAND, 5 );
	
	
	m_OptionsBoxSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* m_RightBoxSizer;
	m_RightBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_OkButton = new wxButton( this, wxID_OK, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RightBoxSizer->Add( m_OkButton, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_ButtonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RightBoxSizer->Add( m_ButtonCancel, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_RightBoxSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxString m_NetSortingOptionChoices[] = { _("Alphabetic"), _("Advanced") };
	int m_NetSortingOptionNChoices = sizeof( m_NetSortingOptionChoices ) / sizeof( wxString );
	m_NetSortingOption = new wxRadioBox( this, ID_NET_SORTING_OPTION, _("Net sorting:"), wxDefaultPosition, wxDefaultSize, m_NetSortingOptionNChoices, m_NetSortingOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_NetSortingOption->SetSelection( 0 );
	m_RightBoxSizer->Add( m_NetSortingOption, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Filter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	m_RightBoxSizer->Add( m_staticText5, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_NetNameFilter = new wxTextCtrl( this, ID_TEXTCTRL_NETNAMES_FILTER, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RightBoxSizer->Add( m_NetNameFilter, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_OptionsBoxSizer->Add( m_RightBoxSizer, 1, wxEXPAND, 5 );
	
	m_MainBoxSize->Add( m_OptionsBoxSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* m_NetAndLayersLiastBoxSizer;
	m_NetAndLayersLiastBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Net:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_NetAndLayersLiastBoxSizer->Add( m_staticText2, 0, wxALL, 5 );
	
	m_ListNetNameSelection = new wxListBox( this, ID_NETNAME_SELECTION, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_NetAndLayersLiastBoxSizer->Add( m_ListNetNameSelection, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	m_NetAndLayersLiastBoxSizer->Add( m_staticText3, 0, wxALL, 5 );
	
	m_LayerSelectionCtrl = new wxListBox( this, ID_LAYER_CHOICE, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_NetAndLayersLiastBoxSizer->Add( m_LayerSelectionCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_MainBoxSize->Add( m_NetAndLayersLiastBoxSizer, 1, wxEXPAND, 5 );
	
	this->SetSizer( m_MainBoxSize );
	this->Layout();
}

dialog_copper_zone_frame::~dialog_copper_zone_frame()
{
}

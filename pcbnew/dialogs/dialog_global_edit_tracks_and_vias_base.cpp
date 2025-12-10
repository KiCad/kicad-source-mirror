///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_global_edit_tracks_and_vias_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerTop;
	bSizerTop = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbScope;
	sbScope = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scope") ), wxVERTICAL );

	m_tracks = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tracks->SetValue(true);
	sbScope->Add( m_tracks, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbScope->Add( 0, 10, 0, wxEXPAND, 5 );

	m_vias = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE );
	m_vias->SetValue(true);
	sbScope->Add( m_vias, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_throughVias = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Through vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_throughVias->SetValue(true);
	sbScope->Add( m_throughVias, 0, wxLEFT, 20 );


	sbScope->Add( 0, 3, 0, wxEXPAND, 5 );

	m_microVias = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Microvias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_microVias->SetValue(true);
	sbScope->Add( m_microVias, 0, wxLEFT, 20 );


	sbScope->Add( 0, 3, 0, wxEXPAND, 5 );

	m_blindVias = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Blind vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_blindVias->SetValue(true);
	sbScope->Add( m_blindVias, 0, wxLEFT, 20 );


	sbScope->Add( 0, 3, 0, wxLEFT, 20 );

	m_buriedVias = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Buried vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buriedVias->SetValue(true);
	sbScope->Add( m_buriedVias, 0, wxLEFT, 20 );


	bSizerTop->Add( sbScope, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxStaticBoxSizer* sbFilters;
	sbFilters = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Filter Items") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 3, 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->AddGrowableRow( 0 );
	fgSizer3->AddGrowableRow( 1 );
	fgSizer3->AddGrowableRow( 2 );
	fgSizer3->AddGrowableRow( 3 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_netFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by net:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_netFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_netFilter = new NET_SELECTOR( sbFilters->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_netFilter, 1, wxEXPAND|wxRIGHT, 5 );

	m_netclassFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by net class:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_netclassFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_netclassFilterChoices;
	m_netclassFilter = new wxChoice( sbFilters->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_netclassFilterChoices, 0 );
	m_netclassFilter->SetSelection( 0 );
	fgSizer3->Add( m_netclassFilter, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	fgSizer3->Add( 0, 7, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_layerFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_layerFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_layerFilter = new PCB_LAYER_BOX_SELECTOR( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizer3->Add( m_layerFilter, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer3->Add( 0, 7, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_filterByTrackWidth = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter tracks by width:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_filterByTrackWidth, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_trackWidthFilterCtrl = new wxTextCtrl( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_trackWidthFilterCtrl, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_trackWidthFilterUnits = new wxStaticText( sbFilters->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackWidthFilterUnits->Wrap( -1 );
	bSizer3->Add( m_trackWidthFilterUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer3->Add( bSizer3, 1, wxEXPAND, 5 );

	m_filterByViaSize = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter vias by diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_filterByViaSize, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxHORIZONTAL );

	m_viaSizeFilterCtrl = new wxTextCtrl( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer31->Add( m_viaSizeFilterCtrl, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_viaSizeFilterUnits = new wxStaticText( sbFilters->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaSizeFilterUnits->Wrap( -1 );
	bSizer31->Add( m_viaSizeFilterUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer3->Add( bSizer31, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	sbFilters->Add( fgSizer3, 1, wxEXPAND|wxBOTTOM, 5 );


	sbFilters->Add( 0, 5, 0, 0, 5 );

	m_selectedItemsFilter = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Selected items only"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFilters->Add( m_selectedItemsFilter, 0, wxALL, 5 );


	bSizerTop->Add( sbFilters, 2, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	bMainSizer->Add( bSizerTop, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbAction;
	sbAction = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Action") ), wxVERTICAL );

	m_setToSpecifiedValues = new wxRadioButton( sbAction->GetStaticBox(), ID_SPECIFIED_NET_TO_SPECIFIED_VALUES, _("Set to specified values:"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_setToSpecifiedValues->SetValue( true );
	sbAction->Add( m_setToSpecifiedValues, 0, 0, 5 );

	wxFlexGridSizer* fgSizerTrackViaPopups;
	fgSizerTrackViaPopups = new wxFlexGridSizer( 5, 2, 0, 10 );
	fgSizerTrackViaPopups->AddGrowableCol( 1 );
	fgSizerTrackViaPopups->SetFlexibleDirection( wxBOTH );
	fgSizerTrackViaPopups->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

	m_layerLabel = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_layerLabel->Wrap( -1 );
	fgSizerTrackViaPopups->Add( m_layerLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_layerCtrl = new PCB_LAYER_BOX_SELECTOR( sbAction->GetStaticBox(), wxID_ANY, _("-- leave unchanged --  "), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizerTrackViaPopups->Add( m_layerCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_trackWidthLabel = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("Track width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackWidthLabel->Wrap( -1 );
	fgSizerTrackViaPopups->Add( m_trackWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	wxArrayString m_trackWidthCtrlChoices;
	m_trackWidthCtrl = new wxChoice( sbAction->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_trackWidthCtrlChoices, 0 );
	m_trackWidthCtrl->SetSelection( 0 );
	fgSizerTrackViaPopups->Add( m_trackWidthCtrl, 4, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_viaSizeLabel = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("Via size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaSizeLabel->Wrap( -1 );
	fgSizerTrackViaPopups->Add( m_viaSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	wxArrayString m_viaSizesCtrlChoices;
	m_viaSizesCtrl = new wxChoice( sbAction->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_viaSizesCtrlChoices, 0 );
	m_viaSizesCtrl->SetSelection( 0 );
	fgSizerTrackViaPopups->Add( m_viaSizesCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_annularRingsLabel = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("Via annular rings:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_annularRingsLabel->Wrap( -1 );
	fgSizerTrackViaPopups->Add( m_annularRingsLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 15 );

	wxString m_annularRingsCtrlChoices[] = { _("All copper layers"), _("Start, end, and connected layers"), _("Connected layers only"), _("Start and end layers only") };
	int m_annularRingsCtrlNChoices = sizeof( m_annularRingsCtrlChoices ) / sizeof( wxString );
	m_annularRingsCtrl = new wxChoice( sbAction->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_annularRingsCtrlNChoices, m_annularRingsCtrlChoices, 0 );
	m_annularRingsCtrl->SetSelection( 1 );
	fgSizerTrackViaPopups->Add( m_annularRingsCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_protectionFeaturesLabel = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("Via protection features:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_protectionFeaturesLabel->Wrap( -1 );
	fgSizerTrackViaPopups->Add( m_protectionFeaturesLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	wxArrayString m_protectionFeaturesChoices;
	m_protectionFeatures = new wxChoice( sbAction->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_protectionFeaturesChoices, 0 );
	m_protectionFeatures->SetSelection( 0 );
	m_protectionFeatures->SetToolTip( _("Select which protection feature according to IPC-4761 the via should have.") );

	fgSizerTrackViaPopups->Add( m_protectionFeatures, 0, wxALL|wxEXPAND, 5 );


	sbAction->Add( fgSizerTrackViaPopups, 0, wxBOTTOM|wxEXPAND|wxLEFT, 25 );

	m_setToDesignRuleValues = new wxRadioButton( sbAction->GetStaticBox(), ID_SPECIFIED_NET_TO_NETCLASS_VALUES, _("Set to net class / custom rule values"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAction->Add( m_setToDesignRuleValues, 0, wxBOTTOM, 5 );


	bMainSizer->Add( sbAction, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_vias->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onVias ), NULL, this );
	m_throughVias->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onViaType ), NULL, this );
	m_microVias->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onViaType ), NULL, this );
	m_blindVias->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onViaType ), NULL, this );
	m_buriedVias->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onViaType ), NULL, this );
	m_netclassFilter->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnNetclassFilterSelect ), NULL, this );
	m_layerFilter->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnLayerFilterSelect ), NULL, this );
	m_trackWidthFilterCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnTrackWidthText ), NULL, this );
	m_viaSizeFilterCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnViaSizeText ), NULL, this );
	m_setToSpecifiedValues->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onActionButtonChange ), NULL, this );
	m_setToDesignRuleValues->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onActionButtonChange ), NULL, this );
}

DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::~DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE()
{
	// Disconnect Events
	m_vias->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onVias ), NULL, this );
	m_throughVias->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onViaType ), NULL, this );
	m_microVias->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onViaType ), NULL, this );
	m_blindVias->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onViaType ), NULL, this );
	m_buriedVias->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onViaType ), NULL, this );
	m_netclassFilter->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnNetclassFilterSelect ), NULL, this );
	m_layerFilter->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnLayerFilterSelect ), NULL, this );
	m_trackWidthFilterCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnTrackWidthText ), NULL, this );
	m_viaSizeFilterCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnViaSizeText ), NULL, this );
	m_setToSpecifiedValues->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onActionButtonChange ), NULL, this );
	m_setToDesignRuleValues->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::onActionButtonChange ), NULL, this );

}

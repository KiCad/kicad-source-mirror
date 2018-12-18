///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 23 2018)
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

	wxStaticBoxSizer* sbScope;
	sbScope = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scope") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_tracks = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tracks->SetValue(true);
	fgSizer1->Add( m_tracks, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_vias = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vias->SetValue(true);
	fgSizer1->Add( m_vias, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbScope->Add( fgSizer1, 0, wxEXPAND, 5 );


	bMainSizer->Add( sbScope, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbFilters;
	sbFilters = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Filters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->AddGrowableCol( 2 );
	fgSizer3->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_netFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by net:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_netFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_netFilter = new NET_SELECTOR( sbFilters->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_netFilter, 1, wxEXPAND|wxRIGHT, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 100 );

	m_netclassFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by net class:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_netclassFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_netclassFilterChoices;
	m_netclassFilter = new wxChoice( sbFilters->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_netclassFilterChoices, 0 );
	m_netclassFilter->SetSelection( 0 );
	fgSizer3->Add( m_netclassFilter, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_layerFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_layerFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_layerFilter = new PCB_LAYER_BOX_SELECTOR( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizer3->Add( m_layerFilter, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	sbFilters->Add( fgSizer3, 1, wxEXPAND, 5 );


	bMainSizer->Add( sbFilters, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbAction;
	sbAction = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Action") ), wxVERTICAL );

	m_setToSpecifiedValues = new wxRadioButton( sbAction->GetStaticBox(), ID_SPECIFIED_NET_TO_SPECIFIED_VALUES, _("Set to specified values:"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_setToSpecifiedValues->SetValue( true );
	sbAction->Add( m_setToSpecifiedValues, 0, wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bSizerTrackViaPopups;
	bSizerTrackViaPopups = new wxBoxSizer( wxHORIZONTAL );

	m_trackWidthSelectBox = new wxComboBox( sbAction->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	bSizerTrackViaPopups->Add( m_trackWidthSelectBox, 4, wxEXPAND|wxRIGHT, 5 );

	m_viaSizesSelectBox = new wxComboBox( sbAction->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	bSizerTrackViaPopups->Add( m_viaSizesSelectBox, 5, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_layerBox = new PCB_LAYER_BOX_SELECTOR( sbAction->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bSizerTrackViaPopups->Add( m_layerBox, 0, wxRIGHT|wxLEFT, 3 );


	sbAction->Add( bSizerTrackViaPopups, 0, wxEXPAND|wxBOTTOM|wxLEFT, 15 );

	m_setToNetclassValues = new wxRadioButton( sbAction->GetStaticBox(), ID_SPECIFIED_NET_TO_NETCLASS_VALUES, _("Set to net class values:"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAction->Add( m_setToNetclassValues, 0, wxTOP|wxBOTTOM, 5 );

	m_netclassGrid = new wxGrid( sbAction->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_DEFAULT|wxVSCROLL );

	// Grid
	m_netclassGrid->CreateGrid( 1, 6 );
	m_netclassGrid->EnableEditing( false );
	m_netclassGrid->EnableGridLines( true );
	m_netclassGrid->EnableDragGridSize( false );
	m_netclassGrid->SetMargins( 0, 0 );

	// Columns
	m_netclassGrid->SetColSize( 0, 95 );
	m_netclassGrid->SetColSize( 1, 95 );
	m_netclassGrid->SetColSize( 2, 95 );
	m_netclassGrid->SetColSize( 3, 95 );
	m_netclassGrid->SetColSize( 4, 95 );
	m_netclassGrid->SetColSize( 5, 95 );
	m_netclassGrid->EnableDragColMove( false );
	m_netclassGrid->EnableDragColSize( false );
	m_netclassGrid->SetColLabelSize( 0 );
	m_netclassGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_netclassGrid->EnableDragRowSize( false );
	m_netclassGrid->SetRowLabelSize( 0 );
	m_netclassGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_netclassGrid->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	m_netclassGrid->SetDefaultCellFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_netclassGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	sbAction->Add( m_netclassGrid, 0, wxEXPAND|wxLEFT, 15 );


	sbAction->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	bMainSizer->Add( sbAction, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnUpdateUI ) );
	m_netclassFilter->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnNetclassFilterSelect ), NULL, this );
	m_layerFilter->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnLayerFilterSelect ), NULL, this );
	m_netclassGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnSizeNetclassGrid ), NULL, this );
}

DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::~DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnUpdateUI ) );
	m_netclassFilter->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnNetclassFilterSelect ), NULL, this );
	m_layerFilter->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnLayerFilterSelect ), NULL, this );
	m_netclassGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnSizeNetclassGrid ), NULL, this );

}

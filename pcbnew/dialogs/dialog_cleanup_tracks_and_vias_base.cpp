///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_cleanup_tracks_and_vias_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerTop;
	bSizerTop = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Actions") ), wxVERTICAL );

	m_cbRefillZones = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Refill zones before and after cleanup"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_cbRefillZones, 0, wxRIGHT|wxLEFT, 5 );


	sbSizer3->Add( 0, 12, 1, wxEXPAND, 5 );

	m_cleanShortCircuitOpt = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Delete &tracks connecting different nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cleanShortCircuitOpt->SetToolTip( _("remove track segments connecting nodes belonging to different nets (short circuit)") );

	sbSizer3->Add( m_cleanShortCircuitOpt, 0, wxRIGHT|wxLEFT, 5 );


	sbSizer3->Add( 0, 10, 1, wxEXPAND, 5 );

	m_cleanViasOpt = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("&Delete redundant vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cleanViasOpt->SetToolTip( _("remove vias on through hole pads and superimposed vias") );

	sbSizer3->Add( m_cleanViasOpt, 0, wxRIGHT|wxLEFT, 5 );

	m_deleteDanglingViasOpt = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Delete vias connected on only one layer"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_deleteDanglingViasOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	sbSizer3->Add( 0, 10, 1, wxEXPAND, 5 );

	m_mergeSegmOpt = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("&Merge co-linear tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mergeSegmOpt->SetToolTip( _("merge aligned track segments, and remove null segments") );

	sbSizer3->Add( m_mergeSegmOpt, 0, wxRIGHT|wxLEFT, 5 );

	m_deleteUnconnectedOpt = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Delete tracks unconnected at one end"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deleteUnconnectedOpt->SetToolTip( _("delete tracks having at least one dangling end") );

	sbSizer3->Add( m_deleteUnconnectedOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_deleteTracksInPadsOpt = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Delete tracks fully inside pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deleteTracksInPadsOpt->SetToolTip( _("Delete tracks that have both start and end positions inside of a pad") );

	sbSizer3->Add( m_deleteTracksInPadsOpt, 0, wxALL, 5 );


	bSizerTop->Add( sbSizer3, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

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
	m_netclassFilter->SetMinSize( wxSize( 140,-1 ) );

	fgSizer3->Add( m_netclassFilter, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	fgSizer3->Add( 0, 7, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_layerFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_layerFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_layerFilter = new PCB_LAYER_BOX_SELECTOR( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizer3->Add( m_layerFilter, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer3->Add( 0, 7, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	sbFilters->Add( fgSizer3, 0, wxEXPAND|wxBOTTOM, 5 );


	sbFilters->Add( 0, 5, 0, 0, 5 );

	m_selectedItemsFilter = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Selected items only"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFilters->Add( m_selectedItemsFilter, 0, wxALL, 5 );


	sbFilters->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizerTop->Add( sbFilters, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerTop, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	bLowerSizer->SetMinSize( wxSize( 660,250 ) );
	m_outputBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_changesPanel = new wxPanel( m_outputBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* changesSizer;
	changesSizer = new wxBoxSizer( wxVERTICAL );

	staticChangesLabel = new wxStaticText( m_changesPanel, wxID_ANY, _("Changes to be applied:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticChangesLabel->Wrap( -1 );
	changesSizer->Add( staticChangesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_changesDataView = new wxDataViewCtrl( m_changesPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	changesSizer->Add( m_changesDataView, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_changesPanel->SetSizer( changesSizer );
	m_changesPanel->Layout();
	changesSizer->Fit( m_changesPanel );
	m_outputBook->AddPage( m_changesPanel, _("a page"), false );
	m_runningPanel = new wxPanel( m_outputBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* progressSizer;
	progressSizer = new wxBoxSizer( wxVERTICAL );

	staticProgressLabel = new wxStaticText( m_runningPanel, wxID_ANY, _("Progress:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticProgressLabel->Wrap( -1 );
	progressSizer->Add( staticProgressLabel, 0, wxALL, 5 );

	m_tcReport = new wxTextCtrl( m_runningPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	progressSizer->Add( m_tcReport, 1, wxEXPAND|wxRIGHT|wxLEFT, 3 );


	m_runningPanel->SetSizer( progressSizer );
	m_runningPanel->Layout();
	progressSizer->Fit( m_runningPanel );
	m_outputBook->AddPage( m_runningPanel, _("a page"), false );

	bLowerSizer->Add( m_outputBook, 1, wxEXPAND, 5 );


	bSizerMain->Add( bLowerSizer, 1, wxEXPAND|wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxALIGN_RIGHT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::onInitDialog ) );
	m_cleanShortCircuitOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_cleanViasOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteDanglingViasOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_mergeSegmOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteUnconnectedOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteTracksInPadsOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_netFilterOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_netclassFilterOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_netclassFilter->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnNetclassFilterSelect ), NULL, this );
	m_layerFilterOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_layerFilter->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnLayerFilterSelect ), NULL, this );
	m_selectedItemsFilter->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_changesDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnSelectItem ), NULL, this );
	m_changesDataView->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnLeftDClickItem ), NULL, this );
}

DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::~DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::onInitDialog ) );
	m_cleanShortCircuitOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_cleanViasOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteDanglingViasOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_mergeSegmOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteUnconnectedOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteTracksInPadsOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_netFilterOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_netclassFilterOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_netclassFilter->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnNetclassFilterSelect ), NULL, this );
	m_layerFilterOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_layerFilter->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnLayerFilterSelect ), NULL, this );
	m_selectedItemsFilter->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_changesDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnSelectItem ), NULL, this );
	m_changesDataView->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnLeftDClickItem ), NULL, this );

}

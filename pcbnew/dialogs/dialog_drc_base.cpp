///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_drc_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DRC_CONTROL_BASE::DIALOG_DRC_CONTROL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 10 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxBoxSizer* bSizerOptions;
	bSizerOptions = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgMinValuesSizer;
	fgMinValuesSizer = new wxFlexGridSizer( 4, 3, 0, 0 );
	fgMinValuesSizer->AddGrowableCol( 1 );
	fgMinValuesSizer->SetFlexibleDirection( wxHORIZONTAL );
	fgMinValuesSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_MinWidthLabel = new wxStaticText( this, wxID_ANY, _("Minimum track width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MinWidthLabel->Wrap( -1 );
	m_MinWidthLabel->SetToolTip( _("Enter the minimum acceptable value for a track width") );

	fgMinValuesSizer->Add( m_MinWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_MinWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgMinValuesSizer->Add( m_MinWidthCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );

	m_MinWidthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MinWidthUnits->Wrap( -1 );
	m_MinWidthUnits->SetToolTip( _("Enter the minimum acceptable value for a track width") );

	fgMinValuesSizer->Add( m_MinWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ViaMinLabel = new wxStaticText( this, wxID_ANY, _("Minimum via size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinLabel->Wrap( -1 );
	m_ViaMinLabel->SetHelpText( _("Enter the minimum acceptable diameter for a standard via") );

	fgMinValuesSizer->Add( m_ViaMinLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ViaMinCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgMinValuesSizer->Add( m_ViaMinCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );

	m_ViaMinUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinUnits->Wrap( -1 );
	m_ViaMinUnits->SetHelpText( _("Enter the minimum acceptable diameter for a standard via") );

	fgMinValuesSizer->Add( m_ViaMinUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_uViaMinLabel = new wxStaticText( this, wxID_ANY, _("Minimum uVia size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_uViaMinLabel->Wrap( -1 );
	m_uViaMinLabel->SetToolTip( _("Enter the minimum acceptable diameter for a micro via") );

	fgMinValuesSizer->Add( m_uViaMinLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_uViaMinCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgMinValuesSizer->Add( m_uViaMinCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );

	m_uViaMinUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_uViaMinUnits->Wrap( -1 );
	m_uViaMinUnits->SetToolTip( _("Enter the minimum acceptable diameter for a micro via") );

	fgMinValuesSizer->Add( m_uViaMinUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerOptions->Add( fgMinValuesSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bSizerOptSettings;
	bSizerOptSettings = new wxBoxSizer( wxVERTICAL );

	m_cbRefillZones = new wxCheckBox( this, wxID_ANY, _("Refill all zones before performing DRC"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerOptSettings->Add( m_cbRefillZones, 0, wxALL, 5 );

	m_cbReportAllTrackErrors = new wxCheckBox( this, wxID_ANY, _("Report all errors for tracks (slower)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbReportAllTrackErrors->SetToolTip( _("If selected, all DRC violations for tracks will be reported.  This can be slow for complicated designs.\n\nIf unselected, only the first DRC violation will be reported for each track connection.") );

	bSizerOptSettings->Add( m_cbReportAllTrackErrors, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbReportTracksToZonesErrors = new wxCheckBox( this, wxID_ANY, _("Test tracks against filled copper areas (very slow)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbReportTracksToZonesErrors->SetToolTip( _("If selected, tracks will be tested against copper zones. \nIf copper zones are up to date, this test should be not needed.\n\nThis test can be *very slow* for complicated designs.") );

	bSizerOptSettings->Add( m_cbReportTracksToZonesErrors, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbTestFootprints = new wxCheckBox( this, wxID_ANY, _("Test footprints against schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerOptSettings->Add( m_cbTestFootprints, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerOptions->Add( bSizerOptSettings, 1, wxEXPAND, 5 );


	gbSizer1->Add( bSizerOptions, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_Messages = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTE_MULTILINE|wxTE_READONLY );
	m_Messages->SetMinSize( wxSize( 280,-1 ) );

	gbSizer1->Add( m_Messages, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	gbSizer1->AddGrowableCol( 0 );
	gbSizer1->AddGrowableCol( 1 );

	m_MainSizer->Add( gbSizer1, 0, wxEXPAND|wxALL, 5 );

	m_Notebook = new wxNotebook( this, ID_NOTEBOOK1, wxDefaultPosition, wxDefaultSize, 0 );
	m_Notebook->SetMinSize( wxSize( 640,280 ) );

	m_panelViolations = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerViolationsBox;
	bSizerViolationsBox = new wxBoxSizer( wxVERTICAL );

	m_markerDataView = new wxDataViewCtrl( m_panelViolations, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	m_markerDataView->SetToolTip( _("Click on items to highlight them on the board.") );

	bSizerViolationsBox->Add( m_markerDataView, 1, wxALL|wxEXPAND, 5 );


	m_panelViolations->SetSizer( bSizerViolationsBox );
	m_panelViolations->Layout();
	bSizerViolationsBox->Fit( m_panelViolations );
	m_Notebook->AddPage( m_panelViolations, _("Violations / Markers (%d)"), false );
	m_panelUnconnectedItems = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerUnconnectedBox;
	bSizerUnconnectedBox = new wxBoxSizer( wxVERTICAL );

	m_unconnectedDataView = new wxDataViewCtrl( m_panelUnconnectedItems, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	bSizerUnconnectedBox->Add( m_unconnectedDataView, 1, wxALL|wxEXPAND, 5 );


	m_panelUnconnectedItems->SetSizer( bSizerUnconnectedBox );
	m_panelUnconnectedItems->Layout();
	bSizerUnconnectedBox->Fit( m_panelUnconnectedItems );
	m_Notebook->AddPage( m_panelUnconnectedItems, _("Unconnected Items (%d)"), true );
	m_panelFootprintWarnings = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerFootprintsBox;
	bSizerFootprintsBox = new wxBoxSizer( wxVERTICAL );

	m_footprintsDataView = new wxDataViewCtrl( m_panelFootprintWarnings, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	bSizerFootprintsBox->Add( m_footprintsDataView, 1, wxALL|wxEXPAND, 5 );


	m_panelFootprintWarnings->SetSizer( bSizerFootprintsBox );
	m_panelFootprintWarnings->Layout();
	bSizerFootprintsBox->Fit( m_panelFootprintWarnings );
	m_Notebook->AddPage( m_panelFootprintWarnings, _("Footprint Warnings (%d)"), false );

	m_MainSizer->Add( m_Notebook, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSeveritySizer;
	bSeveritySizer = new wxBoxSizer( wxHORIZONTAL );

	m_showLabel = new wxStaticText( this, wxID_ANY, _("Show:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showLabel->Wrap( -1 );
	bSeveritySizer->Add( m_showLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_showAll = new wxCheckBox( this, wxID_ANY, _("All"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showAll, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSeveritySizer->Add( 35, 0, 0, wxEXPAND, 5 );

	m_showErrors = new wxCheckBox( this, wxID_ANY, _("Errors"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showErrors, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_errorsBadge = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_errorsBadge->SetMinSize( wxSize( 20,20 ) );

	bSeveritySizer->Add( m_errorsBadge, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 25 );

	m_showWarnings = new wxCheckBox( this, wxID_ANY, _("Warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showWarnings, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_warningsBadge = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_warningsBadge->SetMinSize( wxSize( 20,20 ) );

	bSeveritySizer->Add( m_warningsBadge, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 25 );

	m_showExclusions = new wxCheckBox( this, wxID_ANY, _("Exclusions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showExclusions, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_exclusionsBadge = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_exclusionsBadge, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 25 );


	bSeveritySizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_saveReport = new wxButton( this, wxID_ANY, _("Save..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_saveReport, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizer9->Add( bSeveritySizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_MainSizer->Add( bSizer9, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_MainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_DeleteCurrentMarkerButton = new wxButton( this, wxID_ANY, _("Delete Marker"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerButtons->Add( m_DeleteCurrentMarkerButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_DeleteAllMarkersButton = new wxButton( this, wxID_ANY, _("Delete All Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerButtons->Add( m_DeleteAllMarkersButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_sizerButtons->Add( m_sdbSizer1, 1, wxEXPAND|wxALL, 5 );


	m_MainSizer->Add( m_sizerButtons, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_ACTIVATE, wxActivateEventHandler( DIALOG_DRC_CONTROL_BASE::OnActivateDlg ) );
	m_Notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_DRC_CONTROL_BASE::OnChangingNotebookPage ), NULL, this );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemDClick ), NULL, this );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemRClick ), NULL, this );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemSelected ), NULL, this );
	m_unconnectedDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemDClick ), NULL, this );
	m_unconnectedDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemSelected ), NULL, this );
	m_footprintsDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemDClick ), NULL, this );
	m_footprintsDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemSelected ), NULL, this );
	m_showAll->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnSeverity ), NULL, this );
	m_showErrors->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnSeverity ), NULL, this );
	m_showWarnings->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnSeverity ), NULL, this );
	m_showExclusions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnSeverity ), NULL, this );
	m_saveReport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnSaveReport ), NULL, this );
	m_DeleteCurrentMarkerButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteOneClick ), NULL, this );
	m_DeleteAllMarkersButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteAllClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnRunDRCClick ), NULL, this );
}

DIALOG_DRC_CONTROL_BASE::~DIALOG_DRC_CONTROL_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_ACTIVATE, wxActivateEventHandler( DIALOG_DRC_CONTROL_BASE::OnActivateDlg ) );
	m_Notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_DRC_CONTROL_BASE::OnChangingNotebookPage ), NULL, this );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemDClick ), NULL, this );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemRClick ), NULL, this );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemSelected ), NULL, this );
	m_unconnectedDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemDClick ), NULL, this );
	m_unconnectedDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemSelected ), NULL, this );
	m_footprintsDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemDClick ), NULL, this );
	m_footprintsDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_CONTROL_BASE::OnDRCItemSelected ), NULL, this );
	m_showAll->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnSeverity ), NULL, this );
	m_showErrors->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnSeverity ), NULL, this );
	m_showWarnings->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnSeverity ), NULL, this );
	m_showExclusions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnSeverity ), NULL, this );
	m_saveReport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnSaveReport ), NULL, this );
	m_DeleteCurrentMarkerButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteOneClick ), NULL, this );
	m_DeleteAllMarkersButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteAllClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnRunDRCClick ), NULL, this );

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_drclistbox.h"

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

	m_TrackMinWidthTitle = new wxStaticText( this, wxID_ANY, _("Minimum track width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackMinWidthTitle->Wrap( -1 );
	m_TrackMinWidthTitle->SetToolTip( _("Enter the minimum acceptable value for a track width") );

	fgMinValuesSizer->Add( m_TrackMinWidthTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_SetTrackMinWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgMinValuesSizer->Add( m_SetTrackMinWidthCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );

	m_TrackMinWidthUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackMinWidthUnit->Wrap( -1 );
	m_TrackMinWidthUnit->SetToolTip( _("Enter the minimum acceptable value for a track width") );

	fgMinValuesSizer->Add( m_TrackMinWidthUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ViaMinTitle = new wxStaticText( this, wxID_ANY, _("Minimum via size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinTitle->Wrap( -1 );
	m_ViaMinTitle->SetHelpText( _("Enter the minimum acceptable diameter for a standard via") );

	fgMinValuesSizer->Add( m_ViaMinTitle, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_SetViaMinSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgMinValuesSizer->Add( m_SetViaMinSizeCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );

	m_ViaMinUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinUnit->Wrap( -1 );
	m_ViaMinUnit->SetHelpText( _("Enter the minimum acceptable diameter for a standard via") );

	fgMinValuesSizer->Add( m_ViaMinUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_MicroViaMinTitle = new wxStaticText( this, wxID_ANY, _("Minimum uVia size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MicroViaMinTitle->Wrap( -1 );
	m_MicroViaMinTitle->SetToolTip( _("Enter the minimum acceptable diameter for a micro via") );

	fgMinValuesSizer->Add( m_MicroViaMinTitle, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_SetMicroViakMinSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgMinValuesSizer->Add( m_SetMicroViakMinSizeCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );

	m_MicroViaMinUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MicroViaMinUnit->Wrap( -1 );
	m_MicroViaMinUnit->SetToolTip( _("Enter the minimum acceptable diameter for a micro via") );

	fgMinValuesSizer->Add( m_MicroViaMinUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


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

	gbSizer1->Add( m_Messages, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxLEFT, 5 );

	wxFlexGridSizer* fgSizerRpt;
	fgSizerRpt = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerRpt->AddGrowableCol( 1 );
	fgSizerRpt->SetFlexibleDirection( wxBOTH );
	fgSizerRpt->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_CreateRptCtrl = new wxCheckBox( this, ID_CHECKBOX_RPT_FILE, _("Create report file:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CreateRptCtrl->SetToolTip( _("Enable writing report to this file") );

	fgSizerRpt->Add( m_CreateRptCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_RptFilenameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RptFilenameCtrl->SetToolTip( _("Enter the report filename") );
	m_RptFilenameCtrl->SetMinSize( wxSize( 180,-1 ) );

	fgSizerRpt->Add( m_RptFilenameCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 3 );

	m_BrowseButton = new wxBitmapButton( this, ID_BUTTON_BROWSE_RPT_FILE, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_BrowseButton->SetMinSize( wxSize( 30,28 ) );

	fgSizerRpt->Add( m_BrowseButton, 0, wxALIGN_CENTER_VERTICAL, 2 );


	gbSizer1->Add( fgSizerRpt, wxGBPosition( 1, 0 ), wxGBSpan( 1, 3 ), wxEXPAND|wxTOP|wxRIGHT, 7 );


	gbSizer1->AddGrowableCol( 0 );
	gbSizer1->AddGrowableCol( 1 );

	m_MainSizer->Add( gbSizer1, 0, wxEXPAND|wxALL, 5 );

	m_Notebook = new wxNotebook( this, ID_NOTEBOOK1, wxDefaultPosition, wxDefaultSize, 0 );
	m_Notebook->SetMinSize( wxSize( 640,280 ) );

	m_panelViolations = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerViolationsBox;
	bSizerViolationsBox = new wxBoxSizer( wxVERTICAL );

	m_ClearanceListBox = new DRCLISTBOX( m_panelViolations, ID_CLEARANCE_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_ClearanceListBox->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_ClearanceListBox->SetToolTip( _("Left-click to center on problem marker. \nRight-click to highlight items.") );

	bSizerViolationsBox->Add( m_ClearanceListBox, 1, wxEXPAND|wxALL, 5 );


	m_panelViolations->SetSizer( bSizerViolationsBox );
	m_panelViolations->Layout();
	bSizerViolationsBox->Fit( m_panelViolations );
	m_Notebook->AddPage( m_panelViolations, _("Violations / Markers (%d)"), false );
	m_panelUnconnectedItems = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerUnconnectedBox;
	bSizerUnconnectedBox = new wxBoxSizer( wxVERTICAL );

	m_UnconnectedListBox = new DRCLISTBOX( m_panelUnconnectedItems, ID_UNCONNECTED_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_UnconnectedListBox->SetToolTip( _("Left-click to center on unconnected pair. \nRight-click to highlight unconnected items.") );

	bSizerUnconnectedBox->Add( m_UnconnectedListBox, 1, wxALL|wxEXPAND, 5 );


	m_panelUnconnectedItems->SetSizer( bSizerUnconnectedBox );
	m_panelUnconnectedItems->Layout();
	bSizerUnconnectedBox->Fit( m_panelUnconnectedItems );
	m_Notebook->AddPage( m_panelUnconnectedItems, _("Unconnected Items (%d)"), true );
	m_panelFootprintWarnings = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerFootprintsBox;
	bSizerFootprintsBox = new wxBoxSizer( wxVERTICAL );

	m_FootprintsListBox = new DRCLISTBOX( m_panelFootprintWarnings, ID_FOOTPRINTS_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bSizerFootprintsBox->Add( m_FootprintsListBox, 1, wxALL|wxEXPAND, 5 );


	m_panelFootprintWarnings->SetSizer( bSizerFootprintsBox );
	m_panelFootprintWarnings->Layout();
	bSizerFootprintsBox->Fit( m_panelFootprintWarnings );
	m_Notebook->AddPage( m_panelFootprintWarnings, _("Footprint Warnings (%d)"), false );

	m_MainSizer->Add( m_Notebook, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );

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
	m_CreateRptCtrl->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnReportCheckBoxClicked ), NULL, this );
	m_RptFilenameCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnReportFilenameEdited ), NULL, this );
	m_BrowseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnButtonBrowseRptFileClick ), NULL, this );
	m_Notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_DRC_CONTROL_BASE::OnChangingMarkerList ), NULL, this );
	m_ClearanceListBox->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftDClickClearance ), NULL, this );
	m_ClearanceListBox->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftUpClearance ), NULL, this );
	m_ClearanceListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnMarkerSelectionEvent ), NULL, this );
	m_ClearanceListBox->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnRightUpClearance ), NULL, this );
	m_UnconnectedListBox->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftDClickUnconnected ), NULL, this );
	m_UnconnectedListBox->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftUpUnconnected ), NULL, this );
	m_UnconnectedListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnUnconnectedSelectionEvent ), NULL, this );
	m_UnconnectedListBox->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnRightUpUnconnected ), NULL, this );
	m_FootprintsListBox->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftDClickFootprints ), NULL, this );
	m_FootprintsListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnFootprintsSelectionEvent ), NULL, this );
	m_FootprintsListBox->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnRightUpFootprints ), NULL, this );
	m_DeleteCurrentMarkerButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteOneClick ), NULL, this );
	m_DeleteAllMarkersButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteAllClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnStartdrcClick ), NULL, this );
}

DIALOG_DRC_CONTROL_BASE::~DIALOG_DRC_CONTROL_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_ACTIVATE, wxActivateEventHandler( DIALOG_DRC_CONTROL_BASE::OnActivateDlg ) );
	m_CreateRptCtrl->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnReportCheckBoxClicked ), NULL, this );
	m_RptFilenameCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnReportFilenameEdited ), NULL, this );
	m_BrowseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnButtonBrowseRptFileClick ), NULL, this );
	m_Notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_DRC_CONTROL_BASE::OnChangingMarkerList ), NULL, this );
	m_ClearanceListBox->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftDClickClearance ), NULL, this );
	m_ClearanceListBox->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftUpClearance ), NULL, this );
	m_ClearanceListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnMarkerSelectionEvent ), NULL, this );
	m_ClearanceListBox->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnRightUpClearance ), NULL, this );
	m_UnconnectedListBox->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftDClickUnconnected ), NULL, this );
	m_UnconnectedListBox->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftUpUnconnected ), NULL, this );
	m_UnconnectedListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnUnconnectedSelectionEvent ), NULL, this );
	m_UnconnectedListBox->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnRightUpUnconnected ), NULL, this );
	m_FootprintsListBox->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftDClickFootprints ), NULL, this );
	m_FootprintsListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnFootprintsSelectionEvent ), NULL, this );
	m_FootprintsListBox->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnRightUpFootprints ), NULL, this );
	m_DeleteCurrentMarkerButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteOneClick ), NULL, this );
	m_DeleteAllMarkersButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteAllClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnStartdrcClick ), NULL, this );

}

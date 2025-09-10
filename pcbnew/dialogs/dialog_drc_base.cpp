///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_html_report_box.h"

#include "dialog_drc_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DRC_BASE::DIALOG_DRC_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerOptions;
	bSizerOptions = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );


	bSizerOptions->Add( bSizer12, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerOptSettings;
	bSizerOptSettings = new wxBoxSizer( wxVERTICAL );


	bSizerOptions->Add( bSizerOptSettings, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_MainSizer->Add( bSizerOptions, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 3 );

	wxGridBagSizer* gbSizerOptions;
	gbSizerOptions = new wxGridBagSizer( 0, 0 );
	gbSizerOptions->SetFlexibleDirection( wxBOTH );
	gbSizerOptions->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbRefillZones = new wxCheckBox( this, wxID_ANY, _("Refill all zones before performing DRC"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRefillZones->SetValue(true);
	gbSizerOptions->Add( m_cbRefillZones, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_cbTestFootprints = new wxCheckBox( this, wxID_ANY, _("Test for parity between PCB and schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerOptions->Add( m_cbTestFootprints, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_bMenu = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bMenu->SetMinSize( wxSize( 30,30 ) );

	gbSizerOptions->Add( m_bMenu, wxGBPosition( 0, 2 ), wxGBSpan( 2, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	gbSizerOptions->AddGrowableCol( 0 );
	gbSizerOptions->AddGrowableCol( 1 );
	gbSizerOptions->AddGrowableRow( 0 );
	gbSizerOptions->AddGrowableRow( 1 );

	m_MainSizer->Add( gbSizerOptions, 0, wxEXPAND|wxTOP|wxLEFT, 10 );

	m_runningResultsBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	running = new wxPanel( m_runningResultsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );

	m_runningNotebook = new wxNotebook( running, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelMessages = new wxPanel( m_runningNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	m_messages = new WX_HTML_REPORT_BOX( m_panelMessages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	bSizer10->Add( m_messages, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bGaugeMargins;
	bGaugeMargins = new wxBoxSizer( wxVERTICAL );

	m_gauge = new wxGauge( m_panelMessages, wxID_ANY, 1000, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	m_gauge->SetValue( 0 );
	bGaugeMargins->Add( m_gauge, 0, wxALL|wxEXPAND, 5 );


	bSizer10->Add( bGaugeMargins, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_panelMessages->SetSizer( bSizer10 );
	m_panelMessages->Layout();
	bSizer10->Fit( m_panelMessages );
	m_runningNotebook->AddPage( m_panelMessages, _("Tests Running..."), true );

	bSizer14->Add( m_runningNotebook, 1, wxEXPAND, 5 );


	running->SetSizer( bSizer14 );
	running->Layout();
	bSizer14->Fit( running );
	m_runningResultsBook->AddPage( running, _("a page"), false );
	results = new wxPanel( m_runningResultsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	m_Notebook = new wxNotebook( results, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_Notebook->SetMinSize( wxSize( 640,-1 ) );

	m_panelViolations = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerViolationsBox;
	bSizerViolationsBox = new wxBoxSizer( wxVERTICAL );

	bSizerViolationsBox->SetMinSize( wxSize( -1,320 ) );
	m_markerDataView = new wxDataViewCtrl( m_panelViolations, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	bSizerViolationsBox->Add( m_markerDataView, 1, wxALL|wxEXPAND, 5 );


	bSizerViolationsBox->Add( 0, 8, 0, wxEXPAND, 5 );


	m_panelViolations->SetSizer( bSizerViolationsBox );
	m_panelViolations->Layout();
	bSizerViolationsBox->Fit( m_panelViolations );
	m_Notebook->AddPage( m_panelViolations, _("Violations (%s)"), false );
	m_panelUnconnectedItems = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerUnconnectedBox;
	bSizerUnconnectedBox = new wxBoxSizer( wxVERTICAL );

	m_unconnectedDataView = new wxDataViewCtrl( m_panelUnconnectedItems, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	bSizerUnconnectedBox->Add( m_unconnectedDataView, 1, wxALL|wxEXPAND, 5 );


	bSizerUnconnectedBox->Add( 0, 8, 0, wxEXPAND, 5 );


	m_panelUnconnectedItems->SetSizer( bSizerUnconnectedBox );
	m_panelUnconnectedItems->Layout();
	bSizerUnconnectedBox->Fit( m_panelUnconnectedItems );
	m_Notebook->AddPage( m_panelUnconnectedItems, _("Unconnected Items (%s)"), false );
	m_panelFootprintWarnings = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerFootprintsBox;
	bSizerFootprintsBox = new wxBoxSizer( wxVERTICAL );

	m_footprintsDataView = new wxDataViewCtrl( m_panelFootprintWarnings, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	bSizerFootprintsBox->Add( m_footprintsDataView, 1, wxALL|wxEXPAND, 5 );


	bSizerFootprintsBox->Add( 0, 8, 0, wxEXPAND, 5 );


	m_panelFootprintWarnings->SetSizer( bSizerFootprintsBox );
	m_panelFootprintWarnings->Layout();
	bSizerFootprintsBox->Fit( m_panelFootprintWarnings );
	m_Notebook->AddPage( m_panelFootprintWarnings, _("Schematic Parity (%s)"), false );
	m_panelIgnored = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );

	m_ignoredList = new wxListCtrl( m_panelIgnored, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER|wxLC_REPORT );
	bSizer15->Add( m_ignoredList, 1, wxALL|wxEXPAND, 5 );

	m_violationSeveritiesLink = new wxHyperlinkCtrl( m_panelIgnored, wxID_ANY, _("Edit ignored tests"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	bSizer15->Add( m_violationSeveritiesLink, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer15->Add( 0, 7, 0, wxEXPAND, 5 );


	m_panelIgnored->SetSizer( bSizer15 );
	m_panelIgnored->Layout();
	bSizer15->Fit( m_panelIgnored );
	m_Notebook->AddPage( m_panelIgnored, _("Ignored Tests (%s)"), false );

	bSizer13->Add( m_Notebook, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	results->SetSizer( bSizer13 );
	results->Layout();
	bSizer13->Fit( results );
	m_runningResultsBook->AddPage( results, _("a page"), true );

	m_MainSizer->Add( m_runningResultsBook, 1, wxEXPAND, 5 );

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
	m_showErrors->SetValue(true);
	bSeveritySizer->Add( m_showErrors, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_errorsBadge = new NUMBER_BADGE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_errorsBadge, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 25 );

	m_showWarnings = new wxCheckBox( this, wxID_ANY, _("Warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showWarnings->SetValue(true);
	bSeveritySizer->Add( m_showWarnings, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_warningsBadge = new NUMBER_BADGE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_warningsBadge, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 25 );

	m_showExclusions = new wxCheckBox( this, wxID_ANY, _("Exclusions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showExclusions, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_exclusionsBadge = new NUMBER_BADGE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_exclusionsBadge, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 25 );


	bSeveritySizer->Add( 5, 0, 1, wxEXPAND, 5 );

	m_saveReport = new wxButton( this, wxID_ANY, _("Save..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_saveReport, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizer9->Add( bSeveritySizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_MainSizer->Add( bSizer9, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_sizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_DeleteCurrentMarkerButton = new wxButton( this, wxID_ANY, _("Delete Marker"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerButtons->Add( m_DeleteCurrentMarkerButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_DeleteAllMarkersButton = new wxButton( this, wxID_ANY, _("Delete All Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerButtons->Add( m_DeleteAllMarkersButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_sizerButtons->Add( m_sdbSizer, 1, wxEXPAND|wxALL, 5 );


	m_MainSizer->Add( m_sizerButtons, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_ACTIVATE, wxActivateEventHandler( DIALOG_DRC_BASE::OnActivateDlg ) );
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_DRC_BASE::OnClose ) );
	m_bMenu->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnMenu ), NULL, this );
	m_messages->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_DRC_BASE::OnErrorLinkClicked ), NULL, this );
	m_Notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_DRC_BASE::OnChangingNotebookPage ), NULL, this );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemDClick ), NULL, this );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemRClick ), NULL, this );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemSelected ), NULL, this );
	m_unconnectedDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemDClick ), NULL, this );
	m_unconnectedDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemRClick ), NULL, this );
	m_unconnectedDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemSelected ), NULL, this );
	m_footprintsDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemDClick ), NULL, this );
	m_footprintsDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemRClick ), NULL, this );
	m_footprintsDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemSelected ), NULL, this );
	m_ignoredList->Connect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( DIALOG_DRC_BASE::OnIgnoredItemRClick ), NULL, this );
	m_violationSeveritiesLink->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_DRC_BASE::OnEditViolationSeverities ), NULL, this );
	m_showAll->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnSeverity ), NULL, this );
	m_showErrors->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnSeverity ), NULL, this );
	m_showWarnings->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnSeverity ), NULL, this );
	m_showExclusions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnSeverity ), NULL, this );
	m_saveReport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnSaveReport ), NULL, this );
	m_DeleteCurrentMarkerButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnDeleteOneClick ), NULL, this );
	m_DeleteAllMarkersButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnDeleteAllClick ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnRunDRCClick ), NULL, this );
}

DIALOG_DRC_BASE::~DIALOG_DRC_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_ACTIVATE, wxActivateEventHandler( DIALOG_DRC_BASE::OnActivateDlg ) );
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_DRC_BASE::OnClose ) );
	m_bMenu->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnMenu ), NULL, this );
	m_messages->Disconnect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_DRC_BASE::OnErrorLinkClicked ), NULL, this );
	m_Notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_DRC_BASE::OnChangingNotebookPage ), NULL, this );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemDClick ), NULL, this );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemRClick ), NULL, this );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemSelected ), NULL, this );
	m_unconnectedDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemDClick ), NULL, this );
	m_unconnectedDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemRClick ), NULL, this );
	m_unconnectedDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemSelected ), NULL, this );
	m_footprintsDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemDClick ), NULL, this );
	m_footprintsDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemRClick ), NULL, this );
	m_footprintsDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_DRC_BASE::OnDRCItemSelected ), NULL, this );
	m_ignoredList->Disconnect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( DIALOG_DRC_BASE::OnIgnoredItemRClick ), NULL, this );
	m_violationSeveritiesLink->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_DRC_BASE::OnEditViolationSeverities ), NULL, this );
	m_showAll->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnSeverity ), NULL, this );
	m_showErrors->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnSeverity ), NULL, this );
	m_showWarnings->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnSeverity ), NULL, this );
	m_showExclusions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnSeverity ), NULL, this );
	m_saveReport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnSaveReport ), NULL, this );
	m_DeleteCurrentMarkerButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnDeleteOneClick ), NULL, this );
	m_DeleteAllMarkersButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnDeleteAllClick ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_BASE::OnRunDRCClick ), NULL, this );

}

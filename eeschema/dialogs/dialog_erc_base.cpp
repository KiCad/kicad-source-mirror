///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_html_report_box.h"
#include "widgets/wx_infobar.h"

#include "dialog_erc_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ERC_BASE::DIALOG_ERC_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	bSizer1->Add( m_infoBar, 0, wxEXPAND, 5 );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizerOptions;
	gbSizerOptions = new wxGridBagSizer( 0, 0 );
	gbSizerOptions->SetFlexibleDirection( wxBOTH );
	gbSizerOptions->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_bMenu = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bMenu->SetMinSize( wxSize( 30,30 ) );

	gbSizerOptions->Add( m_bMenu, wxGBPosition( 0, 2 ), wxGBSpan( 2, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbSizerOptions->AddGrowableCol( 0 );
	gbSizerOptions->AddGrowableCol( 1 );
	gbSizerOptions->AddGrowableRow( 0 );
	gbSizerOptions->AddGrowableRow( 1 );

	bMainSizer->Add( gbSizerOptions, 0, wxEXPAND|wxLEFT, 5 );

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

	m_notebook = new wxNotebook( results, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	violationsPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bViolationsSizer;
	bViolationsSizer = new wxBoxSizer( wxVERTICAL );

	m_markerDataView = new wxDataViewCtrl( violationsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	m_markerDataView->SetMinSize( wxSize( 640,260 ) );

	bViolationsSizer->Add( m_markerDataView, 2, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bViolationsSizer->Add( 0, 8, 0, wxEXPAND, 5 );


	violationsPanel->SetSizer( bViolationsSizer );
	violationsPanel->Layout();
	bViolationsSizer->Fit( violationsPanel );
	m_notebook->AddPage( violationsPanel, _("Violations (%s)"), false );
	m_panelIgnored = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );

	m_ignoredList = new wxListCtrl( m_panelIgnored, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER|wxLC_REPORT );
	bSizer15->Add( m_ignoredList, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_violationSeveritiesLink = new wxHyperlinkCtrl( m_panelIgnored, wxID_ANY, _("Edit ignored tests"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	bSizer15->Add( m_violationSeveritiesLink, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer15->Add( 0, 8, 0, wxEXPAND, 5 );


	m_panelIgnored->SetSizer( bSizer15 );
	m_panelIgnored->Layout();
	bSizer15->Fit( m_panelIgnored );
	m_notebook->AddPage( m_panelIgnored, _("Ignored Tests (%s)"), false );

	bSizer13->Add( m_notebook, 1, wxEXPAND, 5 );


	results->SetSizer( bSizer13 );
	results->Layout();
	bSizer13->Fit( results );
	m_runningResultsBook->AddPage( results, _("a page"), true );

	bMainSizer->Add( m_runningResultsBook, 1, wxEXPAND|wxTOP|wxRIGHT, 5 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSeveritySizer;
	bSeveritySizer = new wxBoxSizer( wxHORIZONTAL );

	m_showLabel = new wxStaticText( this, wxID_ANY, _("Show:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showLabel->Wrap( -1 );
	bSeveritySizer->Add( m_showLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_showAll = new wxCheckBox( this, wxID_ANY, _("All"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showAll, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSeveritySizer->Add( 35, 0, 0, wxEXPAND, 5 );

	m_showErrors = new wxCheckBox( this, wxID_ANY, _("Errors"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showErrors, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_errorsBadge = new NUMBER_BADGE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_errorsBadge, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 25 );

	m_showWarnings = new wxCheckBox( this, wxID_ANY, _("Warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showWarnings, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_warningsBadge = new NUMBER_BADGE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_warningsBadge, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 25 );

	m_showExclusions = new wxCheckBox( this, wxID_ANY, _("Exclusions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showExclusions, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_exclusionsBadge = new NUMBER_BADGE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_exclusionsBadge, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 25 );


	bSeveritySizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_saveReport = new wxButton( this, wxID_ANY, _("Save..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_saveReport, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizer11->Add( bSeveritySizer, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bSizer11, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizer1->Add( bMainSizer, 1, wxEXPAND|wxALL, 5 );

	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_deleteOneMarker = new wxButton( this, wxID_ANY, _("Delete Marker"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonsSizer->Add( m_deleteOneMarker, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_deleteAllMarkers = new wxButton( this, ID_ERASE_DRC_MARKERS, _("Delete All Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonsSizer->Add( m_deleteAllMarkers, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 8 );


	m_buttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_buttonsSizer->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );


	bSizer1->Add( m_buttonsSizer, 0, wxEXPAND|wxLEFT, 10 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_ERC_BASE::OnCloseErcDialog ) );
	m_bMenu->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnMenu ), NULL, this );
	m_messages->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_ERC_BASE::OnLinkClicked ), NULL, this );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemDClick ), NULL, this );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemRClick ), NULL, this );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemSelected ), NULL, this );
	m_ignoredList->Connect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( DIALOG_ERC_BASE::OnIgnoredItemRClick ), NULL, this );
	m_violationSeveritiesLink->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_ERC_BASE::OnEditViolationSeverities ), NULL, this );
	m_showAll->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showErrors->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showWarnings->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showExclusions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_saveReport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSaveReport ), NULL, this );
	m_deleteOneMarker->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnDeleteOneClick ), NULL, this );
	m_deleteAllMarkers->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnDeleteAllClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnRunERCClick ), NULL, this );
}

DIALOG_ERC_BASE::~DIALOG_ERC_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_ERC_BASE::OnCloseErcDialog ) );
	m_bMenu->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnMenu ), NULL, this );
	m_messages->Disconnect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_ERC_BASE::OnLinkClicked ), NULL, this );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemDClick ), NULL, this );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemRClick ), NULL, this );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemSelected ), NULL, this );
	m_ignoredList->Disconnect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( DIALOG_ERC_BASE::OnIgnoredItemRClick ), NULL, this );
	m_violationSeveritiesLink->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_ERC_BASE::OnEditViolationSeverities ), NULL, this );
	m_showAll->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showErrors->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showWarnings->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showExclusions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_saveReport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSaveReport ), NULL, this );
	m_deleteOneMarker->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnDeleteOneClick ), NULL, this );
	m_deleteAllMarkers->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnDeleteAllClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnRunERCClick ), NULL, this );

}

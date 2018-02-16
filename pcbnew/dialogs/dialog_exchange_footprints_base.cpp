///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug  4 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_exchange_footprints_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXCHANGE_FOOTPRINTS_BASE::DIALOG_EXCHANGE_FOOTPRINTS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_mainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_upperSizer = new wxGridBagSizer( 0, 0 );
	m_upperSizer->SetFlexibleDirection( wxBOTH );
	m_upperSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_matchAll = new wxRadioButton( this, wxID_ANY, _("%s all footprints on board"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_matchAll, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxEXPAND|wxALL, 5 );
	
	m_matchCurrentRef = new wxRadioButton( this, wxID_ANY, _("%s current footprint (%s)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_matchCurrentRef, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxEXPAND|wxALL, 5 );
	
	m_matchSpecifiedRef = new wxRadioButton( this, wxID_ANY, _("%s footprint with reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_matchSpecifiedRef, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );
	
	m_specifiedRef = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_specifiedRef, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxTOP|wxEXPAND, 5 );
	
	m_matchCurrentValue = new wxRadioButton( this, wxID_ANY, _("%s footprints with matching value (%s)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_matchCurrentValue, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), wxEXPAND|wxALL, 5 );
	
	m_matchSpecifiedValue = new wxRadioButton( this, wxID_ANY, _("%s footprints with value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_matchSpecifiedValue, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALL, 5 );
	
	m_specifiedValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_specifiedValue, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_matchSpecifiedID = new wxRadioButton( this, wxID_ANY, _("%s footprints with identifier:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_matchSpecifiedID, wxGBPosition( 5, 0 ), wxGBSpan( 1, 2 ), wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_specifiedID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_specifiedID->SetMinSize( wxSize( 500,22 ) );
	
	fgSizer1->Add( m_specifiedID, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 3 );
	
	m_specifiedIDBrowseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	fgSizer1->Add( m_specifiedIDBrowseButton, 0, wxALL|wxALIGN_CENTER_VERTICAL, 0 );
	
	
	m_upperSizer->Add( fgSizer1, wxGBPosition( 6, 0 ), wxGBSpan( 1, 2 ), wxEXPAND, 0 );
	
	
	m_upperSizer->AddGrowableCol( 1 );
	
	m_mainSizer->Add( m_upperSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_middleSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticLine* staticline1;
	staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_middleSizer->Add( staticline1, 0, wxBOTTOM|wxEXPAND, 2 );
	
	wxStaticText* newIdLabel;
	newIdLabel = new wxStaticText( this, wxID_ANY, _("New footprint identifier:"), wxDefaultPosition, wxDefaultSize, 0 );
	newIdLabel->Wrap( -1 );
	m_middleSizer->Add( newIdLabel, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer2->AddGrowableCol( 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_newID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_newID->SetMinSize( wxSize( 500,22 ) );
	
	fgSizer2->Add( m_newID, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 3 );
	
	m_newIDBrowseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	fgSizer2->Add( m_newIDBrowseButton, 0, wxALL|wxALIGN_CENTER_VERTICAL, 0 );
	
	
	m_middleSizer->Add( fgSizer2, 0, wxEXPAND, 0 );
	
	
	m_mainSizer->Add( m_middleSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MessageWindow = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_MessageWindow->SetMinSize( wxSize( -1,300 ) );
	
	m_mainSizer->Add( m_MessageWindow, 5, wxALL|wxEXPAND, 5 );
	
	wxStaticLine* staticline2;
	staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_mainSizer->Add( staticline2, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bottomSizer;
	bottomSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_exportButton = new wxButton( this, wxID_ANY, _("Export Footprint Associations"), wxDefaultPosition, wxDefaultSize, 0 );
	bottomSizer->Add( m_exportButton, 0, wxALL, 5 );
	
	wxBoxSizer* paddingSizer;
	paddingSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* padding1;
	padding1 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	padding1->Wrap( -1 );
	paddingSizer->Add( padding1, 1, wxALL, 5 );
	
	
	bottomSizer->Add( paddingSizer, 1, 0, 5 );
	
	m_applyButton = new wxButton( this, wxID_ANY, _("Apply"), wxDefaultPosition, wxDefaultSize, 0 );
	bottomSizer->Add( m_applyButton, 0, wxALL, 5 );
	
	m_closeButton = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bottomSizer->Add( m_closeButton, 0, wxALL, 5 );
	
	
	m_mainSizer->Add( bottomSizer, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::updateMatchModeRadioButtons ) );
	m_matchAll->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchAllClicked ), NULL, this );
	m_matchCurrentRef->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_matchSpecifiedRef->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_matchCurrentValue->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_matchSpecifiedValue->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_matchSpecifiedID->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedIDBrowseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::ViewAndSelectFootprint ), NULL, this );
	m_newIDBrowseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::ViewAndSelectFootprint ), NULL, this );
	m_exportButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::RebuildCmpList ), NULL, this );
	m_applyButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnApplyClick ), NULL, this );
	m_closeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnQuit ), NULL, this );
}

DIALOG_EXCHANGE_FOOTPRINTS_BASE::~DIALOG_EXCHANGE_FOOTPRINTS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::updateMatchModeRadioButtons ) );
	m_matchAll->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchAllClicked ), NULL, this );
	m_matchCurrentRef->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_matchSpecifiedRef->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_matchCurrentValue->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_matchSpecifiedValue->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_matchSpecifiedID->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedIDBrowseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::ViewAndSelectFootprint ), NULL, this );
	m_newIDBrowseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::ViewAndSelectFootprint ), NULL, this );
	m_exportButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::RebuildCmpList ), NULL, this );
	m_applyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnApplyClick ), NULL, this );
	m_closeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnQuit ), NULL, this );
	
}

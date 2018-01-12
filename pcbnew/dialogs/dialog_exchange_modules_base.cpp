///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_exchange_modules_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXCHANGE_MODULE_BASE::DIALOG_EXCHANGE_MODULE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_localizationSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_updateModeTitle = new wxStaticText( this, wxID_ANY, _("Update Footprints from Library"), wxDefaultPosition, wxDefaultSize, 0 );
	m_updateModeTitle->Wrap( -1 );
	m_localizationSizer->Add( m_updateModeTitle, 0, wxALL, 5 );
	
	m_exchangeModeTitle = new wxStaticText( this, wxID_ANY, _("Change Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_exchangeModeTitle->Wrap( -1 );
	m_localizationSizer->Add( m_exchangeModeTitle, 0, wxALL, 5 );
	
	m_updateModeVerb = new wxStaticText( this, wxID_ANY, _("Update"), wxDefaultPosition, wxDefaultSize, 0 );
	m_updateModeVerb->Wrap( -1 );
	m_localizationSizer->Add( m_updateModeVerb, 0, wxALL, 5 );
	
	m_exchangeModeVerb = new wxStaticText( this, wxID_ANY, _("Change"), wxDefaultPosition, wxDefaultSize, 0 );
	m_exchangeModeVerb->Wrap( -1 );
	m_localizationSizer->Add( m_exchangeModeVerb, 0, wxALL, 5 );
	
	
	mainSizer->Add( m_localizationSizer, 0, 0, 5 );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );
	
	m_allSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_matchAll = new wxRadioButton( this, wxID_MATCH_FP_ALL, _("%s all footprints on board"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchAll->SetValue( true ); 
	m_matchAll->SetMinSize( wxSize( -1,26 ) );
	
	m_allSizer->Add( m_matchAll, 0, wxALL, 2 );
	
	
	bUpperSizer->Add( m_allSizer, 29, wxEXPAND, 5 );
	
	m_currentRefSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_matchCurrentRef = new wxRadioButton( this, wxID_MATCH_FP_REF, _("%s current footprint (%s)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchCurrentRef->SetMinSize( wxSize( -1,26 ) );
	
	m_currentRefSizer->Add( m_matchCurrentRef, 0, wxALL, 2 );
	
	
	bUpperSizer->Add( m_currentRefSizer, 31, wxEXPAND, 5 );
	
	m_specifiedRefSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_matchSpecifiedRef = new wxRadioButton( this, wxID_MATCH_FP_REF, _("%s footprint with reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSpecifiedRef->SetMinSize( wxSize( -1,26 ) );
	
	m_specifiedRefSizer->Add( m_matchSpecifiedRef, 0, wxALL, 2 );
	
	m_specifiedRef = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_specifiedRef->SetMinSize( wxSize( -1,24 ) );
	m_specifiedRef->SetMaxSize( wxSize( 80,-1 ) );
	
	m_specifiedRefSizer->Add( m_specifiedRef, 0, wxALL, 3 );
	
	
	bUpperSizer->Add( m_specifiedRefSizer, 29, wxEXPAND, 5 );
	
	m_currentValueSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_matchCurrentValue = new wxRadioButton( this, wxID_MATCH_FP_VAL, _("%s footprints with matching value (%s)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchCurrentValue->SetMinSize( wxSize( -1,26 ) );
	
	m_currentValueSizer->Add( m_matchCurrentValue, 0, wxALL, 2 );
	
	
	bUpperSizer->Add( m_currentValueSizer, 31, wxEXPAND, 5 );
	
	m_specifiedValueSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_matchSpecifiedValue = new wxRadioButton( this, wxID_MATCH_FP_VAL, _("%s footprints with value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSpecifiedValue->SetMinSize( wxSize( -1,26 ) );
	
	m_specifiedValueSizer->Add( m_matchSpecifiedValue, 0, wxALL, 2 );
	
	m_specifiedValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_specifiedValue->SetMinSize( wxSize( 160,24 ) );
	
	m_specifiedValueSizer->Add( m_specifiedValue, 0, wxALL, 3 );
	
	
	bUpperSizer->Add( m_specifiedValueSizer, 31, wxEXPAND, 5 );
	
	m_specifiedIDSizer = new wxBoxSizer( wxVERTICAL );
	
	m_matchSpecifiedID = new wxRadioButton( this, wxID_MATCH_FP_ID, _("%s footprints with identifier:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_specifiedIDSizer->Add( m_matchSpecifiedID, 0, wxALL, 2 );
	
	wxBoxSizer* specifiedIDSizer;
	specifiedIDSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_specifiedID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_specifiedID->SetMinSize( wxSize( 500,24 ) );
	
	specifiedIDSizer->Add( m_specifiedID, 1, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
	
	m_specifiedIDBrowseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	specifiedIDSizer->Add( m_specifiedIDBrowseButton, 0, wxALL, 0 );
	
	
	m_specifiedIDSizer->Add( specifiedIDSizer, 0, wxALIGN_TOP|wxEXPAND, 0 );
	
	
	bUpperSizer->Add( m_specifiedIDSizer, 45, wxEXPAND, 5 );
	
	
	mainSizer->Add( bUpperSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_middleSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticLine* staticline1;
	staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_middleSizer->Add( staticline1, 0, wxEXPAND | wxALL, 0 );
	
	wxStaticText* newIdLabel;
	newIdLabel = new wxStaticText( this, wxID_ANY, _("New footprint identifier:"), wxDefaultPosition, wxDefaultSize, 0 );
	newIdLabel->Wrap( -1 );
	m_middleSizer->Add( newIdLabel, 0, wxALL, 2 );
	
	wxBoxSizer* newIDSizer;
	newIDSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_newID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_newID->SetMinSize( wxSize( 500,24 ) );
	
	newIDSizer->Add( m_newID, 1, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
	
	m_newIDBrowseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	newIDSizer->Add( m_newIDBrowseButton, 0, wxALL, 0 );
	
	
	m_middleSizer->Add( newIDSizer, 0, wxEXPAND, 0 );
	
	
	mainSizer->Add( m_middleSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_MessageWindow = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_MessageWindow->SetMinSize( wxSize( -1,300 ) );
	
	mainSizer->Add( m_MessageWindow, 1, wxALL|wxEXPAND, 5 );
	
	wxStaticLine* staticline2;
	staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( staticline2, 0, wxEXPAND | wxALL, 5 );
	
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
	
	m_closeButton = new wxButton( this, wxID_ANY, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bottomSizer->Add( m_closeButton, 0, wxALL, 5 );
	
	
	mainSizer->Add( bottomSizer, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	// Connect Events
	m_matchAll->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchAllClicked ), NULL, this );
	m_matchCurrentRef->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchRefClicked ), NULL, this );
	m_matchSpecifiedRef->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchRefClicked ), NULL, this );
	m_matchCurrentValue->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchValueClicked ), NULL, this );
	m_matchSpecifiedValue->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchValueClicked ), NULL, this );
	m_matchSpecifiedID->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedID->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedID->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedIDBrowseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::ViewAndSelectFootprint ), NULL, this );
	m_newIDBrowseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::ViewAndSelectFootprint ), NULL, this );
	m_exportButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::RebuildCmpList ), NULL, this );
	m_applyButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnOkClick ), NULL, this );
	m_closeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnQuit ), NULL, this );
}

DIALOG_EXCHANGE_MODULE_BASE::~DIALOG_EXCHANGE_MODULE_BASE()
{
	// Disconnect Events
	m_matchAll->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchAllClicked ), NULL, this );
	m_matchCurrentRef->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchRefClicked ), NULL, this );
	m_matchSpecifiedRef->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchRefClicked ), NULL, this );
	m_matchCurrentValue->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchValueClicked ), NULL, this );
	m_matchSpecifiedValue->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchValueClicked ), NULL, this );
	m_matchSpecifiedID->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedID->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedID->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedIDBrowseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::ViewAndSelectFootprint ), NULL, this );
	m_newIDBrowseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::ViewAndSelectFootprint ), NULL, this );
	m_exportButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::RebuildCmpList ), NULL, this );
	m_applyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnOkClick ), NULL, this );
	m_closeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnQuit ), NULL, this );
	
}

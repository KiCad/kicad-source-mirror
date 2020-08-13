///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun  3 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_change_symbols_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CHANGE_SYMBOLS_BASE::DIALOG_CHANGE_SYMBOLS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_matchSizer = new wxGridBagSizer( 5, 5 );
	m_matchSizer->SetFlexibleDirection( wxBOTH );
	m_matchSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_matchAll = new wxRadioButton( this, wxID_ANY, _("%s all symbols in schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSizer->Add( m_matchAll, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_matchBySelection = new wxRadioButton( this, wxID_ANY, _("%s selected symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSizer->Add( m_matchBySelection, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3 );

	m_matchByReference = new wxRadioButton( this, wxID_ANY, _("%s by reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSizer->Add( m_matchByReference, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_specifiedReference = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), wxTE_PROCESS_ENTER );
	m_matchSizer->Add( m_specifiedReference, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_matchByValue = new wxRadioButton( this, wxID_ANY, _("%s by value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSizer->Add( m_matchByValue, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_specifiedValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_matchSizer->Add( m_specifiedValue, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_matchById = new wxRadioButton( this, wxID_ANY, _("%s by library indentifier:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSizer->Add( m_matchById, wxGBPosition( 4, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_specifiedId = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_matchSizer->Add( m_specifiedId, wxGBPosition( 5, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );

	m_matchIdBrowserButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_matchSizer->Add( m_matchIdBrowserButton, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	m_matchSizer->AddGrowableCol( 1 );
	m_matchSizer->AddGrowableRow( 1 );

	m_mainSizer->Add( m_matchSizer, 0, wxALL|wxEXPAND, 5 );

	m_newIdSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticText* m_newIdLabel;
	m_newIdLabel = new wxStaticText( this, wxID_ANY, _("New library identifier:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_newIdLabel->Wrap( -1 );
	m_newIdSizer->Add( m_newIdLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );

	m_newId = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	bSizer1->Add( m_newId, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_newIdBrowserButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer1->Add( m_newIdBrowserButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	m_newIdSizer->Add( bSizer1, 0, wxEXPAND, 5 );


	m_mainSizer->Add( m_newIdSizer, 0, wxEXPAND|wxLEFT, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_messagePanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_messagePanel->SetMinSize( wxSize( -1,240 ) );

	bSizer2->Add( m_messagePanel, 1, wxEXPAND | wxALL, 5 );


	m_mainSizer->Add( bSizer2, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_mainSizer->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_matchByReference->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchByReference ), NULL, this );
	m_matchByValue->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchByValue ), NULL, this );
	m_matchById->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchById ), NULL, this );
	m_matchIdBrowserButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::launchMatchIdSymbolBrowser ), NULL, this );
	m_newIdBrowserButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::launchNewIdSymbolBrowser ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onOkButtonClicked ), NULL, this );
}

DIALOG_CHANGE_SYMBOLS_BASE::~DIALOG_CHANGE_SYMBOLS_BASE()
{
	// Disconnect Events
	m_matchByReference->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchByReference ), NULL, this );
	m_matchByValue->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchByValue ), NULL, this );
	m_matchById->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchById ), NULL, this );
	m_matchIdBrowserButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::launchMatchIdSymbolBrowser ), NULL, this );
	m_newIdBrowserButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::launchNewIdSymbolBrowser ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onOkButtonClicked ), NULL, this );

}

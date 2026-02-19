///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_template_selector_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TEMPLATE_SELECTOR_BASE::DIALOG_TEMPLATE_SELECTOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 500,400 ), wxDefaultSize );

	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* m_mainContent;
	m_mainContent = new wxBoxSizer( wxHORIZONTAL );

	m_panelMRU = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelMRU->SetMinSize( wxSize( 220,-1 ) );

	wxBoxSizer* bSizerMRU;
	bSizerMRU = new wxBoxSizer( wxVERTICAL );

	m_labelRecentTemplates = new wxStaticText( m_panelMRU, wxID_ANY, _("Recent project templates"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelRecentTemplates->Wrap( -1 );
	bSizerMRU->Add( m_labelRecentTemplates, 0, wxALL, 5 );

	m_scrolledMRU = new wxScrolledWindow( m_panelMRU, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_scrolledMRU->SetScrollRate( 0, 10 );
	m_sizerMRU = new wxBoxSizer( wxVERTICAL );


	m_scrolledMRU->SetSizer( m_sizerMRU );
	m_scrolledMRU->Layout();
	m_sizerMRU->Fit( m_scrolledMRU );
	bSizerMRU->Add( m_scrolledMRU, 1, wxEXPAND|wxALL, 5 );


	m_panelMRU->SetSizer( bSizerMRU );
	m_panelMRU->Layout();
	bSizerMRU->Fit( m_panelMRU );
	m_mainContent->Add( m_panelMRU, 0, wxEXPAND|wxALL, 5 );

	m_panelTemplates = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerTemplates;
	bSizerTemplates = new wxBoxSizer( wxVERTICAL );

	m_searchCtrl = new wxSearchCtrl( m_panelTemplates, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	#ifndef __WXMAC__
	m_searchCtrl->ShowSearchButton( true );
	#endif
	m_searchCtrl->ShowCancelButton( true );
	bSizerTemplates->Add( m_searchCtrl, 0, wxEXPAND|wxALL, 5 );

	wxString m_filterChoiceChoices[] = { _("All Templates"), _("User Templates"), _("System Templates") };
	int m_filterChoiceNChoices = sizeof( m_filterChoiceChoices ) / sizeof( wxString );
	m_filterChoice = new wxChoice( m_panelTemplates, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_filterChoiceNChoices, m_filterChoiceChoices, 0 );
	m_filterChoice->SetSelection( 0 );
	bSizerTemplates->Add( m_filterChoice, 0, wxALL, 5 );

	m_scrolledTemplates = new wxScrolledWindow( m_panelTemplates, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_scrolledTemplates->SetScrollRate( 0, 10 );
	m_sizerTemplateList = new wxBoxSizer( wxVERTICAL );


	m_scrolledTemplates->SetSizer( m_sizerTemplateList );
	m_scrolledTemplates->Layout();
	m_sizerTemplateList->Fit( m_scrolledTemplates );
	bSizerTemplates->Add( m_scrolledTemplates, 1, wxEXPAND|wxALL, 5 );


	m_panelTemplates->SetSizer( bSizerTemplates );
	m_panelTemplates->Layout();
	bSizerTemplates->Fit( m_panelTemplates );
	m_mainContent->Add( m_panelTemplates, 1, wxEXPAND|wxALL, 5 );

	m_panelPreview = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelPreview->Hide();

	wxBoxSizer* bSizerPreview;
	bSizerPreview = new wxBoxSizer( wxVERTICAL );

	m_webviewPlaceholder = new wxPanel( m_panelPreview, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bSizerPreview->Add( m_webviewPlaceholder, 1, wxEXPAND|wxALL, 5 );


	m_panelPreview->SetSizer( bSizerPreview );
	m_panelPreview->Layout();
	bSizerPreview->Fit( m_panelPreview );
	m_mainContent->Add( m_panelPreview, 2, wxEXPAND|wxALL, 5 );


	bmainSizer->Add( m_mainContent, 1, wxEXPAND, 5 );

	m_sizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_btnBack = new wxButton( this, wxID_ANY, _("Go Back"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnBack->Enable( false );

	m_sizerButtons->Add( m_btnBack, 0, wxALL, 5 );


	m_sizerButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_sizerButtons->Add( m_sdbSizer, 0, wxEXPAND, 5 );


	bmainSizer->Add( m_sizerButtons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bmainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_searchCtrl->Connect( wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnSearchCtrlCancel ), NULL, this );
	m_searchCtrl->Connect( wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnSearchCtrl ), NULL, this );
	m_searchCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnSearchCtrl ), NULL, this );
	m_filterChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnFilterChanged ), NULL, this );
	m_btnBack->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnBackClicked ), NULL, this );
}

DIALOG_TEMPLATE_SELECTOR_BASE::~DIALOG_TEMPLATE_SELECTOR_BASE()
{
	// Disconnect Events
	m_searchCtrl->Disconnect( wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnSearchCtrlCancel ), NULL, this );
	m_searchCtrl->Disconnect( wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnSearchCtrl ), NULL, this );
	m_searchCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnSearchCtrl ), NULL, this );
	m_filterChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnFilterChanged ), NULL, this );
	m_btnBack->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnBackClicked ), NULL, this );

}

TEMPLATE_SELECTION_PANEL_BASE::TEMPLATE_SELECTION_PANEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

	m_SizerBase = new wxBoxSizer( wxHORIZONTAL );

	m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxALWAYS_SHOW_SB|wxVSCROLL );
	m_scrolledWindow->SetScrollRate( 0, 25 );
	m_scrolledWindow->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

	m_SizerChoice = new wxBoxSizer( wxVERTICAL );


	m_scrolledWindow->SetSizer( m_SizerChoice );
	m_scrolledWindow->Layout();
	m_SizerChoice->Fit( m_scrolledWindow );
	m_SizerBase->Add( m_scrolledWindow, 0, wxEXPAND, 10 );


	this->SetSizer( m_SizerBase );
	this->Layout();
	m_SizerBase->Fit( this );
}

TEMPLATE_SELECTION_PANEL_BASE::~TEMPLATE_SELECTION_PANEL_BASE()
{
}

TEMPLATE_WIDGET_BASE::TEMPLATE_WIDGET_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );

	m_bitmapIcon = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_bitmapIcon->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	bSizerMain->Add( m_bitmapIcon, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxBoxSizer* bSizerText;
	bSizerText = new wxBoxSizer( wxVERTICAL );

    m_titleLabel = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_titleLabel->Wrap( -1 );
	bSizerText->Add( m_titleLabel, 0, wxBOTTOM, 0 );

    m_descLabel = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_descLabel->Wrap( -1 );
	bSizerText->Add( m_descLabel, 0, wxEXPAND, 0 );


	bSizerMain->Add( bSizerText, 1, wxEXPAND|wxALL, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
}

TEMPLATE_WIDGET_BASE::~TEMPLATE_WIDGET_BASE()
{
}

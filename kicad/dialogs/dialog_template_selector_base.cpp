///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/webview_panel.h"

#include "dialog_template_selector_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TEMPLATE_SELECTOR_BASE::DIALOG_TEMPLATE_SELECTOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bsizerTemplateSelector;
	bsizerTemplateSelector = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextTpath = new wxStaticText( this, wxID_ANY, _("Folder:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTpath->Wrap( -1 );
	bsizerTemplateSelector->Add( m_staticTextTpath, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_tcTemplatePath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	bsizerTemplateSelector->Add( m_tcTemplatePath, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bsizerTemplateSelector->Add( m_browseButton, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_reloadButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bsizerTemplateSelector->Add( m_reloadButton, 0, wxBOTTOM|wxTOP, 5 );


	bmainSizer->Add( bsizerTemplateSelector, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxBoxSizer* bSizerNotebook;
	bSizerNotebook = new wxBoxSizer( wxVERTICAL );

	bSizerNotebook->SetMinSize( wxSize( 700,400 ) );
	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	bSizerNotebook->Add( m_notebook, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_webviewPanel = new WEBVIEW_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_webviewPanel->SetMinSize( wxSize( 700,300 ) );

	bSizerNotebook->Add( m_webviewPanel, 1, wxEXPAND | wxALL, 5 );


	bmainSizer->Add( bSizerNotebook, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bmainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bmainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::onDirectoryBrowseClicked ), NULL, this );
	m_reloadButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::onReload ), NULL, this );
	m_notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnPageChange ), NULL, this );
}

DIALOG_TEMPLATE_SELECTOR_BASE::~DIALOG_TEMPLATE_SELECTOR_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::onDirectoryBrowseClicked ), NULL, this );
	m_reloadButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::onReload ), NULL, this );
	m_notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnPageChange ), NULL, this );

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

	m_bitmapIcon = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 64,64 ), 0 );
	m_bitmapIcon->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_bitmapIcon->SetMinSize( wxSize( 64,64 ) );

	bSizerMain->Add( m_bitmapIcon, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticTitle = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticTitle->Wrap( 100 );
	bSizerMain->Add( m_staticTitle, 1, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxRIGHT, 2 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
}

TEMPLATE_WIDGET_BASE::~TEMPLATE_WIDGET_BASE()
{
}

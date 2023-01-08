///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

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
	bsizerTemplateSelector->Add( m_staticTextTpath, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_tcTemplatePath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	bsizerTemplateSelector->Add( m_tcTemplatePath, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bsizerTemplateSelector->Add( m_browseButton, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_reloadButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bsizerTemplateSelector->Add( m_reloadButton, 0, wxALL, 5 );


	bmainSizer->Add( bsizerTemplateSelector, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_notebook->SetMinSize( wxSize( -1,185 ) );


	bmainSizer->Add( m_notebook, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_htmlWin = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxHW_SCROLLBAR_AUTO );
	m_htmlWin->SetMinSize( wxSize( 750,380 ) );

	bmainSizer->Add( m_htmlWin, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 12 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bmainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bmainSizer );
	this->Layout();
	bmainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::onDirectoryBrowseClicked ), NULL, this );
	m_reloadButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::onReload ), NULL, this );
	m_notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnPageChange ), NULL, this );
	m_htmlWin->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnHtmlLinkActivated ), NULL, this );
}

DIALOG_TEMPLATE_SELECTOR_BASE::~DIALOG_TEMPLATE_SELECTOR_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::onDirectoryBrowseClicked ), NULL, this );
	m_reloadButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::onReload ), NULL, this );
	m_notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnPageChange ), NULL, this );
	m_htmlWin->Disconnect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_TEMPLATE_SELECTOR_BASE::OnHtmlLinkActivated ), NULL, this );

}

TEMPLATE_SELECTION_PANEL_BASE::TEMPLATE_SELECTION_PANEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	m_SizerBase = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxALWAYS_SHOW_SB|wxHSCROLL );
	m_scrolledWindow->SetScrollRate( 5, 5 );
	m_SizerChoice = new wxGridSizer( 1, 0, 0, 0 );


	m_scrolledWindow->SetSizer( m_SizerChoice );
	m_scrolledWindow->Layout();
	m_SizerChoice->Fit( m_scrolledWindow );
	bSizerMargins->Add( m_scrolledWindow, 1, wxEXPAND|wxLEFT, 5 );


	m_SizerBase->Add( bSizerMargins, 1, wxEXPAND|wxBOTTOM|wxLEFT, 5 );


	this->SetSizer( m_SizerBase );
	this->Layout();
}

TEMPLATE_SELECTION_PANEL_BASE::~TEMPLATE_SELECTION_PANEL_BASE()
{
}

TEMPLATE_WIDGET_BASE::TEMPLATE_WIDGET_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetMinSize( wxSize( 108,-1 ) );
	this->SetMaxSize( wxSize( 108,-1 ) );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_bitmapIcon = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 64,64 ), 0 );
	m_bitmapIcon->SetMinSize( wxSize( 64,64 ) );

	bSizerMain->Add( m_bitmapIcon, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_staticTitle = new wxStaticText( this, wxID_ANY, _("Project Template Title"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticTitle->Wrap( 100 );
	bSizerMain->Add( m_staticTitle, 1, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 2 );


	this->SetSizer( bSizerMain );
	this->Layout();
}

TEMPLATE_WIDGET_BASE::~TEMPLATE_WIDGET_BASE()
{
}

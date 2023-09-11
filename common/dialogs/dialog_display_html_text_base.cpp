///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_display_html_text_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DISPLAY_HTML_TEXT_BASE::DIALOG_DISPLAY_HTML_TEXT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 500,300 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	bMainSizer->SetMinSize( wxSize( 540,240 ) );
	m_htmlWindow = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	bMainSizer->Add( m_htmlWindow, 1, wxALL|wxEXPAND, 10 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1->Realize();

	bMainSizer->Add( m_sdbSizer1, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_htmlWindow->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_DISPLAY_HTML_TEXT_BASE::OnHTMLLinkClicked ), NULL, this );
}

DIALOG_DISPLAY_HTML_TEXT_BASE::~DIALOG_DISPLAY_HTML_TEXT_BASE()
{
	// Disconnect Events
	m_htmlWindow->Disconnect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_DISPLAY_HTML_TEXT_BASE::OnHTMLLinkClicked ), NULL, this );

}

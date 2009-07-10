///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_display_info_HTML_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DISPLAY_HTML_TEXT_BASE::DIALOG_DISPLAY_HTML_TEXT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 400,120 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_htmlWindow = new wxHtmlWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO|wxSUNKEN_BORDER );
	bMainSizer->Add( m_htmlWindow, 1, wxEXPAND, 5 );
	
	m_buttonClose = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	m_buttonClose->SetDefault(); 
	bMainSizer->Add( m_buttonClose, 0, wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_htmlWindow->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_DISPLAY_HTML_TEXT_BASE::OnHTMLLinkClicked ), NULL, this );
	m_buttonClose->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_HTML_TEXT_BASE::OnCloseButtonClick ), NULL, this );
}

DIALOG_DISPLAY_HTML_TEXT_BASE::~DIALOG_DISPLAY_HTML_TEXT_BASE()
{
	// Disconnect Events
	m_htmlWindow->Disconnect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_DISPLAY_HTML_TEXT_BASE::OnHTMLLinkClicked ), NULL, this );
	m_buttonClose->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_HTML_TEXT_BASE::OnCloseButtonClick ), NULL, this );
}

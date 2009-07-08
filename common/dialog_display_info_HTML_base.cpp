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
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_htmlWindow = new wxHtmlWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_htmlWindow->SetMinSize( wxSize( 400,150 ) );
	
	bMainSizer->Add( m_htmlWindow, 1, wxALL|wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
}

DIALOG_DISPLAY_HTML_TEXT_BASE::~DIALOG_DISPLAY_HTML_TEXT_BASE()
{
}

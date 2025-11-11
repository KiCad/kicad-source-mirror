///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_defaults_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_DEFAULTS_BASE::PANEL_SETUP_DEFAULTS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scrolledWindow->SetScrollRate( 5, 5 );
	m_scrollSizer = new wxBoxSizer( wxVERTICAL );


	m_scrolledWindow->SetSizer( m_scrollSizer );
	m_scrolledWindow->Layout();
	m_scrollSizer->Fit( m_scrolledWindow );
	mainSizer->Add( m_scrolledWindow, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

PANEL_SETUP_DEFAULTS_BASE::~PANEL_SETUP_DEFAULTS_BASE()
{
}

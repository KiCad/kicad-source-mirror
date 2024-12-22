///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_kicad_launcher_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_KICAD_LAUNCHER_BASE::PANEL_KICAD_LAUNCHER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : PANEL_NOTEBOOK_BASE( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_scrolledWindow->SetScrollRate( 0, 5 );
	wxBoxSizer* bSizerInner;
	bSizerInner = new wxBoxSizer( wxVERTICAL );

	m_toolsSizer = new wxFlexGridSizer( 0, 2, 2, 10 );
	m_toolsSizer->SetFlexibleDirection( wxBOTH );
	m_toolsSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizerInner->Add( m_toolsSizer, 1, wxALL|wxEXPAND, 5 );


	m_scrolledWindow->SetSizer( bSizerInner );
	m_scrolledWindow->Layout();
	bSizerInner->Fit( m_scrolledWindow );
	bSizer2->Add( m_scrolledWindow, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer2 );
	this->Layout();
	bSizer2->Fit( this );
}

PANEL_KICAD_LAUNCHER_BASE::~PANEL_KICAD_LAUNCHER_BASE()
{
}

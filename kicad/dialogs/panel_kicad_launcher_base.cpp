///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.1.0-0-g733bf3d)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_kicad_launcher_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_KICAD_LAUNCHER_BASE::PANEL_KICAD_LAUNCHER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scrolledWindow->SetScrollRate( 5, 5 );
	m_toolsSizer = new wxFlexGridSizer( 0, 2, 2, 10 );
	m_toolsSizer->SetFlexibleDirection( wxBOTH );
	m_toolsSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	m_scrolledWindow->SetSizer( m_toolsSizer );
	m_scrolledWindow->Layout();
	m_toolsSizer->Fit( m_scrolledWindow );
	bSizer2->Add( m_scrolledWindow, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( bSizer2 );
	this->Layout();
	bSizer2->Fit( this );
}

PANEL_KICAD_LAUNCHER_BASE::~PANEL_KICAD_LAUNCHER_BASE()
{
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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

	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_toolsSizer = new wxGridBagSizer( 0, 12 );
	m_toolsSizer->SetFlexibleDirection( wxHORIZONTAL );
	m_toolsSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );


	m_mainSizer->Add( m_toolsSizer, 0, wxALL|wxEXPAND, 5 );


	m_mainSizer->Add( 0, 20, 0, wxEXPAND, 5 );


	bSizer2->Add( m_mainSizer, 1, wxEXPAND|wxTOP|wxLEFT, 10 );


	this->SetSizer( bSizer2 );
	this->Layout();
	bSizer2->Fit( this );
}

PANEL_KICAD_LAUNCHER_BASE::~PANEL_KICAD_LAUNCHER_BASE()
{
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_zones_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_ZONES_BASE::PANEL_SETUP_ZONES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_staticTextDefPropDim = new wxStaticText( this, wxID_ANY, _("Default Properties for New Zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefPropDim->Wrap( -1 );
	m_mainSizer->Add( m_staticTextDefPropDim, 0, wxTOP|wxRIGHT|wxLEFT, 13 );


	m_mainSizer->Add( 0, 2, 0, 0, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_mainSizer->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );
}

PANEL_SETUP_ZONES_BASE::~PANEL_SETUP_ZONES_BASE()
{
}

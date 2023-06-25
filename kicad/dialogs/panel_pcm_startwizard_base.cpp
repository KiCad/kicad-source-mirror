///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_pcm_startwizard_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PCM_STARTWIZARD_BASE::PANEL_PCM_STARTWIZARD_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_stIntro = new wxStaticText( this, wxID_ANY, _("KiCad includes support for third party content via the Plugin and Content Manager (PCM)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_stIntro->Wrap( 400 );
	bSizer8->Add( m_stIntro, 0, wxALL, 5 );

	m_cbAutoUpdate = new wxCheckBox( this, wxID_ANY, _("Automatically check for content updates on launch of KiCad"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_cbAutoUpdate, 0, wxALL, 5 );


	bPanelSizer->Add( bSizer8, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_PCM_STARTWIZARD_BASE::~PANEL_PCM_STARTWIZARD_BASE()
{
}

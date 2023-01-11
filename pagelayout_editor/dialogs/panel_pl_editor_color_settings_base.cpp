///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_pl_editor_color_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PL_EDITOR_COLOR_SETTINGS_BASE::PANEL_PL_EDITOR_COLOR_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* p1mainSizer;
	p1mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_txtTheme = new wxStaticText( this, wxID_ANY, _("Color theme:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtTheme->Wrap( -1 );
	bSizer2->Add( m_txtTheme, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_themesChoices;
	m_themes = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_themesChoices, 0 );
	m_themes->SetSelection( 0 );
	bSizer2->Add( m_themes, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	p1mainSizer->Add( bSizer2, 0, wxTOP|wxLEFT|wxEXPAND, 5 );


	this->SetSizer( p1mainSizer );
	this->Layout();
	p1mainSizer->Fit( this );
}

PANEL_PL_EDITOR_COLOR_SETTINGS_BASE::~PANEL_PL_EDITOR_COLOR_SETTINGS_BASE()
{
}

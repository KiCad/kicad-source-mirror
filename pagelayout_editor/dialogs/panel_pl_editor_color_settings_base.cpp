///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_pl_editor_color_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PL_EDITOR_COLOR_SETTINGS_BASE::PANEL_PL_EDITOR_COLOR_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* p1mainSizer;
	p1mainSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Color Theme") ), wxHORIZONTAL );

	sbSizer1->SetMinSize( wxSize( 250,-1 ) );
	m_txtTheme = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Theme:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtTheme->Wrap( -1 );
	sbSizer1->Add( m_txtTheme, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_themesChoices;
	m_themes = new wxChoice( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_themesChoices, 0 );
	m_themes->SetSelection( 0 );
	sbSizer1->Add( m_themes, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	p1mainSizer->Add( sbSizer1, 0, wxALL, 10 );


	p1mainSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	this->SetSizer( p1mainSizer );
	this->Layout();
	p1mainSizer->Fit( this );
}

PANEL_PL_EDITOR_COLOR_SETTINGS_BASE::~PANEL_PL_EDITOR_COLOR_SETTINGS_BASE()
{
}

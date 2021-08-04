///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_sym_color_settings_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( PANEL_SYM_COLOR_SETTINGS_BASE, wxPanel )
	EVT_CHOICE( wxID_ANY, PANEL_SYM_COLOR_SETTINGS_BASE::_wxFB_OnThemeChanged )
END_EVENT_TABLE()

PANEL_SYM_COLOR_SETTINGS_BASE::PANEL_SYM_COLOR_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* p1mainSizer;
	p1mainSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Color Theme") ), wxVERTICAL );

	sbSizer1->SetMinSize( wxSize( 250,-1 ) );
	m_eeschemaRB = new wxRadioButton( sbSizer1->GetStaticBox(), wxID_ANY, _("Use schematic editor color theme"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	sbSizer1->Add( m_eeschemaRB, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_themeRB = new wxRadioButton( sbSizer1->GetStaticBox(), wxID_ANY, _("Use theme:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_themeRB, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_themesChoices;
	m_themes = new wxChoice( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_themesChoices, 0 );
	m_themes->SetSelection( 0 );
	bSizer2->Add( m_themes, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizer1->Add( bSizer2, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	p1mainSizer->Add( sbSizer1, 0, wxALL, 5 );


	p1mainSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	this->SetSizer( p1mainSizer );
	this->Layout();
	p1mainSizer->Fit( this );
}

PANEL_SYM_COLOR_SETTINGS_BASE::~PANEL_SYM_COLOR_SETTINGS_BASE()
{
}

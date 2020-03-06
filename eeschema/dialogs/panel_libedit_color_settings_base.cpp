///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_libedit_color_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_LIBEDIT_COLOR_SETTINGS_BASE::PANEL_LIBEDIT_COLOR_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* p1mainSizer;
	p1mainSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Color Theme") ), wxVERTICAL );

	sbSizer1->SetMinSize( wxSize( 250,-1 ) );
	m_useEeschemaTheme = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Use Eeschema color theme"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_useEeschemaTheme, 0, wxALL|wxEXPAND, 10 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText16 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Theme:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	bSizer3->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_themeSelectionChoices;
	m_themeSelection = new wxChoice( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_themeSelectionChoices, 0 );
	m_themeSelection->SetSelection( 0 );
	bSizer3->Add( m_themeSelection, 1, wxALL, 5 );


	sbSizer1->Add( bSizer3, 0, wxALL|wxEXPAND, 5 );


	sbSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	p1mainSizer->Add( sbSizer1, 0, 0, 5 );


	p1mainSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	this->SetSizer( p1mainSizer );
	this->Layout();
	p1mainSizer->Fit( this );
}

PANEL_LIBEDIT_COLOR_SETTINGS_BASE::~PANEL_LIBEDIT_COLOR_SETTINGS_BASE()
{
}

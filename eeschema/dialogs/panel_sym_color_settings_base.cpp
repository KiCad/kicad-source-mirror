///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_sym_color_settings_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( PANEL_SYM_COLOR_SETTINGS_BASE, wxPanel )
	EVT_CHECKBOX( wxID_ANY, PANEL_SYM_COLOR_SETTINGS_BASE::_wxFB_OnUseEeschemaThemeChanged )
END_EVENT_TABLE()

PANEL_SYM_COLOR_SETTINGS_BASE::PANEL_SYM_COLOR_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* p1mainSizer;
	p1mainSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Color Theme") ), wxHORIZONTAL );

	sbSizer1->SetMinSize( wxSize( 250,-1 ) );
	m_useEeschemaTheme = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Use Eeschema color theme"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useEeschemaTheme->SetValue(true);
	sbSizer1->Add( m_useEeschemaTheme, 0, wxALL|wxEXPAND, 10 );

	m_txtTheme = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Theme:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtTheme->Wrap( -1 );
	m_txtTheme->Enable( false );

	sbSizer1->Add( m_txtTheme, 0, wxALIGN_CENTER_VERTICAL|wxALL, 10 );

	wxArrayString m_themeSelectionChoices;
	m_themeSelection = new wxChoice( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_themeSelectionChoices, 0 );
	m_themeSelection->SetSelection( 0 );
	m_themeSelection->Enable( false );

	sbSizer1->Add( m_themeSelection, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 10 );


	p1mainSizer->Add( sbSizer1, 0, wxALL, 5 );


	p1mainSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	this->SetSizer( p1mainSizer );
	this->Layout();
	p1mainSizer->Fit( this );
}

PANEL_SYM_COLOR_SETTINGS_BASE::~PANEL_SYM_COLOR_SETTINGS_BASE()
{
}

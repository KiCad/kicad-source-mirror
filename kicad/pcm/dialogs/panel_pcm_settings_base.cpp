///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_pcm_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PCM_SETTINGS_BASE::PANEL_PCM_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_updateCheck = new wxCheckBox( this, wxID_ANY, _("Check for package updates on startup"), wxDefaultPosition, wxDefaultSize, 0 );
	m_updateCheck->SetValue(true);
	bSizer1->Add( m_updateCheck, 0, wxALL, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
}

PANEL_PCM_SETTINGS_BASE::~PANEL_PCM_SETTINGS_BASE()
{
}

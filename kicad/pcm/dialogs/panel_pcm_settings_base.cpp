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

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("General") ), wxVERTICAL );

	m_updateCheck = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Check for package updates on startup"), wxDefaultPosition, wxDefaultSize, 0 );
	m_updateCheck->SetValue(true);
	sbSizer1->Add( m_updateCheck, 0, wxALL, 5 );


	bSizer1->Add( sbSizer1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Library package handling") ), wxVERTICAL );

	m_libAutoAdd = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Automatically add installed libraries to global lib table"), wxDefaultPosition, wxDefaultSize, 0 );
	m_libAutoAdd->SetValue(true);
	sbSizer2->Add( m_libAutoAdd, 0, wxALL, 5 );

	m_libAutoRemove = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Automatically remove uninstalled libraries"), wxDefaultPosition, wxDefaultSize, 0 );
	m_libAutoRemove->SetValue(true);
	sbSizer2->Add( m_libAutoRemove, 0, wxALL, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText1 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Library nickname prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer2->Add( m_staticText1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_libPrefix = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_libPrefix, 0, wxALL, 5 );


	sbSizer2->Add( bSizer2, 1, wxEXPAND, 5 );

	m_libHelp = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("After packages are (un)installed KiCad may need to be restarted to reflect changes in the global library table."), wxDefaultPosition, wxDefaultSize, 0 );
	m_libHelp->Wrap( -1 );
	sbSizer2->Add( m_libHelp, 0, wxALL, 5 );


	bSizer1->Add( sbSizer2, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
}

PANEL_PCM_SETTINGS_BASE::~PANEL_PCM_SETTINGS_BASE()
{
}

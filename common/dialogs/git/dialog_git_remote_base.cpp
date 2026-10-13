///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_git_remote_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GIT_REMOTE_BASE::DIALOG_GIT_REMOTE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 460,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgFields;
	fgFields = new wxFlexGridSizer( 0, 2, 6, 6 );
	fgFields->AddGrowableCol( 1 );
	fgFields->SetFlexibleDirection( wxBOTH );
	fgFields->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_staticText17;
	m_staticText17 = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	fgFields->Add( m_staticText17, 0, wxALIGN_CENTER_VERTICAL, 0 );

	m_nameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFields->Add( m_nameCtrl, 1, wxEXPAND, 0 );

	wxStaticText* m_staticText18;
	m_staticText18 = new wxStaticText( this, wxID_ANY, _("URL:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgFields->Add( m_staticText18, 0, wxALIGN_CENTER_VERTICAL, 0 );

	m_urlCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_urlCtrl->SetMinSize( wxSize( 360,-1 ) );

	fgFields->Add( m_urlCtrl, 1, wxEXPAND, 0 );


	bMainSizer->Add( fgFields, 1, wxEXPAND|wxALL, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 10 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_GIT_REMOTE_BASE::~DIALOG_GIT_REMOTE_BASE()
{
}

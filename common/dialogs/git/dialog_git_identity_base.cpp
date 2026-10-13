///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_git_identity_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GIT_IDENTITY_BASE::DIALOG_GIT_IDENTITY_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 400,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_message = new wxStaticText( this, wxID_ANY, _("Commits record who made them. Enter the name and email to use."), wxDefaultPosition, wxDefaultSize, 0 );
	m_message->Wrap( -1 );
	bMainSizer->Add( m_message, 0, wxALL, 10 );

	wxFlexGridSizer* fgFields;
	fgFields = new wxFlexGridSizer( 0, 2, 6, 6 );
	fgFields->AddGrowableCol( 1 );
	fgFields->SetFlexibleDirection( wxBOTH );
	fgFields->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_staticText13;
	m_staticText13 = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	fgFields->Add( m_staticText13, 0, wxALIGN_CENTER_VERTICAL, 0 );

	m_nameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_nameCtrl->SetMinSize( wxSize( 320,-1 ) );

	fgFields->Add( m_nameCtrl, 1, wxEXPAND, 0 );

	wxStaticText* m_staticText14;
	m_staticText14 = new wxStaticText( this, wxID_ANY, _("Email:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	fgFields->Add( m_staticText14, 0, wxALIGN_CENTER_VERTICAL, 0 );

	m_emailCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_emailCtrl->SetMinSize( wxSize( 320,-1 ) );

	fgFields->Add( m_emailCtrl, 1, wxEXPAND, 0 );


	bMainSizer->Add( fgFields, 1, wxEXPAND|wxLEFT|wxRIGHT, 10 );

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

DIALOG_GIT_IDENTITY_BASE::~DIALOG_GIT_IDENTITY_BASE()
{
}

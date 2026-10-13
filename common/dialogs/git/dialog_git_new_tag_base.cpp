///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_git_new_tag_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GIT_NEW_TAG_BASE::DIALOG_GIT_NEW_TAG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 420,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bNameRow;
	bNameRow = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_staticText15;
	m_staticText15 = new wxStaticText( this, wxID_ANY, _("Tag name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	bNameRow->Add( m_staticText15, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 6 );

	m_nameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bNameRow->Add( m_nameCtrl, 1, wxALIGN_CENTER_VERTICAL, 0 );


	bMainSizer->Add( bNameRow, 0, wxEXPAND|wxALL, 10 );

	wxStaticText* m_staticText16;
	m_staticText16 = new wxStaticText( this, wxID_ANY, _("Message:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	bMainSizer->Add( m_staticText16, 0, wxLEFT|wxRIGHT, 10 );

	m_messageCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_messageCtrl->SetMinSize( wxSize( -1,120 ) );

	bMainSizer->Add( m_messageCtrl, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	m_hint = new wxStaticText( this, wxID_ANY, _("A message makes an annotated tag. Leave it empty for a plain tag."), wxDefaultPosition, wxDefaultSize, 0 );
	m_hint->Wrap( -1 );
	bMainSizer->Add( m_hint, 0, wxLEFT|wxRIGHT|wxTOP, 10 );

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

DIALOG_GIT_NEW_TAG_BASE::~DIALOG_GIT_NEW_TAG_BASE()
{
}

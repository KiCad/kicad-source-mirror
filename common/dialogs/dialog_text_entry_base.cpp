///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_text_entry_base.h"

///////////////////////////////////////////////////////////////////////////

WX_TEXT_ENTRY_DIALOG_BASE::WX_TEXT_ENTRY_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_ContentSizer = new wxBoxSizer( wxVERTICAL );

	m_label = new wxStaticText( this, wxID_ANY, _("MyLabel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_label->Wrap( -1 );
	m_ContentSizer->Add( m_label, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_textCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrl->SetMinSize( wxSize( 300,-1 ) );

	m_ContentSizer->Add( m_textCtrl, 0, wxEXPAND|wxALL, 5 );


	m_mainSizer->Add( m_ContentSizer, 1, wxALL|wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_mainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	this->Centre( wxBOTH );
}

WX_TEXT_ENTRY_DIALOG_BASE::~WX_TEXT_ENTRY_DIALOG_BASE()
{
}

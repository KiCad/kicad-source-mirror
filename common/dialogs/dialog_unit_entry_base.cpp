///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_unit_entry_base.h"

///////////////////////////////////////////////////////////////////////////

WX_UNIT_ENTRY_DIALOG_BASE::WX_UNIT_ENTRY_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerContent;
	bSizerContent = new wxBoxSizer( wxVERTICAL );

	m_label = new wxStaticText( this, wxID_ANY, _("MyLabel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_label->Wrap( -1 );
	bSizerContent->Add( m_label, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerTextAndUnit;
	bSizerTextAndUnit = new wxBoxSizer( wxHORIZONTAL );

	m_textCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrl->SetMinSize( wxSize( 300,-1 ) );

	bSizerTextAndUnit->Add( m_textCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_unit_label = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unit_label->Wrap( -1 );
	bSizerTextAndUnit->Add( m_unit_label, 0, wxALL, 5 );


	bSizerContent->Add( bSizerTextAndUnit, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerContent, 1, wxALL|wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxALL|wxALIGN_RIGHT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );
}

WX_UNIT_ENTRY_DIALOG_BASE::~WX_UNIT_ENTRY_DIALOG_BASE()
{
}

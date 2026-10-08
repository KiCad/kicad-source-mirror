///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_downgrade_report_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DOWNGRADE_REPORT_BASE::DIALOG_DOWNGRADE_REPORT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_headline = new wxStaticText( this, wxID_ANY, _("Exporting will change some features:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_headline->Wrap( -1 );
	bSizerMain->Add( m_headline, 0, wxALL, 10 );

	m_reportList = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES );
	m_reportList->SetMinSize( wxSize( 500,250 ) );

	bSizerMain->Add( m_reportList, 1, wxALL|wxEXPAND, 10 );

	m_note = new wxStaticText( this, wxID_ANY, _("Your current project is not modified."), wxDefaultPosition, wxDefaultSize, 0 );
	m_note->Wrap( -1 );
	bSizerMain->Add( m_note, 0, wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	this->Centre( wxBOTH );
}

DIALOG_DOWNGRADE_REPORT_BASE::~DIALOG_DOWNGRADE_REPORT_BASE()
{
}

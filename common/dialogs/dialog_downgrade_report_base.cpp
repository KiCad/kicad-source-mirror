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

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_buttonSaveReport = new wxButton( this, wxID_ANY, _("Save Report..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSaveReport->SetToolTip( _("Save the compatibility report as a text file.") );

	bSizerButtons->Add( m_buttonSaveReport, 0, wxALIGN_CENTER_VERTICAL, 0 );


	bSizerButtons->Add( 0, 0, 1, wxEXPAND, 0 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerButtons->Add( m_sdbSizer, 0, wxALIGN_CENTER_VERTICAL, 0 );


	bSizerMain->Add( bSizerButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_buttonSaveReport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DOWNGRADE_REPORT_BASE::OnSaveReport ), NULL, this );
}

DIALOG_DOWNGRADE_REPORT_BASE::~DIALOG_DOWNGRADE_REPORT_BASE()
{
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_book_reporter_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BOOK_REPORTER_BASE::DIALOG_BOOK_REPORTER_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_notebook->SetMinSize( wxSize( 550,480 ) );


	bMainSizer->Add( m_notebook, 1, wxEXPAND|wxALL, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_BOOK_REPORTER_BASE::OnClose ) );
}

DIALOG_BOOK_REPORTER_BASE::~DIALOG_BOOK_REPORTER_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_BOOK_REPORTER_BASE::OnClose ) );

}

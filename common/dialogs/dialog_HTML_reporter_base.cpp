///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_box.h"

#include "dialog_HTML_reporter_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_HTML_REPORTER::DIALOG_HTML_REPORTER( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_Reporter = new WX_HTML_REPORT_BOX( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO|wxBORDER_SIMPLE );
	m_Reporter->SetMinSize( wxSize( 640,360 ) );

	bMainSizer->Add( m_Reporter, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

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
	m_Reporter->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_HTML_REPORTER::OnErrorLinkClicked ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_HTML_REPORTER::OnOK ), NULL, this );
}

DIALOG_HTML_REPORTER::~DIALOG_HTML_REPORTER()
{
	// Disconnect Events
	m_Reporter->Disconnect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_HTML_REPORTER::OnErrorLinkClicked ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_HTML_REPORTER::OnOK ), NULL, this );

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_html_report_panel.h"

#include "dialog_symbol_remap_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SYMBOL_REMAP_BASE::DIALOG_SYMBOL_REMAP_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_htmlCtrl = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	bSizer2->Add( m_htmlCtrl, 4, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_buttonRemap = new wxButton( this, wxID_ANY, _("Remap Symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_buttonRemap, 0, wxALL|wxEXPAND, 5 );

	m_buttonClose = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_buttonClose, 0, wxALL|wxEXPAND, 5 );


	bSizer2->Add( bSizer3, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer2, 3, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_messagePanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_messagePanel->SetMinSize( wxSize( -1,200 ) );

	bSizer4->Add( m_messagePanel, 1, wxEXPAND | wxALL, 5 );


	bSizer1->Add( bSizer4, 5, wxEXPAND, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_buttonRemap->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_REMAP_BASE::OnRemapSymbols ), NULL, this );
	m_buttonRemap->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SYMBOL_REMAP_BASE::OnUpdateUIRemapButton ), NULL, this );
}

DIALOG_SYMBOL_REMAP_BASE::~DIALOG_SYMBOL_REMAP_BASE()
{
	// Disconnect Events
	m_buttonRemap->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_REMAP_BASE::OnRemapSymbols ), NULL, this );
	m_buttonRemap->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SYMBOL_REMAP_BASE::OnUpdateUIRemapButton ), NULL, this );

}

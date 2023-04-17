///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-282-g1fa54006)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_printer_list_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PRINTER_LIST_BASE::PANEL_PRINTER_LIST_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_sbSizerPrinters = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Printers") ), wxVERTICAL );

	wxArrayString m_choicePrinterChoices;
	m_choicePrinter = new wxChoice( m_sbSizerPrinters->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePrinterChoices, 0 );
	m_choicePrinter->SetSelection( 0 );
	m_sbSizerPrinters->Add( m_choicePrinter, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	wxBoxSizer* bSizerPrinterState;
	bSizerPrinterState = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextPrn = new wxStaticText( m_sbSizerPrinters->GetStaticBox(), wxID_ANY, _("Info:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrn->Wrap( -1 );
	bSizerPrinterState->Add( m_staticTextPrn, 0, wxALL, 5 );


	bSizerPrinterState->Add( 0, 0, 0, wxRIGHT|wxLEFT, 5 );

	m_stPrinterState = new wxStaticText( m_sbSizerPrinters->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stPrinterState->Wrap( -1 );
	bSizerPrinterState->Add( m_stPrinterState, 0, wxALL, 5 );


	m_sbSizerPrinters->Add( bSizerPrinterState, 0, wxEXPAND, 5 );


	bSizerMain->Add( m_sbSizerPrinters, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	// Connect Events
	m_choicePrinter->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PRINTER_LIST_BASE::onPrinterChoice ), NULL, this );
}

PANEL_PRINTER_LIST_BASE::~PANEL_PRINTER_LIST_BASE()
{
	// Disconnect Events
	m_choicePrinter->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PRINTER_LIST_BASE::onPrinterChoice ), NULL, this );

}

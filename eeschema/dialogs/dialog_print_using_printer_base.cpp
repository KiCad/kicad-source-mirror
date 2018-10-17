///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_print_using_printer_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PRINT_USING_PRINTER_BASE::DIALOG_PRINT_USING_PRINTER_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bleftSizer;
	bleftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_checkReference = new wxCheckBox( this, wxID_ANY, _("Print sheet &reference and title block"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkReference->SetValue(true); 
	m_checkReference->SetToolTip( _("Print (or not) the Frame references.") );
	
	bleftSizer->Add( m_checkReference, 0, wxALL, 5 );
	
	m_checkMonochrome = new wxCheckBox( this, wxID_ANY, _("Print in &black and white only"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkMonochrome->SetValue(true); 
	bleftSizer->Add( m_checkMonochrome, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bMainSizer->Add( bleftSizer, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 10 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bbuttonsSizer;
	bbuttonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonPageSetup = new wxButton( this, wxID_ANY, _("Page Setup..."), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPageSetup, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bbuttonsSizer->Add( 40, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Apply = new wxButton( this, wxID_APPLY );
	m_sdbSizer1->AddButton( m_sdbSizer1Apply );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bbuttonsSizer->Add( m_sdbSizer1, 0, wxALL, 5 );
	
	
	bMainSizer->Add( bbuttonsSizer, 0, wxEXPAND|wxLEFT, 10 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnCloseWindow ) );
	m_buttonPageSetup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPageSetup ), NULL, this );
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPrintPreview ), NULL, this );
}

DIALOG_PRINT_USING_PRINTER_BASE::~DIALOG_PRINT_USING_PRINTER_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnCloseWindow ) );
	m_buttonPageSetup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPageSetup ), NULL, this );
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPrintPreview ), NULL, this );
	
}

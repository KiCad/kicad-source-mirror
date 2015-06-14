///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_print_using_printer_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PRINT_USING_PRINTER_BASE::DIALOG_PRINT_USING_PRINTER_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bleftSizer;
	bleftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Print options:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	m_staticText1->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bleftSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT, 5 );
	
	m_checkReference = new wxCheckBox( this, wxID_ANY, _("Print sheet &reference and title block"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkReference->SetValue(true); 
	m_checkReference->SetToolTip( _("Print (or not) the Frame references.") );
	
	bleftSizer->Add( m_checkReference, 0, wxALL, 10 );
	
	m_checkMonochrome = new wxCheckBox( this, wxID_ANY, _("Print in &black and white only"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkMonochrome->SetValue(true); 
	bleftSizer->Add( m_checkMonochrome, 0, wxBOTTOM|wxRIGHT|wxLEFT, 10 );
	
	
	bMainSizer->Add( bleftSizer, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 12 );
	
	wxBoxSizer* bbuttonsSizer;
	bbuttonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonPageSetup = new wxButton( this, wxID_ANY, _("Page Setup"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPageSetup, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxEXPAND, 5 );
	
	m_buttonPreview = new wxButton( this, wxID_ANY, _("Preview"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPreview, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonPrint = new wxButton( this, wxID_ANY, _("Print"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPrint, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonQuit = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonQuit, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bbuttonsSizer, 0, wxALL, 12 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnCloseWindow ) );
	m_buttonPageSetup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPageSetup ), NULL, this );
	m_buttonPreview->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPrintPreview ), NULL, this );
	m_buttonPrint->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPrintButtonClick ), NULL, this );
	m_buttonQuit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnButtonCancelClick ), NULL, this );
}

DIALOG_PRINT_USING_PRINTER_BASE::~DIALOG_PRINT_USING_PRINTER_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnCloseWindow ) );
	m_buttonPageSetup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPageSetup ), NULL, this );
	m_buttonPreview->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPrintPreview ), NULL, this );
	m_buttonPrint->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPrintButtonClick ), NULL, this );
	m_buttonQuit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnButtonCancelClick ), NULL, this );
	
}

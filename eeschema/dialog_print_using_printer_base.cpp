///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_print_using_printer_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PRINT_USING_PRINTER_base::DIALOG_PRINT_USING_PRINTER_base( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bleftSizer;
	bleftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbOptionsSizer;
	sbOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options:") ), wxVERTICAL );
	
	m_TextPenWidth = new wxStaticText( this, wxID_ANY, _("Default Pen Size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPenWidth->Wrap( -1 );
	sbOptionsSizer->Add( m_TextPenWidth, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DialogPenWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_DialogPenWidth->SetToolTip( _("Selection of the default pen thickness used to draw items, when their thickness is set to 0.") );
	m_DialogPenWidth->SetMinSize( wxSize( 200,-1 ) );
	
	sbOptionsSizer->Add( m_DialogPenWidth, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_Print_Sheet_Ref = new wxCheckBox( this, wxID_FRAME_SEL, _("Print frame ref"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Print_Sheet_Ref->SetValue(true);
	
	m_Print_Sheet_Ref->SetToolTip( _("Print (or not) the Frame references.") );
	
	sbOptionsSizer->Add( m_Print_Sheet_Ref, 0, wxALL, 5 );
	
	bleftSizer->Add( sbOptionsSizer, 1, wxEXPAND|wxALL, 5 );
	
	wxString m_ModeColorOptionChoices[] = { _("Color"), _("Black and white") };
	int m_ModeColorOptionNChoices = sizeof( m_ModeColorOptionChoices ) / sizeof( wxString );
	m_ModeColorOption = new wxRadioBox( this, wxID_PRINT_MODE, _("Print Mode"), wxDefaultPosition, wxDefaultSize, m_ModeColorOptionNChoices, m_ModeColorOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ModeColorOption->SetSelection( 1 );
	m_ModeColorOption->SetToolTip( _("Choose if you wand to draw the sheet like it appears on screen,\nor in black and white mode, better to print it when using  black and white printers") );
	
	bleftSizer->Add( m_ModeColorOption, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_PagesOptionChoices[] = { _("Current"), _("All") };
	int m_PagesOptionNChoices = sizeof( m_PagesOptionChoices ) / sizeof( wxString );
	m_PagesOption = new wxRadioBox( this, wxID_PAGE_MODE, _("Page Print"), wxDefaultPosition, wxDefaultSize, m_PagesOptionNChoices, m_PagesOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_PagesOption->SetSelection( 0 );
	bleftSizer->Add( m_PagesOption, 0, wxALL|wxEXPAND, 5 );
	
	bMainSizer->Add( bleftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bbuttonsSizer;
	bbuttonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonOption = new wxButton( this, wxID_PRINT_OPTIONS, _("Page Options"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonOption, 0, wxALL, 5 );
	
	m_buttonPreview = new wxButton( this, wxID_PREVIEW, _("Preview"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPreview, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonPrint = new wxButton( this, wxID_PRINT_ALL, _("Print"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPrint, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonQuit = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonQuit, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bMainSizer->Add( bbuttonsSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnCloseWindow ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnInitDialog ) );
	m_buttonOption->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPrintSetup ), NULL, this );
	m_buttonPreview->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPrintPreview ), NULL, this );
	m_buttonPrint->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPrintButtonClick ), NULL, this );
	m_buttonQuit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnButtonCancelClick ), NULL, this );
}

DIALOG_PRINT_USING_PRINTER_base::~DIALOG_PRINT_USING_PRINTER_base()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnCloseWindow ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnInitDialog ) );
	m_buttonOption->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPrintSetup ), NULL, this );
	m_buttonPreview->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPrintPreview ), NULL, this );
	m_buttonPrint->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPrintButtonClick ), NULL, this );
	m_buttonQuit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnButtonCancelClick ), NULL, this );
}

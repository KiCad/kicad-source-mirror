///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_print_for_modedit_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PRINT_FOR_MODEDIT_BASE::DIALOG_PRINT_FOR_MODEDIT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bmiddleLeftSizer;
	bmiddleLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_ScaleOptionChoices[] = { _("fit in page"), _("Scale 0.5"), _("Scale 0.7"), _("Scale 1"), _("Scale 1.4"), _("Scale 2"), _("Scale 3"), _("Scale 4"), _("Scale 8"), _("Scale 16") };
	int m_ScaleOptionNChoices = sizeof( m_ScaleOptionChoices ) / sizeof( wxString );
	m_ScaleOption = new wxRadioBox( this, wxID_ANY, _("Approx. Scale:"), wxDefaultPosition, wxDefaultSize, m_ScaleOptionNChoices, m_ScaleOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ScaleOption->SetSelection( 3 );
	bmiddleLeftSizer->Add( m_ScaleOption, 0, wxALL, 5 );
	
	
	bMainSizer->Add( bmiddleLeftSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bmiddleRightSizer;
	bmiddleRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_ModeColorOptionChoices[] = { _("Color"), _("Black and white") };
	int m_ModeColorOptionNChoices = sizeof( m_ModeColorOptionChoices ) / sizeof( wxString );
	m_ModeColorOption = new wxRadioBox( this, wxID_PRINT_MODE, _("Print Mode"), wxDefaultPosition, wxDefaultSize, m_ModeColorOptionNChoices, m_ModeColorOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ModeColorOption->SetSelection( 1 );
	m_ModeColorOption->SetToolTip( _("Choose if you want to draw the sheet like it appears on screen,\nor in black and white mode, better to print it when using  black and white printers") );
	
	bmiddleRightSizer->Add( m_ModeColorOption, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bmiddleRightSizer, 1, 0, 5 );
	
	wxBoxSizer* bbuttonsSizer;
	bbuttonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonOption = new wxButton( this, wxID_PRINT_OPTIONS, _("Page Options"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonOption, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonPreview = new wxButton( this, wxID_PREVIEW, _("Preview"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPreview, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonPrint = new wxButton( this, wxID_PRINT_ALL, _("Print"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPrint, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonQuit = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonQuit, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bbuttonsSizer, 0, 0, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_FOR_MODEDIT_BASE::OnCloseWindow ) );
	m_buttonOption->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_FOR_MODEDIT_BASE::OnPageSetup ), NULL, this );
	m_buttonPreview->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_FOR_MODEDIT_BASE::OnPrintPreview ), NULL, this );
	m_buttonPrint->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_FOR_MODEDIT_BASE::OnPrintButtonClick ), NULL, this );
	m_buttonQuit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_FOR_MODEDIT_BASE::OnButtonCancelClick ), NULL, this );
}

DIALOG_PRINT_FOR_MODEDIT_BASE::~DIALOG_PRINT_FOR_MODEDIT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_FOR_MODEDIT_BASE::OnCloseWindow ) );
	m_buttonOption->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_FOR_MODEDIT_BASE::OnPageSetup ), NULL, this );
	m_buttonPreview->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_FOR_MODEDIT_BASE::OnPrintPreview ), NULL, this );
	m_buttonPrint->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_FOR_MODEDIT_BASE::OnPrintButtonClick ), NULL, this );
	m_buttonQuit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_FOR_MODEDIT_BASE::OnButtonCancelClick ), NULL, this );
	
}

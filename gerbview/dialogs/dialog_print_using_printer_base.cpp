///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
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
	
	wxStaticBoxSizer* sbLayersSizer;
	sbLayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layers:") ), wxVERTICAL );
	
	wxBoxSizer* bleftSizer;
	bleftSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_leftLayersBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layers:") ), wxVERTICAL );
	
	
	bleftSizer->Add( m_leftLayersBoxSizer, 1, wxALL, 5 );
	
	m_rightLayersBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layers:") ), wxVERTICAL );
	
	
	bleftSizer->Add( m_rightLayersBoxSizer, 1, wxALL, 5 );
	
	
	sbLayersSizer->Add( bleftSizer, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( sbLayersSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bmiddleLeftSizer;
	bmiddleLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_ScaleOptionChoices[] = { _("fit in page"), _("Scale 0.5"), _("Scale 0.7"), _("Approx. Scale 1"), _("Accurate Scale 1"), _("Scale 1.4"), _("Scale 2"), _("Scale 3"), _("Scale 4") };
	int m_ScaleOptionNChoices = sizeof( m_ScaleOptionChoices ) / sizeof( wxString );
	m_ScaleOption = new wxRadioBox( this, wxID_ANY, _("Approx. Scale:"), wxDefaultPosition, wxDefaultSize, m_ScaleOptionNChoices, m_ScaleOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ScaleOption->SetSelection( 3 );
	bmiddleLeftSizer->Add( m_ScaleOption, 0, wxALL, 5 );
	
	m_FineAdjustXscaleTitle = new wxStaticText( this, wxID_ANY, _("X Scale Adjust"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FineAdjustXscaleTitle->Wrap( -1 );
	bmiddleLeftSizer->Add( m_FineAdjustXscaleTitle, 0, wxRIGHT|wxLEFT, 5 );
	
	m_FineAdjustXscaleOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FineAdjustXscaleOpt->SetMaxLength( 0 ); 
	m_FineAdjustXscaleOpt->SetToolTip( _("Set X scale adjust for exact scale plotting") );
	
	bmiddleLeftSizer->Add( m_FineAdjustXscaleOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_FineAdjustYscaleTitle = new wxStaticText( this, wxID_ANY, _("Y Scale Adjust"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FineAdjustYscaleTitle->Wrap( -1 );
	bmiddleLeftSizer->Add( m_FineAdjustYscaleTitle, 0, wxRIGHT|wxLEFT, 5 );
	
	m_FineAdjustYscaleOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FineAdjustYscaleOpt->SetMaxLength( 0 ); 
	m_FineAdjustYscaleOpt->SetToolTip( _("Set Y scale adjust for exact scale plotting") );
	
	bmiddleLeftSizer->Add( m_FineAdjustYscaleOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bmiddleLeftSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bmiddleRightSizer;
	bmiddleRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbOptionsSizer;
	sbOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options:") ), wxVERTICAL );
	
	m_Print_Mirror = new wxCheckBox( this, wxID_ANY, _("Mirror"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOptionsSizer->Add( m_Print_Mirror, 0, wxALL, 5 );
	
	
	bmiddleRightSizer->Add( sbOptionsSizer, 0, wxEXPAND|wxALL, 5 );
	
	wxString m_ModeColorOptionChoices[] = { _("Color"), _("Black and white") };
	int m_ModeColorOptionNChoices = sizeof( m_ModeColorOptionChoices ) / sizeof( wxString );
	m_ModeColorOption = new wxRadioBox( this, wxID_PRINT_MODE, _("Print Mode"), wxDefaultPosition, wxDefaultSize, m_ModeColorOptionNChoices, m_ModeColorOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ModeColorOption->SetSelection( 0 );
	m_ModeColorOption->SetToolTip( _("Choose if you want to print sheets in color, or force the black and white mode.") );
	
	bmiddleRightSizer->Add( m_ModeColorOption, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bmiddleRightSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* b_buttonsSizer;
	b_buttonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonOption = new wxButton( this, wxID_PRINT_OPTIONS, _("Page Options"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( m_buttonOption, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonPreview = new wxButton( this, wxID_PREVIEW, _("Preview"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( m_buttonPreview, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonPrint = new wxButton( this, wxID_PRINT_ALL, _("Print"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonPrint->SetDefault(); 
	b_buttonsSizer->Add( m_buttonPrint, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonQuit = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( m_buttonQuit, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( b_buttonsSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnCloseWindow ) );
	m_ScaleOption->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnScaleSelectionClick ), NULL, this );
	m_buttonOption->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPageSetup ), NULL, this );
	m_buttonPreview->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPrintPreview ), NULL, this );
	m_buttonPrint->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPrintButtonClick ), NULL, this );
	m_buttonQuit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnButtonCancelClick ), NULL, this );
}

DIALOG_PRINT_USING_PRINTER_BASE::~DIALOG_PRINT_USING_PRINTER_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnCloseWindow ) );
	m_ScaleOption->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnScaleSelectionClick ), NULL, this );
	m_buttonOption->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPageSetup ), NULL, this );
	m_buttonPreview->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPrintPreview ), NULL, this );
	m_buttonPrint->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnPrintButtonClick ), NULL, this );
	m_buttonQuit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_BASE::OnButtonCancelClick ), NULL, this );
	
}

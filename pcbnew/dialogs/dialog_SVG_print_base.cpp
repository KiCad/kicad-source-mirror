///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 30 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_SVG_print_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SVG_PRINT_base::DIALOG_SVG_PRINT_base( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,350 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbLayersSizer;
	sbLayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layers:") ), wxHORIZONTAL );
	
	m_CopperLayersBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Copper Layers:") ), wxVERTICAL );
	
	sbLayersSizer->Add( m_CopperLayersBoxSizer, 1, wxEXPAND, 5 );
	
	m_TechnicalBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Technical Layers:") ), wxVERTICAL );
	
	sbLayersSizer->Add( m_TechnicalBoxSizer, 1, wxEXPAND, 5 );
	
	bUpperSizer->Add( sbLayersSizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbOptionsSizer;
	sbOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Print SVG options:") ), wxVERTICAL );
	
	m_TextPenWidth = new wxStaticText( this, wxID_ANY, _("Pen width mini"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPenWidth->Wrap( -1 );
	sbOptionsSizer->Add( m_TextPenWidth, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DialogPenWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_DialogPenWidth->SetToolTip( _("Selection of the minimum pen thickness used to draw items.") );
	
	sbOptionsSizer->Add( m_DialogPenWidth, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxString m_ModeColorOptionChoices[] = { _("Color"), _("Black and White") };
	int m_ModeColorOptionNChoices = sizeof( m_ModeColorOptionChoices ) / sizeof( wxString );
	m_ModeColorOption = new wxRadioBox( this, wxID_ANY, _("Print mode"), wxDefaultPosition, wxDefaultSize, m_ModeColorOptionNChoices, m_ModeColorOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ModeColorOption->SetSelection( 0 );
	m_ModeColorOption->SetToolTip( _("Choose if you want to draw the sheet like it appears on screen,\nor in black and white mode, better to print it when using  black and white printers") );
	
	sbOptionsSizer->Add( m_ModeColorOption, 0, wxALL|wxEXPAND, 5 );
	
	m_Print_Frame_Ref_Ctrl = new wxCheckBox( this, wxID_ANY, _("Print Frame Ref"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Print_Frame_Ref_Ctrl->SetValue(true); 
	m_Print_Frame_Ref_Ctrl->SetToolTip( _("Print (or not) the Frame references.") );
	
	sbOptionsSizer->Add( m_Print_Frame_Ref_Ctrl, 0, wxALL, 5 );
	
	m_PrintBoardEdgesCtrl = new wxCheckBox( this, wxID_ANY, _("Print Board Edges"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PrintBoardEdgesCtrl->SetValue(true); 
	m_PrintBoardEdgesCtrl->SetToolTip( _("Print (or not) the edges layer with others layers") );
	
	sbOptionsSizer->Add( m_PrintBoardEdgesCtrl, 0, wxALL, 5 );
	
	bUpperSizer->Add( sbOptionsSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonPrintSelected = new wxButton( this, wxID_PRINT_CURRENT, _("Print Selected"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonPrintSelected, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonBoard = new wxButton( this, wxID_PRINT_BOARD, _("Print Board"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonBoard, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonQuit = new wxButton( this, wxID_CANCEL, _("Quit"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonQuit, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	bUpperSizer->Add( bButtonsSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bMainSizer->Add( bUpperSizer, 0, wxEXPAND, 5 );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Filename:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bMainSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_FileNameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FileNameCtrl->SetToolTip( _("Enter a filename if you do not want to use default file names\nCan be used only when printing the current sheet") );
	m_FileNameCtrl->SetMinSize( wxSize( 450,-1 ) );
	
	bMainSizer->Add( m_FileNameCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bMainSizer->Add( m_staticText2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MessagesBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_MessagesBox->SetMinSize( wxSize( -1,100 ) );
	
	bMainSizer->Add( m_MessagesBox, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SVG_PRINT_base::OnCloseWindow ) );
	m_ModeColorOption->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnSetColorModeSelected ), NULL, this );
	m_buttonPrintSelected->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnButtonPrintSelectedClick ), NULL, this );
	m_buttonBoard->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnButtonPrintBoardClick ), NULL, this );
	m_buttonQuit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnButtonCancelClick ), NULL, this );
}

DIALOG_SVG_PRINT_base::~DIALOG_SVG_PRINT_base()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SVG_PRINT_base::OnCloseWindow ) );
	m_ModeColorOption->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnSetColorModeSelected ), NULL, this );
	m_buttonPrintSelected->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnButtonPrintSelectedClick ), NULL, this );
	m_buttonBoard->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnButtonPrintBoardClick ), NULL, this );
	m_buttonQuit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnButtonCancelClick ), NULL, this );
	
}

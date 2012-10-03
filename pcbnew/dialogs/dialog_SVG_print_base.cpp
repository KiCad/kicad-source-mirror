///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 10 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_SVG_print_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SVG_PRINT_base::DIALOG_SVG_PRINT_base( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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
	
	m_TextPenWidth = new wxStaticText( this, wxID_ANY, _("Default pen size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPenWidth->Wrap( -1 );
	sbOptionsSizer->Add( m_TextPenWidth, 0, wxRIGHT|wxLEFT, 5 );
	
	m_DialogDefaultPenSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_DialogDefaultPenSize->SetToolTip( _("Selection of the pen size used to draw items which have no pen size speicfied.") );
	
	sbOptionsSizer->Add( m_DialogDefaultPenSize, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxString m_ModeColorOptionChoices[] = { _("Color"), _("Black and white") };
	int m_ModeColorOptionNChoices = sizeof( m_ModeColorOptionChoices ) / sizeof( wxString );
	m_ModeColorOption = new wxRadioBox( this, wxID_ANY, _("Print mode"), wxDefaultPosition, wxDefaultSize, m_ModeColorOptionNChoices, m_ModeColorOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ModeColorOption->SetSelection( 1 );
	m_ModeColorOption->SetToolTip( _("Choose if you want to draw the sheet like it appears on screen,\nor in black and white mode, better to print it when using  black and white printers") );
	
	sbOptionsSizer->Add( m_ModeColorOption, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxString m_rbSvgPageSizeOptChoices[] = { _("Full page with  frame ref"), _("Current page size"), _("Board area only") };
	int m_rbSvgPageSizeOptNChoices = sizeof( m_rbSvgPageSizeOptChoices ) / sizeof( wxString );
	m_rbSvgPageSizeOpt = new wxRadioBox( this, wxID_ANY, _("SVG Page Size"), wxDefaultPosition, wxDefaultSize, m_rbSvgPageSizeOptNChoices, m_rbSvgPageSizeOptChoices, 1, wxRA_SPECIFY_COLS );
	m_rbSvgPageSizeOpt->SetSelection( 0 );
	sbOptionsSizer->Add( m_rbSvgPageSizeOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_PrintBoardEdgesCtrl = new wxCheckBox( this, wxID_ANY, _("Print board edges"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PrintBoardEdgesCtrl->SetValue(true); 
	m_PrintBoardEdgesCtrl->SetToolTip( _("Print (or not) the edges layer on others layers") );
	
	sbOptionsSizer->Add( m_PrintBoardEdgesCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_printMirrorOpt = new wxCheckBox( this, wxID_ANY, _("Print mirror"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOptionsSizer->Add( m_printMirrorOpt, 0, wxRIGHT|wxLEFT, 5 );
	
	
	bUpperSizer->Add( sbOptionsSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_rbFileOptChoices[] = { _("One file per layer"), _("All in one file") };
	int m_rbFileOptNChoices = sizeof( m_rbFileOptChoices ) / sizeof( wxString );
	m_rbFileOpt = new wxRadioBox( this, wxID_ANY, _("File option:"), wxDefaultPosition, wxDefaultSize, m_rbFileOptNChoices, m_rbFileOptChoices, 1, wxRA_SPECIFY_COLS );
	m_rbFileOpt->SetSelection( 0 );
	bButtonsSizer->Add( m_rbFileOpt, 0, wxALL, 5 );
	
	m_buttonCreateFile = new wxButton( this, wxID_PRINT_BOARD, _("Plot"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonCreateFile, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonQuit = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonQuit, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bButtonsSizer, 0, wxEXPAND, 5 );
	
	
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
	m_buttonCreateFile->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnButtonPlot ), NULL, this );
	m_buttonQuit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnButtonCancelClick ), NULL, this );
}

DIALOG_SVG_PRINT_base::~DIALOG_SVG_PRINT_base()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SVG_PRINT_base::OnCloseWindow ) );
	m_buttonCreateFile->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnButtonPlot ), NULL, this );
	m_buttonQuit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SVG_PRINT_base::OnButtonCancelClick ), NULL, this );
	
}

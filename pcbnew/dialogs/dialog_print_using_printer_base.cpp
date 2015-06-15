///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_print_using_printer_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PRINT_USING_PRINTER_base::DIALOG_PRINT_USING_PRINTER_base( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbLayersSizer;
	sbLayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layers:") ), wxVERTICAL );
	
	wxBoxSizer* bleftSizer;
	bleftSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_CopperLayersBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Copper Layers:") ), wxVERTICAL );
	
	
	bleftSizer->Add( m_CopperLayersBoxSizer, 1, wxALL, 5 );
	
	m_TechnicalLayersBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Technical Layers:") ), wxVERTICAL );
	
	
	bleftSizer->Add( m_TechnicalLayersBoxSizer, 1, wxALL, 5 );
	
	
	sbLayersSizer->Add( bleftSizer, 1, wxEXPAND, 5 );
	
	m_Exclude_Edges_Pcb = new wxCheckBox( this, wxID_ANY, _("Exclude Edges_Pcb Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Exclude_Edges_Pcb->SetToolTip( _("Exclude contents of Edges_Pcb layer from all other layers") );
	
	sbLayersSizer->Add( m_Exclude_Edges_Pcb, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( sbLayersSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bmiddleLeftSizer;
	bmiddleLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_ScaleOptionChoices[] = { _("fit in page"), _("Scale 0.5"), _("Scale 0.7"), _("Approx. Scale 1"), _("Accurate Scale 1"), _("Scale 1.4"), _("Scale 2"), _("Scale 3"), _("Scale 4") };
	int m_ScaleOptionNChoices = sizeof( m_ScaleOptionChoices ) / sizeof( wxString );
	m_ScaleOption = new wxRadioBox( this, wxID_ANY, _("Approx. Scale:"), wxDefaultPosition, wxDefaultSize, m_ScaleOptionNChoices, m_ScaleOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ScaleOption->SetSelection( 4 );
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
	
	m_TextPenWidth = new wxStaticText( this, wxID_ANY, _("Default pen size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPenWidth->Wrap( -1 );
	m_TextPenWidth->SetToolTip( _("Pen size used to draw items that have no pen size specified.\nUsed mainly to draw items in sketch mode.") );
	
	sbOptionsSizer->Add( m_TextPenWidth, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DialogPenWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_DialogPenWidth->SetMaxLength( 0 ); 
	sbOptionsSizer->Add( m_DialogPenWidth, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_Print_Sheet_Ref = new wxCheckBox( this, wxID_FRAME_SEL, _("Print frame ref"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Print_Sheet_Ref->SetValue(true); 
	m_Print_Sheet_Ref->SetToolTip( _("Print (or not) the Frame references.") );
	
	sbOptionsSizer->Add( m_Print_Sheet_Ref, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Print_Mirror = new wxCheckBox( this, wxID_ANY, _("Mirror"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOptionsSizer->Add( m_Print_Mirror, 0, wxALL, 5 );
	
	
	bmiddleRightSizer->Add( sbOptionsSizer, 0, wxEXPAND|wxALL, 5 );
	
	wxString m_Drill_Shape_OptChoices[] = { _("No drill mark"), _("Small mark"), _("Real drill") };
	int m_Drill_Shape_OptNChoices = sizeof( m_Drill_Shape_OptChoices ) / sizeof( wxString );
	m_Drill_Shape_Opt = new wxRadioBox( this, wxID_ANY, _("Pads Drill Opt"), wxDefaultPosition, wxDefaultSize, m_Drill_Shape_OptNChoices, m_Drill_Shape_OptChoices, 1, wxRA_SPECIFY_COLS );
	m_Drill_Shape_Opt->SetSelection( 1 );
	bmiddleRightSizer->Add( m_Drill_Shape_Opt, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_ModeColorOptionChoices[] = { _("Color"), _("Black and white") };
	int m_ModeColorOptionNChoices = sizeof( m_ModeColorOptionChoices ) / sizeof( wxString );
	m_ModeColorOption = new wxRadioBox( this, wxID_PRINT_MODE, _("Print Mode"), wxDefaultPosition, wxDefaultSize, m_ModeColorOptionNChoices, m_ModeColorOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ModeColorOption->SetSelection( 1 );
	m_ModeColorOption->SetToolTip( _("Choose if you want to draw the sheet like it appears on screen,\nor in black and white mode, better to print it when using  black and white printers") );
	
	bmiddleRightSizer->Add( m_ModeColorOption, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bmiddleRightSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bbuttonsSizer;
	bbuttonsSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_PagesOptionChoices[] = { _("1 Page per Layer"), _("Single page") };
	int m_PagesOptionNChoices = sizeof( m_PagesOptionChoices ) / sizeof( wxString );
	m_PagesOption = new wxRadioBox( this, wxID_PAGE_MODE, _("Page Print"), wxDefaultPosition, wxDefaultSize, m_PagesOptionNChoices, m_PagesOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_PagesOption->SetSelection( 0 );
	bbuttonsSizer->Add( m_PagesOption, 0, wxALL|wxEXPAND, 5 );
	
	
	bbuttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonOption = new wxButton( this, wxID_PRINT_OPTIONS, _("Page Options"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonOption, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonPreview = new wxButton( this, wxID_PREVIEW, _("Preview"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPreview, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonPrint = new wxButton( this, wxID_PRINT_ALL, _("Print"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPrint, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonQuit = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonQuit->SetDefault(); 
	bbuttonsSizer->Add( m_buttonQuit, 0, wxALL|wxEXPAND, 5 );
	
	
	bbuttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bbuttonsSizer, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnCloseWindow ) );
	m_ScaleOption->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnScaleSelectionClick ), NULL, this );
	m_buttonOption->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPageSetup ), NULL, this );
	m_buttonPreview->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPrintPreview ), NULL, this );
	m_buttonPrint->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPrintButtonClick ), NULL, this );
	m_buttonQuit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnButtonCancelClick ), NULL, this );
}

DIALOG_PRINT_USING_PRINTER_base::~DIALOG_PRINT_USING_PRINTER_base()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnCloseWindow ) );
	m_ScaleOption->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnScaleSelectionClick ), NULL, this );
	m_buttonOption->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPageSetup ), NULL, this );
	m_buttonPreview->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPrintPreview ), NULL, this );
	m_buttonPrint->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnPrintButtonClick ), NULL, this );
	m_buttonQuit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_USING_PRINTER_base::OnButtonCancelClick ), NULL, this );
	
}

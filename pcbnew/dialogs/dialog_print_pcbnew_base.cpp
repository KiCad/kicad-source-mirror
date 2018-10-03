///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 17 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_print_pcbnew_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PRINT_PCBNEW_BASE::DIALOG_PRINT_PCBNEW_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbLayersSizer;
	sbLayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Included Layers") ), wxVERTICAL );
	
	wxBoxSizer* bLayerListsSizer;
	bLayerListsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText4 = new wxStaticText( sbLayersSizer->GetStaticBox(), wxID_ANY, _("Copper layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	bSizer6->Add( m_staticText4, 0, wxRIGHT|wxLEFT, 5 );
	
	wxArrayString m_CopperLayersListChoices;
	m_CopperLayersList = new wxCheckListBox( sbLayersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_CopperLayersListChoices, 0 );
	bSizer6->Add( m_CopperLayersList, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bLayerListsSizer->Add( bSizer6, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText5 = new wxStaticText( sbLayersSizer->GetStaticBox(), wxID_ANY, _("Technical layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bSizer7->Add( m_staticText5, 0, wxRIGHT|wxLEFT, 5 );
	
	wxArrayString m_TechnicalLayersListChoices;
	m_TechnicalLayersList = new wxCheckListBox( sbLayersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TechnicalLayersListChoices, 0 );
	bSizer7->Add( m_TechnicalLayersList, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bLayerListsSizer->Add( bSizer7, 1, wxEXPAND, 5 );
	
	
	sbLayersSizer->Add( bLayerListsSizer, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonSelectAll = new wxButton( sbLayersSizer->GetStaticBox(), wxID_ANY, _("Select all"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_buttonSelectAll, 1, wxALL, 5 );
	
	m_buttonDeselectAll = new wxButton( sbLayersSizer->GetStaticBox(), wxID_ANY, _("Deselect all"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_buttonDeselectAll, 1, wxALL, 5 );
	
	
	sbLayersSizer->Add( bSizer9, 0, wxEXPAND, 5 );
	
	m_Exclude_Edges_Pcb = new wxCheckBox( sbLayersSizer->GetStaticBox(), wxID_ANY, _("Exclude PCB edge layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Exclude_Edges_Pcb->SetToolTip( _("Exclude contents of Edges_Pcb layer from all other layers") );
	
	sbLayersSizer->Add( m_Exclude_Edges_Pcb, 0, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( sbLayersSizer, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bOptionsSizer;
	bOptionsSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbOptionsSizer;
	sbOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );
	
	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 2, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_penWidthLabel = new wxStaticText( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Default line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_penWidthLabel->Wrap( -1 );
	m_penWidthLabel->SetToolTip( _("Pen size used to draw items that have no pen size specified.\nUsed mainly to draw items in sketch mode.") );
	
	gbSizer1->Add( m_penWidthLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );
	
	m_penWidthCtrl = new wxTextCtrl( sbOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_penWidthCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_penWidthUnits = new wxStaticText( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_penWidthUnits->Wrap( -1 );
	gbSizer1->Add( m_penWidthUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );
	
	m_drillMarksLabel = new wxStaticText( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Drill marks:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_drillMarksLabel->Wrap( -1 );
	gbSizer1->Add( m_drillMarksLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_drillMarksChoiceChoices[] = { _("No drill mark"), _("Small mark"), _("Real drill") };
	int m_drillMarksChoiceNChoices = sizeof( m_drillMarksChoiceChoices ) / sizeof( wxString );
	m_drillMarksChoice = new wxChoice( sbOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_drillMarksChoiceNChoices, m_drillMarksChoiceChoices, 0 );
	m_drillMarksChoice->SetSelection( 0 );
	gbSizer1->Add( m_drillMarksChoice, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_outputModeLabel = new wxStaticText( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Output mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_outputModeLabel->Wrap( -1 );
	gbSizer1->Add( m_outputModeLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	wxString m_outputModeChoices[] = { _("Color"), _("Black and white") };
	int m_outputModeNChoices = sizeof( m_outputModeChoices ) / sizeof( wxString );
	m_outputMode = new wxChoice( sbOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_outputModeNChoices, m_outputModeChoices, 0 );
	m_outputMode->SetSelection( 0 );
	gbSizer1->Add( m_outputMode, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_Print_Sheet_Ref = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_FRAME_SEL, _("Print border and title block"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Print_Sheet_Ref->SetValue(true); 
	m_Print_Sheet_Ref->SetToolTip( _("Print Frame references.") );
	
	gbSizer1->Add( m_Print_Sheet_Ref, wxGBPosition( 3, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_Print_Mirror = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Print mirrored"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_Print_Mirror, wxGBPosition( 4, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	gbSizer1->AddGrowableCol( 1 );
	
	sbOptionsSizer->Add( gbSizer1, 1, wxEXPAND, 5 );
	
	
	bOptionsSizer->Add( sbOptionsSizer, 1, wxEXPAND|wxALL, 5 );
	
	wxString m_PagesOptionChoices[] = { _("One page per layer"), _("All layers on single page") };
	int m_PagesOptionNChoices = sizeof( m_PagesOptionChoices ) / sizeof( wxString );
	m_PagesOption = new wxRadioBox( this, wxID_PAGE_MODE, _("Pagination"), wxDefaultPosition, wxDefaultSize, m_PagesOptionNChoices, m_PagesOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_PagesOption->SetSelection( 0 );
	bOptionsSizer->Add( m_PagesOption, 0, wxALL|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	wxStaticBoxSizer* bScaleSizer;
	bScaleSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scale") ), wxVERTICAL );
	
	m_scale1 = new wxRadioButton( bScaleSizer->GetStaticBox(), wxID_ANY, _("1:1"), wxDefaultPosition, wxDefaultSize, 0 );
	bScaleSizer->Add( m_scale1, 0, 0, 5 );
	
	m_scaleFit = new wxRadioButton( bScaleSizer->GetStaticBox(), wxID_ANY, _("Fit to page"), wxDefaultPosition, wxDefaultSize, 0 );
	bScaleSizer->Add( m_scaleFit, 0, wxTOP, 5 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );
	
	m_scaleCustom = new wxRadioButton( bScaleSizer->GetStaticBox(), wxID_ANY, _("Custom:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer10->Add( m_scaleCustom, 0, wxTOP, 5 );
	
	m_scaleCustomText = new wxTextCtrl( bScaleSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_scaleCustomText->SetToolTip( _("Set X scale adjust for exact scale plotting") );
	
	bSizer10->Add( m_scaleCustomText, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bScaleSizer->Add( bSizer10, 1, wxEXPAND, 5 );
	
	
	bOptionsSizer->Add( bScaleSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bOptionsSizer, 0, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bUpperSizer, 1, wxEXPAND|wxALL, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );
	
	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonOption = new wxButton( this, wxID_PRINT_OPTIONS, _("Page Setup..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOption->SetMinSize( wxSize( 120,-1 ) );
	
	bButtonsSizer->Add( m_buttonOption, 0, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Apply = new wxButton( this, wxID_APPLY );
	m_sdbSizer1->AddButton( m_sdbSizer1Apply );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bButtonsSizer->Add( m_sdbSizer1, 1, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bButtonsSizer, 0, wxEXPAND|wxLEFT, 10 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_buttonSelectAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnSelectAllClick ), NULL, this );
	m_buttonDeselectAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnDeselectAllClick ), NULL, this );
	m_scaleCustomText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnSetCustomScale ), NULL, this );
	m_buttonOption->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnPageSetup ), NULL, this );
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnPrintPreview ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnPrintButtonClick ), NULL, this );
}

DIALOG_PRINT_PCBNEW_BASE::~DIALOG_PRINT_PCBNEW_BASE()
{
	// Disconnect Events
	m_buttonSelectAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnSelectAllClick ), NULL, this );
	m_buttonDeselectAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnDeselectAllClick ), NULL, this );
	m_scaleCustomText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnSetCustomScale ), NULL, this );
	m_buttonOption->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnPageSetup ), NULL, this );
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnPrintPreview ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_PCBNEW_BASE::OnPrintButtonClick ), NULL, this );
	
}

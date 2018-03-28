///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"
#include "widgets/text_ctrl_eval.h"

#include "dialog_edit_footprint_text_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EDIT_FPTEXT_BASE::DIALOG_EDIT_FPTEXT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* upperSizer;
	upperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxFlexGridSizer* leftSizer;
	leftSizer = new wxFlexGridSizer( 8, 3, 0, 0 );
	leftSizer->AddGrowableCol( 1 );
	leftSizer->AddGrowableRow( 6 );
	leftSizer->SetFlexibleDirection( wxBOTH );
	leftSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	m_TextDataTitle = new wxStaticText( this, wxID_ANY, _("Reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextDataTitle->Wrap( -1 );
	leftSizer->Add( m_TextDataTitle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_Name = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Name->SetMinSize( wxSize( 160,-1 ) );
	
	leftSizer->Add( m_Name, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	
	leftSizer->Add( 0, 0, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, 5 );
	
	m_widthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthLabel->Wrap( -1 );
	leftSizer->Add( m_widthLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_widthCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	leftSizer->Add( m_widthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxLEFT, 5 );
	
	m_widthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthUnits->Wrap( -1 );
	leftSizer->Add( m_widthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	m_heightLabel = new wxStaticText( this, wxID_ANY, _("Height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_heightLabel->Wrap( -1 );
	leftSizer->Add( m_heightLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_heightCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	leftSizer->Add( m_heightCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxLEFT, 5 );
	
	m_heightUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_heightUnits->Wrap( -1 );
	leftSizer->Add( m_heightUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	leftSizer->Add( m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_thicknessCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	leftSizer->Add( m_thicknessCtrl, 0, wxEXPAND|wxTOP|wxLEFT, 5 );
	
	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	leftSizer->Add( m_thicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	m_posXLabel = new wxStaticText( this, wxID_ANY, _("Offset X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXLabel->Wrap( -1 );
	leftSizer->Add( m_posXLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_posXCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	leftSizer->Add( m_posXCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxLEFT, 5 );
	
	m_posXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXUnits->Wrap( -1 );
	leftSizer->Add( m_posXUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	m_posYLabel = new wxStaticText( this, wxID_ANY, _("Offset Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYLabel->Wrap( -1 );
	leftSizer->Add( m_posYLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_posYCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	leftSizer->Add( m_posYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT|wxEXPAND, 5 );
	
	m_posYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYUnits->Wrap( -1 );
	leftSizer->Add( m_posYUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	
	leftSizer->Add( 0, 0, 0, wxEXPAND, 5 );
	
	
	leftSizer->Add( 0, 0, 0, wxEXPAND, 5 );
	
	
	leftSizer->Add( 0, 0, 0, wxEXPAND, 5 );
	
	m_LayerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerLabel->Wrap( -1 );
	leftSizer->Add( m_LayerLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_LayerSelectionCtrl = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	leftSizer->Add( m_LayerSelectionCtrl, 0, wxEXPAND|wxLEFT, 5 );
	
	
	upperSizer->Add( leftSizer, 1, wxALL|wxEXPAND, 10 );
	
	
	upperSizer->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* rightSizer;
	rightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_Show = new wxCheckBox( this, wxID_ANY, _("Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	rightSizer->Add( m_Show, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Italic = new wxCheckBox( this, wxID_ANY, _("Italic"), wxDefaultPosition, wxDefaultSize, 0 );
	rightSizer->Add( m_Italic, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	rightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	stOrienationLabel = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	stOrienationLabel->Wrap( -1 );
	rightSizer->Add( stOrienationLabel, 0, wxALL, 5 );
	
	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 2, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_Orient0 = new wxRadioButton( this, wxID_ANY, _("0.0 deg"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_Orient0, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), 0, 5 );
	
	m_Orient90 = new wxRadioButton( this, wxID_ANY, _("90.0 deg"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_Orient90, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), 0, 5 );
	
	m_Orient270 = new wxRadioButton( this, wxID_ANY, _("-90.0 deg"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_Orient270, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), 0, 5 );
	
	m_Orient180 = new wxRadioButton( this, wxID_ANY, _("180.0 deg"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_Orient180, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), 0, 5 );
	
	m_OrientOther = new wxRadioButton( this, wxID_ANY, _("Other:"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_OrientOther, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );
	
	m_OrientValueCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_OrientValueCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	gbSizer1->AddGrowableCol( 1 );
	
	rightSizer->Add( gbSizer1, 0, wxEXPAND|wxLEFT, 20 );
	
	m_unlock = new wxCheckBox( this, wxID_ANY, _("Unconstrained"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unlock->SetToolTip( _("If orientation is locked, the text will always face near the bottom or right edge of the board.") );
	
	rightSizer->Add( m_unlock, 0, wxALL, 5 );
	
	
	upperSizer->Add( rightSizer, 0, wxEXPAND|wxALL, 10 );
	
	
	bMainSizer->Add( upperSizer, 0, wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline2, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );
	
	wxBoxSizer* lowerSizer;
	lowerSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_statusLine1 = new wxStaticText( this, wxID_ANY, _("Status line 1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_statusLine1->Wrap( -1 );
	m_statusLine1->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizer7->Add( m_statusLine1, 0, wxBOTTOM|wxRIGHT|wxLEFT, 2 );
	
	m_statusLine2 = new wxStaticText( this, wxID_ANY, _("Status line 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_statusLine2->Wrap( -1 );
	m_statusLine2->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizer7->Add( m_statusLine2, 0, wxRIGHT|wxLEFT, 2 );
	
	
	lowerSizer->Add( bSizer7, 0, wxEXPAND|wxTOP|wxLEFT, 8 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	lowerSizer->Add( m_sdbSizer, 1, wxEXPAND|wxALL, 5 );
	
	
	bMainSizer->Add( lowerSizer, 1, wxEXPAND|wxLEFT, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_FPTEXT_BASE::OnInitDlg ) );
	m_Orient0->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::ModuleOrientEvent ), NULL, this );
	m_Orient90->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::ModuleOrientEvent ), NULL, this );
	m_Orient270->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::ModuleOrientEvent ), NULL, this );
	m_Orient180->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::ModuleOrientEvent ), NULL, this );
	m_OrientOther->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::ModuleOrientEvent ), NULL, this );
	m_OrientValueCtrl->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( DIALOG_EDIT_FPTEXT_BASE::OnOtherOrientation ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::OnOkClick ), NULL, this );
}

DIALOG_EDIT_FPTEXT_BASE::~DIALOG_EDIT_FPTEXT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_FPTEXT_BASE::OnInitDlg ) );
	m_Orient0->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::ModuleOrientEvent ), NULL, this );
	m_Orient90->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::ModuleOrientEvent ), NULL, this );
	m_Orient270->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::ModuleOrientEvent ), NULL, this );
	m_Orient180->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::ModuleOrientEvent ), NULL, this );
	m_OrientOther->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::ModuleOrientEvent ), NULL, this );
	m_OrientValueCtrl->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( DIALOG_EDIT_FPTEXT_BASE::OnOtherOrientation ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_FPTEXT_BASE::OnOkClick ), NULL, this );
	
}

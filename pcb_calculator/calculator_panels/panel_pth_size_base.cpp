///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_pth_size_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PTH_SIZE_BASE::PANEL_PTH_SIZE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerLeft;
	sbSizerLeft = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Properties") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerLeft;
	fgSizerLeft = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizerLeft->SetFlexibleDirection( wxBOTH );
	fgSizerLeft->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextLeadShape = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Lead shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLeadShape->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextLeadShape, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_shapeChoiceChoices[] = { _("Round"), _("Square"), _("Rectangular") };
	int m_shapeChoiceNChoices = sizeof( m_shapeChoiceChoices ) / sizeof( wxString );
	m_shapeChoice = new wxChoice( sbSizerLeft->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_shapeChoiceNChoices, m_shapeChoiceChoices, 0 );
	m_shapeChoice->SetSelection( 0 );
	fgSizerLeft->Add( m_shapeChoice, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerLeft->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerLeft->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerLeft->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextMin = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Min"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticTextMin->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextMin, 0, wxALL|wxEXPAND, 5 );

	m_staticTextMax = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Max"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticTextMax->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextMax, 0, wxALL|wxEXPAND, 5 );


	fgSizerLeft->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextSizeX = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeX->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextSizeX, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_sizeXMinCtrl = new wxTextCtrl( sbSizerLeft->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_sizeXMinCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_sizeXMaxCtrl = new wxTextCtrl( sbSizerLeft->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_sizeXMaxCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextSizeXUnit = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeXUnit->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextSizeXUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_staticTextSizeY = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeY->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextSizeY, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_sizeYMinCtrl = new wxTextCtrl( sbSizerLeft->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_sizeYMinCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_sizeYMaxCtrl = new wxTextCtrl( sbSizerLeft->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_sizeYMaxCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextSizeYUnit = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeYUnit->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextSizeYUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );


	fgSizerLeft->Add( 0, 0, 1, wxEXPAND, 5 );


	sbSizerLeft->Add( fgSizerLeft, 0, wxALL|wxEXPAND, 5 );


	bSizer6->Add( sbSizerLeft, 0, wxBOTTOM|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerLeft1;
	sbSizerLeft1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Hole size standard") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerLeft1;
	fgSizerLeft1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerLeft1->AddGrowableCol( 1 );
	fgSizerLeft1->SetFlexibleDirection( wxBOTH );
	fgSizerLeft1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextStandard = new wxStaticText( sbSizerLeft1->GetStaticBox(), wxID_ANY, _("Standard:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStandard->Wrap( -1 );
	fgSizerLeft1->Add( m_staticTextStandard, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	wxArrayString m_standardChoiceChoices;
	m_standardChoice = new wxChoice( sbSizerLeft1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_standardChoiceChoices, 0 );
	m_standardChoice->SetSelection( 0 );
	fgSizerLeft1->Add( m_standardChoice, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerLeft1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextDensity = new wxStaticText( sbSizerLeft1->GetStaticBox(), wxID_ANY, _("Level:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDensity->Wrap( -1 );
	fgSizerLeft1->Add( m_staticTextDensity, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	wxArrayString m_levelChoiceChoices;
	m_levelChoice = new wxChoice( sbSizerLeft1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_levelChoiceChoices, 0 );
	m_levelChoice->SetSelection( 0 );
	fgSizerLeft1->Add( m_levelChoice, 0, wxALL|wxEXPAND, 5 );


	fgSizerLeft1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextRoundTo = new wxStaticText( sbSizerLeft1->GetStaticBox(), wxID_ANY, _("Round to:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRoundTo->Wrap( -1 );
	fgSizerLeft1->Add( m_staticTextRoundTo, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_sizeRoundingCtrl = new wxTextCtrl( sbSizerLeft1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft1->Add( m_sizeRoundingCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextRoundingUnit = new wxStaticText( sbSizerLeft1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRoundingUnit->Wrap( -1 );
	fgSizerLeft1->Add( m_staticTextRoundingUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_staticTextRoundType = new wxStaticText( sbSizerLeft1->GetStaticBox(), wxID_ANY, _("Rounding:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRoundType->Wrap( -1 );
	fgSizerLeft1->Add( m_staticTextRoundType, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	wxString m_choiceRoundingChoices[] = { _("Up"), _("Nearest") };
	int m_choiceRoundingNChoices = sizeof( m_choiceRoundingChoices ) / sizeof( wxString );
	m_choiceRounding = new wxChoice( sbSizerLeft1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRoundingNChoices, m_choiceRoundingChoices, 0 );
	m_choiceRounding->SetSelection( 0 );
	fgSizerLeft1->Add( m_choiceRounding, 0, wxALL|wxEXPAND, 5 );


	fgSizerLeft1->Add( 0, 0, 1, wxEXPAND, 5 );


	sbSizerLeft1->Add( fgSizerLeft1, 0, wxALL|wxEXPAND, 5 );


	bSizer6->Add( sbSizerLeft1, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerLeft11;
	sbSizerLeft11 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Annular ring") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerLeft11;
	fgSizerLeft11 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerLeft11->AddGrowableCol( 1 );
	fgSizerLeft11->SetFlexibleDirection( wxBOTH );
	fgSizerLeft11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextAnnular = new wxStaticText( sbSizerLeft11->GetStaticBox(), wxID_ANY, _("Annular ring:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAnnular->Wrap( -1 );
	fgSizerLeft11->Add( m_staticTextAnnular, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_annularCtrl = new wxTextCtrl( sbSizerLeft11->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft11->Add( m_annularCtrl, 0, wxALL|wxEXPAND, 5 );

	m_staticTextAnnularUnit = new wxStaticText( sbSizerLeft11->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAnnularUnit->Wrap( -1 );
	fgSizerLeft11->Add( m_staticTextAnnularUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );


	sbSizerLeft11->Add( fgSizerLeft11, 0, wxALL|wxEXPAND, 5 );


	bSizer6->Add( sbSizerLeft11, 0, wxEXPAND, 5 );


	bSizer4->Add( bSizer6, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerRight;
	sbSizerRight = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Result") ), wxVERTICAL );

	sbSizerRight->SetMinSize( wxSize( 500,-1 ) );
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );


	bSizer5->Add( 0, 0, 1, wxEXPAND, 5 );

	m_bitmapPreview = new wxStaticBitmap( sbSizerRight->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_bitmapPreview, 0, wxALL, 10 );

	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer5->SetFlexibleDirection( wxBOTH );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbShowMinLeadSize = new wxCheckBox( sbSizerRight->GetStaticBox(), wxID_ANY, _("Show minimum lead size"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_cbShowMinLeadSize, 0, 0, 5 );

	m_cbShowMinHoleSize = new wxCheckBox( sbSizerRight->GetStaticBox(), wxID_ANY, _("Show minimum hole size"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_cbShowMinHoleSize, 0, 0, 5 );

	m_cbShowMaxHoleSize = new wxCheckBox( sbSizerRight->GetStaticBox(), wxID_ANY, _("Show maximum hole size"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_cbShowMaxHoleSize, 0, 0, 5 );


	bSizer5->Add( fgSizer5, 0, wxEXPAND, 5 );


	bSizer5->Add( 0, 0, 1, wxEXPAND, 5 );


	sbSizerRight->Add( bSizer5, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer51;
	fgSizer51 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer51->SetFlexibleDirection( wxBOTH );
	fgSizer51->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextHoleSizeResultTitle = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("Recommended hole size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHoleSizeResultTitle->Wrap( -1 );
	fgSizer51->Add( m_staticTextHoleSizeResultTitle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_holeSizeCtrl = new wxTextCtrl( sbSizerRight->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer51->Add( m_holeSizeCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextHoleSizeResultUnit = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHoleSizeResultUnit->Wrap( -1 );
	fgSizer51->Add( m_staticTextHoleSizeResultUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticTextPadSizeResultTitle = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("Recommended pad size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPadSizeResultTitle->Wrap( -1 );
	fgSizer51->Add( m_staticTextPadSizeResultTitle, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_padSizeCtrl = new wxTextCtrl( sbSizerRight->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer51->Add( m_padSizeCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextPadSizeResultUnit = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPadSizeResultUnit->Wrap( -1 );
	fgSizer51->Add( m_staticTextPadSizeResultUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizer8->Add( fgSizer51, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	m_ResultReport = new wxTextCtrl( sbSizerRight->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_ResultReport->SetMinSize( wxSize( -1,250 ) );

	bSizer10->Add( m_ResultReport, 1, wxALL|wxEXPAND, 5 );

	m_bpCopyAll = new wxBitmapButton( sbSizerRight->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer10->Add( m_bpCopyAll, 0, wxRIGHT|wxTOP, 5 );


	bSizer8->Add( bSizer10, 1, wxEXPAND, 5 );


	sbSizerRight->Add( bSizer8, 1, wxEXPAND, 5 );


	bSizer4->Add( sbSizerRight, 1, wxALL|wxEXPAND, 5 );


	bSizer9->Add( bSizer4, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxVERTICAL );

	m_staticTextSummaryTitle = new wxStaticText( this, wxID_ANY, _("IPC-2222B Summary"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSummaryTitle->Wrap( -1 );
	bSizer61->Add( m_staticTextSummaryTitle, 0, wxALL, 5 );

	m_ipc2222Summary = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_ipc2222Summary->CreateGrid( 2, 3 );
	m_ipc2222Summary->EnableEditing( false );
	m_ipc2222Summary->EnableGridLines( true );
	m_ipc2222Summary->EnableDragGridSize( false );
	m_ipc2222Summary->SetMargins( 0, 0 );

	// Columns
	m_ipc2222Summary->SetColSize( 0, 1 );
	m_ipc2222Summary->SetColSize( 1, 1 );
	m_ipc2222Summary->SetColSize( 2, 1 );
	m_ipc2222Summary->EnableDragColMove( false );
	m_ipc2222Summary->EnableDragColSize( true );
	m_ipc2222Summary->SetColLabelSize( 30 );
	m_ipc2222Summary->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_ipc2222Summary->SetRowSize( 0, 24 );
	m_ipc2222Summary->SetRowSize( 1, 24 );
	m_ipc2222Summary->EnableDragRowSize( false );
	m_ipc2222Summary->SetRowLabelValue( 0, _("Min. hole diameter") );
	m_ipc2222Summary->SetRowLabelValue( 1, _("Max. hole diameter") );
	m_ipc2222Summary->SetRowLabelSize( 150 );
	m_ipc2222Summary->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_ipc2222Summary->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	bSizer61->Add( m_ipc2222Summary, 0, wxALL|wxEXPAND, 5 );


	bSizer9->Add( bSizer61, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizer9, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	m_shapeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnControlsChanged ), NULL, this );
	m_sizeXMinCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_sizeXMaxCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_sizeYMinCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_sizeYMaxCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_standardChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnControlsChanged ), NULL, this );
	m_levelChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnControlsChanged ), NULL, this );
	m_sizeRoundingCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_choiceRounding->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnControlsChanged ), NULL, this );
	m_annularCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_cbShowMinLeadSize->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnPreviewSettingCb ), NULL, this );
	m_cbShowMinHoleSize->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnPreviewSettingCb ), NULL, this );
	m_cbShowMaxHoleSize->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnPreviewSettingCb ), NULL, this );
	m_bpCopyAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnCopyReportText ), NULL, this );
}

PANEL_PTH_SIZE_BASE::~PANEL_PTH_SIZE_BASE()
{
	// Disconnect Events
	m_shapeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnControlsChanged ), NULL, this );
	m_sizeXMinCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_sizeXMaxCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_sizeYMinCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_sizeYMaxCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_standardChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnControlsChanged ), NULL, this );
	m_levelChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnControlsChanged ), NULL, this );
	m_sizeRoundingCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_choiceRounding->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnControlsChanged ), NULL, this );
	m_annularCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnValueChanged ), NULL, this );
	m_cbShowMinLeadSize->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnPreviewSettingCb ), NULL, this );
	m_cbShowMinHoleSize->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnPreviewSettingCb ), NULL, this );
	m_cbShowMaxHoleSize->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnPreviewSettingCb ), NULL, this );
	m_bpCopyAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PTH_SIZE_BASE::OnCopyReportText ), NULL, this );

}

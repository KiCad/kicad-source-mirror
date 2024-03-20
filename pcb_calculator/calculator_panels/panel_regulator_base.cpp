///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_regulator_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_REGULATOR_BASE::PANEL_REGULATOR_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMainReg;
	bSizerMainReg = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerColBalancer;
	bSizerColBalancer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizeLeftpReg;
	bSizeLeftpReg = new wxBoxSizer( wxVERTICAL );

	bSizeLeftpReg->SetMinSize( wxSize( 400,-1 ) );
	wxBoxSizer* bSizerType;
	bSizerType = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextRegType = new wxStaticText( this, wxID_ANY, _("Type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRegType->Wrap( -1 );
	m_staticTextRegType->SetToolTip( _("Type of the regulator.\nThere are 2 types:\n- regulators which have a dedicated sense pin for the voltage regulation.\n- 3 terminal pins.") );

	bSizerType->Add( m_staticTextRegType, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_choiceRegTypeChoices[] = { _("Standard Type"), _("3 Terminal Type") };
	int m_choiceRegTypeNChoices = sizeof( m_choiceRegTypeChoices ) / sizeof( wxString );
	m_choiceRegType = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRegTypeNChoices, m_choiceRegTypeChoices, 0 );
	m_choiceRegType->SetSelection( 0 );
	bSizerType->Add( m_choiceRegType, 1, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizeLeftpReg->Add( bSizerType, 0, wxEXPAND|wxALL, 5 );


	bSizeLeftpReg->Add( 0, 10, 0, wxEXPAND, 5 );

	m_bitmapRegul4pins = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizeLeftpReg->Add( m_bitmapRegul4pins, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 10 );

	m_bitmapRegul3pins = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizeLeftpReg->Add( m_bitmapRegul3pins, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 10 );


	bSizeLeftpReg->Add( 0, 10, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerRegFormula;
	sbSizerRegFormula = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Formula") ), wxVERTICAL );

	m_RegulFormula = new wxStaticText( sbSizerRegFormula->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulFormula->Wrap( -1 );
	m_RegulFormula->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	sbSizerRegFormula->Add( m_RegulFormula, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizeLeftpReg->Add( sbSizerRegFormula, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerColBalancer->Add( bSizeLeftpReg, 0, wxTOP|wxRIGHT|wxEXPAND, 6 );


	bSizerColBalancer->Add( 10, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerRegulRight;
	bSizerRegulRight = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerRegulatorsChooser;
	sbSizerRegulatorsChooser = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Regulator") ), wxVERTICAL );

	wxArrayString m_choiceRegulatorSelectorChoices;
	m_choiceRegulatorSelector = new wxChoice( sbSizerRegulatorsChooser->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRegulatorSelectorChoices, 0 );
	m_choiceRegulatorSelector->SetSelection( 0 );
	sbSizerRegulatorsChooser->Add( m_choiceRegulatorSelector, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticTextRegFile = new wxStaticText( sbSizerRegulatorsChooser->GetStaticBox(), wxID_ANY, _("Regulators data file:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRegFile->Wrap( -1 );
	m_staticTextRegFile->SetToolTip( _("The name of the data file which stores known regulators parameters.") );

	sbSizerRegulatorsChooser->Add( m_staticTextRegFile, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerDataFile;
	bSizerDataFile = new wxBoxSizer( wxHORIZONTAL );

	m_regulators_fileNameCtrl = new wxTextCtrl( sbSizerRegulatorsChooser->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDataFile->Add( m_regulators_fileNameCtrl, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_buttonDataFile = new wxButton( sbSizerRegulatorsChooser->GetStaticBox(), wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDataFile->Add( m_buttonDataFile, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbSizerRegulatorsChooser->Add( bSizerDataFile, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerReulBtn;
	bSizerReulBtn = new wxBoxSizer( wxHORIZONTAL );

	m_buttonEditItem = new wxButton( sbSizerRegulatorsChooser->GetStaticBox(), wxID_ANY, _("Edit Regulator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonEditItem->SetToolTip( _("Edit the current selected regulator.") );

	bSizerReulBtn->Add( m_buttonEditItem, 1, wxALL, 5 );

	m_buttonAddItem = new wxButton( sbSizerRegulatorsChooser->GetStaticBox(), wxID_ANY, _("Add Regulator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAddItem->SetToolTip( _("Enter a new item to the current list of available regulators") );

	bSizerReulBtn->Add( m_buttonAddItem, 1, wxALL, 5 );

	m_buttonRemoveItem = new wxButton( sbSizerRegulatorsChooser->GetStaticBox(), wxID_ANY, _("Remove Regulator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRemoveItem->SetToolTip( _("Remove an item from the current list of available regulators") );

	bSizerReulBtn->Add( m_buttonRemoveItem, 1, wxALL, 5 );


	sbSizerRegulatorsChooser->Add( bSizerReulBtn, 1, wxEXPAND, 5 );


	bSizer9->Add( sbSizerRegulatorsChooser, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizer9->Add( 0, 10, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerRegParams;
	fgSizerRegParams = new wxFlexGridSizer( 7, 6, 4, 0 );
	fgSizerRegParams->AddGrowableCol( 1 );
	fgSizerRegParams->SetFlexibleDirection( wxBOTH );
	fgSizerRegParams->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );

	m_labelValMin = new wxStaticText( this, wxID_ANY, _("min"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelValMin->Wrap( -1 );
	fgSizerRegParams->Add( m_labelValMin, 0, wxALIGN_CENTER|wxALL, 5 );

	m_labelValTyp = new wxStaticText( this, wxID_ANY, _("typ"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelValTyp->Wrap( -1 );
	fgSizerRegParams->Add( m_labelValTyp, 0, wxALIGN_CENTER|wxALL, 5 );

	m_labelValMax = new wxStaticText( this, wxID_ANY, _("max"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelValMax->Wrap( -1 );
	fgSizerRegParams->Add( m_labelValMax, 0, wxALIGN_CENTER|wxALL, 5 );


	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );

	m_rbRegulR1 = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizerRegParams->Add( m_rbRegulR1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_labelRegultR1 = new wxStaticText( this, wxID_ANY, _("R1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelRegultR1->Wrap( -1 );
	fgSizerRegParams->Add( m_labelRegultR1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_r1MinVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_r1MinVal->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	fgSizerRegParams->Add( m_r1MinVal, 0, wxALL|wxEXPAND, 5 );

	m_r1TypVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_r1TypVal, 0, wxALL|wxEXPAND, 5 );

	m_r1MaxVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_r1MaxVal->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	fgSizerRegParams->Add( m_r1MaxVal, 0, wxALL|wxEXPAND, 5 );

	m_labelUnitsR1 = new wxStaticText( this, wxID_ANY, _("kΩ"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelUnitsR1->Wrap( -1 );
	fgSizerRegParams->Add( m_labelUnitsR1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_rbRegulR2 = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_rbRegulR2->SetValue( true );
	fgSizerRegParams->Add( m_rbRegulR2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_labelRegultR2 = new wxStaticText( this, wxID_ANY, _("R2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelRegultR2->Wrap( -1 );
	fgSizerRegParams->Add( m_labelRegultR2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_r2MinVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_r2MinVal->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	fgSizerRegParams->Add( m_r2MinVal, 0, wxALL|wxEXPAND, 5 );

	m_r2TypVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_r2TypVal, 0, wxALL|wxEXPAND, 5 );

	m_r2MaxVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_r2MaxVal->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	fgSizerRegParams->Add( m_r2MaxVal, 0, wxALL|wxEXPAND, 5 );

	m_labelUnitsR2 = new wxStaticText( this, wxID_ANY, _("kΩ"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelUnitsR2->Wrap( -1 );
	fgSizerRegParams->Add( m_labelUnitsR2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_rbRegulVout = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_rbRegulVout, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_labelVout = new wxStaticText( this, wxID_ANY, _("Vout:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelVout->Wrap( -1 );
	fgSizerRegParams->Add( m_labelVout, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_voutMinVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_voutMinVal->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	fgSizerRegParams->Add( m_voutMinVal, 0, wxALL|wxEXPAND, 5 );

	m_voutTypVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_voutTypVal, 0, wxALL|wxEXPAND, 5 );

	m_voutMaxVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_voutMaxVal->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	fgSizerRegParams->Add( m_voutMaxVal, 0, wxALL|wxEXPAND, 5 );

	m_labelUnitsVout = new wxStaticText( this, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelUnitsVout->Wrap( -1 );
	fgSizerRegParams->Add( m_labelUnitsVout, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );

	m_labelVRef = new wxStaticText( this, wxID_ANY, _("Vref:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelVRef->Wrap( -1 );
	m_labelVRef->SetToolTip( _("The internal reference voltage of the regulator.\nShould not be 0.") );

	fgSizerRegParams->Add( m_labelVRef, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_vrefMinVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_vrefMinVal, 0, wxALL|wxEXPAND, 5 );

	m_vrefTypVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_vrefTypVal, 0, wxALL|wxEXPAND, 5 );

	m_vrefMaxVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_vrefMaxVal, 0, wxALL|wxEXPAND, 5 );

	m_labelUnitsVref = new wxStaticText( this, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelUnitsVref->Wrap( -1 );
	fgSizerRegParams->Add( m_labelUnitsVref, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );

	m_RegulIadjTitle = new wxStaticText( this, wxID_ANY, _("Iadj:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulIadjTitle->Wrap( -1 );
	m_RegulIadjTitle->SetToolTip( _("For 3 terminal regulators only, the  Adjust pin current.") );

	fgSizerRegParams->Add( m_RegulIadjTitle, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );

	m_iadjTypVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_iadjTypVal, 0, wxALL|wxEXPAND, 5 );

	m_iadjMaxVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_iadjMaxVal, 0, wxALL|wxEXPAND, 5 );

	m_labelUnitsIadj = new wxStaticText( this, wxID_ANY, _("uA"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelUnitsIadj->Wrap( -1 );
	fgSizerRegParams->Add( m_labelUnitsIadj, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );

	m_labelTolSumary = new wxStaticText( this, wxID_ANY, _("Overall tolerance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelTolSumary->Wrap( -1 );
	fgSizerRegParams->Add( m_labelTolSumary, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_tolTotalMin = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_tolTotalMin->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	fgSizerRegParams->Add( m_tolTotalMin, 0, wxALL, 5 );


	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );

	m_TolTotalMax = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_TolTotalMax->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	fgSizerRegParams->Add( m_TolTotalMax, 0, wxALL, 5 );

	m_labelTotalPercent = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelTotalPercent->Wrap( -1 );
	fgSizerRegParams->Add( m_labelTotalPercent, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizer9->Add( fgSizerRegParams, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 10 );

	wxBoxSizer* bSizerResTol;
	bSizerResTol = new wxBoxSizer( wxHORIZONTAL );

	m_labelResTol = new wxStaticText( this, wxID_ANY, _("Resistor tolerance:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_labelResTol->Wrap( -1 );
	bSizerResTol->Add( m_labelResTol, 0, wxALIGN_CENTER|wxALL, 5 );


	bSizerResTol->Add( 0, 0, 1, 0, 5 );

	m_resTolVal = new wxTextCtrl( this, wxID_ANY, _("1"), wxDefaultPosition, wxSize( 45,-1 ), 0 );
	bSizerResTol->Add( m_resTolVal, 0, wxALIGN_CENTER|wxALL, 5 );

	m_labelResTolUnit = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelResTolUnit->Wrap( -1 );
	bSizerResTol->Add( m_labelResTolUnit, 0, wxALIGN_CENTER|wxALL, 5 );


	bSizer9->Add( bSizerResTol, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizerComment;
	bSizerComment = new wxBoxSizer( wxHORIZONTAL );

	m_labelKicadComment = new wxStaticText( this, wxID_ANY, _("Power Comment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelKicadComment->Wrap( -1 );
	bSizerComment->Add( m_labelKicadComment, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textPowerComment = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), wxTE_CENTER|wxTE_READONLY );
	m_textPowerComment->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	bSizerComment->Add( m_textPowerComment, 0, wxALL, 5 );


	bSizerComment->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btCopyCB = new wxButton( this, wxID_ANY, _("Copy to Clipboard"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerComment->Add( m_btCopyCB, 0, wxALL, 5 );


	bSizer9->Add( bSizerComment, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizerRegulRight->Add( bSizer9, 0, 0, 5 );

	m_RegulMessage = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulMessage->Wrap( -1 );
	bSizerRegulRight->Add( m_RegulMessage, 0, wxALL|wxEXPAND, 10 );

	wxBoxSizer* bSizerRegulButtonCalcReset;
	bSizerRegulButtonCalcReset = new wxBoxSizer( wxHORIZONTAL );

	m_buttonCalculate = new wxButton( this, wxID_ANY, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonCalculate->SetMinSize( wxSize( 120,-1 ) );

	bSizerRegulButtonCalcReset->Add( m_buttonCalculate, 0, wxALL|wxTOP, 5 );


	bSizerRegulButtonCalcReset->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizerRegulRight->Add( bSizerRegulButtonCalcReset, 0, wxEXPAND|wxLEFT, 5 );


	bSizerRegulRight->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonRegulReset = new wxButton( this, wxID_ANY, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRegulRight->Add( m_buttonRegulReset, 0, wxALIGN_RIGHT|wxTOP|wxBOTTOM|wxRIGHT, 10 );


	bSizerColBalancer->Add( bSizerRegulRight, 1, wxEXPAND|wxLEFT, 10 );


	bSizerMainReg->Add( bSizerColBalancer, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerMainReg );
	this->Layout();
	bSizerMainReg->Fit( this );

	// Connect Events
	m_choiceRegType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnRegulTypeSelection ), NULL, this );
	m_choiceRegulatorSelector->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnRegulatorSelection ), NULL, this );
	m_buttonDataFile->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnDataFileSelection ), NULL, this );
	m_buttonEditItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnEditRegulator ), NULL, this );
	m_buttonAddItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnAddRegulator ), NULL, this );
	m_buttonRemoveItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnRemoveRegulator ), NULL, this );
	m_btCopyCB->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnCopyCB ), NULL, this );
	m_buttonCalculate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnRegulatorCalcButtonClick ), NULL, this );
	m_buttonRegulReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnRegulatorResetButtonClick ), NULL, this );
}

PANEL_REGULATOR_BASE::~PANEL_REGULATOR_BASE()
{
	// Disconnect Events
	m_choiceRegType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnRegulTypeSelection ), NULL, this );
	m_choiceRegulatorSelector->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnRegulatorSelection ), NULL, this );
	m_buttonDataFile->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnDataFileSelection ), NULL, this );
	m_buttonEditItem->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnEditRegulator ), NULL, this );
	m_buttonAddItem->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnAddRegulator ), NULL, this );
	m_buttonRemoveItem->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnRemoveRegulator ), NULL, this );
	m_btCopyCB->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnCopyCB ), NULL, this );
	m_buttonCalculate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnRegulatorCalcButtonClick ), NULL, this );
	m_buttonRegulReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_REGULATOR_BASE::OnRegulatorResetButtonClick ), NULL, this );

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/unit_selector.h"

#include "pcb_calculator_frame_base.h"

///////////////////////////////////////////////////////////////////////////

PCB_CALCULATOR_FRAME_BASE::PCB_CALCULATOR_FRAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : KIWAY_PLAYER( parent, id, title, pos, size, style, name )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_menubar = new wxMenuBar( 0 );
	this->SetMenuBar( m_menubar );

	wxBoxSizer* bmainFrameSizer;
	bmainFrameSizer = new wxBoxSizer( wxVERTICAL );

	m_Notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelRegulators = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerMainReg;
	bSizerMainReg = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerColBalancer;
	bSizerColBalancer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizeLeftpReg;
	bSizeLeftpReg = new wxBoxSizer( wxVERTICAL );

	bSizeLeftpReg->SetMinSize( wxSize( 400,-1 ) );
	wxBoxSizer* bSizerType;
	bSizerType = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextRegType = new wxStaticText( m_panelRegulators, wxID_ANY, _("Type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRegType->Wrap( -1 );
	m_staticTextRegType->SetToolTip( _("Type of the regulator.\nThere are 2 types:\n- regulators which have a dedicated sense pin for the voltage regulation.\n- 3 terminal pins.") );

	bSizerType->Add( m_staticTextRegType, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_choiceRegTypeChoices[] = { _("Standard Type"), _("3 Terminal Type") };
	int m_choiceRegTypeNChoices = sizeof( m_choiceRegTypeChoices ) / sizeof( wxString );
	m_choiceRegType = new wxChoice( m_panelRegulators, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRegTypeNChoices, m_choiceRegTypeChoices, 0 );
	m_choiceRegType->SetSelection( 0 );
	bSizerType->Add( m_choiceRegType, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizeLeftpReg->Add( bSizerType, 0, wxEXPAND|wxALL, 5 );


	bSizeLeftpReg->Add( 0, 10, 0, wxEXPAND, 5 );

	m_bitmapRegul4pins = new wxStaticBitmap( m_panelRegulators, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizeLeftpReg->Add( m_bitmapRegul4pins, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 10 );

	m_bitmapRegul3pins = new wxStaticBitmap( m_panelRegulators, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizeLeftpReg->Add( m_bitmapRegul3pins, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 10 );


	bSizeLeftpReg->Add( 0, 0, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerRegFormula;
	sbSizerRegFormula = new wxStaticBoxSizer( new wxStaticBox( m_panelRegulators, wxID_ANY, _("Formula") ), wxVERTICAL );

	m_RegulFormula = new wxStaticText( sbSizerRegFormula->GetStaticBox(), wxID_ANY, _("Vout = Vref * (R1 + R2) / R2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulFormula->Wrap( -1 );
	m_RegulFormula->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	sbSizerRegFormula->Add( m_RegulFormula, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizeLeftpReg->Add( sbSizerRegFormula, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerColBalancer->Add( bSizeLeftpReg, 0, wxTOP|wxRIGHT|wxEXPAND, 6 );


	bSizerColBalancer->Add( 10, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerRegulRight;
	bSizerRegulRight = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerRegParams;
	fgSizerRegParams = new wxFlexGridSizer( 6, 4, 4, 0 );
	fgSizerRegParams->AddGrowableCol( 2 );
	fgSizerRegParams->SetFlexibleDirection( wxBOTH );
	fgSizerRegParams->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rbRegulR1 = new wxRadioButton( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbRegulR1->SetValue( true );
	fgSizerRegParams->Add( m_rbRegulR1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_labelRegultR1 = new wxStaticText( m_panelRegulators, wxID_ANY, _("R1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelRegultR1->Wrap( -1 );
	fgSizerRegParams->Add( m_labelRegultR1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_RegulR1Value = new wxTextCtrl( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_RegulR1Value, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_r1Units = new wxStaticText( m_panelRegulators, wxID_ANY, _("kOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_r1Units->Wrap( -1 );
	fgSizerRegParams->Add( m_r1Units, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_rbRegulR2 = new wxRadioButton( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_rbRegulR2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_labelRegultR2 = new wxStaticText( m_panelRegulators, wxID_ANY, _("R2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelRegultR2->Wrap( -1 );
	fgSizerRegParams->Add( m_labelRegultR2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_RegulR2Value = new wxTextCtrl( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_RegulR2Value, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_r2Units = new wxStaticText( m_panelRegulators, wxID_ANY, _("kOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_r2Units->Wrap( -1 );
	fgSizerRegParams->Add( m_r2Units, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_rbRegulVout = new wxRadioButton( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_rbRegulVout, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_labelVout = new wxStaticText( m_panelRegulators, wxID_ANY, _("Vout:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelVout->Wrap( -1 );
	fgSizerRegParams->Add( m_labelVout, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_RegulVoutValue = new wxTextCtrl( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_RegulVoutValue, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_unitsVout = new wxStaticText( m_panelRegulators, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsVout->Wrap( -1 );
	fgSizerRegParams->Add( m_unitsVout, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );

	m_labelVRef = new wxStaticText( m_panelRegulators, wxID_ANY, _("Vref:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelVRef->Wrap( -1 );
	m_labelVRef->SetToolTip( _("The internal reference voltage of the regulator.\nShould not be 0.") );

	fgSizerRegParams->Add( m_labelVRef, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_RegulVrefValue = new wxTextCtrl( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_RegulVrefValue, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_unitsVref = new wxStaticText( m_panelRegulators, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsVref->Wrap( -1 );
	fgSizerRegParams->Add( m_unitsVref, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );

	m_RegulIadjTitle = new wxStaticText( m_panelRegulators, wxID_ANY, _("Iadj:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulIadjTitle->Wrap( -1 );
	m_RegulIadjTitle->SetToolTip( _("For 3 terminal regulators only, the  Adjust pin current.") );

	fgSizerRegParams->Add( m_RegulIadjTitle, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_RegulIadjValue = new wxTextCtrl( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_RegulIadjValue, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_IadjUnitLabel = new wxStaticText( m_panelRegulators, wxID_ANY, _("uA"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IadjUnitLabel->Wrap( -1 );
	fgSizerRegParams->Add( m_IadjUnitLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerRegulRight->Add( fgSizerRegParams, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bSizerRegulButtonCalcReset;
	bSizerRegulButtonCalcReset = new wxBoxSizer( wxHORIZONTAL );


	bSizerRegulButtonCalcReset->Add( 0, 0, 3, wxEXPAND, 5 );

	m_buttonCalculate = new wxButton( m_panelRegulators, wxID_ANY, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRegulButtonCalcReset->Add( m_buttonCalculate, 6, wxTOP, 10 );


	bSizerRegulButtonCalcReset->Add( 0, 0, 2, wxEXPAND, 5 );

	m_buttonRegulReset = new wxButton( m_panelRegulators, wxID_ANY, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRegulButtonCalcReset->Add( m_buttonRegulReset, 6, wxTOP, 10 );


	bSizerRegulButtonCalcReset->Add( 0, 0, 3, wxEXPAND, 5 );


	bSizerRegulRight->Add( bSizerRegulButtonCalcReset, 0, wxEXPAND|wxLEFT, 30 );

	m_RegulMessage = new wxStaticText( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulMessage->Wrap( -1 );
	bSizerRegulRight->Add( m_RegulMessage, 0, wxALL, 10 );

	wxStaticBoxSizer* sbSizerRegulatorsChooser;
	sbSizerRegulatorsChooser = new wxStaticBoxSizer( new wxStaticBox( m_panelRegulators, wxID_ANY, _("Regulator") ), wxVERTICAL );

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


	bSizerRegulRight->Add( sbSizerRegulatorsChooser, 0, wxEXPAND|wxRIGHT, 10 );


	bSizerColBalancer->Add( bSizerRegulRight, 1, wxEXPAND|wxTOP|wxLEFT, 10 );


	bSizerMainReg->Add( bSizerColBalancer, 0, wxEXPAND, 5 );


	m_panelRegulators->SetSizer( bSizerMainReg );
	m_panelRegulators->Layout();
	bSizerMainReg->Fit( m_panelRegulators );
	m_Notebook->AddPage( m_panelRegulators, _("Regulators"), false );
	m_panelAttenuators = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerAtt;
	bSizerAtt = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizerAtt;
	bLeftSizerAtt = new wxBoxSizer( wxVERTICAL );

	bLeftSizerAtt->SetMinSize( wxSize( 260,-1 ) );
	wxString m_AttenuatorsSelectionChoices[] = { _("PI"), _("Tee"), _("Bridged tee"), _("Resistive splitter") };
	int m_AttenuatorsSelectionNChoices = sizeof( m_AttenuatorsSelectionChoices ) / sizeof( wxString );
	m_AttenuatorsSelection = new wxRadioBox( m_panelAttenuators, wxID_ANY, _("Attenuators"), wxDefaultPosition, wxDefaultSize, m_AttenuatorsSelectionNChoices, m_AttenuatorsSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_AttenuatorsSelection->SetSelection( 2 );
	bLeftSizerAtt->Add( m_AttenuatorsSelection, 0, wxEXPAND|wxALL, 5 );


	bLeftSizerAtt->Add( 0, 5, 0, wxEXPAND, 5 );

	m_attenuatorBitmap = new wxStaticBitmap( m_panelAttenuators, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizerAtt->Add( m_attenuatorBitmap, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 10 );


	bSizerAtt->Add( bLeftSizerAtt, 0, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bMiddleSizerAtt;
	bMiddleSizerAtt = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerAttPrms;
	sbSizerAttPrms = new wxStaticBoxSizer( new wxStaticBox( m_panelAttenuators, wxID_ANY, _("Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerAttPrms;
	fgSizerAttPrms = new wxFlexGridSizer( 3, 3, 3, 0 );
	fgSizerAttPrms->AddGrowableRow( 1 );
	fgSizerAttPrms->SetFlexibleDirection( wxBOTH );
	fgSizerAttPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_attenuationLabel = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("Attenuation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuationLabel->Wrap( -1 );
	fgSizerAttPrms->Add( m_attenuationLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_AttValueCtrl = new wxTextCtrl( sbSizerAttPrms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms->Add( m_AttValueCtrl, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_attUnit = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("dB"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attUnit->Wrap( -1 );
	fgSizerAttPrms->Add( m_attUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_attenuationZinLabel = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("Zin:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuationZinLabel->Wrap( -1 );
	fgSizerAttPrms->Add( m_attenuationZinLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ZinValueCtrl = new wxTextCtrl( sbSizerAttPrms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms->Add( m_ZinValueCtrl, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_attZinUnit = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attZinUnit->Wrap( -1 );
	fgSizerAttPrms->Add( m_attZinUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ZoutLabel = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("Zout:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ZoutLabel->Wrap( -1 );
	fgSizerAttPrms->Add( m_ZoutLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ZoutValueCtrl = new wxTextCtrl( sbSizerAttPrms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms->Add( m_ZoutValueCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_attZoutUnit = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attZoutUnit->Wrap( -1 );
	fgSizerAttPrms->Add( m_attZoutUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerAttPrms->Add( fgSizerAttPrms, 0, wxEXPAND|wxBOTTOM, 5 );


	bMiddleSizerAtt->Add( sbSizerAttPrms, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bSizerAttButt;
	bSizerAttButt = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAlcAtt = new wxButton( m_panelAttenuators, wxID_ANY, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerAttButt->Add( m_buttonAlcAtt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_bpButtonCalcAtt = new wxBitmapButton( m_panelAttenuators, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerAttButt->Add( m_bpButtonCalcAtt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bMiddleSizerAtt->Add( bSizerAttButt, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

	wxStaticBoxSizer* sbSizerAttValues;
	sbSizerAttValues = new wxStaticBoxSizer( new wxStaticBox( m_panelAttenuators, wxID_ANY, _("Values") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerAttResults;
	fgSizerAttResults = new wxFlexGridSizer( 3, 3, 3, 0 );
	fgSizerAttResults->AddGrowableCol( 1 );
	fgSizerAttResults->SetFlexibleDirection( wxBOTH );
	fgSizerAttResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_attenuatorR1Label = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("R1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuatorR1Label->Wrap( -1 );
	fgSizerAttResults->Add( m_attenuatorR1Label, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_Att_R1_Value = new wxTextCtrl( sbSizerAttValues->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttResults->Add( m_Att_R1_Value, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_attR1Unit = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attR1Unit->Wrap( -1 );
	fgSizerAttResults->Add( m_attR1Unit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_attenuatorR2Label = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("R2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuatorR2Label->Wrap( -1 );
	fgSizerAttResults->Add( m_attenuatorR2Label, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_Att_R2_Value = new wxTextCtrl( sbSizerAttValues->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttResults->Add( m_Att_R2_Value, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_attR2Unit = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attR2Unit->Wrap( -1 );
	fgSizerAttResults->Add( m_attR2Unit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_attenuatorR3Label = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("R3:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuatorR3Label->Wrap( -1 );
	fgSizerAttResults->Add( m_attenuatorR3Label, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_Att_R3_Value = new wxTextCtrl( sbSizerAttValues->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttResults->Add( m_Att_R3_Value, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_attR3Unit = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attR3Unit->Wrap( -1 );
	fgSizerAttResults->Add( m_attR3Unit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerAttValues->Add( fgSizerAttResults, 0, wxEXPAND|wxBOTTOM, 5 );


	bMiddleSizerAtt->Add( sbSizerAttValues, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bSizerMessages;
	bSizerMessages = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerIndentLabel;
	bSizerIndentLabel = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextAttMsg = new wxStaticText( m_panelAttenuators, wxID_ANY, _("Messages"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAttMsg->Wrap( -1 );
	bSizerIndentLabel->Add( m_staticTextAttMsg, 0, wxALL, 2 );


	bSizerMessages->Add( bSizerIndentLabel, 0, wxLEFT, 6 );

	m_Attenuator_Messages = new wxHtmlWindow( m_panelAttenuators, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_NO_SELECTION|wxHW_SCROLLBAR_AUTO );
	bSizerMessages->Add( m_Attenuator_Messages, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 8 );


	bMiddleSizerAtt->Add( bSizerMessages, 1, wxEXPAND|wxLEFT, 3 );


	bSizerAtt->Add( bMiddleSizerAtt, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbRightSizerFormula;
	sbRightSizerFormula = new wxStaticBoxSizer( new wxStaticBox( m_panelAttenuators, wxID_ANY, _("Formula") ), wxVERTICAL );

	m_panelAttFormula = new wxHtmlWindow( sbRightSizerFormula->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	sbRightSizerFormula->Add( m_panelAttFormula, 1, wxEXPAND|wxBOTTOM, 5 );


	bSizerAtt->Add( sbRightSizerFormula, 1, wxEXPAND|wxALL, 5 );


	m_panelAttenuators->SetSizer( bSizerAtt );
	m_panelAttenuators->Layout();
	bSizerAtt->Fit( m_panelAttenuators );
	m_Notebook->AddPage( m_panelAttenuators, _("RF Attenuators"), true );
	m_panelESeries = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerESerie;
	bSizerESerie = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMiddleSizerESeries;
	bMiddleSizerESeries = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerESeriesInput;
	sbSizerESeriesInput = new wxStaticBoxSizer( new wxStaticBox( m_panelESeries, wxID_ANY, _("Inputs") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerAttPrms1;
	fgSizerAttPrms1 = new wxFlexGridSizer( 4, 3, 3, 0 );
	fgSizerAttPrms1->AddGrowableRow( 1 );
	fgSizerAttPrms1->SetFlexibleDirection( wxBOTH );
	fgSizerAttPrms1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ESrequired = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("Required resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESrequired->Wrap( -1 );
	fgSizerAttPrms1->Add( m_ESrequired, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_ResRequired = new wxTextCtrl( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms1->Add( m_ResRequired, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_reqResUnits = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("kOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_reqResUnits->Wrap( -1 );
	fgSizerAttPrms1->Add( m_reqResUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESrequired1 = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("Exclude value 1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESrequired1->Wrap( -1 );
	fgSizerAttPrms1->Add( m_ESrequired1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ResExclude1 = new wxTextCtrl( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms1->Add( m_ResExclude1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_exclude1Units = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("kOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_exclude1Units->Wrap( -1 );
	fgSizerAttPrms1->Add( m_exclude1Units, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESrequired11 = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("Exclude value 2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESrequired11->Wrap( -1 );
	fgSizerAttPrms1->Add( m_ESrequired11, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ResExclude2 = new wxTextCtrl( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms1->Add( m_ResExclude2, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_exclude2Units = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("kOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_exclude2Units->Wrap( -1 );
	fgSizerAttPrms1->Add( m_exclude2Units, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerESeriesInput->Add( fgSizerAttPrms1, 0, wxEXPAND|wxBOTTOM, 5 );

	m_staticline6 = new wxStaticLine( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizerESeriesInput->Add( m_staticline6, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bSizer40;
	bSizer40 = new wxBoxSizer( wxHORIZONTAL );

	m_e1 = new wxRadioButton( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("E1"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer40->Add( m_e1, 1, wxALL, 5 );

	m_e3 = new wxRadioButton( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("E3"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer40->Add( m_e3, 1, wxALL, 5 );

	m_e6 = new wxRadioButton( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("E6"), wxDefaultPosition, wxDefaultSize, 0 );
	m_e6->SetValue( true );
	bSizer40->Add( m_e6, 1, wxALL, 5 );

	m_e12 = new wxRadioButton( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("E12"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer40->Add( m_e12, 1, wxALL, 5 );


	sbSizerESeriesInput->Add( bSizer40, 1, wxEXPAND, 5 );


	bMiddleSizerESeries->Add( sbSizerESeriesInput, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerESeriesSolutions;
	sbSizerESeriesSolutions = new wxStaticBoxSizer( new wxStaticBox( m_panelESeries, wxID_ANY, _("Solutions") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerESerieResults;
	fgSizerESerieResults = new wxFlexGridSizer( 6, 5, 3, 0 );
	fgSizerESerieResults->AddGrowableCol( 1 );
	fgSizerESerieResults->SetFlexibleDirection( wxBOTH );
	fgSizerESerieResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ESerieSimpleSolution = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Simple solution:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESerieSimpleSolution->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESerieSimpleSolution, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeries_Sol2R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries_Sol2R->SetMinSize( wxSize( 150,-1 ) );

	fgSizerESerieResults->Add( m_ESeries_Sol2R, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_ESeriesSimpleErr = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Error:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesSimpleErr->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesSimpleErr, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeriesError2R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESerieResults->Add( m_ESeriesError2R, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ESeriesSimplePercent = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesSimplePercent->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesSimplePercent, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESerie3RSolution1 = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("3R solution:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESerie3RSolution1->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESerie3RSolution1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeries_Sol3R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries_Sol3R->SetMinSize( wxSize( 220,-1 ) );

	fgSizerESerieResults->Add( m_ESeries_Sol3R, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_ESeriesAltErr = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Error:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltErr->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesAltErr, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeriesError3R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESerieResults->Add( m_ESeriesError3R, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_ESeriesAltPercent = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltPercent->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesAltPercent, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESeries4RSolution = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("4R solution:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries4RSolution->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeries4RSolution, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeries_Sol4R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries_Sol4R->SetMinSize( wxSize( 290,-1 ) );

	fgSizerESerieResults->Add( m_ESeries_Sol4R, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxEXPAND, 5 );

	m_ESeriesAltErr1 = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Error:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltErr1->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesAltErr1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeriesError4R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESerieResults->Add( m_ESeriesError4R, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeriesAltPercent1 = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltPercent1->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesAltPercent1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerESeriesSolutions->Add( fgSizerESerieResults, 0, wxBOTTOM|wxEXPAND, 5 );

	m_staticline7 = new wxStaticLine( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizerESeriesSolutions->Add( m_staticline7, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_buttonEScalculate = new wxButton( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerESeriesSolutions->Add( m_buttonEScalculate, 0, wxALL, 5 );


	bMiddleSizerESeries->Add( sbSizerESeriesSolutions, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer47;
	bSizer47 = new wxBoxSizer( wxVERTICAL );


	bMiddleSizerESeries->Add( bSizer47, 1, wxALIGN_BOTTOM, 5 );


	bSizerESerie->Add( bMiddleSizerESeries, 0, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bLowerESerie;
	bLowerESerie = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbLowerSizerEseriesHelp;
	sbLowerSizerEseriesHelp = new wxStaticBoxSizer( new wxStaticBox( m_panelESeries, wxID_ANY, _("Help") ), wxVERTICAL );

	m_panelESeriesHelp = new wxHtmlWindow( sbLowerSizerEseriesHelp->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	sbLowerSizerEseriesHelp->Add( m_panelESeriesHelp, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bLowerESerie->Add( sbLowerSizerEseriesHelp, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerESerie->Add( bLowerESerie, 1, wxEXPAND, 5 );


	m_panelESeries->SetSizer( bSizerESerie );
	m_panelESeries->Layout();
	bSizerESerie->Fit( m_panelESeries );
	m_Notebook->AddPage( m_panelESeries, _("E-Series"), false );
	m_panelColorCode = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanelColorCode;
	bSizerPanelColorCode = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer38;
	bSizer38 = new wxBoxSizer( wxVERTICAL );

	wxString m_rbToleranceSelectionChoices[] = { _("10% / 5%"), _("<= 2%") };
	int m_rbToleranceSelectionNChoices = sizeof( m_rbToleranceSelectionChoices ) / sizeof( wxString );
	m_rbToleranceSelection = new wxRadioBox( m_panelColorCode, wxID_ANY, _("Tolerance"), wxDefaultPosition, wxDefaultSize, m_rbToleranceSelectionNChoices, m_rbToleranceSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbToleranceSelection->SetSelection( 0 );
	bSizer38->Add( m_rbToleranceSelection, 0, wxBOTTOM|wxRIGHT, 30 );


	bSizerPanelColorCode->Add( bSizer38, 0, wxALL, 8 );

	wxFlexGridSizer* fgSizerColoCode;
	fgSizerColoCode = new wxFlexGridSizer( 2, 6, 0, 0 );
	fgSizerColoCode->SetFlexibleDirection( wxBOTH );
	fgSizerColoCode->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText31 = new wxStaticText( m_panelColorCode, wxID_ANY, _("1st Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	fgSizerColoCode->Add( m_staticText31, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticText34 = new wxStaticText( m_panelColorCode, wxID_ANY, _("2nd Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText34->Wrap( -1 );
	fgSizerColoCode->Add( m_staticText34, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticText35 = new wxStaticText( m_panelColorCode, wxID_ANY, _("3rd Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText35->Wrap( -1 );
	fgSizerColoCode->Add( m_staticText35, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_Band4Label = new wxStaticText( m_panelColorCode, wxID_ANY, _("4th Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Band4Label->Wrap( -1 );
	fgSizerColoCode->Add( m_Band4Label, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticText37 = new wxStaticText( m_panelColorCode, wxID_ANY, _("Multiplier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText37->Wrap( -1 );
	fgSizerColoCode->Add( m_staticText37, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticText38 = new wxStaticText( m_panelColorCode, wxID_ANY, _("Tolerance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText38->Wrap( -1 );
	fgSizerColoCode->Add( m_staticText38, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_Band1bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band1bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_Band2bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band2bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_Band3bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band3bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_Band4bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band4bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	m_Band_mult_bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band_mult_bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	m_Band_tol_bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band_tol_bitmap, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


	bSizerPanelColorCode->Add( fgSizerColoCode, 1, wxEXPAND|wxLEFT, 5 );


	m_panelColorCode->SetSizer( bSizerPanelColorCode );
	m_panelColorCode->Layout();
	bSizerPanelColorCode->Fit( m_panelColorCode );
	m_Notebook->AddPage( m_panelColorCode, _("Color Code"), false );
	m_panelTransline = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizeTransline;
	bSizeTransline = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	wxString m_TranslineSelectionChoices[] = { _("Microstrip Line"), _("Coplanar wave guide"), _("Coplanar wave guide w/ ground plane"), _("Rectangular Waveguide"), _("Coaxial Line"), _("Coupled Microstrip Line"), _("Stripline"), _("Twisted Pair") };
	int m_TranslineSelectionNChoices = sizeof( m_TranslineSelectionChoices ) / sizeof( wxString );
	m_TranslineSelection = new wxRadioBox( m_panelTransline, wxID_ANY, _("Transmission Line Type"), wxDefaultPosition, wxDefaultSize, m_TranslineSelectionNChoices, m_TranslineSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_TranslineSelection->SetSelection( 2 );
	bLeftSizer->Add( m_TranslineSelection, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	bLeftSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	m_translineBitmap = new wxStaticBitmap( m_panelTransline, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_translineBitmap, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 10 );


	bSizeTransline->Add( bLeftSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSubstrateBoxSizer;
	sbSubstrateBoxSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Substrate Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerSubstPrms;
	fgSizerSubstPrms = new wxFlexGridSizer( 9, 3, 3, 0 );
	fgSizerSubstPrms->AddGrowableCol( 1 );
	fgSizerSubstPrms->SetFlexibleDirection( wxBOTH );
	fgSizerSubstPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_EpsilonR_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Er:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EpsilonR_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_EpsilonR_label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_Value_EpsilonR = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Value_EpsilonR, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_button_EpsilonR = new wxButton( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizerSubstPrms->Add( m_button_EpsilonR, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_TanD_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("TanD:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TanD_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_TanD_label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_Value_TanD = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Value_TanD, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_button_TanD = new wxButton( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizerSubstPrms->Add( m_button_TanD, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Rho_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Rho:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Rho_label->Wrap( -1 );
	m_Rho_label->SetToolTip( _("Specific resistance in ohms * meters") );

	fgSizerSubstPrms->Add( m_Rho_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Value_Rho = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Value_Rho, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_button_Rho = new wxButton( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizerSubstPrms->Add( m_button_Rho, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_substrate_prm4_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("H:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm4_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm4_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Substrate_prm4_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm4_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm4_choiceUnitChoices;
	m_SubsPrm4_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm4_choiceUnitChoices, 0 );
	m_SubsPrm4_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm4_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_substrate_prm5_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("H_t:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm5_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm5_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Substrate_prm5_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm5_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm5_choiceUnitChoices;
	m_SubsPrm5_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm5_choiceUnitChoices, 0 );
	m_SubsPrm5_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm5_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_substrate_prm6_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("T:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm6_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm6_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Substrate_prm6_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm6_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm6_choiceUnitChoices;
	m_SubsPrm6_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm6_choiceUnitChoices, 0 );
	m_SubsPrm6_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm6_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_substrate_prm7_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Rough:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm7_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm7_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Substrate_prm7_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm7_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm7_choiceUnitChoices;
	m_SubsPrm7_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm7_choiceUnitChoices, 0 );
	m_SubsPrm7_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm7_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_substrate_prm8_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("mu insulator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm8_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm8_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Substrate_prm8_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm8_Value, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm8_choiceUnitChoices;
	m_SubsPrm8_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm8_choiceUnitChoices, 0 );
	m_SubsPrm8_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm8_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_substrate_prm9_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("mu conductor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm9_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm9_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Substrate_prm9_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm9_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm9_choiceUnitChoices;
	m_SubsPrm9_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm9_choiceUnitChoices, 0 );
	m_SubsPrm9_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm9_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSubstrateBoxSizer->Add( fgSizerSubstPrms, 1, wxEXPAND|wxBOTTOM, 5 );


	bMiddleSizer->Add( sbSubstrateBoxSizer, 0, wxEXPAND|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbCmpPrmsSizer;
	sbCmpPrmsSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Component Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizeCmpPrms;
	fgSizeCmpPrms = new wxFlexGridSizer( 1, 3, 0, 0 );
	fgSizeCmpPrms->AddGrowableCol( 1 );
	fgSizeCmpPrms->SetFlexibleDirection( wxBOTH );
	fgSizeCmpPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_Frequency_label = new wxStaticText( sbCmpPrmsSizer->GetStaticBox(), wxID_ANY, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Frequency_label->Wrap( -1 );
	fgSizeCmpPrms->Add( m_Frequency_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Value_Frequency_Ctrl = new wxTextCtrl( sbCmpPrmsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeCmpPrms->Add( m_Value_Frequency_Ctrl, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	wxArrayString m_choiceUnit_FrequencyChoices;
	m_choiceUnit_Frequency = new UNIT_SELECTOR_FREQUENCY( sbCmpPrmsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_FrequencyChoices, 0 );
	m_choiceUnit_Frequency->SetSelection( 0 );
	fgSizeCmpPrms->Add( m_choiceUnit_Frequency, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );


	sbCmpPrmsSizer->Add( fgSizeCmpPrms, 0, wxEXPAND|wxBOTTOM, 5 );


	bMiddleSizer->Add( sbCmpPrmsSizer, 0, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizerHelpBitmaps;
	bSizerHelpBitmaps = new wxBoxSizer( wxVERTICAL );

	m_bmCMicrostripZoddZeven = new wxStaticBitmap( m_panelTransline, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerHelpBitmaps->Add( m_bmCMicrostripZoddZeven, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 10 );

	m_fgSizerZcomment = new wxFlexGridSizer( 0, 2, 0, 15 );
	m_fgSizerZcomment->AddGrowableCol( 0 );
	m_fgSizerZcomment->AddGrowableCol( 1 );
	m_fgSizerZcomment->SetFlexibleDirection( wxBOTH );
	m_fgSizerZcomment->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextZdiff = new wxStaticText( m_panelTransline, wxID_ANY, _("Zdiff = Zodd * 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextZdiff->Wrap( -1 );
	m_staticTextZdiff->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgSizerZcomment->Add( m_staticTextZdiff, 0, wxALL, 5 );

	m_staticTextZcommon = new wxStaticText( m_panelTransline, wxID_ANY, _("Zcommon = Zeven / 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextZcommon->Wrap( -1 );
	m_staticTextZcommon->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgSizerZcomment->Add( m_staticTextZcommon, 0, wxALL, 5 );


	bSizerHelpBitmaps->Add( m_fgSizerZcomment, 0, wxEXPAND, 5 );


	bMiddleSizer->Add( bSizerHelpBitmaps, 1, wxALIGN_CENTER_HORIZONTAL, 5 );


	bSizeTransline->Add( bMiddleSizer, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* btranslineRightSizer;
	btranslineRightSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Physical Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerPhysPrms;
	fgSizerPhysPrms = new wxFlexGridSizer( 4, 4, 3, 0 );
	fgSizerPhysPrms->AddGrowableCol( 1 );
	fgSizerPhysPrms->SetFlexibleDirection( wxBOTH );
	fgSizerPhysPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_phys_prm1_label = new wxStaticText( btranslineRightSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm1_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm1_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Phys_prm1_Value = new wxTextCtrl( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_Phys_prm1_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_Param1Choices;
	m_choiceUnit_Param1 = new UNIT_SELECTOR_LEN( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param1Choices, 0 );
	m_choiceUnit_Param1->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param1, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_radioBtnPrm1 = new wxRadioButton( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizerPhysPrms->Add( m_radioBtnPrm1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_phys_prm2_label = new wxStaticText( btranslineRightSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm2_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm2_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Phys_prm2_Value = new wxTextCtrl( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_Phys_prm2_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_Param2Choices;
	m_choiceUnit_Param2 = new UNIT_SELECTOR_LEN( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param2Choices, 0 );
	m_choiceUnit_Param2->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param2, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_radioBtnPrm2 = new wxRadioButton( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_radioBtnPrm2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_phys_prm3_label = new wxStaticText( btranslineRightSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm3_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm3_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Phys_prm3_Value = new wxTextCtrl( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_Phys_prm3_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_Param3Choices;
	m_choiceUnit_Param3 = new UNIT_SELECTOR_LEN( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param3Choices, 0 );
	m_choiceUnit_Param3->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param3, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerPhysPrms->Add( 0, 0, 0, 0, 5 );


	btranslineRightSizer->Add( fgSizerPhysPrms, 0, wxEXPAND|wxBOTTOM, 5 );


	bRightSizer->Add( btranslineRightSizer, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* btranslineButtonsSizer;
	btranslineButtonsSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_bpButtonAnalyze = new wxBitmapButton( m_panelTransline, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerButtons->Add( m_bpButtonAnalyze, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_AnalyseButton = new wxButton( m_panelTransline, wxID_ANY, _("Analyze"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_AnalyseButton, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_SynthetizeButton = new wxButton( m_panelTransline, wxID_ANY, _("Synthesize"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_SynthetizeButton, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_bpButtonSynthetize = new wxBitmapButton( m_panelTransline, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerButtons->Add( m_bpButtonSynthetize, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	btranslineButtonsSizer->Add( bSizerButtons, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


	bRightSizer->Add( btranslineButtonsSizer, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

	wxStaticBoxSizer* sbElectricalResultsSizer;
	sbElectricalResultsSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Electrical Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerResults;
	fgSizerResults = new wxFlexGridSizer( 3, 3, 3, 0 );
	fgSizerResults->AddGrowableCol( 1 );
	fgSizerResults->SetFlexibleDirection( wxBOTH );
	fgSizerResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_elec_prm1_label = new wxStaticText( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm1_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm1_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Elec_prm1_Value = new wxTextCtrl( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerResults->Add( m_Elec_prm1_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_ElecPrm1Choices;
	m_choiceUnit_ElecPrm1 = new UNIT_SELECTOR_RESISTOR( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm1Choices, 0 );
	m_choiceUnit_ElecPrm1->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm1, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_elec_prm2_label = new wxStaticText( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm2_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm2_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Elec_prm2_Value = new wxTextCtrl( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerResults->Add( m_Elec_prm2_Value, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_ElecPrm2Choices;
	m_choiceUnit_ElecPrm2 = new UNIT_SELECTOR_RESISTOR( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm2Choices, 0 );
	m_choiceUnit_ElecPrm2->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm2, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_elec_prm3_label = new wxStaticText( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, _("Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm3_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm3_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Elec_prm3_Value = new wxTextCtrl( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerResults->Add( m_Elec_prm3_Value, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_ElecPrm3Choices;
	m_choiceUnit_ElecPrm3 = new UNIT_SELECTOR_ANGLE( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm3Choices, 0 );
	m_choiceUnit_ElecPrm3->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm3, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbElectricalResultsSizer->Add( fgSizerResults, 0, wxEXPAND|wxBOTTOM, 5 );


	bRightSizer->Add( sbElectricalResultsSizer, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbMessagesSizer;
	sbMessagesSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Results") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerTranslResults;
	fgSizerTranslResults = new wxFlexGridSizer( 7, 2, 0, 0 );
	fgSizerTranslResults->AddGrowableCol( 1 );
	fgSizerTranslResults->SetFlexibleDirection( wxBOTH );
	fgSizerTranslResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_left_message1 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message1->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message1, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message1 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message1->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message1, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message2 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message2->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message2, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message2 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message2->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message2, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_left_message3 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message3->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message3, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message3 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message3->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message3, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message4 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message4->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message4, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message4 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message4->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message4, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message5 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message5->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxALIGN_RIGHT, 5 );

	m_Message5 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message5->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message5, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message6 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message6->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message6, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message6 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message6->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message6, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message7 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message7->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message7, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message7 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message7->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message7, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );


	sbMessagesSizer->Add( fgSizerTranslResults, 1, wxEXPAND, 5 );


	bRightSizer->Add( sbMessagesSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_buttonTransLineReset = new wxButton( m_panelTransline, wxID_ANY, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonTransLineReset, 0, wxALIGN_RIGHT|wxALL, 5 );


	bSizeTransline->Add( bRightSizer, 1, wxEXPAND, 5 );


	m_panelTransline->SetSizer( bSizeTransline );
	m_panelTransline->Layout();
	bSizeTransline->Fit( m_panelTransline );
	m_Notebook->AddPage( m_panelTransline, _("TransLine"), false );
	m_panelViaSize = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerViaSize;
	bSizerViaSize = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerViaLeftColumn;
	bSizerViaLeftColumn = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerVS_Inputs;
	sbSizerVS_Inputs = new wxStaticBoxSizer( new wxStaticBox( m_panelViaSize, wxID_ANY, _("Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerVS_Inputs;
	fgSizerVS_Inputs = new wxFlexGridSizer( 0, 3, 4, 0 );
	fgSizerVS_Inputs->AddGrowableCol( 1 );
	fgSizerVS_Inputs->SetFlexibleDirection( wxBOTH );
	fgSizerVS_Inputs->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextHoleDia = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Finished hole diameter (D):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHoleDia->Wrap( -1 );
	m_staticTextHoleDia->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	fgSizerVS_Inputs->Add( m_staticTextHoleDia, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_textCtrlHoleDia = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlHoleDia, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_choiceHoleDiaChoices;
	m_choiceHoleDia = new UNIT_SELECTOR_LEN( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHoleDiaChoices, 0 );
	m_choiceHoleDia->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choiceHoleDia, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_staticTextPlatingThickness = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Plating thickness (T):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPlatingThickness->Wrap( -1 );
	fgSizerVS_Inputs->Add( m_staticTextPlatingThickness, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_textCtrlPlatingThickness = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlPlatingThickness, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_choicePlatingThicknessChoices;
	m_choicePlatingThickness = new UNIT_SELECTOR_LEN( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePlatingThicknessChoices, 0 );
	m_choicePlatingThickness->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choicePlatingThickness, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_staticTextViaLength = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Via length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextViaLength->Wrap( -1 );
	m_staticTextViaLength->SetToolTip( _("Via length is the board thickness for through hole vias") );

	fgSizerVS_Inputs->Add( m_staticTextViaLength, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_textCtrlViaLength = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlViaLength, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_choiceViaLengthChoices;
	m_choiceViaLength = new UNIT_SELECTOR_LEN( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceViaLengthChoices, 0 );
	m_choiceViaLength->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choiceViaLength, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_staticTextViaPadDia = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Via pad diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextViaPadDia->Wrap( -1 );
	m_staticTextViaPadDia->SetToolTip( _("Diameter of pad surrounding via (annular ring)") );

	fgSizerVS_Inputs->Add( m_staticTextViaPadDia, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_textCtrlViaPadDia = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlViaPadDia, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_choiceViaPadDiaChoices;
	m_choiceViaPadDia = new UNIT_SELECTOR_LEN( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceViaPadDiaChoices, 0 );
	m_choiceViaPadDia->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choiceViaPadDia, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_staticTextClearanceDia = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Clearance hole diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextClearanceDia->Wrap( -1 );
	m_staticTextClearanceDia->SetToolTip( _("Diameter of clearance hole in ground plane(s)") );

	fgSizerVS_Inputs->Add( m_staticTextClearanceDia, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_textCtrlClearanceDia = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlClearanceDia, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_choiceClearanceDiaChoices;
	m_choiceClearanceDia = new UNIT_SELECTOR_LEN( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceClearanceDiaChoices, 0 );
	m_choiceClearanceDia->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choiceClearanceDia, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_staticTextImpedance = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Z0:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextImpedance->Wrap( -1 );
	m_staticTextImpedance->SetToolTip( _("Characteristic impedance of conductor") );

	fgSizerVS_Inputs->Add( m_staticTextImpedance, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_textCtrlImpedance = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlImpedance, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_choiceImpedanceChoices;
	m_choiceImpedance = new UNIT_SELECTOR_RESISTOR( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceImpedanceChoices, 0 );
	m_choiceImpedance->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choiceImpedance, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_staticAppliedCurrent = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Applied current:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticAppliedCurrent->Wrap( -1 );
	fgSizerVS_Inputs->Add( m_staticAppliedCurrent, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_textCtrlAppliedCurrent = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlAppliedCurrent, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextAppliedCurrentUnits = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAppliedCurrentUnits->Wrap( -1 );
	fgSizerVS_Inputs->Add( m_staticTextAppliedCurrentUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextResistivity = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Plating resistivity (Ohm.m):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextResistivity->Wrap( -1 );
	m_staticTextResistivity->SetToolTip( _("Specific resistance in ohms * meters") );

	fgSizerVS_Inputs->Add( m_staticTextResistivity, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_textCtrlPlatingResistivity = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlPlatingResistivity, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_button_ResistivityVia = new wxButton( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizerVS_Inputs->Add( m_button_ResistivityVia, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextPermittivity = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Er:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPermittivity->Wrap( -1 );
	m_staticTextPermittivity->SetToolTip( _("Relative dielectric constant (epsilon r)") );

	fgSizerVS_Inputs->Add( m_staticTextPermittivity, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_textCtrlPlatingPermittivity = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlPlatingPermittivity, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_button_Permittivity = new wxButton( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizerVS_Inputs->Add( m_button_Permittivity, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextTemperatureDiff = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Temperature rise:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTemperatureDiff->Wrap( -1 );
	m_staticTextTemperatureDiff->SetToolTip( _("Maximum acceptable rise in temperature") );

	fgSizerVS_Inputs->Add( m_staticTextTemperatureDiff, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_textCtrlTemperatureDiff = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlTemperatureDiff, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_viaTempUnits = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("deg C"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaTempUnits->Wrap( -1 );
	fgSizerVS_Inputs->Add( m_viaTempUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextRiseTime = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Pulse rise time:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRiseTime->Wrap( -1 );
	m_staticTextRiseTime->SetToolTip( _("Pulse rise time to calculate reactance") );

	fgSizerVS_Inputs->Add( m_staticTextRiseTime, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_textCtrlRiseTime = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlRiseTime, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextRiseTimeUnits = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("ns"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRiseTimeUnits->Wrap( -1 );
	m_staticTextRiseTimeUnits->SetToolTip( _("nanoseconds") );

	fgSizerVS_Inputs->Add( m_staticTextRiseTimeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerVS_Inputs->Add( fgSizerVS_Inputs, 0, wxEXPAND, 5 );


	bSizerViaLeftColumn->Add( sbSizerVS_Inputs, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextWarning = new wxStaticText( m_panelViaSize, wxID_ANY, _("Warning:\nVia pad diameter >= Clearance hole diameter.\nSome parameters cannot be calculated for a via inside a copper zone."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWarning->Wrap( -1 );
	m_staticTextWarning->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerViaLeftColumn->Add( m_staticTextWarning, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 10 );


	bSizerViaSize->Add( bSizerViaLeftColumn, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	bSizerRight->SetMinSize( wxSize( -1,460 ) );
	wxStaticBoxSizer* sbSizerVS_Result;
	sbSizerVS_Result = new wxStaticBoxSizer( new wxStaticBox( m_panelViaSize, wxID_ANY, _("Results") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerTW_Results11;
	fgSizerTW_Results11 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizerTW_Results11->AddGrowableCol( 1 );
	fgSizerTW_Results11->AddGrowableCol( 2 );
	fgSizerTW_Results11->SetFlexibleDirection( wxBOTH );
	fgSizerTW_Results11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextArea11 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextArea11->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticTextArea11, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_ViaResistance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaResistance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaResistance, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_viaResUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaResUnits->Wrap( -1 );
	fgSizerTW_Results11->Add( m_viaResUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticText65111 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Voltage drop:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText65111->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticText65111, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_ViaVoltageDrop = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaVoltageDrop->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaVoltageDrop, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText8411 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8411->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticText8411, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticText66111 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Power loss:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText66111->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticText66111, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_ViaPowerLoss = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaPowerLoss->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaPowerLoss, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText8311 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("W"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8311->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticText8311, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticText79211 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Thermal resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText79211->Wrap( -1 );
	m_staticText79211->SetToolTip( _("Using thermal conductivity value 401 Watts/(meter-Kelvin)") );

	fgSizerTW_Results11->Add( m_staticText79211, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_ViaThermalResistance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaThermalResistance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaThermalResistance, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_viaThermalResUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("deg C/W"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaThermalResUnits->Wrap( -1 );
	fgSizerTW_Results11->Add( m_viaThermalResUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextAmpacity = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Estimated ampacity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAmpacity->Wrap( -1 );
	m_staticTextAmpacity->SetToolTip( _("Based on temperature rise") );

	fgSizerTW_Results11->Add( m_staticTextAmpacity, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ViaAmpacity = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaAmpacity->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaAmpacity, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextAmpacityUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAmpacityUnits->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticTextAmpacityUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextCapacitance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Capacitance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCapacitance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticTextCapacitance, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_ViaCapacitance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaCapacitance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaCapacitance, 0, wxRIGHT|wxLEFT, 5 );

	m_staticTextCapacitanceUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("pF"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCapacitanceUnits->Wrap( -1 );
	m_staticTextCapacitanceUnits->SetToolTip( _("pico-Farad") );

	fgSizerTW_Results11->Add( m_staticTextCapacitanceUnits, 0, wxRIGHT, 5 );

	m_staticTextRiseTimeOutput = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Rise time degradation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRiseTimeOutput->Wrap( -1 );
	m_staticTextRiseTimeOutput->SetToolTip( _("Rise time degradation for given Z0 and calculated capacitance") );

	fgSizerTW_Results11->Add( m_staticTextRiseTimeOutput, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_RiseTimeOutput = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RiseTimeOutput->Wrap( -1 );
	fgSizerTW_Results11->Add( m_RiseTimeOutput, 0, wxRIGHT|wxLEFT, 5 );

	m_staticTextRiseTimeOutputUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("ps"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRiseTimeOutputUnits->Wrap( -1 );
	m_staticTextRiseTimeOutputUnits->SetToolTip( _("picoseconds") );

	fgSizerTW_Results11->Add( m_staticTextRiseTimeOutputUnits, 0, wxRIGHT, 5 );

	m_staticTextInductance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Inductance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInductance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticTextInductance, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_Inductance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Inductance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_Inductance, 0, wxRIGHT|wxLEFT, 5 );

	m_staticTextInductanceUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("nH"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInductanceUnits->Wrap( -1 );
	m_staticTextInductanceUnits->SetToolTip( _("nano-Henry") );

	fgSizerTW_Results11->Add( m_staticTextInductanceUnits, 0, wxRIGHT, 5 );

	m_staticTextReactance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Reactance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextReactance->Wrap( -1 );
	m_staticTextReactance->SetToolTip( _("Inductive reactance for given rise time and calculated inductance") );

	fgSizerTW_Results11->Add( m_staticTextReactance, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_Reactance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Reactance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_Reactance, 0, wxRIGHT|wxLEFT, 5 );

	m_viaReactanceUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaReactanceUnits->Wrap( -1 );
	fgSizerTW_Results11->Add( m_viaReactanceUnits, 0, wxRIGHT, 5 );


	sbSizerVS_Result->Add( fgSizerTW_Results11, 0, wxEXPAND, 5 );


	bSizerRight->Add( sbSizerVS_Result, 0, wxEXPAND|wxALL, 5 );

	m_viaBitmap = new wxStaticBitmap( m_panelViaSize, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_viaBitmap->SetToolTip( _("Top view of via") );

	bSizerRight->Add( m_viaBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 10 );


	bSizerRight->Add( 0, 0, 1, 0, 5 );

	m_buttonViaReset = new wxButton( m_panelViaSize, wxID_ANY, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonViaReset, 0, wxALIGN_RIGHT|wxALL, 5 );


	bSizerViaSize->Add( bSizerRight, 0, wxEXPAND, 5 );


	m_panelViaSize->SetSizer( bSizerViaSize );
	m_panelViaSize->Layout();
	bSizerViaSize->Fit( m_panelViaSize );
	m_Notebook->AddPage( m_panelViaSize, _("Via Size"), false );
	m_panelTrackWidth = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerTrackWidth;
	bSizerTrackWidth = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizeLeft;
	bSizeLeft = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerTW_Prms;
	sbSizerTW_Prms = new wxStaticBoxSizer( new wxStaticBox( m_panelTrackWidth, wxID_ANY, _("Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerTWprms;
	fgSizerTWprms = new wxFlexGridSizer( 4, 3, 0, 0 );
	fgSizerTWprms->AddGrowableCol( 1 );
	fgSizerTWprms->SetFlexibleDirection( wxBOTH );
	fgSizerTWprms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextCurrent = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("Current:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCurrent->Wrap( -1 );
	fgSizerTWprms->Add( m_staticTextCurrent, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TrackCurrentValue = new wxTextCtrl( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTWprms->Add( m_TrackCurrentValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticText62 = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText62->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText62, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText63 = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("Temperature rise:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText63->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText63, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TrackDeltaTValue = new wxTextCtrl( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTWprms->Add( m_TrackDeltaTValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_trackTempUnits = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("deg C"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackTempUnits->Wrap( -1 );
	fgSizerTWprms->Add( m_trackTempUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText66 = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("Conductor length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText66->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText66, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TrackLengthValue = new wxTextCtrl( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTWprms->Add( m_TrackLengthValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_TW_CuLength_choiceUnitChoices;
	m_TW_CuLength_choiceUnit = new UNIT_SELECTOR_LEN( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TW_CuLength_choiceUnitChoices, 0 );
	m_TW_CuLength_choiceUnit->SetSelection( 0 );
	fgSizerTWprms->Add( m_TW_CuLength_choiceUnit, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	m_TWResistivity = new wxTextCtrl( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTWprms->Add( m_TWResistivity, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_staticText103 = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("Resistivity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText103->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText103, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_resistivityUnits = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("Ohm-meter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_resistivityUnits->Wrap( -1 );
	fgSizerTWprms->Add( m_resistivityUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	sbSizerTW_Prms->Add( fgSizerTWprms, 0, wxEXPAND, 5 );


	bSizeLeft->Add( sbSizerTW_Prms, 0, wxALL|wxEXPAND, 5 );

	m_htmlWinFormulas = new wxHtmlWindow( m_panelTrackWidth, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_NO_SELECTION|wxHW_SCROLLBAR_AUTO );
	bSizeLeft->Add( m_htmlWinFormulas, 1, wxEXPAND|wxALL, 8 );


	bSizerTrackWidth->Add( bSizeLeft, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizeRight;
	bSizeRight = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerTW_Result;
	sbSizerTW_Result = new wxStaticBoxSizer( new wxStaticBox( m_panelTrackWidth, wxID_ANY, _("External Layer Traces") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerTW_Results;
	fgSizerTW_Results = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerTW_Results->AddGrowableCol( 1 );
	fgSizerTW_Results->SetFlexibleDirection( wxBOTH );
	fgSizerTW_Results->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextExtWidth = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Trace width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextExtWidth->Wrap( -1 );
	m_staticTextExtWidth->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	fgSizerTW_Results->Add( m_staticTextExtWidth, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_ExtTrackWidthValue = new wxTextCtrl( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTW_Results->Add( m_ExtTrackWidthValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_TW_ExtTrackWidth_choiceUnitChoices;
	m_TW_ExtTrackWidth_choiceUnit = new UNIT_SELECTOR_LEN( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TW_ExtTrackWidth_choiceUnitChoices, 0 );
	m_TW_ExtTrackWidth_choiceUnit->SetSelection( 0 );
	fgSizerTW_Results->Add( m_TW_ExtTrackWidth_choiceUnit, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText65 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Trace thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText65->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText65, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_ExtTrackThicknessValue = new wxTextCtrl( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTW_Results->Add( m_ExtTrackThicknessValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );

	wxArrayString m_ExtTrackThicknessUnitChoices;
	m_ExtTrackThicknessUnit = new UNIT_SELECTOR_THICKNESS( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ExtTrackThicknessUnitChoices, 0 );
	m_ExtTrackThicknessUnit->SetSelection( 0 );
	fgSizerTW_Results->Add( m_ExtTrackThicknessUnit, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );

	m_staticline3 = new wxStaticLine( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_staticline4 = new wxStaticLine( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_staticline5 = new wxStaticLine( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results->Add( m_staticline5, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_staticTextArea = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Cross-section area:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextArea->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticTextArea, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_ExtTrackAreaValue = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackAreaValue->Wrap( -1 );
	fgSizerTW_Results->Add( m_ExtTrackAreaValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_extTrackAreaUnitLabel = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("mm ^ 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_extTrackAreaUnitLabel->Wrap( -1 );
	fgSizerTW_Results->Add( m_extTrackAreaUnitLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText651 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText651->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText651, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_ExtTrackResistValue = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackResistValue->Wrap( -1 );
	fgSizerTW_Results->Add( m_ExtTrackResistValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_extTrackResUnits = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_extTrackResUnits->Wrap( -1 );
	fgSizerTW_Results->Add( m_extTrackResUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText661 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Voltage drop:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText661->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText661, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_ExtTrackVDropValue = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackVDropValue->Wrap( -1 );
	fgSizerTW_Results->Add( m_ExtTrackVDropValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticText83 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText83->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText83, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText79 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Power loss:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText79->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText79, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_ExtTrackLossValue = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackLossValue->Wrap( -1 );
	fgSizerTW_Results->Add( m_ExtTrackLossValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticText791 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("W"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText791->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText791, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	sbSizerTW_Result->Add( fgSizerTW_Results, 0, wxEXPAND, 5 );


	bSizeRight->Add( sbSizerTW_Result, 1, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSizerTW_Result1;
	sbSizerTW_Result1 = new wxStaticBoxSizer( new wxStaticBox( m_panelTrackWidth, wxID_ANY, _("Internal Layer Traces") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerTW_Results1;
	fgSizerTW_Results1 = new wxFlexGridSizer( 7, 3, 0, 0 );
	fgSizerTW_Results1->AddGrowableCol( 1 );
	fgSizerTW_Results1->SetFlexibleDirection( wxBOTH );
	fgSizerTW_Results1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextIntWidth = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Trace width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextIntWidth->Wrap( -1 );
	m_staticTextIntWidth->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	fgSizerTW_Results1->Add( m_staticTextIntWidth, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );

	m_IntTrackWidthValue = new wxTextCtrl( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTW_Results1->Add( m_IntTrackWidthValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_TW_IntTrackWidth_choiceUnitChoices;
	m_TW_IntTrackWidth_choiceUnit = new UNIT_SELECTOR_LEN( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TW_IntTrackWidth_choiceUnitChoices, 0 );
	m_TW_IntTrackWidth_choiceUnit->SetSelection( 0 );
	fgSizerTW_Results1->Add( m_TW_IntTrackWidth_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticText652 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Trace thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText652->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText652, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_IntTrackThicknessValue = new wxTextCtrl( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTW_Results1->Add( m_IntTrackThicknessValue, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	wxArrayString m_IntTrackThicknessUnitChoices;
	m_IntTrackThicknessUnit = new UNIT_SELECTOR_THICKNESS( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_IntTrackThicknessUnitChoices, 0 );
	m_IntTrackThicknessUnit->SetSelection( 0 );
	fgSizerTW_Results1->Add( m_IntTrackThicknessUnit, 0, wxALL, 5 );

	m_staticline8 = new wxStaticLine( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results1->Add( m_staticline8, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_staticline9 = new wxStaticLine( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results1->Add( m_staticline9, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_staticline10 = new wxStaticLine( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results1->Add( m_staticline10, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_staticTextArea1 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Cross-section area:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextArea1->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticTextArea1, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_IntTrackAreaValue = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackAreaValue->Wrap( -1 );
	fgSizerTW_Results1->Add( m_IntTrackAreaValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );

	m_intTrackAreaUnitLabel = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("mm ^ 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_intTrackAreaUnitLabel->Wrap( -1 );
	fgSizerTW_Results1->Add( m_intTrackAreaUnitLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticText6511 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6511->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText6511, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_IntTrackResistValue = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackResistValue->Wrap( -1 );
	fgSizerTW_Results1->Add( m_IntTrackResistValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_intTrackResUnits = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_intTrackResUnits->Wrap( -1 );
	fgSizerTW_Results1->Add( m_intTrackResUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText6611 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Voltage drop:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6611->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText6611, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_IntTrackVDropValue = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackVDropValue->Wrap( -1 );
	fgSizerTW_Results1->Add( m_IntTrackVDropValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticText831 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText831->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText831, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText792 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Power loss:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText792->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText792, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_IntTrackLossValue = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackLossValue->Wrap( -1 );
	fgSizerTW_Results1->Add( m_IntTrackLossValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticText7911 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("W"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7911->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText7911, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	sbSizerTW_Result1->Add( fgSizerTW_Results1, 0, wxEXPAND, 5 );


	bSizeRight->Add( sbSizerTW_Result1, 1, wxEXPAND|wxALL, 5 );

	m_buttonTrackWidthReset = new wxButton( m_panelTrackWidth, wxID_ANY, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizeRight->Add( m_buttonTrackWidthReset, 0, wxALIGN_RIGHT|wxALL, 5 );


	bSizerTrackWidth->Add( bSizeRight, 0, wxEXPAND, 5 );


	m_panelTrackWidth->SetSizer( bSizerTrackWidth );
	m_panelTrackWidth->Layout();
	bSizerTrackWidth->Fit( m_panelTrackWidth );
	m_Notebook->AddPage( m_panelTrackWidth, _("Track Width"), false );
	m_panelElectricalSpacing = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerElectricalClearance;
	bSizerElectricalClearance = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizerElectricalClearance;
	bLeftSizerElectricalClearance = new wxBoxSizer( wxVERTICAL );

	wxArrayString m_ElectricalSpacingUnitsSelectorChoices;
	m_ElectricalSpacingUnitsSelector = new UNIT_SELECTOR_LEN( m_panelElectricalSpacing, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ElectricalSpacingUnitsSelectorChoices, 0 );
	m_ElectricalSpacingUnitsSelector->SetSelection( -1 );
	m_ElectricalSpacingUnitsSelector->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_LIGHT, false, wxEmptyString ) );

	bLeftSizerElectricalClearance->Add( m_ElectricalSpacingUnitsSelector, 0, wxEXPAND|wxALL, 10 );

	m_staticline2 = new wxStaticLine( m_panelElectricalSpacing, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftSizerElectricalClearance->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	m_staticText891 = new wxStaticText( m_panelElectricalSpacing, wxID_ANY, _("Voltage > 500 V:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText891->Wrap( -1 );
	bLeftSizerElectricalClearance->Add( m_staticText891, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_ElectricalSpacingVoltage = new wxTextCtrl( m_panelElectricalSpacing, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizerElectricalClearance->Add( m_ElectricalSpacingVoltage, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_buttonElectSpacingRefresh = new wxButton( m_panelElectricalSpacing, wxID_ANY, _("Update Values"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizerElectricalClearance->Add( m_buttonElectSpacingRefresh, 0, wxEXPAND|wxALL, 5 );


	bSizerElectricalClearance->Add( bLeftSizerElectricalClearance, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_electricalSpacingSizer = new wxBoxSizer( wxVERTICAL );

	m_staticTextElectricalSpacing = new wxStaticText( m_panelElectricalSpacing, wxID_ANY, _("Note: Values are minimal values (from IPC 2221)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextElectricalSpacing->Wrap( -1 );
	m_staticTextElectricalSpacing->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_electricalSpacingSizer->Add( m_staticTextElectricalSpacing, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_gridElectricalSpacingValues = new wxGrid( m_panelElectricalSpacing, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_gridElectricalSpacingValues->CreateGrid( 10, 7 );
	m_gridElectricalSpacingValues->EnableEditing( false );
	m_gridElectricalSpacingValues->EnableGridLines( true );
	m_gridElectricalSpacingValues->EnableDragGridSize( false );
	m_gridElectricalSpacingValues->SetMargins( 0, 0 );

	// Columns
	m_gridElectricalSpacingValues->SetColSize( 0, 100 );
	m_gridElectricalSpacingValues->SetColSize( 1, 100 );
	m_gridElectricalSpacingValues->SetColSize( 2, 100 );
	m_gridElectricalSpacingValues->SetColSize( 3, 100 );
	m_gridElectricalSpacingValues->SetColSize( 4, 100 );
	m_gridElectricalSpacingValues->SetColSize( 5, 100 );
	m_gridElectricalSpacingValues->SetColSize( 6, 100 );
	m_gridElectricalSpacingValues->EnableDragColMove( false );
	m_gridElectricalSpacingValues->EnableDragColSize( true );
	m_gridElectricalSpacingValues->SetColLabelSize( 30 );
	m_gridElectricalSpacingValues->SetColLabelValue( 0, _("B1") );
	m_gridElectricalSpacingValues->SetColLabelValue( 1, _("B2") );
	m_gridElectricalSpacingValues->SetColLabelValue( 2, _("B3") );
	m_gridElectricalSpacingValues->SetColLabelValue( 3, _("B4") );
	m_gridElectricalSpacingValues->SetColLabelValue( 4, _("A5") );
	m_gridElectricalSpacingValues->SetColLabelValue( 5, _("A6") );
	m_gridElectricalSpacingValues->SetColLabelValue( 6, _("A7") );
	m_gridElectricalSpacingValues->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridElectricalSpacingValues->SetRowSize( 0, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 1, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 2, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 3, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 4, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 5, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 6, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 7, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 8, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 9, 24 );
	m_gridElectricalSpacingValues->EnableDragRowSize( false );
	m_gridElectricalSpacingValues->SetRowLabelSize( 100 );
	m_gridElectricalSpacingValues->SetRowLabelValue( 0, _("0 .. 15 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 1, _("16 .. 30 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 2, _("31 .. 50 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 3, _("51 .. 100 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 4, _("101 .. 150 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 5, _("151 .. 170 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 6, _("171 .. 250 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 7, _("251 .. 300 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 8, _("301 .. 500 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 9, _(" > 500 V") );
	m_gridElectricalSpacingValues->SetRowLabelAlignment( wxALIGN_RIGHT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridElectricalSpacingValues->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_TOP );
	m_electricalSpacingSizer->Add( m_gridElectricalSpacingValues, 0, wxALL, 5 );

	m_staticText88 = new wxStaticText( m_panelElectricalSpacing, wxID_ANY, _("*  B1 - Internal Conductors\n*  B2 - External Conductors, uncoated, sea level to 3050 m\n*  B3 - External Conductors, uncoated, over 3050 m\n*  B4 - External Conductors, with permanent polymer coating (any elevation)\n*  A5 - External Conductors, with conformal coating over assembly (any elevation)\n*  A6 - External Component lead/termination, uncoated\n*  A7 - External Component lead termination, with conformal coating (any elevation)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText88->Wrap( -1 );
	m_electricalSpacingSizer->Add( m_staticText88, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerElectricalClearance->Add( m_electricalSpacingSizer, 1, wxEXPAND|wxLEFT, 20 );


	m_panelElectricalSpacing->SetSizer( bSizerElectricalClearance );
	m_panelElectricalSpacing->Layout();
	bSizerElectricalClearance->Fit( m_panelElectricalSpacing );
	m_Notebook->AddPage( m_panelElectricalSpacing, _("Electrical Spacing"), false );
	m_panelBoardClass = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerBoardClass;
	bSizerBoardClass = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerUnitsMargins;
	bSizerUnitsMargins = new wxBoxSizer( wxVERTICAL );

	wxArrayString m_BoardClassesUnitsSelectorChoices;
	m_BoardClassesUnitsSelector = new UNIT_SELECTOR_LEN( m_panelBoardClass, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_BoardClassesUnitsSelectorChoices, 0 );
	m_BoardClassesUnitsSelector->SetSelection( 0 );
	bSizerUnitsMargins->Add( m_BoardClassesUnitsSelector, 0, wxTOP|wxBOTTOM|wxRIGHT, 32 );


	bSizerBoardClass->Add( bSizerUnitsMargins, 0, wxLEFT, 10 );

	wxBoxSizer* brdclsSizerRight;
	brdclsSizerRight = new wxBoxSizer( wxVERTICAL );

	m_staticTextBrdClass = new wxStaticText( m_panelBoardClass, wxID_ANY, _("Note: Values are minimal values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBrdClass->Wrap( -1 );
	m_staticTextBrdClass->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	brdclsSizerRight->Add( m_staticTextBrdClass, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_gridClassesValuesDisplay = new wxGrid( m_panelBoardClass, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_gridClassesValuesDisplay->CreateGrid( 5, 6 );
	m_gridClassesValuesDisplay->EnableEditing( false );
	m_gridClassesValuesDisplay->EnableGridLines( true );
	m_gridClassesValuesDisplay->EnableDragGridSize( false );
	m_gridClassesValuesDisplay->SetMargins( 0, 0 );

	// Columns
	m_gridClassesValuesDisplay->SetColSize( 0, 100 );
	m_gridClassesValuesDisplay->SetColSize( 1, 100 );
	m_gridClassesValuesDisplay->SetColSize( 2, 100 );
	m_gridClassesValuesDisplay->SetColSize( 3, 100 );
	m_gridClassesValuesDisplay->SetColSize( 4, 100 );
	m_gridClassesValuesDisplay->SetColSize( 5, 100 );
	m_gridClassesValuesDisplay->EnableDragColMove( false );
	m_gridClassesValuesDisplay->EnableDragColSize( true );
	m_gridClassesValuesDisplay->SetColLabelSize( 30 );
	m_gridClassesValuesDisplay->SetColLabelValue( 0, _("Class 1") );
	m_gridClassesValuesDisplay->SetColLabelValue( 1, _("Class 2") );
	m_gridClassesValuesDisplay->SetColLabelValue( 2, _("Class 3") );
	m_gridClassesValuesDisplay->SetColLabelValue( 3, _("Class 4") );
	m_gridClassesValuesDisplay->SetColLabelValue( 4, _("Class 5") );
	m_gridClassesValuesDisplay->SetColLabelValue( 5, _("Class 6") );
	m_gridClassesValuesDisplay->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridClassesValuesDisplay->SetRowSize( 0, 24 );
	m_gridClassesValuesDisplay->SetRowSize( 1, 24 );
	m_gridClassesValuesDisplay->SetRowSize( 2, 24 );
	m_gridClassesValuesDisplay->SetRowSize( 3, 24 );
	m_gridClassesValuesDisplay->SetRowSize( 4, 24 );
	m_gridClassesValuesDisplay->EnableDragRowSize( false );
	m_gridClassesValuesDisplay->SetRowLabelSize( 160 );
	m_gridClassesValuesDisplay->SetRowLabelValue( 0, _("Lines width") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 1, _("Min clearance") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 2, _("Via: (diam - drill)") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 3, _("Plated Pad: (diam - drill)") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 4, _("NP Pad: (diam - drill)") );
	m_gridClassesValuesDisplay->SetRowLabelAlignment( wxALIGN_RIGHT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridClassesValuesDisplay->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_TOP );
	brdclsSizerRight->Add( m_gridClassesValuesDisplay, 0, wxALL, 5 );

	m_panelShowClassPrms = new wxPanel( m_panelBoardClass, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	brdclsSizerRight->Add( m_panelShowClassPrms, 1, wxALL|wxEXPAND, 5 );


	bSizerBoardClass->Add( brdclsSizerRight, 1, wxEXPAND, 5 );


	m_panelBoardClass->SetSizer( bSizerBoardClass );
	m_panelBoardClass->Layout();
	bSizerBoardClass->Fit( m_panelBoardClass );
	m_Notebook->AddPage( m_panelBoardClass, _("Board Classes"), false );

	bmainFrameSizer->Add( m_Notebook, 1, wxEXPAND, 5 );


	this->SetSizer( bmainFrameSizer );
	this->Layout();
	bmainFrameSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( PCB_CALCULATOR_FRAME_BASE::OnClosePcbCalc ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PCB_CALCULATOR_FRAME_BASE::OnUpdateUI ) );
	m_choiceRegType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulTypeSelection ), NULL, this );
	m_buttonCalculate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulatorCalcButtonClick ), NULL, this );
	m_buttonRegulReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulatorResetButtonClick ), NULL, this );
	m_choiceRegulatorSelector->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulatorSelection ), NULL, this );
	m_buttonDataFile->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnDataFileSelection ), NULL, this );
	m_buttonEditItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnEditRegulator ), NULL, this );
	m_buttonAddItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnAddRegulator ), NULL, this );
	m_buttonRemoveItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRemoveRegulator ), NULL, this );
	m_AttenuatorsSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnAttenuatorSelection ), NULL, this );
	m_buttonAlcAtt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnCalculateAttenuator ), NULL, this );
	m_bpButtonCalcAtt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnCalculateAttenuator ), NULL, this );
	m_e1->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e3->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e6->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e12->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_buttonEScalculate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnCalculateESeries ), NULL, this );
	m_rbToleranceSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnToleranceSelection ), NULL, this );
	m_TranslineSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSelection ), NULL, this );
	m_button_EpsilonR->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineEpsilonR_Button ), NULL, this );
	m_button_TanD->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineTanD_Button ), NULL, this );
	m_button_Rho->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineRho_Button ), NULL, this );
	m_bpButtonAnalyze->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineAnalyse ), NULL, this );
	m_AnalyseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineAnalyse ), NULL, this );
	m_SynthetizeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSynthetize ), NULL, this );
	m_bpButtonSynthetize->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSynthetize ), NULL, this );
	m_buttonTransLineReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTransLineResetButtonClick ), NULL, this );
	m_textCtrlHoleDia->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choiceHoleDia->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlPlatingThickness->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choicePlatingThickness->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlViaLength->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choiceViaLength->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlViaPadDia->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choiceViaPadDia->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlClearanceDia->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choiceClearanceDia->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlImpedance->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choiceImpedance->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlAppliedCurrent->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlPlatingResistivity->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_button_ResistivityVia->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaRho_Button ), NULL, this );
	m_textCtrlPlatingPermittivity->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_button_Permittivity->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaEpsilonR_Button ), NULL, this );
	m_textCtrlTemperatureDiff->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlRiseTime->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_staticTextWarning->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PCB_CALCULATOR_FRAME_BASE::onUpdateViaCalcErrorText ), NULL, this );
	m_buttonViaReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaResetButtonClick ), NULL, this );
	m_TrackCurrentValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWCalculateFromCurrent ), NULL, this );
	m_TrackDeltaTValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_TrackLengthValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_TW_CuLength_choiceUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_TWResistivity->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackWidthValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWCalculateFromExtWidth ), NULL, this );
	m_TW_ExtTrackWidth_choiceUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackThicknessValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackThicknessUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackWidthValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWCalculateFromIntWidth ), NULL, this );
	m_TW_IntTrackWidth_choiceUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackThicknessValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackThicknessUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_buttonTrackWidthReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWResetButtonClick ), NULL, this );
	m_ElectricalSpacingUnitsSelector->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnElectricalSpacingUnitsSelection ), NULL, this );
	m_buttonElectSpacingRefresh->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnElectricalSpacingRefresh ), NULL, this );
	m_BoardClassesUnitsSelector->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnBoardClassesUnitsSelection ), NULL, this );
}

PCB_CALCULATOR_FRAME_BASE::~PCB_CALCULATOR_FRAME_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( PCB_CALCULATOR_FRAME_BASE::OnClosePcbCalc ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PCB_CALCULATOR_FRAME_BASE::OnUpdateUI ) );
	m_choiceRegType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulTypeSelection ), NULL, this );
	m_buttonCalculate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulatorCalcButtonClick ), NULL, this );
	m_buttonRegulReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulatorResetButtonClick ), NULL, this );
	m_choiceRegulatorSelector->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulatorSelection ), NULL, this );
	m_buttonDataFile->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnDataFileSelection ), NULL, this );
	m_buttonEditItem->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnEditRegulator ), NULL, this );
	m_buttonAddItem->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnAddRegulator ), NULL, this );
	m_buttonRemoveItem->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRemoveRegulator ), NULL, this );
	m_AttenuatorsSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnAttenuatorSelection ), NULL, this );
	m_buttonAlcAtt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnCalculateAttenuator ), NULL, this );
	m_bpButtonCalcAtt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnCalculateAttenuator ), NULL, this );
	m_e1->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e3->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e6->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e12->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_buttonEScalculate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnCalculateESeries ), NULL, this );
	m_rbToleranceSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnToleranceSelection ), NULL, this );
	m_TranslineSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSelection ), NULL, this );
	m_button_EpsilonR->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineEpsilonR_Button ), NULL, this );
	m_button_TanD->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineTanD_Button ), NULL, this );
	m_button_Rho->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineRho_Button ), NULL, this );
	m_bpButtonAnalyze->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineAnalyse ), NULL, this );
	m_AnalyseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineAnalyse ), NULL, this );
	m_SynthetizeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSynthetize ), NULL, this );
	m_bpButtonSynthetize->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSynthetize ), NULL, this );
	m_buttonTransLineReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTransLineResetButtonClick ), NULL, this );
	m_textCtrlHoleDia->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choiceHoleDia->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlPlatingThickness->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choicePlatingThickness->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlViaLength->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choiceViaLength->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlViaPadDia->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choiceViaPadDia->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlClearanceDia->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choiceClearanceDia->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlImpedance->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_choiceImpedance->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlAppliedCurrent->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlPlatingResistivity->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_button_ResistivityVia->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaRho_Button ), NULL, this );
	m_textCtrlPlatingPermittivity->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_button_Permittivity->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaEpsilonR_Button ), NULL, this );
	m_textCtrlTemperatureDiff->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlRiseTime->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaCalculate ), NULL, this );
	m_staticTextWarning->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PCB_CALCULATOR_FRAME_BASE::onUpdateViaCalcErrorText ), NULL, this );
	m_buttonViaReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnViaResetButtonClick ), NULL, this );
	m_TrackCurrentValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWCalculateFromCurrent ), NULL, this );
	m_TrackDeltaTValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_TrackLengthValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_TW_CuLength_choiceUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_TWResistivity->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackWidthValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWCalculateFromExtWidth ), NULL, this );
	m_TW_ExtTrackWidth_choiceUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackThicknessValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackThicknessUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackWidthValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWCalculateFromIntWidth ), NULL, this );
	m_TW_IntTrackWidth_choiceUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackThicknessValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackThicknessUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWParametersChanged ), NULL, this );
	m_buttonTrackWidthReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWResetButtonClick ), NULL, this );
	m_ElectricalSpacingUnitsSelector->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnElectricalSpacingUnitsSelection ), NULL, this );
	m_buttonElectSpacingRefresh->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnElectricalSpacingRefresh ), NULL, this );
	m_BoardClassesUnitsSelector->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnBoardClassesUnitsSelection ), NULL, this );

}

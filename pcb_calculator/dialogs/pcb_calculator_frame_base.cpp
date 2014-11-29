///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  5 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "UnitSelector.h"

#include "pcb_calculator_frame_base.h"

#include "../bitmaps/arrow_bottom.xpm"
#include "../bitmaps/arrow_top.xpm"
#include "../bitmaps/color_code_multiplier.xpm"
#include "../bitmaps/color_code_tolerance.xpm"
#include "../bitmaps/color_code_value.xpm"
#include "../bitmaps/color_code_value_and_name.xpm"
#include "../bitmaps/regul.xpm"
#include "../bitmaps/regul_3pins.xpm"

///////////////////////////////////////////////////////////////////////////

PCB_CALCULATOR_FRAME_BASE::PCB_CALCULATOR_FRAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : KIWAY_PLAYER( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_menubar = new wxMenuBar( 0 );
	this->SetMenuBar( m_menubar );
	
	m_statusBar = this->CreateStatusBar( 1, wxST_SIZEGRIP, wxID_ANY );
	wxBoxSizer* bmainFrameSizer;
	bmainFrameSizer = new wxBoxSizer( wxVERTICAL );
	
	m_Notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelRegulators = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerMainReg;
	bSizerMainReg = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizeLeftpReg;
	bSizeLeftpReg = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerBitmapReg;
	bSizerBitmapReg = new wxBoxSizer( wxVERTICAL );
	
	
	bSizerBitmapReg->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bitmapRegul4pins = new wxStaticBitmap( m_panelRegulators, wxID_ANY, wxBitmap( regul_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBitmapReg->Add( m_bitmapRegul4pins, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bitmapRegul3pins = new wxStaticBitmap( m_panelRegulators, wxID_ANY, wxBitmap( regul_3pins_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBitmapReg->Add( m_bitmapRegul3pins, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizerBitmapReg->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizeLeftpReg->Add( bSizerBitmapReg, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerRegFormula;
	sbSizerRegFormula = new wxStaticBoxSizer( new wxStaticBox( m_panelRegulators, wxID_ANY, _("Formula") ), wxVERTICAL );
	
	m_RegulFormula = new wxStaticText( m_panelRegulators, wxID_ANY, _("Vout = Vref * (R1 + R2) / R2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulFormula->Wrap( -1 );
	m_RegulFormula->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	sbSizerRegFormula->Add( m_RegulFormula, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizeLeftpReg->Add( sbSizerRegFormula, 0, wxEXPAND, 5 );
	
	
	bSizerMainReg->Add( bSizeLeftpReg, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRegulRight;
	bSizerRegulRight = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizerRegParams;
	fgSizerRegParams = new wxFlexGridSizer( 6, 4, 0, 0 );
	fgSizerRegParams->AddGrowableCol( 2 );
	fgSizerRegParams->SetFlexibleDirection( wxBOTH );
	fgSizerRegParams->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_rbRegulR1 = new wxRadioButton( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbRegulR1->SetValue( true ); 
	fgSizerRegParams->Add( m_rbRegulR1, 0, wxALL, 5 );
	
	m_labelRegultR1 = new wxStaticText( m_panelRegulators, wxID_ANY, _("R1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelRegultR1->Wrap( -1 );
	fgSizerRegParams->Add( m_labelRegultR1, 0, wxALL, 5 );
	
	m_RegulR1Value = new wxTextCtrl( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulR1Value->SetMaxLength( 0 ); 
	fgSizerRegParams->Add( m_RegulR1Value, 0, wxALL|wxEXPAND, 5 );
	
	m_UnitRegultR11 = new wxStaticText( m_panelRegulators, wxID_ANY, _("KOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_UnitRegultR11->Wrap( -1 );
	fgSizerRegParams->Add( m_UnitRegultR11, 0, wxALL, 5 );
	
	m_rbRegulR2 = new wxRadioButton( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_rbRegulR2, 0, wxALL, 5 );
	
	m_labelRegultR2 = new wxStaticText( m_panelRegulators, wxID_ANY, _("R2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelRegultR2->Wrap( -1 );
	fgSizerRegParams->Add( m_labelRegultR2, 0, wxALL, 5 );
	
	m_RegulR2Value = new wxTextCtrl( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulR2Value->SetMaxLength( 0 ); 
	fgSizerRegParams->Add( m_RegulR2Value, 0, wxALL|wxEXPAND, 5 );
	
	m_UnitRegultR1 = new wxStaticText( m_panelRegulators, wxID_ANY, _("KOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_UnitRegultR1->Wrap( -1 );
	fgSizerRegParams->Add( m_UnitRegultR1, 0, wxALL, 5 );
	
	m_rbRegulVout = new wxRadioButton( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRegParams->Add( m_rbRegulVout, 0, wxALL, 5 );
	
	m_labelVout = new wxStaticText( m_panelRegulators, wxID_ANY, _("Vout"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelVout->Wrap( -1 );
	fgSizerRegParams->Add( m_labelVout, 0, wxALL, 5 );
	
	m_RegulVoutValue = new wxTextCtrl( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulVoutValue->SetMaxLength( 0 ); 
	fgSizerRegParams->Add( m_RegulVoutValue, 0, wxALL|wxEXPAND, 5 );
	
	m_unitsVout = new wxStaticText( m_panelRegulators, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsVout->Wrap( -1 );
	fgSizerRegParams->Add( m_unitsVout, 0, wxALL, 5 );
	
	
	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_labelVRef = new wxStaticText( m_panelRegulators, wxID_ANY, _("Vref"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelVRef->Wrap( -1 );
	m_labelVRef->SetToolTip( _("The internal reference voltage of the regulator.\nShould not be 0.") );
	
	fgSizerRegParams->Add( m_labelVRef, 0, wxALL, 5 );
	
	m_RegulVrefValue = new wxTextCtrl( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulVrefValue->SetMaxLength( 0 ); 
	fgSizerRegParams->Add( m_RegulVrefValue, 0, wxALL|wxEXPAND, 5 );
	
	m_unitsVref = new wxStaticText( m_panelRegulators, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsVref->Wrap( -1 );
	fgSizerRegParams->Add( m_unitsVref, 0, wxALL, 5 );
	
	
	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_RegulIadjTitle = new wxStaticText( m_panelRegulators, wxID_ANY, _("Iadj"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulIadjTitle->Wrap( -1 );
	m_RegulIadjTitle->SetToolTip( _("For 3 terminal regulators only, the  Adjust pin current.") );
	
	fgSizerRegParams->Add( m_RegulIadjTitle, 0, wxALL, 5 );
	
	m_RegulIadjValue = new wxTextCtrl( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulIadjValue->SetMaxLength( 0 ); 
	fgSizerRegParams->Add( m_RegulIadjValue, 0, wxALL|wxEXPAND, 5 );
	
	m_IadjUnitLabel = new wxStaticText( m_panelRegulators, wxID_ANY, _("uA"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IadjUnitLabel->Wrap( -1 );
	fgSizerRegParams->Add( m_IadjUnitLabel, 0, wxALL, 5 );
	
	
	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticTextRegType = new wxStaticText( m_panelRegulators, wxID_ANY, _("Type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRegType->Wrap( -1 );
	m_staticTextRegType->SetToolTip( _("Type of the regulator.\nThere are 2 types:\n- regulators which have a dedicted sense pin for the voltage regulation.\n- 3 terminal pins.") );
	
	fgSizerRegParams->Add( m_staticTextRegType, 0, wxALL, 5 );
	
	wxString m_choiceRegTypeChoices[] = { _("Standard Type"), _("3 Terminal Type") };
	int m_choiceRegTypeNChoices = sizeof( m_choiceRegTypeChoices ) / sizeof( wxString );
	m_choiceRegType = new wxChoice( m_panelRegulators, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRegTypeNChoices, m_choiceRegTypeChoices, 0 );
	m_choiceRegType->SetSelection( 0 );
	fgSizerRegParams->Add( m_choiceRegType, 0, wxALL|wxEXPAND, 5 );
	
	
	fgSizerRegParams->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizerRegulRight->Add( fgSizerRegParams, 0, wxEXPAND, 5 );
	
	m_buttonCalculate = new wxButton( m_panelRegulators, wxID_ANY, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRegulRight->Add( m_buttonCalculate, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxStaticBoxSizer* sbSizerRegulatorsChooser;
	sbSizerRegulatorsChooser = new wxStaticBoxSizer( new wxStaticBox( m_panelRegulators, wxID_ANY, _("Regulator") ), wxVERTICAL );
	
	wxArrayString m_choiceRegulatorSelectorChoices;
	m_choiceRegulatorSelector = new wxChoice( m_panelRegulators, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRegulatorSelectorChoices, 0 );
	m_choiceRegulatorSelector->SetSelection( 0 );
	sbSizerRegulatorsChooser->Add( m_choiceRegulatorSelector, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextRegFile = new wxStaticText( m_panelRegulators, wxID_ANY, _("Regulators data file:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRegFile->Wrap( -1 );
	m_staticTextRegFile->SetToolTip( _("The name of the data file which stores known regulators parameters.") );
	
	sbSizerRegulatorsChooser->Add( m_staticTextRegFile, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerDataFile;
	bSizerDataFile = new wxBoxSizer( wxHORIZONTAL );
	
	m_regulators_fileNameCtrl = new wxTextCtrl( m_panelRegulators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_regulators_fileNameCtrl->SetMaxLength( 0 ); 
	bSizerDataFile->Add( m_regulators_fileNameCtrl, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_buttonDataFile = new wxButton( m_panelRegulators, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDataFile->Add( m_buttonDataFile, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizerRegulatorsChooser->Add( bSizerDataFile, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerReulBtn;
	bSizerReulBtn = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonEditItem = new wxButton( m_panelRegulators, wxID_ANY, _("Edit Regulator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonEditItem->SetToolTip( _("Edit the current selected regulator.") );
	
	bSizerReulBtn->Add( m_buttonEditItem, 0, wxALL, 5 );
	
	m_buttonAddItem = new wxButton( m_panelRegulators, wxID_ANY, _("Add Regulator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAddItem->SetToolTip( _("Enter a new item to the current list of available regulators") );
	
	bSizerReulBtn->Add( m_buttonAddItem, 1, wxALL, 5 );
	
	m_buttonRemoveItem = new wxButton( m_panelRegulators, wxID_ANY, _("Remove Regulator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRemoveItem->SetToolTip( _("Remove an item from the current list of available regulators") );
	
	bSizerReulBtn->Add( m_buttonRemoveItem, 1, wxALL, 5 );
	
	
	sbSizerRegulatorsChooser->Add( bSizerReulBtn, 1, wxEXPAND, 5 );
	
	
	bSizerRegulRight->Add( sbSizerRegulatorsChooser, 0, wxEXPAND, 5 );
	
	m_RegulMessage = new wxStaticText( m_panelRegulators, wxID_ANY, _("Message"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulMessage->Wrap( -1 );
	bSizerRegulRight->Add( m_RegulMessage, 0, wxALL, 5 );
	
	
	bSizerMainReg->Add( bSizerRegulRight, 1, wxEXPAND, 5 );
	
	
	m_panelRegulators->SetSizer( bSizerMainReg );
	m_panelRegulators->Layout();
	bSizerMainReg->Fit( m_panelRegulators );
	m_Notebook->AddPage( m_panelRegulators, _("Regulators"), true );
	m_panelTrackWidth = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerTrackWidth;
	bSizerTrackWidth = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizerTW_Prms;
	sbSizerTW_Prms = new wxStaticBoxSizer( new wxStaticBox( m_panelTrackWidth, wxID_ANY, _("Parameters:") ), wxVERTICAL );
	
	m_staticTextTW_WarningMessage = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Valid max values:\n35A for external traces and 17.5A for internal.\n400mil widths.\nMaximum temperature rise of 100 deg C."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTW_WarningMessage->Wrap( -1 );
	m_staticTextTW_WarningMessage->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	sbSizerTW_Prms->Add( m_staticTextTW_WarningMessage, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxFlexGridSizer* fgSizerTWprms;
	fgSizerTWprms = new wxFlexGridSizer( 4, 3, 0, 0 );
	fgSizerTWprms->AddGrowableCol( 1 );
	fgSizerTWprms->SetFlexibleDirection( wxBOTH );
	fgSizerTWprms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextCurrent = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Current"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCurrent->Wrap( -1 );
	fgSizerTWprms->Add( m_staticTextCurrent, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_TrackCurrentValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackCurrentValue->SetMaxLength( 0 ); 
	fgSizerTWprms->Add( m_TrackCurrentValue, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText62 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText62->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText62, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText63 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Temperature rise"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText63->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText63, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_TrackDeltaTValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackDeltaTValue->SetMaxLength( 0 ); 
	fgSizerTWprms->Add( m_TrackDeltaTValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticText64 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("deg C"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText64->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText64, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText65 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Cu thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText65->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText65, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );
	
	m_TrackThicknessValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackThicknessValue->SetMaxLength( 0 ); 
	fgSizerTWprms->Add( m_TrackThicknessValue, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_TW_CuThickness_choiceUnitChoices;
	m_TW_CuThickness_choiceUnit = new UNIT_SELECTOR_LEN( m_panelTrackWidth, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TW_CuThickness_choiceUnitChoices, 0 );
	m_TW_CuThickness_choiceUnit->SetSelection( 0 );
	fgSizerTWprms->Add( m_TW_CuThickness_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticText66 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Conductor length"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText66->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText66, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );
	
	m_TrackLengthValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackLengthValue->SetMaxLength( 0 ); 
	fgSizerTWprms->Add( m_TrackLengthValue, 0, wxEXPAND|wxALL, 5 );
	
	wxArrayString m_TW_CuLength_choiceUnitChoices;
	m_TW_CuLength_choiceUnit = new UNIT_SELECTOR_LEN( m_panelTrackWidth, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TW_CuLength_choiceUnitChoices, 0 );
	m_TW_CuLength_choiceUnit->SetSelection( 0 );
	fgSizerTWprms->Add( m_TW_CuLength_choiceUnit, 0, wxEXPAND|wxALL, 5 );
	
	
	sbSizerTW_Prms->Add( fgSizerTWprms, 0, wxEXPAND, 5 );
	
	m_htmlWinFormulas = new wxHtmlWindow( m_panelTrackWidth, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_NO_SELECTION|wxHW_SCROLLBAR_AUTO|wxSIMPLE_BORDER );
	sbSizerTW_Prms->Add( m_htmlWinFormulas, 1, wxEXPAND|wxTOP, 5 );
	
	
	bSizerTrackWidth->Add( sbSizerTW_Prms, 1, wxALL|wxEXPAND, 5 );
	
	m_buttonTW = new wxButton( m_panelTrackWidth, wxID_ANY, _(">>>"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerTrackWidth->Add( m_buttonTW, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizeRight;
	bSizeRight = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerTW_Result;
	sbSizerTW_Result = new wxStaticBoxSizer( new wxStaticBox( m_panelTrackWidth, wxID_ANY, _("Track Characteristics (External Layers):") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerTW_Results;
	fgSizerTW_Results = new wxFlexGridSizer( 5, 3, 0, 0 );
	fgSizerTW_Results->AddGrowableCol( 1 );
	fgSizerTW_Results->SetFlexibleDirection( wxBOTH );
	fgSizerTW_Results->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextWidth = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Required trace width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWidth->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticTextWidth, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_ExtTrackWidthValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackWidthValue->SetMaxLength( 0 ); 
	fgSizerTW_Results->Add( m_ExtTrackWidthValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxArrayString m_TW_ExtTrackWidth_choiceUnitChoices;
	m_TW_ExtTrackWidth_choiceUnit = new UNIT_SELECTOR_LEN( m_panelTrackWidth, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TW_ExtTrackWidth_choiceUnitChoices, 0 );
	m_TW_ExtTrackWidth_choiceUnit->SetSelection( 0 );
	fgSizerTW_Results->Add( m_TW_ExtTrackWidth_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextArea = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Cross-section area"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextArea->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticTextArea, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_ExtTrackAreaValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackAreaValue->SetMaxLength( 0 ); 
	fgSizerTW_Results->Add( m_ExtTrackAreaValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ExtTrackAreaUnitLabel = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("mm ^ 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackAreaUnitLabel->Wrap( -1 );
	fgSizerTW_Results->Add( m_ExtTrackAreaUnitLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText651 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Resistance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText651->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText651, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );
	
	m_ExtTrackResistValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackResistValue->SetMaxLength( 0 ); 
	fgSizerTW_Results->Add( m_ExtTrackResistValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticText84 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText84->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText84, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText661 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Voltage drop"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText661->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText661, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );
	
	m_ExtTrackVDropValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackVDropValue->SetMaxLength( 0 ); 
	fgSizerTW_Results->Add( m_ExtTrackVDropValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticText83 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Volt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText83->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText83, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText79 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Loss"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText79->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText79, 0, wxRIGHT|wxLEFT|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ExtTrackLossValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackLossValue->SetMaxLength( 0 ); 
	fgSizerTW_Results->Add( m_ExtTrackLossValue, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticText791 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Watt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText791->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText791, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerTW_Result->Add( fgSizerTW_Results, 0, wxEXPAND, 5 );
	
	
	bSizeRight->Add( sbSizerTW_Result, 1, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizerTW_Result1;
	sbSizerTW_Result1 = new wxStaticBoxSizer( new wxStaticBox( m_panelTrackWidth, wxID_ANY, _("Track Characteristics (Internal Layers):") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerTW_Results1;
	fgSizerTW_Results1 = new wxFlexGridSizer( 6, 3, 0, 0 );
	fgSizerTW_Results1->AddGrowableCol( 1 );
	fgSizerTW_Results1->SetFlexibleDirection( wxBOTH );
	fgSizerTW_Results1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextWidth11 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Required trace width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWidth11->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticTextWidth11, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_IntTrackWidthValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackWidthValue->SetMaxLength( 0 ); 
	fgSizerTW_Results1->Add( m_IntTrackWidthValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxArrayString m_TW_IntTrackWidth_choiceUnitChoices;
	m_TW_IntTrackWidth_choiceUnit = new UNIT_SELECTOR_LEN( m_panelTrackWidth, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TW_IntTrackWidth_choiceUnitChoices, 0 );
	m_TW_IntTrackWidth_choiceUnit->SetSelection( 0 );
	fgSizerTW_Results1->Add( m_TW_IntTrackWidth_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextArea1 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Cross-section area"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextArea1->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticTextArea1, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_IntTrackAreaValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackAreaValue->SetMaxLength( 0 ); 
	fgSizerTW_Results1->Add( m_IntTrackAreaValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_IntTrackAreaUnitLabel = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("mm ^ 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackAreaUnitLabel->Wrap( -1 );
	fgSizerTW_Results1->Add( m_IntTrackAreaUnitLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText6511 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Resistance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6511->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText6511, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );
	
	m_IntTrackResistValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackResistValue->SetMaxLength( 0 ); 
	fgSizerTW_Results1->Add( m_IntTrackResistValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticText841 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText841->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText841, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText6611 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Voltage drop"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6611->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText6611, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );
	
	m_IntTrackVDropValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackVDropValue->SetMaxLength( 0 ); 
	fgSizerTW_Results1->Add( m_IntTrackVDropValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticText831 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Volt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText831->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText831, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText792 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Loss"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText792->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText792, 0, wxRIGHT|wxLEFT|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_IntTrackLossValue = new wxTextCtrl( m_panelTrackWidth, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackLossValue->SetMaxLength( 0 ); 
	fgSizerTW_Results1->Add( m_IntTrackLossValue, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticText7911 = new wxStaticText( m_panelTrackWidth, wxID_ANY, _("Watt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7911->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText7911, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerTW_Result1->Add( fgSizerTW_Results1, 0, wxEXPAND, 5 );
	
	
	bSizeRight->Add( sbSizerTW_Result1, 1, wxEXPAND|wxALL, 5 );
	
	
	bSizerTrackWidth->Add( bSizeRight, 1, wxEXPAND, 5 );
	
	
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
	bLeftSizerElectricalClearance->Add( m_ElectricalSpacingUnitsSelector, 0, wxALL|wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( m_panelElectricalSpacing, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftSizerElectricalClearance->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );
	
	m_staticText891 = new wxStaticText( m_panelElectricalSpacing, wxID_ANY, _("Voltage > 500V:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText891->Wrap( -1 );
	bLeftSizerElectricalClearance->Add( m_staticText891, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ElectricalSpacingVoltage = new wxTextCtrl( m_panelElectricalSpacing, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ElectricalSpacingVoltage->SetMaxLength( 0 ); 
	bLeftSizerElectricalClearance->Add( m_ElectricalSpacingVoltage, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_buttonElectSpacingRefresh = new wxButton( m_panelElectricalSpacing, wxID_ANY, _("Update Values"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizerElectricalClearance->Add( m_buttonElectSpacingRefresh, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerElectricalClearance->Add( bLeftSizerElectricalClearance, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bElectricalSpacingSizerRight;
	bElectricalSpacingSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextElectricalSpacing = new wxStaticText( m_panelElectricalSpacing, wxID_ANY, _("Note: Values are minimal values (from IPC 2221)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextElectricalSpacing->Wrap( -1 );
	m_staticTextElectricalSpacing->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 93, 92, false, wxEmptyString ) );
	
	bElectricalSpacingSizerRight->Add( m_staticTextElectricalSpacing, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_gridElectricalSpacingValues = new wxGrid( m_panelElectricalSpacing, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridElectricalSpacingValues->CreateGrid( 10, 7 );
	m_gridElectricalSpacingValues->EnableEditing( false );
	m_gridElectricalSpacingValues->EnableGridLines( true );
	m_gridElectricalSpacingValues->EnableDragGridSize( false );
	m_gridElectricalSpacingValues->SetMargins( 0, 0 );
	
	// Columns
	m_gridElectricalSpacingValues->EnableDragColMove( false );
	m_gridElectricalSpacingValues->EnableDragColSize( true );
	m_gridElectricalSpacingValues->SetColLabelSize( 70 );
	m_gridElectricalSpacingValues->SetColLabelValue( 0, _("B1") );
	m_gridElectricalSpacingValues->SetColLabelValue( 1, _("B2") );
	m_gridElectricalSpacingValues->SetColLabelValue( 2, _("B3") );
	m_gridElectricalSpacingValues->SetColLabelValue( 3, _("B4") );
	m_gridElectricalSpacingValues->SetColLabelValue( 4, _("A5") );
	m_gridElectricalSpacingValues->SetColLabelValue( 5, _("A6") );
	m_gridElectricalSpacingValues->SetColLabelValue( 6, _("A7") );
	m_gridElectricalSpacingValues->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridElectricalSpacingValues->AutoSizeRows();
	m_gridElectricalSpacingValues->EnableDragRowSize( false );
	m_gridElectricalSpacingValues->SetRowLabelSize( 100 );
	m_gridElectricalSpacingValues->SetRowLabelValue( 0, _("0 ... 15V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 1, _("16 ... 30V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 2, _("31 ... 50V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 3, _("51 ... 100V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 4, _("101 ... 150V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 5, _("151 ... 170V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 6, _("171 ... 250V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 7, _("251 ... 300V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 8, _("301 ... 500V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 9, _(" > 500V") );
	m_gridElectricalSpacingValues->SetRowLabelAlignment( wxALIGN_RIGHT, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridElectricalSpacingValues->SetDefaultCellAlignment( wxALIGN_CENTRE, wxALIGN_TOP );
	bElectricalSpacingSizerRight->Add( m_gridElectricalSpacingValues, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText88 = new wxStaticText( m_panelElectricalSpacing, wxID_ANY, _("*  B1 - Internal Conductors\n*  B2 - External Conductors, uncoated, sea level to 3050 m\n*  B3 - External Conductors, uncoated, over 3050 m\n*  B4 - External Conductors, with permanent polymer coating (any elevation)\n*  A5 - External Conductors, with conformal coating over assembly (any elevation)\n*  A6 - External Component lead/termination, uncoated\n*  A7 - External Component lead termination, with conformal coating (any elevation)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText88->Wrap( -1 );
	bElectricalSpacingSizerRight->Add( m_staticText88, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerElectricalClearance->Add( bElectricalSpacingSizerRight, 1, wxEXPAND, 5 );
	
	
	m_panelElectricalSpacing->SetSizer( bSizerElectricalClearance );
	m_panelElectricalSpacing->Layout();
	bSizerElectricalClearance->Fit( m_panelElectricalSpacing );
	m_Notebook->AddPage( m_panelElectricalSpacing, _("Electrical Spacing"), false );
	m_panelTransline = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizeTransline;
	bSizeTransline = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_TranslineSelectionChoices[] = { _("Microstrip Line"), _("Coplanar wave guide"), _("Grounded Coplanar wave guide"), _("Rectangular Waveguide"), _("Coaxial Line"), _("Coupled Microstrip Line"), _("Stripline"), _("Twisted Pair") };
	int m_TranslineSelectionNChoices = sizeof( m_TranslineSelectionChoices ) / sizeof( wxString );
	m_TranslineSelection = new wxRadioBox( m_panelTransline, wxID_ANY, _("Transmission Line Type:"), wxDefaultPosition, wxDefaultSize, m_TranslineSelectionNChoices, m_TranslineSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_TranslineSelection->SetSelection( 0 );
	bLeftSizer->Add( m_TranslineSelection, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_panelDisplayshape = new wxPanel( m_panelTransline, wxID_ANY, wxDefaultPosition, wxSize( 205,205 ), wxTAB_TRAVERSAL );
	bLeftSizer->Add( m_panelDisplayshape, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	bSizeTransline->Add( bLeftSizer, 0, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizeTransline->Add( m_staticline1, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSubstrateBoxSizer;
	sbSubstrateBoxSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Substrate Parameters") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerSubstPrms;
	fgSizerSubstPrms = new wxFlexGridSizer( 9, 3, 0, 0 );
	fgSizerSubstPrms->AddGrowableCol( 1 );
	fgSizerSubstPrms->SetFlexibleDirection( wxBOTH );
	fgSizerSubstPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_EpsilonR_label = new wxStaticText( m_panelTransline, wxID_ANY, _("Er"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EpsilonR_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_EpsilonR_label, 0, wxRIGHT|wxLEFT|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_Value_EpsilonR = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Value_EpsilonR->SetMaxLength( 0 ); 
	fgSizerSubstPrms->Add( m_Value_EpsilonR, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_button_EpsilonR = new wxButton( m_panelTransline, wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizerSubstPrms->Add( m_button_EpsilonR, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TanD_label = new wxStaticText( m_panelTransline, wxID_ANY, _("TanD"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TanD_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_TanD_label, 0, wxRIGHT|wxLEFT|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_Value_TanD = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Value_TanD->SetMaxLength( 0 ); 
	fgSizerSubstPrms->Add( m_Value_TanD, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_button_TanD = new wxButton( m_panelTransline, wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizerSubstPrms->Add( m_button_TanD, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_Rho_label = new wxStaticText( m_panelTransline, wxID_ANY, _("Rho"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Rho_label->Wrap( -1 );
	m_Rho_label->SetToolTip( _("Specific resistance in ohms * meters") );
	
	fgSizerSubstPrms->Add( m_Rho_label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Value_Rho = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Value_Rho->SetMaxLength( 0 ); 
	fgSizerSubstPrms->Add( m_Value_Rho, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button_Rho = new wxButton( m_panelTransline, wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizerSubstPrms->Add( m_button_Rho, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_substrate_prm4_label = new wxStaticText( m_panelTransline, wxID_ANY, _("H"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm4_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm4_label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Substrate_prm4_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Substrate_prm4_Value->SetMaxLength( 0 ); 
	fgSizerSubstPrms->Add( m_Substrate_prm4_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_SubsPrm4_choiceUnitChoices;
	m_SubsPrm4_choiceUnit = new UNIT_SELECTOR_LEN( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm4_choiceUnitChoices, 0 );
	m_SubsPrm4_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm4_choiceUnit, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_substrate_prm5_label = new wxStaticText( m_panelTransline, wxID_ANY, _("H_t"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm5_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm5_label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Substrate_prm5_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Substrate_prm5_Value->SetMaxLength( 0 ); 
	fgSizerSubstPrms->Add( m_Substrate_prm5_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_SubsPrm5_choiceUnitChoices;
	m_SubsPrm5_choiceUnit = new UNIT_SELECTOR_LEN( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm5_choiceUnitChoices, 0 );
	m_SubsPrm5_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm5_choiceUnit, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_substrate_prm6_label = new wxStaticText( m_panelTransline, wxID_ANY, _("T"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm6_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm6_label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Substrate_prm6_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Substrate_prm6_Value->SetMaxLength( 0 ); 
	fgSizerSubstPrms->Add( m_Substrate_prm6_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_SubsPrm6_choiceUnitChoices;
	m_SubsPrm6_choiceUnit = new UNIT_SELECTOR_LEN( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm6_choiceUnitChoices, 0 );
	m_SubsPrm6_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm6_choiceUnit, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_substrate_prm7_label = new wxStaticText( m_panelTransline, wxID_ANY, _("Rough"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm7_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm7_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );
	
	m_Substrate_prm7_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Substrate_prm7_Value->SetMaxLength( 0 ); 
	fgSizerSubstPrms->Add( m_Substrate_prm7_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_SubsPrm7_choiceUnitChoices;
	m_SubsPrm7_choiceUnit = new UNIT_SELECTOR_LEN( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm7_choiceUnitChoices, 0 );
	m_SubsPrm7_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm7_choiceUnit, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_substrate_prm8_label = new wxStaticText( m_panelTransline, wxID_ANY, _("Mur"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm8_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm8_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );
	
	m_Substrate_prm8_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Substrate_prm8_Value->SetMaxLength( 0 ); 
	fgSizerSubstPrms->Add( m_Substrate_prm8_Value, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_SubsPrm8_choiceUnitChoices;
	m_SubsPrm8_choiceUnit = new UNIT_SELECTOR_LEN( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm8_choiceUnitChoices, 0 );
	m_SubsPrm8_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm8_choiceUnit, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_substrate_prm9_label = new wxStaticText( m_panelTransline, wxID_ANY, _("MurC"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm9_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm9_label, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Substrate_prm9_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Substrate_prm9_Value->SetMaxLength( 0 ); 
	fgSizerSubstPrms->Add( m_Substrate_prm9_Value, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_SubsPrm9_choiceUnitChoices;
	m_SubsPrm9_choiceUnit = new UNIT_SELECTOR_LEN( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm9_choiceUnitChoices, 0 );
	m_SubsPrm9_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm9_choiceUnit, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	sbSubstrateBoxSizer->Add( fgSizerSubstPrms, 1, wxEXPAND, 5 );
	
	
	bMiddleSizer->Add( sbSubstrateBoxSizer, 0, wxEXPAND|wxBOTTOM, 5 );
	
	wxStaticBoxSizer* sbCmpPrmsSizer;
	sbCmpPrmsSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Component Parameters:") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizeCmpPrms;
	fgSizeCmpPrms = new wxFlexGridSizer( 1, 3, 0, 0 );
	fgSizeCmpPrms->AddGrowableCol( 1 );
	fgSizeCmpPrms->SetFlexibleDirection( wxBOTH );
	fgSizeCmpPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_Frequency_label = new wxStaticText( m_panelTransline, wxID_ANY, _("Frequency"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Frequency_label->Wrap( -1 );
	fgSizeCmpPrms->Add( m_Frequency_label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Value_Frequency_Ctrl = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Value_Frequency_Ctrl->SetMaxLength( 0 ); 
	fgSizeCmpPrms->Add( m_Value_Frequency_Ctrl, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_choiceUnit_FrequencyChoices;
	m_choiceUnit_Frequency = new UNIT_SELECTOR_FREQUENCY( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_FrequencyChoices, 0 );
	m_choiceUnit_Frequency->SetSelection( 0 );
	fgSizeCmpPrms->Add( m_choiceUnit_Frequency, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	sbCmpPrmsSizer->Add( fgSizeCmpPrms, 0, wxEXPAND, 5 );
	
	
	bMiddleSizer->Add( sbCmpPrmsSizer, 0, wxEXPAND|wxTOP, 5 );
	
	
	bSizeTransline->Add( bMiddleSizer, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* btranslineRightSizer;
	btranslineRightSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Physical Parameters") ), wxVERTICAL );
	
	wxBoxSizer* sbRightBoxizer;
	sbRightBoxizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizerPhysPrms;
	fgSizerPhysPrms = new wxFlexGridSizer( 4, 4, 0, 0 );
	fgSizerPhysPrms->AddGrowableCol( 1 );
	fgSizerPhysPrms->SetFlexibleDirection( wxBOTH );
	fgSizerPhysPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_phys_prm1_label = new wxStaticText( m_panelTransline, wxID_ANY, _("Prm1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm1_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm1_label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Phys_prm1_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Phys_prm1_Value->SetMaxLength( 0 ); 
	fgSizerPhysPrms->Add( m_Phys_prm1_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_choiceUnit_Param1Choices;
	m_choiceUnit_Param1 = new UNIT_SELECTOR_LEN( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param1Choices, 0 );
	m_choiceUnit_Param1->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param1, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_radioBtnPrm1 = new wxRadioButton( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizerPhysPrms->Add( m_radioBtnPrm1, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_phys_prm2_label = new wxStaticText( m_panelTransline, wxID_ANY, _("prm2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm2_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm2_label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Phys_prm2_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Phys_prm2_Value->SetMaxLength( 0 ); 
	fgSizerPhysPrms->Add( m_Phys_prm2_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_choiceUnit_Param2Choices;
	m_choiceUnit_Param2 = new UNIT_SELECTOR_LEN( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param2Choices, 0 );
	m_choiceUnit_Param2->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param2, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_radioBtnPrm2 = new wxRadioButton( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_radioBtnPrm2, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_phys_prm3_label = new wxStaticText( m_panelTransline, wxID_ANY, _("prm3"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm3_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm3_label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Phys_prm3_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Phys_prm3_Value->SetMaxLength( 0 ); 
	fgSizerPhysPrms->Add( m_Phys_prm3_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_choiceUnit_Param3Choices;
	m_choiceUnit_Param3 = new UNIT_SELECTOR_LEN( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param3Choices, 0 );
	m_choiceUnit_Param3->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param3, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerPhysPrms->Add( 0, 0, 0, 0, 5 );
	
	
	sbRightBoxizer->Add( fgSizerPhysPrms, 0, wxEXPAND, 5 );
	
	
	btranslineRightSizer->Add( sbRightBoxizer, 0, wxBOTTOM|wxEXPAND, 5 );
	
	
	bRightSizer->Add( btranslineRightSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* btranslineButtonsSizer;
	btranslineButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapAnalyse = new wxStaticBitmap( m_panelTransline, wxID_ANY, wxBitmap( arrow_bottom_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_bitmapAnalyse, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_AnalyseButton = new wxButton( m_panelTransline, wxID_ANY, _("Analyze"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_AnalyseButton, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_SynthetizeButton = new wxButton( m_panelTransline, wxID_ANY, _("Synthetize"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_SynthetizeButton, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_bitmapSynthetize = new wxStaticBitmap( m_panelTransline, wxID_ANY, wxBitmap( arrow_top_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_bitmapSynthetize, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	btranslineButtonsSizer->Add( bSizerButtons, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bRightSizer->Add( btranslineButtonsSizer, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxStaticBoxSizer* sbElectricalResultsSizer;
	sbElectricalResultsSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Electrical Parameters:") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerResults;
	fgSizerResults = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizerResults->AddGrowableCol( 1 );
	fgSizerResults->SetFlexibleDirection( wxBOTH );
	fgSizerResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_elec_prm1_label = new wxStaticText( m_panelTransline, wxID_ANY, _("Z"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm1_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm1_label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Elec_prm1_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Elec_prm1_Value->SetMaxLength( 0 ); 
	fgSizerResults->Add( m_Elec_prm1_Value, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxArrayString m_choiceUnit_ElecPrm1Choices;
	m_choiceUnit_ElecPrm1 = new UNIT_SELECTOR_RESISTOR( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm1Choices, 0 );
	m_choiceUnit_ElecPrm1->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_elec_prm2_label = new wxStaticText( m_panelTransline, wxID_ANY, _("Z"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm2_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm2_label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Elec_prm2_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Elec_prm2_Value->SetMaxLength( 0 ); 
	fgSizerResults->Add( m_Elec_prm2_Value, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	wxArrayString m_choiceUnit_ElecPrm2Choices;
	m_choiceUnit_ElecPrm2 = new UNIT_SELECTOR_RESISTOR( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm2Choices, 0 );
	m_choiceUnit_ElecPrm2->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm2, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_elec_prm3_label = new wxStaticText( m_panelTransline, wxID_ANY, _("Angle"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm3_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm3_label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Elec_prm3_Value = new wxTextCtrl( m_panelTransline, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Elec_prm3_Value->SetMaxLength( 0 ); 
	fgSizerResults->Add( m_Elec_prm3_Value, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_choiceUnit_ElecPrm3Choices;
	m_choiceUnit_ElecPrm3 = new UNIT_SELECTOR_ANGLE( m_panelTransline, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm3Choices, 0 );
	m_choiceUnit_ElecPrm3->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm3, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbElectricalResultsSizer->Add( fgSizerResults, 0, wxEXPAND, 5 );
	
	
	bRightSizer->Add( sbElectricalResultsSizer, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbMessagesSizer;
	sbMessagesSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Results:") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerTranslResults;
	fgSizerTranslResults = new wxFlexGridSizer( 7, 2, 0, 0 );
	fgSizerTranslResults->AddGrowableCol( 1 );
	fgSizerTranslResults->SetFlexibleDirection( wxBOTH );
	fgSizerTranslResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_left_message1 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message1->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message1, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Message1 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message1->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message1, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_left_message2 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message2->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message2, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Message2 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message2->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message2, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_left_message3 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message3->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message3, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Message3 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message3->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message3, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_left_message4 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message4->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message4, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Message4 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message4->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message4, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_left_message5 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message5->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxALIGN_RIGHT, 5 );
	
	m_Message5 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message5->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_left_message6 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message6->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message6, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Message6 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message6->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message6, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_left_message7 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message7->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message7, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_Message7 = new wxStaticText( m_panelTransline, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message7->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message7, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	sbMessagesSizer->Add( fgSizerTranslResults, 1, wxEXPAND, 5 );
	
	
	bRightSizer->Add( sbMessagesSizer, 1, wxEXPAND|wxTOP, 5 );
	
	
	bSizeTransline->Add( bRightSizer, 1, wxEXPAND, 5 );
	
	
	m_panelTransline->SetSizer( bSizeTransline );
	m_panelTransline->Layout();
	bSizeTransline->Fit( m_panelTransline );
	m_Notebook->AddPage( m_panelTransline, _("TransLine"), false );
	m_panelAttenuators = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbSizerAtt;
	sbSizerAtt = new wxStaticBoxSizer( new wxStaticBox( m_panelAttenuators, wxID_ANY, _("label") ), wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizerAtt;
	bLeftSizerAtt = new wxBoxSizer( wxVERTICAL );
	
	wxString m_AttenuatorsSelectionChoices[] = { _("PI"), _("Tee"), _("Bridged Tee"), _("Resistive Splitter") };
	int m_AttenuatorsSelectionNChoices = sizeof( m_AttenuatorsSelectionChoices ) / sizeof( wxString );
	m_AttenuatorsSelection = new wxRadioBox( m_panelAttenuators, wxID_ANY, _("Attenuators:"), wxDefaultPosition, wxDefaultSize, m_AttenuatorsSelectionNChoices, m_AttenuatorsSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_AttenuatorsSelection->SetSelection( 0 );
	bLeftSizerAtt->Add( m_AttenuatorsSelection, 0, wxEXPAND|wxALL, 5 );
	
	m_panelDisplayAttenuator = new wxPanel( m_panelAttenuators, wxID_ANY, wxDefaultPosition, wxSize( 256,256 ), wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	bLeftSizerAtt->Add( m_panelDisplayAttenuator, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	
	sbSizerAtt->Add( bLeftSizerAtt, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bMiddleSizerAtt;
	bMiddleSizerAtt = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerAttPrms;
	sbSizerAttPrms = new wxStaticBoxSizer( new wxStaticBox( m_panelAttenuators, wxID_ANY, _("Parameters:") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerAttPrms;
	fgSizerAttPrms = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizerAttPrms->AddGrowableRow( 1 );
	fgSizerAttPrms->SetFlexibleDirection( wxBOTH );
	fgSizerAttPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_attenuationLabel = new wxStaticText( m_panelAttenuators, wxID_ANY, _("Attenuation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuationLabel->Wrap( -1 );
	fgSizerAttPrms->Add( m_attenuationLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_AttValueCtrl = new wxTextCtrl( m_panelAttenuators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_AttValueCtrl->SetMaxLength( 0 ); 
	fgSizerAttPrms->Add( m_AttValueCtrl, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_attUnit = new wxStaticText( m_panelAttenuators, wxID_ANY, _("dB"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attUnit->Wrap( -1 );
	fgSizerAttPrms->Add( m_attUnit, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_attenuationZinLabel = new wxStaticText( m_panelAttenuators, wxID_ANY, _("Zin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuationZinLabel->Wrap( -1 );
	fgSizerAttPrms->Add( m_attenuationZinLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ZinValueCtrl = new wxTextCtrl( m_panelAttenuators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ZinValueCtrl->SetMaxLength( 0 ); 
	fgSizerAttPrms->Add( m_ZinValueCtrl, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_attZinUnit = new wxStaticText( m_panelAttenuators, wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attZinUnit->Wrap( -1 );
	fgSizerAttPrms->Add( m_attZinUnit, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ZoutLabel = new wxStaticText( m_panelAttenuators, wxID_ANY, _("Zout"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ZoutLabel->Wrap( -1 );
	fgSizerAttPrms->Add( m_ZoutLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ZoutValueCtrl = new wxTextCtrl( m_panelAttenuators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ZoutValueCtrl->SetMaxLength( 0 ); 
	fgSizerAttPrms->Add( m_ZoutValueCtrl, 0, wxALL, 5 );
	
	m_attZoutUnit = new wxStaticText( m_panelAttenuators, wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attZoutUnit->Wrap( -1 );
	fgSizerAttPrms->Add( m_attZoutUnit, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerAttPrms->Add( fgSizerAttPrms, 0, wxEXPAND, 5 );
	
	
	bMiddleSizerAtt->Add( sbSizerAttPrms, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerAttButt;
	bSizerAttButt = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonAlcAtt = new wxButton( m_panelAttenuators, wxID_ANY, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerAttButt->Add( m_buttonAlcAtt, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bitmapAnalyse1 = new wxStaticBitmap( m_panelAttenuators, wxID_ANY, wxBitmap( arrow_bottom_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerAttButt->Add( m_bitmapAnalyse1, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMiddleSizerAtt->Add( bSizerAttButt, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxStaticBoxSizer* sbSizerAttValues;
	sbSizerAttValues = new wxStaticBoxSizer( new wxStaticBox( m_panelAttenuators, wxID_ANY, _("Values") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerAttResults;
	fgSizerAttResults = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizerAttResults->AddGrowableRow( 1 );
	fgSizerAttResults->SetFlexibleDirection( wxBOTH );
	fgSizerAttResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_attenuatorR1Label = new wxStaticText( m_panelAttenuators, wxID_ANY, _("R1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuatorR1Label->Wrap( -1 );
	fgSizerAttResults->Add( m_attenuatorR1Label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_Att_R1_Value = new wxTextCtrl( m_panelAttenuators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Att_R1_Value->SetMaxLength( 0 ); 
	fgSizerAttResults->Add( m_Att_R1_Value, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_attR1Unit = new wxStaticText( m_panelAttenuators, wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attR1Unit->Wrap( -1 );
	fgSizerAttResults->Add( m_attR1Unit, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_attenuatorR2Label = new wxStaticText( m_panelAttenuators, wxID_ANY, _("R2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuatorR2Label->Wrap( -1 );
	fgSizerAttResults->Add( m_attenuatorR2Label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_Att_R2_Value = new wxTextCtrl( m_panelAttenuators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Att_R2_Value->SetMaxLength( 0 ); 
	fgSizerAttResults->Add( m_Att_R2_Value, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_attR2Unit1 = new wxStaticText( m_panelAttenuators, wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attR2Unit1->Wrap( -1 );
	fgSizerAttResults->Add( m_attR2Unit1, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_attenuatorR3Label = new wxStaticText( m_panelAttenuators, wxID_ANY, _("R3"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuatorR3Label->Wrap( -1 );
	fgSizerAttResults->Add( m_attenuatorR3Label, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_Att_R3_Value = new wxTextCtrl( m_panelAttenuators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Att_R3_Value->SetMaxLength( 0 ); 
	fgSizerAttResults->Add( m_Att_R3_Value, 0, wxALL, 5 );
	
	m_attR3Unit = new wxStaticText( m_panelAttenuators, wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attR3Unit->Wrap( -1 );
	fgSizerAttResults->Add( m_attR3Unit, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerAttValues->Add( fgSizerAttResults, 0, wxEXPAND, 5 );
	
	
	bMiddleSizerAtt->Add( sbSizerAttValues, 0, wxEXPAND, 5 );
	
	m_staticTextAttMsg = new wxStaticText( m_panelAttenuators, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAttMsg->Wrap( -1 );
	bMiddleSizerAtt->Add( m_staticTextAttMsg, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Attenuator_Messages = new wxTextCtrl( m_panelAttenuators, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	bMiddleSizerAtt->Add( m_Attenuator_Messages, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizerAtt->Add( bMiddleSizerAtt, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbRightSizerFormula;
	sbRightSizerFormula = new wxStaticBoxSizer( new wxStaticBox( m_panelAttenuators, wxID_ANY, _("Formula") ), wxVERTICAL );
	
	m_panelAttFormula = new wxPanel( m_panelAttenuators, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	m_panelAttFormula->SetMinSize( wxSize( 200,-1 ) );
	
	sbRightSizerFormula->Add( m_panelAttFormula, 1, wxALL|wxEXPAND, 5 );
	
	
	sbSizerAtt->Add( sbRightSizerFormula, 1, wxEXPAND, 5 );
	
	
	m_panelAttenuators->SetSizer( sbSizerAtt );
	m_panelAttenuators->Layout();
	sbSizerAtt->Fit( m_panelAttenuators );
	m_Notebook->AddPage( m_panelAttenuators, _("RF Attenuators"), false );
	m_panelColorCode = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanelColorCode;
	bSizerPanelColorCode = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_rbToleranceSelectionChoices[] = { _("10%"), _("5%"), _("2%"), _("1%"), _("0.5%"), _("0.25%"), _("0.1%"), _("0.05%") };
	int m_rbToleranceSelectionNChoices = sizeof( m_rbToleranceSelectionChoices ) / sizeof( wxString );
	m_rbToleranceSelection = new wxRadioBox( m_panelColorCode, wxID_ANY, _("Tolerance"), wxDefaultPosition, wxDefaultSize, m_rbToleranceSelectionNChoices, m_rbToleranceSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbToleranceSelection->SetSelection( 0 );
	bSizerPanelColorCode->Add( m_rbToleranceSelection, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxFlexGridSizer* fgSizerColoCode;
	fgSizerColoCode = new wxFlexGridSizer( 2, 6, 0, 0 );
	fgSizerColoCode->SetFlexibleDirection( wxBOTH );
	fgSizerColoCode->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText31 = new wxStaticText( m_panelColorCode, wxID_ANY, _("1st Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	fgSizerColoCode->Add( m_staticText31, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText34 = new wxStaticText( m_panelColorCode, wxID_ANY, _("2nd Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText34->Wrap( -1 );
	fgSizerColoCode->Add( m_staticText34, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText35 = new wxStaticText( m_panelColorCode, wxID_ANY, _("3rd Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText35->Wrap( -1 );
	fgSizerColoCode->Add( m_staticText35, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_Band4Label = new wxStaticText( m_panelColorCode, wxID_ANY, _("4rd Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Band4Label->Wrap( -1 );
	fgSizerColoCode->Add( m_Band4Label, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText37 = new wxStaticText( m_panelColorCode, wxID_ANY, _("Multiplier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText37->Wrap( -1 );
	fgSizerColoCode->Add( m_staticText37, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText38 = new wxStaticText( m_panelColorCode, wxID_ANY, _("Tolerance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText38->Wrap( -1 );
	fgSizerColoCode->Add( m_staticText38, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_Band1bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxBitmap( color_code_value_and_name_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band1bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_Band2bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxBitmap( color_code_value_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band2bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_Band3bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxBitmap( color_code_value_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band3bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_Band4bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxBitmap( color_code_value_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band4bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_Band_mult_bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxBitmap( color_code_multiplier_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band_mult_bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_Band_tol_bitmap = new wxStaticBitmap( m_panelColorCode, wxID_ANY, wxBitmap( color_code_tolerance_xpm ), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band_tol_bitmap, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizerPanelColorCode->Add( fgSizerColoCode, 1, wxEXPAND|wxLEFT, 5 );
	
	
	m_panelColorCode->SetSizer( bSizerPanelColorCode );
	m_panelColorCode->Layout();
	bSizerPanelColorCode->Fit( m_panelColorCode );
	m_Notebook->AddPage( m_panelColorCode, _("Color Code"), false );
	m_panelBoardClass = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerBoardClass;
	bSizerBoardClass = new wxBoxSizer( wxHORIZONTAL );
	
	wxArrayString m_BoardClassesUnitsSelectorChoices;
	m_BoardClassesUnitsSelector = new UNIT_SELECTOR_LEN( m_panelBoardClass, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_BoardClassesUnitsSelectorChoices, 0 );
	m_BoardClassesUnitsSelector->SetSelection( -1 );
	bSizerBoardClass->Add( m_BoardClassesUnitsSelector, 0, wxALL, 5 );
	
	wxBoxSizer* brdclsSizerRight;
	brdclsSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextBrdClass = new wxStaticText( m_panelBoardClass, wxID_ANY, _("Note: Values are minimal values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBrdClass->Wrap( -1 );
	m_staticTextBrdClass->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 93, 92, false, wxEmptyString ) );
	
	brdclsSizerRight->Add( m_staticTextBrdClass, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_gridClassesValuesDisplay = new wxGrid( m_panelBoardClass, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridClassesValuesDisplay->CreateGrid( 5, 6 );
	m_gridClassesValuesDisplay->EnableEditing( false );
	m_gridClassesValuesDisplay->EnableGridLines( true );
	m_gridClassesValuesDisplay->EnableDragGridSize( false );
	m_gridClassesValuesDisplay->SetMargins( 0, 0 );
	
	// Columns
	m_gridClassesValuesDisplay->EnableDragColMove( false );
	m_gridClassesValuesDisplay->EnableDragColSize( true );
	m_gridClassesValuesDisplay->SetColLabelSize( 70 );
	m_gridClassesValuesDisplay->SetColLabelValue( 0, _("Class 1") );
	m_gridClassesValuesDisplay->SetColLabelValue( 1, _("Class 2") );
	m_gridClassesValuesDisplay->SetColLabelValue( 2, _("Class 3") );
	m_gridClassesValuesDisplay->SetColLabelValue( 3, _("Class 4") );
	m_gridClassesValuesDisplay->SetColLabelValue( 4, _("Class 5") );
	m_gridClassesValuesDisplay->SetColLabelValue( 5, _("Class 6") );
	m_gridClassesValuesDisplay->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridClassesValuesDisplay->AutoSizeRows();
	m_gridClassesValuesDisplay->EnableDragRowSize( false );
	m_gridClassesValuesDisplay->SetRowLabelSize( 160 );
	m_gridClassesValuesDisplay->SetRowLabelValue( 0, _("Lines width") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 1, _("Min clearance") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 2, _("Via: (diam - drill)") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 3, _("Plated Pad: (diam - drill)") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 4, _("NP Pad: (diam - drill)") );
	m_gridClassesValuesDisplay->SetRowLabelAlignment( wxALIGN_RIGHT, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridClassesValuesDisplay->SetDefaultCellAlignment( wxALIGN_CENTRE, wxALIGN_TOP );
	brdclsSizerRight->Add( m_gridClassesValuesDisplay, 0, wxALL|wxEXPAND, 5 );
	
	m_panelShowClassPrms = new wxPanel( m_panelBoardClass, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	brdclsSizerRight->Add( m_panelShowClassPrms, 1, wxEXPAND | wxALL, 5 );
	
	
	bSizerBoardClass->Add( brdclsSizerRight, 1, wxEXPAND, 5 );
	
	
	m_panelBoardClass->SetSizer( bSizerBoardClass );
	m_panelBoardClass->Layout();
	bSizerBoardClass->Fit( m_panelBoardClass );
	m_Notebook->AddPage( m_panelBoardClass, _("Board Classes"), false );
	
	bmainFrameSizer->Add( m_Notebook, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bmainFrameSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( PCB_CALCULATOR_FRAME_BASE::OnClosePcbCalc ) );
	m_choiceRegType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulTypeSelection ), NULL, this );
	m_buttonCalculate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulatorCalcButtonClick ), NULL, this );
	m_choiceRegulatorSelector->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulatorSelection ), NULL, this );
	m_buttonDataFile->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnDataFileSelection ), NULL, this );
	m_buttonEditItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnEditRegulator ), NULL, this );
	m_buttonAddItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnAddRegulator ), NULL, this );
	m_buttonRemoveItem->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRemoveRegulator ), NULL, this );
	m_buttonTW->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWCalculateButt ), NULL, this );
	m_ElectricalSpacingUnitsSelector->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnElectricalSpacingUnitsSelection ), NULL, this );
	m_buttonElectSpacingRefresh->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnElectricalSpacingRefresh ), NULL, this );
	m_TranslineSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSelection ), NULL, this );
	m_panelDisplayshape->Connect( wxEVT_PAINT, wxPaintEventHandler( PCB_CALCULATOR_FRAME_BASE::OnPaintTranslinePanel ), NULL, this );
	m_button_EpsilonR->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineEpsilonR_Button ), NULL, this );
	m_button_TanD->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineTanD_Button ), NULL, this );
	m_button_Rho->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineRho_Button ), NULL, this );
	m_AnalyseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineAnalyse ), NULL, this );
	m_SynthetizeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSynthetize ), NULL, this );
	m_AttenuatorsSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnAttenuatorSelection ), NULL, this );
	m_panelDisplayAttenuator->Connect( wxEVT_PAINT, wxPaintEventHandler( PCB_CALCULATOR_FRAME_BASE::OnPaintAttenuatorPanel ), NULL, this );
	m_buttonAlcAtt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnCalculateAttenuator ), NULL, this );
	m_panelAttFormula->Connect( wxEVT_PAINT, wxPaintEventHandler( PCB_CALCULATOR_FRAME_BASE::OnPaintAttFormulaPanel ), NULL, this );
	m_rbToleranceSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnToleranceSelection ), NULL, this );
	m_BoardClassesUnitsSelector->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnBoardClassesUnitsSelection ), NULL, this );
}

PCB_CALCULATOR_FRAME_BASE::~PCB_CALCULATOR_FRAME_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( PCB_CALCULATOR_FRAME_BASE::OnClosePcbCalc ) );
	m_choiceRegType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulTypeSelection ), NULL, this );
	m_buttonCalculate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulatorCalcButtonClick ), NULL, this );
	m_choiceRegulatorSelector->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRegulatorSelection ), NULL, this );
	m_buttonDataFile->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnDataFileSelection ), NULL, this );
	m_buttonEditItem->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnEditRegulator ), NULL, this );
	m_buttonAddItem->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnAddRegulator ), NULL, this );
	m_buttonRemoveItem->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnRemoveRegulator ), NULL, this );
	m_buttonTW->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTWCalculateButt ), NULL, this );
	m_ElectricalSpacingUnitsSelector->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnElectricalSpacingUnitsSelection ), NULL, this );
	m_buttonElectSpacingRefresh->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnElectricalSpacingRefresh ), NULL, this );
	m_TranslineSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSelection ), NULL, this );
	m_panelDisplayshape->Disconnect( wxEVT_PAINT, wxPaintEventHandler( PCB_CALCULATOR_FRAME_BASE::OnPaintTranslinePanel ), NULL, this );
	m_button_EpsilonR->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineEpsilonR_Button ), NULL, this );
	m_button_TanD->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineTanD_Button ), NULL, this );
	m_button_Rho->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineRho_Button ), NULL, this );
	m_AnalyseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineAnalyse ), NULL, this );
	m_SynthetizeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSynthetize ), NULL, this );
	m_AttenuatorsSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnAttenuatorSelection ), NULL, this );
	m_panelDisplayAttenuator->Disconnect( wxEVT_PAINT, wxPaintEventHandler( PCB_CALCULATOR_FRAME_BASE::OnPaintAttenuatorPanel ), NULL, this );
	m_buttonAlcAtt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnCalculateAttenuator ), NULL, this );
	m_panelAttFormula->Disconnect( wxEVT_PAINT, wxPaintEventHandler( PCB_CALCULATOR_FRAME_BASE::OnPaintAttFormulaPanel ), NULL, this );
	m_rbToleranceSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnToleranceSelection ), NULL, this );
	m_BoardClassesUnitsSelector->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnBoardClassesUnitsSelection ), NULL, this );
	
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/unit_selector.h"

#include "panel_transline_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_TRANSLINE_BASE::PANEL_TRANSLINE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizeTransline;
	bSizeTransline = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

    wxString m_TranslineSelectionChoices[] = { _( "Microstrip Line" ),
                                               _( "Coupled Microstrip Line" ),
                                               _( "Stripline" ),
                                               _( "Coupled Stripline" ),
                                               _( "Coplanar wave guide" ),
                                               _( "Coplanar wave guide w/ ground plane" ),
                                               _( "Rectangular Waveguide" ),
                                               _( "Coaxial Line" ),
                                               _( "Twisted Pair" ) };
    int      m_TranslineSelectionNChoices = sizeof( m_TranslineSelectionChoices ) / sizeof( wxString );
    m_TranslineSelection = new wxRadioBox( this, wxID_ANY, _("Transmission Line Type"), wxDefaultPosition, wxDefaultSize, m_TranslineSelectionNChoices, m_TranslineSelectionChoices, 1, wxRA_SPECIFY_COLS );
    m_TranslineSelection->SetSelection( 0 );
    bLeftSizer->Add( m_TranslineSelection, 0, wxTOP | wxRIGHT | wxLEFT | wxEXPAND, 5 );


    bLeftSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	m_translineBitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_translineBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM, 10 );


	bSizeTransline->Add( bLeftSizer, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSubstrateBoxSizer;
	sbSubstrateBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Substrate Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerSubstPrms;
	fgSizerSubstPrms = new wxFlexGridSizer( 9, 3, 3, 0 );
	fgSizerSubstPrms->AddGrowableCol( 1 );
	fgSizerSubstPrms->SetFlexibleDirection( wxBOTH );
	fgSizerSubstPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_EpsilonR_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Er:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EpsilonR_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_EpsilonR_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer441;
	bSizer441 = new wxBoxSizer( wxHORIZONTAL );

	m_Value_EpsilonR = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer441->Add( m_Value_EpsilonR, 1, wxEXPAND|wxLEFT, 5 );

	m_button_EpsilonR = new wxButton( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizer441->Add( m_button_EpsilonR, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerSubstPrms->Add( bSizer441, 1, wxEXPAND, 5 );


	fgSizerSubstPrms->Add( 0, 0, 1, wxEXPAND, 5 );

	m_TanD_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Tan delta:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TanD_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_TanD_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer442;
	bSizer442 = new wxBoxSizer( wxHORIZONTAL );

	m_Value_TanD = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer442->Add( m_Value_TanD, 1, wxEXPAND|wxLEFT, 5 );

	m_button_TanD = new wxButton( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizer442->Add( m_button_TanD, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerSubstPrms->Add( bSizer442, 1, wxEXPAND, 5 );


	fgSizerSubstPrms->Add( 0, 0, 1, wxEXPAND, 5 );

	m_Rho_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Rho:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Rho_label->Wrap( -1 );
	m_Rho_label->SetToolTip( _("Specific resistance in ohms * meters") );

	fgSizerSubstPrms->Add( m_Rho_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer443;
	bSizer443 = new wxBoxSizer( wxHORIZONTAL );

	m_Value_Rho = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer443->Add( m_Value_Rho, 1, wxEXPAND|wxLEFT, 5 );

	m_button_Rho = new wxButton( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizer443->Add( m_button_Rho, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerSubstPrms->Add( bSizer443, 1, wxEXPAND, 5 );

	m_substrate_prm3_labelUnit = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("ohm-meter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm3_labelUnit->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm3_labelUnit, 0, wxALL, 5 );

	m_substrate_prm4_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("H:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm4_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm4_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Substrate_prm4_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm4_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_SubsPrm4_choiceUnitChoices;
	m_SubsPrm4_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm4_choiceUnitChoices, 0 );
	m_SubsPrm4_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm4_choiceUnit, 0, wxEXPAND, 5 );

	m_substrate_prm5_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("H_t:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm5_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm5_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Substrate_prm5_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm5_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_SubsPrm5_choiceUnitChoices;
	m_SubsPrm5_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm5_choiceUnitChoices, 0 );
	m_SubsPrm5_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm5_choiceUnit, 0, wxEXPAND, 5 );

	m_substrate_prm6_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("T:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm6_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm6_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Substrate_prm6_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm6_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_SubsPrm6_choiceUnitChoices;
	m_SubsPrm6_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm6_choiceUnitChoices, 0 );
	m_SubsPrm6_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm6_choiceUnit, 0, wxEXPAND, 5 );

	m_substrate_prm7_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Rough:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm7_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm7_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Substrate_prm7_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm7_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_SubsPrm7_choiceUnitChoices;
	m_SubsPrm7_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm7_choiceUnitChoices, 0 );
	m_SubsPrm7_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm7_choiceUnit, 0, wxEXPAND, 5 );

	m_substrate_prm8_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Insulator mu:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm8_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm8_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Substrate_prm8_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm8_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_SubsPrm8_choiceUnitChoices;
	m_SubsPrm8_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm8_choiceUnitChoices, 0 );
	m_SubsPrm8_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm8_choiceUnit, 0, wxEXPAND, 5 );

	m_substrate_prm9_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Conductor mu:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm9_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm9_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Substrate_prm9_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm9_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_SubsPrm9_choiceUnitChoices;
	m_SubsPrm9_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm9_choiceUnitChoices, 0 );
	m_SubsPrm9_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm9_choiceUnit, 0, wxEXPAND, 5 );


	sbSubstrateBoxSizer->Add( fgSizerSubstPrms, 1, wxALL|wxEXPAND, 5 );


	bMiddleSizer->Add( sbSubstrateBoxSizer, 0, wxEXPAND|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbCmpPrmsSizer;
	sbCmpPrmsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Component Parameters") ), wxVERTICAL );

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
	fgSizeCmpPrms->Add( m_choiceUnit_Frequency, 0, wxEXPAND, 5 );


	sbCmpPrmsSizer->Add( fgSizeCmpPrms, 0, wxALL|wxEXPAND, 5 );


	bMiddleSizer->Add( sbCmpPrmsSizer, 0, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizerHelpBitmaps;
	bSizerHelpBitmaps = new wxBoxSizer( wxVERTICAL );

	m_bmCMicrostripZoddZeven = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerHelpBitmaps->Add( m_bmCMicrostripZoddZeven, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM, 10 );


	bMiddleSizer->Add( bSizerHelpBitmaps, 1, wxALIGN_CENTER_HORIZONTAL, 5 );


	bSizeTransline->Add( bMiddleSizer, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* btranslineRightSizer;
	btranslineRightSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Physical Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerPhysPrms;
	fgSizerPhysPrms = new wxFlexGridSizer( 4, 4, 3, 0 );
	fgSizerPhysPrms->AddGrowableCol( 1 );
	fgSizerPhysPrms->SetFlexibleDirection( wxBOTH );
	fgSizerPhysPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_phys_prm1_label = new wxStaticText( btranslineRightSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm1_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm1_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Phys_prm1_Value = new wxTextCtrl( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_Phys_prm1_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceUnit_Param1Choices;
	m_choiceUnit_Param1 = new UNIT_SELECTOR_LEN( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param1Choices, 0 );
	m_choiceUnit_Param1->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param1, 0, wxEXPAND, 5 );

	m_radioBtnPrm1 = new wxRadioButton( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizerPhysPrms->Add( m_radioBtnPrm1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_phys_prm2_label = new wxStaticText( btranslineRightSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm2_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm2_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Phys_prm2_Value = new wxTextCtrl( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_Phys_prm2_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceUnit_Param2Choices;
	m_choiceUnit_Param2 = new UNIT_SELECTOR_LEN( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param2Choices, 0 );
	m_choiceUnit_Param2->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param2, 0, wxEXPAND, 5 );

	m_radioBtnPrm2 = new wxRadioButton( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_radioBtnPrm2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_phys_prm3_label = new wxStaticText( btranslineRightSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm3_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm3_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Phys_prm3_Value = new wxTextCtrl( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_Phys_prm3_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceUnit_Param3Choices;
	m_choiceUnit_Param3 = new UNIT_SELECTOR_LEN( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param3Choices, 0 );
	m_choiceUnit_Param3->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param3, 0, wxEXPAND, 5 );


	fgSizerPhysPrms->Add( 0, 0, 0, 0, 5 );


	btranslineRightSizer->Add( fgSizerPhysPrms, 0, wxALL|wxEXPAND, 5 );


	bSizer11->Add( btranslineRightSizer, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* btranslineButtonsSizer;
	btranslineButtonsSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_AnalyseButton = new wxButton( this, wxID_ANY, _("Analyze"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_AnalyseButton, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_bpButtonAnalyze = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerButtons->Add( m_bpButtonAnalyze, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizerButtons->Add( 30, 0, 1, wxEXPAND, 5 );

	m_SynthetizeButton = new wxButton( this, wxID_ANY, _("Synthesize"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_SynthetizeButton, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_bpButtonSynthetize = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerButtons->Add( m_bpButtonSynthetize, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	btranslineButtonsSizer->Add( bSizerButtons, 0, wxLEFT, 5 );


	bSizer11->Add( btranslineButtonsSizer, 0, 0, 5 );

	wxStaticBoxSizer* sbElectricalResultsSizer;
	sbElectricalResultsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Electrical Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerResults;
	fgSizerResults = new wxFlexGridSizer( 3, 3, 3, 0 );
	fgSizerResults->AddGrowableCol( 1 );
	fgSizerResults->SetFlexibleDirection( wxBOTH );
	fgSizerResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_elec_prm1_label = new wxStaticText( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm1_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm1_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Elec_prm1_Value = new wxTextCtrl( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerResults->Add( m_Elec_prm1_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceUnit_ElecPrm1Choices;
	m_choiceUnit_ElecPrm1 = new UNIT_SELECTOR_RESISTOR( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm1Choices, 0 );
	m_choiceUnit_ElecPrm1->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm1, 0, wxEXPAND, 5 );

	m_elec_prm2_label = new wxStaticText( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm2_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm2_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Elec_prm2_Value = new wxTextCtrl( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerResults->Add( m_Elec_prm2_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceUnit_ElecPrm2Choices;
	m_choiceUnit_ElecPrm2 = new UNIT_SELECTOR_RESISTOR( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm2Choices, 0 );
	m_choiceUnit_ElecPrm2->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm2, 0, wxEXPAND, 5 );

	m_elec_prm3_label = new wxStaticText( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, _("Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm3_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm3_label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Elec_prm3_Value = new wxTextCtrl( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerResults->Add( m_Elec_prm3_Value, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceUnit_ElecPrm3Choices;
	m_choiceUnit_ElecPrm3 = new UNIT_SELECTOR_ANGLE( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm3Choices, 0 );
	m_choiceUnit_ElecPrm3->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm3, 0, 0, 5 );


	sbElectricalResultsSizer->Add( fgSizerResults, 0, wxALL|wxEXPAND, 5 );


	bSizer11->Add( sbElectricalResultsSizer, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbMessagesSizer;
	sbMessagesSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Results") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerTranslResults;
	fgSizerTranslResults = new wxFlexGridSizer( 10, 2, 4, 0 );
	fgSizerTranslResults->AddGrowableCol( 1 );
	fgSizerTranslResults->SetFlexibleDirection( wxBOTH );
	fgSizerTranslResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_left_message1 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message1->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Message1 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message1->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message1, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message2 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message2->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message2, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Message2 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message2->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message2, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_left_message3 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message3->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message3, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Message3 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message3->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message3, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message4 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message4->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message4, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Message4 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message4->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message4, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message5 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message5->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Message5 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message5->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message5, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message6 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message6->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message6, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Message6 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message6->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message6, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message7 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message7->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message7, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Message7 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message7->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message7, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message8 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message8->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message8, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Message8 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message8->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message8, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_left_message9 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message9->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message9, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Message9 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message9->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message9, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_left_message10 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message10->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message10, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Message10 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message10->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message10, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	sbMessagesSizer->Add( fgSizerTranslResults, 1, wxALL|wxEXPAND, 5 );


	bSizer11->Add( sbMessagesSizer, 0, wxEXPAND|wxALL, 5 );


	bRightSizer->Add( bSizer11, 0, 0, 5 );


	bRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonTransLineReset = new wxButton( this, wxID_ANY, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonTransLineReset, 0, wxALIGN_RIGHT|wxALL, 10 );


	bSizeTransline->Add( bRightSizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	this->SetSizer( bSizeTransline );
	this->Layout();
	bSizeTransline->Fit( this );

	// Connect Events
	m_TranslineSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineSelection ), NULL, this );
	m_button_EpsilonR->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineEpsilonR_Button ), NULL, this );
	m_button_TanD->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineTanD_Button ), NULL, this );
	m_button_Rho->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineRho_Button ), NULL, this );
	m_AnalyseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineAnalyse ), NULL, this );
	m_bpButtonAnalyze->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineAnalyse ), NULL, this );
	m_SynthetizeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineSynthetize ), NULL, this );
	m_bpButtonSynthetize->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineSynthetize ), NULL, this );
	m_buttonTransLineReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTransLineResetButtonClick ), NULL, this );
}

PANEL_TRANSLINE_BASE::~PANEL_TRANSLINE_BASE()
{
	// Disconnect Events
	m_TranslineSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineSelection ), NULL, this );
	m_button_EpsilonR->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineEpsilonR_Button ), NULL, this );
	m_button_TanD->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineTanD_Button ), NULL, this );
	m_button_Rho->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineRho_Button ), NULL, this );
	m_AnalyseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineAnalyse ), NULL, this );
	m_bpButtonAnalyze->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineAnalyse ), NULL, this );
	m_SynthetizeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineSynthetize ), NULL, this );
	m_bpButtonSynthetize->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTranslineSynthetize ), NULL, this );
	m_buttonTransLineReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRANSLINE_BASE::OnTransLineResetButtonClick ), NULL, this );

}

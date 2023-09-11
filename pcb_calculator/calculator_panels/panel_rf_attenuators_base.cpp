///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/html_window.h"
#include "widgets/std_bitmap_button.h"

#include "panel_rf_attenuators_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_RF_ATTENUATORS_BASE::PANEL_RF_ATTENUATORS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerAtt;
	bSizerAtt = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizerAtt;
	bLeftSizerAtt = new wxBoxSizer( wxVERTICAL );

	bLeftSizerAtt->SetMinSize( wxSize( 260,-1 ) );
	wxString m_AttenuatorsSelectionChoices[] = { _("Pi"), _("Tee"), _("Bridged tee"), _("Resistive splitter") };
	int m_AttenuatorsSelectionNChoices = sizeof( m_AttenuatorsSelectionChoices ) / sizeof( wxString );
	m_AttenuatorsSelection = new wxRadioBox( this, wxID_ANY, _("Attenuators"), wxDefaultPosition, wxDefaultSize, m_AttenuatorsSelectionNChoices, m_AttenuatorsSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_AttenuatorsSelection->SetSelection( 1 );
	bLeftSizerAtt->Add( m_AttenuatorsSelection, 0, wxEXPAND|wxALL, 5 );


	bLeftSizerAtt->Add( 0, 5, 0, wxEXPAND, 5 );

	m_attenuatorBitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizerAtt->Add( m_attenuatorBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM, 10 );


	bSizerAtt->Add( bLeftSizerAtt, 0, wxBOTTOM|wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bMiddleSizerAtt;
	bMiddleSizerAtt = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerAttPrms;
	sbSizerAttPrms = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerAttPrms;
	fgSizerAttPrms = new wxFlexGridSizer( 3, 3, 3, 0 );
	fgSizerAttPrms->AddGrowableRow( 1 );
	fgSizerAttPrms->SetFlexibleDirection( wxBOTH );
	fgSizerAttPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_attenuationLabel = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("Attenuation (a):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuationLabel->Wrap( -1 );
	fgSizerAttPrms->Add( m_attenuationLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_AttValueCtrl = new wxTextCtrl( sbSizerAttPrms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_AttValueCtrl->SetMinSize( wxSize( 100,-1 ) );

	fgSizerAttPrms->Add( m_AttValueCtrl, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_attUnit = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("dB"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attUnit->Wrap( -1 );
	fgSizerAttPrms->Add( m_attUnit, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_attenuationZinLabel = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("Zin:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuationZinLabel->Wrap( -1 );
	fgSizerAttPrms->Add( m_attenuationZinLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_ZinValueCtrl = new wxTextCtrl( sbSizerAttPrms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ZinValueCtrl->SetMinSize( wxSize( 100,-1 ) );

	fgSizerAttPrms->Add( m_ZinValueCtrl, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_attZinUnit = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attZinUnit->Wrap( -1 );
	fgSizerAttPrms->Add( m_attZinUnit, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_ZoutLabel = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("Zout:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ZoutLabel->Wrap( -1 );
	fgSizerAttPrms->Add( m_ZoutLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_ZoutValueCtrl = new wxTextCtrl( sbSizerAttPrms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ZoutValueCtrl->SetMinSize( wxSize( 100,-1 ) );

	fgSizerAttPrms->Add( m_ZoutValueCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_attZoutUnit = new wxStaticText( sbSizerAttPrms->GetStaticBox(), wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attZoutUnit->Wrap( -1 );
	fgSizerAttPrms->Add( m_attZoutUnit, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbSizerAttPrms->Add( fgSizerAttPrms, 0, wxALL|wxEXPAND, 5 );


	bMiddleSizerAtt->Add( sbSizerAttPrms, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bSizerAttButt;
	bSizerAttButt = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAlcAtt = new wxButton( this, wxID_ANY, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAlcAtt->SetMinSize( wxSize( 120,-1 ) );

	bSizerAttButt->Add( m_buttonAlcAtt, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_bpButtonCalcAtt = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerAttButt->Add( m_bpButtonCalcAtt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bMiddleSizerAtt->Add( bSizerAttButt, 0, 0, 5 );

	wxStaticBoxSizer* sbSizerAttValues;
	sbSizerAttValues = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Values") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerAttResults;
	fgSizerAttResults = new wxFlexGridSizer( 3, 3, 3, 0 );
	fgSizerAttResults->AddGrowableCol( 1 );
	fgSizerAttResults->SetFlexibleDirection( wxBOTH );
	fgSizerAttResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_attenuatorR1Label = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("R1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuatorR1Label->Wrap( -1 );
	fgSizerAttResults->Add( m_attenuatorR1Label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Att_R1_Value = new wxTextCtrl( sbSizerAttValues->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttResults->Add( m_Att_R1_Value, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_attR1Unit = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attR1Unit->Wrap( -1 );
	fgSizerAttResults->Add( m_attR1Unit, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_attenuatorR2Label = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("R2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuatorR2Label->Wrap( -1 );
	fgSizerAttResults->Add( m_attenuatorR2Label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Att_R2_Value = new wxTextCtrl( sbSizerAttValues->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttResults->Add( m_Att_R2_Value, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_attR2Unit = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attR2Unit->Wrap( -1 );
	fgSizerAttResults->Add( m_attR2Unit, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_attenuatorR3Label = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("R3:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attenuatorR3Label->Wrap( -1 );
	fgSizerAttResults->Add( m_attenuatorR3Label, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Att_R3_Value = new wxTextCtrl( sbSizerAttValues->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttResults->Add( m_Att_R3_Value, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_attR3Unit = new wxStaticText( sbSizerAttValues->GetStaticBox(), wxID_ANY, _("Ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_attR3Unit->Wrap( -1 );
	fgSizerAttResults->Add( m_attR3Unit, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbSizerAttValues->Add( fgSizerAttResults, 0, wxALL|wxEXPAND, 5 );


	bMiddleSizerAtt->Add( sbSizerAttValues, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bSizerMessages;
	bSizerMessages = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerIndentLabel;
	bSizerIndentLabel = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextAttMsg = new wxStaticText( this, wxID_ANY, _("Messages"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAttMsg->Wrap( -1 );
	bSizerIndentLabel->Add( m_staticTextAttMsg, 0, wxALL, 2 );


	bSizerMessages->Add( bSizerIndentLabel, 0, wxLEFT, 6 );

	m_Attenuator_Messages = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_NO_SELECTION|wxHW_SCROLLBAR_AUTO );
	bSizerMessages->Add( m_Attenuator_Messages, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 8 );


	bMiddleSizerAtt->Add( bSizerMessages, 1, wxEXPAND|wxLEFT, 3 );


	bSizerAtt->Add( bMiddleSizerAtt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbRightSizerFormula;
	sbRightSizerFormula = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Formula") ), wxVERTICAL );

	m_panelAttFormula = new HTML_WINDOW( sbRightSizerFormula->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_panelAttFormula->SetMinSize( wxSize( 200,-1 ) );

	sbRightSizerFormula->Add( m_panelAttFormula, 1, wxALL|wxEXPAND, 5 );


	bSizer7->Add( sbRightSizerFormula, 1, wxALL|wxEXPAND, 5 );


	bSizerAtt->Add( bSizer7, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	this->SetSizer( bSizerAtt );
	this->Layout();
	bSizerAtt->Fit( this );

	// Connect Events
	m_AttenuatorsSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PANEL_RF_ATTENUATORS_BASE::OnAttenuatorSelection ), NULL, this );
	m_buttonAlcAtt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_RF_ATTENUATORS_BASE::OnCalculateAttenuator ), NULL, this );
	m_bpButtonCalcAtt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_RF_ATTENUATORS_BASE::OnCalculateAttenuator ), NULL, this );
}

PANEL_RF_ATTENUATORS_BASE::~PANEL_RF_ATTENUATORS_BASE()
{
	// Disconnect Events
	m_AttenuatorsSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PANEL_RF_ATTENUATORS_BASE::OnAttenuatorSelection ), NULL, this );
	m_buttonAlcAtt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_RF_ATTENUATORS_BASE::OnCalculateAttenuator ), NULL, this );
	m_bpButtonCalcAtt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_RF_ATTENUATORS_BASE::OnCalculateAttenuator ), NULL, this );

}

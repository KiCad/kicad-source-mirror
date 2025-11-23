///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/unit_selector.h"

#include "panel_track_width_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_TRACK_WIDTH_BASE::PANEL_TRACK_WIDTH_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerTrackWidth;
	bSizerTrackWidth = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizeLeft;
	bSizeLeft = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerTW_Prms;
	sbSizerTW_Prms = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Parameters") ), wxHORIZONTAL );

	wxFlexGridSizer* fgSizerTWprms;
	fgSizerTWprms = new wxFlexGridSizer( 4, 3, 0, 0 );
	fgSizerTWprms->AddGrowableCol( 1 );
	fgSizerTWprms->SetFlexibleDirection( wxBOTH );
	fgSizerTWprms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextCurrent = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("Current (I):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCurrent->Wrap( -1 );
	fgSizerTWprms->Add( m_staticTextCurrent, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_TrackCurrentValue = new wxTextCtrl( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTWprms->Add( m_TrackCurrentValue, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticText62 = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText62->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText62, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_staticText63 = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("Temperature rise:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText63->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText63, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_TrackDeltaTValue = new wxTextCtrl( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTWprms->Add( m_TrackDeltaTValue, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_trackTempUnits = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("deg C"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackTempUnits->Wrap( -1 );
	fgSizerTWprms->Add( m_trackTempUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_staticText66 = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("Conductor length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText66->SetLabelMarkup( _("Conductor length:") );
	m_staticText66->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText66, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_TrackLengthValue = new wxTextCtrl( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTWprms->Add( m_TrackLengthValue, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_TW_CuLength_choiceUnitChoices;
	m_TW_CuLength_choiceUnit = new UNIT_SELECTOR_LEN( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TW_CuLength_choiceUnitChoices, 0 );
	m_TW_CuLength_choiceUnit->SetSelection( 0 );
	fgSizerTWprms->Add( m_TW_CuLength_choiceUnit, 0, wxBOTTOM|wxEXPAND, 5 );

	m_staticText103 = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("Copper resistivity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText103->Wrap( -1 );
	fgSizerTWprms->Add( m_staticText103, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_TWResistivity = new wxTextCtrl( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_TWResistivity->Enable( false );

	fgSizerTWprms->Add( m_TWResistivity, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_resistivityUnits = new wxStaticText( sbSizerTW_Prms->GetStaticBox(), wxID_ANY, _("ohm-meter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_resistivityUnits->Wrap( -1 );
	fgSizerTWprms->Add( m_resistivityUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbSizerTW_Prms->Add( fgSizerTWprms, 0, wxALL|wxEXPAND, 5 );


	bSizeLeft->Add( sbSizerTW_Prms, 0, wxALL|wxEXPAND, 5 );

	m_htmlWinFormulas = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_NO_SELECTION|wxHW_SCROLLBAR_AUTO );
	bSizeLeft->Add( m_htmlWinFormulas, 1, wxEXPAND|wxALL, 8 );


	bSizerTrackWidth->Add( bSizeLeft, 1, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizeRight;
	bSizeRight = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerTW_Result;
	sbSizerTW_Result = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("External Layer Tracks") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerTW_Results;
	fgSizerTW_Results = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerTW_Results->AddGrowableCol( 1 );
	fgSizerTW_Results->SetFlexibleDirection( wxBOTH );
	fgSizerTW_Results->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextExtWidth = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Track width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextExtWidth->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticTextExtWidth, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ExtTrackWidthValue = new wxTextCtrl( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTW_Results->Add( m_ExtTrackWidthValue, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_TW_ExtTrackWidth_choiceUnitChoices;
	m_TW_ExtTrackWidth_choiceUnit = new UNIT_SELECTOR_LEN( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TW_ExtTrackWidth_choiceUnitChoices, 0 );
	m_TW_ExtTrackWidth_choiceUnit->SetSelection( 0 );
	fgSizerTW_Results->Add( m_TW_ExtTrackWidth_choiceUnit, 0, wxEXPAND, 5 );

	m_staticText65 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Track thickness (H):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText65->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText65, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ExtTrackThicknessValue = new wxTextCtrl( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTW_Results->Add( m_ExtTrackThicknessValue, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_ExtTrackThicknessUnitChoices;
	m_ExtTrackThicknessUnit = new UNIT_SELECTOR_THICKNESS( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ExtTrackThicknessUnitChoices, 0 );
	m_ExtTrackThicknessUnit->SetSelection( 0 );
	fgSizerTW_Results->Add( m_ExtTrackThicknessUnit, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	m_staticline3 = new wxStaticLine( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results->Add( m_staticline3, 0, wxEXPAND|wxBOTTOM, 5 );

	m_staticline4 = new wxStaticLine( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results->Add( m_staticline4, 0, wxEXPAND|wxBOTTOM, 5 );

	m_staticline5 = new wxStaticLine( sbSizerTW_Result->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results->Add( m_staticline5, 0, wxEXPAND|wxBOTTOM, 5 );

	m_staticTextArea = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Cross-section area:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextArea->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticTextArea, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ExtTrackAreaValue = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackAreaValue->Wrap( -1 );
	fgSizerTW_Results->Add( m_ExtTrackAreaValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );

	m_extTrackAreaUnitLabel = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("mm ^ 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_extTrackAreaUnitLabel->Wrap( -1 );
	fgSizerTW_Results->Add( m_extTrackAreaUnitLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticText651 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText651->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText651, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_ExtTrackResistValue = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackResistValue->Wrap( -1 );
	fgSizerTW_Results->Add( m_ExtTrackResistValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_extTrackResUnits = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_extTrackResUnits->Wrap( -1 );
	fgSizerTW_Results->Add( m_extTrackResUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText661 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Voltage drop:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText661->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText661, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_ExtTrackVDropValue = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackVDropValue->Wrap( -1 );
	fgSizerTW_Results->Add( m_ExtTrackVDropValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticText83 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText83->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText83, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText79 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("Power loss:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText79->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText79, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ExtTrackLossValue = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExtTrackLossValue->Wrap( -1 );
	fgSizerTW_Results->Add( m_ExtTrackLossValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticText791 = new wxStaticText( sbSizerTW_Result->GetStaticBox(), wxID_ANY, _("W"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText791->Wrap( -1 );
	fgSizerTW_Results->Add( m_staticText791, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerTW_Result->Add( fgSizerTW_Results, 0, wxALL|wxEXPAND, 5 );


	bSizeRight->Add( sbSizerTW_Result, 1, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSizerTW_Result1;
	sbSizerTW_Result1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Internal Layer Tracks") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerTW_Results1;
	fgSizerTW_Results1 = new wxFlexGridSizer( 7, 3, 0, 0 );
	fgSizerTW_Results1->AddGrowableCol( 1 );
	fgSizerTW_Results1->SetFlexibleDirection( wxBOTH );
	fgSizerTW_Results1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextIntWidth = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Track width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextIntWidth->Wrap( -1 );
	m_staticTextIntWidth->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	fgSizerTW_Results1->Add( m_staticTextIntWidth, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_IntTrackWidthValue = new wxTextCtrl( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTW_Results1->Add( m_IntTrackWidthValue, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_TW_IntTrackWidth_choiceUnitChoices;
	m_TW_IntTrackWidth_choiceUnit = new UNIT_SELECTOR_LEN( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TW_IntTrackWidth_choiceUnitChoices, 0 );
	m_TW_IntTrackWidth_choiceUnit->SetSelection( 0 );
	fgSizerTW_Results1->Add( m_TW_IntTrackWidth_choiceUnit, 0, wxEXPAND, 5 );

	m_staticText652 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Track thickness (H):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText652->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText652, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_IntTrackThicknessValue = new wxTextCtrl( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTW_Results1->Add( m_IntTrackThicknessValue, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	wxArrayString m_IntTrackThicknessUnitChoices;
	m_IntTrackThicknessUnit = new UNIT_SELECTOR_THICKNESS( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_IntTrackThicknessUnitChoices, 0 );
	m_IntTrackThicknessUnit->SetSelection( 0 );
	fgSizerTW_Results1->Add( m_IntTrackThicknessUnit, 0, wxBOTTOM|wxTOP, 5 );

	m_staticline8 = new wxStaticLine( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results1->Add( m_staticline8, 0, wxEXPAND|wxBOTTOM, 5 );

	m_staticline9 = new wxStaticLine( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results1->Add( m_staticline9, 0, wxEXPAND|wxBOTTOM, 5 );

	m_staticline10 = new wxStaticLine( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerTW_Results1->Add( m_staticline10, 0, wxEXPAND|wxBOTTOM, 5 );

	m_staticTextArea1 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Cross-section area:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextArea1->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticTextArea1, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );

	m_IntTrackAreaValue = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackAreaValue->Wrap( -1 );
	fgSizerTW_Results1->Add( m_IntTrackAreaValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );

	m_intTrackAreaUnitLabel = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("mm ^ 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_intTrackAreaUnitLabel->Wrap( -1 );
	fgSizerTW_Results1->Add( m_intTrackAreaUnitLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticText6511 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6511->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText6511, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_IntTrackResistValue = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackResistValue->Wrap( -1 );
	fgSizerTW_Results1->Add( m_IntTrackResistValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_intTrackResUnits = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_intTrackResUnits->Wrap( -1 );
	fgSizerTW_Results1->Add( m_intTrackResUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText6611 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Voltage drop:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6611->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText6611, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_IntTrackVDropValue = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackVDropValue->Wrap( -1 );
	fgSizerTW_Results1->Add( m_IntTrackVDropValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticText831 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText831->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText831, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText792 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("Power loss:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText792->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText792, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_IntTrackLossValue = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IntTrackLossValue->Wrap( -1 );
	fgSizerTW_Results1->Add( m_IntTrackLossValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticText7911 = new wxStaticText( sbSizerTW_Result1->GetStaticBox(), wxID_ANY, _("W"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7911->Wrap( -1 );
	fgSizerTW_Results1->Add( m_staticText7911, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerTW_Result1->Add( fgSizerTW_Results1, 0, wxALL|wxEXPAND, 5 );


	bSizeRight->Add( sbSizerTW_Result1, 1, wxEXPAND|wxALL, 5 );

	m_buttonTrackWidthReset = new wxButton( this, wxID_ANY, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizeRight->Add( m_buttonTrackWidthReset, 0, wxALIGN_RIGHT|wxALL, 5 );


	bSizerTrackWidth->Add( bSizeRight, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	this->SetSizer( bSizerTrackWidth );
	this->Layout();
	bSizerTrackWidth->Fit( this );

	// Connect Events
	m_TrackCurrentValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWCalculateFromCurrent ), NULL, this );
	m_TrackDeltaTValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_TrackLengthValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_TW_CuLength_choiceUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_TWResistivity->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackWidthValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWCalculateFromExtWidth ), NULL, this );
	m_TW_ExtTrackWidth_choiceUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackThicknessValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackThicknessUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackWidthValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWCalculateFromIntWidth ), NULL, this );
	m_TW_IntTrackWidth_choiceUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackThicknessValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackThicknessUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_buttonTrackWidthReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWResetButtonClick ), NULL, this );
}

PANEL_TRACK_WIDTH_BASE::~PANEL_TRACK_WIDTH_BASE()
{
	// Disconnect Events
	m_TrackCurrentValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWCalculateFromCurrent ), NULL, this );
	m_TrackDeltaTValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_TrackLengthValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_TW_CuLength_choiceUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_TWResistivity->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackWidthValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWCalculateFromExtWidth ), NULL, this );
	m_TW_ExtTrackWidth_choiceUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackThicknessValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_ExtTrackThicknessUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackWidthValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWCalculateFromIntWidth ), NULL, this );
	m_TW_IntTrackWidth_choiceUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackThicknessValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_IntTrackThicknessUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWParametersChanged ), NULL, this );
	m_buttonTrackWidthReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TRACK_WIDTH_BASE::OnTWResetButtonClick ), NULL, this );

}

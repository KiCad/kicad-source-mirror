///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_render_job_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_RENDER_JOB_BASE::DIALOG_RENDER_JOB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerTop;
	fgSizerTop = new wxFlexGridSizer( 0, 2, 3, 5 );
	fgSizerTop->AddGrowableCol( 1 );
	fgSizerTop->SetFlexibleDirection( wxBOTH );
	fgSizerTop->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textOutputPath = new wxStaticText( this, wxID_ANY, _("Output file:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizerTop->Add( m_textOutputPath, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_textCtrlOutputFile = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputFile->SetMinSize( wxSize( 350,-1 ) );

	fgSizerTop->Add( m_textCtrlOutputFile, 0, wxEXPAND, 5 );

	m_formatLabel = new wxStaticText( this, wxID_ANY, _("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_formatLabel->Wrap( -1 );
	fgSizerTop->Add( m_formatLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxArrayString m_choiceFormatChoices;
	m_choiceFormat = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFormatChoices, 0 );
	m_choiceFormat->SetSelection( 0 );
	fgSizerTop->Add( m_choiceFormat, 0, 0, 5 );

	m_dimensionsLabel = new wxStaticText( this, wxID_ANY, _("Dimensions:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dimensionsLabel->Wrap( -1 );
	fgSizerTop->Add( m_dimensionsLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxBoxSizer* bSizerDimensions;
	bSizerDimensions = new wxBoxSizer( wxHORIZONTAL );

	m_spinCtrlWidth = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 128, 32767, 0 );
	bSizerDimensions->Add( m_spinCtrlWidth, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText17 = new wxStaticText( this, wxID_ANY, _("px"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	bSizerDimensions->Add( m_staticText17, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticText19 = new wxStaticText( this, wxID_ANY, _("x"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	bSizerDimensions->Add( m_staticText19, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_spinCtrlHeight = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 128, 32767, 0 );
	bSizerDimensions->Add( m_spinCtrlHeight, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText182 = new wxStaticText( this, wxID_ANY, _("px"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText182->Wrap( -1 );
	bSizerDimensions->Add( m_staticText182, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerTop->Add( bSizerDimensions, 1, wxEXPAND, 5 );

	m_presetLabel = new wxStaticText( this, wxID_ANY, _("Appearance preset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_presetLabel->Wrap( -1 );
	fgSizerTop->Add( m_presetLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_presetCtrlChoices[] = { _("Default"), _("Follow PCB Editor"), _("Follow PCB Plot Settings") };
	int m_presetCtrlNChoices = sizeof( m_presetCtrlChoices ) / sizeof( wxString );
	m_presetCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_presetCtrlNChoices, m_presetCtrlChoices, 0 );
	m_presetCtrl->SetSelection( 0 );
	fgSizerTop->Add( m_presetCtrl, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_backgroundStyleLabel = new wxStaticText( this, wxID_ANY, _("Background style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundStyleLabel->Wrap( -1 );
	fgSizerTop->Add( m_backgroundStyleLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxArrayString m_choiceBgStyleChoices;
	m_choiceBgStyle = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceBgStyleChoices, 0 );
	m_choiceBgStyle->SetSelection( 0 );
	fgSizerTop->Add( m_choiceBgStyle, 0, 0, 5 );


	bSizerMain->Add( fgSizerTop, 1, wxALL|wxEXPAND, 10 );

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 1, 6, 0 );

	m_cbUseBoardStackupColors = new wxCheckBox( this, wxID_ANY, _("Use board stackup colors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUseBoardStackupColors->SetValue(true);
	gSizer1->Add( m_cbUseBoardStackupColors, 0, wxRIGHT|wxLEFT, 5 );

	m_cbRaytracing_proceduralTextures = new wxCheckBox( this, wxID_ANY, _("Procedural textures"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRaytracing_proceduralTextures->SetValue(true);
	gSizer1->Add( m_cbRaytracing_proceduralTextures, 0, wxRIGHT|wxLEFT, 5 );

	m_cbRaytracing_addFloor = new wxCheckBox( this, wxID_ANY, _("Add floor"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRaytracing_addFloor->SetValue(true);
	gSizer1->Add( m_cbRaytracing_addFloor, 0, wxRIGHT|wxLEFT, 5 );

	m_cbRaytracing_antiAliasing = new wxCheckBox( this, wxID_ANY, _("Anti-aliasing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRaytracing_antiAliasing->SetValue(true);
	gSizer1->Add( m_cbRaytracing_antiAliasing, 0, wxRIGHT|wxLEFT, 5 );

	m_cbRaytracing_postProcessing = new wxCheckBox( this, wxID_ANY, _("Screen space ambient occlusions and global illumination reflections"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRaytracing_postProcessing->SetValue(true);
	gSizer1->Add( m_cbRaytracing_postProcessing, 0, wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( gSizer1, 0, wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizerViewProjection;
	bSizerViewProjection = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("View") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 5, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_sideLabel = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Side:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sideLabel->Wrap( -1 );
	gbSizer1->Add( m_sideLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxArrayString m_choiceSideChoices;
	m_choiceSide = new wxChoice( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceSideChoices, 0 );
	m_choiceSide->SetSelection( 0 );
	gbSizer1->Add( m_choiceSide, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), 0, 5 );

	m_zoomLabel = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Zoom:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_zoomLabel->Wrap( -1 );
	gbSizer1->Add( m_zoomLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_spinCtrlZoom = new wxSpinCtrlDouble( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 1, 0.1 );
	m_spinCtrlZoom->SetDigits( 2 );
	gbSizer1->Add( m_spinCtrlZoom, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer1->Add( gbSizer1, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerViewProjection->Add( sbSizer1, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxString m_radioProjectionChoices[] = { _("Perspective"), _("Orthogonal") };
	int m_radioProjectionNChoices = sizeof( m_radioProjectionChoices ) / sizeof( wxString );
	m_radioProjection = new wxRadioBox( this, wxID_ANY, _("Projection"), wxDefaultPosition, wxDefaultSize, m_radioProjectionNChoices, m_radioProjectionChoices, 1, wxRA_SPECIFY_COLS );
	m_radioProjection->SetSelection( 1 );
	bSizerViewProjection->Add( m_radioProjection, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerViewProjection, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbSizerPositioning;
	sbSizerPositioning = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Positioning") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 5, 5, 5 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_labelX = new wxStaticText( sbSizerPositioning->GetStaticBox(), wxID_ANY, _("X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelX->Wrap( -1 );
	fgSizer2->Add( m_labelX, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

	m_labelY = new wxStaticText( sbSizerPositioning->GetStaticBox(), wxID_ANY, _("Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelY->Wrap( -1 );
	fgSizer2->Add( m_labelY, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

	m_labelZ = new wxStaticText( sbSizerPositioning->GetStaticBox(), wxID_ANY, _("Z"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelZ->Wrap( -1 );
	fgSizer2->Add( m_labelZ, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_labelxx = new wxStaticText( sbSizerPositioning->GetStaticBox(), wxID_ANY, _("Pivot:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelxx->Wrap( -1 );
	fgSizer2->Add( m_labelxx, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_spinCtrlPivotX = new wxSpinCtrlDouble( sbSizerPositioning->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, 0, 1 );
	m_spinCtrlPivotX->SetDigits( 3 );
	fgSizer2->Add( m_spinCtrlPivotX, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlPivotY = new wxSpinCtrlDouble( sbSizerPositioning->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, 0, 1 );
	m_spinCtrlPivotY->SetDigits( 3 );
	fgSizer2->Add( m_spinCtrlPivotY, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spinCtrlPivotZ = new wxSpinCtrlDouble( sbSizerPositioning->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, 0, 1 );
	m_spinCtrlPivotZ->SetDigits( 3 );
	fgSizer2->Add( m_spinCtrlPivotZ, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_labelMM1 = new wxStaticText( sbSizerPositioning->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelMM1->Wrap( -1 );
	fgSizer2->Add( m_labelMM1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_labelxx2 = new wxStaticText( sbSizerPositioning->GetStaticBox(), wxID_ANY, _("Pan:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelxx2->Wrap( -1 );
	fgSizer2->Add( m_labelxx2, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_spinCtrlPanX = new wxSpinCtrlDouble( sbSizerPositioning->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, 0, 1 );
	m_spinCtrlPanX->SetDigits( 3 );
	fgSizer2->Add( m_spinCtrlPanX, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlPanY = new wxSpinCtrlDouble( sbSizerPositioning->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, 0, 1 );
	m_spinCtrlPanY->SetDigits( 3 );
	fgSizer2->Add( m_spinCtrlPanY, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spinCtrlPanZ = new wxSpinCtrlDouble( sbSizerPositioning->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, 0, 1 );
	m_spinCtrlPanZ->SetDigits( 3 );
	fgSizer2->Add( m_spinCtrlPanZ, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_labelMM2 = new wxStaticText( sbSizerPositioning->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelMM2->Wrap( -1 );
	fgSizer2->Add( m_labelMM2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_labelxx21 = new wxStaticText( sbSizerPositioning->GetStaticBox(), wxID_ANY, _("Rotation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelxx21->Wrap( -1 );
	fgSizer2->Add( m_labelxx21, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_spinCtrlRotX = new wxSpinCtrlDouble( sbSizerPositioning->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, 0, 1 );
	m_spinCtrlRotX->SetDigits( 3 );
	fgSizer2->Add( m_spinCtrlRotX, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlRotY = new wxSpinCtrlDouble( sbSizerPositioning->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, 0, 1 );
	m_spinCtrlRotY->SetDigits( 3 );
	fgSizer2->Add( m_spinCtrlRotY, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spinCtrlRotZ = new wxSpinCtrlDouble( sbSizerPositioning->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, 0, 1 );
	m_spinCtrlRotZ->SetDigits( 3 );
	fgSizer2->Add( m_spinCtrlRotZ, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_labelDeg1 = new wxStaticText( sbSizerPositioning->GetStaticBox(), wxID_ANY, _("°"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelDeg1->Wrap( -1 );
	fgSizer2->Add( m_labelDeg1, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbSizerPositioning->Add( fgSizer2, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( sbSizerPositioning, 0, wxALL|wxEXPAND, 10 );

	wxBoxSizer* bSizerLights;
	bSizerLights = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerLightsIntensity;
	sbSizerLightsIntensity = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Lights Intensity") ), wxVERTICAL );

	wxGridBagSizer* gbSizer11;
	gbSizer11 = new wxGridBagSizer( 5, 5 );
	gbSizer11->SetFlexibleDirection( wxBOTH );
	gbSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_TopLabel = new wxStaticText( sbSizerLightsIntensity->GetStaticBox(), wxID_ANY, _("Top:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TopLabel->Wrap( -1 );
	gbSizer11->Add( m_TopLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlLightsTop = new wxSpinCtrlDouble( sbSizerLightsIntensity->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1, 0, 0.1 );
	m_spinCtrlLightsTop->SetDigits( 2 );
	gbSizer11->Add( m_spinCtrlLightsTop, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_bottomLabel = new wxStaticText( sbSizerLightsIntensity->GetStaticBox(), wxID_ANY, _("Bottom:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bottomLabel->Wrap( -1 );
	gbSizer11->Add( m_bottomLabel, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_spinCtrlLightsBottom = new wxSpinCtrlDouble( sbSizerLightsIntensity->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1, 0, 0.1 );
	m_spinCtrlLightsBottom->SetDigits( 2 );
	gbSizer11->Add( m_spinCtrlLightsBottom, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_sidesLabel = new wxStaticText( sbSizerLightsIntensity->GetStaticBox(), wxID_ANY, _("Side:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sidesLabel->Wrap( -1 );
	gbSizer11->Add( m_sidesLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlLightsSides = new wxSpinCtrlDouble( sbSizerLightsIntensity->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1, 0.5, 0.1 );
	m_spinCtrlLightsSides->SetDigits( 2 );
	gbSizer11->Add( m_spinCtrlLightsSides, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_cameraLabel = new wxStaticText( sbSizerLightsIntensity->GetStaticBox(), wxID_ANY, _("Camera:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cameraLabel->Wrap( -1 );
	gbSizer11->Add( m_cameraLabel, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_spinCtrlLightsCamera = new wxSpinCtrlDouble( sbSizerLightsIntensity->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1, 0, 0.1 );
	m_spinCtrlLightsCamera->SetDigits( 2 );
	gbSizer11->Add( m_spinCtrlLightsCamera, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	gbSizer11->AddGrowableCol( 1 );
	gbSizer11->AddGrowableCol( 3 );

	sbSizerLightsIntensity->Add( gbSizer11, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerLights->Add( sbSizerLightsIntensity, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbSizerLightsPosition;
	sbSizerLightsPosition = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Lights Position") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelSideLightsElevation = new wxStaticText( sbSizerLightsPosition->GetStaticBox(), wxID_ANY, _("Side lights elevation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelSideLightsElevation->Wrap( -1 );
	fgSizer3->Add( m_labelSideLightsElevation, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_spinCtrlLightsSideElevation = new wxSpinCtrl( sbSizerLightsPosition->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 90, 60 );
	fgSizer3->Add( m_spinCtrlLightsSideElevation, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_labelDegrees = new wxStaticText( sbSizerLightsPosition->GetStaticBox(), wxID_ANY, _("°"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelDegrees->Wrap( -1 );
	fgSizer3->Add( m_labelDegrees, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbSizerLightsPosition->Add( fgSizer3, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerLights->Add( sbSizerLightsPosition, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerLights, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_choiceFormat->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
	m_choiceBgStyle->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
	m_choiceSide->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
}

DIALOG_RENDER_JOB_BASE::~DIALOG_RENDER_JOB_BASE()
{
	// Disconnect Events
	m_choiceFormat->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
	m_choiceBgStyle->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
	m_choiceSide->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );

}

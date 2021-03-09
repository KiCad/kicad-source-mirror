///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_3D_view_option_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_3D_VIEW_OPTIONS_BASE::DIALOG_3D_VIEW_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelDspOpt = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerDisplayOptions;
	bSizerDisplayOptions = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizeLeft;
	bSizeLeft = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbModelVisibility;
	sbModelVisibility = new wxStaticBoxSizer( new wxStaticBox( m_panelDspOpt, wxID_ANY, _("3D Model Visibility") ), wxVERTICAL );

	m_checkBox3DshapesTH = new wxCheckBox( sbModelVisibility->GetStaticBox(), wxID_ANY, _("Show 3D through hole models"), wxDefaultPosition, wxDefaultSize, 0 );
	sbModelVisibility->Add( m_checkBox3DshapesTH, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBox3DshapesSMD = new wxCheckBox( sbModelVisibility->GetStaticBox(), wxID_ANY, _("Show 3D SMD models"), wxDefaultPosition, wxDefaultSize, 0 );
	sbModelVisibility->Add( m_checkBox3DshapesSMD, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBox3DshapesVirtual = new wxCheckBox( sbModelVisibility->GetStaticBox(), wxID_ANY, _("Show 3D virtual models"), wxDefaultPosition, wxDefaultSize, 0 );
	sbModelVisibility->Add( m_checkBox3DshapesVirtual, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizeLeft->Add( sbModelVisibility, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbBoardLayers;
	sbBoardLayers = new wxStaticBoxSizer( new wxStaticBox( m_panelDspOpt, wxID_ANY, _("Board Layers") ), wxVERTICAL );

	m_checkBoxSilkscreen = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show silkscreen layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbBoardLayers->Add( m_checkBoxSilkscreen, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxSolderMask = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show solder mask layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbBoardLayers->Add( m_checkBoxSolderMask, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxSolderpaste = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show solder paste layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbBoardLayers->Add( m_checkBoxSolderpaste, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxAdhesive = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show adhesive layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbBoardLayers->Add( m_checkBoxAdhesive, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizeLeft->Add( sbBoardLayers, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbUserLayers;
	sbUserLayers = new wxStaticBoxSizer( new wxStaticBox( m_panelDspOpt, wxID_ANY, _("User Layers (not shown in realistic mode)") ), wxVERTICAL );

	m_checkBoxComments = new wxCheckBox( sbUserLayers->GetStaticBox(), wxID_ANY, _("Show comments and drawings layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbUserLayers->Add( m_checkBoxComments, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxECO = new wxCheckBox( sbUserLayers->GetStaticBox(), wxID_ANY, _("Show ECO layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbUserLayers->Add( m_checkBoxECO, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizeLeft->Add( sbUserLayers, 0, wxALL|wxEXPAND, 5 );


	bSizerDisplayOptions->Add( bSizeLeft, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbRenderOptions;
	sbRenderOptions = new wxStaticBoxSizer( new wxStaticBox( m_panelDspOpt, wxID_ANY, _("Render Options") ), wxVERTICAL );

	m_checkBoxRealisticMode = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Realistic mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRenderOptions->Add( m_checkBoxRealisticMode, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxBoardBody = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Show board body"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRenderOptions->Add( m_checkBoxBoardBody, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxAreas = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Show filled areas in zones"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRenderOptions->Add( m_checkBoxAreas, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxSubtractMaskFromSilk = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Subtract soldermask from silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRenderOptions->Add( m_checkBoxSubtractMaskFromSilk, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxClipSilkOnViaAnnulus = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Clip silkscreen at via annulus"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRenderOptions->Add( m_checkBoxClipSilkOnViaAnnulus, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxRenderPlatedPadsAsPlated = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Use bare copper color for unplated copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRenderPlatedPadsAsPlated->SetToolTip( _("Use different colors for plated and unplated copper. (Slow)") );

	sbRenderOptions->Add( m_checkBoxRenderPlatedPadsAsPlated, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerRight->Add( sbRenderOptions, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbCameraOptions;
	sbCameraOptions = new wxStaticBoxSizer( new wxStaticBox( m_panelDspOpt, wxID_ANY, _("Camera Options") ), wxVERTICAL );

	wxBoxSizer* bSizerRotAngle;
	bSizerRotAngle = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextRotAngle = new wxStaticText( sbCameraOptions->GetStaticBox(), wxID_ANY, _("Rotation Increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotAngle->Wrap( -1 );
	bSizerRotAngle->Add( m_staticTextRotAngle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlRotationAngle = new wxSpinCtrlDouble( sbCameraOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 359, 10, 1 );
	m_spinCtrlRotationAngle->SetDigits( 0 );
	bSizerRotAngle->Add( m_spinCtrlRotationAngle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticTextRotAngleUnits = new wxStaticText( sbCameraOptions->GetStaticBox(), wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotAngleUnits->Wrap( -1 );
	bSizerRotAngle->Add( m_staticTextRotAngleUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	sbCameraOptions->Add( bSizerRotAngle, 0, wxEXPAND|wxBOTTOM, 5 );

	m_staticline3 = new wxStaticLine( sbCameraOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbCameraOptions->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_checkBoxEnableAnimation = new wxCheckBox( sbCameraOptions->GetStaticBox(), wxID_ANY, _("Enable animation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxEnableAnimation->SetValue(true);
	sbCameraOptions->Add( m_checkBoxEnableAnimation, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerSlider;
	bSizerSlider = new wxBoxSizer( wxHORIZONTAL );

	m_staticAnimationSpeed = new wxStaticText( sbCameraOptions->GetStaticBox(), wxID_ANY, _("Animation speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticAnimationSpeed->Wrap( -1 );
	bSizerSlider->Add( m_staticAnimationSpeed, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_sliderAnimationSpeed = new wxSlider( sbCameraOptions->GetStaticBox(), wxID_ANY, 3, 1, 5, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	m_sliderAnimationSpeed->SetMinSize( wxSize( 100,-1 ) );

	bSizerSlider->Add( m_sliderAnimationSpeed, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	sbCameraOptions->Add( bSizerSlider, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bSizerRight->Add( sbCameraOptions, 0, wxALL|wxEXPAND, 5 );


	bSizerDisplayOptions->Add( bSizerRight, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_panelDspOpt->SetSizer( bSizerDisplayOptions );
	m_panelDspOpt->Layout();
	bSizerDisplayOptions->Fit( m_panelDspOpt );
	m_notebook->AddPage( m_panelDspOpt, _("Display Options"), true );
	m_panelOpenGL = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerOpenGL;
	bSizerOpenGL = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerOpenGLRenderoptions;
	sbSizerOpenGLRenderoptions = new wxStaticBoxSizer( new wxStaticBox( m_panelOpenGL, wxID_ANY, _("OpenGL Render Options") ), wxVERTICAL );

	m_checkBoxBoundingBoxes = new wxCheckBox( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, _("Show model bounding boxes"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerOpenGLRenderoptions->Add( m_checkBoxBoundingBoxes, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxCuThickness = new wxCheckBox( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, _("Show copper thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerOpenGLRenderoptions->Add( m_checkBoxCuThickness, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerOpenGL->Add( sbSizerOpenGLRenderoptions, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerOtherOptions;
	sbSizerOtherOptions = new wxStaticBoxSizer( new wxStaticBox( m_panelOpenGL, wxID_ANY, _("Other Options") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer7;
	fgSizer7 = new wxFlexGridSizer( 2, 2, 2, 0 );
	fgSizer7->SetFlexibleDirection( wxBOTH );
	fgSizer7->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText221 = new wxStaticText( sbSizerOtherOptions->GetStaticBox(), wxID_ANY, _("Anti-aliasing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText221->Wrap( -1 );
	fgSizer7->Add( m_staticText221, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceAntiAliasingChoices[] = { _("Disabled"), _("2x"), _("4x"), _("8x") };
	int m_choiceAntiAliasingNChoices = sizeof( m_choiceAntiAliasingChoices ) / sizeof( wxString );
	m_choiceAntiAliasing = new wxChoice( sbSizerOtherOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceAntiAliasingNChoices, m_choiceAntiAliasingChoices, 0 );
	m_choiceAntiAliasing->SetSelection( 0 );
	m_choiceAntiAliasing->SetToolTip( _("3D-Viewer must be closed and re-opened to apply this setting") );

	fgSizer7->Add( m_choiceAntiAliasing, 0, wxALL|wxEXPAND, 5 );

	m_staticText231 = new wxStaticText( sbSizerOtherOptions->GetStaticBox(), wxID_ANY, _("Selection color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText231->Wrap( -1 );
	fgSizer7->Add( m_staticText231, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_colourPickerSelection = new wxColourPickerCtrl( sbSizerOtherOptions->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE );
	fgSizer7->Add( m_colourPickerSelection, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	sbSizerOtherOptions->Add( fgSizer7, 1, wxEXPAND|wxRIGHT, 5 );


	bSizerOpenGL->Add( sbSizerOtherOptions, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerWhileMoving;
	sbSizerWhileMoving = new wxStaticBoxSizer( new wxStaticBox( m_panelOpenGL, wxID_ANY, _("While Moving") ), wxVERTICAL );

	m_checkBoxDisableAAMove = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable anti-aliasing"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerWhileMoving->Add( m_checkBoxDisableAAMove, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxDisableMoveThickness = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerWhileMoving->Add( m_checkBoxDisableMoveThickness, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxDisableMoveVias = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable vias"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerWhileMoving->Add( m_checkBoxDisableMoveVias, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxDisableMoveHoles = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable holes"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerWhileMoving->Add( m_checkBoxDisableMoveHoles, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerOpenGL->Add( sbSizerWhileMoving, 0, wxALL|wxEXPAND, 5 );


	m_panelOpenGL->SetSizer( bSizerOpenGL );
	m_panelOpenGL->Layout();
	bSizerOpenGL->Fit( m_panelOpenGL );
	m_notebook->AddPage( m_panelOpenGL, _("OpenGL"), false );
	m_panelRaytracing = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );

	m_notebook2 = new wxNotebook( m_panelRaytracing, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelRaytracingCfg = new wxPanel( m_notebook2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerRaytracing;
	bSizerRaytracing = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerRaytracingRenderOptions;
	sbSizerRaytracingRenderOptions = new wxStaticBoxSizer( new wxStaticBox( m_panelRaytracingCfg, wxID_ANY, _("Raytracing Render Options") ), wxVERTICAL );

	sbSizerRaytracingRenderOptions->SetMinSize( wxSize( -1,32 ) );
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );

	m_checkBoxRaytracing_proceduralTextures = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Procedural textures"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_proceduralTextures->SetValue(true);
	bSizer16->Add( m_checkBoxRaytracing_proceduralTextures, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxRaytracing_addFloor = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Add floor"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_addFloor->SetValue(true);
	bSizer16->Add( m_checkBoxRaytracing_addFloor, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxRaytracing_antiAliasing = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Anti-aliasing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_antiAliasing->SetValue(true);
	bSizer16->Add( m_checkBoxRaytracing_antiAliasing, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxRaytracing_postProcessing = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Post-processing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_postProcessing->SetValue(true);
	bSizer16->Add( m_checkBoxRaytracing_postProcessing, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbSizerRaytracingRenderOptions->Add( bSizer16, 0, wxEXPAND|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizer111;
	fgSizer111 = new wxFlexGridSizer( 0, 4, 6, 8 );
	fgSizer111->SetFlexibleDirection( wxBOTH );
	fgSizer111->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer111->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText19 = new wxStaticText( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Number of Samples"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	fgSizer111->Add( m_staticText19, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText201 = new wxStaticText( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Spread Factor %"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText201->Wrap( -1 );
	fgSizer111->Add( m_staticText201, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText211 = new wxStaticText( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Recursion Level"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText211->Wrap( -1 );
	fgSizer111->Add( m_staticText211, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxRaytracing_renderShadows = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Shadows:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_renderShadows->SetValue(true);
	fgSizer111->Add( m_checkBoxRaytracing_renderShadows, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spinCtrl_NrSamples_Shadows = new wxSpinCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 1, 64, 0 );
	m_spinCtrl_NrSamples_Shadows->SetToolTip( _("Number of rays that will be cast, into light direction, to evaluate a shadow point") );

	fgSizer111->Add( m_spinCtrl_NrSamples_Shadows, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlDouble_SpreadFactor_Shadows = new wxSpinCtrlDouble( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 0.1, 25, 0, 1 );
	m_spinCtrlDouble_SpreadFactor_Shadows->SetDigits( 1 );
	m_spinCtrlDouble_SpreadFactor_Shadows->SetToolTip( _("Random direction factor of the cast rays") );

	fgSizer111->Add( m_spinCtrlDouble_SpreadFactor_Shadows, 0, wxRIGHT|wxLEFT, 5 );


	fgSizer111->Add( 0, 0, 1, wxEXPAND, 5 );

	m_checkBoxRaytracing_showReflections = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Reflections:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_showReflections->SetValue(true);
	fgSizer111->Add( m_checkBoxRaytracing_showReflections, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spinCtrl_NrSamples_Reflections = new wxSpinCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 1, 32, 0 );
	m_spinCtrl_NrSamples_Reflections->SetToolTip( _("Number of rays that will be cast to evaluate a reflection point") );

	fgSizer111->Add( m_spinCtrl_NrSamples_Reflections, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlDouble_SpreadFactor_Reflections = new wxSpinCtrlDouble( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 0.1, 25, 0, 1 );
	m_spinCtrlDouble_SpreadFactor_Reflections->SetDigits( 1 );
	m_spinCtrlDouble_SpreadFactor_Reflections->SetToolTip( _("Random direction factor of the cast rays") );

	fgSizer111->Add( m_spinCtrlDouble_SpreadFactor_Reflections, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlRecursiveLevel_Reflections = new wxSpinCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 1, 5, 0 );
	m_spinCtrlRecursiveLevel_Reflections->SetToolTip( _("Interactions number that a ray can travel through objects. (higher number of levels improve results, specially on very transparent boards)") );

	fgSizer111->Add( m_spinCtrlRecursiveLevel_Reflections, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_checkBoxRaytracing_showRefractions = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Refractions:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_showRefractions->SetValue(true);
	fgSizer111->Add( m_checkBoxRaytracing_showRefractions, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spinCtrl_NrSamples_Refractions = new wxSpinCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 1, 5, 0 );
	m_spinCtrl_NrSamples_Refractions->SetToolTip( _("Number of rays that will be cast to evaluate a refraction point") );

	fgSizer111->Add( m_spinCtrl_NrSamples_Refractions, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlDouble_SpreadFactor_Refractions = new wxSpinCtrlDouble( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 0.1, 25, 0, 1 );
	m_spinCtrlDouble_SpreadFactor_Refractions->SetDigits( 1 );
	m_spinCtrlDouble_SpreadFactor_Refractions->SetToolTip( _("Random direction factor of the cast rays") );

	fgSizer111->Add( m_spinCtrlDouble_SpreadFactor_Refractions, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlRecursiveLevel_Refractions = new wxSpinCtrl( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, 1, 7, 0 );
	m_spinCtrlRecursiveLevel_Refractions->SetToolTip( _("Number of bounces that a ray can hit reflective objects") );

	fgSizer111->Add( m_spinCtrlRecursiveLevel_Refractions, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	sbSizerRaytracingRenderOptions->Add( fgSizer111, 0, wxTOP|wxBOTTOM|wxRIGHT|wxEXPAND, 5 );


	bSizerRaytracing->Add( sbSizerRaytracingRenderOptions, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	m_panelRaytracingCfg->SetSizer( bSizerRaytracing );
	m_panelRaytracingCfg->Layout();
	bSizerRaytracing->Fit( m_panelRaytracingCfg );
	m_notebook2->AddPage( m_panelRaytracingCfg, _("Render Options"), true );
	m_panelLightsConfig = new wxPanel( m_notebook2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerRaytracingLightConfiguration;
	sbSizerRaytracingLightConfiguration = new wxStaticBoxSizer( new wxStaticBox( m_panelLightsConfig, wxID_ANY, _("Lights Configuration") ), wxVERTICAL );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );


	bSizer11->Add( 0, 0, 1, 0, 5 );

	m_staticText17 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Ambient camera light:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	bSizer11->Add( m_staticText17, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_colourPickerCameraLight = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxColour( 51, 51, 51 ), wxDefaultPosition, wxSize( 72,-1 ), wxCLRP_DEFAULT_STYLE );
	bSizer11->Add( m_colourPickerCameraLight, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizer11->Add( 0, 0, 1, 0, 5 );


	sbSizerRaytracingLightConfiguration->Add( bSizer11, 0, wxBOTTOM|wxEXPAND, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );


	bSizer13->Add( 0, 0, 1, 0, 5 );

	m_staticText5 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Top light:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bSizer13->Add( m_staticText5, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_colourPickerTopLight = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxColour( 63, 63, 63 ), wxDefaultPosition, wxSize( 72,-1 ), wxCLRP_DEFAULT_STYLE );
	bSizer13->Add( m_colourPickerTopLight, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizer13->Add( 0, 0, 1, 0, 5 );

	m_staticText6 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Bottom light:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	bSizer13->Add( m_staticText6, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_colourPickerBottomLight = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxColour( 63, 63, 63 ), wxDefaultPosition, wxSize( 72,-1 ), wxCLRP_DEFAULT_STYLE );
	bSizer13->Add( m_colourPickerBottomLight, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizer13->Add( 0, 0, 1, 0, 5 );


	sbSizerRaytracingLightConfiguration->Add( bSizer13, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 0, 9, 0, 0 );
	fgSizer11->AddGrowableCol( 4 );
	fgSizer11->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer11->Add( 0, 0, 0, 0, 5 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );

	m_staticText20 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Elevation (degrees)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText20->Wrap( -1 );
	fgSizer11->Add( m_staticText20, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText18 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Azimuth (degrees)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer11->Add( m_staticText18, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );

	m_staticText27 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Elevation (degrees)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText27->Wrap( -1 );
	fgSizer11->Add( m_staticText27, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );

	m_staticText28 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Azimuth (degrees)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	fgSizer11->Add( m_staticText28, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );

	m_staticText21 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	fgSizer11->Add( m_staticText21, 0, wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL, 5 );

	m_colourPickerLight1 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( 72,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlLightElevation1 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_spinCtrlLightAzimuth1 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );


	fgSizer11->Add( 10, 0, 0, 0, 5 );

	m_staticText22 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("5:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	fgSizer11->Add( m_staticText22, 0, wxALIGN_CENTER|wxLEFT, 5 );

	m_colourPickerLight5 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( 72,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight5, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlLightElevation5 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_spinCtrlLightAzimuth5 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_staticText23 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgSizer11->Add( m_staticText23, 0, wxALIGN_CENTER, 5 );

	m_colourPickerLight2 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( 72,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlLightElevation2 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation2, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_spinCtrlLightAzimuth2 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth2, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );

	m_staticText24 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("6:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	fgSizer11->Add( m_staticText24, 0, wxALIGN_CENTER|wxLEFT, 5 );

	m_colourPickerLight6 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( 72,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight6, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlLightElevation6 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation6, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_spinCtrlLightAzimuth6 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth6, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_staticText25 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("3:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	fgSizer11->Add( m_staticText25, 0, wxALIGN_CENTER, 5 );

	m_colourPickerLight3 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( 72,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight3, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlLightElevation3 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation3, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_spinCtrlLightAzimuth3 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth3, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );

	m_staticText26 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("7:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	fgSizer11->Add( m_staticText26, 0, wxALIGN_CENTER|wxLEFT, 5 );

	m_colourPickerLight7 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( 72,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight7, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlLightElevation7 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation7, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_spinCtrlLightAzimuth7 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth7, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_staticText171 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("4:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText171->Wrap( -1 );
	fgSizer11->Add( m_staticText171, 0, wxALIGN_CENTER, 5 );

	m_colourPickerLight4 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( 72,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight4, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlLightElevation4 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation4, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_spinCtrlLightAzimuth4 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth4, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );


	fgSizer11->Add( 0, 0, 0, 0, 5 );

	m_staticText181 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("8:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText181->Wrap( -1 );
	fgSizer11->Add( m_staticText181, 0, wxALIGN_CENTER|wxLEFT, 5 );

	m_colourPickerLight8 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( 72,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight8, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spinCtrlLightElevation8 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation8, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_spinCtrlLightAzimuth8 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth8, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );


	sbSizerRaytracingLightConfiguration->Add( fgSizer11, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxVERTICAL );


	bSizer19->Add( 0, 0, 0, wxEXPAND, 5 );

	m_buttonLightsResetToDefaults = new wxButton( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Reset to defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer19->Add( m_buttonLightsResetToDefaults, 0, wxALL, 5 );


	sbSizerRaytracingLightConfiguration->Add( bSizer19, 0, wxALL, 5 );


	bSizer17->Add( sbSizerRaytracingLightConfiguration, 0, wxEXPAND|wxALL, 5 );


	m_panelLightsConfig->SetSizer( bSizer17 );
	m_panelLightsConfig->Layout();
	bSizer17->Fit( m_panelLightsConfig );
	m_notebook2->AddPage( m_panelLightsConfig, _("Lights Configuration"), false );

	bSizer14->Add( m_notebook2, 0, wxEXPAND | wxALL, 5 );


	m_panelRaytracing->SetSizer( bSizer14 );
	m_panelRaytracing->Layout();
	bSizer14->Fit( m_panelRaytracing );
	m_notebook->AddPage( m_panelRaytracing, _("Raytracing"), false );

	bSizerMain->Add( m_notebook, 1, wxALL|wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALL, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_checkBoxRealisticMode->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckRealisticMode ), NULL, this );
	m_checkBoxEnableAnimation->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckEnableAnimation ), NULL, this );
	m_buttonLightsResetToDefaults->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnLightsResetToDefaults ), NULL, this );
}

DIALOG_3D_VIEW_OPTIONS_BASE::~DIALOG_3D_VIEW_OPTIONS_BASE()
{
	// Disconnect Events
	m_checkBoxRealisticMode->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckRealisticMode ), NULL, this );
	m_checkBoxEnableAnimation->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckEnableAnimation ), NULL, this );
	m_buttonLightsResetToDefaults->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnLightsResetToDefaults ), NULL, this );

}

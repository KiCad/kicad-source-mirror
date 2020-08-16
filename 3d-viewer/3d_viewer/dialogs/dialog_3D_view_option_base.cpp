///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
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

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_panelDspOpt, wxID_ANY, _("Render Options") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerRenderOptions;
	fgSizerRenderOptions = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerRenderOptions->SetFlexibleDirection( wxBOTH );
	fgSizerRenderOptions->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizerRenderOptions->Add( 0, 0, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_bitmapRealisticMode = new wxStaticBitmap( sbSizer1->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapRealisticMode, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxRealisticMode = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Realistic mode"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxRealisticMode, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRenderOptions->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapBoardBody = new wxStaticBitmap( sbSizer1->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapBoardBody, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxBoardBody = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Show board body"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxBoardBody, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRenderOptions->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapAreas = new wxStaticBitmap( sbSizer1->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapAreas, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxAreas = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Show filled areas in zones"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxAreas, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRenderOptions->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapSubtractMaskFromSilk = new wxStaticBitmap( sbSizer1->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapSubtractMaskFromSilk, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxSubtractMaskFromSilk = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Subtract soldermask from silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxSubtractMaskFromSilk, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRenderOptions->Add( 0, 0, 0, wxALIGN_LEFT|wxALIGN_RIGHT, 10 );

	m_bitmapClipSilkOnViaAnnulus = new wxStaticBitmap( sbSizer1->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapClipSilkOnViaAnnulus, 0, wxALL, 5 );

	m_checkBoxClipSilkOnViaAnnulus = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Clip silkscreen at via annulus"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxClipSilkOnViaAnnulus, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizerRenderOptions->Add( fgSizer3, 1, wxEXPAND, 5 );


	sbSizer1->Add( fgSizerRenderOptions, 0, wxEXPAND|wxBOTTOM, 5 );


	bSizeLeft->Add( sbSizer1, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer3DVis;
	sbSizer3DVis = new wxStaticBoxSizer( new wxStaticBox( m_panelDspOpt, wxID_ANY, _("3D Model Visibility") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer3DVisibility;
	fgSizer3DVisibility = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer3DVisibility->SetFlexibleDirection( wxBOTH );
	fgSizer3DVisibility->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer3DVisibility->Add( 0, 0, 1, wxLEFT|wxRIGHT, 10 );

	m_bitmap3DshapesTH = new wxStaticBitmap( sbSizer3DVis->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_bitmap3DshapesTH, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBox3DshapesTH = new wxCheckBox( sbSizer3DVis->GetStaticBox(), wxID_ANY, _("Show 3D through hole models"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_checkBox3DshapesTH, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer3DVisibility->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmap3DshapesSMD = new wxStaticBitmap( sbSizer3DVis->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_bitmap3DshapesSMD, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBox3DshapesSMD = new wxCheckBox( sbSizer3DVis->GetStaticBox(), wxID_ANY, _("Show 3D SMD models"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_checkBox3DshapesSMD, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer3DVisibility->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmap3DshapesVirtual = new wxStaticBitmap( sbSizer3DVis->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_bitmap3DshapesVirtual, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBox3DshapesVirtual = new wxCheckBox( sbSizer3DVis->GetStaticBox(), wxID_ANY, _("Show 3D virtual models"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_checkBox3DshapesVirtual, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer3DVis->Add( fgSizer3DVisibility, 0, wxEXPAND, 5 );


	bSizeLeft->Add( sbSizer3DVis, 0, wxALL|wxEXPAND, 5 );


	bSizerDisplayOptions->Add( bSizeLeft, 1, wxALL|wxEXPAND, 5 );

	m_staticlineVertical = new wxStaticLine( m_panelDspOpt, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizerDisplayOptions->Add( m_staticlineVertical, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbBoardLayers;
	sbBoardLayers = new wxStaticBoxSizer( new wxStaticBox( m_panelDspOpt, wxID_ANY, _("Board Layers") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerShowBrdLayersOpts;
	fgSizerShowBrdLayersOpts = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerShowBrdLayersOpts->SetFlexibleDirection( wxBOTH );
	fgSizerShowBrdLayersOpts->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapSilkscreen = new wxStaticBitmap( sbBoardLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapSilkscreen, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxSilkscreen = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show silkscreen layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxSilkscreen, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapSolderMask = new wxStaticBitmap( sbBoardLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapSolderMask, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxSolderMask = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show solder mask layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxSolderMask, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapSolderPaste = new wxStaticBitmap( sbBoardLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapSolderPaste, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxSolderpaste = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show solder paste layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxSolderpaste, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapAdhesive = new wxStaticBitmap( sbBoardLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapAdhesive, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxAdhesive = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show adhesive layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxAdhesive, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbBoardLayers->Add( fgSizerShowBrdLayersOpts, 0, wxEXPAND, 5 );


	bSizerRight->Add( sbBoardLayers, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbUserLayers;
	sbUserLayers = new wxStaticBoxSizer( new wxStaticBox( m_panelDspOpt, wxID_ANY, _("User Layers (not shown in realistic mode)") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerShowUserLayersOpts;
	fgSizerShowUserLayersOpts = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerShowUserLayersOpts->SetFlexibleDirection( wxBOTH );
	fgSizerShowUserLayersOpts->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizerShowUserLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapComments = new wxStaticBitmap( sbUserLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_bitmapComments, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxComments = new wxCheckBox( sbUserLayers->GetStaticBox(), wxID_ANY, _("Show comments and drawings layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_checkBoxComments, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerShowUserLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapECO = new wxStaticBitmap( sbUserLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_bitmapECO, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxECO = new wxCheckBox( sbUserLayers->GetStaticBox(), wxID_ANY, _("Show ECO layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_checkBoxECO, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbUserLayers->Add( fgSizerShowUserLayersOpts, 0, wxEXPAND, 5 );


	bSizerRight->Add( sbUserLayers, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbCameraOptions;
	sbCameraOptions = new wxStaticBoxSizer( new wxStaticBox( m_panelDspOpt, wxID_ANY, _("Camera Options") ), wxVERTICAL );

	wxBoxSizer* bSizerRotAngle;
	bSizerRotAngle = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextRotAngle = new wxStaticText( sbCameraOptions->GetStaticBox(), wxID_ANY, _("Rotation Increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotAngle->Wrap( -1 );
	bSizerRotAngle->Add( m_staticTextRotAngle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_spinCtrlRotationAngle = new wxSpinCtrlDouble( sbCameraOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 359, 10, 1 );
	m_spinCtrlRotationAngle->SetDigits( 0 );
	bSizerRotAngle->Add( m_spinCtrlRotationAngle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticTextRotAngleUnits = new wxStaticText( sbCameraOptions->GetStaticBox(), wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotAngleUnits->Wrap( -1 );
	bSizerRotAngle->Add( m_staticTextRotAngleUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbCameraOptions->Add( bSizerRotAngle, 1, wxEXPAND, 5 );

	m_staticline3 = new wxStaticLine( sbCameraOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbCameraOptions->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );

	m_checkBoxEnableAnimation = new wxCheckBox( sbCameraOptions->GetStaticBox(), wxID_ANY, _("Enable animation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxEnableAnimation->SetValue(true);
	sbCameraOptions->Add( m_checkBoxEnableAnimation, 0, wxALL, 5 );

	wxBoxSizer* bSizerSlider;
	bSizerSlider = new wxBoxSizer( wxHORIZONTAL );

	m_staticAnimationSpeed = new wxStaticText( sbCameraOptions->GetStaticBox(), wxID_ANY, _("Animation speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticAnimationSpeed->Wrap( -1 );
	bSizerSlider->Add( m_staticAnimationSpeed, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_sliderAnimationSpeed = new wxSlider( sbCameraOptions->GetStaticBox(), wxID_ANY, 3, 1, 5, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	m_sliderAnimationSpeed->SetMinSize( wxSize( 100,-1 ) );

	bSizerSlider->Add( m_sliderAnimationSpeed, 1, wxALL|wxEXPAND, 5 );


	sbCameraOptions->Add( bSizerSlider, 1, wxEXPAND, 5 );


	bSizerRight->Add( sbCameraOptions, 0, wxALL|wxEXPAND, 5 );


	bSizerDisplayOptions->Add( bSizerRight, 0, wxALL|wxEXPAND, 5 );


	m_panelDspOpt->SetSizer( bSizerDisplayOptions );
	m_panelDspOpt->Layout();
	bSizerDisplayOptions->Fit( m_panelDspOpt );
	m_notebook->AddPage( m_panelDspOpt, _("Display Options"), true );
	m_panelOpenGL = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerOpenGL;
	bSizerOpenGL = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerOpenGLRenderoptions;
	sbSizerOpenGLRenderoptions = new wxStaticBoxSizer( new wxStaticBox( m_panelOpenGL, wxID_ANY, _("OpenGL Render Options") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer( 2, 3, 0, 0 );
	fgSizer6->SetFlexibleDirection( wxBOTH );
	fgSizer6->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer6->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_bitmapBoundingBoxes = new wxStaticBitmap( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_bitmapBoundingBoxes, 0, wxALL, 5 );

	m_checkBoxBoundingBoxes = new wxCheckBox( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, _("Show model bounding boxes"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_checkBoxBoundingBoxes, 0, wxALL, 5 );


	fgSizer6->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_bitmapCuThickness = new wxStaticBitmap( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_bitmapCuThickness, 0, wxALL, 5 );

	m_checkBoxCuThickness = new wxCheckBox( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, _("Show copper thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_checkBoxCuThickness, 0, wxALL, 5 );


	sbSizerOpenGLRenderoptions->Add( fgSizer6, 1, wxALL|wxEXPAND, 5 );


	bSizer7->Add( sbSizerOpenGLRenderoptions, 1, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerAntialiasing;
	sbSizerAntialiasing = new wxStaticBoxSizer( new wxStaticBox( m_panelOpenGL, wxID_ANY, _("Anti-aliasing") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer7;
	fgSizer7 = new wxFlexGridSizer( 1, 2, 0, 0 );
	fgSizer7->SetFlexibleDirection( wxBOTH );
	fgSizer7->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer7->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	wxString m_choiceAntiAliasingChoices[] = { _("Disabled"), _("2x"), _("4x"), _("8x"), wxEmptyString };
	int m_choiceAntiAliasingNChoices = sizeof( m_choiceAntiAliasingChoices ) / sizeof( wxString );
	m_choiceAntiAliasing = new wxChoice( sbSizerAntialiasing->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceAntiAliasingNChoices, m_choiceAntiAliasingChoices, 0 );
	m_choiceAntiAliasing->SetSelection( 0 );
	m_choiceAntiAliasing->SetToolTip( _("3D-Viewer must be closed and re-opened to apply this setting") );

	fgSizer7->Add( m_choiceAntiAliasing, 0, 0, 5 );


	sbSizerAntialiasing->Add( fgSizer7, 1, wxALL|wxEXPAND, 5 );


	bSizer7->Add( sbSizerAntialiasing, 1, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerWhileMoving;
	sbSizerWhileMoving = new wxStaticBoxSizer( new wxStaticBox( m_panelOpenGL, wxID_ANY, _("While Moving") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 2, 4, 0, 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer8->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxDisableAAMove = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable anti-aliasing"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_checkBoxDisableAAMove, 0, wxALL, 5 );


	fgSizer8->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxDisableMoveThickness = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_checkBoxDisableMoveThickness, 0, wxALL, 5 );


	fgSizer8->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxDisableMoveVias = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable vias"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_checkBoxDisableMoveVias, 0, wxALL, 5 );


	fgSizer8->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxDisableMoveHoles = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable holes"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_checkBoxDisableMoveHoles, 0, wxALL, 5 );


	sbSizerWhileMoving->Add( fgSizer8, 1, wxALL|wxEXPAND, 5 );


	bSizer7->Add( sbSizerWhileMoving, 1, wxALL|wxEXPAND, 5 );


	bSizerOpenGL->Add( bSizer7, 1, wxALL|wxEXPAND, 5 );


	m_panelOpenGL->SetSizer( bSizerOpenGL );
	m_panelOpenGL->Layout();
	bSizerOpenGL->Fit( m_panelOpenGL );
	m_notebook->AddPage( m_panelOpenGL, _("OpenGL"), false );
	m_panelRaytracing = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerRaytracing;
	bSizerRaytracing = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerRaytracingRenderOptions;
	sbSizerRaytracingRenderOptions = new wxStaticBoxSizer( new wxStaticBox( m_panelRaytracing, wxID_ANY, _("Raytracing Render Options") ), wxVERTICAL );

	sbSizerRaytracingRenderOptions->SetMinSize( wxSize( -1,32 ) );
	wxFlexGridSizer* fgSizer9;
	fgSizer9 = new wxFlexGridSizer( 4, 4, 0, 0 );
	fgSizer9->SetFlexibleDirection( wxBOTH );
	fgSizer9->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_renderShadows = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Shadows"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_renderShadows->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_renderShadows, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_proceduralTextures = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Procedural textures"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_proceduralTextures->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_proceduralTextures, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_addFloor = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Add floor"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_addFloor->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_addFloor, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_showRefractions = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Refractions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_showRefractions->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_showRefractions, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_showReflections = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Reflections"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_showReflections->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_showReflections, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_antiAliasing = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Anti-aliasing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_antiAliasing->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_antiAliasing, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_postProcessing = new wxCheckBox( sbSizerRaytracingRenderOptions->GetStaticBox(), wxID_ANY, _("Post-processing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_postProcessing->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_postProcessing, 0, wxALL, 5 );


	sbSizerRaytracingRenderOptions->Add( fgSizer9, 1, wxALL, 5 );


	bSizer12->Add( sbSizerRaytracingRenderOptions, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerRaytracingLightConfiguration;
	sbSizerRaytracingLightConfiguration = new wxStaticBoxSizer( new wxStaticBox( m_panelRaytracing, wxID_ANY, _("Lights configuration") ), wxVERTICAL );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );


	bSizer11->Add( 0, 0, 1, 0, 5 );

	m_staticText17 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Ambient Camera Light"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	bSizer11->Add( m_staticText17, 0, wxALL, 5 );

	m_colourPickerCameraLight = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxColour( 51, 51, 51 ), wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE );
	bSizer11->Add( m_colourPickerCameraLight, 0, 0, 5 );


	bSizer11->Add( 0, 0, 1, 0, 5 );


	sbSizerRaytracingLightConfiguration->Add( bSizer11, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );


	bSizer13->Add( 0, 0, 1, 0, 5 );

	m_staticText5 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Top Light"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bSizer13->Add( m_staticText5, 0, wxALL, 5 );

	m_colourPickerTopLight = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxColour( 63, 63, 63 ), wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE );
	bSizer13->Add( m_colourPickerTopLight, 0, 0, 5 );


	bSizer13->Add( 0, 0, 1, 0, 5 );

	m_staticText6 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Bottom Light"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	bSizer13->Add( m_staticText6, 0, wxALL, 5 );

	m_colourPickerBottomLight = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxColour( 63, 63, 63 ), wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE );
	bSizer13->Add( m_colourPickerBottomLight, 0, 0, 5 );


	bSizer13->Add( 0, 0, 1, 0, 5 );


	sbSizerRaytracingLightConfiguration->Add( bSizer13, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 0, 9, 0, 0 );
	fgSizer11->SetFlexibleDirection( wxBOTH );
	fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer11->Add( 0, 0, 1, 0, 5 );


	fgSizer11->Add( 0, 0, 1, 0, 5 );

	m_staticText20 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Elevation (degrees)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText20->Wrap( -1 );
	fgSizer11->Add( m_staticText20, 0, wxALL, 5 );

	m_staticText18 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Azimuth (degrees)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer11->Add( m_staticText18, 0, wxALL, 5 );


	fgSizer11->Add( 0, 0, 1, 0, 5 );


	fgSizer11->Add( 0, 0, 1, 0, 5 );


	fgSizer11->Add( 0, 0, 1, 0, 5 );

	m_staticText27 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Elevation (degrees)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText27->Wrap( -1 );
	fgSizer11->Add( m_staticText27, 0, wxALL, 5 );

	m_staticText28 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("Azimuth (degrees)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	fgSizer11->Add( m_staticText28, 0, wxALL, 5 );

	m_staticText21 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	fgSizer11->Add( m_staticText21, 0, wxALIGN_CENTER|wxALL, 5 );

	m_colourPickerLight1 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( -1,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight1, 0, wxALL, 5 );

	m_spinCtrlLightElevation1 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation1, 0, wxALL, 1 );

	m_spinCtrlLightAzimuth1 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth1, 0, wxALL, 1 );


	fgSizer11->Add( 0, 0, 1, wxALL, 5 );

	m_staticText22 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("5"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	fgSizer11->Add( m_staticText22, 0, wxALIGN_CENTER|wxALL, 5 );

	m_colourPickerLight5 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( -1,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight5, 0, wxALL, 5 );

	m_spinCtrlLightElevation5 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 10, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation5, 0, wxALL, 1 );

	m_spinCtrlLightAzimuth5 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth5, 0, wxALL, 1 );

	m_staticText23 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgSizer11->Add( m_staticText23, 0, wxALIGN_CENTER|wxALL, 5 );

	m_colourPickerLight2 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( -1,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight2, 0, wxALL, 5 );

	m_spinCtrlLightElevation2 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation2, 0, wxALL, 1 );

	m_spinCtrlLightAzimuth2 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth2, 0, wxALL, 1 );


	fgSizer11->Add( 0, 0, 1, wxALL, 5 );

	m_staticText24 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("6"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	fgSizer11->Add( m_staticText24, 0, wxALIGN_CENTER|wxALL, 5 );

	m_colourPickerLight6 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( -1,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight6, 0, wxALL, 5 );

	m_spinCtrlLightElevation6 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 10, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation6, 0, wxALL, 1 );

	m_spinCtrlLightAzimuth6 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth6, 0, wxALL, 1 );

	m_staticText25 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("3"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	fgSizer11->Add( m_staticText25, 0, wxALIGN_CENTER|wxALL, 5 );

	m_colourPickerLight3 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( -1,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight3, 0, wxALL, 5 );

	m_spinCtrlLightElevation3 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation3, 0, wxALL, 1 );

	m_spinCtrlLightAzimuth3 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth3, 0, wxALL, 1 );


	fgSizer11->Add( 0, 0, 1, wxALL, 5 );

	m_staticText26 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("7"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	fgSizer11->Add( m_staticText26, 0, wxALIGN_CENTER|wxALL, 5 );

	m_colourPickerLight7 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( -1,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight7, 0, wxALL, 5 );

	m_spinCtrlLightElevation7 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 10, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation7, 0, wxALL, 1 );

	m_spinCtrlLightAzimuth7 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth7, 0, wxALL, 1 );

	m_staticText171 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("4"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText171->Wrap( -1 );
	fgSizer11->Add( m_staticText171, 0, wxALIGN_CENTER|wxALL, 5 );

	m_colourPickerLight4 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( -1,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight4, 0, wxALL, 5 );

	m_spinCtrlLightElevation4 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 90, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation4, 0, wxALL, 1 );

	m_spinCtrlLightAzimuth4 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth4, 0, wxALL, 1 );


	fgSizer11->Add( 0, 0, 1, wxALL|wxEXPAND, 5 );

	m_staticText181 = new wxStaticText( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, _("8"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText181->Wrap( -1 );
	fgSizer11->Add( m_staticText181, 0, wxALIGN_CENTER|wxALL, 5 );

	m_colourPickerLight8 = new wxColourPickerCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxSize( -1,-1 ), wxCLRP_DEFAULT_STYLE );
	fgSizer11->Add( m_colourPickerLight8, 0, wxALL, 5 );

	m_spinCtrlLightElevation8 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS, -90, 10, 0 );
	fgSizer11->Add( m_spinCtrlLightElevation8, 0, wxALL, 1 );

	m_spinCtrlLightAzimuth8 = new wxSpinCtrl( sbSizerRaytracingLightConfiguration->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 124,-1 ), wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizer11->Add( m_spinCtrlLightAzimuth8, 0, wxALL, 1 );


	sbSizerRaytracingLightConfiguration->Add( fgSizer11, 0, wxALL|wxEXPAND, 5 );


	bSizer12->Add( sbSizerRaytracingLightConfiguration, 1, wxALL|wxEXPAND, 5 );


	bSizerRaytracing->Add( bSizer12, 1, wxALL|wxEXPAND, 5 );


	m_panelRaytracing->SetSizer( bSizerRaytracing );
	m_panelRaytracing->Layout();
	bSizerRaytracing->Fit( m_panelRaytracing );
	m_notebook->AddPage( m_panelRaytracing, _("Raytracing"), false );

	bSizerMain->Add( m_notebook, 1, wxALL|wxEXPAND, 5 );

	m_staticlineH = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticlineH, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

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
}

DIALOG_3D_VIEW_OPTIONS_BASE::~DIALOG_3D_VIEW_OPTIONS_BASE()
{
	// Disconnect Events
	m_checkBoxRealisticMode->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckRealisticMode ), NULL, this );
	m_checkBoxEnableAnimation->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckEnableAnimation ), NULL, this );

}

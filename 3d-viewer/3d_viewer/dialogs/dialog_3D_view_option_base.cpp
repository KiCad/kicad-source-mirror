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
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizeLeft;
	bSizeLeft = new wxBoxSizer( wxVERTICAL );

	m_staticText3DRenderOpts = new wxStaticText( m_panelDspOpt, wxID_ANY, _("Render options:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3DRenderOpts->Wrap( -1 );
	m_staticText3DRenderOpts->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizeLeft->Add( m_staticText3DRenderOpts, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizerRenderOptions;
	fgSizerRenderOptions = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerRenderOptions->SetFlexibleDirection( wxBOTH );
	fgSizerRenderOptions->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizerRenderOptions->Add( 0, 0, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_bitmapRealisticMode = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapRealisticMode, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxRealisticMode = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Realistic mode"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxRealisticMode, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRenderOptions->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapBoardBody = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapBoardBody, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxBoardBody = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Show board body"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxBoardBody, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRenderOptions->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapAreas = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapAreas, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxAreas = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Show filled areas in zones"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxAreas, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRenderOptions->Add( 0, 0, 0, wxALIGN_LEFT|wxALIGN_RIGHT, 10 );

	m_bitmapSubtractMaskFromSilk = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_bitmapSubtractMaskFromSilk, 0, wxALL, 5 );

	m_checkBoxSubtractMaskFromSilk = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Subtract soldermask from silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRenderOptions->Add( m_checkBoxSubtractMaskFromSilk, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizerRenderOptions->Add( fgSizer3, 1, wxEXPAND, 5 );


	bSizeLeft->Add( fgSizerRenderOptions, 0, wxEXPAND|wxBOTTOM, 5 );


	bSizeLeft->Add( 0, 10, 0, 0, 5 );

	m_staticText3DmodelVisibility = new wxStaticText( m_panelDspOpt, wxID_ANY, _("3D model visibility:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3DmodelVisibility->Wrap( -1 );
	m_staticText3DmodelVisibility->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizeLeft->Add( m_staticText3DmodelVisibility, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizer3DVisibility;
	fgSizer3DVisibility = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer3DVisibility->SetFlexibleDirection( wxBOTH );
	fgSizer3DVisibility->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer3DVisibility->Add( 0, 0, 1, wxRIGHT|wxLEFT, 10 );

	m_bitmap3DshapesTH = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_bitmap3DshapesTH, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBox3DshapesTH = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Show 3D through hole models"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_checkBox3DshapesTH, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer3DVisibility->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmap3DshapesSMD = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_bitmap3DshapesSMD, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBox3DshapesSMD = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Show 3D SMD models"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_checkBox3DshapesSMD, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer3DVisibility->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmap3DshapesVirtual = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_bitmap3DshapesVirtual, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBox3DshapesVirtual = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Show 3D virtual models"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3DVisibility->Add( m_checkBox3DshapesVirtual, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizeLeft->Add( fgSizer3DVisibility, 0, wxEXPAND, 5 );


	bSizerUpper->Add( bSizeLeft, 1, wxEXPAND, 5 );

	m_staticlineVertical = new wxStaticLine( m_panelDspOpt, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizerUpper->Add( m_staticlineVertical, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizeLayer;
	bSizeLayer = new wxBoxSizer( wxVERTICAL );

	m_staticTextBoardLayers = new wxStaticText( m_panelDspOpt, wxID_ANY, _("Board layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBoardLayers->Wrap( -1 );
	m_staticTextBoardLayers->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizeLayer->Add( m_staticTextBoardLayers, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizerShowBrdLayersOpts;
	fgSizerShowBrdLayersOpts = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerShowBrdLayersOpts->SetFlexibleDirection( wxBOTH );
	fgSizerShowBrdLayersOpts->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapSilkscreen = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapSilkscreen, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxSilkscreen = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Show silkscreen layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxSilkscreen, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapSolderMask = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapSolderMask, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxSolderMask = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Show solder mask layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxSolderMask, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapSolderPaste = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapSolderPaste, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxSolderpaste = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Show solder paste layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxSolderpaste, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerShowBrdLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapAdhesive = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_bitmapAdhesive, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxAdhesive = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Show adhesive layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowBrdLayersOpts->Add( m_checkBoxAdhesive, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizeLayer->Add( fgSizerShowBrdLayersOpts, 0, wxEXPAND, 5 );


	bSizeLayer->Add( 0, 10, 0, 0, 5 );

	m_staticTextUserLayers = new wxStaticText( m_panelDspOpt, wxID_ANY, _("User layers (not shown in realistic mode):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUserLayers->Wrap( -1 );
	m_staticTextUserLayers->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizeLayer->Add( m_staticTextUserLayers, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizerShowUserLayersOpts;
	fgSizerShowUserLayersOpts = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerShowUserLayersOpts->SetFlexibleDirection( wxBOTH );
	fgSizerShowUserLayersOpts->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizerShowUserLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapComments = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_bitmapComments, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxComments = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Show comments and drawings layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_checkBoxComments, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerShowUserLayersOpts->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_bitmapECO = new wxStaticBitmap( m_panelDspOpt, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_bitmapECO, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkBoxECO = new wxCheckBox( m_panelDspOpt, wxID_ANY, _("Show ECO layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShowUserLayersOpts->Add( m_checkBoxECO, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizeLayer->Add( fgSizerShowUserLayersOpts, 0, wxEXPAND, 5 );


	bSizerRight->Add( bSizeLayer, 1, wxEXPAND, 5 );


	bSizerUpper->Add( bSizerRight, 1, wxEXPAND, 5 );


	m_panelDspOpt->SetSizer( bSizerUpper );
	m_panelDspOpt->Layout();
	bSizerUpper->Fit( m_panelDspOpt );
	m_notebook->AddPage( m_panelDspOpt, _("Display Options"), true );
	m_panelOpenGL = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerOpenGL;
	bSizerOpenGL = new wxBoxSizer( wxVERTICAL );

	m_staticTextOpenGLRenderOpts = new wxStaticText( m_panelOpenGL, wxID_ANY, _("OpenGL Render options:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOpenGLRenderOpts->Wrap( -1 );
	m_staticTextOpenGLRenderOpts->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerOpenGL->Add( m_staticTextOpenGLRenderOpts, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer6->SetFlexibleDirection( wxBOTH );
	fgSizer6->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer6->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_bitmapBoundingBoxes = new wxStaticBitmap( m_panelOpenGL, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_bitmapBoundingBoxes, 0, wxALL, 5 );

	m_checkBoxBoundingBoxes = new wxCheckBox( m_panelOpenGL, wxID_ANY, _("Show model bounding boxes"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_checkBoxBoundingBoxes, 0, wxALL, 5 );


	fgSizer6->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_bitmapCuThickness = new wxStaticBitmap( m_panelOpenGL, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_bitmapCuThickness, 0, wxALL, 5 );

	m_checkBoxCuThickness = new wxCheckBox( m_panelOpenGL, wxID_ANY, _("Show copper thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_checkBoxCuThickness, 0, wxALL, 5 );


	bSizerOpenGL->Add( fgSizer6, 1, wxEXPAND, 5 );

	m_staticTextOpenGLRenderOptsAA = new wxStaticText( m_panelOpenGL, wxID_ANY, _("Anti-aliasing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOpenGLRenderOptsAA->Wrap( -1 );
	m_staticTextOpenGLRenderOptsAA->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerOpenGL->Add( m_staticTextOpenGLRenderOptsAA, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizer7;
	fgSizer7 = new wxFlexGridSizer( 1, 2, 0, 0 );
	fgSizer7->SetFlexibleDirection( wxBOTH );
	fgSizer7->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer7->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	wxString m_choiceAntiAliasingChoices[] = { _("Disabled"), _("2x"), _("4x"), _("8x"), wxEmptyString };
	int m_choiceAntiAliasingNChoices = sizeof( m_choiceAntiAliasingChoices ) / sizeof( wxString );
	m_choiceAntiAliasing = new wxChoice( m_panelOpenGL, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceAntiAliasingNChoices, m_choiceAntiAliasingChoices, 0 );
	m_choiceAntiAliasing->SetSelection( 0 );
	fgSizer7->Add( m_choiceAntiAliasing, 0, 0, 5 );


	fgSizer7->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_staticText14 = new wxStaticText( m_panelOpenGL, wxID_ANY, _("(3D-Viewer must be closed and re-opened to apply this setting)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	fgSizer7->Add( m_staticText14, 0, wxLEFT, 5 );


	bSizerOpenGL->Add( fgSizer7, 1, wxEXPAND, 5 );

	m_staticTextOpenGLWhileMoving = new wxStaticText( m_panelOpenGL, wxID_ANY, _("While Moving"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOpenGLWhileMoving->Wrap( -1 );
	m_staticTextOpenGLWhileMoving->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerOpenGL->Add( m_staticTextOpenGLWhileMoving, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer8->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxDisableAAMove = new wxCheckBox( m_panelOpenGL, wxID_ANY, _("Disable anti-aliasing"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_checkBoxDisableAAMove, 0, wxALL, 5 );


	fgSizer8->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxDisableMoveThickness = new wxCheckBox( m_panelOpenGL, wxID_ANY, _("Disable thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_checkBoxDisableMoveThickness, 0, wxALL, 5 );


	fgSizer8->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxDisableMoveVias = new wxCheckBox( m_panelOpenGL, wxID_ANY, _("Disable vias"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_checkBoxDisableMoveVias, 0, wxALL, 5 );


	fgSizer8->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxDisableMoveHoles = new wxCheckBox( m_panelOpenGL, wxID_ANY, _("Disable holes"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_checkBoxDisableMoveHoles, 0, wxALL, 5 );


	bSizerOpenGL->Add( fgSizer8, 1, wxEXPAND, 5 );


	m_panelOpenGL->SetSizer( bSizerOpenGL );
	m_panelOpenGL->Layout();
	bSizerOpenGL->Fit( m_panelOpenGL );
	m_notebook->AddPage( m_panelOpenGL, _("OpenGL"), false );
	m_panelRaytracing = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );

	m_staticTextRaytracingRenderOpts = new wxStaticText( m_panelRaytracing, wxID_ANY, _("Raytracing Render options:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRaytracingRenderOpts->Wrap( -1 );
	m_staticTextRaytracingRenderOpts->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer14->Add( m_staticTextRaytracingRenderOpts, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizer9;
	fgSizer9 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer9->SetFlexibleDirection( wxBOTH );
	fgSizer9->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_renderShadows = new wxCheckBox( m_panelRaytracing, wxID_ANY, _("Render Shadows"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_renderShadows->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_renderShadows, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_proceduralTextures = new wxCheckBox( m_panelRaytracing, wxID_ANY, _("Procedural Textures"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_proceduralTextures->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_proceduralTextures, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_addFloor = new wxCheckBox( m_panelRaytracing, wxID_ANY, _("Add Floor"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_addFloor->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_addFloor, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_showRefractions = new wxCheckBox( m_panelRaytracing, wxID_ANY, _("Refractions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_showRefractions->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_showRefractions, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_showReflections = new wxCheckBox( m_panelRaytracing, wxID_ANY, _("Reflections"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_showReflections->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_showReflections, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_antiAliasing = new wxCheckBox( m_panelRaytracing, wxID_ANY, _("Anti-aliasing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_antiAliasing->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_antiAliasing, 0, wxALL, 5 );


	fgSizer9->Add( 0, 0, 1, wxLEFT|wxRIGHT, 5 );

	m_checkBoxRaytracing_postProcessing = new wxCheckBox( m_panelRaytracing, wxID_ANY, _("Post-processing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRaytracing_postProcessing->SetValue(true);
	fgSizer9->Add( m_checkBoxRaytracing_postProcessing, 0, wxALL, 5 );


	bSizer14->Add( fgSizer9, 1, wxEXPAND, 5 );


	m_panelRaytracing->SetSizer( bSizer14 );
	m_panelRaytracing->Layout();
	bSizer14->Fit( m_panelRaytracing );
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

	bSizerMain->Add( m_sdbSizer, 0, wxALL|wxALIGN_RIGHT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_checkBoxRealisticMode->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckRealisticMode ), NULL, this );
}

DIALOG_3D_VIEW_OPTIONS_BASE::~DIALOG_3D_VIEW_OPTIONS_BASE()
{
	// Disconnect Events
	m_checkBoxRealisticMode->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_3D_VIEW_OPTIONS_BASE::OnCheckRealisticMode ), NULL, this );

}

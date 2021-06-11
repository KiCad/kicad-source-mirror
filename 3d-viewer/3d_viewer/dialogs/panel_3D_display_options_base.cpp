///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_3D_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_3D_DISPLAY_OPTIONS_BASE::PANEL_3D_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizeLeft;
	bSizeLeft = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbModelVisibility;
	sbModelVisibility = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("3D Model Visibility") ), wxVERTICAL );

	m_checkBox3DshapesTH = new wxCheckBox( sbModelVisibility->GetStaticBox(), wxID_ANY, _("Show 3D through hole models"), wxDefaultPosition, wxDefaultSize, 0 );
	sbModelVisibility->Add( m_checkBox3DshapesTH, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBox3DshapesSMD = new wxCheckBox( sbModelVisibility->GetStaticBox(), wxID_ANY, _("Show 3D SMD models"), wxDefaultPosition, wxDefaultSize, 0 );
	sbModelVisibility->Add( m_checkBox3DshapesSMD, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBox3DshapesVirtual = new wxCheckBox( sbModelVisibility->GetStaticBox(), wxID_ANY, _("Show 3D virtual models"), wxDefaultPosition, wxDefaultSize, 0 );
	sbModelVisibility->Add( m_checkBox3DshapesVirtual, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizeLeft->Add( sbModelVisibility, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbBoardLayers;
	sbBoardLayers = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Board Layers") ), wxVERTICAL );

	m_checkBoxSilkscreen = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show silkscreen layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbBoardLayers->Add( m_checkBoxSilkscreen, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxSolderMask = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show solder mask layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbBoardLayers->Add( m_checkBoxSolderMask, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxSolderpaste = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show solder paste layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbBoardLayers->Add( m_checkBoxSolderpaste, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxAdhesive = new wxCheckBox( sbBoardLayers->GetStaticBox(), wxID_ANY, _("Show adhesive layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbBoardLayers->Add( m_checkBoxAdhesive, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizeLeft->Add( sbBoardLayers, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbUserLayers;
	sbUserLayers = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("User Layers (not shown in realistic mode)") ), wxVERTICAL );

	m_checkBoxComments = new wxCheckBox( sbUserLayers->GetStaticBox(), wxID_ANY, _("Show comments and drawings layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbUserLayers->Add( m_checkBoxComments, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxECO = new wxCheckBox( sbUserLayers->GetStaticBox(), wxID_ANY, _("Show ECO layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbUserLayers->Add( m_checkBoxECO, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizeLeft->Add( sbUserLayers, 0, wxEXPAND|wxALL, 5 );


	bSizerMain->Add( bSizeLeft, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 10 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbRenderOptions;
	sbRenderOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Render Options") ), wxVERTICAL );

	m_checkBoxRealisticMode = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Realistic mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRenderOptions->Add( m_checkBoxRealisticMode, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxBoardBody = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Show board body"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRenderOptions->Add( m_checkBoxBoardBody, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxAreas = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Show filled areas in zones"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRenderOptions->Add( m_checkBoxAreas, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxSubtractMaskFromSilk = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Subtract soldermask from silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRenderOptions->Add( m_checkBoxSubtractMaskFromSilk, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxClipSilkOnViaAnnulus = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Clip silkscreen at via annuli"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRenderOptions->Add( m_checkBoxClipSilkOnViaAnnulus, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_checkBoxRenderPlatedPadsAsPlated = new wxCheckBox( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Use bare copper color for unplated copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRenderPlatedPadsAsPlated->SetToolTip( _("Use different colors for plated and unplated copper. (Slow)") );

	sbRenderOptions->Add( m_checkBoxRenderPlatedPadsAsPlated, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizerMaterials;
	bSizerMaterials = new wxBoxSizer( wxHORIZONTAL );

	m_materialPropertiesLabel = new wxStaticText( sbRenderOptions->GetStaticBox(), wxID_ANY, _("Material properties:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_materialPropertiesLabel->Wrap( -1 );
	bSizerMaterials->Add( m_materialPropertiesLabel, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxString m_materialPropertiesChoices[] = { _("Use all properties"), _("Diffuse properties only"), _("CAD color style") };
	int m_materialPropertiesNChoices = sizeof( m_materialPropertiesChoices ) / sizeof( wxString );
	m_materialProperties = new wxChoice( sbRenderOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_materialPropertiesNChoices, m_materialPropertiesChoices, 0 );
	m_materialProperties->SetSelection( 0 );
	bSizerMaterials->Add( m_materialProperties, 0, wxALL, 5 );


	sbRenderOptions->Add( bSizerMaterials, 1, wxEXPAND, 5 );


	bSizerRight->Add( sbRenderOptions, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxStaticBoxSizer* sbCameraOptions;
	sbCameraOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Camera Options") ), wxVERTICAL );

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
	bSizerRotAngle->Add( m_staticTextRotAngleUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


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


	bSizerRight->Add( sbCameraOptions, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	bSizerMain->Add( bSizerRight, 1, wxEXPAND|wxTOP|wxBOTTOM, 10 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	m_checkBoxRealisticMode->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_3D_DISPLAY_OPTIONS_BASE::OnCheckRealisticMode ), NULL, this );
	m_checkBoxEnableAnimation->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_3D_DISPLAY_OPTIONS_BASE::OnCheckEnableAnimation ), NULL, this );
}

PANEL_3D_DISPLAY_OPTIONS_BASE::~PANEL_3D_DISPLAY_OPTIONS_BASE()
{
	// Disconnect Events
	m_checkBoxRealisticMode->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_3D_DISPLAY_OPTIONS_BASE::OnCheckRealisticMode ), NULL, this );
	m_checkBoxEnableAnimation->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_3D_DISPLAY_OPTIONS_BASE::OnCheckEnableAnimation ), NULL, this );

}

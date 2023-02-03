///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_3D_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_3D_DISPLAY_OPTIONS_BASE::PANEL_3D_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizeLeft;
	bSizeLeft = new wxBoxSizer( wxVERTICAL );

	m_boardLayersLabel = new wxStaticText( this, wxID_ANY, _("Board Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_boardLayersLabel->Wrap( -1 );
	bSizeLeft->Add( m_boardLayersLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizeLeft->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 1, 4, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_checkBoxSilkscreen = new wxCheckBox( this, wxID_ANY, _("Show silkscreen layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_checkBoxSilkscreen, 0, wxLEFT, 5 );

	m_checkBoxSubtractMaskFromSilk = new wxCheckBox( this, wxID_ANY, _("Clip silkscreen at solder mask edges"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_checkBoxSubtractMaskFromSilk, 0, wxLEFT, 25 );

	m_checkBoxClipSilkOnViaAnnulus = new wxCheckBox( this, wxID_ANY, _("Clip silkscreen at via annuli"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_checkBoxClipSilkOnViaAnnulus, 0, wxLEFT, 25 );

	m_checkBoxSolderMask = new wxCheckBox( this, wxID_ANY, _("Show solder mask layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_checkBoxSolderMask, 0, wxTOP|wxLEFT, 5 );

	m_checkBoxSolderpaste = new wxCheckBox( this, wxID_ANY, _("Show solder paste layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_checkBoxSolderpaste, 0, wxLEFT, 5 );

	m_checkBoxAdhesive = new wxCheckBox( this, wxID_ANY, _("Show adhesive layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_checkBoxAdhesive, 0, wxLEFT, 5 );


	bSizeLeft->Add( fgSizer1, 0, wxEXPAND|wxALL, 5 );


	bSizeLeft->Add( 0, 15, 0, wxEXPAND, 5 );

	m_userLayersLabel = new wxStaticText( this, wxID_ANY, _("User Layers (not shown in realistic mode)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_userLayersLabel->Wrap( -1 );
	bSizeLeft->Add( m_userLayersLabel, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline31 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizeLeft->Add( m_staticline31, 0, wxEXPAND|wxBOTTOM, 5 );


	bSizeLeft->Add( 0, 5, 0, wxEXPAND, 5 );

	m_checkBoxComments = new wxCheckBox( this, wxID_ANY, _("Show comment and drawing layers"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizeLeft->Add( m_checkBoxComments, 0, wxRIGHT|wxLEFT, 10 );


	bSizeLeft->Add( 0, 4, 0, wxEXPAND, 5 );

	m_checkBoxECO = new wxCheckBox( this, wxID_ANY, _("Show ECO layers"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizeLeft->Add( m_checkBoxECO, 0, wxRIGHT|wxLEFT, 10 );


	bSizeLeft->Add( 0, 15, 0, wxEXPAND, 5 );

	m_renderOptionsLabel = new wxStaticText( this, wxID_ANY, _("Render Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_renderOptionsLabel->Wrap( -1 );
	bSizeLeft->Add( m_renderOptionsLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline4 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizeLeft->Add( m_staticline4, 0, wxEXPAND|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 1, 4, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_checkBoxBoardBody = new wxCheckBox( this, wxID_ANY, _("Show board body"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_checkBoxBoardBody, 0, wxRIGHT|wxLEFT, 5 );

	m_checkBoxRealisticMode = new wxCheckBox( this, wxID_ANY, _("Realistic mode"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_checkBoxRealisticMode, 0, wxRIGHT|wxLEFT, 5 );

	m_checkBoxAreas = new wxCheckBox( this, wxID_ANY, _("Show filled areas in zones"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_checkBoxAreas, 0, wxRIGHT|wxLEFT, 5 );

	m_checkBoxRenderPlatedPadsAsPlated = new wxCheckBox( this, wxID_ANY, _("Use bare copper color for unplated copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxRenderPlatedPadsAsPlated->SetToolTip( _("Use different colors for plated and unplated copper. (Slow)") );

	fgSizer2->Add( m_checkBoxRenderPlatedPadsAsPlated, 0, wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerMaterials;
	bSizerMaterials = new wxBoxSizer( wxHORIZONTAL );

	m_materialPropertiesLabel = new wxStaticText( this, wxID_ANY, _("Material properties:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_materialPropertiesLabel->Wrap( -1 );
	bSizerMaterials->Add( m_materialPropertiesLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxString m_materialPropertiesChoices[] = { _("Realistic"), _("Solid colors"), _("CAD colors") };
	int m_materialPropertiesNChoices = sizeof( m_materialPropertiesChoices ) / sizeof( wxString );
	m_materialProperties = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_materialPropertiesNChoices, m_materialPropertiesChoices, 0 );
	m_materialProperties->SetSelection( 0 );
	bSizerMaterials->Add( m_materialProperties, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer2->Add( bSizerMaterials, 1, wxEXPAND, 5 );


	bSizeLeft->Add( fgSizer2, 1, wxEXPAND|wxTOP|wxLEFT, 5 );


	bSizer7->Add( bSizeLeft, 0, wxEXPAND|wxRIGHT, 20 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	m_cameraOptionsLabel = new wxStaticText( this, wxID_ANY, _("Camera Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cameraOptionsLabel->Wrap( -1 );
	bSizerRight->Add( m_cameraOptionsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline5 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerRight->Add( m_staticline5, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizerRotAngle;
	bSizerRotAngle = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextRotAngle = new wxStaticText( this, wxID_ANY, _("Rotation increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotAngle->Wrap( -1 );
	bSizerRotAngle->Add( m_staticTextRotAngle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlRotationAngle = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 359, 10, 1 );
	m_spinCtrlRotationAngle->SetDigits( 0 );
	bSizerRotAngle->Add( m_spinCtrlRotationAngle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticTextRotAngleUnits = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotAngleUnits->Wrap( -1 );
	bSizerRotAngle->Add( m_staticTextRotAngleUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerRight->Add( bSizerRotAngle, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	m_checkBoxEnableAnimation = new wxCheckBox( this, wxID_ANY, _("Enable animation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxEnableAnimation->SetValue(true);
	bSizer9->Add( m_checkBoxEnableAnimation, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerRight->Add( bSizer9, 0, wxEXPAND|wxLEFT, 5 );

	wxBoxSizer* bSizerSlider;
	bSizerSlider = new wxBoxSizer( wxHORIZONTAL );

	m_staticAnimationSpeed = new wxStaticText( this, wxID_ANY, _("Animation speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticAnimationSpeed->Wrap( -1 );
	bSizerSlider->Add( m_staticAnimationSpeed, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_sliderAnimationSpeed = new wxSlider( this, wxID_ANY, 3, 1, 5, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	m_sliderAnimationSpeed->SetMinSize( wxSize( 100,-1 ) );

	bSizerSlider->Add( m_sliderAnimationSpeed, 1, wxEXPAND|wxLEFT|wxRIGHT, 15 );


	bSizerRight->Add( bSizerSlider, 0, wxEXPAND|wxLEFT, 5 );


	bSizer7->Add( bSizerRight, 0, wxEXPAND, 15 );


	bSizerMain->Add( bSizer7, 1, 0, 5 );


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

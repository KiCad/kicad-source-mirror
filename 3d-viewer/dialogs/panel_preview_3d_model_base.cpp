///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/text_ctrl_eval.h"

#include "panel_preview_3d_model_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PREVIEW_3D_MODEL_BASE::PANEL_PREVIEW_3D_MODEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizermain;
	bSizermain = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerScale;
	sbSizerScale = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scale") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerScale;
	fgSizerScale = new wxFlexGridSizer( 0, 3, 3, 3 );
	fgSizerScale->AddGrowableCol( 1 );
	fgSizerScale->SetFlexibleDirection( wxBOTH );
	fgSizerScale->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText1 = new wxStaticText( sbSizerScale->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizerScale->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	xscale = new TEXT_CTRL_EVAL( sbSizerScale->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerScale->Add( xscale, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

	m_spinXscale = new wxSpinButton( sbSizerScale->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_VERTICAL );
	fgSizerScale->Add( m_spinXscale, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticText2 = new wxStaticText( sbSizerScale->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizerScale->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	yscale = new TEXT_CTRL_EVAL( sbSizerScale->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerScale->Add( yscale, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

	m_spinYscale = new wxSpinButton( sbSizerScale->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_VERTICAL );
	fgSizerScale->Add( m_spinYscale, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText3 = new wxStaticText( sbSizerScale->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizerScale->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	zscale = new TEXT_CTRL_EVAL( sbSizerScale->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerScale->Add( zscale, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

	m_spinZscale = new wxSpinButton( sbSizerScale->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_VERTICAL );
	fgSizerScale->Add( m_spinZscale, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


	sbSizerScale->Add( fgSizerScale, 0, wxBOTTOM|wxEXPAND|wxRIGHT, 2 );


	bSizerLeft->Add( sbSizerScale, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbSizerRotation;
	sbSizerRotation = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Rotation") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerRotate;
	fgSizerRotate = new wxFlexGridSizer( 0, 3, 1, 3 );
	fgSizerRotate->AddGrowableCol( 1 );
	fgSizerRotate->SetFlexibleDirection( wxBOTH );
	fgSizerRotate->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText11 = new wxStaticText( sbSizerRotation->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizerRotate->Add( m_staticText11, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	xrot = new TEXT_CTRL_EVAL( sbSizerRotation->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRotate->Add( xrot, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

	m_spinXrot = new wxSpinButton( sbSizerRotation->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_VERTICAL );
	fgSizerRotate->Add( m_spinXrot, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticText21 = new wxStaticText( sbSizerRotation->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	fgSizerRotate->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	yrot = new TEXT_CTRL_EVAL( sbSizerRotation->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRotate->Add( yrot, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

	m_spinYrot = new wxSpinButton( sbSizerRotation->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_VERTICAL );
	fgSizerRotate->Add( m_spinYrot, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText31 = new wxStaticText( sbSizerRotation->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	fgSizerRotate->Add( m_staticText31, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	zrot = new TEXT_CTRL_EVAL( sbSizerRotation->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRotate->Add( zrot, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

	m_spinZrot = new wxSpinButton( sbSizerRotation->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_VERTICAL );
	fgSizerRotate->Add( m_spinZrot, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


	sbSizerRotation->Add( fgSizerRotate, 0, wxBOTTOM|wxRIGHT|wxEXPAND, 2 );


	bSizerLeft->Add( sbSizerRotation, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxStaticBoxSizer* sbSizerOffset;
	sbSizerOffset = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Offset") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerOffset;
	fgSizerOffset = new wxFlexGridSizer( 0, 3, 1, 3 );
	fgSizerOffset->AddGrowableCol( 1 );
	fgSizerOffset->SetFlexibleDirection( wxBOTH );
	fgSizerOffset->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText12 = new wxStaticText( sbSizerOffset->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizerOffset->Add( m_staticText12, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	xoff = new TEXT_CTRL_EVAL( sbSizerOffset->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerOffset->Add( xoff, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

	m_spinXoffset = new wxSpinButton( sbSizerOffset->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_VERTICAL );
	fgSizerOffset->Add( m_spinXoffset, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticText22 = new wxStaticText( sbSizerOffset->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	fgSizerOffset->Add( m_staticText22, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	yoff = new TEXT_CTRL_EVAL( sbSizerOffset->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerOffset->Add( yoff, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

	m_spinYoffset = new wxSpinButton( sbSizerOffset->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_VERTICAL );
	fgSizerOffset->Add( m_spinYoffset, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText32 = new wxStaticText( sbSizerOffset->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText32->Wrap( -1 );
	fgSizerOffset->Add( m_staticText32, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	zoff = new TEXT_CTRL_EVAL( sbSizerOffset->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerOffset->Add( zoff, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

	m_spinZoffset = new wxSpinButton( sbSizerOffset->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_VERTICAL );
	fgSizerOffset->Add( m_spinZoffset, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


	sbSizerOffset->Add( fgSizerOffset, 0, wxBOTTOM|wxRIGHT|wxEXPAND, 2 );


	bSizerLeft->Add( sbSizerOffset, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Opacity") ), wxVERTICAL );

	m_opacity = new wxSlider( sbSizer4->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	sbSizer4->Add( m_opacity, 0, wxRIGHT|wxLEFT|wxEXPAND, 2 );


	bSizerLeft->Add( sbSizer4, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	bSizerLeft->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizermain->Add( bSizerLeft, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_previewLabel = new wxStaticText( this, wxID_ANY, _("Preview"), wxDefaultPosition, wxDefaultSize, 0 );
	m_previewLabel->Wrap( -1 );
	m_previewLabel->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer6->Add( m_previewLabel, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerRight->Add( bSizer6, 0, wxEXPAND, 5 );

	m_SizerPanelView = new wxBoxSizer( wxVERTICAL );


	bSizerRight->Add( m_SizerPanelView, 1, wxBOTTOM|wxEXPAND, 5 );


	bSizermain->Add( bSizerRight, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizer3DButtons;
	bSizer3DButtons = new wxBoxSizer( wxVERTICAL );


	bSizer3DButtons->Add( 0, 14, 0, wxEXPAND, 5 );

	m_bpvISO = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_bpvISO->SetToolTip( _("Enable/disable orthographic projection") );

	bSizer3DButtons->Add( m_bpvISO, 0, wxTOP, 5 );

	m_bpvBodyStyle = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_bpvBodyStyle->SetToolTip( _("Show or hide the board body\nIf hidden, show only copper and silkscreen layers.") );

	bSizer3DButtons->Add( m_bpvBodyStyle, 0, wxTOP, 5 );


	bSizer3DButtons->Add( 0, 20, 0, wxEXPAND, 5 );

	m_bpvLeft = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_bpvLeft->SetToolTip( _("View Left") );

	bSizer3DButtons->Add( m_bpvLeft, 0, wxBOTTOM, 5 );

	m_bpvRight = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_bpvRight->SetToolTip( _("View Right") );

	bSizer3DButtons->Add( m_bpvRight, 0, wxBOTTOM, 5 );

	m_bpvFront = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_bpvFront->SetToolTip( _("View Front") );

	bSizer3DButtons->Add( m_bpvFront, 0, wxBOTTOM, 5 );

	m_bpvBack = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_bpvBack->SetToolTip( _("View Back") );

	bSizer3DButtons->Add( m_bpvBack, 0, wxBOTTOM, 5 );

	m_bpvTop = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_bpvTop->SetToolTip( _("View Top") );

	bSizer3DButtons->Add( m_bpvTop, 0, wxBOTTOM, 5 );

	m_bpvBottom = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_bpvBottom->SetToolTip( _("View Bottom") );

	bSizer3DButtons->Add( m_bpvBottom, 0, 0, 5 );


	bSizer3DButtons->Add( 0, 20, 0, wxEXPAND, 5 );

	m_bpUpdate = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_bpUpdate->SetToolTip( _("Reload board and 3D models") );

	bSizer3DButtons->Add( m_bpUpdate, 0, wxTOP, 5 );

	m_bpSettings = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer3DButtons->Add( m_bpSettings, 0, wxTOP|wxBOTTOM, 5 );


	bSizermain->Add( bSizer3DButtons, 0, wxEXPAND|wxRIGHT, 5 );


	this->SetSizer( bSizermain );
	this->Layout();
	bSizermain->Fit( this );

	// Connect Events
	xscale->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelScale ), NULL, this );
	xscale->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinXscale->Connect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementScale ), NULL, this );
	m_spinXscale->Connect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementScale ), NULL, this );
	yscale->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelScale ), NULL, this );
	yscale->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinYscale->Connect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementScale ), NULL, this );
	m_spinYscale->Connect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementScale ), NULL, this );
	zscale->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelScale ), NULL, this );
	zscale->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinZscale->Connect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementScale ), NULL, this );
	m_spinZscale->Connect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementScale ), NULL, this );
	xrot->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelRot ), NULL, this );
	xrot->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinXrot->Connect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementRot ), NULL, this );
	m_spinXrot->Connect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementRot ), NULL, this );
	yrot->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelRot ), NULL, this );
	yrot->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinYrot->Connect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementRot ), NULL, this );
	m_spinYrot->Connect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementRot ), NULL, this );
	zrot->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelRot ), NULL, this );
	zrot->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinZrot->Connect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementRot ), NULL, this );
	m_spinZrot->Connect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementRot ), NULL, this );
	xoff->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelOffset ), NULL, this );
	xoff->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinXoffset->Connect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementOffset ), NULL, this );
	m_spinXoffset->Connect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementOffset ), NULL, this );
	yoff->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelOffset ), NULL, this );
	yoff->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinYoffset->Connect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementOffset ), NULL, this );
	m_spinYoffset->Connect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementOffset ), NULL, this );
	zoff->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelOffset ), NULL, this );
	zoff->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinZoffset->Connect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementOffset ), NULL, this );
	m_spinZoffset->Connect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementOffset ), NULL, this );
	m_opacity->Connect( wxEVT_SLIDER, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onOpacitySlider ), NULL, this );
	m_bpvISO->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DISO ), NULL, this );
	m_bpvBodyStyle->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::setBodyStyleView ), NULL, this );
	m_bpvLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DLeft ), NULL, this );
	m_bpvRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DRight ), NULL, this );
	m_bpvFront->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DFront ), NULL, this );
	m_bpvBack->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DBack ), NULL, this );
	m_bpvTop->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DTop ), NULL, this );
	m_bpvBottom->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DBottom ), NULL, this );
	m_bpUpdate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DUpdate ), NULL, this );
	m_bpSettings->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DSettings ), NULL, this );
}

PANEL_PREVIEW_3D_MODEL_BASE::~PANEL_PREVIEW_3D_MODEL_BASE()
{
	// Disconnect Events
	xscale->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelScale ), NULL, this );
	xscale->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinXscale->Disconnect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementScale ), NULL, this );
	m_spinXscale->Disconnect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementScale ), NULL, this );
	yscale->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelScale ), NULL, this );
	yscale->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinYscale->Disconnect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementScale ), NULL, this );
	m_spinYscale->Disconnect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementScale ), NULL, this );
	zscale->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelScale ), NULL, this );
	zscale->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinZscale->Disconnect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementScale ), NULL, this );
	m_spinZscale->Disconnect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementScale ), NULL, this );
	xrot->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelRot ), NULL, this );
	xrot->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinXrot->Disconnect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementRot ), NULL, this );
	m_spinXrot->Disconnect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementRot ), NULL, this );
	yrot->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelRot ), NULL, this );
	yrot->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinYrot->Disconnect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementRot ), NULL, this );
	m_spinYrot->Disconnect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementRot ), NULL, this );
	zrot->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelRot ), NULL, this );
	zrot->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinZrot->Disconnect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementRot ), NULL, this );
	m_spinZrot->Disconnect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementRot ), NULL, this );
	xoff->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelOffset ), NULL, this );
	xoff->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinXoffset->Disconnect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementOffset ), NULL, this );
	m_spinXoffset->Disconnect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementOffset ), NULL, this );
	yoff->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelOffset ), NULL, this );
	yoff->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinYoffset->Disconnect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementOffset ), NULL, this );
	m_spinYoffset->Disconnect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementOffset ), NULL, this );
	zoff->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onMouseWheelOffset ), NULL, this );
	zoff->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::updateOrientation ), NULL, this );
	m_spinZoffset->Disconnect( wxEVT_SCROLL_LINEDOWN, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onDecrementOffset ), NULL, this );
	m_spinZoffset->Disconnect( wxEVT_SCROLL_LINEUP, wxSpinEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onIncrementOffset ), NULL, this );
	m_opacity->Disconnect( wxEVT_SLIDER, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::onOpacitySlider ), NULL, this );
	m_bpvISO->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DISO ), NULL, this );
	m_bpvBodyStyle->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::setBodyStyleView ), NULL, this );
	m_bpvLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DLeft ), NULL, this );
	m_bpvRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DRight ), NULL, this );
	m_bpvFront->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DFront ), NULL, this );
	m_bpvBack->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DBack ), NULL, this );
	m_bpvTop->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DTop ), NULL, this );
	m_bpvBottom->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DBottom ), NULL, this );
	m_bpUpdate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DUpdate ), NULL, this );
	m_bpSettings->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL_BASE::View3DSettings ), NULL, this );

}

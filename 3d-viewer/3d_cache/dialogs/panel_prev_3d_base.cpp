///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec  4 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_prev_3d_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PREV_3D_BASE::PANEL_PREV_3D_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizermain;
	bSizermain = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* vbScale;
	vbScale = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scale") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerScale;
	fgSizerScale = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerScale->SetFlexibleDirection( wxBOTH );
	fgSizerScale->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( vbScale->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizerScale->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	xscale = new wxTextCtrl( vbScale->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerScale->Add( xscale, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText2 = new wxStaticText( vbScale->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizerScale->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	yscale = new wxTextCtrl( vbScale->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerScale->Add( yscale, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText3 = new wxStaticText( vbScale->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizerScale->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	zscale = new wxTextCtrl( vbScale->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerScale->Add( zscale, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	vbScale->Add( fgSizerScale, 1, wxEXPAND, 5 );
	
	
	bSizerLeft->Add( vbScale, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* vbRotate;
	vbRotate = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Rotation (degrees)") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerRotate;
	fgSizerRotate = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerRotate->SetFlexibleDirection( wxBOTH );
	fgSizerRotate->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText11 = new wxStaticText( vbRotate->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizerRotate->Add( m_staticText11, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	xrot = new wxTextCtrl( vbRotate->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !xrot->HasFlag( wxTE_MULTILINE ) )
	{
	xrot->SetMaxLength( 9 );
	}
	#else
	xrot->SetMaxLength( 9 );
	#endif
	fgSizerRotate->Add( xrot, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText21 = new wxStaticText( vbRotate->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	fgSizerRotate->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	yrot = new wxTextCtrl( vbRotate->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !yrot->HasFlag( wxTE_MULTILINE ) )
	{
	yrot->SetMaxLength( 9 );
	}
	#else
	yrot->SetMaxLength( 9 );
	#endif
	fgSizerRotate->Add( yrot, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText31 = new wxStaticText( vbRotate->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	fgSizerRotate->Add( m_staticText31, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	zrot = new wxTextCtrl( vbRotate->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !zrot->HasFlag( wxTE_MULTILINE ) )
	{
	zrot->SetMaxLength( 9 );
	}
	#else
	zrot->SetMaxLength( 9 );
	#endif
	fgSizerRotate->Add( zrot, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	vbRotate->Add( fgSizerRotate, 1, wxEXPAND, 5 );
	
	
	bSizerLeft->Add( vbRotate, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* vbOffset;
	vbOffset = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Offset") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerOffset;
	fgSizerOffset = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerOffset->SetFlexibleDirection( wxBOTH );
	fgSizerOffset->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText12 = new wxStaticText( vbOffset->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizerOffset->Add( m_staticText12, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	xoff = new wxTextCtrl( vbOffset->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerOffset->Add( xoff, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText22 = new wxStaticText( vbOffset->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	fgSizerOffset->Add( m_staticText22, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	yoff = new wxTextCtrl( vbOffset->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerOffset->Add( yoff, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText32 = new wxStaticText( vbOffset->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText32->Wrap( -1 );
	fgSizerOffset->Add( m_staticText32, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	zoff = new wxTextCtrl( vbOffset->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerOffset->Add( zoff, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	vbOffset->Add( fgSizerOffset, 1, wxEXPAND, 5 );
	
	
	bSizerLeft->Add( vbOffset, 0, wxEXPAND, 5 );
	
	
	bSizermain->Add( bSizerLeft, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_SizerPanelView = new wxBoxSizer( wxVERTICAL );
	
	
	bSizerRight->Add( m_SizerPanelView, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3DButtons;
	bSizer3DButtons = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer3DButtons->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_fgSizerButtons = new wxFlexGridSizer( 0, 4, 0, 0 );
	m_fgSizerButtons->AddGrowableCol( 0 );
	m_fgSizerButtons->AddGrowableCol( 1 );
	m_fgSizerButtons->AddGrowableCol( 2 );
	m_fgSizerButtons->AddGrowableCol( 3 );
	m_fgSizerButtons->SetFlexibleDirection( wxBOTH );
	m_fgSizerButtons->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_bpvISO = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_bpvISO->SetToolTip( _("Change to isometric perspective") );
	
	m_fgSizerButtons->Add( m_bpvISO, 0, wxALL|wxEXPAND, 5 );
	
	m_bpvLeft = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_fgSizerButtons->Add( m_bpvLeft, 0, wxALL|wxEXPAND, 5 );
	
	m_bpvFront = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_fgSizerButtons->Add( m_bpvFront, 0, wxALL|wxEXPAND, 5 );
	
	m_bpvTop = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_fgSizerButtons->Add( m_bpvTop, 0, wxALL|wxEXPAND, 5 );
	
	m_bpUpdate = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_bpUpdate->SetToolTip( _("Reload board and 3D models") );
	
	m_fgSizerButtons->Add( m_bpUpdate, 0, wxALL|wxEXPAND, 5 );
	
	m_bpvRight = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_fgSizerButtons->Add( m_bpvRight, 0, wxALL|wxEXPAND, 5 );
	
	m_bpvBack = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_fgSizerButtons->Add( m_bpvBack, 0, wxALL|wxEXPAND, 5 );
	
	m_bpvBottom = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_fgSizerButtons->Add( m_bpvBottom, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer3DButtons->Add( m_fgSizerButtons, 6, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	bSizer3DButtons->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizerRight->Add( bSizer3DButtons, 0, wxEXPAND, 5 );
	
	
	bSizermain->Add( bSizerRight, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizermain );
	this->Layout();
	
	// Connect Events
	xscale->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	yscale->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	zscale->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	xrot->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	yrot->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	zrot->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	xoff->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	yoff->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	zoff->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	m_bpvISO->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DISO ), NULL, this );
	m_bpvLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DLeft ), NULL, this );
	m_bpvFront->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DFront ), NULL, this );
	m_bpvTop->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DTop ), NULL, this );
	m_bpUpdate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DUpdate ), NULL, this );
	m_bpvRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DRight ), NULL, this );
	m_bpvBack->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DBack ), NULL, this );
	m_bpvBottom->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DBottom ), NULL, this );
}

PANEL_PREV_3D_BASE::~PANEL_PREV_3D_BASE()
{
	// Disconnect Events
	xscale->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	yscale->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	zscale->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	xrot->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	yrot->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	zrot->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	xoff->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	yoff->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	zoff->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PREV_3D_BASE::updateOrientation ), NULL, this );
	m_bpvISO->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DISO ), NULL, this );
	m_bpvLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DLeft ), NULL, this );
	m_bpvFront->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DFront ), NULL, this );
	m_bpvTop->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DTop ), NULL, this );
	m_bpUpdate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DUpdate ), NULL, this );
	m_bpvRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DRight ), NULL, this );
	m_bpvBack->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DBack ), NULL, this );
	m_bpvBottom->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PREV_3D_BASE::View3DBottom ), NULL, this );
	
}

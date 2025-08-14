///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_spacemouse_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SPACEMOUSE_BASE::PANEL_SPACEMOUSE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_panRotateLabel = new wxStaticText( this, wxID_ANY, _("Pan and Rotate"), wxDefaultPosition, wxDefaultSize, 0 );
	m_panRotateLabel->Wrap( -1 );
	bSizer1->Add( m_panRotateLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_gbSizer = new wxGridBagSizer( 1, 10 );
	m_gbSizer->SetFlexibleDirection( wxBOTH );
	m_gbSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	m_gbSizer->SetEmptyCellSize( wxSize( -1,16 ) );

	m_rotationSpeedLabel = new wxStaticText( this, wxID_ANY, _("Rotation speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationSpeedLabel->Wrap( -1 );
	m_gbSizer->Add( m_rotationSpeedLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_rotationSpeed = new wxSlider( this, wxID_ANY, 5, 1, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	m_rotationSpeed->SetToolTip( _("How far to zoom in for each rotation of the mouse wheel") );
	m_rotationSpeed->SetMinSize( wxSize( 160,-1 ) );

	m_gbSizer->Add( m_rotationSpeed, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_BOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkEnablePanH = new wxCheckBox( this, wxID_ANY, _("Reverse rotation direction"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkEnablePanH->SetToolTip( _("Swap the direction of rotation") );

	m_gbSizer->Add( m_checkEnablePanH, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_panSpeedLabel = new wxStaticText( this, wxID_ANY, _("Pan speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_panSpeedLabel->Wrap( -1 );
	m_gbSizer->Add( m_panSpeedLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_autoPanSpeed = new wxSlider( this, wxID_ANY, 5, 1, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	m_autoPanSpeed->SetToolTip( _("How fast to pan when moving an object off the edge of the screen") );
	m_autoPanSpeed->SetMinSize( wxSize( 160,-1 ) );

	m_gbSizer->Add( m_autoPanSpeed, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_BOTTOM|wxRIGHT|wxLEFT, 5 );

	m_reverseY = new wxCheckBox( this, wxID_ANY, _("Reverse vertical pan direction"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gbSizer->Add( m_reverseY, wxGBPosition( 4, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_reverseX = new wxCheckBox( this, wxID_ANY, _("Reverse horizontal pan direction"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gbSizer->Add( m_reverseX, wxGBPosition( 5, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxTOP, 3 );

	m_reverseZ = new wxCheckBox( this, wxID_ANY, _("Reverse zoom direction"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gbSizer->Add( m_reverseZ, wxGBPosition( 7, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );


	m_gbSizer->AddGrowableCol( 1 );
	m_gbSizer->AddGrowableRow( 0 );
	m_gbSizer->AddGrowableRow( 3 );

	bSizer8->Add( m_gbSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	bSizer1->Add( bSizer8, 0, wxEXPAND|wxRIGHT, 10 );


	bSizer10->Add( bSizer1, 1, 0, 5 );


	this->SetSizer( bSizer10 );
	this->Layout();
	bSizer10->Fit( this );
}

PANEL_SPACEMOUSE_BASE::~PANEL_SPACEMOUSE_BASE()
{
}

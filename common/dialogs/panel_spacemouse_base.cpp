///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Rotate speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	gbSizer1->Add( m_staticText1, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_zoomSpeed = new wxSlider( this, wxID_ANY, 5, 1, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	m_zoomSpeed->SetToolTip( _("How far to zoom in for each rotation of the mouse wheel") );
	m_zoomSpeed->SetMinSize( wxSize( 120,-1 ) );

	gbSizer1->Add( m_zoomSpeed, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticText22 = new wxStaticText( this, wxID_ANY, _("Pan speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	gbSizer1->Add( m_staticText22, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_autoPanSpeed = new wxSlider( this, wxID_ANY, 5, 1, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	m_autoPanSpeed->SetToolTip( _("How fast to pan when moving an object off the edge of the screen") );
	m_autoPanSpeed->SetMinSize( wxSize( 120,-1 ) );

	gbSizer1->Add( m_autoPanSpeed, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_checkEnablePanH = new wxCheckBox( this, wxID_ANY, _("Reverse rotation direction"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkEnablePanH->SetToolTip( _("Swap the direction of rotation") );

	gbSizer1->Add( m_checkEnablePanH, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_reverseY = new wxCheckBox( this, wxID_ANY, _("Reverse vertical pan direction"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_reverseY, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxLEFT, 5 );

	m_reverseX = new wxCheckBox( this, wxID_ANY, _("Reverse horizontal pan direction"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_reverseX, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxLEFT, 5 );

	m_dummy1 = new wxSlider( this, wxID_ANY, 5, 1, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	m_dummy1->Hide();
	m_dummy1->SetMinSize( wxSize( 120,-1 ) );

	gbSizer1->Add( m_dummy1, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxRIGHT, 5 );

	m_reverseZ = new wxCheckBox( this, wxID_ANY, _("Reverse zoom direction"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_reverseZ, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_dummy2 = new wxSlider( this, wxID_ANY, 5, 1, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	m_dummy2->Hide();
	m_dummy2->SetMinSize( wxSize( 120,-1 ) );

	gbSizer1->Add( m_dummy2, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxRIGHT, 5 );


	gbSizer1->AddGrowableCol( 0 );
	gbSizer1->AddGrowableCol( 1 );
	gbSizer1->AddGrowableCol( 2 );

	bSizer8->Add( gbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizer1->Add( bSizer8, 0, wxEXPAND|wxRIGHT, 10 );


	bSizer10->Add( bSizer1, 1, 0, 5 );


	this->SetSizer( bSizer10 );
	this->Layout();
	bSizer10->Fit( this );
}

PANEL_SPACEMOUSE_BASE::~PANEL_SPACEMOUSE_BASE()
{
}

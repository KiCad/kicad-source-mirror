///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_pcbnew_display_origin_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PCBNEW_DISPLAY_ORIGIN_BASE::PANEL_PCBNEW_DISPLAY_ORIGIN_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	m_displayOrigin = new wxBoxSizer( wxVERTICAL );

	displayOriginLabel = new wxStaticText( this, wxID_ANY, _("Display Origin"), wxDefaultPosition, wxDefaultSize, 0 );
	displayOriginLabel->Wrap( -1 );
	m_displayOrigin->Add( displayOriginLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_displayOrigin->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 1, 4, 0 );

	m_pageOrigin = new wxRadioButton( this, wxID_ANY, _("Page origin"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	gSizer1->Add( m_pageOrigin, 0, wxRIGHT, 5 );

	m_drillPlaceOrigin = new wxRadioButton( this, wxID_ANY, _("Drill/place file origin"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_drillPlaceOrigin, 0, wxRIGHT, 5 );

	m_gridOrigin = new wxRadioButton( this, wxID_ANY, _("Grid origin"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_gridOrigin, 0, wxRIGHT, 5 );


	m_displayOrigin->Add( gSizer1, 0, wxEXPAND|wxALL, 10 );


	bLeftSizer->Add( m_displayOrigin, 0, wxEXPAND, 5 );

	xAxisLabel = new wxStaticText( this, wxID_ANY, _("X Axis"), wxDefaultPosition, wxDefaultSize, 0 );
	xAxisLabel->Wrap( -1 );
	bLeftSizer->Add( xAxisLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftSizer->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxGridSizer* gSizer2;
	gSizer2 = new wxGridSizer( 0, 1, 4, 0 );

	m_xIncreasesRight = new wxRadioButton( this, wxID_ANY, _("Increases right"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	gSizer2->Add( m_xIncreasesRight, 0, wxRIGHT, 5 );

	m_xIncreasesLeft = new wxRadioButton( this, wxID_ANY, _("Increases left"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer2->Add( m_xIncreasesLeft, 0, wxRIGHT, 5 );


	bLeftSizer->Add( gSizer2, 0, wxEXPAND|wxALL, 10 );

	yAxisLabel = new wxStaticText( this, wxID_ANY, _("Y Axis"), wxDefaultPosition, wxDefaultSize, 0 );
	yAxisLabel->Wrap( -1 );
	bLeftSizer->Add( yAxisLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftSizer->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxGridSizer* gSizer4;
	gSizer4 = new wxGridSizer( 0, 1, 4, 0 );

	m_yIncreasesUp = new wxRadioButton( this, wxID_ANY, _("Increases up"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	gSizer4->Add( m_yIncreasesUp, 0, wxRIGHT, 5 );

	m_yIncreasesDown = new wxRadioButton( this, wxID_ANY, _("Increases down"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer4->Add( m_yIncreasesDown, 0, wxRIGHT, 5 );


	bLeftSizer->Add( gSizer4, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	bMargins->Add( bLeftSizer, 1, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );


	bRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	bMargins->Add( bRightSizer, 1, wxEXPAND|wxRIGHT, 5 );


	bPanelSizer->Add( bMargins, 1, wxTOP|wxRIGHT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_PCBNEW_DISPLAY_ORIGIN_BASE::~PANEL_PCBNEW_DISPLAY_ORIGIN_BASE()
{
}

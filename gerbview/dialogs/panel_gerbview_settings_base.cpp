///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_gerbview_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GERBVIEW_SETTINGS_BASE::PANEL_GERBVIEW_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bDialogSizer;
	bDialogSizer = new wxBoxSizer( wxVERTICAL );
	
	m_UpperSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_PolarDisplayChoices[] = { _("Cartesian coordinates"), _("Polar coordinates") };
	int m_PolarDisplayNChoices = sizeof( m_PolarDisplayChoices ) / sizeof( wxString );
	m_PolarDisplay = new wxRadioBox( this, wxID_ANY, _("Coordinates"), wxDefaultPosition, wxDefaultSize, m_PolarDisplayNChoices, m_PolarDisplayChoices, 1, wxRA_SPECIFY_COLS );
	m_PolarDisplay->SetSelection( 0 );
	bLeftSizer->Add( m_PolarDisplay, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	wxString m_BoxUnitsChoices[] = { _("Inches"), _("Millimeters") };
	int m_BoxUnitsNChoices = sizeof( m_BoxUnitsChoices ) / sizeof( wxString );
	m_BoxUnits = new wxRadioBox( this, wxID_ANY, _("Units"), wxDefaultPosition, wxDefaultSize, m_BoxUnitsNChoices, m_BoxUnitsChoices, 1, wxRA_SPECIFY_COLS );
	m_BoxUnits->SetSelection( 0 );
	bLeftSizer->Add( m_BoxUnits, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	wxString m_PageSizeChoices[] = { _("Full size"), _("Size A4"), _("Size A3"), _("Size A2"), _("Size A"), _("Size B"), _("Size C") };
	int m_PageSizeNChoices = sizeof( m_PageSizeChoices ) / sizeof( wxString );
	m_PageSize = new wxRadioBox( this, wxID_ANY, _("Page Size"), wxDefaultPosition, wxDefaultSize, m_PageSizeNChoices, m_PageSizeChoices, 1, wxRA_SPECIFY_COLS );
	m_PageSize->SetSelection( 0 );
	bLeftSizer->Add( m_PageSize, 0, wxEXPAND|wxTOP|wxLEFT, 5 );
	
	m_ShowPageLimitsOpt = new wxCheckBox( this, wxID_ANY, _("Show page limits"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_ShowPageLimitsOpt, 0, wxALL, 10 );
	
	
	m_UpperSizer->Add( bLeftSizer, 0, wxEXPAND|wxRIGHT, 5 );
	
	
	bDialogSizer->Add( m_UpperSizer, 0, wxEXPAND|wxRIGHT, 5 );
	
	
	this->SetSizer( bDialogSizer );
	this->Layout();
	bDialogSizer->Fit( this );
}

PANEL_GERBVIEW_SETTINGS_BASE::~PANEL_GERBVIEW_SETTINGS_BASE()
{
}

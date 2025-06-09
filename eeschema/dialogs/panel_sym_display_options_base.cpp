///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_sym_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SYM_DISPLAY_OPTIONS_BASE::PANEL_SYM_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	m_galOptionsSizer = new wxBoxSizer( wxVERTICAL );


	bPanelSizer->Add( m_galOptionsSizer, 0, wxEXPAND|wxRIGHT, 15 );

	wxBoxSizer* bRightColumn;
	bRightColumn = new wxBoxSizer( wxVERTICAL );

	m_appearanceLabel = new wxStaticText( this, wxID_ANY, _("Appearance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_appearanceLabel->Wrap( -1 );
	bRightColumn->Add( m_appearanceLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightColumn->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bAppearanceSizer;
	bAppearanceSizer = new wxBoxSizer( wxVERTICAL );

	m_checkShowHiddenPins = new wxCheckBox( this, wxID_ANY, _("S&how hidden pins"), wxDefaultPosition, wxDefaultSize, 0 );
	bAppearanceSizer->Add( m_checkShowHiddenPins, 0, wxEXPAND|wxALL, 5 );

	m_checkShowHiddenFields = new wxCheckBox( this, wxID_ANY, _("Show hidden fields"), wxDefaultPosition, wxDefaultSize, 0 );
	bAppearanceSizer->Add( m_checkShowHiddenFields, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_showPinElectricalTypes = new wxCheckBox( this, wxID_ANY, _("Show pin &electrical type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showPinElectricalTypes->SetValue(true);
	bAppearanceSizer->Add( m_showPinElectricalTypes, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkShowPinAltModeIcons = new wxCheckBox( this, wxID_ANY, _("Show pin &alternate mode indicator icons"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowPinAltModeIcons->SetValue(true);
	bAppearanceSizer->Add( m_checkShowPinAltModeIcons, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bRightColumn->Add( bAppearanceSizer, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bPanelSizer->Add( bRightColumn, 0, wxEXPAND|wxBOTTOM|wxLEFT, 10 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_SYM_DISPLAY_OPTIONS_BASE::~PANEL_SYM_DISPLAY_OPTIONS_BASE()
{
}

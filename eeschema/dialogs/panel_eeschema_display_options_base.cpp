///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_eeschema_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE::PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_galOptionsSizer = new wxBoxSizer( wxVERTICAL );
	
	
	bPanelSizer->Add( m_galOptionsSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bRightColumn;
	bRightColumn = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer32;
	fgSizer32 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer32->AddGrowableCol( 1 );
	fgSizer32->SetFlexibleDirection( wxBOTH );
	fgSizer32->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer32->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_busWidthLabel = new wxStaticText( this, wxID_ANY, _("&Bus thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_busWidthLabel->Wrap( -1 );
	fgSizer32->Add( m_busWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_busWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer32->Add( m_busWidthCtrl, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_busWidthUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_busWidthUnits->Wrap( -1 );
	fgSizer32->Add( m_busWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_lineWidthLabel = new wxStaticText( this, wxID_ANY, _("&Line thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	fgSizer32->Add( m_lineWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_lineWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer32->Add( m_lineWidthCtrl, 0, wxEXPAND|wxBOTTOM, 5 );
	
	m_lineWidthUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	fgSizer32->Add( m_lineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	fgSizer32->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 10 );
	
	
	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText26 = new wxStaticText( this, wxID_ANY, _("Symbol unit notation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	fgSizer32->Add( m_staticText26, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_choiceSeparatorRefIdChoices[] = { _("A"), _(".A"), _("-A"), _("_A"), _(".1"), _("-1"), _("_1") };
	int m_choiceSeparatorRefIdNChoices = sizeof( m_choiceSeparatorRefIdChoices ) / sizeof( wxString );
	m_choiceSeparatorRefId = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceSeparatorRefIdNChoices, m_choiceSeparatorRefIdChoices, 0 );
	m_choiceSeparatorRefId->SetSelection( 0 );
	fgSizer32->Add( m_choiceSeparatorRefId, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bRightColumn->Add( fgSizer32, 0, wxBOTTOM|wxRIGHT|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer92;
	bSizer92 = new wxBoxSizer( wxVERTICAL );
	
	m_checkShowHiddenPins = new wxCheckBox( this, wxID_ANY, _("S&how hidden pins"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer92->Add( m_checkShowHiddenPins, 0, wxEXPAND|wxALL, 5 );
	
	m_checkPageLimits = new wxCheckBox( this, wxID_ANY, _("Show page limi&ts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkPageLimits->SetValue(true); 
	bSizer92->Add( m_checkPageLimits, 0, wxEXPAND|wxALL, 5 );
	
	
	bRightColumn->Add( bSizer92, 0, wxTOP|wxRIGHT, 12 );
	
	
	bPanelSizer->Add( bRightColumn, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE::~PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE()
{
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_symbol_parity_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_SYMBOL_PARITY_BASE::PANEL_SETUP_SYMBOL_PARITY_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftColumn;
	bLeftColumn = new wxBoxSizer( wxVERTICAL );

	m_staticText26 = new wxStaticText( this, wxID_ANY, _("Fields"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	bLeftColumn->Add( m_staticText26, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftColumn->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 1, 5, 5 );
	fgSizer2->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

	m_missingFields = new wxCheckBox( this, wxID_ANY, _("Check for missing fields"), wxDefaultPosition, wxDefaultSize, 0 );
	m_missingFields->SetValue(true);
	fgSizer2->Add( m_missingFields, 0, wxRIGHT|wxLEFT, 5 );

	m_extraFields = new wxCheckBox( this, wxID_ANY, _("Check for extra fields"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_extraFields, 0, wxRIGHT|wxLEFT, 5 );

	m_fieldTextOpt = new wxCheckBox( this, wxID_ANY, _("Compare non-reference field text"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_fieldTextOpt, 0, wxRIGHT|wxLEFT, 5 );

	m_fieldVisibilitiesOpt = new wxCheckBox( this, wxID_ANY, _("Compare field visibilities"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_fieldVisibilitiesOpt, 0, wxRIGHT|wxLEFT, 5 );

	m_fieldStyleOpt = new wxCheckBox( this, wxID_ANY, _("Compare field text sizes and styles"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_fieldStyleOpt, 0, wxRIGHT|wxLEFT, 5 );

	m_fieldPositionsOpt = new wxCheckBox( this, wxID_ANY, _("Compare field positions"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_fieldPositionsOpt, 0, wxRIGHT|wxLEFT, 5 );


	bLeftColumn->Add( fgSizer2, 0, wxEXPAND|wxALL, 5 );


	bLeftColumn->Add( 0, 5, 0, wxEXPAND, 5 );

	m_staticText27 = new wxStaticText( this, wxID_ANY, _("Pins"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText27->Wrap( -1 );
	bLeftColumn->Add( m_staticText27, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftColumn->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer321;
	fgSizer321 = new wxFlexGridSizer( 0, 1, 5, 0 );
	fgSizer321->SetFlexibleDirection( wxBOTH );
	fgSizer321->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_pinVisibilitiesOpt = new wxCheckBox( this, wxID_ANY, _("Compare pin name/number visibilities"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer321->Add( m_pinVisibilitiesOpt, 0, wxRIGHT|wxLEFT, 5 );

	m_altPinFunctionsOpt = new wxCheckBox( this, wxID_ANY, _("Compare alternate pin functions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_altPinFunctionsOpt->SetValue(true);
	fgSizer321->Add( m_altPinFunctionsOpt, 0, wxRIGHT|wxLEFT, 5 );


	bLeftColumn->Add( fgSizer321, 0, wxEXPAND|wxALL, 5 );


	bLeftColumn->Add( 0, 5, 0, wxEXPAND, 5 );

	m_staticText28 = new wxStaticText( this, wxID_ANY, _("Attributes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	bLeftColumn->Add( m_staticText28, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftColumn->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer3211;
	fgSizer3211 = new wxFlexGridSizer( 0, 1, 5, 0 );
	fgSizer3211->SetFlexibleDirection( wxBOTH );
	fgSizer3211->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_excludeFromBoardOpt = new wxCheckBox( this, wxID_ANY, _("Compare exclude from board flags"), wxDefaultPosition, wxDefaultSize, 0 );
	m_excludeFromBoardOpt->SetValue(true);
	fgSizer3211->Add( m_excludeFromBoardOpt, 0, wxRIGHT|wxLEFT, 5 );

	m_DNPOpt = new wxCheckBox( this, wxID_ANY, _("Compare do not populate flags"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3211->Add( m_DNPOpt, 0, wxRIGHT|wxLEFT, 5 );

	m_excludeFromBOMOpt = new wxCheckBox( this, wxID_ANY, _("Compare exlucde from bill of materials flags"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3211->Add( m_excludeFromBOMOpt, 0, wxRIGHT|wxLEFT, 5 );

	m_excludeFromPosFilesOpt = new wxCheckBox( this, wxID_ANY, _("Compare exclude from position file flags"), wxDefaultPosition, wxDefaultSize, 0 );
	m_excludeFromPosFilesOpt->SetValue(true);
	fgSizer3211->Add( m_excludeFromPosFilesOpt, 0, wxRIGHT|wxLEFT, 5 );


	bLeftColumn->Add( fgSizer3211, 0, wxEXPAND|wxALL, 5 );


	bPanelSizer->Add( bLeftColumn, 1, wxEXPAND|wxRIGHT, 15 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_SETUP_SYMBOL_PARITY_BASE::~PANEL_SETUP_SYMBOL_PARITY_BASE()
{
}

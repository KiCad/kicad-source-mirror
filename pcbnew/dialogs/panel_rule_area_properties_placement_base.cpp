///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_rule_area_properties_placement_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE::PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMarginsSizer;
	bMarginsSizer = new wxBoxSizer( wxVERTICAL );

	m_DisabledRb = new wxRadioButton( this, wxID_ANY, _("No placement"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DisabledRb->SetValue( true );
	bMarginsSizer->Add( m_DisabledRb, 0, wxALL, 5 );

	m_SheetRb = new wxRadioButton( this, wxID_ANY, _("Place items from sheet:"), wxDefaultPosition, wxDefaultSize, 0 );
	bMarginsSizer->Add( m_SheetRb, 0, wxALL, 5 );

	m_sheetCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	bMarginsSizer->Add( m_sheetCombo, 0, wxLEFT|wxEXPAND, 25 );


	bMarginsSizer->Add( 0, 15, 0, wxEXPAND, 5 );

	m_ComponentsRb = new wxRadioButton( this, wxID_ANY, _("Place items matching component class:"), wxDefaultPosition, wxDefaultSize, 0 );
	bMarginsSizer->Add( m_ComponentsRb, 0, wxALL, 5 );

	m_componentClassCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	bMarginsSizer->Add( m_componentClassCombo, 0, wxLEFT|wxEXPAND, 25 );


	bMarginsSizer->Add( 0, 15, 0, 0, 5 );

	m_GroupRb = new wxRadioButton( this, wxID_ANY, _("Place items in group:"), wxDefaultPosition, wxDefaultSize, 0 );
	bMarginsSizer->Add( m_GroupRb, 0, wxALL, 5 );

	m_groupCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	bMarginsSizer->Add( m_groupCombo, 0, wxEXPAND|wxLEFT, 25 );


	bMarginsSizer->Add( 0, 5, 1, wxEXPAND, 5 );


	bMainSizer->Add( bMarginsSizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE::~PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE()
{
}

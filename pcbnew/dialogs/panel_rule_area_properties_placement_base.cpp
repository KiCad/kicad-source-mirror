///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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

	m_SheetCb = new wxCheckBox( this, wxID_ANY, _("Place items from sheet:"), wxDefaultPosition, wxDefaultSize, 0 );
	bMarginsSizer->Add( m_SheetCb, 0, wxALL, 5 );

	m_sheetCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	bMarginsSizer->Add( m_sheetCombo, 0, wxLEFT|wxEXPAND, 25 );


	bMarginsSizer->Add( 0, 15, 0, wxEXPAND, 5 );

	m_ComponentsCb = new wxCheckBox( this, wxID_ANY, _("Place items matching component class:"), wxDefaultPosition, wxDefaultSize, 0 );
	bMarginsSizer->Add( m_ComponentsCb, 0, wxALL, 5 );

	m_componentClassCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	bMarginsSizer->Add( m_componentClassCombo, 0, wxLEFT|wxEXPAND, 25 );


	bMarginsSizer->Add( 0, 5, 0, 0, 5 );


	bMainSizer->Add( bMarginsSizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE::~PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE()
{
}

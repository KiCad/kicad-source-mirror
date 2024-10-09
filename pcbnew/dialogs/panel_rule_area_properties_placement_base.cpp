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
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_enabled = new wxCheckBox( this, wxID_ANY, _("Enabled"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_enabled, 0, wxALL, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Placement Source") ), wxVERTICAL );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 0, 0 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_sourceSheetnameRb = new wxRadioButton( sbSizer2->GetStaticBox(), wxID_ANY, _("Sheet"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_sourceSheetnameRb, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_sourceComponentClassRb = new wxRadioButton( sbSizer2->GetStaticBox(), wxID_ANY, _("Component Class"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_sourceComponentClassRb, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_sourceObjectNameCb = new wxComboBox( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	gbSizer2->Add( m_sourceObjectNameCb, wxGBPosition( 1, 0 ), wxGBSpan( 1, 3 ), wxALL|wxEXPAND, 5 );


	gbSizer2->AddGrowableCol( 2 );

	sbSizer2->Add( gbSizer2, 1, wxEXPAND, 5 );


	bSizer2->Add( sbSizer2, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer2 );
	this->Layout();
	bSizer2->Fit( this );
}

PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE::~PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE()
{
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_rule_area_properties_keepout_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_RULE_AREA_PROPERTIES_KEEPOUT_BASE::PANEL_RULE_AREA_PROPERTIES_KEEPOUT_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	m_keepoutRuleSizer = new wxFlexGridSizer( 0, 1, 3, 0 );
	m_keepoutRuleSizer->SetFlexibleDirection( wxBOTH );
	m_keepoutRuleSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbTracksCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTracksCtrl->SetToolTip( _("Prevent tracks from routing into this area") );

	m_keepoutRuleSizer->Add( m_cbTracksCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_cbViasCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbViasCtrl->SetToolTip( _("Prevent vias from being placed in this area") );

	m_keepoutRuleSizer->Add( m_cbViasCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_cbPadsCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPadsCtrl->SetToolTip( _("Raise a DRC error if a pad overlaps this area") );

	m_keepoutRuleSizer->Add( m_cbPadsCtrl, 0, wxRIGHT|wxLEFT, 5 );

	m_cbCopperPourCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out zone fills"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbCopperPourCtrl->SetToolTip( _("Zones will not fill copper into this area") );

	m_keepoutRuleSizer->Add( m_cbCopperPourCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_cbFootprintsCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbFootprintsCtrl->SetToolTip( _("Raise a DRC error if a footprint courtyard overlaps this area") );

	m_keepoutRuleSizer->Add( m_cbFootprintsCtrl, 0, wxRIGHT|wxLEFT, 5 );


	this->SetSizer( m_keepoutRuleSizer );
	this->Layout();
	m_keepoutRuleSizer->Fit( this );
}

PANEL_RULE_AREA_PROPERTIES_KEEPOUT_BASE::~PANEL_RULE_AREA_PROPERTIES_KEEPOUT_BASE()
{
}

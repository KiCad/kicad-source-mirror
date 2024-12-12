///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "drc_re_routing_width_panel_base.h"

///////////////////////////////////////////////////////////////////////////

DRC_RE_ROUTING_WIDTH_PANEL_BASE::DRC_RE_ROUTING_WIDTH_PANEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bConstraintImageAndValueSizer;
	bConstraintImageAndValueSizer = new wxBoxSizer( wxHORIZONTAL );

	bConstraintImageSizer = new wxBoxSizer( wxVERTICAL );


	bConstraintImageAndValueSizer->Add( bConstraintImageSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT, 5 );

	wxBoxSizer* bConstraintContentSizer;
	bConstraintContentSizer = new wxBoxSizer( wxVERTICAL );


	bConstraintContentSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer31;
	fgSizer31 = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizer31->SetFlexibleDirection( wxBOTH );
	fgSizer31->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText13 = new wxStaticText( this, wxID_ANY, _("Minimum Routing Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	fgSizer31->Add( m_staticText13, 0, wxALL, 5 );

	m_minRoutingWidthTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_minRoutingWidthTextCtrl, 0, wxALL, 5 );

	m_staticText14 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	fgSizer31->Add( m_staticText14, 0, wxALL, 5 );

	m_staticText15 = new wxStaticText( this, wxID_ANY, _("Preferred Routing Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizer31->Add( m_staticText15, 0, wxALL, 5 );

	m_preferredRoutingWidthTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_preferredRoutingWidthTextCtrl, 0, wxALL, 5 );

	m_staticText16 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizer31->Add( m_staticText16, 0, wxALL, 5 );

	m_staticText17 = new wxStaticText( this, wxID_ANY, _("Maximum Routing Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	fgSizer31->Add( m_staticText17, 0, wxALL, 5 );

	m_maxRoutingWidthTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_maxRoutingWidthTextCtrl, 0, wxALL, 5 );

	m_staticText18 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer31->Add( m_staticText18, 0, wxALL, 5 );


	bConstraintContentSizer->Add( fgSizer31, 0, wxEXPAND, 5 );


	bConstraintContentSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	bConstraintImageAndValueSizer->Add( bConstraintContentSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	mainSizer->Add( bConstraintImageAndValueSizer, 0, wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

DRC_RE_ROUTING_WIDTH_PANEL_BASE::~DRC_RE_ROUTING_WIDTH_PANEL_BASE()
{
}

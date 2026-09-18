///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_zone_conversion_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_ZONE_CONVERSION_BASE::PANEL_ZONE_CONVERSION_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* conversionSizer;
	conversionSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Conversion Settings") ), wxVERTICAL );

	m_rbCenterline = new wxRadioButton( conversionSizer->GetStaticBox(), wxID_ANY, _("Use centerlines"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	conversionSizer->Add( m_rbCenterline, 0, wxLEFT|wxRIGHT, 5 );


	conversionSizer->Add( 0, 2, 0, 0, 0 );

	m_rbEnvelope = new wxRadioButton( conversionSizer->GetStaticBox(), wxID_ANY, _("Create bounding hull"), wxDefaultPosition, wxDefaultSize, 0 );
	conversionSizer->Add( m_rbEnvelope, 0, wxLEFT|wxRIGHT, 5 );


	conversionSizer->Add( 0, 2, 0, 0, 0 );

	wxBoxSizer* hullParamsSizer;
	hullParamsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_gapLabel = new wxStaticText( conversionSizer->GetStaticBox(), wxID_ANY, _("Gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gapLabel->Wrap( -1 );
	hullParamsSizer->Add( m_gapLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_gapCtrl = new wxTextCtrl( conversionSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	hullParamsSizer->Add( m_gapCtrl, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_gapUnits = new wxStaticText( conversionSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gapUnits->Wrap( -1 );
	hullParamsSizer->Add( m_gapUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	conversionSizer->Add( hullParamsSizer, 0, wxLEFT, 26 );


	conversionSizer->Add( 0, 6, 0, 0, 0 );

	m_cbDeleteOriginals = new wxCheckBox( conversionSizer->GetStaticBox(), wxID_ANY, _("Delete source objects after conversion"), wxDefaultPosition, wxDefaultSize, 0 );
	conversionSizer->Add( m_cbDeleteOriginals, 0, wxALL, 5 );


	mainSizer->Add( conversionSizer, 1, wxEXPAND, 0 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_ZONE_CONVERSION_BASE::OnUpdateUI ) );
}

PANEL_ZONE_CONVERSION_BASE::~PANEL_ZONE_CONVERSION_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_ZONE_CONVERSION_BASE::OnUpdateUI ) );

}

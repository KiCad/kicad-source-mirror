///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_copper_zones_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_COPPER_ZONE_BASE::DIALOG_COPPER_ZONE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_MainBoxSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );

	m_staticTextLayerSelection = new wxStaticText( this, wxID_ANY, _("Layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLayerSelection->Wrap( -1 );
	bSizerLeft->Add( m_staticTextLayerSelection, 0, wxLEFT|wxRIGHT|wxTOP, 7 );


	bSizerLeft->Add( 0, 2, 0, wxEXPAND, 5 );

	m_layers = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER|wxBORDER_SIMPLE );
	m_layers->SetMinSize( wxSize( 180,-1 ) );

	bSizerLeft->Add( m_layers, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bSizerLeft, 0, wxEXPAND, 5 );


	bMainSizer->Add( 5, 0, 0, wxEXPAND, 5 );

	m_sizerRight = new wxBoxSizer( wxVERTICAL );


	bMainSizer->Add( m_sizerRight, 1, wxEXPAND|wxTOP|wxRIGHT, 5 );


	m_MainBoxSizer->Add( bMainSizer, 1, wxEXPAND|wxLEFT, 5 );

	wxBoxSizer* bSizerbottom;
	bSizerbottom = new wxBoxSizer( wxHORIZONTAL );

	m_openZoneManager = new wxButton( this, wxID_ANY, _("Open Zone Manager..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerbottom->Add( m_openZoneManager, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerbottom->Add( m_sdbSizer, 1, wxALL|wxEXPAND, 5 );


	m_MainBoxSizer->Add( bSizerbottom, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( m_MainBoxSizer );
	this->Layout();
	m_MainBoxSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_COPPER_ZONE_BASE::OnClose ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_COPPER_ZONE_BASE::OnUpdateUI ) );
	m_layers->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_COPPER_ZONE_BASE::OnLayerSelection ), NULL, this );
	m_openZoneManager->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::onZoneManager ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnButtonCancelClick ), NULL, this );
}

DIALOG_COPPER_ZONE_BASE::~DIALOG_COPPER_ZONE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_COPPER_ZONE_BASE::OnClose ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_COPPER_ZONE_BASE::OnUpdateUI ) );
	m_layers->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_COPPER_ZONE_BASE::OnLayerSelection ), NULL, this );
	m_openZoneManager->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::onZoneManager ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_ZONE_BASE::OnButtonCancelClick ), NULL, this );

}

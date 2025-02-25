///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_layers_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_LAYERS_BASE::PANEL_SETUP_LAYERS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerLayerCnt;
	bSizerLayerCnt = new wxBoxSizer( wxHORIZONTAL );


	bSizerLayerCnt->Add( 0, 0, 1, wxEXPAND, 5 );

	m_addUserDefinedLayerButton = new wxButton( this, wxID_ANY, _("Add User Defined Layer..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLayerCnt->Add( m_addUserDefinedLayerButton, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerMargins->Add( bSizerLayerCnt, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMargins->Add( m_staticline2, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_LayersListPanel = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL|wxVSCROLL );
	m_LayersListPanel->SetScrollRate( 0, 5 );
	m_LayersSizer = new wxFlexGridSizer( 0, 3, 2, 8 );
	m_LayersSizer->AddGrowableCol( 1 );
	m_LayersSizer->AddGrowableCol( 2 );
	m_LayersSizer->SetFlexibleDirection( wxHORIZONTAL );
	m_LayersSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );


	m_LayersListPanel->SetSizer( m_LayersSizer );
	m_LayersListPanel->Layout();
	m_LayersSizer->Fit( m_LayersListPanel );
	bSizerMargins->Add( m_LayersListPanel, 1, wxEXPAND|wxTOP, 5 );


	bMainSizer->Add( bSizerMargins, 1, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_addUserDefinedLayerButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::addUserDefinedLayer ), NULL, this );
}

PANEL_SETUP_LAYERS_BASE::~PANEL_SETUP_LAYERS_BASE()
{
	// Disconnect Events
	m_addUserDefinedLayerButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::addUserDefinedLayer ), NULL, this );

}

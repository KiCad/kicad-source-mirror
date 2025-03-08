///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "appearance_controls_3D_base.h"

///////////////////////////////////////////////////////////////////////////

APPEARANCE_CONTROLS_3D_BASE::APPEARANCE_CONTROLS_3D_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : WX_PANEL( parent, id, pos, size, style, name )
{
	this->SetMinSize( wxSize( 210,360 ) );

	m_sizerOuter = new wxBoxSizer( wxVERTICAL );

	m_panelLayers = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelLayersSizer = new wxBoxSizer( wxVERTICAL );

	m_windowLayers = new wxScrolledCanvas( m_panelLayers, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_panelLayersSizer->Add( m_windowLayers, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	m_panelLayers->SetSizer( m_panelLayersSizer );
	m_panelLayers->Layout();
	m_panelLayersSizer->Fit( m_panelLayers );
	m_sizerOuter->Add( m_panelLayers, 1, wxEXPAND, 5 );

	wxBoxSizer* bBottomMargin;
	bBottomMargin = new wxBoxSizer( wxVERTICAL );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bBottomMargin->Add( m_staticline1, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 3 );

	wxBoxSizer* bPresets;
	bPresets = new wxBoxSizer( wxVERTICAL );

	m_presetsLabel = new wxStaticText( this, wxID_ANY, _("Presets (Ctrl+Tab):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_presetsLabel->Wrap( -1 );
	bPresets->Add( m_presetsLabel, 1, 0, 5 );

	wxString m_cbLayerPresetsChoices[] = { _("Follow PCB Editor"), _("Follow PCB Plot Settings") };
	int m_cbLayerPresetsNChoices = sizeof( m_cbLayerPresetsChoices ) / sizeof( wxString );
	m_cbLayerPresets = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbLayerPresetsNChoices, m_cbLayerPresetsChoices, 0 );
	m_cbLayerPresets->SetSelection( 1 );
	bPresets->Add( m_cbLayerPresets, 0, wxEXPAND|wxTOP, 5 );


	bBottomMargin->Add( bPresets, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bBottomMargin->Add( 0, 2, 0, wxEXPAND, 5 );

	wxBoxSizer* bViewports;
	bViewports = new wxBoxSizer( wxVERTICAL );

	m_viewportsLabel = new wxStaticText( this, wxID_ANY, _("Viewports (Alt+Tab):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viewportsLabel->Wrap( -1 );
	bViewports->Add( m_viewportsLabel, 1, wxTOP, 5 );

	wxString m_cbViewportsChoices[] = { _("(unsaved)") };
	int m_cbViewportsNChoices = sizeof( m_cbViewportsChoices ) / sizeof( wxString );
	m_cbViewports = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbViewportsNChoices, m_cbViewportsChoices, 0 );
	m_cbViewports->SetSelection( 1 );
	bViewports->Add( m_cbViewports, 0, wxEXPAND|wxTOP, 5 );


	bBottomMargin->Add( bViewports, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_sizerOuter->Add( bBottomMargin, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );


	this->SetSizer( m_sizerOuter );
	this->Layout();
	m_sizerOuter->Fit( this );

	// Connect Events
	this->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_3D_BASE::OnSetFocus ) );
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( APPEARANCE_CONTROLS_3D_BASE::OnSize ) );
	m_panelLayers->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_3D_BASE::OnSetFocus ), NULL, this );
	m_cbLayerPresets->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_3D_BASE::onLayerPresetChanged ), NULL, this );
	m_cbViewports->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_3D_BASE::onViewportChanged ), NULL, this );
	m_cbViewports->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( APPEARANCE_CONTROLS_3D_BASE::onUpdateViewportsCb ), NULL, this );
}

APPEARANCE_CONTROLS_3D_BASE::~APPEARANCE_CONTROLS_3D_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_3D_BASE::OnSetFocus ) );
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( APPEARANCE_CONTROLS_3D_BASE::OnSize ) );
	m_panelLayers->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_3D_BASE::OnSetFocus ), NULL, this );
	m_cbLayerPresets->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_3D_BASE::onLayerPresetChanged ), NULL, this );
	m_cbViewports->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_3D_BASE::onViewportChanged ), NULL, this );
	m_cbViewports->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( APPEARANCE_CONTROLS_3D_BASE::onUpdateViewportsCb ), NULL, this );

}

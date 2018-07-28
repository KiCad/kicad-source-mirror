///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_hotkeys_editor_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_HOTKEYS_EDITOR_BASE::PANEL_HOTKEYS_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	m_mainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );
	
	m_panelHotkeys = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelHotkeys->SetMinSize( wxSize( -1,350 ) );
	
	bMargins->Add( m_panelHotkeys, 1, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 2 );
	
	wxBoxSizer* b_buttonsSizer;
	b_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_resetButton = new wxButton( this, wxID_RESET, _("Reset Hotkeys"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( m_resetButton, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );
	
	m_defaultButton = new wxButton( this, wxID_ANY, _("Set to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( m_defaultButton, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	b_buttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	btnImport = new wxButton( this, wxID_ANY, _("Import..."), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( btnImport, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	btnExport = new wxButton( this, wxID_ANY, _("Export..."), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( btnExport, 0, wxTOP|wxLEFT, 5 );
	
	
	bMargins->Add( b_buttonsSizer, 0, wxEXPAND, 5 );
	
	
	m_mainSizer->Add( bMargins, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );
	
	
	this->SetSizer( m_mainSizer );
	this->Layout();
	
	// Connect Events
	m_resetButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_HOTKEYS_EDITOR_BASE::ResetClicked ), NULL, this );
	m_defaultButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_HOTKEYS_EDITOR_BASE::DefaultsClicked ), NULL, this );
	btnImport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_HOTKEYS_EDITOR_BASE::OnImport ), NULL, this );
	btnExport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_HOTKEYS_EDITOR_BASE::OnExport ), NULL, this );
}

PANEL_HOTKEYS_EDITOR_BASE::~PANEL_HOTKEYS_EDITOR_BASE()
{
	// Disconnect Events
	m_resetButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_HOTKEYS_EDITOR_BASE::ResetClicked ), NULL, this );
	m_defaultButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_HOTKEYS_EDITOR_BASE::DefaultsClicked ), NULL, this );
	btnImport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_HOTKEYS_EDITOR_BASE::OnImport ), NULL, this );
	btnExport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_HOTKEYS_EDITOR_BASE::OnExport ), NULL, this );
	
}

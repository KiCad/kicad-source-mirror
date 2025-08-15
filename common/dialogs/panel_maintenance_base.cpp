///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_maintenance_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_MAINTENANCE_BASE::PANEL_MAINTENANCE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* margins;
	margins = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* b3DCacheSizer;
	b3DCacheSizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextClear3DCache = new wxStaticText( this, wxID_ANY, _("3D cache file duration:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextClear3DCache->Wrap( -1 );
	b3DCacheSizer->Add( m_staticTextClear3DCache, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_Clear3DCacheFilesOlder = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 120, 30 );
	m_Clear3DCacheFilesOlder->SetToolTip( _("3D cache files older than this are deleted.\nIf set to 0, cache clearing is disabled") );

	b3DCacheSizer->Add( m_Clear3DCacheFilesOlder, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_staticTextDays = new wxStaticText( this, wxID_ANY, _("days"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDays->Wrap( -1 );
	b3DCacheSizer->Add( m_staticTextDays, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	margins->Add( b3DCacheSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bResetStateSizer;
	bResetStateSizer = new wxBoxSizer( wxVERTICAL );

	m_clearFileHistory = new wxButton( this, wxID_ANY, _("Clear \"Open Recent\" History"), wxDefaultPosition, wxDefaultSize, 0 );
	bResetStateSizer->Add( m_clearFileHistory, 0, wxALL|wxEXPAND, 5 );


	bResetStateSizer->Add( 0, 10, 1, wxEXPAND, 5 );

	m_clearDontShowAgain = new wxButton( this, wxID_ANY, _("Reset \"Don't Show Again\" Dialogs"), wxDefaultPosition, wxDefaultSize, 0 );
	bResetStateSizer->Add( m_clearDontShowAgain, 0, wxALL|wxEXPAND, 5 );

	m_clearDialogState = new wxButton( this, wxID_ANY, _("Reset All Dialogs to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bResetStateSizer->Add( m_clearDialogState, 0, wxALL|wxEXPAND, 5 );

	m_resetAll = new wxButton( this, wxID_ANY, _("Reset All Program Settings to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bResetStateSizer->Add( m_resetAll, 0, wxALL, 5 );


	margins->Add( bResetStateSizer, 1, wxEXPAND|wxTOP, 10 );


	bPanelSizer->Add( margins, 0, wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_clearFileHistory->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MAINTENANCE_BASE::onClearFileHistory ), NULL, this );
	m_clearDontShowAgain->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MAINTENANCE_BASE::onClearDontShowAgain ), NULL, this );
	m_clearDialogState->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MAINTENANCE_BASE::onClearDialogState ), NULL, this );
	m_resetAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MAINTENANCE_BASE::onResetAll ), NULL, this );
}

PANEL_MAINTENANCE_BASE::~PANEL_MAINTENANCE_BASE()
{
	// Disconnect Events
	m_clearFileHistory->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MAINTENANCE_BASE::onClearFileHistory ), NULL, this );
	m_clearDontShowAgain->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MAINTENANCE_BASE::onClearDontShowAgain ), NULL, this );
	m_clearDialogState->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MAINTENANCE_BASE::onClearDialogState ), NULL, this );
	m_resetAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MAINTENANCE_BASE::onResetAll ), NULL, this );

}

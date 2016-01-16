///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 28 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_hotkeys_editor_base.h"

///////////////////////////////////////////////////////////////////////////

HOTKEYS_EDITOR_DIALOG_BASE::HOTKEYS_EDITOR_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_mainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Double-click to edit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( 400 );
	m_mainSizer->Add( m_staticText1, 0, wxALL|wxEXPAND, 5 );
	
	m_panelHotkeys = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_mainSizer->Add( m_panelHotkeys, 1, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* b_buttonsSizer;
	b_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_resetButton = new wxButton( this, wxID_RESET, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( m_resetButton, 0, wxALL|wxEXPAND, 5 );
	
	
	b_buttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	b_buttonsSizer->Add( m_sdbSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	m_mainSizer->Add( b_buttonsSizer, 0, wxALIGN_RIGHT|wxEXPAND, 5 );
	
	
	this->SetSizer( m_mainSizer );
	this->Layout();
	
	// Connect Events
	m_resetButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::ResetClicked ), NULL, this );
}

HOTKEYS_EDITOR_DIALOG_BASE::~HOTKEYS_EDITOR_DIALOG_BASE()
{
	// Disconnect Events
	m_resetButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::ResetClicked ), NULL, this );
	
}

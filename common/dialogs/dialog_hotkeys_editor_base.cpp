///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_hotkeys_editor_base.h"

///////////////////////////////////////////////////////////////////////////

HOTKEYS_EDITOR_DIALOG_BASE::HOTKEYS_EDITOR_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Select a row and press a new key combination to alter the binding."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( 400 );
	bMainSizer->Add( m_staticText1, 0, wxALL|wxEXPAND, 5 );
	
	m_hotkeySections = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	bMainSizer->Add( m_hotkeySections, 1, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* b_buttonsSizer;
	b_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_OKButton = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( m_OKButton, 0, wxALL|wxEXPAND, 5 );
	
	 m_cancelButton = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add(  m_cancelButton, 0, wxALL|wxEXPAND, 5 );
	
	m_undoButton = new wxButton( this, wxID_CANCEL, _("Undo"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( m_undoButton, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( b_buttonsSizer, 0, wxALIGN_CENTER|wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_OKButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnOKClicked ), NULL, this );
	 m_cancelButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::CancelClicked ), NULL, this );
	m_undoButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::UndoClicked ), NULL, this );
}

HOTKEYS_EDITOR_DIALOG_BASE::~HOTKEYS_EDITOR_DIALOG_BASE()
{
	// Disconnect Events
	m_OKButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnOKClicked ), NULL, this );
	 m_cancelButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::CancelClicked ), NULL, this );
	m_undoButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::UndoClicked ), NULL, this );
	
}

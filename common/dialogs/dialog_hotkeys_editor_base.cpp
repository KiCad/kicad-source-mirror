///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
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
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	b_buttonsSizer->Add( m_sdbSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_undoButton = new wxButton( this, wxID_UNDO, _("Undo"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( m_undoButton, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( b_buttonsSizer, 0, wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnOKClicked ), NULL, this );
	m_undoButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::UndoClicked ), NULL, this );
}

HOTKEYS_EDITOR_DIALOG_BASE::~HOTKEYS_EDITOR_DIALOG_BASE()
{
	// Disconnect Events
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnOKClicked ), NULL, this );
	m_undoButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::UndoClicked ), NULL, this );
	
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  6 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_exit_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXIT_BASE::DIALOG_EXIT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerBitmap;
	bSizerBitmap = new wxBoxSizer( wxVERTICAL );
	
	m_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBitmap->Add( m_bitmap, 0, wxALL, 5 );
	
	
	bSizerUpper->Add( bSizerBitmap, 0, 0, 5 );
	
	wxBoxSizer* bSizerMessages;
	bSizerMessages = new wxBoxSizer( wxVERTICAL );
	
	m_TextInfo = new wxStaticText( this, wxID_ANY, _("Save the changes before closing?"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextInfo->Wrap( -1 );
	m_TextInfo->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizerMessages->Add( m_TextInfo, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizerMessages->Add( 10, 10, 0, 0, 5 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("If you don't save, all your changes will be permanently lost."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizerMessages->Add( m_staticText2, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizerUpper->Add( bSizerMessages, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerLower;
	bSizerLower = new wxBoxSizer( wxVERTICAL );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerLower->Add( m_staticline, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonSaveAndExit = new wxButton( this, wxID_ANY, _("Save and Exit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSaveAndExit->SetDefault(); 
	bSizerButtons->Add( m_buttonSaveAndExit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonExitNoSave = new wxButton( this, wxID_ANY, _("Exit without Save"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonExitNoSave, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonCancel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerLower->Add( bSizerButtons, 0, wxALIGN_RIGHT, 5 );
	
	
	bSizerMain->Add( bSizerLower, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_buttonSaveAndExit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnSaveAndExit ), NULL, this );
	m_buttonExitNoSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnExitNoSave ), NULL, this );
}

DIALOG_EXIT_BASE::~DIALOG_EXIT_BASE()
{
	// Disconnect Events
	m_buttonSaveAndExit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnSaveAndExit ), NULL, this );
	m_buttonExitNoSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnExitNoSave ), NULL, this );
	
}

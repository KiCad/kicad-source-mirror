///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
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
	
	m_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerUpper->Add( m_bitmap, 0, wxALL, 5 );
	
	wxBoxSizer* bSizerMessages;
	bSizerMessages = new wxBoxSizer( wxVERTICAL );
	
	m_TextInfo = new wxStaticText( this, wxID_ANY, _("Save changes?"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextInfo->Wrap( -1 );
	m_TextInfo->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizerMessages->Add( m_TextInfo, 0, wxALL, 5 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("If you don't save, all your changes will be permanently lost."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizerMessages->Add( m_staticText2, 0, wxALL, 5 );
	
	
	bSizerUpper->Add( bSizerMessages, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND|wxALL, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );
	
	m_buttonSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonSizer->SetMinSize( wxSize( 475,-1 ) ); 
	m_ApplyToAllOpt = new wxCheckBox( this, wxID_ANY, _("Apply to all"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSizer->Add( m_ApplyToAllOpt, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );
	
	
	m_buttonSizer->Add( 20, 0, 1, wxRIGHT|wxLEFT, 5 );
	
	m_DiscardButton = new wxButton( this, wxID_ANY, _("Discard Changes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSizer->Add( m_DiscardButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_buttonSizer->Add( 20, 0, 0, wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	m_buttonSizer->Add( m_sdbSizer1, 0, wxALL, 5 );
	
	
	bSizerMain->Add( m_buttonSizer, 0, wxEXPAND|wxLEFT, 10 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_DiscardButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnDiscard ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnSave ), NULL, this );
}

DIALOG_EXIT_BASE::~DIALOG_EXIT_BASE()
{
	// Disconnect Events
	m_DiscardButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnDiscard ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnSave ), NULL, this );
	
}

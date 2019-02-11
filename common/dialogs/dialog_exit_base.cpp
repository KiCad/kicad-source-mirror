///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 21 2018)
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

	m_staticTextWarningMessage = new wxStaticText( this, wxID_ANY, _("If you don't save, all your changes will be permanently lost."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWarningMessage->Wrap( 300 );
	bSizerMessages->Add( m_staticTextWarningMessage, 0, wxALL, 5 );


	bSizerUpper->Add( bSizerMessages, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND|wxALL, 5 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_buttonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_ApplyToAllOpt = new wxCheckBox( this, wxID_ANY, _("Apply to all"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSizer->Add( m_ApplyToAllOpt, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxTOP, 5 );


	m_buttonSizer->Add( 20, 0, 1, wxRIGHT|wxLEFT, 5 );

	m_DiscardButton = new wxButton( this, wxID_ANY, _("Discard Changes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSizer->Add( m_DiscardButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_buttonSizer->Add( 20, 0, 0, wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_buttonSizer->Add( m_sdbSizer, 0, wxBOTTOM|wxTOP, 5 );


	bSizerMain->Add( m_buttonSizer, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_DiscardButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnDiscard ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnSave ), NULL, this );
}

DIALOG_EXIT_BASE::~DIALOG_EXIT_BASE()
{
	// Disconnect Events
	m_DiscardButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnDiscard ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXIT_BASE::OnSave ), NULL, this );

}

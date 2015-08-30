///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_scripting_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SCRIPTING_BASE::DIALOG_SCRIPTING_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	m_txScript = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTE_MULTILINE );
	m_txScript->SetMaxLength( 0 ); 
	m_txScript->SetMinSize( wxSize( 480,500 ) );
	
	bSizerMain->Add( m_txScript, 1, wxALL|wxEXPAND, 5 );
	
	m_btRun = new wxButton( this, wxID_ANY, wxT("&Run"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMain->Add( m_btRun, 0, wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_btRun->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCRIPTING_BASE::OnRunButtonClick ), NULL, this );
}

DIALOG_SCRIPTING_BASE::~DIALOG_SCRIPTING_BASE()
{
	// Disconnect Events
	m_btRun->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCRIPTING_BASE::OnRunButtonClick ), NULL, this );
	
}

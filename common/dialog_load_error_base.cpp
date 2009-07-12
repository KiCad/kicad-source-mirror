///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 19 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_load_error_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LOAD_ERROR_BASE::DIALOG_LOAD_ERROR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 320,240 ), wxDefaultSize );
	
	wxBoxSizer* MainSizer;
	MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* MessageSizer;
	MessageSizer = new wxBoxSizer( wxVERTICAL );
	
	StaticTextMessage = new wxStaticText( this, wxID_ANY, _("message dummy"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	StaticTextMessage->Wrap( -1 );
	MessageSizer->Add( StaticTextMessage, 0, wxALIGN_LEFT|wxALIGN_TOP|wxALL|wxEXPAND, 5 );
	
	MainSizer->Add( MessageSizer, 0, wxALIGN_BOTTOM|wxALIGN_LEFT|wxALIGN_RIGHT|wxALIGN_TOP|wxEXPAND, 5 );
	
	wxBoxSizer* ListSizer;
	ListSizer = new wxBoxSizer( wxVERTICAL );
	
	TextCtrlList = new wxTextCtrl( this, wxID_ANY, _("list dummy"), wxPoint( 0,0 ), wxSize( -1,-1 ), wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL|wxNO_BORDER|wxVSCROLL );
	ListSizer->Add( TextCtrlList, 1, wxALIGN_BOTTOM|wxALIGN_CENTER_HORIZONTAL|wxALIGN_TOP|wxEXPAND|wxALL, 5 );
	
	MainSizer->Add( ListSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	OkButton = new wxButton( this, wxID_ANY, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	OkButton->SetDefault(); 
	bSizer5->Add( OkButton, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	MainSizer->Add( bSizer5, 0, wxEXPAND, 5 );
	
	this->SetSizer( MainSizer );
	this->Layout();
	MainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	OkButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LOAD_ERROR_BASE::OnOkClick ), NULL, this );
}

DIALOG_LOAD_ERROR_BASE::~DIALOG_LOAD_ERROR_BASE()
{
	// Disconnect Events
	OkButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LOAD_ERROR_BASE::OnOkClick ), NULL, this );
}

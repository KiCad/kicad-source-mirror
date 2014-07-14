///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 30 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_enum_pads_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ENUM_PADS_BASE::DIALOG_ENUM_PADS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bPrefixSizer;
	bPrefixSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_lblPadPrefix = new wxStaticText( this, wxID_ANY, _("Pad name prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPadPrefix->Wrap( -1 );
	bPrefixSizer->Add( m_lblPadPrefix, 1, wxALL, 5 );
	
	m_padPrefix = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_padPrefix->SetMaxLength( 4 ); 
	bPrefixSizer->Add( m_padPrefix, 0, wxALL, 5 );
	
	
	bMainSizer->Add( bPrefixSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bPadNumSizer;
	bPadNumSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_lblPadStartNum = new wxStaticText( this, wxID_ANY, _("First pad number:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPadStartNum->Wrap( -1 );
	bPadNumSizer->Add( m_lblPadStartNum, 1, wxALL, 5 );
	
	m_padStartNum = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999, 1 );
	bPadNumSizer->Add( m_padStartNum, 0, wxALL, 5 );
	
	
	bMainSizer->Add( bPadNumSizer, 1, wxEXPAND, 5 );
	
	m_lblInfo = new wxStaticText( this, wxID_ANY, _("Pad names are restricted to 4 characters (including number)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblInfo->Wrap( 320 );
	bMainSizer->Add( m_lblInfo, 0, wxALL, 5 );
	
	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();
	
	bMainSizer->Add( m_stdButtons, 2, wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_ENUM_PADS_BASE::~DIALOG_ENUM_PADS_BASE()
{
}

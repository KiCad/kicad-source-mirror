///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  6 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_about_base.h"

///////////////////////////////////////////////////////////////////////////

dialog_about_base::dialog_about_base( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bitmapApp = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_bitmapApp, 1, wxALIGN_CENTER|wxALL, 5 );
	
	wxBoxSizer* b_apptitleSizer;
	b_apptitleSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextAppTitle = new wxStaticText( this, wxID_ANY, _("App Title"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticTextAppTitle->Wrap( -1 );
	m_staticTextAppTitle->SetFont( wxFont( 14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	b_apptitleSizer->Add( m_staticTextAppTitle, 0, wxALIGN_CENTER|wxALL, 5 );
	
	m_staticTextCopyright = new wxStaticText( this, wxID_ANY, _("Copyright Info"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticTextCopyright->Wrap( -1 );
	b_apptitleSizer->Add( m_staticTextCopyright, 0, wxALIGN_CENTER|wxALL, 1 );
	
	m_staticTextBuildVersion = new wxStaticText( this, wxID_ANY, _("Build Version Info"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticTextBuildVersion->Wrap( -1 );
	b_apptitleSizer->Add( m_staticTextBuildVersion, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticTextLibVersion = new wxStaticText( this, wxID_ANY, _("Lib Version Info"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticTextLibVersion->Wrap( -1 );
	b_apptitleSizer->Add( m_staticTextLibVersion, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	
	bSizer3->Add( b_apptitleSizer, 10, wxALL|wxEXPAND, 5 );
	
	
	bSizer3->Add( 0, 0, 2, wxEXPAND, 5 );
	
	
	bSizer1->Add( bSizer3, 0, wxEXPAND, 5 );
	
	m_auiNotebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_SCROLL_BUTTONS|wxAUI_NB_TAB_FIXED_WIDTH );
	m_auiNotebook->SetMinSize( wxSize( 750,350 ) );
	
	
	bSizer1->Add( m_auiNotebook, 2, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer4->Add( 0, 0, 1, wxEXPAND, 5 );
	
	copyVersionInfo = new wxButton( this, wxID_COPY, _("Copy Version Info"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( copyVersionInfo, 0, wxALL, 5 );
	
	ok = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	ok->SetDefault(); 
	bSizer4->Add( ok, 0, wxALL, 5 );
	
	
	bSizer1->Add( bSizer4, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	this->SetSizer( bSizer1 );
	this->Layout();
}

dialog_about_base::~dialog_about_base()
{
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
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
	m_staticTextAppTitle->SetFont( wxFont( 14, 70, 90, 92, false, wxEmptyString ) );
	
	b_apptitleSizer->Add( m_staticTextAppTitle, 0, wxALIGN_CENTER|wxALL, 5 );
	
	m_staticTextCopyright = new wxStaticText( this, wxID_ANY, _("Copyright Info"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticTextCopyright->Wrap( -1 );
	b_apptitleSizer->Add( m_staticTextCopyright, 0, wxALIGN_CENTER|wxALL, 1 );
	
	m_staticTextBuildVersion = new wxStaticText( this, wxID_ANY, _("Build Version Info"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticTextBuildVersion->Wrap( -1 );
	b_apptitleSizer->Add( m_staticTextBuildVersion, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticTextLibVersion = new wxStaticText( this, wxID_ANY, _("Lib Version Info"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticTextLibVersion->Wrap( -1 );
	b_apptitleSizer->Add( m_staticTextLibVersion, 0, wxALIGN_CENTER|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	
	bSizer3->Add( b_apptitleSizer, 10, wxEXPAND, 5 );
	
	
	bSizer3->Add( 0, 0, 2, wxEXPAND, 5 );
	
	
	bSizer1->Add( bSizer3, 0, wxEXPAND, 5 );
	
	wxStaticLine* m_staticline1;
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_auiNotebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_SCROLL_BUTTONS|wxAUI_NB_TAB_FIXED_WIDTH );
	m_auiNotebook->SetMinSize( wxSize( 750,350 ) );
	
	
	bSizer1->Add( m_auiNotebook, 2, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizer->Realize();
	
	bSizer1->Add( m_sdbSizer, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	this->SetSizer( bSizer1 );
	this->Layout();
}

dialog_about_base::~dialog_about_base()
{
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 10 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_show_page_borders_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE::DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bDialogSizer;
	bDialogSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_ShowPageLimitsChoices[] = { _("Full size. Do not show page limits"), _("Full size"), _("Size A4"), _("Size A3"), _("Size A2"), _("Size A"), _("Size B"), _("Size C") };
	int m_ShowPageLimitsNChoices = sizeof( m_ShowPageLimitsChoices ) / sizeof( wxString );
	m_ShowPageLimits = new wxRadioBox( this, wxID_ANY, _("Show Page Limits:"), wxDefaultPosition, wxDefaultSize, m_ShowPageLimitsNChoices, m_ShowPageLimitsChoices, 1, wxRA_SPECIFY_COLS );
	m_ShowPageLimits->SetSelection( 0 );
	bRightSizer->Add( m_ShowPageLimits, 0, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bRightSizer, 1, wxEXPAND, 5 );
	
	
	bDialogSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bDialogSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bDialogSizer->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bDialogSizer );
	this->Layout();
	
	// Connect Events
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE::OnOKBUttonClick ), NULL, this );
}

DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE::~DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE()
{
	// Disconnect Events
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE::OnOKBUttonClick ), NULL, this );
	
}

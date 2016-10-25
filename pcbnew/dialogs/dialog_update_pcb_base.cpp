///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_update_pcb_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_UPDATE_PCB_BASE::DIALOG_UPDATE_PCB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 800,700 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Match components by:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer2->Add( m_staticText1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_matchByReference = new wxRadioButton( this, wxID_ANY, _("Reference"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_matchByReference, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_matchByTimestamp = new wxRadioButton( this, wxID_ANY, _("Timestamp"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_matchByTimestamp, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainSizer->Add( fgSizer2, 0, wxEXPAND|wxALL, 5 );
	
	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );
	
	m_messagePanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_messagePanel->SetMinSize( wxSize( -300,150 ) );
	
	bLowerSizer->Add( m_messagePanel, 1, wxEXPAND | wxALL, 5 );
	
	
	bMainSizer->Add( bLowerSizer, 1, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 1, 5, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_btnCancel = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_btnCancel, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_btnPerformUpdate = new wxButton( this, wxID_ANY, _("Perform PCB Update"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_btnPerformUpdate, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainSizer->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	m_matchByReference->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnMatchChange ), NULL, this );
	m_matchByTimestamp->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnMatchChange ), NULL, this );
	m_btnPerformUpdate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnUpdateClick ), NULL, this );
}

DIALOG_UPDATE_PCB_BASE::~DIALOG_UPDATE_PCB_BASE()
{
	// Disconnect Events
	m_matchByReference->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnMatchChange ), NULL, this );
	m_matchByTimestamp->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnMatchChange ), NULL, this );
	m_btnPerformUpdate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnUpdateClick ), NULL, this );
	
}

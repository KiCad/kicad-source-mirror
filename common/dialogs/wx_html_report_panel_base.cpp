///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel_base.h"

///////////////////////////////////////////////////////////////////////////

WX_HTML_REPORT_PANEL_BASE::WX_HTML_REPORT_PANEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Messages:") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 2, 1, 0, 0 );
	fgSizer4->AddGrowableCol( 0 );
	fgSizer4->AddGrowableRow( 0 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_htmlView = new wxHtmlWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_htmlView->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	fgSizer4->Add( m_htmlView, 1, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 1, 7, 0, 0 );
	fgSizer3->AddGrowableCol( 6 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, wxT("Filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer3->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	m_checkBoxShowAll = new wxCheckBox( this, wxID_ANY, wxT("All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowAll->SetValue(true); 
	fgSizer3->Add( m_checkBoxShowAll, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_checkBoxShowWarnings = new wxCheckBox( this, wxID_ANY, wxT("Warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowWarnings->Enable( false );
	
	fgSizer3->Add( m_checkBoxShowWarnings, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_checkBoxShowErrors = new wxCheckBox( this, wxID_ANY, wxT("Errors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowErrors->Enable( false );
	
	fgSizer3->Add( m_checkBoxShowErrors, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_checkBoxShowInfos = new wxCheckBox( this, wxID_ANY, wxT("Infos"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowInfos->Enable( false );
	
	fgSizer3->Add( m_checkBoxShowInfos, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_checkBoxShowActions = new wxCheckBox( this, wxID_ANY, wxT("Actions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowActions->Enable( false );
	
	fgSizer3->Add( m_checkBoxShowActions, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_btnSaveReportToFile = new wxButton( this, wxID_ANY, wxT("Save report to file..."), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_btnSaveReportToFile, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxTOP|wxLEFT, 5 );
	
	
	fgSizer4->Add( fgSizer3, 1, wxEXPAND, 5 );
	
	
	sbSizer3->Add( fgSizer4, 1, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( sbSizer3 );
	this->Layout();
	
	// Connect Events
	m_checkBoxShowAll->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBoxShowAll ), NULL, this );
	m_checkBoxShowWarnings->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBoxShowWarnings ), NULL, this );
	m_checkBoxShowErrors->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBoxShowErrors ), NULL, this );
	m_checkBoxShowInfos->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBoxShowInfos ), NULL, this );
	m_checkBoxShowActions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBoxShowActions ), NULL, this );
	m_btnSaveReportToFile->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onBtnSaveToFile ), NULL, this );
}

WX_HTML_REPORT_PANEL_BASE::~WX_HTML_REPORT_PANEL_BASE()
{
	// Disconnect Events
	m_checkBoxShowAll->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBoxShowAll ), NULL, this );
	m_checkBoxShowWarnings->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBoxShowWarnings ), NULL, this );
	m_checkBoxShowErrors->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBoxShowErrors ), NULL, this );
	m_checkBoxShowInfos->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBoxShowInfos ), NULL, this );
	m_checkBoxShowActions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBoxShowActions ), NULL, this );
	m_btnSaveReportToFile->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onBtnSaveToFile ), NULL, this );
	
}

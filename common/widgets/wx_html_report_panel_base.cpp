///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel_base.h"

///////////////////////////////////////////////////////////////////////////

WX_HTML_REPORT_PANEL_BASE::WX_HTML_REPORT_PANEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetMinSize( wxSize( -1,225 ) );

	m_box = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Output Messages") ), wxVERTICAL );

	m_fgSizer = new wxFlexGridSizer( 2, 1, 0, 0 );
	m_fgSizer->AddGrowableCol( 0 );
	m_fgSizer->AddGrowableRow( 0 );
	m_fgSizer->SetFlexibleDirection( wxBOTH );
	m_fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_htmlView = new HTML_WINDOW( m_box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_fgSizer->Add( m_htmlView, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextShow = new wxStaticText( m_box->GetStaticBox(), wxID_ANY, _("Show:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextShow->Wrap( -1 );
	bSizerBottom->Add( m_staticTextShow, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 2 );

	m_checkBoxShowAll = new wxCheckBox( m_box->GetStaticBox(), wxID_ANY, _("All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowAll->SetValue(true);
	bSizerBottom->Add( m_checkBoxShowAll, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizerBottom->Add( 30, 0, 0, wxEXPAND, 5 );

	m_checkBoxShowErrors = new wxCheckBox( m_box->GetStaticBox(), wxID_ANY, _("Errors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowErrors->SetValue(true);
	bSizerBottom->Add( m_checkBoxShowErrors, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_errorsBadge = new NUMBER_BADGE( m_box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_errorsBadge->SetMinSize( wxSize( 10,10 ) );

	bSizerBottom->Add( m_errorsBadge, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 4 );


	bSizerBottom->Add( 25, 0, 0, wxEXPAND, 5 );

	m_checkBoxShowWarnings = new wxCheckBox( m_box->GetStaticBox(), wxID_ANY, _("Warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowWarnings->SetValue(true);
	bSizerBottom->Add( m_checkBoxShowWarnings, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_warningsBadge = new NUMBER_BADGE( m_box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_warningsBadge->SetMinSize( wxSize( 10,10 ) );

	bSizerBottom->Add( m_warningsBadge, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 4 );


	bSizerBottom->Add( 25, 0, 0, wxEXPAND, 5 );

	m_checkBoxShowActions = new wxCheckBox( m_box->GetStaticBox(), wxID_ANY, _("Actions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowActions->SetValue(true);
	bSizerBottom->Add( m_checkBoxShowActions, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizerBottom->Add( 30, 0, 0, wxEXPAND, 5 );

	m_checkBoxShowInfos = new wxCheckBox( m_box->GetStaticBox(), wxID_ANY, _("Infos"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowInfos->SetValue(true);
	bSizerBottom->Add( m_checkBoxShowInfos, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizerBottom->Add( 30, 0, 1, wxEXPAND, 5 );

	m_btnSaveReportToFile = new wxButton( m_box->GetStaticBox(), wxID_ANY, _("Save..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_btnSaveReportToFile, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_fgSizer->Add( bSizerBottom, 0, wxEXPAND, 5 );


	m_box->Add( m_fgSizer, 1, wxEXPAND, 5 );


	this->SetSizer( m_box );
	this->Layout();
	m_box->Fit( this );

	// Connect Events
	m_htmlView->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( WX_HTML_REPORT_PANEL_BASE::onRightClick ), NULL, this );
	m_checkBoxShowAll->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBox ), NULL, this );
	m_checkBoxShowErrors->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBox ), NULL, this );
	m_checkBoxShowWarnings->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBox ), NULL, this );
	m_checkBoxShowActions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBox ), NULL, this );
	m_checkBoxShowInfos->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBox ), NULL, this );
	m_btnSaveReportToFile->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onBtnSaveToFile ), NULL, this );
}

WX_HTML_REPORT_PANEL_BASE::~WX_HTML_REPORT_PANEL_BASE()
{
	// Disconnect Events
	m_htmlView->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( WX_HTML_REPORT_PANEL_BASE::onRightClick ), NULL, this );
	m_checkBoxShowAll->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBox ), NULL, this );
	m_checkBoxShowErrors->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBox ), NULL, this );
	m_checkBoxShowWarnings->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBox ), NULL, this );
	m_checkBoxShowActions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBox ), NULL, this );
	m_checkBoxShowInfos->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onCheckBox ), NULL, this );
	m_btnSaveReportToFile->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WX_HTML_REPORT_PANEL_BASE::onBtnSaveToFile ), NULL, this );

}

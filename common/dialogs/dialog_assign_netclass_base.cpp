///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_html_report_box.h"

#include "dialog_assign_netclass_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ASSIGN_NETCLASS_BASE::DIALOG_ASSIGN_NETCLASS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );

	stPatternLabel = new wxStaticText( this, wxID_ANY, _("Pattern:"), wxDefaultPosition, wxDefaultSize, 0 );
	stPatternLabel->Wrap( -1 );
	bUpperSizer->Add( stPatternLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_patternCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_patternCtrl->SetMinSize( wxSize( 240,-1 ) );

	bUpperSizer->Add( m_patternCtrl, 1, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	stNetclassLabel = new wxStaticText( this, wxID_ANY, _("Net class:"), wxDefaultPosition, wxDefaultSize, 0 );
	stNetclassLabel->Wrap( -1 );
	bUpperSizer->Add( stNetclassLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_netclassCtrl = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	bUpperSizer->Add( m_netclassCtrl, 0, wxALL, 5 );


	bMainSizer->Add( bUpperSizer, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	m_matchingNets = new WX_HTML_REPORT_BOX( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_matchingNets->SetMinSize( wxSize( -1,200 ) );

	bLowerSizer->Add( m_matchingNets, 2, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_info = new wxStaticText( this, wxID_ANY, _("Note: complete netclass assignments can be edited in Schematic Setup > Project."), wxDefaultPosition, wxDefaultSize, 0 );
	m_info->Wrap( -1 );
	bLowerSizer->Add( m_info, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bLowerSizer, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();

	bMainSizer->Add( m_sdbSizerStdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_patternCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_ASSIGN_NETCLASS_BASE::onPatternText ), NULL, this );
}

DIALOG_ASSIGN_NETCLASS_BASE::~DIALOG_ASSIGN_NETCLASS_BASE()
{
	// Disconnect Events
	m_patternCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_ASSIGN_NETCLASS_BASE::onPatternText ), NULL, this );

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_pns_diff_pair_dimensions_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 400,-1 ), wxDefaultSize );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_traceWidthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_traceWidthLabel->Wrap( -1 );
	fgSizer1->Add( m_traceWidthLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_traceWidthText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_traceWidthText, 0, wxALL|wxEXPAND, 5 );
	
	m_traceWidthUnit = new wxStaticText( this, wxID_ANY, _("u"), wxDefaultPosition, wxDefaultSize, 0 );
	m_traceWidthUnit->Wrap( -1 );
	fgSizer1->Add( m_traceWidthUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_traceGapLabel = new wxStaticText( this, wxID_ANY, _("Trace gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_traceGapLabel->Wrap( -1 );
	fgSizer1->Add( m_traceGapLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_traceGapText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_traceGapText, 0, wxALL|wxEXPAND, 5 );
	
	m_traceGapUnit = new wxStaticText( this, wxID_ANY, _("u"), wxDefaultPosition, wxDefaultSize, 0 );
	m_traceGapUnit->Wrap( -1 );
	m_traceGapUnit->SetMaxSize( wxSize( 40,-1 ) );
	
	fgSizer1->Add( m_traceGapUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_viaGapLabel = new wxStaticText( this, wxID_ANY, _("Via gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaGapLabel->Wrap( -1 );
	m_viaGapLabel->Enable( false );
	
	fgSizer1->Add( m_viaGapLabel, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_viaGapText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_viaGapText->Enable( false );
	
	fgSizer1->Add( m_viaGapText, 0, wxALL|wxEXPAND, 5 );
	
	m_viaGapUnit = new wxStaticText( this, wxID_ANY, _("u"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaGapUnit->Wrap( -1 );
	m_viaGapUnit->Enable( false );
	m_viaGapUnit->SetMaxSize( wxSize( 40,-1 ) );
	
	fgSizer1->Add( m_viaGapUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer7->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	m_viaTraceGapEqual = new wxCheckBox( this, wxID_ANY, _("Via gap same as trace gap"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaTraceGapEqual->SetValue(true); 
	bSizer7->Add( m_viaTraceGapEqual, 0, wxALL|wxEXPAND, 5 );
	
	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();
	
	bSizer7->Add( m_stdButtons, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer7 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::OnClose ) );
	m_viaTraceGapEqual->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::onViaTraceGapEqualCheck ), NULL, this );
	m_stdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::OnCancelClick ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::OnOkClick ), NULL, this );
}

DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::~DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::OnClose ) );
	m_viaTraceGapEqual->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::onViaTraceGapEqualCheck ), NULL, this );
	m_stdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::OnCancelClick ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::OnOkClick ), NULL, this );
	
}

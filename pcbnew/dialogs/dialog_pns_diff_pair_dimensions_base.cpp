///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_pns_diff_pair_dimensions_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_traceWidthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_traceWidthLabel->Wrap( -1 );
	fgSizer1->Add( m_traceWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_traceWidthText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_traceWidthText, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_traceWidthUnit = new wxStaticText( this, wxID_ANY, _("u"), wxDefaultPosition, wxDefaultSize, 0 );
	m_traceWidthUnit->Wrap( -1 );
	fgSizer1->Add( m_traceWidthUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_traceGapLabel = new wxStaticText( this, wxID_ANY, _("Track gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_traceGapLabel->Wrap( -1 );
	fgSizer1->Add( m_traceGapLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_traceGapText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_traceGapText, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_traceGapUnit = new wxStaticText( this, wxID_ANY, _("u"), wxDefaultPosition, wxDefaultSize, 0 );
	m_traceGapUnit->Wrap( -1 );
	fgSizer1->Add( m_traceGapUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_viaGapLabel = new wxStaticText( this, wxID_ANY, _("Via gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaGapLabel->Wrap( -1 );
	m_viaGapLabel->Enable( false );

	fgSizer1->Add( m_viaGapLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	m_viaGapText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_viaGapText->Enable( false );

	fgSizer1->Add( m_viaGapText, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_viaGapUnit = new wxStaticText( this, wxID_ANY, _("u"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaGapUnit->Wrap( -1 );
	m_viaGapUnit->Enable( false );

	fgSizer1->Add( m_viaGapUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizer7->Add( fgSizer1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_viaTraceGapEqual = new wxCheckBox( this, wxID_ANY, _("Via gap same as track gap"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaTraceGapEqual->SetValue(true);
	bSizer7->Add( m_viaTraceGapEqual, 0, wxALL|wxEXPAND, 10 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bSizer7->Add( m_stdButtons, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizer7 );
	this->Layout();
	bSizer7->Fit( this );

	// Connect Events
	m_viaTraceGapEqual->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::OnViaTraceGapEqualCheck ), NULL, this );
}

DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::~DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE()
{
	// Disconnect Events
	m_viaTraceGapEqual->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE::OnViaTraceGapEqualCheck ), NULL, this );

}

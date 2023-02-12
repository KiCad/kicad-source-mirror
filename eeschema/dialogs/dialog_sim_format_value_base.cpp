///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_sim_format_value_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SIM_FORMAT_VALUE_BASE::DIALOG_SIM_FORMAT_VALUE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 3, 2, 8, 0 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_precisionLabel = new wxStaticText( this, wxID_ANY, _("Significant digits:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_precisionLabel->Wrap( -1 );
	fgSizer->Add( m_precisionLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_precisionCtrl = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9, 3 );
	fgSizer->Add( m_precisionCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_rangeLabel = new wxStaticText( this, wxID_ANY, _("Range:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rangeLabel->Wrap( -1 );
	fgSizer->Add( m_rangeLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_rangeCtrlChoices[] = { _("Auto"), _("f"), _("p"), _("n"), _("u"), _("m"), wxEmptyString, _("K"), _("M"), _("G"), _("T"), _("P") };
	int m_rangeCtrlNChoices = sizeof( m_rangeCtrlChoices ) / sizeof( wxString );
	m_rangeCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_rangeCtrlNChoices, m_rangeCtrlChoices, 0 );
	m_rangeCtrl->SetSelection( 0 );
	fgSizer->Add( m_rangeCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	bMainSizer->Add( fgSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bBottomSizer;
	bBottomSizer = new wxBoxSizer( wxHORIZONTAL );


	bBottomSizer->Add( 50, 0, 1, wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bBottomSizer->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bBottomSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_SIM_FORMAT_VALUE_BASE::~DIALOG_SIM_FORMAT_VALUE_BASE()
{
}

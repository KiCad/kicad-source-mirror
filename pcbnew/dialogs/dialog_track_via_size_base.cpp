///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_track_via_size_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TRACK_VIA_SIZE_BASE::DIALOG_TRACK_VIA_SIZE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bSizes;
	bSizes = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_trackWidthLabel = new wxStaticText( this, wxID_ANY, _("Track width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackWidthLabel->Wrap( -1 );
	fgSizer1->Add( m_trackWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_trackWidthText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_trackWidthText, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_trackWidthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackWidthUnits->Wrap( -1 );
	fgSizer1->Add( m_trackWidthUnits, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_viaDiameterLabel = new wxStaticText( this, wxID_ANY, _("Via diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaDiameterLabel->Wrap( -1 );
	fgSizer1->Add( m_viaDiameterLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_viaDiameterText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_viaDiameterText, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_viaDiameterUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaDiameterUnits->Wrap( -1 );
	fgSizer1->Add( m_viaDiameterUnits, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_viaDrillLabel = new wxStaticText( this, wxID_ANY, _("Via hole:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaDrillLabel->Wrap( -1 );
	fgSizer1->Add( m_viaDrillLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_viaDrillText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_viaDrillText, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_viaDrillUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaDrillUnits->Wrap( -1 );
	fgSizer1->Add( m_viaDrillUnits, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizes->Add( fgSizer1, 1, wxEXPAND|wxALL, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizes->Add( m_staticline1, 0, wxEXPAND, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bSizes->Add( m_stdButtons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bSizes );
	this->Layout();
	bSizes->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_TRACK_VIA_SIZE_BASE::~DIALOG_TRACK_VIA_SIZE_BASE()
{
}

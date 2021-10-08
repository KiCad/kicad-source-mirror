///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_color_code_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_COLOR_CODE_BASE::PANEL_COLOR_CODE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerPanelColorCode;
	bSizerPanelColorCode = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerOpts;
	bSizerOpts = new wxBoxSizer( wxVERTICAL );

	wxString m_rbToleranceSelectionChoices[] = { _("10% / 5%"), _("<= 2%") };
	int m_rbToleranceSelectionNChoices = sizeof( m_rbToleranceSelectionChoices ) / sizeof( wxString );
	m_rbToleranceSelection = new wxRadioBox( this, wxID_ANY, _("Tolerance"), wxDefaultPosition, wxDefaultSize, m_rbToleranceSelectionNChoices, m_rbToleranceSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbToleranceSelection->SetSelection( 0 );
	bSizerOpts->Add( m_rbToleranceSelection, 0, wxBOTTOM|wxRIGHT, 30 );


	bSizerPanelColorCode->Add( bSizerOpts, 0, wxALL, 8 );

	wxFlexGridSizer* fgSizerColoCode;
	fgSizerColoCode = new wxFlexGridSizer( 2, 6, 0, 0 );
	fgSizerColoCode->SetFlexibleDirection( wxBOTH );
	fgSizerColoCode->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextBand1 = new wxStaticText( this, wxID_ANY, _("1st Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBand1->Wrap( -1 );
	fgSizerColoCode->Add( m_staticTextBand1, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextBand2 = new wxStaticText( this, wxID_ANY, _("2nd Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBand2->Wrap( -1 );
	fgSizerColoCode->Add( m_staticTextBand2, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextBand3 = new wxStaticText( this, wxID_ANY, _("3rd Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBand3->Wrap( -1 );
	fgSizerColoCode->Add( m_staticTextBand3, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextBand4 = new wxStaticText( this, wxID_ANY, _("4th Band"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBand4->Wrap( -1 );
	fgSizerColoCode->Add( m_staticTextBand4, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextBand5 = new wxStaticText( this, wxID_ANY, _("Multiplier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBand5->Wrap( -1 );
	fgSizerColoCode->Add( m_staticTextBand5, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextBand6 = new wxStaticText( this, wxID_ANY, _("Tolerance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBand6->Wrap( -1 );
	fgSizerColoCode->Add( m_staticTextBand6, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_Band1bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band1bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_Band2bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band2bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_Band3bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band3bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_Band4bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band4bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	m_Band_mult_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band_mult_bitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	m_Band_tol_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerColoCode->Add( m_Band_tol_bitmap, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


	bSizerPanelColorCode->Add( fgSizerColoCode, 1, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( bSizerPanelColorCode );
	this->Layout();

	// Connect Events
	m_rbToleranceSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PANEL_COLOR_CODE_BASE::OnToleranceSelection ), NULL, this );
}

PANEL_COLOR_CODE_BASE::~PANEL_COLOR_CODE_BASE()
{
	// Disconnect Events
	m_rbToleranceSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PANEL_COLOR_CODE_BASE::OnToleranceSelection ), NULL, this );

}

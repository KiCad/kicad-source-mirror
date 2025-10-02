///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_formatting_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_FORMATTING_BASE::PANEL_SETUP_FORMATTING_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_dashedLinesLabel = new wxStaticText( this, wxID_ANY, _("Dashed Lines"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dashedLinesLabel->Wrap( -1 );
	bMargins->Add( m_dashedLinesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 8 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMargins->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	wxFlexGridSizer* fgSizer41;
	fgSizer41 = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizer41->SetFlexibleDirection( wxBOTH );
	fgSizer41->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	dashLengthLabel = new wxStaticText( this, wxID_ANY, _("Dash length:"), wxDefaultPosition, wxDefaultSize, 0 );
	dashLengthLabel->Wrap( -1 );
	fgSizer41->Add( dashLengthLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_dashLengthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer41->Add( m_dashLengthCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	gapLengthLabel = new wxStaticText( this, wxID_ANY, _("Gap length:"), wxDefaultPosition, wxDefaultSize, 0 );
	gapLengthLabel->Wrap( -1 );
	fgSizer41->Add( gapLengthLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_gapLengthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer41->Add( m_gapLengthCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bMargins->Add( fgSizer41, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_dashedLineHelp = new wxStaticText( this, wxID_ANY, _("Dash and dot lengths are ratios of the line width."), wxDefaultPosition, wxDefaultSize, 0 );
	m_dashedLineHelp->Wrap( -1 );
	bMargins->Add( m_dashedLineHelp, 0, wxALL, 10 );


	bMargins->Add( 0, 10, 0, wxEXPAND, 5 );

	m_staticText5 = new wxStaticText( this, wxID_ANY, _("When Adding Footprints to Board"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bMargins->Add( m_staticText5, 0, wxTOP|wxRIGHT|wxLEFT, 8 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMargins->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_styleFields = new wxCheckBox( this, wxID_ANY, _("Apply board defaults to footprint fields"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_styleFields, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_styleText = new wxCheckBox( this, wxID_ANY, _("Apply board defaults to footprint text"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_styleText, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_styleShapes = new wxCheckBox( this, wxID_ANY, _("Apply board defaults to non-copper footprint shapes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_styleShapes, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_styleDimensions = new wxCheckBox( this, wxID_ANY, _("Apply board defaults to footprint dimensions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_styleDimensions, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_styleBarcodes = new wxCheckBox( this, wxID_ANY, _("Apply board defaults to footprint barcodes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_styleBarcodes, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bMargins->Add( bSizer3, 1, wxEXPAND|wxTOP|wxLEFT, 5 );


	bMainSizer->Add( bMargins, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_SETUP_FORMATTING_BASE::~PANEL_SETUP_FORMATTING_BASE()
{
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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

	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Dashed Lines") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer41;
	fgSizer41 = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizer41->SetFlexibleDirection( wxBOTH );
	fgSizer41->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	dashLengthLabel = new wxStaticText( sbSizer6->GetStaticBox(), wxID_ANY, _("Dash length:"), wxDefaultPosition, wxDefaultSize, 0 );
	dashLengthLabel->Wrap( -1 );
	fgSizer41->Add( dashLengthLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_dashLengthCtrl = new wxTextCtrl( sbSizer6->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer41->Add( m_dashLengthCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	gapLengthLabel = new wxStaticText( sbSizer6->GetStaticBox(), wxID_ANY, _("Gap length:"), wxDefaultPosition, wxDefaultSize, 0 );
	gapLengthLabel->Wrap( -1 );
	fgSizer41->Add( gapLengthLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_gapLengthCtrl = new wxTextCtrl( sbSizer6->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer41->Add( m_gapLengthCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer6->Add( fgSizer41, 0, wxEXPAND|wxBOTTOM, 5 );

	m_dashedLineHelp = new wxStaticText( sbSizer6->GetStaticBox(), wxID_ANY, _("Dash and dot lengths are ratios of the line width."), wxDefaultPosition, wxDefaultSize, 0 );
	m_dashedLineHelp->Wrap( -1 );
	sbSizer6->Add( m_dashedLineHelp, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );


	bMargins->Add( sbSizer6, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bMargins, 1, wxEXPAND|wxTOP|wxRIGHT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_SETUP_FORMATTING_BASE::~PANEL_SETUP_FORMATTING_BASE()
{
}

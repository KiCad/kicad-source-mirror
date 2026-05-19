///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"

#include "dialog_diff_phase_skew_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DIFF_PHASE_SKEW_PROPERTIES_BASE::DIALOG_DIFF_PHASE_SKEW_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* singleTrackSizer;
	singleTrackSizer = new wxBoxSizer( wxHORIZONTAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 4, 4 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( 10,8 ) );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Zero skew color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	gbSizer1->Add( m_staticText3, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_zeroSwatch = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_zeroSwatch, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_staticText15 = new wxStaticText( this, wxID_ANY, _("Negative skew color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	gbSizer1->Add( m_staticText15, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_negativeSwatch = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_negativeSwatch, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Positive skew color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	gbSizer1->Add( m_staticText2, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_positiveSwatch = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_positiveSwatch, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_staticText21 = new wxStaticText( this, wxID_ANY, _("Unknown skew color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	gbSizer1->Add( m_staticText21, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_unknownSwatch = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_unknownSwatch, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_logScale = new wxCheckBox( this, wxID_ANY, _("Logarithmic color scale"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_logScale, 0, wxALL|wxEXPAND, 5 );


	gbSizer1->Add( bSizer3, wxGBPosition( 4, 0 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );


	gbSizer1->AddGrowableCol( 1 );

	singleTrackSizer->Add( gbSizer1, 1, wxEXPAND, 5 );


	bMainSizer->Add( singleTrackSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

DIALOG_DIFF_PHASE_SKEW_PROPERTIES_BASE::~DIALOG_DIFF_PHASE_SKEW_PROPERTIES_BASE()
{
}

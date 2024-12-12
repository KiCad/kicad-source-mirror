///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "drc_re_numeric_input_panel_base.h"

///////////////////////////////////////////////////////////////////////////

DRC_RE_NUMERIC_INPUT_PANEL_BASE::DRC_RE_NUMERIC_INPUT_PANEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bConstraintImageAndValueSizer;
	bConstraintImageAndValueSizer = new wxBoxSizer( wxHORIZONTAL );

	bConstraintImageSizer = new wxBoxSizer( wxVERTICAL );


	bConstraintImageAndValueSizer->Add( bConstraintImageSizer, 1, wxBOTTOM|wxEXPAND|wxLEFT, 5 );

	wxBoxSizer* bConstraintContentSizer;
	bConstraintContentSizer = new wxBoxSizer( wxVERTICAL );


	bConstraintContentSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_numericConstraintLabel = new wxStaticText( this, wxID_ANY, _("Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_numericConstraintLabel->Wrap( -1 );
	fgSizer3->Add( m_numericConstraintLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_numericConstraintCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	fgSizer3->Add( m_numericConstraintCtrl, 0, wxALL|wxEXPAND|wxRIGHT, 5 );

	m_numericConstraintUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_numericConstraintUnit->Wrap( -1 );
	fgSizer3->Add( m_numericConstraintUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bConstraintContentSizer->Add( fgSizer3, 0, wxEXPAND, 5 );


	bConstraintContentSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	bConstraintImageAndValueSizer->Add( bConstraintContentSizer, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	mainSizer->Add( bConstraintImageAndValueSizer, 0, wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

DRC_RE_NUMERIC_INPUT_PANEL_BASE::~DRC_RE_NUMERIC_INPUT_PANEL_BASE()
{
}

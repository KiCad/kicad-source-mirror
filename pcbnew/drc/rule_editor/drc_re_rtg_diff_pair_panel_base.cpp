///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "drc_re_rtg_diff_pair_panel_base.h"

///////////////////////////////////////////////////////////////////////////

DRC_RE_ROUTING_DIFF_PAIR_PANEL_BASE::DRC_RE_ROUTING_DIFF_PAIR_PANEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bConstraintImageAndValueSizer;
	bConstraintImageAndValueSizer = new wxBoxSizer( wxHORIZONTAL );

	bConstraintImageSizer = new wxBoxSizer( wxVERTICAL );


	bConstraintImageAndValueSizer->Add( bConstraintImageSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT, 5 );

	wxBoxSizer* bConstraintContentSizer;
	bConstraintContentSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer31;
	fgSizer31 = new wxFlexGridSizer( 1, 3, 0, 0 );
	fgSizer31->SetFlexibleDirection( wxBOTH );
	fgSizer31->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText13 = new wxStaticText( this, wxID_ANY, _("Max Uncoupled Length"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	fgSizer31->Add( m_staticText13, 0, wxALL, 5 );

	m_maxUncoupledLengthTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_maxUncoupledLengthTextCtrl, 0, wxALL, 5 );

	m_staticText14 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	fgSizer31->Add( m_staticText14, 0, wxALL, 5 );


	bConstraintContentSizer->Add( fgSizer31, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Width") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText1 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Minimum"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer3->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_minWidthTextCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	fgSizer3->Add( m_minWidthTextCtrl, 0, wxALL|wxEXPAND|wxRIGHT, 5 );

	m_staticText2 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer3->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticText3 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Maximum"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer3->Add( m_staticText3, 0, wxALL, 5 );

	m_maxWidthTextCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_maxWidthTextCtrl, 0, wxALL, 5 );

	m_staticText4 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer3->Add( m_staticText4, 0, wxALL, 5 );

	m_staticText5 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Preferred"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer3->Add( m_staticText5, 0, wxALL, 5 );

	m_preferredWidthTextCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_preferredWidthTextCtrl, 0, wxALL, 5 );

	m_staticText6 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	fgSizer3->Add( m_staticText6, 0, wxALL, 5 );


	sbSizer1->Add( fgSizer3, 0, wxEXPAND, 5 );


	bConstraintContentSizer->Add( sbSizer1, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Gap") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText7 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Minimum"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer2->Add( m_staticText7, 0, wxALL, 5 );

	m_minGapTextCtrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_minGapTextCtrl, 0, wxALL, 5 );

	m_staticText8 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizer2->Add( m_staticText8, 0, wxALL, 5 );

	m_staticText9 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Maximum"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer2->Add( m_staticText9, 0, wxALL, 5 );

	m_maxGapTextCtrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_maxGapTextCtrl, 0, wxALL, 5 );

	m_staticText10 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizer2->Add( m_staticText10, 0, wxALL, 5 );

	m_staticText11 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Preferred"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer2->Add( m_staticText11, 0, wxALL, 5 );

	m_preferredGapTextCtrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_preferredGapTextCtrl, 0, wxALL, 5 );

	m_staticText12 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer2->Add( m_staticText12, 0, wxALL, 5 );


	sbSizer2->Add( fgSizer2, 1, wxEXPAND, 5 );


	bConstraintContentSizer->Add( sbSizer2, 1, wxEXPAND, 5 );


	bConstraintImageAndValueSizer->Add( bConstraintContentSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	mainSizer->Add( bConstraintImageAndValueSizer, 0, wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

DRC_RE_ROUTING_DIFF_PAIR_PANEL_BASE::~DRC_RE_ROUTING_DIFF_PAIR_PANEL_BASE()
{
}

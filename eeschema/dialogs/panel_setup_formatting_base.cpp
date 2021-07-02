///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun  3 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_formatting_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( PANEL_SETUP_FORMATTING_BASE, wxPanel )
	EVT_CHECKBOX( wxID_ANY, PANEL_SETUP_FORMATTING_BASE::_wxFB_onCheckBoxIref )
END_EVENT_TABLE()

PANEL_SETUP_FORMATTING_BASE::PANEL_SETUP_FORMATTING_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftColumn;
	bLeftColumn = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Annotations") ), wxVERTICAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText26 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Symbol unit notation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	bSizer6->Add( m_staticText26, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceSeparatorRefIdChoices[] = { _("A"), _(".A"), _("-A"), _("_A"), _(".1"), _("-1"), _("_1") };
	int m_choiceSeparatorRefIdNChoices = sizeof( m_choiceSeparatorRefIdChoices ) / sizeof( wxString );
	m_choiceSeparatorRefId = new wxChoice( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceSeparatorRefIdNChoices, m_choiceSeparatorRefIdChoices, 0 );
	m_choiceSeparatorRefId->SetSelection( 0 );
	bSizer6->Add( m_choiceSeparatorRefId, 1, wxEXPAND|wxRIGHT, 5 );


	sbSizer1->Add( bSizer6, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	bLeftColumn->Add( sbSizer1, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Text") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textSizeLabel = new wxStaticText( sbSizer4->GetStaticBox(), wxID_ANY, _("Default text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	fgSizer2->Add( m_textSizeLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textSizeCtrl = new wxTextCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_textSizeCtrl, 0, wxEXPAND, 5 );

	m_textSizeUnits = new wxStaticText( sbSizer4->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	fgSizer2->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5 );

	m_textOffsetRatioLabel = new wxStaticText( sbSizer4->GetStaticBox(), wxID_ANY, _("Text offset ratio:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOffsetRatioLabel->Wrap( -1 );
	m_textOffsetRatioLabel->SetToolTip( _("Percentage of the text size to offset text above or below wire or bus") );

	fgSizer2->Add( m_textOffsetRatioLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textOffsetRatioCtrl = new wxTextCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_textOffsetRatioCtrl, 0, wxEXPAND, 5 );

	m_offsetRatioUnits = new wxStaticText( sbSizer4->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetRatioUnits->Wrap( -1 );
	fgSizer2->Add( m_offsetRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5 );


	sbSizer4->Add( fgSizer2, 1, wxALL|wxEXPAND, 5 );


	bLeftColumn->Add( sbSizer4, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxStaticBoxSizer* sbSizer41;
	sbSizer41 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Symbols") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer321;
	fgSizer321 = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSizer321->AddGrowableCol( 1 );
	fgSizer321->SetFlexibleDirection( wxBOTH );
	fgSizer321->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lineWidthLabel = new wxStaticText( sbSizer41->GetStaticBox(), wxID_ANY, _("Default line thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	fgSizer321->Add( m_lineWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_lineWidthCtrl = new wxTextCtrl( sbSizer41->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer321->Add( m_lineWidthCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lineWidthUnits = new wxStaticText( sbSizer41->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	fgSizer321->Add( m_lineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE|wxLEFT|wxRIGHT, 5 );

	m_pinSymbolSizeLabel = new wxStaticText( sbSizer41->GetStaticBox(), wxID_ANY, _("Pin symbol size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinSymbolSizeLabel->Wrap( -1 );
	fgSizer321->Add( m_pinSymbolSizeLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_pinSymbolSizeCtrl = new wxTextCtrl( sbSizer41->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer321->Add( m_pinSymbolSizeCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_pinSymbolSizeUnits = new wxStaticText( sbSizer41->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinSymbolSizeUnits->Wrap( -1 );
	fgSizer321->Add( m_pinSymbolSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE|wxLEFT|wxRIGHT, 5 );


	sbSizer41->Add( fgSizer321, 1, wxEXPAND|wxBOTTOM, 5 );


	bLeftColumn->Add( sbSizer41, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Connections") ), wxVERTICAL );

	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText261 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Junction dot size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText261->Wrap( -1 );
	bSizer61->Add( m_staticText261, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceJunctionDotSizeChoices[] = { _("None"), _("Smallest"), _("Small"), _("Default"), _("Large"), _("Largest") };
	int m_choiceJunctionDotSizeNChoices = sizeof( m_choiceJunctionDotSizeChoices ) / sizeof( wxString );
	m_choiceJunctionDotSize = new wxChoice( sbSizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceJunctionDotSizeNChoices, m_choiceJunctionDotSizeChoices, 0 );
	m_choiceJunctionDotSize->SetSelection( 3 );
	bSizer61->Add( m_choiceJunctionDotSize, 1, wxEXPAND|wxRIGHT, 5 );


	sbSizer2->Add( bSizer61, 1, wxBOTTOM|wxEXPAND, 5 );


	bLeftColumn->Add( sbSizer2, 0, wxEXPAND|wxALL, 5 );


	bPanelSizer->Add( bLeftColumn, 1, wxLEFT|wxTOP, 10 );

	wxBoxSizer* bRightColumn;
	bRightColumn = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerIREf;
	sbSizerIREf = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Inter-sheet References") ), wxVERTICAL );

	m_showIntersheetsReferences = new wxCheckBox( sbSizerIREf->GetStaticBox(), wxID_ANY, _("Show inter-sheet references"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerIREf->Add( m_showIntersheetsReferences, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	m_listOwnPage = new wxCheckBox( sbSizerIREf->GetStaticBox(), wxID_ANY, _("Show own page reference"), wxDefaultPosition, wxDefaultSize, 0 );
	m_listOwnPage->SetValue(true);
	bSizerMargins->Add( m_listOwnPage, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_radioFormatStandard = new wxRadioButton( sbSizerIREf->GetStaticBox(), wxID_ANY, _("Standard (1,2,3)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioFormatStandard->SetValue( true );
	bSizerMargins->Add( m_radioFormatStandard, 0, wxALL, 5 );

	m_radioFormatAbbreviated = new wxRadioButton( sbSizerIREf->GetStaticBox(), wxID_ANY, _("Abbreviated (1..3)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMargins->Add( m_radioFormatAbbreviated, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer4->AddGrowableCol( 1 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_prefixLabel = new wxStaticText( sbSizerIREf->GetStaticBox(), wxID_ANY, _("Prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_prefixLabel->Wrap( -1 );
	fgSizer4->Add( m_prefixLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_prefixCtrl = new wxTextCtrl( sbSizerIREf->GetStaticBox(), wxID_ANY, _("["), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer4->Add( m_prefixCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_suffixLabel = new wxStaticText( sbSizerIREf->GetStaticBox(), wxID_ANY, _("Suffix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_suffixLabel->Wrap( -1 );
	fgSizer4->Add( m_suffixLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_suffixCtrl = new wxTextCtrl( sbSizerIREf->GetStaticBox(), wxID_ANY, _("]"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer4->Add( m_suffixCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerMargins->Add( fgSizer4, 1, wxEXPAND, 5 );


	sbSizerIREf->Add( bSizerMargins, 1, wxEXPAND|wxLEFT, 15 );


	bRightColumn->Add( sbSizerIREf, 0, wxALL|wxEXPAND, 5 );


	bRightColumn->Add( 0, 0, 0, wxEXPAND, 5 );


	bPanelSizer->Add( bRightColumn, 1, wxLEFT|wxTOP, 10 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_SETUP_FORMATTING_BASE::~PANEL_SETUP_FORMATTING_BASE()
{
}

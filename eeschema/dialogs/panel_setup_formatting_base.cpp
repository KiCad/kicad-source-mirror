///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
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

	m_staticText26 = new wxStaticText( this, wxID_ANY, _("Text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	bLeftColumn->Add( m_staticText26, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftColumn->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Default text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	fgSizer2->Add( m_textSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_textSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_textSizeCtrl, 0, wxEXPAND, 5 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	fgSizer2->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5 );

	m_overbarHieghtLabel = new wxStaticText( this, wxID_ANY, _("Overbar offset ratio:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_overbarHieghtLabel->Wrap( -1 );
	fgSizer2->Add( m_overbarHieghtLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_overbarHeightCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_overbarHeightCtrl, 0, wxEXPAND, 5 );

	m_overbarHeightUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_overbarHeightUnits->Wrap( -1 );
	fgSizer2->Add( m_overbarHeightUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textOffsetRatioLabel = new wxStaticText( this, wxID_ANY, _("Label offset ratio:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOffsetRatioLabel->Wrap( -1 );
	m_textOffsetRatioLabel->SetToolTip( _("Percentage of the text size to offset labels above (or below) a wire, bus, or pin") );

	fgSizer2->Add( m_textOffsetRatioLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_textOffsetRatioCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textOffsetRatioCtrl->SetToolTip( _("Percentage of the text size to offset labels above (or below) a wire, bus, or pin") );

	fgSizer2->Add( m_textOffsetRatioCtrl, 0, wxEXPAND, 5 );

	m_offsetRatioUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetRatioUnits->Wrap( -1 );
	fgSizer2->Add( m_offsetRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5 );

	m_labelSizeRatioLabel = new wxStaticText( this, wxID_ANY, _("Global label margin ratio:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelSizeRatioLabel->Wrap( -1 );
	m_labelSizeRatioLabel->SetToolTip( _("Percentage of the text size to use as space around a global label") );

	fgSizer2->Add( m_labelSizeRatioLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_labelSizeRatioCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_labelSizeRatioCtrl->SetToolTip( _("Percentage of the text size to use as space around a global label") );

	fgSizer2->Add( m_labelSizeRatioCtrl, 0, wxEXPAND, 5 );

	m_labelSizeRatioUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelSizeRatioUnits->Wrap( -1 );
	fgSizer2->Add( m_labelSizeRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5 );


	bLeftColumn->Add( fgSizer2, 0, wxEXPAND|wxALL, 5 );


	bLeftColumn->Add( 0, 5, 0, wxEXPAND, 5 );

	m_staticText27 = new wxStaticText( this, wxID_ANY, _("Symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText27->Wrap( -1 );
	bLeftColumn->Add( m_staticText27, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftColumn->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer321;
	fgSizer321 = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSizer321->AddGrowableCol( 1 );
	fgSizer321->SetFlexibleDirection( wxBOTH );
	fgSizer321->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lineWidthLabel = new wxStaticText( this, wxID_ANY, _("Default line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	fgSizer321->Add( m_lineWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_lineWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer321->Add( m_lineWidthCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lineWidthUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	fgSizer321->Add( m_lineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE|wxLEFT|wxRIGHT, 5 );

	m_pinSymbolSizeLabel = new wxStaticText( this, wxID_ANY, _("Pin symbol size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinSymbolSizeLabel->Wrap( -1 );
	fgSizer321->Add( m_pinSymbolSizeLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_pinSymbolSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer321->Add( m_pinSymbolSizeCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_pinSymbolSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinSymbolSizeUnits->Wrap( -1 );
	fgSizer321->Add( m_pinSymbolSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE|wxLEFT|wxRIGHT, 5 );


	bLeftColumn->Add( fgSizer321, 0, wxEXPAND|wxALL, 5 );

	m_showDNPMarkers = new wxCheckBox( this, wxID_ANY, _("Show 'Do not populate' markers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showDNPMarkers->SetValue(true);
	m_showDNPMarkers->SetToolTip( _("Draw a cross over symbols marked 'Do not populate'") );

	bLeftColumn->Add( m_showDNPMarkers, 0, wxBOTTOM|wxRIGHT|wxLEFT, 10 );


	bLeftColumn->Add( 0, 5, 0, wxEXPAND, 5 );

	m_staticText28 = new wxStaticText( this, wxID_ANY, _("Connections"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	bLeftColumn->Add( m_staticText28, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftColumn->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 5, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_junctionDotLabel = new wxStaticText( this, wxID_ANY, _("Junction dot size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_junctionDotLabel->Wrap( -1 );
	gbSizer1->Add( m_junctionDotLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_choiceJunctionDotSizeChoices[] = { _("None"), _("Smallest"), _("Small"), _("Default"), _("Large"), _("Largest") };
	int m_choiceJunctionDotSizeNChoices = sizeof( m_choiceJunctionDotSizeChoices ) / sizeof( wxString );
	m_choiceJunctionDotSize = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceJunctionDotSizeNChoices, m_choiceJunctionDotSizeChoices, 0 );
	m_choiceJunctionDotSize->SetSelection( 3 );
	gbSizer1->Add( m_choiceJunctionDotSize, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_hopOverLabel = new wxStaticText( this, wxID_ANY, _("Hop-over size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hopOverLabel->Wrap( -1 );
	gbSizer1->Add( m_hopOverLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_choiceHopOverSizeChoices[] = { _("None"), _("Smallest"), _("Small"), _("Medium"), _("Large"), _("Largest") };
	int m_choiceHopOverSizeNChoices = sizeof( m_choiceHopOverSizeChoices ) / sizeof( wxString );
	m_choiceHopOverSize = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHopOverSizeNChoices, m_choiceHopOverSizeChoices, 0 );
	m_choiceHopOverSize->SetSelection( 0 );
	gbSizer1->Add( m_choiceHopOverSize, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_connectionGridLabel = new wxStaticText( this, wxID_ANY, _("Connection grid:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_connectionGridLabel->Wrap( -1 );
	gbSizer1->Add( m_connectionGridLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_connectionGridCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_connectionGridCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_connectionGridUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_connectionGridUnits->Wrap( -1 );
	gbSizer1->Add( m_connectionGridUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	gbSizer1->AddGrowableCol( 1 );

	bLeftColumn->Add( gbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bPanelSizer->Add( bLeftColumn, 1, wxEXPAND|wxRIGHT, 15 );

	wxBoxSizer* bRightColumn;
	bRightColumn = new wxBoxSizer( wxVERTICAL );

	m_staticText29 = new wxStaticText( this, wxID_ANY, _("Inter-sheet References"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText29->Wrap( -1 );
	bRightColumn->Add( m_staticText29, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline4 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightColumn->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_showIntersheetsReferences = new wxCheckBox( this, wxID_ANY, _("Show inter-sheet references"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_showIntersheetsReferences, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRightColumn->Add( bSizer5, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	m_listOwnPage = new wxCheckBox( this, wxID_ANY, _("Show own page reference"), wxDefaultPosition, wxDefaultSize, 0 );
	m_listOwnPage->SetValue(true);
	bSizerMargins->Add( m_listOwnPage, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_radioFormatStandard = new wxRadioButton( this, wxID_ANY, _("Standard (1,2,3)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioFormatStandard->SetValue( true );
	bSizerMargins->Add( m_radioFormatStandard, 0, wxALL, 5 );

	m_radioFormatAbbreviated = new wxRadioButton( this, wxID_ANY, _("Abbreviated (1..3)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMargins->Add( m_radioFormatAbbreviated, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer4->AddGrowableCol( 1 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_prefixLabel = new wxStaticText( this, wxID_ANY, _("Prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_prefixLabel->Wrap( -1 );
	fgSizer4->Add( m_prefixLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_prefixCtrl = new wxTextCtrl( this, wxID_ANY, _("["), wxDefaultPosition, wxDefaultSize, 0 );
	m_prefixCtrl->SetMinSize( wxSize( 160,-1 ) );

	fgSizer4->Add( m_prefixCtrl, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_suffixLabel = new wxStaticText( this, wxID_ANY, _("Suffix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_suffixLabel->Wrap( -1 );
	fgSizer4->Add( m_suffixLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_suffixCtrl = new wxTextCtrl( this, wxID_ANY, _("]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_suffixCtrl->SetMinSize( wxSize( 160,-1 ) );

	fgSizer4->Add( m_suffixCtrl, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerMargins->Add( fgSizer4, 1, wxEXPAND, 5 );


	bRightColumn->Add( bSizerMargins, 1, wxEXPAND|wxLEFT, 25 );

	m_staticText30 = new wxStaticText( this, wxID_ANY, _("Dashed Lines"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText30->Wrap( -1 );
	bRightColumn->Add( m_staticText30, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline5 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightColumn->Add( m_staticline5, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer41;
	fgSizer41 = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizer41->SetFlexibleDirection( wxBOTH );
	fgSizer41->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	dashLengthLabel = new wxStaticText( this, wxID_ANY, _("Dash length:"), wxDefaultPosition, wxDefaultSize, 0 );
	dashLengthLabel->Wrap( -1 );
	fgSizer41->Add( dashLengthLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_dashLengthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer41->Add( m_dashLengthCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	gapLengthLabel = new wxStaticText( this, wxID_ANY, _("Gap length:"), wxDefaultPosition, wxDefaultSize, 0 );
	gapLengthLabel->Wrap( -1 );
	fgSizer41->Add( gapLengthLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_gapLengthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer41->Add( m_gapLengthCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bRightColumn->Add( fgSizer41, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_dashedLineHelp = new wxStaticText( this, wxID_ANY, _("Dash and dot lengths are ratios of the line width."), wxDefaultPosition, wxDefaultSize, 0 );
	m_dashedLineHelp->Wrap( -1 );
	bSizer6->Add( m_dashedLineHelp, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRightColumn->Add( bSizer6, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRightColumn->Add( 0, 5, 0, wxEXPAND, 5 );

	m_staticText31 = new wxStaticText( this, wxID_ANY, _("Operating-point Overlay"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	bRightColumn->Add( m_staticText31, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline6 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightColumn->Add( m_staticline6, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 4, 2, 6, 0 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* vPrecisionLabel;
	vPrecisionLabel = new wxStaticText( this, wxID_ANY, _("Significant digits (voltages):"), wxDefaultPosition, wxDefaultSize, 0 );
	vPrecisionLabel->Wrap( -1 );
	fgSizer->Add( vPrecisionLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_vPrecisionCtrl = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 3 );
	fgSizer->Add( m_vPrecisionCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticText* vRangeLabel;
	vRangeLabel = new wxStaticText( this, wxID_ANY, _("Range (voltages):"), wxDefaultPosition, wxDefaultSize, 0 );
	vRangeLabel->Wrap( -1 );
	fgSizer->Add( vRangeLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_vRangeCtrlChoices[] = { _("Auto"), _("fV"), _("pV"), _("nV"), _("uV"), _("mV"), _("V"), _("KV"), _("MV"), _("GV"), _("TV"), _("PV") };
	int m_vRangeCtrlNChoices = sizeof( m_vRangeCtrlChoices ) / sizeof( wxString );
	m_vRangeCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_vRangeCtrlNChoices, m_vRangeCtrlChoices, 0 );
	m_vRangeCtrl->SetSelection( 0 );
	fgSizer->Add( m_vRangeCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticText* iPrecisionLabel;
	iPrecisionLabel = new wxStaticText( this, wxID_ANY, _("Significant digits (currents):"), wxDefaultPosition, wxDefaultSize, 0 );
	iPrecisionLabel->Wrap( -1 );
	fgSizer->Add( iPrecisionLabel, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_iPrecisionCtrl = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 3 );
	fgSizer->Add( m_iPrecisionCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticText* iRangeLabel;
	iRangeLabel = new wxStaticText( this, wxID_ANY, _("Range (currents):"), wxDefaultPosition, wxDefaultSize, 0 );
	iRangeLabel->Wrap( -1 );
	fgSizer->Add( iRangeLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_iRangeCtrlChoices[] = { _("Auto"), _("fA"), _("pA"), _("nA"), _("uA"), _("mA"), _("A"), _("KA"), _("MA"), _("GA"), _("TA"), _("PA") };
	int m_iRangeCtrlNChoices = sizeof( m_iRangeCtrlChoices ) / sizeof( wxString );
	m_iRangeCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_iRangeCtrlNChoices, m_iRangeCtrlChoices, 0 );
	m_iRangeCtrl->SetSelection( 0 );
	fgSizer->Add( m_iRangeCtrl, 0, wxBOTTOM|wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bRightColumn->Add( fgSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bPanelSizer->Add( bRightColumn, 1, wxEXPAND|wxLEFT, 10 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_SETUP_FORMATTING_BASE::~PANEL_SETUP_FORMATTING_BASE()
{
}

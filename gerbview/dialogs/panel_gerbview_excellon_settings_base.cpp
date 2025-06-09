///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_gerbview_excellon_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE::PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bDialogSizer;
	bDialogSizer = new wxBoxSizer( wxVERTICAL );

	m_fileFormatLabel = new wxStaticText( this, wxID_ANY, _("File Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fileFormatLabel->Wrap( -1 );
	bDialogSizer->Add( m_fileFormatLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bDialogSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_fileFormatHelp = new wxStaticText( this, wxID_ANY, _("These parameters are usually specified in files, but not always."), wxDefaultPosition, wxDefaultSize, 0 );
	m_fileFormatHelp->Wrap( -1 );
	bSizer5->Add( m_fileFormatHelp, 0, wxALL, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 4, 8 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,10 ) );

	unitsLabel = new wxStaticText( this, wxID_ANY, _("File units:"), wxDefaultPosition, wxDefaultSize, 0 );
	unitsLabel->Wrap( -1 );
	gbSizer1->Add( unitsLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), 0, 5 );

	m_rbInches = new wxRadioButton( this, wxID_ANY, _("Inches"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	gbSizer1->Add( m_rbInches, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), 0, 5 );

	m_rbMM = new wxRadioButton( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbMM, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), 0, 5 );

	zeroFormatLabel = new wxStaticText( this, wxID_ANY, _("Zero format:"), wxDefaultPosition, wxDefaultSize, 0 );
	zeroFormatLabel->Wrap( -1 );
	gbSizer1->Add( zeroFormatLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), 0, 5 );

	m_rbTZ = new wxRadioButton( this, wxID_ANY, _("No leading zeros (TZ format)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	gbSizer1->Add( m_rbTZ, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), 0, 5 );

	m_rbLZ = new wxRadioButton( this, wxID_ANY, _("No trailing zeros (LZ format)"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbLZ, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), 0, 5 );


	bSizer5->Add( gbSizer1, 1, wxEXPAND|wxALL, 5 );


	bDialogSizer->Add( bSizer5, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );


	bDialogSizer->Add( 0, 10, 0, wxEXPAND, 5 );

	m_coordinatesLabel = new wxStaticText( this, wxID_ANY, _("Coordinates Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_coordinatesLabel->Wrap( -1 );
	bDialogSizer->Add( m_coordinatesLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bDialogSizer->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer51;
	bSizer51 = new wxBoxSizer( wxVERTICAL );

	m_coordsFormatHelp = new wxStaticText( this, wxID_ANY, _("The coordinates format is not specified in Excellon format."), wxDefaultPosition, wxDefaultSize, 0 );
	m_coordsFormatHelp->Wrap( -1 );
	bSizer51->Add( m_coordsFormatHelp, 0, wxALL, 5 );

	m_hint1 = new wxStaticText( this, wxID_ANY, _("(The decimal format does not use these settings)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hint1->Wrap( -1 );
	bSizer51->Add( m_hint1, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizerFmt;
	fgSizerFmt = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerFmt->SetFlexibleDirection( wxBOTH );
	fgSizerFmt->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextUnitsmm = new wxStaticText( this, wxID_ANY, _("Format for mm:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnitsmm->Wrap( -1 );
	fgSizerFmt->Add( m_staticTextUnitsmm, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	wxString m_choiceIntegerMMChoices[] = { _("2"), _("3"), _("4"), _("5"), _("6") };
	int m_choiceIntegerMMNChoices = sizeof( m_choiceIntegerMMChoices ) / sizeof( wxString );
	m_choiceIntegerMM = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceIntegerMMNChoices, m_choiceIntegerMMChoices, 0 );
	m_choiceIntegerMM->SetSelection( 1 );
	bSizer4->Add( m_choiceIntegerMM, 0, wxALL, 5 );

	m_staticText8 = new wxStaticText( this, wxID_ANY, _(":"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	bSizer4->Add( m_staticText8, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	wxString m_choiceMantissaMMChoices[] = { _("2"), _("3"), _("4"), _("5"), _("6") };
	int m_choiceMantissaMMNChoices = sizeof( m_choiceMantissaMMChoices ) / sizeof( wxString );
	m_choiceMantissaMM = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceMantissaMMNChoices, m_choiceMantissaMMChoices, 0 );
	m_choiceMantissaMM->SetSelection( 1 );
	bSizer4->Add( m_choiceMantissaMM, 0, wxALL, 5 );


	fgSizerFmt->Add( bSizer4, 0, wxEXPAND, 5 );

	m_staticTextUnitsInch = new wxStaticText( this, wxID_ANY, _("Format for inches:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnitsInch->Wrap( -1 );
	fgSizerFmt->Add( m_staticTextUnitsInch, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	wxString m_choiceIntegerInchChoices[] = { _("2"), _("3"), _("4"), _("5"), _("6") };
	int m_choiceIntegerInchNChoices = sizeof( m_choiceIntegerInchChoices ) / sizeof( wxString );
	m_choiceIntegerInch = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceIntegerInchNChoices, m_choiceIntegerInchChoices, 0 );
	m_choiceIntegerInch->SetSelection( 0 );
	bSizer3->Add( m_choiceIntegerInch, 0, wxALL, 5 );

	m_staticText9 = new wxStaticText( this, wxID_ANY, _(":"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	bSizer3->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	wxString m_choiceMantissaInchChoices[] = { _("2"), _("3"), _("4"), _("5"), _("6") };
	int m_choiceMantissaInchNChoices = sizeof( m_choiceMantissaInchChoices ) / sizeof( wxString );
	m_choiceMantissaInch = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceMantissaInchNChoices, m_choiceMantissaInchChoices, 0 );
	m_choiceMantissaInch->SetSelection( 2 );
	bSizer3->Add( m_choiceMantissaInch, 0, wxALL, 5 );


	fgSizerFmt->Add( bSizer3, 0, wxEXPAND, 5 );


	bSizer51->Add( fgSizerFmt, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_hint2 = new wxStaticText( this, wxID_ANY, _("Usually: 3:3 in mm and 2:4 in inches"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hint2->Wrap( -1 );
	bSizer51->Add( m_hint2, 0, wxALL, 5 );


	bDialogSizer->Add( bSizer51, 1, wxEXPAND|wxTOP|wxLEFT, 5 );


	this->SetSizer( bDialogSizer );
	this->Layout();
	bDialogSizer->Fit( this );

	// Connect Events
	m_choiceIntegerMM->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE::onUnitsChange ), NULL, this );
	m_choiceIntegerInch->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE::onUnitsChange ), NULL, this );
}

PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE::~PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE()
{
	// Disconnect Events
	m_choiceIntegerMM->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE::onUnitsChange ), NULL, this );
	m_choiceIntegerInch->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE::onUnitsChange ), NULL, this );

}

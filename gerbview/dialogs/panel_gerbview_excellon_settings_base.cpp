///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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

	wxStaticBoxSizer* sbSizerFileFormat;
	sbSizerFileFormat = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("File Format") ), wxVERTICAL );

	m_staticText11 = new wxStaticText( sbSizerFileFormat->GetStaticBox(), wxID_ANY, _("These parameters are usually specified in files, but not always."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	sbSizerFileFormat->Add( m_staticText11, 0, wxALL, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	wxString m_rbUnitsChoices[] = { _("Inches"), _("mm") };
	int m_rbUnitsNChoices = sizeof( m_rbUnitsChoices ) / sizeof( wxString );
	m_rbUnits = new wxRadioBox( sbSizerFileFormat->GetStaticBox(), wxID_ANY, _("File units"), wxDefaultPosition, wxDefaultSize, m_rbUnitsNChoices, m_rbUnitsChoices, 1, wxRA_SPECIFY_COLS );
	m_rbUnits->SetSelection( 0 );
	bSizer8->Add( m_rbUnits, 1, wxALL, 5 );

	wxString m_rbZeroFormatChoices[] = { _("No leading zeros (TZ format)"), _("No trailing zeros (LZ format)") };
	int m_rbZeroFormatNChoices = sizeof( m_rbZeroFormatChoices ) / sizeof( wxString );
	m_rbZeroFormat = new wxRadioBox( sbSizerFileFormat->GetStaticBox(), wxID_ANY, _("Zero format"), wxDefaultPosition, wxDefaultSize, m_rbZeroFormatNChoices, m_rbZeroFormatChoices, 1, wxRA_SPECIFY_COLS );
	m_rbZeroFormat->SetSelection( 0 );
	m_rbZeroFormat->SetToolTip( _("Integers in files can have their zeros stripped.\nNo leading zeros format means the leading zeros are stripped\nNo trailing zeros  format means the trainling zeros are stripped") );

	bSizer8->Add( m_rbZeroFormat, 1, wxALL, 5 );


	sbSizerFileFormat->Add( bSizer8, 1, wxEXPAND, 5 );


	bDialogSizer->Add( sbSizerFileFormat, 1, wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bDialogSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxStaticBoxSizer* sbSizerCoordinates;
	sbSizerCoordinates = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Coordinates Format") ), wxVERTICAL );

	m_staticText6 = new wxStaticText( sbSizerCoordinates->GetStaticBox(), wxID_ANY, _("The coordinates format is not specified in Excellon format."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	sbSizerCoordinates->Add( m_staticText6, 0, wxALL, 5 );

	m_staticText10 = new wxStaticText( sbSizerCoordinates->GetStaticBox(), wxID_ANY, _("(The decimal format does not use these settings)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	sbSizerCoordinates->Add( m_staticText10, 0, wxALL, 5 );

	m_staticText7 = new wxStaticText( sbSizerCoordinates->GetStaticBox(), wxID_ANY, _("Usually: 3:3 in mm and 2:4 in inches"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	sbSizerCoordinates->Add( m_staticText7, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizerFmt;
	fgSizerFmt = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizerFmt->SetFlexibleDirection( wxBOTH );
	fgSizerFmt->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextUnitsmm = new wxStaticText( sbSizerCoordinates->GetStaticBox(), wxID_ANY, _("Format for mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnitsmm->Wrap( -1 );
	fgSizerFmt->Add( m_staticTextUnitsmm, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceIntegerMMChoices[] = { _("2"), _("3"), _("4"), _("5"), _("6") };
	int m_choiceIntegerMMNChoices = sizeof( m_choiceIntegerMMChoices ) / sizeof( wxString );
	m_choiceIntegerMM = new wxChoice( sbSizerCoordinates->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceIntegerMMNChoices, m_choiceIntegerMMChoices, 0 );
	m_choiceIntegerMM->SetSelection( 1 );
	fgSizerFmt->Add( m_choiceIntegerMM, 0, wxALL, 5 );

	m_staticText8 = new wxStaticText( sbSizerCoordinates->GetStaticBox(), wxID_ANY, _(":"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizerFmt->Add( m_staticText8, 0, wxALL, 5 );

	wxString m_choiceMantissaMMChoices[] = { _("2"), _("3"), _("4"), _("5"), _("6") };
	int m_choiceMantissaMMNChoices = sizeof( m_choiceMantissaMMChoices ) / sizeof( wxString );
	m_choiceMantissaMM = new wxChoice( sbSizerCoordinates->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceMantissaMMNChoices, m_choiceMantissaMMChoices, 0 );
	m_choiceMantissaMM->SetSelection( 1 );
	fgSizerFmt->Add( m_choiceMantissaMM, 0, wxALL, 5 );

	m_staticTextUnitsInch = new wxStaticText( sbSizerCoordinates->GetStaticBox(), wxID_ANY, _("Format for inches"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnitsInch->Wrap( -1 );
	fgSizerFmt->Add( m_staticTextUnitsInch, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceIntegerInchChoices[] = { _("2"), _("3"), _("4"), _("5"), _("6") };
	int m_choiceIntegerInchNChoices = sizeof( m_choiceIntegerInchChoices ) / sizeof( wxString );
	m_choiceIntegerInch = new wxChoice( sbSizerCoordinates->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceIntegerInchNChoices, m_choiceIntegerInchChoices, 0 );
	m_choiceIntegerInch->SetSelection( 0 );
	fgSizerFmt->Add( m_choiceIntegerInch, 0, wxALL, 5 );

	m_staticText9 = new wxStaticText( sbSizerCoordinates->GetStaticBox(), wxID_ANY, _(":"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizerFmt->Add( m_staticText9, 0, wxALL, 5 );

	wxString m_choiceMantissaInchChoices[] = { _("2"), _("3"), _("4"), _("5"), _("6") };
	int m_choiceMantissaInchNChoices = sizeof( m_choiceMantissaInchChoices ) / sizeof( wxString );
	m_choiceMantissaInch = new wxChoice( sbSizerCoordinates->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceMantissaInchNChoices, m_choiceMantissaInchChoices, 0 );
	m_choiceMantissaInch->SetSelection( 2 );
	fgSizerFmt->Add( m_choiceMantissaInch, 0, wxALL, 5 );


	sbSizerCoordinates->Add( fgSizerFmt, 1, wxEXPAND, 5 );


	bDialogSizer->Add( sbSizerCoordinates, 1, wxEXPAND, 5 );


	this->SetSizer( bDialogSizer );
	this->Layout();

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

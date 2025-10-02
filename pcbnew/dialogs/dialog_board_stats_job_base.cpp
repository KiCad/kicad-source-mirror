///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_board_stats_job_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BOARD_STATS_JOB_BASE::DIALOG_BOARD_STATS_JOB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textOutputPath = new wxStaticText( this, wxID_ANY, _("Output file:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizer1->Add( m_textOutputPath, 0, wxALIGN_CENTER, 5 );

	m_textCtrlOutputPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputPath->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlOutputPath, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticText18 = new wxStaticText( this, wxID_ANY, _("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer1->Add( m_staticText18, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceFormatChoices;
	m_choiceFormat = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFormatChoices, 0 );
	m_choiceFormat->SetSelection( 0 );
	fgSizer1->Add( m_choiceFormat, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_lblUnits = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblUnits->Wrap( -1 );
	fgSizer1->Add( m_lblUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceUnitsChoices[] = { _("Millimeters"), _("Inches") };
	int m_choiceUnitsNChoices = sizeof( m_choiceUnitsChoices ) / sizeof( wxString );
	m_choiceUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitsNChoices, m_choiceUnitsChoices, 0 );
	m_choiceUnits->SetSelection( 0 );
	fgSizer1->Add( m_choiceUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( fgSizer1, 1, wxALL|wxEXPAND, 10 );

	wxGridSizer* gOptionsSizer;
	gOptionsSizer = new wxGridSizer( 0, 1, 10, 0 );

	m_checkBoxSubtractHoles = new wxCheckBox( this, wxID_ANY, _("Subtract holes from board area"), wxDefaultPosition, wxDefaultSize, 0 );
	gOptionsSizer->Add( m_checkBoxSubtractHoles, 0, 0, 5 );

	m_checkBoxSubtractHolesFromCopper = new wxCheckBox( this, wxID_ANY, _("Subtract holes from copper areas"), wxDefaultPosition, wxDefaultSize, 0 );
	gOptionsSizer->Add( m_checkBoxSubtractHolesFromCopper, 0, 0, 5 );

	m_checkBoxExcludeFootprintsWithoutPads = new wxCheckBox( this, wxID_ANY, _("Exclude footprints with no pads"), wxDefaultPosition, wxDefaultSize, 0 );
	gOptionsSizer->Add( m_checkBoxExcludeFootprintsWithoutPads, 0, 0, 5 );


	bSizerMain->Add( gOptionsSizer, 1, wxBOTTOM|wxEXPAND|wxLEFT, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxALL|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_choiceFormat->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_BOARD_STATS_JOB_BASE::OnFormatChoice ), NULL, this );
	m_checkBoxSubtractHoles->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATS_JOB_BASE::checkboxClicked ), NULL, this );
	m_checkBoxSubtractHolesFromCopper->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATS_JOB_BASE::checkboxClicked ), NULL, this );
	m_checkBoxExcludeFootprintsWithoutPads->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATS_JOB_BASE::checkboxClicked ), NULL, this );
}

DIALOG_BOARD_STATS_JOB_BASE::~DIALOG_BOARD_STATS_JOB_BASE()
{
	// Disconnect Events
	m_choiceFormat->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_BOARD_STATS_JOB_BASE::OnFormatChoice ), NULL, this );
	m_checkBoxSubtractHoles->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATS_JOB_BASE::checkboxClicked ), NULL, this );
	m_checkBoxSubtractHolesFromCopper->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATS_JOB_BASE::checkboxClicked ), NULL, this );
	m_checkBoxExcludeFootprintsWithoutPads->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATS_JOB_BASE::checkboxClicked ), NULL, this );

}

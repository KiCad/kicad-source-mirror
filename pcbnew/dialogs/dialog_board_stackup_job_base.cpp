///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_board_stackup_job_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BOARD_STACKUP_JOB_BASE::DIALOG_BOARD_STACKUP_JOB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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

	m_labelFormat = new wxStaticText( this, wxID_ANY, _("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelFormat->Wrap( -1 );
	fgSizer1->Add( m_labelFormat, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceFormatChoices;
	m_choiceFormat = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFormatChoices, 0 );
	m_choiceFormat->SetSelection( 0 );
	fgSizer1->Add( m_choiceFormat, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_labelUnits = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelUnits->Wrap( -1 );
	fgSizer1->Add( m_labelUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceUnitsChoices[] = { _("Millimeters"), _("Inches") };
	int m_choiceUnitsNChoices = sizeof( m_choiceUnitsChoices ) / sizeof( wxString );
	m_choiceUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitsNChoices, m_choiceUnitsChoices, 0 );
	m_choiceUnits->SetSelection( 0 );
	fgSizer1->Add( m_choiceUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( fgSizer1, 0, wxEXPAND|wxALL, 10 );

	m_sbSizerFields = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Include Fields") ), wxVERTICAL );

	m_cbThickness = new wxCheckBox( m_sbSizerFields->GetStaticBox(), wxID_ANY, _("Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sbSizerFields->Add( m_cbThickness, 0, wxBOTTOM, 3 );

	m_cbMaterial = new wxCheckBox( m_sbSizerFields->GetStaticBox(), wxID_ANY, _("Material"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sbSizerFields->Add( m_cbMaterial, 0, wxBOTTOM, 3 );

	m_cbColor = new wxCheckBox( m_sbSizerFields->GetStaticBox(), wxID_ANY, _("Color"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sbSizerFields->Add( m_cbColor, 0, wxBOTTOM, 3 );

	m_cbEpsilonR = new wxCheckBox( m_sbSizerFields->GetStaticBox(), wxID_ANY, _("Epsilon R"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sbSizerFields->Add( m_cbEpsilonR, 0, wxBOTTOM, 3 );

	m_cbLossTangent = new wxCheckBox( m_sbSizerFields->GetStaticBox(), wxID_ANY, _("Loss tangent"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sbSizerFields->Add( m_cbLossTangent, 0, wxBOTTOM, 3 );

	m_cbFinish = new wxCheckBox( m_sbSizerFields->GetStaticBox(), wxID_ANY, _("Board finish"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sbSizerFields->Add( m_cbFinish, 0, wxBOTTOM, 3 );

	m_cbBoardOptions = new wxCheckBox( m_sbSizerFields->GetStaticBox(), wxID_ANY, _("Board options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sbSizerFields->Add( m_cbBoardOptions, 0, wxBOTTOM, 3 );


	bSizerMain->Add( m_sbSizerFields, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxALL|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_choiceFormat->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_BOARD_STACKUP_JOB_BASE::OnFormatChoice ), NULL, this );
}

DIALOG_BOARD_STACKUP_JOB_BASE::~DIALOG_BOARD_STACKUP_JOB_BASE()
{
	// Disconnect Events
	m_choiceFormat->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_BOARD_STACKUP_JOB_BASE::OnFormatChoice ), NULL, this );

}

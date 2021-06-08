///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Apr 22 2021)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"

#include "dialog_export_step_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_STEP_BASE::DIALOG_EXPORT_STEP_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerSTEPFile;
	bSizerSTEPFile = new wxBoxSizer( wxVERTICAL );

	m_txtBrdFile = new wxStaticText( this, wxID_ANY, _("File name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtBrdFile->Wrap( -1 );
	bSizerSTEPFile->Add( m_txtBrdFile, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_filePickerSTEP = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, _("Select a STEP export filename"), _("STEP files (*.stp;*.step)|*.stp;*.step"), wxDefaultPosition, wxSize( -1,-1 ), wxFLP_SAVE|wxFLP_USE_TEXTCTRL );
	bSizerSTEPFile->Add( m_filePickerSTEP, 0, wxEXPAND|wxALL, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerSTEPFile->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	m_staticText6 = new wxStaticText( this, wxID_ANY, _("Coordinate origin options:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_staticText6->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer7->Add( m_staticText6, 0, wxALL, 5 );

	m_rbDrillAndPlotOrigin = new wxRadioButton( this, wxID_ANY, _("Drill and plot origin"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer7->Add( m_rbDrillAndPlotOrigin, 0, wxALL, 5 );

	m_rbGridOrigin = new wxRadioButton( this, wxID_ANY, _("Grid origin"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_rbGridOrigin, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_rbUserDefinedOrigin = new wxRadioButton( this, wxID_ANY, _("User defined origin"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_rbUserDefinedOrigin, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_rbBoardCenterOrigin = new wxRadioButton( this, wxID_ANY, _("Board center origin"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_rbBoardCenterOrigin, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizer2->Add( bSizer7, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("User defined origin:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_staticText2->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer3->Add( m_staticText2, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextUnits = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnits->Wrap( -1 );
	fgSizer1->Add( m_staticTextUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_STEP_OrgUnitChoiceChoices[] = { _("mm"), _("inch") };
	int m_STEP_OrgUnitChoiceNChoices = sizeof( m_STEP_OrgUnitChoiceChoices ) / sizeof( wxString );
	m_STEP_OrgUnitChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_STEP_OrgUnitChoiceNChoices, m_STEP_OrgUnitChoiceChoices, 0 );
	m_STEP_OrgUnitChoice->SetSelection( 0 );
	fgSizer1->Add( m_STEP_OrgUnitChoice, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextXpos = new wxStaticText( this, wxID_ANY, _("X position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextXpos->Wrap( -1 );
	fgSizer1->Add( m_staticTextXpos, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_STEP_Xorg = new TEXT_CTRL_EVAL( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_STEP_Xorg->HasFlag( wxTE_MULTILINE ) )
	{
	m_STEP_Xorg->SetMaxLength( 8 );
	}
	#else
	m_STEP_Xorg->SetMaxLength( 8 );
	#endif
	fgSizer1->Add( m_STEP_Xorg, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextYpos = new wxStaticText( this, wxID_ANY, _("Y position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextYpos->Wrap( -1 );
	fgSizer1->Add( m_staticTextYpos, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_STEP_Yorg = new TEXT_CTRL_EVAL( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_STEP_Yorg->HasFlag( wxTE_MULTILINE ) )
	{
	m_STEP_Yorg->SetMaxLength( 8 );
	}
	#else
	m_STEP_Yorg->SetMaxLength( 8 );
	#endif
	fgSizer1->Add( m_STEP_Yorg, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	bSizer3->Add( fgSizer1, 1, wxEXPAND, 5 );


	bSizer2->Add( bSizer3, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Other options:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	m_staticText7->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer8->Add( m_staticText7, 0, wxALL, 5 );

	m_cbRemoveVirtual = new wxCheckBox( this, wxID_ANY, _("Ignore virtual components"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_cbRemoveVirtual, 0, wxALL, 5 );

	m_cbSubstModels = new wxCheckBox( this, wxID_ANY, _("Substitute similarly named models"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbSubstModels->SetToolTip( _("Replace VRML models with STEP models of the same name") );

	bSizer8->Add( m_cbSubstModels, 0, wxALL, 5 );

	m_cbOverwriteFile = new wxCheckBox( this, wxID_ANY, _("Overwrite old file"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_cbOverwriteFile, 0, wxALL, 5 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer8->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );

	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Board outline builder:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	bSizer8->Add( m_staticText9, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerTolerance;
	bSizerTolerance = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextTolerance = new wxStaticText( this, wxID_ANY, _("Tolerance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTolerance->Wrap( -1 );
	bSizerTolerance->Add( m_staticTextTolerance, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_toleranceChoices[] = { _("Tight (0.001 mm)"), _("Standard (0.01 mm)"), _("Loose (0.1 mm)") };
	int m_toleranceNChoices = sizeof( m_toleranceChoices ) / sizeof( wxString );
	m_tolerance = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_toleranceNChoices, m_toleranceChoices, 0 );
	m_tolerance->SetSelection( 1 );
	m_tolerance->SetToolTip( _("Tolerance sets the distance between two points that are considered joined.") );

	bSizerTolerance->Add( m_tolerance, 1, wxALL, 5 );


	bSizer8->Add( bSizerTolerance, 0, wxEXPAND, 5 );


	bSizer2->Add( bSizer8, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerSTEPFile->Add( bSizer2, 1, wxEXPAND, 5 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerSTEPFile->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerSTEPFile->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	this->SetSizer( bSizerSTEPFile );
	this->Layout();
	bSizerSTEPFile->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_STEP_OrgUnitChoice->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateUnits ), NULL, this );
	m_STEP_Xorg->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateXPos ), NULL, this );
	m_STEP_Yorg->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateYPos ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_STEP_BASE::onExportButton ), NULL, this );
}

DIALOG_EXPORT_STEP_BASE::~DIALOG_EXPORT_STEP_BASE()
{
	// Disconnect Events
	m_STEP_OrgUnitChoice->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateUnits ), NULL, this );
	m_STEP_Xorg->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateXPos ), NULL, this );
	m_STEP_Yorg->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateYPos ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_STEP_BASE::onExportButton ), NULL, this );

}

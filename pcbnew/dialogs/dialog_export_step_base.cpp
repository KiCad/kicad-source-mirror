///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun  3 2020)
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

	wxBoxSizer* bSizerTop;
	bSizerTop = new wxBoxSizer( wxHORIZONTAL );

	m_txtBrdFile = new wxStaticText( this, wxID_ANY, _("File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtBrdFile->Wrap( -1 );
	bSizerTop->Add( m_txtBrdFile, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	m_filePickerSTEP = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, _("Select a STEP export filename"), _("STEP files (*.stp;*.step)|*.stp;*.step"), wxDefaultPosition, wxSize( -1,-1 ), wxFLP_SAVE|wxFLP_USE_TEXTCTRL );
	bSizerTop->Add( m_filePickerSTEP, 1, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerSTEPFile->Add( bSizerTop, 0, wxEXPAND|wxALL, 10 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbCoordinates;
	sbCoordinates = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Coordinates") ), wxVERTICAL );

	m_rbDrillAndPlotOrigin = new wxRadioButton( sbCoordinates->GetStaticBox(), wxID_ANY, _("Drill/place file origin"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	sbCoordinates->Add( m_rbDrillAndPlotOrigin, 0, wxBOTTOM|wxRIGHT, 5 );

	m_rbGridOrigin = new wxRadioButton( sbCoordinates->GetStaticBox(), wxID_ANY, _("Grid origin"), wxDefaultPosition, wxDefaultSize, 0 );
	sbCoordinates->Add( m_rbGridOrigin, 0, wxBOTTOM|wxRIGHT, 5 );

	m_rbUserDefinedOrigin = new wxRadioButton( sbCoordinates->GetStaticBox(), wxID_ANY, _("User defined origin"), wxDefaultPosition, wxDefaultSize, 0 );
	sbCoordinates->Add( m_rbUserDefinedOrigin, 0, wxBOTTOM|wxRIGHT, 5 );

	m_rbBoardCenterOrigin = new wxRadioButton( sbCoordinates->GetStaticBox(), wxID_ANY, _("Board center origin"), wxDefaultPosition, wxDefaultSize, 0 );
	sbCoordinates->Add( m_rbBoardCenterOrigin, 0, wxBOTTOM|wxRIGHT, 5 );


	bSizer2->Add( sbCoordinates, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	wxStaticBoxSizer* sbUserDefinedOrigin;
	sbUserDefinedOrigin = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("User Defined Origin") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextUnits = new wxStaticText( sbUserDefinedOrigin->GetStaticBox(), wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnits->Wrap( -1 );
	fgSizer1->Add( m_staticTextUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_STEP_OrgUnitChoiceChoices[] = { _("mm"), _("inch") };
	int m_STEP_OrgUnitChoiceNChoices = sizeof( m_STEP_OrgUnitChoiceChoices ) / sizeof( wxString );
	m_STEP_OrgUnitChoice = new wxChoice( sbUserDefinedOrigin->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_STEP_OrgUnitChoiceNChoices, m_STEP_OrgUnitChoiceChoices, 0 );
	m_STEP_OrgUnitChoice->SetSelection( 0 );
	fgSizer1->Add( m_STEP_OrgUnitChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextXpos = new wxStaticText( sbUserDefinedOrigin->GetStaticBox(), wxID_ANY, _("X position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextXpos->Wrap( -1 );
	fgSizer1->Add( m_staticTextXpos, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_STEP_Xorg = new TEXT_CTRL_EVAL( sbUserDefinedOrigin->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_STEP_Xorg->HasFlag( wxTE_MULTILINE ) )
	{
	m_STEP_Xorg->SetMaxLength( 8 );
	}
	#else
	m_STEP_Xorg->SetMaxLength( 8 );
	#endif
	fgSizer1->Add( m_STEP_Xorg, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextYpos = new wxStaticText( sbUserDefinedOrigin->GetStaticBox(), wxID_ANY, _("Y position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextYpos->Wrap( -1 );
	fgSizer1->Add( m_staticTextYpos, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_STEP_Yorg = new TEXT_CTRL_EVAL( sbUserDefinedOrigin->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_STEP_Yorg->HasFlag( wxTE_MULTILINE ) )
	{
	m_STEP_Yorg->SetMaxLength( 8 );
	}
	#else
	m_STEP_Yorg->SetMaxLength( 8 );
	#endif
	fgSizer1->Add( m_STEP_Yorg, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );


	sbUserDefinedOrigin->Add( fgSizer1, 1, wxEXPAND, 5 );


	bSizer2->Add( sbUserDefinedOrigin, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	wxStaticBoxSizer* sbOtherOptions;
	sbOtherOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Other Options") ), wxVERTICAL );

	m_cbRemoveVirtual = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Ignore virtual components"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOtherOptions->Add( m_cbRemoveVirtual, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbSubstModels = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Substitute similarly named models"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbSubstModels->SetToolTip( _("Replace VRML models with STEP models of the same name") );

	sbOtherOptions->Add( m_cbSubstModels, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbOverwriteFile = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Overwrite old file"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOtherOptions->Add( m_cbOverwriteFile, 0, wxBOTTOM|wxRIGHT, 5 );

	m_staticTextTolerance = new wxStaticText( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Board outline chaining tolerance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTolerance->Wrap( -1 );
	sbOtherOptions->Add( m_staticTextTolerance, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	wxString m_toleranceChoices[] = { _("Tight (0.001 mm)"), _("Standard (0.01 mm)"), _("Loose (0.1 mm)") };
	int m_toleranceNChoices = sizeof( m_toleranceChoices ) / sizeof( wxString );
	m_tolerance = new wxChoice( sbOtherOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_toleranceNChoices, m_toleranceChoices, 0 );
	m_tolerance->SetSelection( 1 );
	m_tolerance->SetToolTip( _("Tolerance sets the distance between two points that are considered joined.") );

	sbOtherOptions->Add( m_tolerance, 0, wxALL|wxEXPAND, 5 );


	bSizer2->Add( sbOtherOptions, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bSizerSTEPFile->Add( bSizer2, 1, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizer81;
	bSizer81 = new wxBoxSizer( wxHORIZONTAL );


	bSizerSTEPFile->Add( bSizer81, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerSTEPFile->Add( m_staticline, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

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

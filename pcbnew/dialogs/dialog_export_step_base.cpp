///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/text_ctrl_eval.h"

#include "dialog_export_step_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_STEP_BASE::DIALOG_EXPORT_STEP_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	bSizerSTEPFile = new wxBoxSizer( wxVERTICAL );

	bSizerTop = new wxBoxSizer( wxHORIZONTAL );

	m_txtBrdFile = new wxStaticText( this, wxID_ANY, _("File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtBrdFile->Wrap( -1 );
	bSizerTop->Add( m_txtBrdFile, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_outputFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputFileName->SetToolTip( _("Enter a filename if you do not want to use default file names\nCan be used only when printing the current sheet") );
	m_outputFileName->SetMinSize( wxSize( 450,-1 ) );

	bSizerTop->Add( m_outputFileName, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizerTop->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


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

	m_cbRemoveDNP = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Ignore 'Do not populate' components"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRemoveDNP->SetToolTip( _("Do not show components marked 'Do not populate'") );

	sbOtherOptions->Add( m_cbRemoveDNP, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbRemoveUnspecified = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Ignore 'Unspecified' components"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRemoveUnspecified->SetToolTip( _("Do not show components with Footprint Type 'Unspecified'") );

	sbOtherOptions->Add( m_cbRemoveUnspecified, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbSubstModels = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Substitute similarly named models"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbSubstModels->SetToolTip( _("Replace VRML models with STEP models of the same name") );

	sbOtherOptions->Add( m_cbSubstModels, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbOverwriteFile = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Overwrite old file"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOtherOptions->Add( m_cbOverwriteFile, 0, wxBOTTOM|wxRIGHT, 5 );

	m_staticline11_hidden = new wxStaticLine( sbOtherOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_staticline11_hidden->Hide();

	sbOtherOptions->Add( m_staticline11_hidden, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_cbOptimizeStep = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Optimize STEP file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbOptimizeStep->SetToolTip( _("Disables writing parametric curves. Optimizes file size and write/read times, but may reduce compatibility with other software.") );

	sbOtherOptions->Add( m_cbOptimizeStep, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbExportCompound_hidden = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Export as Compound shape"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExportCompound_hidden->Hide();
	m_cbExportCompound_hidden->SetToolTip( _("Merges all shapes into a single Compound shape. Useful for external software that does de-duplication based on shape names.") );

	sbOtherOptions->Add( m_cbExportCompound_hidden, 0, wxBOTTOM|wxRIGHT, 5 );

	m_staticline1 = new wxStaticLine( sbOtherOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbOtherOptions->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_cbExportBody = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Export board body"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOtherOptions->Add( m_cbExportBody, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbExportComponents = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Export components"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOtherOptions->Add( m_cbExportComponents, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbExportTracks = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Export tracks, pads and vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExportTracks->SetToolTip( _("Export tracks, pads and vias on external copper layers.") );

	sbOtherOptions->Add( m_cbExportTracks, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbExportZones = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Export zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExportZones->SetToolTip( _("Export zones on external copper layers.") );

	sbOtherOptions->Add( m_cbExportZones, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbExportInnerCopper = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Export inner copper layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOtherOptions->Add( m_cbExportInnerCopper, 0, wxBOTTOM|wxEXPAND|wxRIGHT, 5 );

	m_cbFuseShapes = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Fuse shapes (time consuming)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbFuseShapes->SetToolTip( _("Combine intersecting geometry into one shape.") );

	sbOtherOptions->Add( m_cbFuseShapes, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbExportSilkscreen_hidden = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Export silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExportSilkscreen_hidden->Hide();
	m_cbExportSilkscreen_hidden->SetToolTip( _("Export silkscreen graphics.") );

	sbOtherOptions->Add( m_cbExportSilkscreen_hidden, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbExportSoldermask_hidden = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Export solder mask"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExportSoldermask_hidden->Hide();
	m_cbExportSoldermask_hidden->SetToolTip( _("Export solder mask graphics.") );

	sbOtherOptions->Add( m_cbExportSoldermask_hidden, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbExportSolderpaste_hidden = new wxCheckBox( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Export solder paste"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExportSolderpaste_hidden->Hide();
	m_cbExportSolderpaste_hidden->SetToolTip( _("Export solder paste graphics.") );

	sbOtherOptions->Add( m_cbExportSolderpaste_hidden, 0, wxBOTTOM|wxRIGHT, 5 );

	m_staticTextNetFilter = new wxStaticText( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Net filter (supports wildcards):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNetFilter->Wrap( -1 );
	sbOtherOptions->Add( m_staticTextNetFilter, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_txtNetFilter = new wxTextCtrl( sbOtherOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtNetFilter->SetToolTip( _("Only copper items belonging to nets matching this filter will be exported.") );

	sbOtherOptions->Add( m_txtNetFilter, 0, wxALL|wxEXPAND, 5 );

	m_staticTextTolerance = new wxStaticText( sbOtherOptions->GetStaticBox(), wxID_ANY, _("Board outline chaining tolerance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTolerance->Wrap( -1 );
	sbOtherOptions->Add( m_staticTextTolerance, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	wxString m_choiceToleranceChoices[] = { _("Tight (0.001 mm)"), _("Standard (0.01 mm)"), _("Loose (0.1 mm)") };
	int m_choiceToleranceNChoices = sizeof( m_choiceToleranceChoices ) / sizeof( wxString );
	m_choiceTolerance = new wxChoice( sbOtherOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceToleranceNChoices, m_choiceToleranceChoices, 0 );
	m_choiceTolerance->SetSelection( 1 );
	m_choiceTolerance->SetToolTip( _("Tolerance sets the distance between two points that are considered joined when building the board outlines.") );

	sbOtherOptions->Add( m_choiceTolerance, 0, wxALL|wxEXPAND, 5 );


	bSizer2->Add( sbOtherOptions, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bSizerSTEPFile->Add( bSizer2, 1, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizer81;
	bSizer81 = new wxBoxSizer( wxHORIZONTAL );


	bSizerSTEPFile->Add( bSizer81, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerSTEPFile->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxRIGHT|wxTOP, 5 );


	this->SetSizer( bSizerSTEPFile );
	this->Layout();
	bSizerSTEPFile->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_STEP_BASE::onBrowseClicked ), NULL, this );
	m_STEP_OrgUnitChoice->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateUnits ), NULL, this );
	m_STEP_Xorg->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateXPos ), NULL, this );
	m_STEP_Yorg->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateYPos ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_STEP_BASE::onExportButton ), NULL, this );
}

DIALOG_EXPORT_STEP_BASE::~DIALOG_EXPORT_STEP_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_STEP_BASE::onBrowseClicked ), NULL, this );
	m_STEP_OrgUnitChoice->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateUnits ), NULL, this );
	m_STEP_Xorg->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateXPos ), NULL, this );
	m_STEP_Yorg->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_STEP_BASE::onUpdateYPos ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_STEP_BASE::onExportButton ), NULL, this );

}

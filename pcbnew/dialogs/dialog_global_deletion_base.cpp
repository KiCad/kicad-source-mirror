///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun 18 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_global_deletion_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GLOBAL_DELETION_BASE::DIALOG_GLOBAL_DELETION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerLeft;
	sbSizerLeft = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Items to Delete") ), wxVERTICAL );

	m_delZones = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Zones"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_delZones, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_delTexts = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Text"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_delTexts, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_delBoardEdges = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Board outlines"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_delBoardEdges, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_delDrawings = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_delDrawings, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_delFootprints = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_delFootprints, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_delTracks = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Tracks && vias"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_delTracks, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_delMarkers = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_delMarkers, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_delAll = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Clear board"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_delAll, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerUpper->Add( sbSizerLeft, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	sbFilter = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Filter Settings") ), wxVERTICAL );

	m_drawingFilterLocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Locked graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFilter->Add( m_drawingFilterLocked, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_drawingFilterUnlocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Unlocked graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFilter->Add( m_drawingFilterUnlocked, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_footprintFilterLocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Locked footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFilter->Add( m_footprintFilterLocked, 0, wxALL, 5 );

	m_footprintFilterUnlocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Unlocked footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_footprintFilterUnlocked->SetValue(true);
	sbFilter->Add( m_footprintFilterUnlocked, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_trackFilterLocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Locked tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFilter->Add( m_trackFilterLocked, 0, wxALL, 5 );

	m_trackFilterUnlocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Unlocked tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackFilterUnlocked->SetValue(true);
	sbFilter->Add( m_trackFilterUnlocked, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_trackFilterVias = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackFilterVias->SetValue(true);
	sbFilter->Add( m_trackFilterVias, 0, wxRIGHT|wxLEFT, 5 );


	bSizerRight->Add( sbFilter, 0, wxALL|wxEXPAND, 5 );

	wxString m_rbLayersOptionChoices[] = { _("All layers"), _("Current layer only") };
	int m_rbLayersOptionNChoices = sizeof( m_rbLayersOptionChoices ) / sizeof( wxString );
	m_rbLayersOption = new wxRadioBox( this, wxID_ANY, _("Layer Filter"), wxDefaultPosition, wxDefaultSize, m_rbLayersOptionNChoices, m_rbLayersOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbLayersOption->SetSelection( 0 );
	bSizerRight->Add( m_rbLayersOption, 0, wxALL|wxEXPAND, 5 );


	bSizerUpper->Add( bSizerRight, 0, wxEXPAND, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Current layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer1->Add( m_staticText1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlCurrLayer = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer1->Add( m_textCtrlCurrLayer, 0, wxEXPAND|wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( fgSizer1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_delDrawings->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteDrawings ), NULL, this );
	m_delFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteFootprints ), NULL, this );
	m_delTracks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteTracks ), NULL, this );
}

DIALOG_GLOBAL_DELETION_BASE::~DIALOG_GLOBAL_DELETION_BASE()
{
	// Disconnect Events
	m_delDrawings->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteDrawings ), NULL, this );
	m_delFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteFootprints ), NULL, this );
	m_delTracks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteTracks ), NULL, this );

}

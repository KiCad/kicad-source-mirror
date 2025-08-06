///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
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

	m_delTeardrops = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Teardrops"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_delTeardrops, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_delMarkers = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_delMarkers, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_delAll = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Clear board"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_delAll, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerUpper->Add( sbSizerLeft, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	sbFilter = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Filter Settings") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 1, 3, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_drawingFilterLocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Locked graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_drawingFilterLocked, 0, wxRIGHT|wxLEFT, 5 );

	m_drawingFilterUnlocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Unlocked graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	m_drawingFilterUnlocked->SetValue(true);
	fgSizer2->Add( m_drawingFilterUnlocked, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_footprintFilterLocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Locked footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_footprintFilterLocked, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_footprintFilterUnlocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Unlocked footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_footprintFilterUnlocked->SetValue(true);
	fgSizer2->Add( m_footprintFilterUnlocked, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_trackFilterLocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Locked tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_trackFilterLocked, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_trackFilterUnlocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Unlocked tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackFilterUnlocked->SetValue(true);
	fgSizer2->Add( m_trackFilterUnlocked, 0, wxRIGHT|wxLEFT, 5 );

	m_viaFilterLocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Locked vias"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_viaFilterLocked, 0, wxRIGHT|wxLEFT, 5 );

	m_viaFilterUnlocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Unlocked vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaFilterUnlocked->SetValue(true);
	fgSizer2->Add( m_viaFilterUnlocked, 0, wxRIGHT|wxLEFT, 5 );


	sbFilter->Add( fgSizer2, 1, wxEXPAND, 5 );


	bSizerRight->Add( sbFilter, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	bSizerUpper->Add( bSizerRight, 0, wxEXPAND|wxLEFT, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND|wxALL, 5 );

	wxString m_rbLayersOptionChoices[] = { _("All layers"), _("Current layer (%s) only") };
	int m_rbLayersOptionNChoices = sizeof( m_rbLayersOptionChoices ) / sizeof( wxString );
	m_rbLayersOption = new wxRadioBox( this, wxID_ANY, _("Layer Filter"), wxDefaultPosition, wxDefaultSize, m_rbLayersOptionNChoices, m_rbLayersOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbLayersOption->SetSelection( 1 );
	bSizerMain->Add( m_rbLayersOption, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_delBoardEdges->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteBoardOutlines ), NULL, this );
	m_delDrawings->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteDrawings ), NULL, this );
	m_delFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteFootprints ), NULL, this );
	m_delTracks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteTracks ), NULL, this );
}

DIALOG_GLOBAL_DELETION_BASE::~DIALOG_GLOBAL_DELETION_BASE()
{
	// Disconnect Events
	m_delBoardEdges->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteBoardOutlines ), NULL, this );
	m_delDrawings->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteDrawings ), NULL, this );
	m_delFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteFootprints ), NULL, this );
	m_delTracks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::onCheckDeleteTracks ), NULL, this );

}

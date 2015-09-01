///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
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
	
	m_DelZones = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Zones"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelZones, 0, wxALL, 5 );
	
	m_DelTexts = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Text"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelTexts, 0, wxALL, 5 );
	
	m_DelBoardEdges = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Board outlines"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelBoardEdges, 0, wxALL, 5 );
	
	m_DelDrawings = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Drawings"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelDrawings, 0, wxALL, 5 );
	
	m_DelModules = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelModules, 0, wxALL, 5 );
	
	m_DelTracks = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelTracks, 0, wxALL, 5 );
	
	m_DelMarkers = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelMarkers, 0, wxALL, 5 );
	
	m_DelAlls = new wxCheckBox( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Clear Board"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelAlls, 0, wxALL, 5 );
	
	
	bSizerUpper->Add( sbSizerLeft, 2, wxEXPAND|wxALL, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	sbFilter = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Filter Settings") ), wxVERTICAL );
	
	m_TrackFilterAR = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Automatically routed tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackFilterAR->SetValue(true); 
	sbFilter->Add( m_TrackFilterAR, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TrackFilterLocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Locked tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFilter->Add( m_TrackFilterLocked, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TrackFilterNormal = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Unlocked tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackFilterNormal->SetValue(true); 
	sbFilter->Add( m_TrackFilterNormal, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TrackFilterVias = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackFilterVias->SetValue(true); 
	sbFilter->Add( m_TrackFilterVias, 0, wxALL, 5 );
	
	m_ModuleFilterLocked = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Locked footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFilter->Add( m_ModuleFilterLocked, 0, wxALL, 5 );
	
	m_ModuleFilterNormal = new wxCheckBox( sbFilter->GetStaticBox(), wxID_ANY, _("Unlocked footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ModuleFilterNormal->SetValue(true); 
	sbFilter->Add( m_ModuleFilterNormal, 0, wxALL, 5 );
	
	
	bSizerRight->Add( sbFilter, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_rbLayersOptionChoices[] = { _("All layers"), _("Current layer only") };
	int m_rbLayersOptionNChoices = sizeof( m_rbLayersOptionChoices ) / sizeof( wxString );
	m_rbLayersOption = new wxRadioBox( this, wxID_ANY, _("Layer Filter"), wxDefaultPosition, wxDefaultSize, m_rbLayersOptionNChoices, m_rbLayersOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbLayersOption->SetSelection( 0 );
	bSizerRight->Add( m_rbLayersOption, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerUpper->Add( bSizerRight, 3, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxALL|wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Current layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer1->Add( m_staticText1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlCurrLayer = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_textCtrlCurrLayer->SetMaxLength( 0 ); 
	fgSizer1->Add( m_textCtrlCurrLayer, 0, wxEXPAND|wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain->Add( fgSizer1, 0, wxEXPAND, 5 );
	
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
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_DelModules->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnCheckDeleteModules ), NULL, this );
	m_DelTracks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnCheckDeleteTracks ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnOkClick ), NULL, this );
}

DIALOG_GLOBAL_DELETION_BASE::~DIALOG_GLOBAL_DELETION_BASE()
{
	// Disconnect Events
	m_DelModules->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnCheckDeleteModules ), NULL, this );
	m_DelTracks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnCheckDeleteTracks ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnOkClick ), NULL, this );
	
}

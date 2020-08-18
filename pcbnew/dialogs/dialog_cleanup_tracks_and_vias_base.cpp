///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_cleanup_tracks_and_vias_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );

	m_cleanShortCircuitOpt = new wxCheckBox( this, wxID_ANY, _("Delete &tracks connecting different nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cleanShortCircuitOpt->SetToolTip( _("remove track segments connecting nodes belonging to different nets (short circuit)") );

	bSizerUpper->Add( m_cleanShortCircuitOpt, 0, wxALL, 5 );

	m_cleanViasOpt = new wxCheckBox( this, wxID_ANY, _("&Delete redundant vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cleanViasOpt->SetToolTip( _("remove vias on through hole pads and superimposed vias") );

	bSizerUpper->Add( m_cleanViasOpt, 0, wxALL, 5 );

	m_deleteDanglingViasOpt = new wxCheckBox( this, wxID_ANY, _("Delete vias connected on only one layer"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerUpper->Add( m_deleteDanglingViasOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_mergeSegmOpt = new wxCheckBox( this, wxID_ANY, _("&Merge co-linear tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mergeSegmOpt->SetToolTip( _("merge aligned track segments, and remove null segments") );

	bSizerUpper->Add( m_mergeSegmOpt, 0, wxALL, 5 );

	m_deleteUnconnectedOpt = new wxCheckBox( this, wxID_ANY, _("Delete tracks unconnected at one end"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deleteUnconnectedOpt->SetToolTip( _("delete tracks having at least one dangling end") );

	bSizerUpper->Add( m_deleteUnconnectedOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_deleteTracksInPadsOpt = new wxCheckBox( this, wxID_ANY, _("Delete tracks fully inside pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deleteTracksInPadsOpt->SetToolTip( _("Delete tracks that have both start and end positions inside of a pad") );

	bSizerUpper->Add( m_deleteTracksInPadsOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerMain->Add( bSizerUpper, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	bLowerSizer->SetMinSize( wxSize( 660,250 ) );
	staticChangesLabel = new wxStaticText( this, wxID_ANY, _("Changes To Be Applied:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticChangesLabel->Wrap( -1 );
	bLowerSizer->Add( staticChangesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_changesDataView = new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	bLowerSizer->Add( m_changesDataView, 1, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bLowerSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_cleanShortCircuitOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_cleanViasOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteDanglingViasOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_mergeSegmOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteUnconnectedOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteTracksInPadsOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_changesDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnSelectItem ), NULL, this );
	m_changesDataView->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnLeftDClickItem ), NULL, this );
}

DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::~DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE()
{
	// Disconnect Events
	m_cleanShortCircuitOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_cleanViasOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteDanglingViasOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_mergeSegmOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteUnconnectedOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteTracksInPadsOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_changesDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnSelectItem ), NULL, this );
	m_changesDataView->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnLeftDClickItem ), NULL, this );

}

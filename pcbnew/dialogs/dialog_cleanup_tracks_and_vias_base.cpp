///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 23 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_drclistbox.h"

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

	bSizerUpper->Add( m_cleanViasOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_mergeSegmOpt = new wxCheckBox( this, wxID_ANY, _("&Merge co-linear tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mergeSegmOpt->SetToolTip( _("merge aligned track segments, and remove null segments") );

	bSizerUpper->Add( m_mergeSegmOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_deleteUnconnectedOpt = new wxCheckBox( this, wxID_ANY, _("Delete &dangling tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deleteUnconnectedOpt->SetToolTip( _("delete tracks having at least one dangling end") );

	bSizerUpper->Add( m_deleteUnconnectedOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_deleteTracksInPadsOpt = new wxCheckBox( this, wxID_ANY, _("Delete Tracks in Pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deleteTracksInPadsOpt->SetToolTip( _("Delete tracks that have both start and end positions inside of a pad") );

	bSizerUpper->Add( m_deleteTracksInPadsOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerMain->Add( bSizerUpper, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	bLowerSizer->SetMinSize( wxSize( 660,250 ) );
	staticChangesLabel = new wxStaticText( this, wxID_ANY, _("Changes To Be Applied:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticChangesLabel->Wrap( -1 );
	bLowerSizer->Add( staticChangesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_ItemsListBox = new DRCLISTBOX( this, ID_CLEANUP_ITEMS_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bLowerSizer->Add( m_ItemsListBox, 1, wxEXPAND | wxALL, 5 );


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
	m_mergeSegmOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteUnconnectedOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteTracksInPadsOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_ItemsListBox->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnLeftDClickItem ), NULL, this );
	m_ItemsListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnSelectItem ), NULL, this );
	m_ItemsListBox->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnRightUpItem ), NULL, this );
}

DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::~DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE()
{
	// Disconnect Events
	m_cleanShortCircuitOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_cleanViasOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_mergeSegmOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteUnconnectedOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_deleteTracksInPadsOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnCheckBox ), NULL, this );
	m_ItemsListBox->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnLeftDClickItem ), NULL, this );
	m_ItemsListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnSelectItem ), NULL, this );
	m_ItemsListBox->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE::OnRightUpItem ), NULL, this );

}

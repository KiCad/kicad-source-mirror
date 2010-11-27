///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_global_deletion_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GLOBAL_DELETION_BASE::DIALOG_GLOBAL_DELETION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizerLeft;
	sbSizerLeft = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Items to delete") ), wxVERTICAL );
	
	m_DelZones = new wxCheckBox( this, wxID_ANY, _("Delete Zones"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelZones, 0, wxALL, 5 );
	
	m_DelTexts = new wxCheckBox( this, wxID_ANY, _("Delete Texts"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelTexts, 0, wxALL, 5 );
	
	m_DelBoardEdges = new wxCheckBox( this, wxID_ANY, _("Delete Board Outlines"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelBoardEdges, 0, wxALL, 5 );
	
	m_DelDrawings = new wxCheckBox( this, wxID_ANY, _("Delete Drawings"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelDrawings, 0, wxALL, 5 );
	
	m_DelModules = new wxCheckBox( this, wxID_ANY, _("Delete Modules"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelModules, 0, wxALL, 5 );
	
	m_DelTracks = new wxCheckBox( this, wxID_ANY, _("Delete Tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelTracks, 0, wxALL, 5 );
	
	m_DelMarkers = new wxCheckBox( this, wxID_ANY, _("Delete Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelMarkers, 0, wxALL, 5 );
	
	m_DelAlls = new wxCheckBox( this, wxID_ANY, _("Clear Board"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLeft->Add( m_DelAlls, 0, wxALL, 5 );
	
	bSizerMain->Add( sbSizerLeft, 1, wxEXPAND|wxALL, 5 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerTrackFilter;
	sbSizerTrackFilter = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Track Filter") ), wxVERTICAL );
	
	m_TrackFilterAR = new wxCheckBox( this, wxID_ANY, _("Include AutoRouted Tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTrackFilter->Add( m_TrackFilterAR, 0, wxALL, 5 );
	
	m_TrackFilterLocked = new wxCheckBox( this, wxID_ANY, _("Include Locked Tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTrackFilter->Add( m_TrackFilterLocked, 0, wxALL, 5 );
	
	bSizer2->Add( sbSizerTrackFilter, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer2->Add( 0, 10, 0, 0, 5 );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->SetDefault(); 
	bSizer2->Add( m_buttonOK, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_buttonCancel, 0, wxALL|wxEXPAND, 5 );
	
	bSizerMain->Add( bSizer2, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnOkClick ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnCancelClick ), NULL, this );
}

DIALOG_GLOBAL_DELETION_BASE::~DIALOG_GLOBAL_DELETION_BASE()
{
	// Disconnect Events
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnOkClick ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_DELETION_BASE::OnCancelClick ), NULL, this );
	
}

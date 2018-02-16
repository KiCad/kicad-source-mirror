///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DISPLAY_OPTIONS_BASE::DIALOG_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	sLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	
	bupperSizer->Add( sLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbAnnotations;
	sbAnnotations = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Annotations") ), wxVERTICAL );
	
	wxString m_ShowNetNamesOptionChoices[] = { _("Do not show"), _("On pads"), _("On tracks"), _("On pads and tracks") };
	int m_ShowNetNamesOptionNChoices = sizeof( m_ShowNetNamesOptionChoices ) / sizeof( wxString );
	m_ShowNetNamesOption = new wxRadioBox( sbAnnotations->GetStaticBox(), wxID_ANY, _("Show net names:"), wxDefaultPosition, wxDefaultSize, m_ShowNetNamesOptionNChoices, m_ShowNetNamesOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ShowNetNamesOption->SetSelection( 0 );
	m_ShowNetNamesOption->SetToolTip( _("Show or hide net names on pads and/or tracks.") );
	
	sbAnnotations->Add( m_ShowNetNamesOption, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_OptDisplayPadNumber = new wxCheckBox( sbAnnotations->GetStaticBox(), wxID_ANY, _("Show pad numbers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNumber->SetValue(true); 
	sbAnnotations->Add( m_OptDisplayPadNumber, 0, wxALL, 10 );
	
	m_OptDisplayPadNoConn = new wxCheckBox( sbAnnotations->GetStaticBox(), wxID_ANY, _("Show pad no net connection indicator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNoConn->SetValue(true); 
	sbAnnotations->Add( m_OptDisplayPadNoConn, 0, wxBOTTOM|wxLEFT|wxRIGHT, 10 );
	
	
	bRightSizer->Add( sbAnnotations, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbClearance;
	sbClearance = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Clearance Outlines") ), wxVERTICAL );
	
	wxString m_OptDisplayTracksClearanceChoices[] = { _("Never"), _("New track"), _("New track with via area"), _("New and edited tracks with via area"), _("Always") };
	int m_OptDisplayTracksClearanceNChoices = sizeof( m_OptDisplayTracksClearanceChoices ) / sizeof( wxString );
	m_OptDisplayTracksClearance = new wxRadioBox( sbClearance->GetStaticBox(), ID_SHOW_CLEARANCE, _("Show track clearance:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayTracksClearanceNChoices, m_OptDisplayTracksClearanceChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayTracksClearance->SetSelection( 1 );
	m_OptDisplayTracksClearance->SetToolTip( _("Show or hide the track and via clearance area. If \"New track\" is selected, track clearance area is shown only when creating the track.") );
	
	sbClearance->Add( m_OptDisplayTracksClearance, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_OptDisplayPadClearence = new wxCheckBox( sbClearance->GetStaticBox(), wxID_ANY, _("Show pad clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	sbClearance->Add( m_OptDisplayPadClearence, 0, wxALL, 10 );
	
	
	bRightSizer->Add( sbClearance, 1, wxALL|wxEXPAND, 5 );
	
	
	bupperSizer->Add( bRightSizer, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bupperSizer, 1, wxEXPAND, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bMainSizer->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

DIALOG_DISPLAY_OPTIONS_BASE::~DIALOG_DISPLAY_OPTIONS_BASE()
{
}

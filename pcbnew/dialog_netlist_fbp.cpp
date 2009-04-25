///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_netlist_fbp.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_NETLIST_FBP::DIALOG_NETLIST_FBP( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_Select_By_TimestampChoices[] = { _("Reference"), _("Timestamp") };
	int m_Select_By_TimestampNChoices = sizeof( m_Select_By_TimestampChoices ) / sizeof( wxString );
	m_Select_By_Timestamp = new wxRadioBox( this, wxID_ANY, _("Module Selection:"), wxDefaultPosition, wxDefaultSize, m_Select_By_TimestampNChoices, m_Select_By_TimestampChoices, 1, wxRA_SPECIFY_COLS );
	m_Select_By_Timestamp->SetSelection( 0 );
	m_Select_By_Timestamp->SetToolTip( _("Select how footprints are recognized:\nby their reference (U1, R3...) (normal setting)\nor their time stamp (special setting after a full schematic reannotation)") );
	
	bLeftSizer->Add( m_Select_By_Timestamp, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_ChangeExistingFootprintCtrlChoices[] = { _("Keep"), _("Change") };
	int m_ChangeExistingFootprintCtrlNChoices = sizeof( m_ChangeExistingFootprintCtrlChoices ) / sizeof( wxString );
	m_ChangeExistingFootprintCtrl = new wxRadioBox( this, wxID_ANY, _("Exchange Module:"), wxDefaultPosition, wxDefaultSize, m_ChangeExistingFootprintCtrlNChoices, m_ChangeExistingFootprintCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_ChangeExistingFootprintCtrl->SetSelection( 0 );
	m_ChangeExistingFootprintCtrl->SetToolTip( _("Keep or change an existing footprint when the netlist gives a different footprint") );
	
	bLeftSizer->Add( m_ChangeExistingFootprintCtrl, 0, wxALL|wxEXPAND, 5 );
	
	bUpperSizer->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_DeleteBadTracksChoices[] = { _("Keep"), _("Delete") };
	int m_DeleteBadTracksNChoices = sizeof( m_DeleteBadTracksChoices ) / sizeof( wxString );
	m_DeleteBadTracks = new wxRadioBox( this, wxID_ANY, _("Bad Tracks Deletion:"), wxDefaultPosition, wxDefaultSize, m_DeleteBadTracksNChoices, m_DeleteBadTracksChoices, 1, wxRA_SPECIFY_COLS );
	m_DeleteBadTracks->SetSelection( 0 );
	m_DeleteBadTracks->SetToolTip( _("Keep or delete bad tracks after a netlist change") );
	
	bMiddleSizer->Add( m_DeleteBadTracks, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_RemoveExtraFootprintsCtrlChoices[] = { _("Keep"), _("Delete") };
	int m_RemoveExtraFootprintsCtrlNChoices = sizeof( m_RemoveExtraFootprintsCtrlChoices ) / sizeof( wxString );
	m_RemoveExtraFootprintsCtrl = new wxRadioBox( this, wxID_ANY, _("Extra Footprints"), wxDefaultPosition, wxDefaultSize, m_RemoveExtraFootprintsCtrlNChoices, m_RemoveExtraFootprintsCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_RemoveExtraFootprintsCtrl->SetSelection( 0 );
	m_RemoveExtraFootprintsCtrl->SetToolTip( _("Remove footprints found on the Board but not in netlist\nNote: only not locked footprints will be removed") );
	
	bMiddleSizer->Add( m_RemoveExtraFootprintsCtrl, 0, wxALL|wxEXPAND, 5 );
	
	bUpperSizer->Add( bMiddleSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizerButtons;
	bRightSizerButtons = new wxBoxSizer( wxVERTICAL );
	
	m_button1 = new wxButton( this, ID_OPEN_NELIST, _("Browse Netlist Files"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizerButtons->Add( m_button1, 0, wxALL|wxEXPAND, 5 );
	
	m_button2 = new wxButton( this, ID_READ_NETLIST_FILE, _("Read Current Netlist"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button2->SetToolTip( _("Read the current netlist and update connections and connectivity info") );
	
	bRightSizerButtons->Add( m_button2, 0, wxALL|wxEXPAND, 5 );
	
	m_button3 = new wxButton( this, ID_TEST_NETLIST, _("Footprints Test"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button3->SetToolTip( _("Read the current neltist file and list missing and extra footprints") );
	
	bRightSizerButtons->Add( m_button3, 0, wxALL|wxEXPAND, 5 );
	
	m_button4 = new wxButton( this, ID_COMPILE_RATSNEST, _("Rebuild Board Connectivity"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button4->SetToolTip( _("Rebuild the full ratsnest (usefull after a manual pad netname edition)") );
	
	bRightSizerButtons->Add( m_button4, 0, wxALL|wxEXPAND, 5 );
	
	m_button5 = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizerButtons->Add( m_button5, 0, wxALL|wxEXPAND, 5 );
	
	bUpperSizer->Add( bRightSizerButtons, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bMainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_staticTextNetfilename = new wxStaticText( this, wxID_ANY, _("Netlist File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNetfilename->Wrap( -1 );
	bMainSizer->Add( m_staticTextNetfilename, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_NetlistFilenameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bMainSizer->Add( m_NetlistFilenameCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bMainSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MessageWindow = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CHARWRAP|wxTE_MULTILINE|wxTE_READONLY|wxTE_WORDWRAP );
	bMainSizer->Add( m_MessageWindow, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_button1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_FBP::OnOpenNelistClick ), NULL, this );
	m_button2->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_FBP::OnReadNetlistFileClick ), NULL, this );
	m_button3->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_FBP::OnTestFootprintsClick ), NULL, this );
	m_button4->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_FBP::OnCompileRatsnestClick ), NULL, this );
	m_button5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_FBP::OnCancelClick ), NULL, this );
}

DIALOG_NETLIST_FBP::~DIALOG_NETLIST_FBP()
{
	// Disconnect Events
	m_button1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_FBP::OnOpenNelistClick ), NULL, this );
	m_button2->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_FBP::OnReadNetlistFileClick ), NULL, this );
	m_button3->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_FBP::OnTestFootprintsClick ), NULL, this );
	m_button4->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_FBP::OnCompileRatsnestClick ), NULL, this );
	m_button5->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_FBP::OnCancelClick ), NULL, this );
}

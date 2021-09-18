///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_netlist_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_NETLIST_BASE::DIALOG_NETLIST_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerNetlistFilename;
	bSizerNetlistFilename = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* staticTextNetlistFile;
	staticTextNetlistFile = new wxStaticText( this, wxID_ANY, _("Netlist file:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticTextNetlistFile->Wrap( -1 );
	bSizerNetlistFilename->Add( staticTextNetlistFile, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_NetlistFilenameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerNetlistFilename->Add( m_NetlistFilenameCtrl, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_browseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_browseButton->SetMinSize( wxSize( 30,28 ) );

	bSizerNetlistFilename->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2 );


	bMainSizer->Add( bSizerNetlistFilename, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxString m_matchByTimestampChoices[] = { _("Link footprints using component tstamps (unique ids)"), _("Link footprints using reference designators") };
	int m_matchByTimestampNChoices = sizeof( m_matchByTimestampChoices ) / sizeof( wxString );
	m_matchByTimestamp = new wxRadioBox( this, wxID_ANY, _("Link Method"), wxDefaultPosition, wxDefaultSize, m_matchByTimestampNChoices, m_matchByTimestampChoices, 1, wxRA_SPECIFY_COLS );
	m_matchByTimestamp->SetSelection( 0 );
	m_matchByTimestamp->SetToolTip( _("Select whether to update footprint references to match their currently-assigned symbols, or to re-assign footprints to symbols which match their current references.") );

	bUpperSizer->Add( m_matchByTimestamp, 1, wxALIGN_TOP|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	m_cbUpdateFootprints = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Replace footprints with those specified in netlist"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_cbUpdateFootprints, 0, wxBOTTOM, 5 );

	m_cbDeleteExtraFootprints = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Delete footprints with no components in netlist"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_cbDeleteExtraFootprints, 0, wxBOTTOM, 5 );

	m_cbDeleteShortingTracks = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Delete tracks shorting multiple nets"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_cbDeleteShortingTracks, 0, wxBOTTOM, 5 );


	bUpperSizer->Add( sbSizer1, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	bMainSizer->Add( bUpperSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	bLowerSizer->SetMinSize( wxSize( -1,250 ) );
	m_MessageWindow = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bLowerSizer->Add( m_MessageWindow, 1, wxEXPAND | wxALL, 5 );


	bMainSizer->Add( bLowerSizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_buttonsSizer->Add( m_sdbSizer1, 1, wxEXPAND, 5 );


	bMainSizer->Add( m_buttonsSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_NetlistFilenameCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_NETLIST_BASE::OnFilenameKillFocus ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnOpenNetlistClick ), NULL, this );
	m_matchByTimestamp->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnMatchChanged ), NULL, this );
	m_cbUpdateFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteExtraFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteShortingTracks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnUpdatePCB ), NULL, this );
}

DIALOG_NETLIST_BASE::~DIALOG_NETLIST_BASE()
{
	// Disconnect Events
	m_NetlistFilenameCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_NETLIST_BASE::OnFilenameKillFocus ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnOpenNetlistClick ), NULL, this );
	m_matchByTimestamp->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnMatchChanged ), NULL, this );
	m_cbUpdateFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteExtraFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteShortingTracks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NETLIST_BASE::OnUpdatePCB ), NULL, this );

}

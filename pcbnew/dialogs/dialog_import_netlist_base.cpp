///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_html_report_panel.h"

#include "dialog_import_netlist_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMPORT_NETLIST_BASE::DIALOG_IMPORT_NETLIST_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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
	bSizerNetlistFilename->Add( m_NetlistFilenameCtrl, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizerNetlistFilename->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL, 2 );


	bMainSizer->Add( bSizerNetlistFilename, 0, wxEXPAND|wxALL, 10 );

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

	m_cbDeleteExtraFootprints = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Delete footprints with no components in netlist"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_cbDeleteExtraFootprints, 0, wxBOTTOM, 5 );

	m_cbUpdateFootprints = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Replace footprints with those specified in netlist"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUpdateFootprints->SetValue(true);
	sbSizer1->Add( m_cbUpdateFootprints, 0, wxBOTTOM, 5 );

	m_cbTransferGroups = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Group footprints based on symbol group"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTransferGroups->SetValue(true);
	sbSizer1->Add( m_cbTransferGroups, 0, wxBOTTOM, 5 );

	m_cbOverrideLocks = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Delete/replace footprints even if locked"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_cbOverrideLocks, 0, wxBOTTOM|wxRIGHT, 5 );

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

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_buttonsSizer->Add( m_sdbSizer, 1, wxEXPAND, 5 );


	bMainSizer->Add( m_buttonsSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_NetlistFilenameCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnFilenameKillFocus ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::onBrowseNetlistFiles ), NULL, this );
	m_matchByTimestamp->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnMatchChanged ), NULL, this );
	m_cbDeleteExtraFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbTransferGroups->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbOverrideLocks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteShortingTracks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizerApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::onUpdatePCB ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::onImportNetlist ), NULL, this );
}

DIALOG_IMPORT_NETLIST_BASE::~DIALOG_IMPORT_NETLIST_BASE()
{
	// Disconnect Events
	m_NetlistFilenameCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnFilenameKillFocus ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::onBrowseNetlistFiles ), NULL, this );
	m_matchByTimestamp->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnMatchChanged ), NULL, this );
	m_cbDeleteExtraFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbTransferGroups->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbOverrideLocks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteShortingTracks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizerApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::onUpdatePCB ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_NETLIST_BASE::onImportNetlist ), NULL, this );

}

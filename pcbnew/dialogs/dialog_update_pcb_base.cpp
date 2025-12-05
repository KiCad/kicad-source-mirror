///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_html_report_panel.h"

#include "dialog_update_pcb_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_UPDATE_PCB_BASE::DIALOG_UPDATE_PCB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbOptions;
	sbOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	m_cbRelinkFootprints = new wxCheckBox( sbOptions->GetStaticBox(), wxID_ANY, _("Re-link footprints to schematic symbols based on their reference designators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRelinkFootprints->SetToolTip( _("Normally footprints are linked to their symbols via their Unique IDs.  Select this option only if you want to reset the footprint linkages based on their reference designators.") );

	sbOptions->Add( m_cbRelinkFootprints, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbTransferGroups = new wxCheckBox( sbOptions->GetStaticBox(), wxID_ANY, _("Group footprints based on symbol group"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOptions->Add( m_cbTransferGroups, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bUpperSizer->Add( sbOptions, 1, wxEXPAND|wxALL, 5 );


	bUpperSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbFootprints;
	sbFootprints = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Update Footprints") ), wxVERTICAL );

	m_cbUpdateFootprints = new wxCheckBox( sbFootprints->GetStaticBox(), wxID_ANY, _("Replace footprints with those specified by symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUpdateFootprints->SetValue(true);
	m_cbUpdateFootprints->SetToolTip( _("Normally footprints on the board should be changed to match footprint assignment changes made in the schematic. Uncheck this only if you don't want to change existing footprints on the board.") );

	sbFootprints->Add( m_cbUpdateFootprints, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbDeleteExtraFootprints = new wxCheckBox( sbFootprints->GetStaticBox(), wxID_ANY, _("Delete footprints with no symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbDeleteExtraFootprints->SetToolTip( _("Remove from the board unlocked footprints which are not linked to a schematic symbol.") );

	sbFootprints->Add( m_cbDeleteExtraFootprints, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbOverrideLocks = new wxCheckBox( sbFootprints->GetStaticBox(), wxID_ANY, _("Override locks"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFootprints->Add( m_cbOverrideLocks, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bUpperSizer->Add( sbFootprints, 0, wxEXPAND|wxALL, 5 );


	bUpperSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbFields;
	sbFields = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Update Fields") ), wxVERTICAL );

	m_cbUpdateFields = new wxCheckBox( sbFields->GetStaticBox(), wxID_ANY, _("Update footprint fields from symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUpdateFields->SetValue(true);
	sbFields->Add( m_cbUpdateFields, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbRemoveExtraFields = new wxCheckBox( sbFields->GetStaticBox(), wxID_ANY, _("Remove footprint fields not found in symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFields->Add( m_cbRemoveExtraFields, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bUpperSizer->Add( sbFields, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bUpperSizer, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	bLowerSizer->SetMinSize( wxSize( 660,300 ) );
	m_messagePanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );
	bLowerSizer->Add( m_messagePanel, 1, wxEXPAND | wxALL, 5 );


	bMainSizer->Add( bLowerSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bMainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_cbRelinkFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbTransferGroups->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteExtraFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbOverrideLocks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateFields->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbRemoveExtraFields->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnUpdateClick ), NULL, this );
}

DIALOG_UPDATE_PCB_BASE::~DIALOG_UPDATE_PCB_BASE()
{
	// Disconnect Events
	m_cbRelinkFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbTransferGroups->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteExtraFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbOverrideLocks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateFields->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbRemoveExtraFields->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnUpdateClick ), NULL, this );

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_update_pcb_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_UPDATE_PCB_BASE::DIALOG_UPDATE_PCB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	m_cbRelinkFootprints = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Re-link footprints to schematic symbols based on their reference designators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRelinkFootprints->SetToolTip( _("Normally footprints are linked to their symbols via their Unique IDs.  Select this option only if you want to reset the footprint linkages based on their reference designators.") );

	sbSizer1->Add( m_cbRelinkFootprints, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbDeleteExtraFootprints = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Delete footprints with no symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbDeleteExtraFootprints->SetToolTip( _("Remove from the board unlocked footprints which are not linked to a schematic symbol.") );

	sbSizer1->Add( m_cbDeleteExtraFootprints, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbUpdateFootprints = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Replace footprints with those specified in the schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUpdateFootprints->SetValue(true);
	m_cbUpdateFootprints->SetToolTip( _("Normally footprints on the board should be changed to match footprint assignment changes made in the schematic. Uncheck this only if you don't want to change existing footprints on the board.") );

	sbSizer1->Add( m_cbUpdateFootprints, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bUpperSizer->Add( sbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


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
	m_cbDeleteExtraFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnUpdateClick ), NULL, this );
}

DIALOG_UPDATE_PCB_BASE::~DIALOG_UPDATE_PCB_BASE()
{
	// Disconnect Events
	m_cbRelinkFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteExtraFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnUpdateClick ), NULL, this );

}

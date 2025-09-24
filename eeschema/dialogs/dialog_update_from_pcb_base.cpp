///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_html_report_panel.h"

#include "dialog_update_from_pcb_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_UPDATE_FROM_PCB_BASE::DIALOG_UPDATE_FROM_PCB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerOptions;
	sbSizerOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxVERTICAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbRelinkFootprints = new wxCheckBox( sbSizerOptions->GetStaticBox(), wxID_ANY, _("Re-link footprints to schematic symbols based on their reference designators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRelinkFootprints->SetToolTip( _("Normally footprints are linked to their symbols via their Unique IDs.  Select this option only if you want to reset the footprint linkages based on their reference designators.") );

	fgSizer1->Add( m_cbRelinkFootprints, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbSizerOptions->Add( fgSizer1, 1, wxEXPAND|wxBOTTOM, 5 );


	bUpperSizer->Add( sbSizerOptions, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Update") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer2->AddGrowableCol( 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbUpdateReferences = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Reference designators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUpdateReferences->SetValue(true);
	m_cbUpdateReferences->SetToolTip( _("Update references of symbols that have been changed in the PCB editor.") );

	fgSizer2->Add( m_cbUpdateReferences, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbUpdateFootprints = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Footprint assignments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUpdateFootprints->SetValue(true);
	m_cbUpdateFootprints->SetToolTip( _("Update footprint associations of symbols whose footprints have been replaced with different footprints in PCB.") );

	fgSizer2->Add( m_cbUpdateFootprints, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbUpdateValues = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUpdateValues->SetValue(true);
	m_cbUpdateValues->SetToolTip( _("Update symbols values that have been replaced in the PCB editor.") );

	fgSizer2->Add( m_cbUpdateValues, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbUpdateNetNames = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Net names"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_cbUpdateNetNames, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbUpdateAttributes = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Attributes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUpdateAttributes->SetValue(true);
	fgSizer2->Add( m_cbUpdateAttributes, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_cbPreferUnitSwaps = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Prefer symbol unit swaps over label swaps"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPreferUnitSwaps->SetValue(true);
	m_cbPreferUnitSwaps->SetToolTip( _("When possible, detect footprint gate swaps and apply corresponding symbol unit swaps instead of changing net labels.") );

	fgSizer2->Add( m_cbPreferUnitSwaps, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_cbUpdateOtherFields = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Other fields"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUpdateOtherFields->SetValue(true);
	m_cbUpdateOtherFields->SetToolTip( _("Update all other fields in the symbol from the footprint") );

	fgSizer2->Add( m_cbUpdateOtherFields, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_cbPreferPinSwaps = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Prefer symbol pin swaps over label swaps"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPreferPinSwaps->SetValue(true);
	m_cbPreferPinSwaps->SetToolTip( _("When possible, swap symbol pins to match footprint pad net swaps instead of changing net labels.") );

	fgSizer2->Add( m_cbPreferPinSwaps, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	sbSizer2->Add( fgSizer2, 1, wxEXPAND, 5 );


	bUpperSizer->Add( sbSizer2, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bUpperSizer, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	bLowerSizer->SetMinSize( wxSize( 600,260 ) );
	m_messagePanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bLowerSizer->Add( m_messagePanel, 1, wxEXPAND | wxALL, 5 );


	bMainSizer->Add( bLowerSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_cbRelinkFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateReferences->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateValues->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateNetNames->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateAttributes->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbPreferUnitSwaps->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateOtherFields->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbPreferPinSwaps->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnUpdateClick ), NULL, this );
}

DIALOG_UPDATE_FROM_PCB_BASE::~DIALOG_UPDATE_FROM_PCB_BASE()
{
	// Disconnect Events
	m_cbRelinkFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateReferences->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateValues->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateNetNames->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateAttributes->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbPreferUnitSwaps->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbUpdateOtherFields->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbPreferPinSwaps->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FROM_PCB_BASE::OnUpdateClick ), NULL, this );

}

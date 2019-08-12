///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
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
	
	wxString m_matchByTimestampChoices[] = { _("Update footprint references to match any changed symbol references"), _("Update footprint associations by existing references") };
	int m_matchByTimestampNChoices = sizeof( m_matchByTimestampChoices ) / sizeof( wxString );
	m_matchByTimestamp = new wxRadioBox( this, wxID_ANY, _("Match Method"), wxDefaultPosition, wxDefaultSize, m_matchByTimestampNChoices, m_matchByTimestampChoices, 1, wxRA_SPECIFY_COLS );
	m_matchByTimestamp->SetSelection( 0 );
	m_matchByTimestamp->SetToolTip( _("The first option uses the existing links between symbols and their footprints to update the footprints based on changes made to their symbols.  \n\nThe second option uses the symbol and footprint references to establish a new set of links between symbols and footprints, and then updates the footprints accordingly.") );
	
	bUpperSizer->Add( m_matchByTimestamp, 0, wxALIGN_TOP|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );
	
	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 2, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_cbUpdateFootprints = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Replace footprints of symbols whose footprint assignments have changed"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUpdateFootprints->SetToolTip( _("Normally footprints on the board should be changed to match footprint assignment changes made in the schematic. Uncheck this only if you don't want to change existing footprints on the board.") );
	
	gbSizer1->Add( m_cbUpdateFootprints, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_cbDeleteExtraFootprints = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Delete footprints with no associated symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbDeleteExtraFootprints, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_cbDeleteSinglePadNets = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Delete nets containing only a single pad"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbDeleteSinglePadNets, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer1->Add( gbSizer1, 1, wxEXPAND, 5 );
	
	
	bUpperSizer->Add( sbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	bMainSizer->Add( bUpperSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );
	
	bLowerSizer->SetMinSize( wxSize( 660,300 ) ); 
	m_messagePanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
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
	m_matchByTimestamp->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnMatchChanged ), NULL, this );
	m_cbUpdateFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteExtraFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteSinglePadNets->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnUpdateClick ), NULL, this );
}

DIALOG_UPDATE_PCB_BASE::~DIALOG_UPDATE_PCB_BASE()
{
	// Disconnect Events
	m_matchByTimestamp->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnMatchChanged ), NULL, this );
	m_cbUpdateFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteExtraFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_cbDeleteSinglePadNets->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnOptionChanged ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_PCB_BASE::OnUpdateClick ), NULL, this );
	
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_map_gerber_layers_to_pcb_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE::DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* sbUpperSizer;
	sbUpperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLayerSelection;
	bSizerLayerSelection = new wxBoxSizer( wxVERTICAL );

	m_staticTextLayerSel = new wxStaticText( this, wxID_ANY, _("Layer selection:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLayerSel->Wrap( -1 );
	bSizerLayerSelection->Add( m_staticTextLayerSel, 0, wxALL, 5 );

	m_bSizerLayerList = new wxBoxSizer( wxHORIZONTAL );


	m_bSizerLayerList->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	m_flexLeftColumnBoxSizer = new wxFlexGridSizer( 16, 4, 0, 0 );
	m_flexLeftColumnBoxSizer->AddGrowableCol( 1 );
	m_flexLeftColumnBoxSizer->SetFlexibleDirection( wxBOTH );
	m_flexLeftColumnBoxSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	m_bSizerLayerList->Add( m_flexLeftColumnBoxSizer, 1, wxEXPAND, 5 );

	m_staticlineSep = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	m_bSizerLayerList->Add( m_staticlineSep, 0, wxEXPAND | wxALL, 5 );

	m_flexRightColumnBoxSizer = new wxFlexGridSizer( 16, 4, 0, 0 );
	m_flexRightColumnBoxSizer->AddGrowableCol( 1 );
	m_flexRightColumnBoxSizer->SetFlexibleDirection( wxBOTH );
	m_flexRightColumnBoxSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	m_bSizerLayerList->Add( m_flexRightColumnBoxSizer, 1, wxEXPAND, 5 );


	bSizerLayerSelection->Add( m_bSizerLayerList, 1, wxEXPAND, 5 );


	sbUpperSizer->Add( bSizerLayerSelection, 1, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerLyrCnt;
	bSizerLyrCnt = new wxBoxSizer( wxVERTICAL );

	m_staticTextCopperlayerCount = new wxStaticText( this, wxID_ANY, _("Copper layers count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCopperlayerCount->Wrap( -1 );
	bSizerLyrCnt->Add( m_staticTextCopperlayerCount, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_comboCopperLayersCount = new wxComboBox( this, wxID_ANY, _("2 Layers"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_comboCopperLayersCount->Append( _("2 Layers") );
	m_comboCopperLayersCount->Append( _("4 Layers") );
	m_comboCopperLayersCount->Append( _("6 Layers") );
	m_comboCopperLayersCount->Append( _("8 Layers") );
	m_comboCopperLayersCount->Append( _("10 Layers") );
	m_comboCopperLayersCount->Append( _("12 Layers") );
	m_comboCopperLayersCount->Append( _("14 Layers") );
	m_comboCopperLayersCount->Append( _("16 Layers") );
	m_comboCopperLayersCount->Append( _("18 Layers") );
	m_comboCopperLayersCount->Append( _("20 Layers") );
	m_comboCopperLayersCount->Append( _("22 Layers") );
	m_comboCopperLayersCount->Append( _("24 Layers") );
	m_comboCopperLayersCount->Append( _("26 Layers") );
	m_comboCopperLayersCount->Append( _("28 Layers") );
	m_comboCopperLayersCount->Append( _("30 Layers") );
	m_comboCopperLayersCount->Append( _("32 Layers") );
	bSizerLyrCnt->Add( m_comboCopperLayersCount, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRightSizer->Add( bSizerLyrCnt, 0, wxEXPAND, 5 );


	bRightSizer->Add( 5, 15, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxVERTICAL );

	m_buttonStore = new wxButton( this, wxID_ANY, _("Store Choice"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonStore, 0, wxALL|wxEXPAND, 5 );

	m_buttonRetrieve = new wxButton( this, wxID_ANY, _("Get Stored Choice"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonRetrieve, 0, wxALL|wxEXPAND, 5 );

	m_buttonReset = new wxButton( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonReset, 0, wxALL|wxEXPAND, 5 );


	bRightSizer->Add( bSizerButtons, 0, wxEXPAND, 5 );


	sbUpperSizer->Add( bRightSizer, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );


	bSizerMain->Add( sbUpperSizer, 1, wxEXPAND|wxALL, 5 );

	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();

	bSizerMain->Add( m_sdbSizerButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_comboCopperLayersCount->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE::OnBrdLayersCountSelection ), NULL, this );
	m_buttonStore->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE::OnStoreSetup ), NULL, this );
	m_buttonRetrieve->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE::OnGetSetup ), NULL, this );
	m_buttonReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE::OnResetClick ), NULL, this );
}

DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE::~DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE()
{
	// Disconnect Events
	m_comboCopperLayersCount->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE::OnBrdLayersCountSelection ), NULL, this );
	m_buttonStore->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE::OnStoreSetup ), NULL, this );
	m_buttonRetrieve->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE::OnGetSetup ), NULL, this );
	m_buttonReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE::OnResetClick ), NULL, this );

}

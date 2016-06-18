///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  6 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_layers_select_to_pcb_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( LAYERS_MAP_DIALOG_BASE, DIALOG_SHIM )
	EVT_COMBOBOX( ID_M_COMBOCOPPERLAYERSCOUNT, LAYERS_MAP_DIALOG_BASE::_wxFB_OnBrdLayersCountSelection )
	EVT_BUTTON( ID_STORE_CHOICE, LAYERS_MAP_DIALOG_BASE::_wxFB_OnStoreSetup )
	EVT_BUTTON( ID_GET_PREVIOUS_CHOICE, LAYERS_MAP_DIALOG_BASE::_wxFB_OnGetSetup )
	EVT_BUTTON( ID_RESET_CHOICE, LAYERS_MAP_DIALOG_BASE::_wxFB_OnResetClick )
	EVT_BUTTON( wxID_OK, LAYERS_MAP_DIALOG_BASE::_wxFB_OnOkClick )
END_EVENT_TABLE()

LAYERS_MAP_DIALOG_BASE::LAYERS_MAP_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* sbUpperSizer;
	sbUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	sbSizerLayersTable = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layers selection:") ), wxHORIZONTAL );
	
	m_flexLeftColumnBoxSizer = new wxFlexGridSizer( 16, 4, 0, 0 );
	m_flexLeftColumnBoxSizer->AddGrowableCol( 0 );
	m_flexLeftColumnBoxSizer->AddGrowableCol( 1 );
	m_flexLeftColumnBoxSizer->AddGrowableCol( 2 );
	m_flexLeftColumnBoxSizer->AddGrowableCol( 3 );
	m_flexLeftColumnBoxSizer->SetFlexibleDirection( wxBOTH );
	m_flexLeftColumnBoxSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	sbSizerLayersTable->Add( m_flexLeftColumnBoxSizer, 1, wxEXPAND, 5 );
	
	m_staticlineSep = new wxStaticLine( sbSizerLayersTable->GetStaticBox(), ID_M_STATICLINESEP, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	sbSizerLayersTable->Add( m_staticlineSep, 0, wxEXPAND | wxALL, 5 );
	
	
	sbUpperSizer->Add( sbSizerLayersTable, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerLyrCnt;
	bSizerLyrCnt = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextCopperlayerCount = new wxStaticText( this, ID_M_STATICTEXTCOPPERLAYERCOUNT, _("Copper layers count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCopperlayerCount->Wrap( -1 );
	bSizerLyrCnt->Add( m_staticTextCopperlayerCount, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_comboCopperLayersCount = new wxComboBox( this, ID_M_COMBOCOPPERLAYERSCOUNT, _("2 Layers"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_comboCopperLayersCount->Append( _("2 Layers") );
	m_comboCopperLayersCount->Append( _("4 Layers") );
	m_comboCopperLayersCount->Append( _("6 Layers") );
	m_comboCopperLayersCount->Append( _("8 Layers") );
	m_comboCopperLayersCount->Append( _("10 Layers") );
	m_comboCopperLayersCount->Append( _("12 Layers") );
	m_comboCopperLayersCount->Append( _("14 Layers") );
	m_comboCopperLayersCount->Append( _("16 Layers") );
	bSizerLyrCnt->Add( m_comboCopperLayersCount, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bRightSizer->Add( bSizerLyrCnt, 0, wxEXPAND, 5 );
	
	
	bRightSizer->Add( 5, 15, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxVERTICAL );
	
	m_buttonStore = new wxButton( this, ID_STORE_CHOICE, _("Store Choice"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonStore, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonRetrieve = new wxButton( this, ID_GET_PREVIOUS_CHOICE, _("Get Stored Choice"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonRetrieve, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonReset = new wxButton( this, ID_RESET_CHOICE, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonReset, 0, wxALL|wxEXPAND, 5 );
	
	
	bRightSizer->Add( bSizerButtons, 0, wxEXPAND, 5 );
	
	
	sbUpperSizer->Add( bRightSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain->Add( sbUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();
	
	bSizerMain->Add( m_sdbSizerButtons, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
}

LAYERS_MAP_DIALOG_BASE::~LAYERS_MAP_DIALOG_BASE()
{
}

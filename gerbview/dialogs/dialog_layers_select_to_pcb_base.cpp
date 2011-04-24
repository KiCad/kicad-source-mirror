///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 17 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_layers_select_to_pcb_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( LAYERS_TABLE_DIALOG_BASE, wxDialog )
	EVT_BUTTON( ID_STORE_CHOICE, LAYERS_TABLE_DIALOG_BASE::_wxFB_OnStoreSetup )
	EVT_BUTTON( ID_GET_PREVIOUS_CHOICE, LAYERS_TABLE_DIALOG_BASE::_wxFB_OnGetSetup )
	EVT_BUTTON( ID_RESET_CHOICE, LAYERS_TABLE_DIALOG_BASE::_wxFB_OnResetClick )
	EVT_BUTTON( wxID_CANCEL, LAYERS_TABLE_DIALOG_BASE::_wxFB_OnCancelClick )
	EVT_BUTTON( wxID_OK, LAYERS_TABLE_DIALOG_BASE::_wxFB_OnOkClick )
END_EVENT_TABLE()

LAYERS_TABLE_DIALOG_BASE::LAYERS_TABLE_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
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
	
	m_staticlineSep = new wxStaticLine( this, ID_M_STATICLINESEP, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	sbSizerLayersTable->Add( m_staticlineSep, 0, wxEXPAND | wxALL, 5 );
	
	m_flexRightColumnBoxSizer = new wxFlexGridSizer( 16, 4, 0, 0 );
	m_flexRightColumnBoxSizer->AddGrowableCol( 0 );
	m_flexRightColumnBoxSizer->AddGrowableCol( 1 );
	m_flexRightColumnBoxSizer->AddGrowableCol( 2 );
	m_flexRightColumnBoxSizer->AddGrowableCol( 3 );
	m_flexRightColumnBoxSizer->SetFlexibleDirection( wxBOTH );
	m_flexRightColumnBoxSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	sbSizerLayersTable->Add( m_flexRightColumnBoxSizer, 1, wxEXPAND, 5 );
	
	sbUpperSizer->Add( sbSizerLayersTable, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxVERTICAL );
	
	m_buttonStore = new wxButton( this, ID_STORE_CHOICE, _("Store Choice"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonStore, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonRetrieve = new wxButton( this, ID_GET_PREVIOUS_CHOICE, _("Get Stored Choice"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonRetrieve, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonReset = new wxButton( this, ID_RESET_CHOICE, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonReset, 0, wxALL|wxEXPAND, 5 );
	
	sbUpperSizer->Add( bSizerButtons, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
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

LAYERS_TABLE_DIALOG_BASE::~LAYERS_TABLE_DIALOG_BASE()
{
}

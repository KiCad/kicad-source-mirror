///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_dielectric_list_manager_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DIELECTRIC_MATERIAL_BASE::DIALOG_DIELECTRIC_MATERIAL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerNewItem;
	bSizerNewItem = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizerNewDielectric;
	fgSizerNewDielectric = new wxFlexGridSizer( 2, 3, 0, 5 );
	fgSizerNewDielectric->AddGrowableCol( 0 );
	fgSizerNewDielectric->SetFlexibleDirection( wxBOTH );
	fgSizerNewDielectric->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTexMaterial = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTexMaterial->Wrap( -1 );
	fgSizerNewDielectric->Add( m_staticTexMaterial, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextEpsilonR = new wxStaticText( this, wxID_ANY, _("Epsilon R:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEpsilonR->Wrap( -1 );
	fgSizerNewDielectric->Add( m_staticTextEpsilonR, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextLossTg = new wxStaticText( this, wxID_ANY, _("Loss Tan:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLossTg->Wrap( -1 );
	fgSizerNewDielectric->Add( m_staticTextLossTg, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_tcMaterial = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_tcMaterial->SetMinSize( wxSize( 150,-1 ) );

	fgSizerNewDielectric->Add( m_tcMaterial, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_tcEpsilonR = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerNewDielectric->Add( m_tcEpsilonR, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_tcLossTg = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerNewDielectric->Add( m_tcLossTg, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerNewItem->Add( fgSizerNewDielectric, 1, 0, 5 );


	bSizerUpper->Add( bSizerNewItem, 0, wxEXPAND, 5 );

	m_staticText = new wxStaticText( this, wxID_ANY, _("Common materials:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText->Wrap( -1 );
	bSizerUpper->Add( m_staticText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_lcMaterials = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizerUpper->Add( m_lcMaterials, 1, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_lcMaterials->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_DIELECTRIC_MATERIAL_BASE::onListItemSelected ), NULL, this );
	m_lcMaterials->Connect( wxEVT_COMMAND_LIST_KEY_DOWN, wxListEventHandler( DIALOG_DIELECTRIC_MATERIAL_BASE::onListKeyDown ), NULL, this );
}

DIALOG_DIELECTRIC_MATERIAL_BASE::~DIALOG_DIELECTRIC_MATERIAL_BASE()
{
	// Disconnect Events
	m_lcMaterials->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_DIELECTRIC_MATERIAL_BASE::onListItemSelected ), NULL, this );
	m_lcMaterials->Disconnect( wxEVT_COMMAND_LIST_KEY_DOWN, wxListEventHandler( DIALOG_DIELECTRIC_MATERIAL_BASE::onListKeyDown ), NULL, this );

}

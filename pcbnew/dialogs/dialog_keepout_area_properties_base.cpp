///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_keepout_area_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::DIALOG_KEEPOUT_AREA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLayersListSizer;
	bLayersListSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextLayerSelection = new wxStaticText( this, wxID_ANY, _("Layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLayerSelection->Wrap( -1 );
	bLayersListSizer->Add( m_staticTextLayerSelection, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_layers = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	bLayersListSizer->Add( m_layers, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bUpperSizer->Add( bLayersListSizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_cbTracksCtrl = new wxCheckBox( this, wxID_ANY, _("Keepout tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_cbTracksCtrl, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_cbViasCtrl = new wxCheckBox( this, wxID_ANY, _("Keepout vias"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_cbViasCtrl, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_cbCopperPourCtrl = new wxCheckBox( this, wxID_ANY, _("Keepout copper pours"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_cbCopperPourCtrl, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerRight->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizerLowerRight;
	bSizerLowerRight = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextSlope = new wxStaticText( this, wxID_ANY, _("Outline slope:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSlope->Wrap( -1 );
	bSizerLowerRight->Add( m_staticTextSlope, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_OrientEdgesOptChoices[] = { _("Arbitrary"), _("H, V, and 45 deg only") };
	int m_OrientEdgesOptNChoices = sizeof( m_OrientEdgesOptChoices ) / sizeof( wxString );
	m_OrientEdgesOpt = new wxChoice( this, ID_M_ORIENTEDGESOPT, wxDefaultPosition, wxDefaultSize, m_OrientEdgesOptNChoices, m_OrientEdgesOptChoices, 0 );
	m_OrientEdgesOpt->SetSelection( 0 );
	bSizerLowerRight->Add( m_OrientEdgesOpt, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_staticTextStyle = new wxStaticText( this, wxID_ANY, _("Outline style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyle->Wrap( -1 );
	bSizerLowerRight->Add( m_staticTextStyle, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_OutlineAppearanceCtrlChoices[] = { _("Line"), _("Hatched"), _("Fully hatched") };
	int m_OutlineAppearanceCtrlNChoices = sizeof( m_OutlineAppearanceCtrlChoices ) / sizeof( wxString );
	m_OutlineAppearanceCtrl = new wxChoice( this, ID_M_OUTLINEAPPEARANCECTRL, wxDefaultPosition, wxDefaultSize, m_OutlineAppearanceCtrlNChoices, m_OutlineAppearanceCtrlChoices, 0 );
	m_OutlineAppearanceCtrl->SetSelection( 0 );
	bSizerLowerRight->Add( m_OutlineAppearanceCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	bSizerRight->Add( bSizerLowerRight, 1, wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bSizerRight, 0, wxEXPAND|wxALL, 5 );
	
	
	bMainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();
	
	bMainSizer->Add( m_sdbSizerButtons, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_layers->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::OnLayerSelection ), NULL, this );
	m_layers->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::OnSizeLayersList ), NULL, this );
}

DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::~DIALOG_KEEPOUT_AREA_PROPERTIES_BASE()
{
	// Disconnect Events
	m_layers->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::OnLayerSelection ), NULL, this );
	m_layers->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::OnSizeLayersList ), NULL, this );
	
}

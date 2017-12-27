///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 22 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_keepout_area_properties_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_KEEPOUT_AREA_PROPERTIES_BASE, DIALOG_SHIM )
	EVT_DATAVIEW_ITEM_VALUE_CHANGED( wxID_ANY, DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::_wxFB_OnLayerSelection )
END_EVENT_TABLE()

DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::DIALOG_KEEPOUT_AREA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 500,-1 ), wxDefaultSize );
	
	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_UpperSizer;
	m_UpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* m_layersListSizer;
	m_layersListSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextLayerSelection = new wxStaticText( this, wxID_ANY, _("Keepout area layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLayerSelection->Wrap( -1 );
	m_layersListSizer->Add( m_staticTextLayerSelection, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_layers = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES|wxDV_NO_HEADER );
	m_layersListSizer->Add( m_layers, 1, wxALL|wxEXPAND, 5 );
	
	
	m_UpperSizer->Add( m_layersListSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextprops = new wxStaticText( this, wxID_ANY, _("Properties:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextprops->Wrap( -1 );
	bSizerRight->Add( m_staticTextprops, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxString m_OrientEdgesOptChoices[] = { _("Any orientation"), _("180, 90, and 45 degrees") };
	int m_OrientEdgesOptNChoices = sizeof( m_OrientEdgesOptChoices ) / sizeof( wxString );
	m_OrientEdgesOpt = new wxRadioBox( this, wxID_ANY, _("Zone Edge Orientation:"), wxDefaultPosition, wxDefaultSize, m_OrientEdgesOptNChoices, m_OrientEdgesOptChoices, 1, wxRA_SPECIFY_COLS );
	m_OrientEdgesOpt->SetSelection( 1 );
	bSizerRight->Add( m_OrientEdgesOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxString m_OutlineAppearanceCtrlChoices[] = { _("Line"), _("Hatched outline"), _("Full hatched") };
	int m_OutlineAppearanceCtrlNChoices = sizeof( m_OutlineAppearanceCtrlChoices ) / sizeof( wxString );
	m_OutlineAppearanceCtrl = new wxRadioBox( this, wxID_ANY, _("Outline Appearence:"), wxDefaultPosition, wxDefaultSize, m_OutlineAppearanceCtrlNChoices, m_OutlineAppearanceCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_OutlineAppearanceCtrl->SetSelection( 1 );
	bSizerRight->Add( m_OutlineAppearanceCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	wxStaticBoxSizer* sbSizerCutoutOpts;
	sbSizerCutoutOpts = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Keepout Options:") ), wxVERTICAL );
	
	m_cbTracksCtrl = new wxCheckBox( sbSizerCutoutOpts->GetStaticBox(), wxID_ANY, _("No tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerCutoutOpts->Add( m_cbTracksCtrl, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_cbViasCtrl = new wxCheckBox( sbSizerCutoutOpts->GetStaticBox(), wxID_ANY, _("No vias"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerCutoutOpts->Add( m_cbViasCtrl, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_cbCopperPourCtrl = new wxCheckBox( sbSizerCutoutOpts->GetStaticBox(), wxID_ANY, _("No copper pour"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerCutoutOpts->Add( m_cbCopperPourCtrl, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerRight->Add( sbSizerCutoutOpts, 0, wxEXPAND|wxALL, 5 );
	
	
	m_UpperSizer->Add( bSizerRight, 0, wxEXPAND, 5 );
	
	
	m_MainSizer->Add( m_UpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_MainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();
	
	m_MainSizer->Add( m_sdbSizerButtons, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::~DIALOG_KEEPOUT_AREA_PROPERTIES_BASE()
{
}

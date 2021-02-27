///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Dec 30 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_rule_area_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_RULE_AREA_PROPERTIES_BASE::DIALOG_RULE_AREA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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

	m_layers = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER|wxBORDER_SIMPLE );
	bLayersListSizer->Add( m_layers, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bUpperSizer->Add( bLayersListSizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	m_staticTextBasicRules = new wxStaticText( this, wxID_ANY, _("Basic Rules"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBasicRules->Wrap( -1 );
	bSizerRight->Add( m_staticTextBasicRules, 0, wxALL, 5 );

	m_cbTracksCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTracksCtrl->SetToolTip( _("Prevent tracks from routing into this area") );

	bSizerRight->Add( m_cbTracksCtrl, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_cbViasCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbViasCtrl->SetToolTip( _("Prevent vias from being placed in this area") );

	bSizerRight->Add( m_cbViasCtrl, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_cbPadsCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPadsCtrl->SetToolTip( _("Raise a DRC error if a pad overlaps this area") );

	bSizerRight->Add( m_cbPadsCtrl, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_cbCopperPourCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out copper fill"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbCopperPourCtrl->SetToolTip( _("Zones will not fill copper into this area") );

	bSizerRight->Add( m_cbCopperPourCtrl, 0, wxALL|wxEXPAND, 5 );


	bSizerRight->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_cbFootprintsCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbFootprintsCtrl->SetToolTip( _("Raise a DRC error if a footprint courtyard overlaps this area") );

	bSizerRight->Add( m_cbFootprintsCtrl, 0, wxALL, 5 );


	bSizerRight->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_cbConstrainCtrl = new wxCheckBox( this, wxID_ANY, _("Constrain outline to H, V and 45 deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbConstrainCtrl->SetToolTip( _("Draw the area using horizontal, verical and 45 degree lines only") );

	bSizerRight->Add( m_cbConstrainCtrl, 0, wxALL, 5 );


	bSizerRight->Add( 0, 0, 0, wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bSizerLowerRight;
	bSizerLowerRight = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextStyle = new wxStaticText( this, wxID_ANY, _("Outline display:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyle->Wrap( -1 );
	bSizerLowerRight->Add( m_staticTextStyle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_OutlineDisplayCtrlChoices[] = { _("Line"), _("Hatched"), _("Fully hatched") };
	int m_OutlineDisplayCtrlNChoices = sizeof( m_OutlineDisplayCtrlChoices ) / sizeof( wxString );
	m_OutlineDisplayCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_OutlineDisplayCtrlNChoices, m_OutlineDisplayCtrlChoices, 0 );
	m_OutlineDisplayCtrl->SetSelection( 0 );
	bSizerLowerRight->Add( m_OutlineDisplayCtrl, 0, wxALL, 5 );


	bSizerRight->Add( bSizerLowerRight, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Area name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	m_staticText3->SetToolTip( _("A unique name for this rule area for use in DRC rules") );

	bSizer6->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_tcName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_tcName->SetToolTip( _("A unique name for this rule area for use in DRC rules") );

	bSizer6->Add( m_tcName, 1, wxALL, 5 );


	bSizerRight->Add( bSizer6, 1, wxEXPAND, 5 );


	bUpperSizer->Add( bSizerRight, 0, wxALL|wxEXPAND, 10 );


	bMainSizer->Add( bUpperSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();

	bMainSizer->Add( m_sdbSizerButtons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_layers->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::OnLayerSelection ), NULL, this );
	m_layers->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::onLayerListRightDown ), NULL, this );
	m_layers->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::OnSizeLayersList ), NULL, this );
}

DIALOG_RULE_AREA_PROPERTIES_BASE::~DIALOG_RULE_AREA_PROPERTIES_BASE()
{
	// Disconnect Events
	m_layers->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::OnLayerSelection ), NULL, this );
	m_layers->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::onLayerListRightDown ), NULL, this );
	m_layers->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::OnSizeLayersList ), NULL, this );

}

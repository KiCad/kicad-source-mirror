///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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
	bLayersListSizer->Add( m_staticTextLayerSelection, 0, wxALL, 4 );

	m_layers = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER|wxBORDER_SIMPLE );
	bLayersListSizer->Add( m_layers, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bUpperSizer->Add( bLayersListSizer, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bUpperSizer->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Area name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	m_staticText3->SetToolTip( _("A unique name for this rule area for use in DRC rules") );

	bSizer6->Add( m_staticText3, 0, wxRIGHT|wxLEFT, 5 );


	bSizer6->Add( 0, 2, 0, 0, 5 );

	m_tcName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_tcName->SetToolTip( _("A unique name for this rule area for use in DRC rules") );

	bSizer6->Add( m_tcName, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_cbLocked = new wxCheckBox( this, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_cbLocked, 0, wxALL, 5 );


	bSizerRight->Add( bSizer6, 0, wxEXPAND|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 1, 3, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextBasicRules = new wxStaticText( this, wxID_ANY, _("Basic rules:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBasicRules->Wrap( -1 );
	fgSizer2->Add( m_staticTextBasicRules, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_cbTracksCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTracksCtrl->SetToolTip( _("Prevent tracks from routing into this area") );

	fgSizer2->Add( m_cbTracksCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_cbViasCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbViasCtrl->SetToolTip( _("Prevent vias from being placed in this area") );

	fgSizer2->Add( m_cbViasCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_cbPadsCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPadsCtrl->SetToolTip( _("Raise a DRC error if a pad overlaps this area") );

	fgSizer2->Add( m_cbPadsCtrl, 0, wxRIGHT|wxLEFT, 5 );

	m_cbCopperPourCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out zone fills"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbCopperPourCtrl->SetToolTip( _("Zones will not fill copper into this area") );

	fgSizer2->Add( m_cbCopperPourCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_cbFootprintsCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbFootprintsCtrl->SetToolTip( _("Raise a DRC error if a footprint courtyard overlaps this area") );

	fgSizer2->Add( m_cbFootprintsCtrl, 0, wxRIGHT|wxLEFT, 5 );


	bSizerRight->Add( fgSizer2, 0, wxEXPAND, 5 );


	bSizerRight->Add( 0, 3, 0, wxEXPAND, 5 );


	bSizerRight->Add( 0, 0, 0, wxEXPAND, 5 );


	bSizerRight->Add( 0, 0, 0, wxEXPAND|wxBOTTOM, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,2 ) );

	m_staticTextStyle = new wxStaticText( this, wxID_ANY, _("Outline display:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyle->Wrap( -1 );
	gbSizer1->Add( m_staticTextStyle, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_OutlineDisplayCtrlChoices[] = { _("Line"), _("Hatched"), _("Fully hatched") };
	int m_OutlineDisplayCtrlNChoices = sizeof( m_OutlineDisplayCtrlChoices ) / sizeof( wxString );
	m_OutlineDisplayCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_OutlineDisplayCtrlNChoices, m_OutlineDisplayCtrlChoices, 0 );
	m_OutlineDisplayCtrl->SetSelection( 0 );
	gbSizer1->Add( m_OutlineDisplayCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_stBorderHatchPitchText = new wxStaticText( this, wxID_ANY, _("Outline hatch pitch:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stBorderHatchPitchText->Wrap( -1 );
	gbSizer1->Add( m_stBorderHatchPitchText, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_outlineHatchPitchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outlineHatchPitchCtrl->SetToolTip( _("A unique name for this rule area for use in DRC rules") );

	gbSizer1->Add( m_outlineHatchPitchCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_outlineHatchUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_outlineHatchUnits->Wrap( -1 );
	gbSizer1->Add( m_outlineHatchUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	bSizerRight->Add( gbSizer1, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bUpperSizer->Add( bSizerRight, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bUpperSizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();

	bMainSizer->Add( m_sdbSizerButtons, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


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

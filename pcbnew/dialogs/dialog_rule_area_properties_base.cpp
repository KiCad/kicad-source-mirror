///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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

	bLayersListSizer->SetMinSize( wxSize( 150,-1 ) );
	m_staticTextLayerSelection = new wxStaticText( this, wxID_ANY, _("Layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLayerSelection->Wrap( -1 );
	bLayersListSizer->Add( m_staticTextLayerSelection, 0, wxALL, 4 );

	m_layers = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER|wxBORDER_SIMPLE );
	bLayersListSizer->Add( m_layers, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bUpperSizer->Add( bLayersListSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


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


	bSizerRight->Add( bSizer6, 0, wxBOTTOM|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 1, 3, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxString m_rbRuleTypeChoices[] = { _("Keepout"), _("Placement") };
	int m_rbRuleTypeNChoices = sizeof( m_rbRuleTypeChoices ) / sizeof( wxString );
	m_rbRuleType = new wxRadioBox( this, wxID_ANY, _("Rule type"), wxDefaultPosition, wxDefaultSize, m_rbRuleTypeNChoices, m_rbRuleTypeChoices, 2, wxRA_SPECIFY_COLS );
	m_rbRuleType->SetSelection( 0 );
	fgSizer2->Add( m_rbRuleType, 0, wxALL|wxEXPAND, 5 );

	m_keepoutRuleSizer = new wxFlexGridSizer( 0, 1, 3, 0 );
	m_keepoutRuleSizer->SetFlexibleDirection( wxBOTH );
	m_keepoutRuleSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbTracksCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTracksCtrl->SetToolTip( _("Prevent tracks from routing into this area") );

	m_keepoutRuleSizer->Add( m_cbTracksCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_cbViasCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbViasCtrl->SetToolTip( _("Prevent vias from being placed in this area") );

	m_keepoutRuleSizer->Add( m_cbViasCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_cbPadsCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPadsCtrl->SetToolTip( _("Raise a DRC error if a pad overlaps this area") );

	m_keepoutRuleSizer->Add( m_cbPadsCtrl, 0, wxRIGHT|wxLEFT, 5 );

	m_cbCopperPourCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out zone fills"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbCopperPourCtrl->SetToolTip( _("Zones will not fill copper into this area") );

	m_keepoutRuleSizer->Add( m_cbCopperPourCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_cbFootprintsCtrl = new wxCheckBox( this, wxID_ANY, _("Keep out footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbFootprintsCtrl->SetToolTip( _("Raise a DRC error if a footprint courtyard overlaps this area") );

	m_keepoutRuleSizer->Add( m_cbFootprintsCtrl, 0, wxRIGHT|wxLEFT, 5 );


	fgSizer2->Add( m_keepoutRuleSizer, 1, wxEXPAND, 5 );


	bSizerRight->Add( fgSizer2, 1, wxEXPAND, 5 );

	m_placementRuleSizer = new wxFlexGridSizer( 0, 1, 3, 0 );
	m_placementRuleSizer->AddGrowableCol( 0 );
	m_placementRuleSizer->SetFlexibleDirection( wxBOTH );
	m_placementRuleSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText31 = new wxStaticText( this, wxID_ANY, _("Condition:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	m_staticText31->SetToolTip( _("A unique name for this rule area for use in DRC rules") );

	m_placementRuleSizer->Add( m_staticText31, 0, wxALL|wxEXPAND, 5 );

	m_ruleText = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString );
	m_ruleText->SetUseTabs( true );
	m_ruleText->SetTabWidth( 4 );
	m_ruleText->SetIndent( 4 );
	m_ruleText->SetTabIndents( true );
	m_ruleText->SetBackSpaceUnIndents( true );
	m_ruleText->SetViewEOL( false );
	m_ruleText->SetViewWhiteSpace( false );
	m_ruleText->SetMarginWidth( 2, 0 );
	m_ruleText->SetIndentationGuides( true );
	m_ruleText->SetReadOnly( false );
	m_ruleText->SetMarginType( 1, wxSTC_MARGIN_SYMBOL );
	m_ruleText->SetMarginMask( 1, wxSTC_MASK_FOLDERS );
	m_ruleText->SetMarginWidth( 1, 16);
	m_ruleText->SetMarginSensitive( 1, true );
	m_ruleText->SetProperty( wxT("fold"), wxT("1") );
	m_ruleText->SetFoldFlags( wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED );
	m_ruleText->SetMarginType( 0, wxSTC_MARGIN_NUMBER );
	m_ruleText->SetMarginWidth( 0, m_ruleText->TextWidth( wxSTC_STYLE_LINENUMBER, wxT("_99999") ) );
	m_ruleText->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_ruleText->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_ruleText->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_ruleText->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_ruleText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_ruleText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_ruleText->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_ruleText->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_ruleText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_ruleText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_ruleText->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_ruleText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_ruleText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_ruleText->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_ruleText->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_ruleText->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_ruleText->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	m_placementRuleSizer->Add( m_ruleText, 0, wxALL|wxEXPAND, 5 );


	bSizerRight->Add( m_placementRuleSizer, 0, wxEXPAND, 5 );

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


	bUpperSizer->Add( bSizerRight, 1, wxEXPAND|wxALL, 5 );


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

	this->Centre( wxBOTH );

	// Connect Events
	m_layers->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::OnLayerSelection ), NULL, this );
	m_layers->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::onLayerListRightDown ), NULL, this );
	m_layers->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::OnSizeLayersList ), NULL, this );
	m_rbRuleType->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::OnRuleTypeSelect ), NULL, this );
}

DIALOG_RULE_AREA_PROPERTIES_BASE::~DIALOG_RULE_AREA_PROPERTIES_BASE()
{
	// Disconnect Events
	m_layers->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::OnLayerSelection ), NULL, this );
	m_layers->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::onLayerListRightDown ), NULL, this );
	m_layers->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::OnSizeLayersList ), NULL, this );
	m_rbRuleType->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_RULE_AREA_PROPERTIES_BASE::OnRuleTypeSelect ), NULL, this );

}

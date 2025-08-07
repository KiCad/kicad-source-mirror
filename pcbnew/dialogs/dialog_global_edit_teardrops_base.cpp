///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_global_edit_teardrops_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::DIALOG_GLOBAL_EDIT_TEARDROPS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerTop;
	bSizerTop = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbScope;
	sbScope = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scope") ), wxVERTICAL );

	m_pthPads = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("PTH pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pthPads->SetValue(true);
	sbScope->Add( m_pthPads, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_smdPads = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("SMD pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_smdPads->SetValue(true);
	sbScope->Add( m_smdPads, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_vias = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vias->SetValue(true);
	sbScope->Add( m_vias, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_trackToTrack = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Track to track"), wxDefaultPosition, wxDefaultSize, 0 );
	sbScope->Add( m_trackToTrack, 0, wxALL, 5 );


	bSizerTop->Add( sbScope, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbFilters;
	sbFilters = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Filter Items") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->AddGrowableRow( 0 );
	fgSizer3->AddGrowableRow( 1 );
	fgSizer3->AddGrowableRow( 2 );
	fgSizer3->AddGrowableRow( 3 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_netFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by net:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_netFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_netFilter = new NET_SELECTOR( sbFilters->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_netFilter, 1, wxEXPAND|wxRIGHT, 5 );

	m_netclassFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by net class:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_netclassFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_netclassFilterChoices;
	m_netclassFilter = new wxChoice( sbFilters->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_netclassFilterChoices, 0 );
	m_netclassFilter->SetSelection( 0 );
	fgSizer3->Add( m_netclassFilter, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 3 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 3 );

	m_layerFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_layerFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_layerFilter = new PCB_LAYER_BOX_SELECTOR( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizer3->Add( m_layerFilter, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer3->Add( 0, 5, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	sbFilters->Add( fgSizer3, 1, wxEXPAND, 5 );


	sbFilters->Add( 0, 5, 0, 0, 5 );

	m_roundPadsFilter = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Round pads only"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFilters->Add( m_roundPadsFilter, 0, wxRIGHT|wxLEFT, 5 );

	m_existingFilter = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Existing teardrops only"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFilters->Add( m_existingFilter, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_selectedItemsFilter = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Selected items only"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFilters->Add( m_selectedItemsFilter, 0, wxALL, 5 );


	bSizerTop->Add( sbFilters, 2, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bSizerTop, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbAction;
	sbAction = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Action") ), wxVERTICAL );

	m_removeTeardrops = new wxRadioButton( sbAction->GetStaticBox(), wxID_ANY, _("Remove teardrops"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_removeTeardrops->SetToolTip( _("Remove teardrops according to filtering options") );

	sbAction->Add( m_removeTeardrops, 0, wxBOTTOM|wxRIGHT, 10 );

	m_removeAllTeardrops = new wxRadioButton( sbAction->GetStaticBox(), wxID_ANY, _("Remove all teardrops"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeAllTeardrops->SetToolTip( _("Remove all teardrops, regardless of filtering options") );

	sbAction->Add( m_removeAllTeardrops, 0, wxBOTTOM|wxRIGHT, 10 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );

	m_addTeardrops = new wxRadioButton( sbAction->GetStaticBox(), wxID_ANY, _("Add teardrops with default values for shape"), wxDefaultPosition, wxDefaultSize, 0 );
	m_addTeardrops->SetValue( true );
	bSizer12->Add( m_addTeardrops, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_boardSetupLink = new wxHyperlinkCtrl( sbAction->GetStaticBox(), wxID_ANY, _("Edit default values in Board Setup"), wxT("#teardrops"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	bSizer12->Add( m_boardSetupLink, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );


	sbAction->Add( bSizer12, 0, wxEXPAND|wxBOTTOM, 15 );

	m_specifiedValues = new wxRadioButton( sbAction->GetStaticBox(), wxID_ANY, _("Add teardrops with specified values:"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAction->Add( m_specifiedValues, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerSpecifiedValues;
	bSizerSpecifiedValues = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerCols11;
	bSizerCols11 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol11;
	bSizerLeftCol11 = new wxBoxSizer( wxVERTICAL );

	m_cbPreferZoneConnection = new wxCheckBox( sbAction->GetStaticBox(), wxID_ANY, _("Prefer zone connection"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	m_cbPreferZoneConnection->SetToolTip( _("Do not create teardrops on tracks connected to pads that are also connected to a copper zone.") );

	bSizerLeftCol11->Add( m_cbPreferZoneConnection, 0, wxBOTTOM|wxRIGHT, 5 );

	m_cbTeardropsUseNextTrack = new wxCheckBox( sbAction->GetStaticBox(), wxID_ANY, _("Allow teardrops to span two track segments"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	m_cbTeardropsUseNextTrack->SetToolTip( _("Allows a teardrop to extend over the first 2 connected track segments if the first track segment is too short to accommodate the best length.") );

	bSizerLeftCol11->Add( m_cbTeardropsUseNextTrack, 0, wxBOTTOM|wxRIGHT, 5 );


	bSizerCols11->Add( bSizerLeftCol11, 1, wxEXPAND|wxTOP, 3 );


	bSizerCols11->Add( 25, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerRightCol11;
	bSizerRightCol11 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer39;
	bSizer39 = new wxBoxSizer( wxHORIZONTAL );

	m_stHDRatio = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("Track width limit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio->Wrap( -1 );
	m_stHDRatio->SetToolTip( _("Max pad/via size to track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	bSizer39->Add( m_stHDRatio, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_tcHDRatio = new wxTextCtrl( sbAction->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_tcHDRatio->SetToolTip( _("Tracks which are similar in size to the pad or via do not need teardrops.") );

	bSizer39->Add( m_tcHDRatio, 0, wxRIGHT|wxLEFT, 5 );

	m_stHDRatioUnits = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatioUnits->Wrap( -1 );
	bSizer39->Add( m_stHDRatioUnits, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerRightCol11->Add( bSizer39, 0, wxEXPAND, 3 );

	m_minTrackWidthHint = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("(as a percentage of pad/via minor dimension)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthHint->Wrap( -1 );
	bSizerRightCol11->Add( m_minTrackWidthHint, 0, wxTOP|wxBOTTOM|wxLEFT, 2 );


	bSizerCols11->Add( bSizerRightCol11, 1, wxEXPAND|wxLEFT, 10 );


	bSizerSpecifiedValues->Add( bSizerCols11, 0, wxEXPAND|wxTOP, 3 );


	bSizerSpecifiedValues->Add( 0, 5, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerShapeColumns;
	bSizerShapeColumns = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol;
	bSizerLeftCol = new wxBoxSizer( wxVERTICAL );

	m_bitmapTeardrop = new wxStaticBitmap( sbAction->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeftCol->Add( m_bitmapTeardrop, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxHORIZONTAL );


	bSizerLeftCol->Add( bSizer41, 0, wxEXPAND|wxBOTTOM, 5 );


	bSizerShapeColumns->Add( bSizerLeftCol, 1, wxEXPAND|wxRIGHT, 10 );


	bSizerShapeColumns->Add( 25, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer121;
	bSizer121 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerRightCol;
	fgSizerRightCol = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSizerRightCol->AddGrowableCol( 1 );
	fgSizerRightCol->SetFlexibleDirection( wxBOTH );
	fgSizerRightCol->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_stLenPercentLabel = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("Best length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentLabel->Wrap( -1 );
	fgSizerRightCol->Add( m_stLenPercentLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_tcLenPercent = new wxTextCtrl( sbAction->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRightCol->Add( m_tcLenPercent, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer131;
	bSizer131 = new wxBoxSizer( wxHORIZONTAL );

	m_lengthUnitsPrefix = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_lengthUnitsPrefix->Wrap( -1 );
	bSizer131->Add( m_lengthUnitsPrefix, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_lengthUnitsHint = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lengthUnitsHint->Wrap( -1 );
	m_lengthUnitsHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer131->Add( m_lengthUnitsHint, 0, wxALIGN_BOTTOM, 1 );

	m_lengthUnitsSuffix = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lengthUnitsSuffix->Wrap( -1 );
	bSizer131->Add( m_lengthUnitsSuffix, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerRightCol->Add( bSizer131, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLen = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("Maximum length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen->Wrap( -1 );
	fgSizerRightCol->Add( m_stMaxLen, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_tcTdMaxLen = new wxTextCtrl( sbAction->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRightCol->Add( m_tcTdMaxLen, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_stMaxLenUnits = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenUnits->Wrap( -1 );
	fgSizerRightCol->Add( m_stMaxLenUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerRightCol->Add( 0, 5, 1, wxEXPAND, 5 );


	fgSizerRightCol->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerRightCol->Add( 0, 0, 1, wxEXPAND, 5 );

	m_stHeightPercentLabel = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("Best width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHeightPercentLabel->Wrap( -1 );
	fgSizerRightCol->Add( m_stHeightPercentLabel, 0, wxALIGN_CENTER_VERTICAL, 10 );

	m_tcHeightPercent = new wxTextCtrl( sbAction->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRightCol->Add( m_tcHeightPercent, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer1311;
	bSizer1311 = new wxBoxSizer( wxHORIZONTAL );

	m_widthUnitsPrefix = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthUnitsPrefix->Wrap( -1 );
	bSizer1311->Add( m_widthUnitsPrefix, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_widthUnitsHint = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthUnitsHint->Wrap( -1 );
	m_widthUnitsHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer1311->Add( m_widthUnitsHint, 0, wxALIGN_BOTTOM, 1 );

	m_widthUnitsSuffix = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthUnitsSuffix->Wrap( -1 );
	bSizer1311->Add( m_widthUnitsSuffix, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerRightCol->Add( bSizer1311, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxHeight = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("Maximum width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxHeight->Wrap( -1 );
	fgSizerRightCol->Add( m_stMaxHeight, 0, wxALIGN_CENTER_VERTICAL, 10 );

	m_tcMaxHeight = new wxTextCtrl( sbAction->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRightCol->Add( m_tcMaxHeight, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_stMaxHeightUnits = new wxStaticText( sbAction->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxHeightUnits->Wrap( -1 );
	fgSizerRightCol->Add( m_stMaxHeightUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizer121->Add( fgSizerRightCol, 1, wxEXPAND|wxLEFT, 10 );


	bSizer121->Add( 0, 8, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer44;
	bSizer44 = new wxBoxSizer( wxHORIZONTAL );

	m_curvedEdges = new wxCheckBox( sbAction->GetStaticBox(), wxID_ANY, _("Curved edges"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	bSizer44->Add( m_curvedEdges, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizer121->Add( bSizer44, 0, wxBOTTOM|wxEXPAND|wxLEFT, 3 );


	bSizerShapeColumns->Add( bSizer121, 1, wxEXPAND|wxTOP, 5 );


	bSizerSpecifiedValues->Add( bSizerShapeColumns, 1, wxEXPAND|wxBOTTOM, 3 );


	sbAction->Add( bSizerSpecifiedValues, 0, wxEXPAND|wxLEFT, 25 );


	bMainSizer->Add( sbAction, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_trackToTrack->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onTrackToTrack ), NULL, this );
	m_netFilterOpt->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_netFilter->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_netclassFilterOpt->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_netclassFilter->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::OnNetclassFilterSelect ), NULL, this );
	m_netclassFilter->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_layerFilterOpt->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_layerFilter->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::OnLayerFilterSelect ), NULL, this );
	m_layerFilter->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_roundPadsFilter->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_existingFilter->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::OnExistingFilterSelect ), NULL, this );
	m_existingFilter->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_selectedItemsFilter->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_boardSetupLink->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onShowBoardSetup ), NULL, this );
	m_cbPreferZoneConnection->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_cbTeardropsUseNextTrack->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stHDRatio->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_tcHDRatio->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stHDRatioUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_minTrackWidthHint->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_bitmapTeardrop->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stLenPercentLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_tcLenPercent->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_lengthUnitsPrefix->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_lengthUnitsHint->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_lengthUnitsSuffix->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stMaxLen->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_tcTdMaxLen->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stMaxLenUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stHeightPercentLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_tcHeightPercent->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_widthUnitsPrefix->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_widthUnitsHint->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_widthUnitsSuffix->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stMaxHeight->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_tcMaxHeight->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stMaxHeightUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_curvedEdges->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
}

DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::~DIALOG_GLOBAL_EDIT_TEARDROPS_BASE()
{
	// Disconnect Events
	m_trackToTrack->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onTrackToTrack ), NULL, this );
	m_netFilterOpt->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_netFilter->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_netclassFilterOpt->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_netclassFilter->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::OnNetclassFilterSelect ), NULL, this );
	m_netclassFilter->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_layerFilterOpt->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_layerFilter->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::OnLayerFilterSelect ), NULL, this );
	m_layerFilter->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_roundPadsFilter->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_existingFilter->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::OnExistingFilterSelect ), NULL, this );
	m_existingFilter->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_selectedItemsFilter->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onFilterUpdateUi ), NULL, this );
	m_boardSetupLink->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onShowBoardSetup ), NULL, this );
	m_cbPreferZoneConnection->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_cbTeardropsUseNextTrack->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stHDRatio->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_tcHDRatio->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stHDRatioUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_minTrackWidthHint->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_bitmapTeardrop->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stLenPercentLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_tcLenPercent->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_lengthUnitsPrefix->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_lengthUnitsHint->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_lengthUnitsSuffix->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stMaxLen->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_tcTdMaxLen->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stMaxLenUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stHeightPercentLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_tcHeightPercent->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_widthUnitsPrefix->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_widthUnitsHint->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_widthUnitsSuffix->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stMaxHeight->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_tcMaxHeight->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_stMaxHeightUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );
	m_curvedEdges->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEARDROPS_BASE::onSpecifiedValuesUpdateUi ), NULL, this );

}

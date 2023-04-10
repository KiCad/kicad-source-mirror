///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/text_ctrl_eval.h"
#include "widgets/wx_grid.h"

#include "dialog_footprint_properties_fp_editor_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );

	m_NoteBook = new wxNotebook( this, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelGeneral = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_PanelPropertiesBoxSizer;
	m_PanelPropertiesBoxSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerTexts;
	sbSizerTexts = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, wxEmptyString ), wxVERTICAL );

	m_itemsGrid = new WX_GRID( sbSizerTexts->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), 0 );

	// Grid
	m_itemsGrid->CreateGrid( 2, 11 );
	m_itemsGrid->EnableEditing( true );
	m_itemsGrid->EnableGridLines( true );
	m_itemsGrid->EnableDragGridSize( false );
	m_itemsGrid->SetMargins( 0, 0 );

	// Columns
	m_itemsGrid->SetColSize( 0, 124 );
	m_itemsGrid->SetColSize( 1, 60 );
	m_itemsGrid->SetColSize( 2, 110 );
	m_itemsGrid->SetColSize( 3, 110 );
	m_itemsGrid->SetColSize( 4, 110 );
	m_itemsGrid->SetColSize( 5, 60 );
	m_itemsGrid->SetColSize( 6, 110 );
	m_itemsGrid->SetColSize( 7, 110 );
	m_itemsGrid->SetColSize( 8, 110 );
	m_itemsGrid->SetColSize( 9, 110 );
	m_itemsGrid->SetColSize( 10, 110 );
	m_itemsGrid->EnableDragColMove( false );
	m_itemsGrid->EnableDragColSize( true );
	m_itemsGrid->SetColLabelValue( 0, _("Text Items") );
	m_itemsGrid->SetColLabelValue( 1, _("Show") );
	m_itemsGrid->SetColLabelValue( 2, _("Width") );
	m_itemsGrid->SetColLabelValue( 3, _("Height") );
	m_itemsGrid->SetColLabelValue( 4, _("Thickness") );
	m_itemsGrid->SetColLabelValue( 5, _("Italic") );
	m_itemsGrid->SetColLabelValue( 6, _("Layer") );
	m_itemsGrid->SetColLabelValue( 7, _("Orientation") );
	m_itemsGrid->SetColLabelValue( 8, _("Unconstrained") );
	m_itemsGrid->SetColLabelValue( 9, _("X Offset") );
	m_itemsGrid->SetColLabelValue( 10, _("Y Offset") );
	m_itemsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_itemsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_itemsGrid->EnableDragRowSize( false );
	m_itemsGrid->SetRowLabelValue( 0, _("Reference designator") );
	m_itemsGrid->SetRowLabelValue( 1, _("Value") );
	m_itemsGrid->SetRowLabelSize( 160 );
	m_itemsGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_itemsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_itemsGrid->SetMinSize( wxSize( 800,140 ) );

	sbSizerTexts->Add( m_itemsGrid, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bButtonSize;
	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new STD_BITMAP_BUTTON( sbSizerTexts->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize->Add( m_bpAdd, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDelete = new STD_BITMAP_BUTTON( sbSizerTexts->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize->Add( m_bpDelete, 0, wxRIGHT, 5 );


	sbSizerTexts->Add( bButtonSize, 0, wxEXPAND, 5 );


	m_PanelPropertiesBoxSizer->Add( sbSizerTexts, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizerFPID;
	fgSizerFPID = new wxFlexGridSizer( 4, 2, 3, 0 );
	fgSizerFPID->AddGrowableCol( 1 );
	fgSizerFPID->SetFlexibleDirection( wxBOTH );
	fgSizerFPID->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* staticFPNameLabel;
	staticFPNameLabel = new wxStaticText( m_PanelGeneral, wxID_ANY, _("Footprint name:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticFPNameLabel->Wrap( -1 );
	fgSizerFPID->Add( staticFPNameLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_FootprintNameCtrl = new wxTextCtrl( m_PanelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerFPID->Add( m_FootprintNameCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticDescriptionLabel;
	staticDescriptionLabel = new wxStaticText( m_PanelGeneral, wxID_ANY, _("Description:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticDescriptionLabel->Wrap( -1 );
	fgSizerFPID->Add( staticDescriptionLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_DocCtrl = new wxTextCtrl( m_PanelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerFPID->Add( m_DocCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	staticKeywordsLabel = new wxStaticText( m_PanelGeneral, wxID_ANY, _("Keywords:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticKeywordsLabel->Wrap( -1 );
	fgSizerFPID->Add( staticKeywordsLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_KeywordCtrl = new wxTextCtrl( m_PanelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerFPID->Add( m_KeywordCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_PanelPropertiesBoxSizer->Add( fgSizerFPID, 0, wxEXPAND|wxBOTTOM, 10 );

	wxBoxSizer* bSizerProperties;
	bSizerProperties = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* bSizerPrivateLayers;
	bSizerPrivateLayers = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Private Layers") ), wxVERTICAL );

	m_privateLayersGrid = new WX_GRID( bSizerPrivateLayers->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), 0 );

	// Grid
	m_privateLayersGrid->CreateGrid( 2, 1 );
	m_privateLayersGrid->EnableEditing( true );
	m_privateLayersGrid->EnableGridLines( true );
	m_privateLayersGrid->EnableDragGridSize( false );
	m_privateLayersGrid->SetMargins( 0, 0 );

	// Columns
	m_privateLayersGrid->SetColSize( 0, 180 );
	m_privateLayersGrid->EnableDragColMove( false );
	m_privateLayersGrid->EnableDragColSize( true );
	m_privateLayersGrid->SetColLabelSize( 0 );
	m_privateLayersGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_privateLayersGrid->EnableDragRowSize( false );
	m_privateLayersGrid->SetRowLabelSize( 0 );
	m_privateLayersGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_privateLayersGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerPrivateLayers->Add( m_privateLayersGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bButtonSize1;
	bButtonSize1 = new wxBoxSizer( wxHORIZONTAL );

	m_bpAddLayer = new STD_BITMAP_BUTTON( bSizerPrivateLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize1->Add( m_bpAddLayer, 0, wxRIGHT|wxLEFT, 5 );


	bButtonSize1->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDeleteLayer = new STD_BITMAP_BUTTON( bSizerPrivateLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize1->Add( m_bpDeleteLayer, 0, wxRIGHT, 5 );


	bSizerPrivateLayers->Add( bButtonSize1, 0, wxEXPAND, 5 );


	bSizerProperties->Add( bSizerPrivateLayers, 1, wxEXPAND|wxRIGHT, 15 );

	wxStaticBoxSizer* sbFabSizer;
	sbFabSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Fabrication Attributes") ), wxVERTICAL );

	wxBoxSizer* bPartTypeSizer;
	bPartTypeSizer = new wxBoxSizer( wxHORIZONTAL );

	m_componentTypeLabel = new wxStaticText( sbFabSizer->GetStaticBox(), wxID_ANY, _("Component type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_componentTypeLabel->Wrap( -1 );
	bPartTypeSizer->Add( m_componentTypeLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_componentTypeChoices[] = { _("Through hole"), _("SMD"), _("Unspecified") };
	int m_componentTypeNChoices = sizeof( m_componentTypeChoices ) / sizeof( wxString );
	m_componentType = new wxChoice( sbFabSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_componentTypeNChoices, m_componentTypeChoices, 0 );
	m_componentType->SetSelection( 0 );
	bPartTypeSizer->Add( m_componentType, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbFabSizer->Add( bPartTypeSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	m_boardOnly = new wxCheckBox( sbFabSizer->GetStaticBox(), wxID_ANY, _("Not in schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFabSizer->Add( m_boardOnly, 0, wxALL, 5 );

	m_excludeFromPosFiles = new wxCheckBox( sbFabSizer->GetStaticBox(), wxID_ANY, _("Exclude from position files"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFabSizer->Add( m_excludeFromPosFiles, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_excludeFromBOM = new wxCheckBox( sbFabSizer->GetStaticBox(), wxID_ANY, _("Exclude from bill of materials"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFabSizer->Add( m_excludeFromBOM, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_noCourtyards = new wxCheckBox( sbFabSizer->GetStaticBox(), wxID_ANY, _("Exempt from courtyard requirement"), wxDefaultPosition, wxDefaultSize, 0 );
	m_noCourtyards->SetToolTip( _("Will not generate \"missing courtyard\" DRC violations") );

	sbFabSizer->Add( m_noCourtyards, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbDNP = new wxCheckBox( sbFabSizer->GetStaticBox(), wxID_ANY, _("Do not populate"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFabSizer->Add( m_cbDNP, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerProperties->Add( sbFabSizer, 1, wxEXPAND|wxRIGHT, 5 );


	m_PanelPropertiesBoxSizer->Add( bSizerProperties, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	m_PanelGeneral->SetSizer( m_PanelPropertiesBoxSizer );
	m_PanelGeneral->Layout();
	m_PanelPropertiesBoxSizer->Fit( m_PanelGeneral );
	m_NoteBook->AddPage( m_PanelGeneral, _("General"), true );
	m_PanelClearances = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanelClearances;
	bSizerPanelClearances = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerLocalProperties;
	sbSizerLocalProperties = new wxStaticBoxSizer( new wxStaticBox( m_PanelClearances, wxID_ANY, _("Clearances") ), wxVERTICAL );

	m_staticTextInfo = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Set values to 0 to use netclass values."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfo, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 4, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,15 ) );

	m_NetClearanceLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Pad clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceLabel->Wrap( -1 );
	m_NetClearanceLabel->SetToolTip( _("This is the local net clearance for all pads of this footprint.\nIf 0, the Netclass values are used.\nThis value can be overridden on a pad-by-pad basis in the Local\nClearance and Settings tab of Pad Properties.") );

	gbSizer1->Add( m_NetClearanceLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_NetClearanceCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_NetClearanceCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_NetClearanceUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceUnits->Wrap( -1 );
	gbSizer1->Add( m_NetClearanceUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SolderMaskMarginLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder mask expansion:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginLabel->Wrap( -1 );
	m_SolderMaskMarginLabel->SetToolTip( _("This is the local clearance between pads and the solder mask for \nthis footprint.\nIf 0, the global value is used.\nThis value can be overridden on a pad-by-pad basis in the Local\nClearance and Settings tab of Pad Properties.") );

	gbSizer1->Add( m_SolderMaskMarginLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SolderMaskMarginCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_SolderMaskMarginCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_SolderMaskMarginUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginUnits->Wrap( -1 );
	gbSizer1->Add( m_SolderMaskMarginUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_allowBridges = new wxCheckBox( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Allow bridged solder mask apertures between pads"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_allowBridges, wxGBPosition( 3, 0 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SolderPasteMarginLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder paste absolute clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginLabel->Wrap( -1 );
	m_SolderPasteMarginLabel->SetToolTip( _("This is the local clearance between pads and the solder paste for\nthis footprint.\nThe final clearance value is the sum of this value and the clearance value ratio.\nA negative value means a smaller stencil aperture size than pad size.\nThis value can be overridden on a pad-by-pad basis in the Local\nClearance and Settings tab of Pad Properties.") );

	gbSizer1->Add( m_SolderPasteMarginLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SolderPasteMarginCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_SolderPasteMarginCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_SolderPasteMarginUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	gbSizer1->Add( m_SolderPasteMarginUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_PasteMarginRatioLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginRatioLabel->Wrap( -1 );
	m_PasteMarginRatioLabel->SetToolTip( _("This is the local clearance ratio applied as a percentage of the pad width and height for this footprint.\nA value of 10 means the horizontal clearance value is 10% of the pad’s width, and the vertical clearance value is 10% of the pad’s height.\nThe final clearance value is the sum of this value and the absolute clearance value.\nA negative value means a smaller stencil aperture size than pad size.\nThis value can be overridden on a pad-by-pad basis in the Local Clearance and Settings tab of Pad Properties.") );

	gbSizer1->Add( m_PasteMarginRatioLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_PasteMarginRatioCtrl = new TEXT_CTRL_EVAL( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_PasteMarginRatioCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_PasteMarginRatioUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginRatioUnits->Wrap( -1 );
	gbSizer1->Add( m_PasteMarginRatioUnits, wxGBPosition( 6, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbSizerLocalProperties->Add( gbSizer1, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	wxFlexGridSizer* fgSizerClearances;
	fgSizerClearances = new wxFlexGridSizer( 5, 3, 0, 0 );
	fgSizerClearances->AddGrowableCol( 1 );
	fgSizerClearances->SetFlexibleDirection( wxBOTH );
	fgSizerClearances->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	sbSizerLocalProperties->Add( fgSizerClearances, 1, wxEXPAND, 5 );

	m_staticTextInfoCopper = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Note: solder mask and paste values are used only for pads on copper layers."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoCopper->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfoCopper, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextInfoPaste = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Note: solder paste clearances (absolute and relative) are added to determine the final clearance."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPaste->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfoPaste, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerPanelClearances->Add( sbSizerLocalProperties, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSizerCopperZones;
	sbSizerCopperZones = new wxStaticBoxSizer( new wxStaticBox( m_PanelClearances, wxID_ANY, _("Connection to Copper Zones") ), wxHORIZONTAL );

	m_staticText16 = new wxStaticText( sbSizerCopperZones->GetStaticBox(), wxID_ANY, _("Pad connection to zones:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	sbSizerCopperZones->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxString m_ZoneConnectionChoiceChoices[] = { _("Use zone setting"), _("Solid"), _("Thermal relief"), _("None") };
	int m_ZoneConnectionChoiceNChoices = sizeof( m_ZoneConnectionChoiceChoices ) / sizeof( wxString );
	m_ZoneConnectionChoice = new wxChoice( sbSizerCopperZones->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneConnectionChoiceNChoices, m_ZoneConnectionChoiceChoices, 0 );
	m_ZoneConnectionChoice->SetSelection( 0 );
	sbSizerCopperZones->Add( m_ZoneConnectionChoice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerPanelClearances->Add( sbSizerCopperZones, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerNetTies;
	sbSizerNetTies = new wxStaticBoxSizer( new wxStaticBox( m_PanelClearances, wxID_ANY, _("Net Ties") ), wxVERTICAL );

	m_padGroupsLabel = new wxStaticText( sbSizerNetTies->GetStaticBox(), wxID_ANY, _("Pad groups allowed to short different nets:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padGroupsLabel->Wrap( -1 );
	sbSizerNetTies->Add( m_padGroupsLabel, 0, wxRIGHT|wxLEFT, 5 );

	m_padGroupsGrid = new WX_GRID( sbSizerNetTies->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_padGroupsGrid->CreateGrid( 0, 1 );
	m_padGroupsGrid->EnableEditing( true );
	m_padGroupsGrid->EnableGridLines( true );
	m_padGroupsGrid->EnableDragGridSize( false );
	m_padGroupsGrid->SetMargins( 0, 0 );

	// Columns
	m_padGroupsGrid->SetColSize( 0, 320 );
	m_padGroupsGrid->EnableDragColMove( false );
	m_padGroupsGrid->EnableDragColSize( true );
	m_padGroupsGrid->SetColLabelSize( 0 );
	m_padGroupsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_padGroupsGrid->EnableDragRowSize( true );
	m_padGroupsGrid->SetRowLabelSize( 0 );
	m_padGroupsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_padGroupsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_padGroupsGrid->SetMinSize( wxSize( -1,30 ) );

	sbSizerNetTies->Add( m_padGroupsGrid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bButtonSize2;
	bButtonSize2 = new wxBoxSizer( wxHORIZONTAL );

	m_bpAddPadGroup = new STD_BITMAP_BUTTON( sbSizerNetTies->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize2->Add( m_bpAddPadGroup, 0, wxRIGHT|wxLEFT, 5 );


	bButtonSize2->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpRemovePadGroup = new STD_BITMAP_BUTTON( sbSizerNetTies->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize2->Add( m_bpRemovePadGroup, 0, wxRIGHT, 5 );


	sbSizerNetTies->Add( bButtonSize2, 0, wxEXPAND, 2 );


	bSizerPanelClearances->Add( sbSizerNetTies, 1, wxEXPAND|wxALL, 5 );


	m_PanelClearances->SetSizer( bSizerPanelClearances );
	m_PanelClearances->Layout();
	bSizerPanelClearances->Fit( m_PanelClearances );
	m_NoteBook->AddPage( m_PanelClearances, _("Clearance Overrides and Settings"), false );

	m_GeneralBoxSizer->Add( m_NoteBook, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );

	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();

	bSizer16->Add( m_sdbSizerStdButtons, 1, wxEXPAND|wxALL, 5 );


	m_GeneralBoxSizer->Add( bSizer16, 0, wxEXPAND|wxTOP, 5 );


	this->SetSizer( m_GeneralBoxSizer );
	this->Layout();
	m_GeneralBoxSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnInitDlg ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnUpdateUI ) );
	m_itemsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnGridSize ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddField ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnDeleteField ), NULL, this );
	m_FootprintNameCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnFootprintNameText ), NULL, this );
	m_privateLayersGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnGridSize ), NULL, this );
	m_bpAddLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddLayer ), NULL, this );
	m_bpDeleteLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnDeleteLayer ), NULL, this );
	m_padGroupsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnGridSize ), NULL, this );
	m_bpAddPadGroup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddPadGroup ), NULL, this );
	m_bpRemovePadGroup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnRemovePadGroup ), NULL, this );
}

DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::~DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnInitDlg ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnUpdateUI ) );
	m_itemsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnGridSize ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddField ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnDeleteField ), NULL, this );
	m_FootprintNameCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnFootprintNameText ), NULL, this );
	m_privateLayersGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnGridSize ), NULL, this );
	m_bpAddLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddLayer ), NULL, this );
	m_bpDeleteLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnDeleteLayer ), NULL, this );
	m_padGroupsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnGridSize ), NULL, this );
	m_bpAddPadGroup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddPadGroup ), NULL, this );
	m_bpRemovePadGroup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnRemovePadGroup ), NULL, this );

}

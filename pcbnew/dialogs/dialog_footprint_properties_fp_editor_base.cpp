///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
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
	sbSizerTexts = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Fields") ), wxVERTICAL );

	m_itemsGrid = new WX_GRID( sbSizerTexts->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), 0 );

	// Grid
	m_itemsGrid->CreateGrid( 2, 11 );
	m_itemsGrid->EnableEditing( true );
	m_itemsGrid->EnableGridLines( true );
	m_itemsGrid->EnableDragGridSize( false );
	m_itemsGrid->SetMargins( 0, 0 );

	// Columns
	m_itemsGrid->SetColSize( 0, 84 );
	m_itemsGrid->SetColSize( 1, 48 );
	m_itemsGrid->SetColSize( 2, 84 );
	m_itemsGrid->SetColSize( 3, 84 );
	m_itemsGrid->SetColSize( 4, 84 );
	m_itemsGrid->SetColSize( 5, 60 );
	m_itemsGrid->SetColSize( 6, 84 );
	m_itemsGrid->SetColSize( 7, 84 );
	m_itemsGrid->SetColSize( 8, 48 );
	m_itemsGrid->SetColSize( 9, 84 );
	m_itemsGrid->SetColSize( 10, 84 );
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
	m_itemsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
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

	wxBoxSizer* bColumns;
	bColumns = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbMetadataSizer;
	sbMetadataSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Metadata") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerFPID;
	fgSizerFPID = new wxFlexGridSizer( 4, 2, 3, 0 );
	fgSizerFPID->AddGrowableCol( 1 );
	fgSizerFPID->SetFlexibleDirection( wxBOTH );
	fgSizerFPID->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* staticFPNameLabel;
	staticFPNameLabel = new wxStaticText( sbMetadataSizer->GetStaticBox(), wxID_ANY, _("Footprint name:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticFPNameLabel->Wrap( -1 );
	fgSizerFPID->Add( staticFPNameLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_FootprintNameCtrl = new wxTextCtrl( sbMetadataSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerFPID->Add( m_FootprintNameCtrl, 0, wxEXPAND|wxLEFT, 5 );

	wxStaticText* staticDescriptionLabel;
	staticDescriptionLabel = new wxStaticText( sbMetadataSizer->GetStaticBox(), wxID_ANY, _("Description:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticDescriptionLabel->Wrap( -1 );
	fgSizerFPID->Add( staticDescriptionLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_TOP, 5 );

	m_DocCtrl = new wxTextCtrl( sbMetadataSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerFPID->Add( m_DocCtrl, 0, wxEXPAND|wxLEFT, 5 );

	staticKeywordsLabel = new wxStaticText( sbMetadataSizer->GetStaticBox(), wxID_ANY, _("Keywords:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticKeywordsLabel->Wrap( -1 );
	fgSizerFPID->Add( staticKeywordsLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_KeywordCtrl = new wxTextCtrl( sbMetadataSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerFPID->Add( m_KeywordCtrl, 0, wxEXPAND|wxLEFT, 5 );


	sbMetadataSizer->Add( fgSizerFPID, 1, wxEXPAND, 10 );


	bColumns->Add( sbMetadataSizer, 1, wxEXPAND|wxALL, 5 );


	bColumns->Add( 5, 0, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbAttributesSizer;
	sbAttributesSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Fabrication Attributes") ), wxVERTICAL );

	wxBoxSizer* bPartTypeSizer;
	bPartTypeSizer = new wxBoxSizer( wxHORIZONTAL );

	m_componentTypeLabel = new wxStaticText( sbAttributesSizer->GetStaticBox(), wxID_ANY, _("Component type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_componentTypeLabel->Wrap( -1 );
	bPartTypeSizer->Add( m_componentTypeLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_componentTypeChoices[] = { _("Through hole"), _("SMD"), _("Unspecified") };
	int m_componentTypeNChoices = sizeof( m_componentTypeChoices ) / sizeof( wxString );
	m_componentType = new wxChoice( sbAttributesSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_componentTypeNChoices, m_componentTypeChoices, 0 );
	m_componentType->SetSelection( 0 );
	bPartTypeSizer->Add( m_componentType, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	sbAttributesSizer->Add( bPartTypeSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	m_boardOnly = new wxCheckBox( sbAttributesSizer->GetStaticBox(), wxID_ANY, _("Not in schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAttributesSizer->Add( m_boardOnly, 0, wxALL, 5 );

	m_excludeFromPosFiles = new wxCheckBox( sbAttributesSizer->GetStaticBox(), wxID_ANY, _("Exclude from position files"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAttributesSizer->Add( m_excludeFromPosFiles, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_excludeFromBOM = new wxCheckBox( sbAttributesSizer->GetStaticBox(), wxID_ANY, _("Exclude from bill of materials"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAttributesSizer->Add( m_excludeFromBOM, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbDNP = new wxCheckBox( sbAttributesSizer->GetStaticBox(), wxID_ANY, _("Do not populate"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAttributesSizer->Add( m_cbDNP, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bColumns->Add( sbAttributesSizer, 0, wxEXPAND|wxALL, 5 );


	m_PanelPropertiesBoxSizer->Add( bColumns, 0, wxEXPAND, 5 );


	m_PanelGeneral->SetSizer( m_PanelPropertiesBoxSizer );
	m_PanelGeneral->Layout();
	m_PanelPropertiesBoxSizer->Fit( m_PanelGeneral );
	m_NoteBook->AddPage( m_PanelGeneral, _("General"), true );
	m_LayersPanel = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer11;
	sbSizer11 = new wxStaticBoxSizer( new wxStaticBox( m_LayersPanel, wxID_ANY, _("Custom Layers") ), wxVERTICAL );

	wxGridBagSizer* gbSizer3;
	gbSizer3 = new wxGridBagSizer( 0, 0 );
	gbSizer3->SetFlexibleDirection( wxBOTH );
	gbSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbCustomLayers = new wxCheckBox( sbSizer11->GetStaticBox(), wxID_ANY, _("Use custom stackup"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer3->Add( m_cbCustomLayers, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxALL|wxEXPAND, 5 );

	m_copperLayerCountLabel = new wxStaticText( sbSizer11->GetStaticBox(), wxID_ANY, _("Copper layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_copperLayerCountLabel->Wrap( -1 );
	gbSizer3->Add( m_copperLayerCountLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_copperLayerCountChoices[] = { _("2"), _("4"), _("6"), _("8"), _("10"), _("12"), _("14"), _("16"), _("18"), _("20"), _("22"), _("24"), _("26"), _("28"), _("30"), _("32"), wxEmptyString };
	int m_copperLayerCountNChoices = sizeof( m_copperLayerCountChoices ) / sizeof( wxString );
	m_copperLayerCount = new wxChoice( sbSizer11->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_copperLayerCountNChoices, m_copperLayerCountChoices, 0 );
	m_copperLayerCount->SetSelection( 0 );
	m_copperLayerCount->Enable( false );

	gbSizer3->Add( m_copperLayerCount, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	wxStaticBoxSizer* sbUserCustomerLayers;
	sbUserCustomerLayers = new wxStaticBoxSizer( new wxStaticBox( sbSizer11->GetStaticBox(), wxID_ANY, _("User Layers") ), wxVERTICAL );

	m_customUserLayersGrid = new WX_GRID( sbUserCustomerLayers->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), 0 );

	// Grid
	m_customUserLayersGrid->CreateGrid( 2, 1 );
	m_customUserLayersGrid->EnableEditing( true );
	m_customUserLayersGrid->EnableGridLines( true );
	m_customUserLayersGrid->EnableDragGridSize( false );
	m_customUserLayersGrid->SetMargins( 0, 0 );

	// Columns
	m_customUserLayersGrid->SetColSize( 0, 180 );
	m_customUserLayersGrid->EnableDragColMove( false );
	m_customUserLayersGrid->EnableDragColSize( true );
	m_customUserLayersGrid->SetColLabelSize( 0 );
	m_customUserLayersGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_customUserLayersGrid->EnableDragRowSize( false );
	m_customUserLayersGrid->SetRowLabelSize( 0 );
	m_customUserLayersGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_customUserLayersGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	sbUserCustomerLayers->Add( m_customUserLayersGrid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bButtonSize11;
	bButtonSize11 = new wxBoxSizer( wxHORIZONTAL );

	m_bpAddCustomLayer = new STD_BITMAP_BUTTON( sbUserCustomerLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize11->Add( m_bpAddCustomLayer, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bButtonSize11->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDeleteCustomLayer = new STD_BITMAP_BUTTON( sbUserCustomerLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize11->Add( m_bpDeleteCustomLayer, 0, wxBOTTOM|wxRIGHT, 5 );


	sbUserCustomerLayers->Add( bButtonSize11, 0, wxEXPAND, 5 );


	gbSizer3->Add( sbUserCustomerLayers, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxALL|wxEXPAND, 5 );


	gbSizer3->AddGrowableCol( 1 );
	gbSizer3->AddGrowableRow( 2 );

	sbSizer11->Add( gbSizer3, 1, wxEXPAND, 5 );


	bSizer14->Add( sbSizer11, 1, wxEXPAND|wxRIGHT|wxTOP, 5 );

	wxStaticBoxSizer* bSizerPrivateLayers;
	bSizerPrivateLayers = new wxStaticBoxSizer( new wxStaticBox( m_LayersPanel, wxID_ANY, _("Private Layers") ), wxVERTICAL );

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
	m_privateLayersGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizerPrivateLayers->Add( m_privateLayersGrid, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bPrivateLayerButtonsSizer;
	bPrivateLayerButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_bpAddPrivateLayer = new STD_BITMAP_BUTTON( bSizerPrivateLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bPrivateLayerButtonsSizer->Add( m_bpAddPrivateLayer, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bPrivateLayerButtonsSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDeletePrivateLayer = new STD_BITMAP_BUTTON( bSizerPrivateLayers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bPrivateLayerButtonsSizer->Add( m_bpDeletePrivateLayer, 0, wxBOTTOM|wxRIGHT, 5 );


	bSizerPrivateLayers->Add( bPrivateLayerButtonsSizer, 0, wxEXPAND, 5 );


	bSizer14->Add( bSizerPrivateLayers, 1, wxEXPAND|wxLEFT|wxTOP, 5 );


	m_LayersPanel->SetSizer( bSizer14 );
	m_LayersPanel->Layout();
	bSizer14->Fit( m_LayersPanel );
	m_NoteBook->AddPage( m_LayersPanel, _("Layers"), false );
	m_PanelClearances = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanelClearances;
	bSizerPanelClearances = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerLocalProperties;
	sbSizerLocalProperties = new wxStaticBoxSizer( new wxStaticBox( m_PanelClearances, wxID_ANY, _("Clearances") ), wxVERTICAL );

	m_staticTextInfo = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Leave values blank to use netclass values."), wxDefaultPosition, wxDefaultSize, 0 );
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

	m_NetClearanceCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
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

	m_SolderPasteMarginLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder paste clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginLabel->Wrap( -1 );
	m_SolderPasteMarginLabel->SetToolTip( _("Solder paste clearance relative to pad size.\nEnter an absolute value (e.g., -0.1mm), a percentage (e.g., -5%), or both (e.g., -0.1mm - 5%).\nThis value can be superseded by local values for a footprint or a pad.") );

	gbSizer1->Add( m_SolderPasteMarginLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SolderPasteMarginCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginCtrl->SetToolTip( _("Local solder paste clearance for this footprint.\nEnter an absolute value (e.g., -0.1mm), a percentage (e.g., -5%), or both (e.g., -0.1mm - 5%).\nIf blank, the global value is used.") );

	gbSizer1->Add( m_SolderPasteMarginCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_SolderPasteMarginUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	gbSizer1->Add( m_SolderPasteMarginUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_PasteMarginRatioLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginRatioLabel->Wrap( -1 );
	m_PasteMarginRatioLabel->SetToolTip( _("This is the local clearance ratio applied as a percentage of the pad width and height for this footprint.\nA value of 10 means the horizontal clearance value is 10% of the pad’s width, and the vertical clearance value is 10% of the pad’s height.\nThe final clearance value is the sum of this value and the absolute clearance value.\nA negative value means a smaller stencil aperture size than pad size.\nThis value can be overridden on a pad-by-pad basis in the Local Clearance and Settings tab of Pad Properties.") );

	gbSizer1->Add( m_PasteMarginRatioLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_PasteMarginRatioCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
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

	m_staticTextInfoPaste = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPaste->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfoPaste, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerPanelClearances->Add( sbSizerLocalProperties, 0, wxEXPAND|wxALL, 5 );


	bSizerPanelClearances->Add( 0, 5, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerCourtyards;
	sbSizerCourtyards = new wxStaticBoxSizer( new wxStaticBox( m_PanelClearances, wxID_ANY, _("Courtyards") ), wxVERTICAL );

	m_noCourtyards = new wxCheckBox( sbSizerCourtyards->GetStaticBox(), wxID_ANY, _("Exempt from courtyard requirement"), wxDefaultPosition, wxDefaultSize, 0 );
	m_noCourtyards->SetToolTip( _("Will not generate \"missing courtyard\" DRC violations") );

	sbSizerCourtyards->Add( m_noCourtyards, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerPanelClearances->Add( sbSizerCourtyards, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_PanelClearances->SetSizer( bSizerPanelClearances );
	m_PanelClearances->Layout();
	bSizerPanelClearances->Fit( m_PanelClearances );
	m_NoteBook->AddPage( m_PanelClearances, _("Clearance Overrides"), false );
	m_PanelPadConnections = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerCopperZones;
	sbSizerCopperZones = new wxStaticBoxSizer( new wxStaticBox( m_PanelPadConnections, wxID_ANY, _("Connection to Copper Zones") ), wxHORIZONTAL );

	m_staticText16 = new wxStaticText( sbSizerCopperZones->GetStaticBox(), wxID_ANY, _("Pad connection to zones:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	sbSizerCopperZones->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxString m_ZoneConnectionChoiceChoices[] = { _("Use zone setting"), _("Solid"), _("Thermal relief"), _("None") };
	int m_ZoneConnectionChoiceNChoices = sizeof( m_ZoneConnectionChoiceChoices ) / sizeof( wxString );
	m_ZoneConnectionChoice = new wxChoice( sbSizerCopperZones->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneConnectionChoiceNChoices, m_ZoneConnectionChoiceChoices, 0 );
	m_ZoneConnectionChoice->SetSelection( 0 );
	sbSizerCopperZones->Add( m_ZoneConnectionChoice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizer19->Add( sbSizerCopperZones, 0, wxALL|wxEXPAND, 5 );


	bSizer19->Add( 0, 5, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerNetTies;
	sbSizerNetTies = new wxStaticBoxSizer( new wxStaticBox( m_PanelPadConnections, wxID_ANY, _("Net Ties") ), wxVERTICAL );

	m_nettieGroupsLabel = new wxStaticText( sbSizerNetTies->GetStaticBox(), wxID_ANY, _("Pad groups allowed to short different nets:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nettieGroupsLabel->Wrap( -1 );
	sbSizerNetTies->Add( m_nettieGroupsLabel, 0, wxRIGHT|wxLEFT, 5 );

	m_nettieGroupsGrid = new WX_GRID( sbSizerNetTies->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_nettieGroupsGrid->CreateGrid( 0, 1 );
	m_nettieGroupsGrid->EnableEditing( true );
	m_nettieGroupsGrid->EnableGridLines( true );
	m_nettieGroupsGrid->EnableDragGridSize( false );
	m_nettieGroupsGrid->SetMargins( 0, 0 );

	// Columns
	m_nettieGroupsGrid->SetColSize( 0, 320 );
	m_nettieGroupsGrid->EnableDragColMove( false );
	m_nettieGroupsGrid->EnableDragColSize( true );
	m_nettieGroupsGrid->SetColLabelSize( 0 );
	m_nettieGroupsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_nettieGroupsGrid->EnableDragRowSize( true );
	m_nettieGroupsGrid->SetRowLabelSize( 0 );
	m_nettieGroupsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_nettieGroupsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_nettieGroupsGrid->SetMinSize( wxSize( -1,30 ) );

	sbSizerNetTies->Add( m_nettieGroupsGrid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bButtonSize2;
	bButtonSize2 = new wxBoxSizer( wxHORIZONTAL );

	m_bpAddNettieGroup = new STD_BITMAP_BUTTON( sbSizerNetTies->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize2->Add( m_bpAddNettieGroup, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bButtonSize2->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpRemoveNettieGroup = new STD_BITMAP_BUTTON( sbSizerNetTies->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize2->Add( m_bpRemoveNettieGroup, 0, wxBOTTOM|wxRIGHT, 5 );


	sbSizerNetTies->Add( bButtonSize2, 0, wxEXPAND, 2 );


	bSizer19->Add( sbSizerNetTies, 1, wxALL|wxEXPAND, 5 );


	bSizer19->Add( 0, 5, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbJumpers;
	sbJumpers = new wxStaticBoxSizer( new wxStaticBox( m_PanelPadConnections, wxID_ANY, _("Jumpers") ), wxVERTICAL );

	m_cbDuplicatePadsAreJumpers = new wxCheckBox( sbJumpers->GetStaticBox(), wxID_ANY, _("All pads with duplicate numbers are jumpers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbDuplicatePadsAreJumpers->SetToolTip( _("When enabled, this footprint can have more than one pad with the same number, and pads with the same number will be considered to be jumpered together internally.") );

	sbJumpers->Add( m_cbDuplicatePadsAreJumpers, 0, wxALL, 5 );


	sbJumpers->Add( 0, 5, 0, wxEXPAND, 5 );

	m_jumperGroupsLabel = new wxStaticText( sbJumpers->GetStaticBox(), wxID_ANY, _("Explicit jumper pad groups:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_jumperGroupsLabel->Wrap( -1 );
	sbJumpers->Add( m_jumperGroupsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_jumperGroupsGrid = new WX_GRID( sbJumpers->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_jumperGroupsGrid->CreateGrid( 0, 1 );
	m_jumperGroupsGrid->EnableEditing( true );
	m_jumperGroupsGrid->EnableGridLines( true );
	m_jumperGroupsGrid->EnableDragGridSize( false );
	m_jumperGroupsGrid->SetMargins( 0, 0 );

	// Columns
	m_jumperGroupsGrid->SetColSize( 0, 320 );
	m_jumperGroupsGrid->EnableDragColMove( false );
	m_jumperGroupsGrid->EnableDragColSize( true );
	m_jumperGroupsGrid->SetColLabelSize( 0 );
	m_jumperGroupsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_jumperGroupsGrid->EnableDragRowSize( true );
	m_jumperGroupsGrid->SetRowLabelSize( 0 );
	m_jumperGroupsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_jumperGroupsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_jumperGroupsGrid->SetMinSize( wxSize( -1,30 ) );

	sbJumpers->Add( m_jumperGroupsGrid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bButtonSize21;
	bButtonSize21 = new wxBoxSizer( wxHORIZONTAL );

	m_bpAddJumperGroup = new STD_BITMAP_BUTTON( sbJumpers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize21->Add( m_bpAddJumperGroup, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bButtonSize21->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpRemoveJumperGroup = new STD_BITMAP_BUTTON( sbJumpers->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize21->Add( m_bpRemoveJumperGroup, 0, wxBOTTOM|wxRIGHT, 5 );


	sbJumpers->Add( bButtonSize21, 0, wxEXPAND, 5 );


	bSizer19->Add( sbJumpers, 1, wxALL|wxTOP|wxEXPAND, 5 );


	m_PanelPadConnections->SetSizer( bSizer19 );
	m_PanelPadConnections->Layout();
	bSizer19->Fit( m_PanelPadConnections );
	m_NoteBook->AddPage( m_PanelPadConnections, _("Pad Connections"), false );

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
	m_NoteBook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, wxNotebookEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnPageChanging ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddField ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnDeleteField ), NULL, this );
	m_FootprintNameCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnText ), NULL, this );
	m_DocCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnText ), NULL, this );
	m_KeywordCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnText ), NULL, this );
	m_componentType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnChoice ), NULL, this );
	m_boardOnly->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnCheckBox ), NULL, this );
	m_excludeFromPosFiles->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnCheckBox ), NULL, this );
	m_excludeFromBOM->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnCheckBox ), NULL, this );
	m_cbDNP->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnCheckBox ), NULL, this );
	m_cbCustomLayers->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnUseCustomLayers ), NULL, this );
	m_bpAddCustomLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddCustomLayer ), NULL, this );
	m_bpDeleteCustomLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnDeleteCustomLayer ), NULL, this );
	m_bpAddPrivateLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddPrivateLayer ), NULL, this );
	m_bpDeletePrivateLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnDeletePrivateLayer ), NULL, this );
	m_noCourtyards->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnCheckBox ), NULL, this );
	m_bpAddNettieGroup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddNettieGroup ), NULL, this );
	m_bpRemoveNettieGroup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnRemoveNettieGroup ), NULL, this );
	m_bpAddJumperGroup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddJumperGroup ), NULL, this );
	m_bpRemoveJumperGroup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnRemoveJumperGroup ), NULL, this );
}

DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::~DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnInitDlg ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnUpdateUI ) );
	m_NoteBook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, wxNotebookEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnPageChanging ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddField ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnDeleteField ), NULL, this );
	m_FootprintNameCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnText ), NULL, this );
	m_DocCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnText ), NULL, this );
	m_KeywordCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnText ), NULL, this );
	m_componentType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnChoice ), NULL, this );
	m_boardOnly->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnCheckBox ), NULL, this );
	m_excludeFromPosFiles->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnCheckBox ), NULL, this );
	m_excludeFromBOM->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnCheckBox ), NULL, this );
	m_cbDNP->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnCheckBox ), NULL, this );
	m_cbCustomLayers->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnUseCustomLayers ), NULL, this );
	m_bpAddCustomLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddCustomLayer ), NULL, this );
	m_bpDeleteCustomLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnDeleteCustomLayer ), NULL, this );
	m_bpAddPrivateLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddPrivateLayer ), NULL, this );
	m_bpDeletePrivateLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnDeletePrivateLayer ), NULL, this );
	m_noCourtyards->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnCheckBox ), NULL, this );
	m_bpAddNettieGroup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddNettieGroup ), NULL, this );
	m_bpRemoveNettieGroup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnRemoveNettieGroup ), NULL, this );
	m_bpAddJumperGroup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnAddJumperGroup ), NULL, this );
	m_bpRemoveJumperGroup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE::OnRemoveJumperGroup ), NULL, this );

}

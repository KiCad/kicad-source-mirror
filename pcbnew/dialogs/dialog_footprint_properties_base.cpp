///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/text_ctrl_eval.h"
#include "widgets/wx_grid.h"

#include "dialog_footprint_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FOOTPRINT_PROPERTIES_BASE::DIALOG_FOOTPRINT_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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
	m_itemsGrid->SetColSize( 6, 140 );
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
	m_itemsGrid->SetColLabelValue( 8, _("Keep Upright") );
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

	sbSizerTexts->Add( m_itemsGrid, 1, wxALL|wxBOTTOM|wxEXPAND, 5 );

	wxBoxSizer* bButtonSize;
	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new STD_BITMAP_BUTTON( sbSizerTexts->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize->Add( m_bpAdd, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDelete = new STD_BITMAP_BUTTON( sbSizerTexts->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize->Add( m_bpDelete, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	sbSizerTexts->Add( bButtonSize, 0, wxEXPAND, 5 );


	m_PanelPropertiesBoxSizer->Add( sbSizerTexts, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerProperties;
	bSizerProperties = new wxBoxSizer( wxHORIZONTAL );

	bSizerLeft = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer7;
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Position") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerPos;
	fgSizerPos = new wxFlexGridSizer( 4, 3, 3, 0 );
	fgSizerPos->AddGrowableCol( 1 );
	fgSizerPos->AddGrowableRow( 2 );
	fgSizerPos->SetFlexibleDirection( wxBOTH );
	fgSizerPos->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_XPosLabel = new wxStaticText( sbSizer7->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_XPosLabel->Wrap( -1 );
	fgSizerPos->Add( m_XPosLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ModPositionX = new wxTextCtrl( sbSizer7->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPos->Add( m_ModPositionX, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_XPosUnit = new wxStaticText( sbSizer7->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_XPosUnit->Wrap( -1 );
	fgSizerPos->Add( m_XPosUnit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_YPosLabel = new wxStaticText( sbSizer7->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_YPosLabel->Wrap( -1 );
	fgSizerPos->Add( m_YPosLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ModPositionY = new wxTextCtrl( sbSizer7->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPos->Add( m_ModPositionY, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, 1 );

	m_YPosUnit = new wxStaticText( sbSizer7->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_YPosUnit->Wrap( -1 );
	fgSizerPos->Add( m_YPosUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_orientationLabel = new wxStaticText( sbSizer7->GetStaticBox(), wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_orientationLabel->Wrap( -1 );
	fgSizerPos->Add( m_orientationLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_orientationCtrl = new wxComboBox( sbSizer7->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_orientationCtrl->Append( _("0") );
	m_orientationCtrl->Append( _("90") );
	m_orientationCtrl->Append( _("-90") );
	m_orientationCtrl->Append( _("180") );
	fgSizerPos->Add( m_orientationCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerPos->Add( 0, 0, 1, wxEXPAND, 5 );

	m_BoardSideLabel = new wxStaticText( sbSizer7->GetStaticBox(), wxID_ANY, _("Side:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BoardSideLabel->Wrap( -1 );
	fgSizerPos->Add( m_BoardSideLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_BoardSideCtrlChoices[] = { _("Front"), _("Back") };
	int m_BoardSideCtrlNChoices = sizeof( m_BoardSideCtrlChoices ) / sizeof( wxString );
	m_BoardSideCtrl = new wxChoice( sbSizer7->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_BoardSideCtrlNChoices, m_BoardSideCtrlChoices, 0 );
	m_BoardSideCtrl->SetSelection( 1 );
	fgSizerPos->Add( m_BoardSideCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	sbSizer7->Add( fgSizerPos, 0, wxEXPAND|wxBOTTOM, 3 );

	m_cbLocked = new wxCheckBox( sbSizer7->GetStaticBox(), wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer7->Add( m_cbLocked, 0, wxALL, 5 );


	bSizerLeft->Add( sbSizer7, 1, wxEXPAND|wxALL, 5 );


	bSizerProperties->Add( bSizerLeft, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbFabSizer;
	sbFabSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Fabrication Attributes") ), wxVERTICAL );

	wxBoxSizer* bPartTypeSizer;
	bPartTypeSizer = new wxBoxSizer( wxHORIZONTAL );

	m_componentTypeLabel = new wxStaticText( sbFabSizer->GetStaticBox(), wxID_ANY, _("Footprint type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_componentTypeLabel->Wrap( -1 );
	bPartTypeSizer->Add( m_componentTypeLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_componentTypeChoices[] = { _("Through hole"), _("SMD"), _("Unspecified") };
	int m_componentTypeNChoices = sizeof( m_componentTypeChoices ) / sizeof( wxString );
	m_componentType = new wxChoice( sbFabSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_componentTypeNChoices, m_componentTypeChoices, 0 );
	m_componentType->SetSelection( 0 );
	bPartTypeSizer->Add( m_componentType, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbFabSizer->Add( bPartTypeSizer, 0, wxEXPAND, 5 );


	sbFabSizer->Add( 0, 0, 1, wxEXPAND, 5 );

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


	bSizerMiddle->Add( sbFabSizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerProperties->Add( bSizerMiddle, 1, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxVERTICAL );

	m_buttonUpdate = new wxButton( m_PanelGeneral, wxID_ANY, _("Update Footprint from Library..."), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonUpdate, 0, wxEXPAND|wxALL, 5 );

	m_buttonExchange = new wxButton( m_PanelGeneral, wxID_ANY, _("Change Footprint..."), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonExchange, 0, wxEXPAND|wxALL, 5 );

	m_buttonModuleEditor = new wxButton( m_PanelGeneral, wxID_ANY, _("Edit Footprint..."), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonModuleEditor, 0, wxEXPAND|wxALL, 5 );


	bButtonsSizer->Add( 0, 15, 0, wxEXPAND, 5 );

	m_button5 = new wxButton( m_PanelGeneral, wxID_ANY, _("Edit Library Footprint..."), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_button5, 0, wxEXPAND|wxALL, 5 );


	bSizerRight->Add( bButtonsSizer, 0, wxEXPAND|wxALL, 5 );


	bSizerRight->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizerProperties->Add( bSizerRight, 1, wxEXPAND|wxTOP, 5 );


	m_PanelPropertiesBoxSizer->Add( bSizerProperties, 0, wxEXPAND, 5 );


	m_PanelGeneral->SetSizer( m_PanelPropertiesBoxSizer );
	m_PanelGeneral->Layout();
	m_PanelPropertiesBoxSizer->Fit( m_PanelGeneral );
	m_NoteBook->AddPage( m_PanelGeneral, _("General"), true );
	m_PanelClearances = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanelClearances;
	bSizerPanelClearances = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerLocalProperties;
	sbSizerLocalProperties = new wxStaticBoxSizer( new wxStaticBox( m_PanelClearances, wxID_ANY, _("Clearances") ), wxVERTICAL );

	m_staticTextInfo = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Set values to 0 to use Board Setup values."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfo, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 4, 0 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer2->SetEmptyCellSize( wxSize( -1,15 ) );

	m_NetClearanceLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Pad clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceLabel->Wrap( -1 );
	m_NetClearanceLabel->SetToolTip( _("This is the local net clearance for all pad of this footprint\nIf 0, the Netclass values are used\nThis value can be superseded by a pad local value.") );

	gbSizer2->Add( m_NetClearanceLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_NetClearanceCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_NetClearanceCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_NetClearanceUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceUnits->Wrap( -1 );
	gbSizer2->Add( m_NetClearanceUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SolderMaskMarginLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder mask expansion:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginLabel->Wrap( -1 );
	m_SolderMaskMarginLabel->SetToolTip( _("This is the local clearance between pads and the solder mask for this footprint.\nThis value can be superseded by a pad local value.\nIf 0, the global value is used.") );

	gbSizer2->Add( m_SolderMaskMarginLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SolderMaskMarginCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_SolderMaskMarginCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_SolderMaskMarginUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginUnits->Wrap( -1 );
	gbSizer2->Add( m_SolderMaskMarginUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_allowSolderMaskBridges = new wxCheckBox( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Allow bridged solder mask apertures between pads"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_allowSolderMaskBridges, wxGBPosition( 3, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_SolderPasteMarginLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder paste absolute clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginLabel->Wrap( -1 );
	m_SolderPasteMarginLabel->SetToolTip( _("This is the local clearance between pads and the solder paste for this footprint.\nThis value can be superseded by a pad local values.\nThe final clearance value is the sum of this value and the clearance value ratio.\nA negative value means a smaller stencil aperture size than pad size.") );

	gbSizer2->Add( m_SolderPasteMarginLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SolderPasteMarginCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_SolderPasteMarginCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_SolderPasteMarginUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	gbSizer2->Add( m_SolderPasteMarginUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_PasteMarginRatioLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginRatioLabel->Wrap( -1 );
	m_PasteMarginRatioLabel->SetToolTip( _("This is the local clearance ratio applied as a percentage of the pad width and height for this footprint.\nA value of 10 means the horizontal clearance value is 10% of the pad’s width, and the vertical clearance value is 10% of the pad’s height.\nThis value can be superseded by a pad local value.\nThe final clearance value is the sum of this value and the absolute clearance value.\nA negative value means a smaller stencil aperture size than pad size.") );

	gbSizer2->Add( m_PasteMarginRatioLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_PasteMarginRatioCtrl = new TEXT_CTRL_EVAL( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_PasteMarginRatioCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_PasteMarginRatioUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginRatioUnits->Wrap( -1 );
	gbSizer2->Add( m_PasteMarginRatioUnits, wxGBPosition( 6, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbSizerLocalProperties->Add( gbSizer2, 1, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	wxFlexGridSizer* fgSizerClearances;
	fgSizerClearances = new wxFlexGridSizer( 5, 3, 5, 0 );
	fgSizerClearances->AddGrowableCol( 1 );
	fgSizerClearances->SetFlexibleDirection( wxBOTH );
	fgSizerClearances->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	sbSizerLocalProperties->Add( fgSizerClearances, 0, wxTOP|wxBOTTOM, 10 );

	m_staticTextInfoCopper = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Note: solder mask and paste values are used only for pads on copper layers."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoCopper->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfoCopper, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextInfoPaste = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Note: solder paste clearances (absolute and relative) are added to determine the final clearance."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPaste->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfoPaste, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerPanelClearances->Add( sbSizerLocalProperties, 0, wxEXPAND|wxALL, 10 );

	wxStaticBoxSizer* sbSizerZoneConnection;
	sbSizerZoneConnection = new wxStaticBoxSizer( new wxStaticBox( m_PanelClearances, wxID_ANY, _("Connection to Copper Zones") ), wxHORIZONTAL );

	m_staticText16 = new wxStaticText( sbSizerZoneConnection->GetStaticBox(), wxID_ANY, _("Pad connection to zones:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	sbSizerZoneConnection->Add( m_staticText16, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_ZoneConnectionChoiceChoices[] = { _("Use zone setting"), _("Solid"), _("Thermal relief"), _("None") };
	int m_ZoneConnectionChoiceNChoices = sizeof( m_ZoneConnectionChoiceChoices ) / sizeof( wxString );
	m_ZoneConnectionChoice = new wxChoice( sbSizerZoneConnection->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneConnectionChoiceNChoices, m_ZoneConnectionChoiceChoices, 0 );
	m_ZoneConnectionChoice->SetSelection( 0 );
	sbSizerZoneConnection->Add( m_ZoneConnectionChoice, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerPanelClearances->Add( sbSizerZoneConnection, 0, wxALL|wxEXPAND, 10 );


	m_PanelClearances->SetSizer( bSizerPanelClearances );
	m_PanelClearances->Layout();
	bSizerPanelClearances->Fit( m_PanelClearances );
	m_NoteBook->AddPage( m_PanelClearances, _("Clearance Overrides and Settings"), false );

	m_GeneralBoxSizer->Add( m_NoteBook, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizerSymbolRef;
	fgSizerSymbolRef = new wxFlexGridSizer( 0, 2, 4, 0 );
	fgSizerSymbolRef->AddGrowableCol( 1 );
	fgSizerSymbolRef->SetFlexibleDirection( wxBOTH );
	fgSizerSymbolRef->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_libraryIDLabel = new wxStaticText( this, wxID_ANY, _("Library link:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_libraryIDLabel->Wrap( -1 );
	fgSizerSymbolRef->Add( m_libraryIDLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 2 );

	m_tcLibraryID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxBORDER_NONE );
	m_tcLibraryID->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	m_tcLibraryID->SetToolTip( _("The library ID and footprint ID currently assigned.  Use \"Change Footprint...\" to assign a different footprint.") );

	fgSizerSymbolRef->Add( m_tcLibraryID, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerButtons->Add( fgSizerSymbolRef, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );


	bSizerButtons->Add( 20, 0, 0, wxEXPAND, 5 );

	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();

	bSizerButtons->Add( m_sdbSizerStdButtons, 0, wxEXPAND|wxALL, 5 );


	m_GeneralBoxSizer->Add( bSizerButtons, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	this->SetSizer( m_GeneralBoxSizer );
	this->Layout();
	m_GeneralBoxSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnInitDlg ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnUpdateUI ) );
	m_NoteBook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnPageChange ), NULL, this );
	m_itemsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnGridSize ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_buttonUpdate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::UpdateFootprint ), NULL, this );
	m_buttonExchange->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::ChangeFootprint ), NULL, this );
	m_buttonModuleEditor->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::EditFootprint ), NULL, this );
	m_button5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::EditLibraryFootprint ), NULL, this );
}

DIALOG_FOOTPRINT_PROPERTIES_BASE::~DIALOG_FOOTPRINT_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnInitDlg ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnUpdateUI ) );
	m_NoteBook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnPageChange ), NULL, this );
	m_itemsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnGridSize ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_buttonUpdate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::UpdateFootprint ), NULL, this );
	m_buttonExchange->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::ChangeFootprint ), NULL, this );
	m_buttonModuleEditor->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::EditFootprint ), NULL, this );
	m_button5->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::EditLibraryFootprint ), NULL, this );

}

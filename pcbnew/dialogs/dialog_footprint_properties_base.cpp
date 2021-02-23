///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

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
	m_itemsGrid->EnableDragColSize( false );
	m_itemsGrid->SetColLabelSize( 24 );
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
	m_itemsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_itemsGrid->EnableDragRowSize( false );
	m_itemsGrid->SetRowLabelSize( 160 );
	m_itemsGrid->SetRowLabelValue( 0, _("Reference designator") );
	m_itemsGrid->SetRowLabelValue( 1, _("Value") );
	m_itemsGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_itemsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_itemsGrid->SetMinSize( wxSize( 800,140 ) );

	sbSizerTexts->Add( m_itemsGrid, 1, wxALL|wxBOTTOM|wxEXPAND, 5 );

	wxBoxSizer* bButtonSize;
	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new wxBitmapButton( sbSizerTexts->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize->Add( m_bpAdd, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDelete = new wxBitmapButton( sbSizerTexts->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize->Add( m_bpDelete, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	sbSizerTexts->Add( bButtonSize, 0, wxEXPAND, 5 );


	m_PanelPropertiesBoxSizer->Add( sbSizerTexts, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerProperties;
	bSizerProperties = new wxBoxSizer( wxHORIZONTAL );

	bSizerLeft = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer7;
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Position") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerPos;
	fgSizerPos = new wxFlexGridSizer( 3, 3, 2, 0 );
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
	fgSizerPos->Add( m_ModPositionY, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_YPosUnit = new wxStaticText( sbSizer7->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_YPosUnit->Wrap( -1 );
	fgSizerPos->Add( m_YPosUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_BoardSideLabel = new wxStaticText( sbSizer7->GetStaticBox(), wxID_ANY, _("Side:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BoardSideLabel->Wrap( -1 );
	fgSizerPos->Add( m_BoardSideLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_BoardSideCtrlChoices[] = { _("Front"), _("Back") };
	int m_BoardSideCtrlNChoices = sizeof( m_BoardSideCtrlChoices ) / sizeof( wxString );
	m_BoardSideCtrl = new wxChoice( sbSizer7->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_BoardSideCtrlNChoices, m_BoardSideCtrlChoices, 0 );
	m_BoardSideCtrl->SetSelection( 0 );
	fgSizerPos->Add( m_BoardSideCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxTOP, 5 );


	sbSizer7->Add( fgSizerPos, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerLeft->Add( sbSizer7, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerLeft->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 3 );

	wxStaticBoxSizer* sbOrientationSizer;
	sbOrientationSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Orientation") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 4, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_Orient0 = new wxRadioButton( sbOrientationSizer->GetStaticBox(), wxID_ANY, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_Orient0, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), 0, 5 );

	m_Orient90 = new wxRadioButton( sbOrientationSizer->GetStaticBox(), wxID_ANY, _("90.0"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_Orient90, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), 0, 5 );

	m_Orient270 = new wxRadioButton( sbOrientationSizer->GetStaticBox(), wxID_ANY, _("-90.0"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_Orient270, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), 0, 5 );

	m_Orient180 = new wxRadioButton( sbOrientationSizer->GetStaticBox(), wxID_ANY, _("180.0"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_Orient180, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), 0, 5 );

	m_OrientOther = new wxRadioButton( sbOrientationSizer->GetStaticBox(), wxID_ANY, _("Other:"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_OrientOther, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_OrientValueCtrl = new wxTextCtrl( sbOrientationSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_OrientValueCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	gbSizer1->AddGrowableCol( 1 );

	sbOrientationSizer->Add( gbSizer1, 1, wxEXPAND, 5 );


	bSizerLeft->Add( sbOrientationSizer, 0, wxEXPAND|wxALL, 5 );


	bSizerProperties->Add( bSizerLeft, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxVERTICAL );

	wxString m_AutoPlaceCtrlChoices[] = { _("Free"), _("Lock pads"), _("Lock footprint") };
	int m_AutoPlaceCtrlNChoices = sizeof( m_AutoPlaceCtrlChoices ) / sizeof( wxString );
	m_AutoPlaceCtrl = new wxRadioBox( m_PanelGeneral, wxID_ANY, _("Move and Place"), wxDefaultPosition, wxDefaultSize, m_AutoPlaceCtrlNChoices, m_AutoPlaceCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_AutoPlaceCtrl->SetSelection( 0 );
	bSizerMiddle->Add( m_AutoPlaceCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerMiddle->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sizerAP = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Auto-placement Rules") ), wxVERTICAL );

	m_sizerAllow90 = new wxBoxSizer( wxVERTICAL );

	m_allow90Label = new wxStaticText( m_sizerAP->GetStaticBox(), wxID_ANY, _("Allow 90 degree rotated placement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_allow90Label->Wrap( -1 );
	m_allow90Label->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_sizerAllow90->Add( m_allow90Label, 0, wxALL, 5 );

	m_CostRot90Ctrl = new wxSlider( m_sizerAP->GetStaticBox(), wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	m_sizerAllow90->Add( m_CostRot90Ctrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_sizerAP->Add( m_sizerAllow90, 0, wxEXPAND, 5 );


	m_sizerAP->Add( 0, 8, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_sizerAllow180 = new wxBoxSizer( wxVERTICAL );

	m_allow180Label = new wxStaticText( m_sizerAP->GetStaticBox(), wxID_ANY, _("Allow 180 degree rotated placement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_allow180Label->Wrap( -1 );
	m_allow180Label->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_sizerAllow180->Add( m_allow180Label, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_CostRot180Ctrl = new wxSlider( m_sizerAP->GetStaticBox(), wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	m_sizerAllow180->Add( m_CostRot180Ctrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_sizerAP->Add( m_sizerAllow180, 0, wxEXPAND, 5 );


	bSizerMiddle->Add( m_sizerAP, 0, wxEXPAND|wxALL, 5 );


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


	bButtonsSizer->Add( 0, 10, 0, wxEXPAND, 5 );

	m_button5 = new wxButton( m_PanelGeneral, wxID_ANY, _("Edit Library Footprint..."), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_button5, 0, wxEXPAND|wxALL, 5 );


	bSizerRight->Add( bButtonsSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerRight->Add( 0, 0, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sbFabSizer;
	sbFabSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Fabrication Attributes") ), wxVERTICAL );

	wxBoxSizer* bPartTypeSizer;
	bPartTypeSizer = new wxBoxSizer( wxHORIZONTAL );

	m_componentTypeLabel = new wxStaticText( sbFabSizer->GetStaticBox(), wxID_ANY, _("Component:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_componentTypeLabel->Wrap( -1 );
	bPartTypeSizer->Add( m_componentTypeLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_componentTypeChoices[] = { _("Through hole"), _("SMD"), _("Other") };
	int m_componentTypeNChoices = sizeof( m_componentTypeChoices ) / sizeof( wxString );
	m_componentType = new wxChoice( sbFabSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_componentTypeNChoices, m_componentTypeChoices, 0 );
	m_componentType->SetSelection( 0 );
	bPartTypeSizer->Add( m_componentType, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbFabSizer->Add( bPartTypeSizer, 1, wxEXPAND, 5 );

	m_boardOnly = new wxCheckBox( sbFabSizer->GetStaticBox(), wxID_ANY, _("Not in schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFabSizer->Add( m_boardOnly, 0, wxALL, 5 );

	m_excludeFromPosFiles = new wxCheckBox( sbFabSizer->GetStaticBox(), wxID_ANY, _("Exclude from position files"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFabSizer->Add( m_excludeFromPosFiles, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_excludeFromBOM = new wxCheckBox( sbFabSizer->GetStaticBox(), wxID_ANY, _("Exclude from BOM"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFabSizer->Add( m_excludeFromBOM, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerRight->Add( sbFabSizer, 0, wxEXPAND|wxALL, 5 );


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
	m_staticTextInfo->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	sbSizerLocalProperties->Add( m_staticTextInfo, 0, wxLEFT|wxRIGHT, 5 );

	m_staticTextInfoValPos = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Positive clearance means area bigger than the pad (usual for mask clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoValPos->Wrap( -1 );
	m_staticTextInfoValPos->SetFont( wxFont( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	sbSizerLocalProperties->Add( m_staticTextInfoValPos, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextInfoValNeg = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Negative clearance means area smaller than the pad (usual for paste clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoValNeg->Wrap( -1 );
	m_staticTextInfoValNeg->SetFont( wxFont( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	sbSizerLocalProperties->Add( m_staticTextInfoValNeg, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxFlexGridSizer* fgSizerClearances;
	fgSizerClearances = new wxFlexGridSizer( 5, 3, 0, 0 );
	fgSizerClearances->AddGrowableCol( 1 );
	fgSizerClearances->SetFlexibleDirection( wxBOTH );
	fgSizerClearances->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_NetClearanceLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Pad clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceLabel->Wrap( -1 );
	m_NetClearanceLabel->SetToolTip( _("This is the local net clearance for all pad of this footprint\nIf 0, the Netclass values are used\nThis value can be superseded by a pad local value.") );

	fgSizerClearances->Add( m_NetClearanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_NetClearanceCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerClearances->Add( m_NetClearanceCtrl, 1, wxEXPAND|wxALL, 5 );

	m_NetClearanceUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceUnits->Wrap( -1 );
	fgSizerClearances->Add( m_NetClearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SolderMaskMarginLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginLabel->Wrap( -1 );
	m_SolderMaskMarginLabel->SetToolTip( _("This is the local clearance between pads and the solder mask for this footprint.\nThis value can be superseded by a pad local value.\nIf 0, the global value is used.") );

	fgSizerClearances->Add( m_SolderMaskMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_SolderMaskMarginCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerClearances->Add( m_SolderMaskMarginCtrl, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_SolderMaskMarginUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginUnits->Wrap( -1 );
	fgSizerClearances->Add( m_SolderMaskMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SolderPasteMarginLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder paste absolute clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginLabel->Wrap( -1 );
	m_SolderPasteMarginLabel->SetToolTip( _("This is the local clearance between pads and the solder paste for this footprint.\nThis value can be superseded by a pad local values.\nThe final clearance value is the sum of this value and the clearance value ratio.\nA negative value means a smaller mask size than pad size.") );

	fgSizerClearances->Add( m_SolderPasteMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_SolderPasteMarginCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerClearances->Add( m_SolderPasteMarginCtrl, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_SolderPasteMarginUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	fgSizerClearances->Add( m_SolderPasteMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextRatio = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRatio->Wrap( -1 );
	m_staticTextRatio->SetToolTip( _("This is the local clearance ratio in percent between pads and the solder paste for this footprint.\nA value of 10 means the clearance value is 10 percent of the pad size.\nThis value can be superseded by a pad local value.\nThe final clearance value is the sum of this value and the clearance value.\nA negative value means a smaller mask size than pad size.") );

	fgSizerClearances->Add( m_staticTextRatio, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_SolderPasteMarginRatioCtrl = new TEXT_CTRL_EVAL( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerClearances->Add( m_SolderPasteMarginRatioCtrl, 1, wxALL|wxEXPAND, 5 );

	m_SolderPasteRatioMarginUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteRatioMarginUnits->Wrap( -1 );
	fgSizerClearances->Add( m_SolderPasteRatioMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	sbSizerLocalProperties->Add( fgSizerClearances, 1, wxEXPAND, 5 );

	m_staticTextInfoCopper = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Note: solder mask and paste values are used only for pads on copper layers."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoCopper->Wrap( -1 );
	m_staticTextInfoCopper->SetFont( wxFont( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	sbSizerLocalProperties->Add( m_staticTextInfoCopper, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextInfoPaste = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Note: solder paste clearances (absolute and relative) are added to determine the final clearance."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPaste->Wrap( -1 );
	m_staticTextInfoPaste->SetFont( wxFont( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	sbSizerLocalProperties->Add( m_staticTextInfoPaste, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerPanelClearances->Add( sbSizerLocalProperties, 0, wxEXPAND|wxALL, 10 );

	wxStaticBoxSizer* sbSizerZoneConnection;
	sbSizerZoneConnection = new wxStaticBoxSizer( new wxStaticBox( m_PanelClearances, wxID_ANY, _("Connection to Copper Zones") ), wxHORIZONTAL );

	m_staticText16 = new wxStaticText( sbSizerZoneConnection->GetStaticBox(), wxID_ANY, _("Pad connection to zones:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	sbSizerZoneConnection->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_ZoneConnectionChoiceChoices[] = { _("Use zone setting"), _("Solid"), _("Thermal relief"), _("None") };
	int m_ZoneConnectionChoiceNChoices = sizeof( m_ZoneConnectionChoiceChoices ) / sizeof( wxString );
	m_ZoneConnectionChoice = new wxChoice( sbSizerZoneConnection->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneConnectionChoiceNChoices, m_ZoneConnectionChoiceChoices, 0 );
	m_ZoneConnectionChoice->SetSelection( 0 );
	sbSizerZoneConnection->Add( m_ZoneConnectionChoice, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerPanelClearances->Add( sbSizerZoneConnection, 0, wxALL|wxEXPAND, 10 );


	m_PanelClearances->SetSizer( bSizerPanelClearances );
	m_PanelClearances->Layout();
	bSizerPanelClearances->Fit( m_PanelClearances );
	m_NoteBook->AddPage( m_PanelClearances, _("Local Clearance and Settings"), false );
	m_Panel3D = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bSizerMain3D = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_Panel3D, wxID_ANY, wxEmptyString ), wxVERTICAL );

	m_modelsGrid = new WX_GRID( sbSizer3->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_modelsGrid->CreateGrid( 3, 2 );
	m_modelsGrid->EnableEditing( true );
	m_modelsGrid->EnableGridLines( true );
	m_modelsGrid->EnableDragGridSize( false );
	m_modelsGrid->SetMargins( 0, 0 );

	// Columns
	m_modelsGrid->SetColSize( 0, 650 );
	m_modelsGrid->SetColSize( 1, 65 );
	m_modelsGrid->EnableDragColMove( false );
	m_modelsGrid->EnableDragColSize( false );
	m_modelsGrid->SetColLabelSize( 24 );
	m_modelsGrid->SetColLabelValue( 0, _("3D Model(s)") );
	m_modelsGrid->SetColLabelValue( 1, _("Show") );
	m_modelsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_modelsGrid->EnableDragRowSize( false );
	m_modelsGrid->SetRowLabelSize( 0 );
	m_modelsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_modelsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_modelsGrid->SetMinSize( wxSize( -1,56 ) );

	sbSizer3->Add( m_modelsGrid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer3DButtons;
	bSizer3DButtons = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAdd = new wxBitmapButton( sbSizer3->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizer3DButtons->Add( m_buttonAdd, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_buttonBrowse = new wxBitmapButton( sbSizer3->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizer3DButtons->Add( m_buttonBrowse, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bSizer3DButtons->Add( 20, 0, 0, wxEXPAND, 5 );

	m_buttonRemove = new wxBitmapButton( sbSizer3->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizer3DButtons->Add( m_buttonRemove, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bSizer3DButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonCfgPath = new wxButton( sbSizer3->GetStaticBox(), wxID_ANY, _("Configure Paths..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonCfgPath, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	sbSizer3->Add( bSizer3DButtons, 0, wxEXPAND, 5 );


	bSizerMain3D->Add( sbSizer3, 4, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	bLowerSizer3D = new wxBoxSizer( wxHORIZONTAL );


	bSizerMain3D->Add( bLowerSizer3D, 11, wxEXPAND, 5 );


	m_Panel3D->SetSizer( bSizerMain3D );
	m_Panel3D->Layout();
	bSizerMain3D->Fit( m_Panel3D );
	m_NoteBook->AddPage( m_Panel3D, _("3D Models"), false );

	m_GeneralBoxSizer->Add( m_NoteBook, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizerSymbolRef;
	fgSizerSymbolRef = new wxFlexGridSizer( 0, 2, 4, 0 );
	fgSizerSymbolRef->AddGrowableCol( 1 );
	fgSizerSymbolRef->SetFlexibleDirection( wxBOTH );
	fgSizerSymbolRef->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_libraryIDLabel = new wxStaticText( this, wxID_ANY, _("Library link:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_libraryIDLabel->Wrap( -1 );
	m_libraryIDLabel->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	fgSizerSymbolRef->Add( m_libraryIDLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );

	m_tcLibraryID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxBORDER_NONE );
	m_tcLibraryID->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	m_tcLibraryID->SetToolTip( _("The library ID and footprint ID currently assigned.  Use “Change Footprint…” to assign a different footprint.") );

	fgSizerSymbolRef->Add( m_tcLibraryID, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	bSizerButtons->Add( fgSizerSymbolRef, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );


	bSizerButtons->Add( 20, 0, 0, wxEXPAND, 5 );

	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();

	bSizerButtons->Add( m_sdbSizerStdButtons, 0, wxEXPAND|wxALL, 5 );


	m_GeneralBoxSizer->Add( bSizerButtons, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );


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
	m_Orient0->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::FootprintOrientEvent ), NULL, this );
	m_Orient90->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::FootprintOrientEvent ), NULL, this );
	m_Orient270->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::FootprintOrientEvent ), NULL, this );
	m_Orient180->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::FootprintOrientEvent ), NULL, this );
	m_OrientOther->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::FootprintOrientEvent ), NULL, this );
	m_OrientValueCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnOtherOrientation ), NULL, this );
	m_buttonUpdate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::UpdateFootprint ), NULL, this );
	m_buttonExchange->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::ChangeFootprint ), NULL, this );
	m_buttonModuleEditor->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::EditFootprint ), NULL, this );
	m_button5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::EditLibraryFootprint ), NULL, this );
	m_modelsGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::On3DModelCellChanged ), NULL, this );
	m_modelsGrid->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::On3DModelSelected ), NULL, this );
	m_buttonAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnAdd3DRow ), NULL, this );
	m_buttonBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnAdd3DModel ), NULL, this );
	m_buttonRemove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnRemove3DModel ), NULL, this );
	m_buttonCfgPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::Cfg3DPath ), NULL, this );
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
	m_Orient0->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::FootprintOrientEvent ), NULL, this );
	m_Orient90->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::FootprintOrientEvent ), NULL, this );
	m_Orient270->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::FootprintOrientEvent ), NULL, this );
	m_Orient180->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::FootprintOrientEvent ), NULL, this );
	m_OrientOther->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::FootprintOrientEvent ), NULL, this );
	m_OrientValueCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnOtherOrientation ), NULL, this );
	m_buttonUpdate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::UpdateFootprint ), NULL, this );
	m_buttonExchange->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::ChangeFootprint ), NULL, this );
	m_buttonModuleEditor->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::EditFootprint ), NULL, this );
	m_button5->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::EditLibraryFootprint ), NULL, this );
	m_modelsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::On3DModelCellChanged ), NULL, this );
	m_modelsGrid->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::On3DModelSelected ), NULL, this );
	m_buttonAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnAdd3DRow ), NULL, this );
	m_buttonBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnAdd3DModel ), NULL, this );
	m_buttonRemove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::OnRemove3DModel ), NULL, this );
	m_buttonCfgPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_PROPERTIES_BASE::Cfg3DPath ), NULL, this );

}

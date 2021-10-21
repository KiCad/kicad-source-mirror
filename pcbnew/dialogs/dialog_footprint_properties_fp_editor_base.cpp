///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

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
	m_itemsGrid->SetColLabelSize( 24 );
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

	m_bpAdd = new wxBitmapButton( sbSizerTexts->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize->Add( m_bpAdd, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDelete = new wxBitmapButton( sbSizerTexts->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
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
	fgSizerFPID->Add( staticFPNameLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_FootprintNameCtrl = new wxTextCtrl( m_PanelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerFPID->Add( m_FootprintNameCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

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


	m_PanelPropertiesBoxSizer->Add( fgSizerFPID, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizerProperties;
	bSizerProperties = new wxBoxSizer( wxHORIZONTAL );

	m_sizerAP = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Auto-placement Rules") ), wxVERTICAL );

	m_sizerAllow90 = new wxBoxSizer( wxVERTICAL );

	m_allow90Label = new wxStaticText( m_sizerAP->GetStaticBox(), wxID_ANY, _("Allow 90 degree rotated placement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_allow90Label->Wrap( -1 );
	m_sizerAllow90->Add( m_allow90Label, 0, wxLEFT, 5 );

	m_CostRot90Ctrl = new wxSlider( m_sizerAP->GetStaticBox(), wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	m_sizerAllow90->Add( m_CostRot90Ctrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	m_sizerAP->Add( m_sizerAllow90, 0, wxEXPAND, 5 );


	m_sizerAP->Add( 0, 8, 1, wxEXPAND, 5 );

	m_sizerAllow180 = new wxBoxSizer( wxVERTICAL );

	m_allow180Label = new wxStaticText( m_sizerAP->GetStaticBox(), wxID_ANY, _("Allow 180 degree rotated placement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_allow180Label->Wrap( -1 );
	m_sizerAllow180->Add( m_allow180Label, 0, wxLEFT, 5 );

	m_CostRot180Ctrl = new wxSlider( m_sizerAP->GetStaticBox(), wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	m_sizerAllow180->Add( m_CostRot180Ctrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	m_sizerAP->Add( m_sizerAllow180, 0, wxEXPAND|wxBOTTOM, 5 );


	bSizerProperties->Add( m_sizerAP, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerProperties->Add( 10, 0, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbFabSizer;
	sbFabSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelGeneral, wxID_ANY, _("Fabrication Attributes") ), wxVERTICAL );

	wxBoxSizer* bPartTypeSizer;
	bPartTypeSizer = new wxBoxSizer( wxHORIZONTAL );

	m_componentTypeLabel = new wxStaticText( sbFabSizer->GetStaticBox(), wxID_ANY, _("Component type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_componentTypeLabel->Wrap( -1 );
	bPartTypeSizer->Add( m_componentTypeLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_componentTypeChoices[] = { _("Through hole"), _("SMD"), _("Other") };
	int m_componentTypeNChoices = sizeof( m_componentTypeChoices ) / sizeof( wxString );
	m_componentType = new wxChoice( sbFabSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_componentTypeNChoices, m_componentTypeChoices, 0 );
	m_componentType->SetSelection( 0 );
	bPartTypeSizer->Add( m_componentType, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbFabSizer->Add( bPartTypeSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	m_boardOnly = new wxCheckBox( sbFabSizer->GetStaticBox(), wxID_ANY, _("Not in schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFabSizer->Add( m_boardOnly, 0, wxALL, 5 );

	m_excludeFromPosFiles = new wxCheckBox( sbFabSizer->GetStaticBox(), wxID_ANY, _("Exclude from position files"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFabSizer->Add( m_excludeFromPosFiles, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_excludeFromBOM = new wxCheckBox( sbFabSizer->GetStaticBox(), wxID_ANY, _("Exclude from BOM"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFabSizer->Add( m_excludeFromBOM, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerProperties->Add( sbFabSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_PanelPropertiesBoxSizer->Add( bSizerProperties, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	m_PanelGeneral->SetSizer( m_PanelPropertiesBoxSizer );
	m_PanelGeneral->Layout();
	m_PanelPropertiesBoxSizer->Fit( m_PanelGeneral );
	m_NoteBook->AddPage( m_PanelGeneral, _("General"), false );
	m_PanelClearances = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanelClearances;
	bSizerPanelClearances = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerLocalProperties;
	sbSizerLocalProperties = new wxStaticBoxSizer( new wxStaticBox( m_PanelClearances, wxID_ANY, _("Clearances") ), wxVERTICAL );

	m_staticTextInfo = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Set values to 0 to use netclass values."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfo, 0, wxLEFT|wxRIGHT, 5 );

	m_staticTextInfoValPos = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Positive clearance means area bigger than the pad (usual for mask clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoValPos->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfoValPos, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextInfoValNeg = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Negative clearance means area smaller than the pad (usual for paste clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoValNeg->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfoValNeg, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxFlexGridSizer* fgSizerClearances;
	fgSizerClearances = new wxFlexGridSizer( 5, 3, 0, 0 );
	fgSizerClearances->AddGrowableCol( 1 );
	fgSizerClearances->SetFlexibleDirection( wxBOTH );
	fgSizerClearances->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_NetClearanceLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Pad clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceLabel->Wrap( -1 );
	m_NetClearanceLabel->SetToolTip( _("This is the local net clearance for all pads of this footprint.\nIf 0, the Netclass values are used.\nThis value can be overridden on a pad-by-pad basis in the Local\nClearance and Settings tab of Pad Properties.") );

	fgSizerClearances->Add( m_NetClearanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_NetClearanceCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerClearances->Add( m_NetClearanceCtrl, 1, wxEXPAND|wxALL, 5 );

	m_NetClearanceUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceUnits->Wrap( -1 );
	fgSizerClearances->Add( m_NetClearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SolderMaskMarginLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginLabel->Wrap( -1 );
	m_SolderMaskMarginLabel->SetToolTip( _("This is the local clearance between pads and the solder mask for \nthis footprint.\nIf 0, the global value is used.\nThis value can be overridden on a pad-by-pad basis in the Local\nClearance and Settings tab of Pad Properties.") );

	fgSizerClearances->Add( m_SolderMaskMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_SolderMaskMarginCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerClearances->Add( m_SolderMaskMarginCtrl, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_SolderMaskMarginUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginUnits->Wrap( -1 );
	fgSizerClearances->Add( m_SolderMaskMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SolderPasteMarginLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder paste absolute clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginLabel->Wrap( -1 );
	m_SolderPasteMarginLabel->SetToolTip( _("This is the local clearance between pads and the solder paste for\nthis footprint.\nThe final clearance value is the sum of this value and the clearance value ratio.\nA negative value means a smaller mask size than pad size.\nThis value can be overridden on a pad-by-pad basis in the Local\nClearance and Settings tab of Pad Properties.") );

	fgSizerClearances->Add( m_SolderPasteMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_SolderPasteMarginCtrl = new wxTextCtrl( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerClearances->Add( m_SolderPasteMarginCtrl, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_SolderPasteMarginUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	fgSizerClearances->Add( m_SolderPasteMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_PasteMarginRatioLabel = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginRatioLabel->Wrap( -1 );
	m_PasteMarginRatioLabel->SetToolTip( _("This is the local clearance ratio in percent between pads and the\nsolder paste for this footprint.\nA value of 10 means the clearance value is 10 percent of the pad size.\nThe final clearance value is the sum of this value and the clearance value.\nA negative value means a smaller mask size than pad size.\nThis value can be overridden on a pad-by-pad basis in the Local\nClearance and Settings tab of Pad Properties.") );

	fgSizerClearances->Add( m_PasteMarginRatioLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_PasteMarginRatioCtrl = new TEXT_CTRL_EVAL( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerClearances->Add( m_PasteMarginRatioCtrl, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_PasteMarginRatioUnits = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginRatioUnits->Wrap( -1 );
	fgSizerClearances->Add( m_PasteMarginRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerLocalProperties->Add( fgSizerClearances, 1, wxEXPAND, 5 );

	m_staticTextInfoCopper = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Note: solder mask and paste values are used only for pads on copper layers."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoCopper->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfoCopper, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextInfoPaste = new wxStaticText( sbSizerLocalProperties->GetStaticBox(), wxID_ANY, _("Note: solder paste clearances (absolute and relative) are added to determine the final clearance."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPaste->Wrap( -1 );
	sbSizerLocalProperties->Add( m_staticTextInfoPaste, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerPanelClearances->Add( sbSizerLocalProperties, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( m_PanelClearances, wxID_ANY, _("Connection to Copper Zones") ), wxHORIZONTAL );

	m_staticText16 = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Pad connection to zones:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	sbSizer5->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxString m_ZoneConnectionChoiceChoices[] = { _("Use zone setting"), _("Solid"), _("Thermal relief"), _("None") };
	int m_ZoneConnectionChoiceNChoices = sizeof( m_ZoneConnectionChoiceChoices ) / sizeof( wxString );
	m_ZoneConnectionChoice = new wxChoice( sbSizer5->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneConnectionChoiceNChoices, m_ZoneConnectionChoiceChoices, 0 );
	m_ZoneConnectionChoice->SetSelection( 0 );
	sbSizer5->Add( m_ZoneConnectionChoice, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizerPanelClearances->Add( sbSizer5, 0, wxALL|wxEXPAND, 5 );


	m_PanelClearances->SetSizer( bSizerPanelClearances );
	m_PanelClearances->Layout();
	bSizerPanelClearances->Fit( m_PanelClearances );
	m_NoteBook->AddPage( m_PanelClearances, _("Clearance Overrides and Settings"), true );

	m_GeneralBoxSizer->Add( m_NoteBook, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );

	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();

	bSizer16->Add( m_sdbSizerStdButtons, 1, wxEXPAND|wxALL, 5 );


	m_GeneralBoxSizer->Add( bSizer16, 0, wxEXPAND, 5 );


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

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"
#include "widgets/font_choice.h"

#include "dialog_global_edit_text_and_graphics_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerTop;
	bSizerTop = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbScope;
	sbScope = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scope") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 5, 30 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_references = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Reference designators"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_references, 0, 0, 5 );

	m_boardGraphics = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("PCB graphic items"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_boardGraphics, 0, 0, 5 );

	m_values = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Values"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_values, 0, 0, 5 );

	m_boardText = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("PCB text items"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_boardText, 0, 0, 5 );

	m_otherFootprintFields = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Other footprint fields"), wxDefaultPosition, wxDefaultSize, 0 );
	m_otherFootprintFields->SetToolTip( _("Footprint fields that are not the reference designator or value field") );

	fgSizer3->Add( m_otherFootprintFields, 0, 0, 5 );

	m_boardDimensions = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("PCB dimensions"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_boardDimensions, 0, 0, 5 );

	m_footprintGraphics = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Footprint graphic items"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_footprintGraphics, 0, wxTOP, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_footprintTexts = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Footprint text items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_footprintTexts->SetToolTip( _("Footprint text items not associated with a field") );

	fgSizer3->Add( m_footprintTexts, 0, 0, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_footprintDimensions = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Footprint dimensions"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_footprintDimensions, 0, 0, 5 );


	sbScope->Add( fgSizer3, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerTop->Add( sbScope, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxStaticBoxSizer* sbFilters;
	sbFilters = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Filter Items") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 3, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_layerFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("By layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_layerFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_layerFilter = new PCB_LAYER_BOX_SELECTOR( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizer2->Add( m_layerFilter, 0, wxEXPAND|wxLEFT, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 2 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_referenceFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("By parent reference designator:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_referenceFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_referenceFilter = new wxTextCtrl( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_referenceFilter, 0, wxEXPAND|wxLEFT, 5 );

	m_footprintFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("By parent footprint library link:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_footprintFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_footprintFilter = new wxTextCtrl( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_footprintFilter, 0, wxBOTTOM|wxEXPAND|wxLEFT, 5 );

	m_selectedItemsFilter = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Selected items only"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_selectedItemsFilter, 0, wxALL, 5 );


	sbFilters->Add( fgSizer2, 1, wxEXPAND|wxRIGHT, 5 );


	bSizerTop->Add( sbFilters, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	bMainSizer->Add( bSizerTop, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sbAction;
	sbAction = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Action") ), wxVERTICAL );

	m_setToSpecifiedValues = new wxRadioButton( sbAction->GetStaticBox(), wxID_ANY, _("Set to specified values:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_setToSpecifiedValues->SetValue( true );
	sbAction->Add( m_setToSpecifiedValues, 0, wxRIGHT, 5 );

	m_specifiedValues = new wxPanel( sbAction->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 5, 2, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->AddGrowableCol( 4 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_LayerLabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerLabel->Wrap( -1 );
	fgSizer1->Add( m_LayerLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_LayerCtrl = new PCB_LAYER_BOX_SELECTOR( m_specifiedValues, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizer1->Add( m_LayerCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizer1->Add( 0, 0, 0, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_visible = new wxCheckBox( m_specifiedValues, wxID_ANY, _("Visible  (fields only)"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	m_visible->SetValue(true);
	fgSizer1->Add( m_visible, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 120 );

	m_lineWidthLabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("Line thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	fgSizer1->Add( m_lineWidthLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_LineWidthCtrl = new wxTextCtrl( m_specifiedValues, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_LineWidthCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_lineWidthUnits = new wxStaticText( m_specifiedValues, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	fgSizer1->Add( m_lineWidthUnits, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_fontLabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	fgSizer1->Add( m_fontLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_fontCtrlChoices[] = { _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( m_specifiedValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	fgSizer1->Add( m_fontCtrl, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_bold = new wxCheckBox( m_specifiedValues, wxID_ANY, _("Bold"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	fgSizer1->Add( m_bold, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 120 );

	m_SizeXlabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("Text width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXlabel->Wrap( -1 );
	fgSizer1->Add( m_SizeXlabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_SizeXCtrl = new wxTextCtrl( m_specifiedValues, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXCtrl->SetMinSize( wxSize( 120,-1 ) );

	fgSizer1->Add( m_SizeXCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_SizeXunit = new wxStaticText( m_specifiedValues, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXunit->Wrap( -1 );
	fgSizer1->Add( m_SizeXunit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer1->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 20 );

	m_italic = new wxCheckBox( m_specifiedValues, wxID_ANY, _("Italic"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	fgSizer1->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 120 );

	m_SizeYlabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("Text height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYlabel->Wrap( -1 );
	fgSizer1->Add( m_SizeYlabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SizeYCtrl = new wxTextCtrl( m_specifiedValues, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_SizeYCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_SizeYunit = new wxStaticText( m_specifiedValues, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYunit->Wrap( -1 );
	fgSizer1->Add( m_SizeYunit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer1->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 20 );

	m_keepUpright = new wxCheckBox( m_specifiedValues, wxID_ANY, _("Keep upright"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	m_keepUpright->SetValue(true);
	fgSizer1->Add( m_keepUpright, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 120 );

	m_ThicknessLabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("Text thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessLabel->Wrap( -1 );
	fgSizer1->Add( m_ThicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ThicknessCtrl = new wxTextCtrl( m_specifiedValues, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_ThicknessCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_ThicknessUnit = new wxStaticText( m_specifiedValues, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessUnit->Wrap( -1 );
	fgSizer1->Add( m_ThicknessUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer1->Add( 0, 0, 0, wxEXPAND|wxLEFT|wxRIGHT, 20 );

	m_centerOnFP = new wxCheckBox( m_specifiedValues, wxID_ANY, _("Center on footprint"), wxDefaultPosition, wxDefaultSize, 0 );
	m_centerOnFP->SetToolTip( _("Move footprint texts to the center of their parent") );

	fgSizer1->Add( m_centerOnFP, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 120 );


	bSizer2->Add( fgSizer1, 1, wxBOTTOM|wxEXPAND|wxTOP, 2 );


	m_specifiedValues->SetSizer( bSizer2 );
	m_specifiedValues->Layout();
	bSizer2->Fit( m_specifiedValues );
	sbAction->Add( m_specifiedValues, 0, wxEXPAND|wxBOTTOM|wxLEFT, 18 );

	m_setToLayerDefaults = new wxRadioButton( sbAction->GetStaticBox(), wxID_ANY, _("Set to layer default values:"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAction->Add( m_setToLayerDefaults, 0, wxTOP|wxBOTTOM|wxEXPAND, 3 );

	m_grid = new wxGrid( sbAction->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE );

	// Grid
	m_grid->CreateGrid( 7, 7 );
	m_grid->EnableEditing( false );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 132 );
	m_grid->SetColSize( 1, 120 );
	m_grid->SetColSize( 2, 120 );
	m_grid->SetColSize( 3, 120 );
	m_grid->SetColSize( 4, 120 );
	m_grid->SetColSize( 5, 96 );
	m_grid->SetColSize( 6, 96 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( false );
	m_grid->SetColLabelSize( 0 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	m_grid->SetDefaultCellFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	sbAction->Add( m_grid, 0, wxEXPAND|wxLEFT, 23 );


	sbAction->Add( 0, 0, 0, wxEXPAND|wxBOTTOM, 5 );


	bMainSizer->Add( sbAction, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsApply = new wxButton( this, wxID_APPLY );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsApply );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();

	bMainSizer->Add( m_sdbSizerButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_boardDimensions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onDimensionItemCheckbox ), NULL, this );
	m_footprintDimensions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onDimensionItemCheckbox ), NULL, this );
	m_layerFilter->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnLayerFilterSelect ), NULL, this );
	m_referenceFilter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnReferenceFilterText ), NULL, this );
	m_footprintFilter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnFootprintFilterText ), NULL, this );
	m_setToSpecifiedValues->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onActionButtonChange ), NULL, this );
	m_LayerLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_LayerCtrl->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_visible->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_fontCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onFontSelected ), NULL, this );
	m_bold->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_italic->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_keepUpright->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_setToLayerDefaults->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onActionButtonChange ), NULL, this );
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnSizeNetclassGrid ), NULL, this );
}

DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::~DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE()
{
	// Disconnect Events
	m_boardDimensions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onDimensionItemCheckbox ), NULL, this );
	m_footprintDimensions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onDimensionItemCheckbox ), NULL, this );
	m_layerFilter->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnLayerFilterSelect ), NULL, this );
	m_referenceFilter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnReferenceFilterText ), NULL, this );
	m_footprintFilter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnFootprintFilterText ), NULL, this );
	m_setToSpecifiedValues->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onActionButtonChange ), NULL, this );
	m_LayerLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_LayerCtrl->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_visible->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_fontCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onFontSelected ), NULL, this );
	m_bold->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_italic->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_keepUpright->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onSpecifiedValueUpdateUI ), NULL, this );
	m_setToLayerDefaults->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::onActionButtonChange ), NULL, this );
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnSizeNetclassGrid ), NULL, this );

}

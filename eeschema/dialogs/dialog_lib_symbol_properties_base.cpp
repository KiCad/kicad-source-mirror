///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_lib_symbol_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_SYMBOL_PROPERTIES_BASE::DIALOG_LIB_SYMBOL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	m_NoteBook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0|wxTAB_TRAVERSAL );
	m_PanelBasic = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerBasicPanel;
	bSizerBasicPanel = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( m_PanelBasic, wxID_ANY, _("Fields") ), wxVERTICAL );

	m_grid = new WX_GRID( sbSizer4->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 4, 14 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 72 );
	m_grid->SetColSize( 1, 8 );
	m_grid->SetColSize( 2, 48 );
	m_grid->SetColSize( 3, 84 );
	m_grid->SetColSize( 4, 66 );
	m_grid->SetColSize( 5, 66 );
	m_grid->SetColSize( 6, 48 );
	m_grid->SetColSize( 7, 48 );
	m_grid->SetColSize( 8, 80 );
	m_grid->SetColSize( 9, 84 );
	m_grid->SetColSize( 10, 84 );
	m_grid->SetColSize( 11, 84 );
	m_grid->SetColSize( 12, 10 );
	m_grid->SetColSize( 13, 48 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelValue( 0, _("Name") );
	m_grid->SetColLabelValue( 1, _("Value") );
	m_grid->SetColLabelValue( 2, _("Show") );
	m_grid->SetColLabelValue( 3, _("Show Name") );
	m_grid->SetColLabelValue( 4, _("H Align") );
	m_grid->SetColLabelValue( 5, _("V Align") );
	m_grid->SetColLabelValue( 6, _("Italic") );
	m_grid->SetColLabelValue( 7, _("Bold") );
	m_grid->SetColLabelValue( 8, _("Text Size") );
	m_grid->SetColLabelValue( 9, _("Orientation") );
	m_grid->SetColLabelValue( 10, _("X Position") );
	m_grid->SetColLabelValue( 11, _("Y Position") );
	m_grid->SetColLabelValue( 12, _("Font") );
	m_grid->SetColLabelValue( 13, _("Color") );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_grid->SetMinSize( wxSize( -1,160 ) );

	sbSizer4->Add( m_grid, 1, wxALL|wxEXPAND, 5 );

	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new STD_BITMAP_BUTTON( sbSizer4->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpAdd->SetToolTip( _("Add field") );

	bButtonSize->Add( m_bpAdd, 0, wxRIGHT|wxLEFT, 5 );

	m_bpMoveUp = new STD_BITMAP_BUTTON( sbSizer4->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveUp->SetToolTip( _("Move up") );

	bButtonSize->Add( m_bpMoveUp, 0, wxRIGHT, 5 );

	m_bpMoveDown = new STD_BITMAP_BUTTON( sbSizer4->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveDown->SetToolTip( _("Move down") );

	bButtonSize->Add( m_bpMoveDown, 0, wxRIGHT, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDelete = new STD_BITMAP_BUTTON( sbSizer4->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpDelete->SetToolTip( _("Delete field") );

	bButtonSize->Add( m_bpDelete, 0, wxRIGHT|wxLEFT, 10 );


	sbSizer4->Add( bButtonSize, 0, wxEXPAND|wxBOTTOM, 5 );


	bSizerBasicPanel->Add( sbSizer4, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerMidBasicPanel;
	bSizerMidBasicPanel = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerFPID;
	fgSizerFPID = new wxFlexGridSizer( 4, 2, 3, 0 );
	fgSizerFPID->AddGrowableCol( 1 );
	fgSizerFPID->SetFlexibleDirection( wxBOTH );
	fgSizerFPID->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* staticNameLabel;
	staticNameLabel = new wxStaticText( m_PanelBasic, wxID_ANY, _("Symbol name:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticNameLabel->Wrap( -1 );
	fgSizerFPID->Add( staticNameLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 8 );

	m_SymbolNameCtrl = new wxTextCtrl( m_PanelBasic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerFPID->Add( m_SymbolNameCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 8 );

	staticKeywordsLabel = new wxStaticText( m_PanelBasic, wxID_ANY, _("Keywords:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticKeywordsLabel->Wrap( -1 );
	fgSizerFPID->Add( staticKeywordsLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 8 );

	m_KeywordCtrl = new wxTextCtrl( m_PanelBasic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerFPID->Add( m_KeywordCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 8 );

	m_inheritsStaticText = new wxStaticText( m_PanelBasic, wxID_ANY, _("Derive from symbol:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_inheritsStaticText->Wrap( -1 );
	fgSizerFPID->Add( m_inheritsStaticText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 8 );

	m_inheritanceSelectCombo = new wxComboBox( m_PanelBasic, wxID_ANY, _("<None>"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_READONLY );
	m_inheritanceSelectCombo->SetToolTip( _("Select symbol to derive this symbol from or select\n<None> for root symbol.\n\nDerived symbols were formerly referred to as aliases.\nThis is no longer the case and all symbols are either\nderived from another symbols or they stand alone as\nroot symbols.") );

	fgSizerFPID->Add( m_inheritanceSelectCombo, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 7 );


	bSizerMidBasicPanel->Add( fgSizerFPID, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bSizerBasicPanel->Add( bSizerMidBasicPanel, 0, wxEXPAND, 5 );

	bSizerLowerBasicPanel = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol;
	bSizerLeftCol = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerSymbol;
	sbSizerSymbol = new wxStaticBoxSizer( new wxStaticBox( m_PanelBasic, wxID_ANY, _("General") ), wxVERTICAL );

	m_OptionPower = new wxCheckBox( sbSizerSymbol->GetStaticBox(), wxID_ANY, _("Define as power symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptionPower->SetToolTip( _("Power symbols define a global net with the value as a netname.\nThey will not be included in the BOM and cannot be assigned a footprint.") );

	sbSizerSymbol->Add( m_OptionPower, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );


	bSizer16->Add( 0, 0, 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_OptionLocalPower = new wxCheckBox( sbSizerSymbol->GetStaticBox(), wxID_ANY, _("Define as local power symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptionLocalPower->SetToolTip( _("Local power symbols create labels that are limited to the sheet in which they are used") );

	bSizer16->Add( m_OptionLocalPower, 0, wxBOTTOM|wxLEFT|wxRIGHT, 4 );


	sbSizerSymbol->Add( bSizer16, 1, wxEXPAND, 5 );


	bSizerLeftCol->Add( sbSizerSymbol, 1, wxEXPAND|wxALL, 5 );


	bSizerLowerBasicPanel->Add( bSizerLeftCol, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerPinTextCol;
	bSizerPinTextCol = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerPinTextOpts;
	sbSizerPinTextOpts = new wxStaticBoxSizer( new wxStaticBox( m_PanelBasic, wxID_ANY, _("Pin Text Options") ), wxVERTICAL );

	m_ShowPinNumButt = new wxCheckBox( sbSizerPinTextOpts->GetStaticBox(), wxID_ANY, _("Show pin number"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ShowPinNumButt->SetValue(true);
	m_ShowPinNumButt->SetToolTip( _("Show or hide pin numbers") );

	sbSizerPinTextOpts->Add( m_ShowPinNumButt, 0, wxRIGHT|wxLEFT, 5 );

	m_ShowPinNameButt = new wxCheckBox( sbSizerPinTextOpts->GetStaticBox(), wxID_ANY, _("Show pin name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ShowPinNameButt->SetValue(true);
	m_ShowPinNameButt->SetToolTip( _("Show or hide pin names") );

	sbSizerPinTextOpts->Add( m_ShowPinNameButt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	sbSizerPinTextOpts->Add( 0, 12, 0, wxEXPAND, 5 );

	m_PinsNameInsideButt = new wxCheckBox( sbSizerPinTextOpts->GetStaticBox(), wxID_ANY, _("Place pin names inside"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PinsNameInsideButt->SetValue(true);
	m_PinsNameInsideButt->SetToolTip( _("Check this option to have pin names inside the body and pin number outside.\nIf not checked pins names and pins numbers are outside.") );

	sbSizerPinTextOpts->Add( m_PinsNameInsideButt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerNameOffset;
	bSizerNameOffset = new wxBoxSizer( wxHORIZONTAL );

	m_nameOffsetLabel = new wxStaticText( sbSizerPinTextOpts->GetStaticBox(), wxID_ANY, _("Position offset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nameOffsetLabel->Wrap( -1 );
	m_nameOffsetLabel->SetToolTip( _("Margin between the pin name position and the symbol body.") );

	bSizerNameOffset->Add( m_nameOffsetLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 24 );

	m_nameOffsetCtrl = new wxTextCtrl( sbSizerPinTextOpts->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerNameOffset->Add( m_nameOffsetCtrl, 1, wxLEFT|wxRIGHT, 5 );

	m_nameOffsetUnits = new wxStaticText( sbSizerPinTextOpts->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nameOffsetUnits->Wrap( -1 );
	bSizerNameOffset->Add( m_nameOffsetUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerPinTextOpts->Add( bSizerNameOffset, 0, wxEXPAND|wxTOP, 2 );


	sbSizerPinTextOpts->Add( 0, 0, 0, wxEXPAND, 5 );


	bSizerPinTextCol->Add( sbSizerPinTextOpts, 1, wxALL|wxEXPAND, 5 );


	bSizerLowerBasicPanel->Add( bSizerPinTextCol, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerRightCol;
	bSizerRightCol = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerAttributes;
	sbSizerAttributes = new wxStaticBoxSizer( new wxStaticBox( m_PanelBasic, wxID_ANY, _("Attributes") ), wxVERTICAL );

	m_excludeFromSimCheckBox = new wxCheckBox( sbSizerAttributes->GetStaticBox(), wxID_ANY, _("Exclude from simulation"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerAttributes->Add( m_excludeFromSimCheckBox, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbSizerAttributes->Add( 0, 10, 0, wxEXPAND, 5 );

	m_excludeFromBomCheckBox = new wxCheckBox( sbSizerAttributes->GetStaticBox(), wxID_ANY, _("Exclude from bill of materials"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerAttributes->Add( m_excludeFromBomCheckBox, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_excludeFromBoardCheckBox = new wxCheckBox( sbSizerAttributes->GetStaticBox(), wxID_ANY, _("Exclude from board"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerAttributes->Add( m_excludeFromBoardCheckBox, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_excludeFromPosFilesCheckBox = new wxCheckBox( sbSizerAttributes->GetStaticBox(), wxID_ANY, _("Exclude from position files"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerAttributes->Add( m_excludeFromPosFilesCheckBox, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerRightCol->Add( sbSizerAttributes, 1, wxEXPAND|wxALL, 5 );


	bSizerLowerBasicPanel->Add( bSizerRightCol, 1, wxEXPAND, 5 );


	bSizerBasicPanel->Add( bSizerLowerBasicPanel, 0, wxEXPAND, 5 );


	m_PanelBasic->SetSizer( bSizerBasicPanel );
	m_PanelBasic->Layout();
	bSizerBasicPanel->Fit( m_PanelBasic );
	m_NoteBook->AddPage( m_PanelBasic, _("General"), true );
	m_PanelUnitsAndBodyStyles = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerUnitsAndBodyStyles;
	bSizerUnitsAndBodyStyles = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerUnits;
	sbSizerUnits = new wxStaticBoxSizer( new wxStaticBox( m_PanelUnitsAndBodyStyles, wxID_ANY, _("Symbol Units") ), wxVERTICAL );

	wxBoxSizer* bSizerUnitCount;
	bSizerUnitCount = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextNbUnits = new wxStaticText( sbSizerUnits->GetStaticBox(), wxID_ANY, _("Number of units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNbUnits->Wrap( -1 );
	m_staticTextNbUnits->SetToolTip( _("Enter the number of units for a symbol that contains more than one unit") );

	bSizerUnitCount->Add( m_staticTextNbUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_unitSpinCtrl = new wxSpinCtrl( sbSizerUnits->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 64, 1 );
	bSizerUnitCount->Add( m_unitSpinCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbSizerUnits->Add( bSizerUnitCount, 0, wxEXPAND|wxBOTTOM|wxLEFT, 4 );


	sbSizerUnits->Add( 0, 2, 0, wxEXPAND, 5 );

	m_OptionPartsInterchangeable = new wxCheckBox( sbSizerUnits->GetStaticBox(), wxID_ANY, _("All units are interchangeable"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptionPartsInterchangeable->SetToolTip( _("Check this option when all symbol units have the same function.\nFor instance, this should be checked for a quad NAND gate, while it should not be checked for a dual triode (where unit C is the filament).") );

	sbSizerUnits->Add( m_OptionPartsInterchangeable, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );


	sbSizerUnits->Add( 0, 15, 0, wxEXPAND, 5 );

	m_unitNamesLabel = new wxStaticText( sbSizerUnits->GetStaticBox(), wxID_ANY, _("Unit display names (optional):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitNamesLabel->Wrap( -1 );
	sbSizerUnits->Add( m_unitNamesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_unitNamesGrid = new WX_GRID( sbSizerUnits->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_unitNamesGrid->CreateGrid( 0, 2 );
	m_unitNamesGrid->EnableEditing( true );
	m_unitNamesGrid->EnableGridLines( true );
	m_unitNamesGrid->EnableDragGridSize( false );
	m_unitNamesGrid->SetMargins( 0, 0 );

	// Columns
	m_unitNamesGrid->SetColSize( 0, 36 );
	m_unitNamesGrid->SetColSize( 1, 400 );
	m_unitNamesGrid->EnableDragColMove( false );
	m_unitNamesGrid->EnableDragColSize( false );
	m_unitNamesGrid->SetColLabelSize( 0 );
	m_unitNamesGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_unitNamesGrid->EnableDragRowSize( false );
	m_unitNamesGrid->SetRowLabelSize( 0 );
	m_unitNamesGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_unitNamesGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbSizerUnits->Add( m_unitNamesGrid, 1, wxALL|wxEXPAND, 5 );


	bSizerUnitsAndBodyStyles->Add( sbSizerUnits, 1, wxEXPAND|wxALL, 10 );

	wxStaticBoxSizer* sbSizerBodyStyles;
	sbSizerBodyStyles = new wxStaticBoxSizer( new wxStaticBox( m_PanelUnitsAndBodyStyles, wxID_ANY, _("Body Styles") ), wxVERTICAL );

	m_radioSingle = new wxRadioButton( sbSizerBodyStyles->GetStaticBox(), wxID_ANY, _("Single body style"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	sbSizerBodyStyles->Add( m_radioSingle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	sbSizerBodyStyles->Add( 0, 3, 0, wxEXPAND, 5 );

	m_radioDeMorgan = new wxRadioButton( sbSizerBodyStyles->GetStaticBox(), wxID_ANY, _("‘Standard’ and ‘Alternate’ De Morgan body styles"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerBodyStyles->Add( m_radioDeMorgan, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	sbSizerBodyStyles->Add( 0, 3, 0, wxEXPAND, 5 );

	m_radioCustom = new wxRadioButton( sbSizerBodyStyles->GetStaticBox(), wxID_ANY, _("Custom body styles:"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerBodyStyles->Add( m_radioCustom, 0, wxALL, 5 );

	wxBoxSizer* bSizerIndent;
	bSizerIndent = new wxBoxSizer( wxVERTICAL );

	m_bodyStyleNamesGrid = new WX_GRID( sbSizerBodyStyles->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_bodyStyleNamesGrid->CreateGrid( 0, 1 );
	m_bodyStyleNamesGrid->EnableEditing( true );
	m_bodyStyleNamesGrid->EnableGridLines( true );
	m_bodyStyleNamesGrid->EnableDragGridSize( false );
	m_bodyStyleNamesGrid->SetMargins( 0, 0 );

	// Columns
	m_bodyStyleNamesGrid->SetColSize( 0, 400 );
	m_bodyStyleNamesGrid->EnableDragColMove( false );
	m_bodyStyleNamesGrid->EnableDragColSize( false );
	m_bodyStyleNamesGrid->SetColLabelSize( 0 );
	m_bodyStyleNamesGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_bodyStyleNamesGrid->EnableDragRowSize( false );
	m_bodyStyleNamesGrid->SetRowLabelSize( 0 );
	m_bodyStyleNamesGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_bodyStyleNamesGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerIndent->Add( m_bodyStyleNamesGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	bButtonSize1 = new wxBoxSizer( wxHORIZONTAL );

	m_bpAddBodyStyle = new STD_BITMAP_BUTTON( sbSizerBodyStyles->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpAddBodyStyle->SetToolTip( _("Add field") );

	bButtonSize1->Add( m_bpAddBodyStyle, 0, wxRIGHT|wxLEFT, 5 );

	m_bpMoveUpBodyStyle = new STD_BITMAP_BUTTON( sbSizerBodyStyles->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveUpBodyStyle->SetToolTip( _("Move up") );

	bButtonSize1->Add( m_bpMoveUpBodyStyle, 0, wxRIGHT, 5 );

	m_bpMoveDownBodyStyle = new STD_BITMAP_BUTTON( sbSizerBodyStyles->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveDownBodyStyle->SetToolTip( _("Move down") );

	bButtonSize1->Add( m_bpMoveDownBodyStyle, 0, wxRIGHT, 5 );


	bButtonSize1->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDeleteBodyStyle = new STD_BITMAP_BUTTON( sbSizerBodyStyles->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpDeleteBodyStyle->SetToolTip( _("Delete field") );

	bButtonSize1->Add( m_bpDeleteBodyStyle, 0, wxRIGHT|wxLEFT, 10 );


	bSizerIndent->Add( bButtonSize1, 0, wxEXPAND, 5 );


	sbSizerBodyStyles->Add( bSizerIndent, 1, wxEXPAND|wxLEFT, 24 );


	bSizerUnitsAndBodyStyles->Add( sbSizerBodyStyles, 1, wxEXPAND|wxALL, 10 );


	m_PanelUnitsAndBodyStyles->SetSizer( bSizerUnitsAndBodyStyles );
	m_PanelUnitsAndBodyStyles->Layout();
	bSizerUnitsAndBodyStyles->Fit( m_PanelUnitsAndBodyStyles );
	m_NoteBook->AddPage( m_PanelUnitsAndBodyStyles, _("Units && Body Styles"), false );
	m_PanelFootprintFilters = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerFPFilters;
	bSizerFPFilters = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bFPFiltersMargins;
	bFPFiltersMargins = new wxBoxSizer( wxVERTICAL );

	m_staticTextFootprints = new wxStaticText( m_PanelFootprintFilters, wxID_ANY, _("Footprint filters:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFootprints->Wrap( -1 );
	m_staticTextFootprints->SetToolTip( _("A list of footprints names that can be used for this symbol.\nFootprints names can used wildcards like sm* to allow all footprints names starting by sm.") );

	bFPFiltersMargins->Add( m_staticTextFootprints, 0, wxRIGHT|wxLEFT, 5 );

	m_FootprintFilterListBox = new wxListBox( m_PanelFootprintFilters, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_MULTIPLE );
	bFPFiltersMargins->Add( m_FootprintFilterListBox, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bFpFilterRightBoxSizer;
	bFpFilterRightBoxSizer = new wxBoxSizer( wxHORIZONTAL );

	m_addFilterButton = new STD_BITMAP_BUTTON( m_PanelFootprintFilters, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_addFilterButton->SetToolTip( _("Add footprint filter") );

	bFpFilterRightBoxSizer->Add( m_addFilterButton, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_editFilterButton = new STD_BITMAP_BUTTON( m_PanelFootprintFilters, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_editFilterButton->SetToolTip( _("Edit footprint filter") );

	bFpFilterRightBoxSizer->Add( m_editFilterButton, 0, wxALL, 5 );


	bFpFilterRightBoxSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_deleteFilterButton = new STD_BITMAP_BUTTON( m_PanelFootprintFilters, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_deleteFilterButton->SetToolTip( _("Delete footprint filter") );

	bFpFilterRightBoxSizer->Add( m_deleteFilterButton, 0, wxALL, 5 );


	bFPFiltersMargins->Add( bFpFilterRightBoxSizer, 0, 0, 5 );


	bSizerFPFilters->Add( bFPFiltersMargins, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_PanelFootprintFilters->SetSizer( bSizerFPFilters );
	m_PanelFootprintFilters->Layout();
	bSizerFPFilters->Fit( m_PanelFootprintFilters );
	m_NoteBook->AddPage( m_PanelFootprintFilters, _("Footprint Filters"), false );
	m_PanelPinConnections = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPinConnections;
	bSizerPinConnections = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_cbDuplicatePinsAreJumpers = new wxCheckBox( m_PanelPinConnections, wxID_ANY, _("All pins with duplicate numbers are jumpers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbDuplicatePinsAreJumpers->SetToolTip( _("When enabled, this footprint can have more than one pad with the same number, and pads with the same number will be considered to be jumpered together internally.") );

	bMargins->Add( m_cbDuplicatePinsAreJumpers, 0, wxALL, 5 );


	bMargins->Add( 0, 5, 0, wxEXPAND, 5 );

	m_jumperGroupsLabel = new wxStaticText( m_PanelPinConnections, wxID_ANY, _("Explicit jumper pin groups:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_jumperGroupsLabel->Wrap( -1 );
	bMargins->Add( m_jumperGroupsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_jumperGroupsGrid = new WX_GRID( m_PanelPinConnections, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

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

	bMargins->Add( m_jumperGroupsGrid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bButtonSize21;
	bButtonSize21 = new wxBoxSizer( wxHORIZONTAL );

	m_bpAddJumperGroup = new STD_BITMAP_BUTTON( m_PanelPinConnections, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize21->Add( m_bpAddJumperGroup, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bButtonSize21->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpRemoveJumperGroup = new STD_BITMAP_BUTTON( m_PanelPinConnections, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize21->Add( m_bpRemoveJumperGroup, 0, wxBOTTOM|wxRIGHT, 5 );


	bMargins->Add( bButtonSize21, 0, wxEXPAND, 5 );


	bSizerPinConnections->Add( bMargins, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_PanelPinConnections->SetSizer( bSizerPinConnections );
	m_PanelPinConnections->Layout();
	bSizerPinConnections->Fit( m_PanelPinConnections );
	m_NoteBook->AddPage( m_PanelPinConnections, _("Pin Connections"), false );

	bUpperSizer->Add( m_NoteBook, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	bMainSizer->Add( bUpperSizer, 1, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizer101;
	bSizer101 = new wxBoxSizer( wxHORIZONTAL );


	bSizer101->Add( 0, 0, 1, wxEXPAND, 5 );

	m_spiceFieldsButton = new wxButton( this, wxID_ANY, _("Edit Simulation Model..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer101->Add( m_spiceFieldsButton, 0, wxEXPAND|wxALL, 5 );


	bSizer101->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );

	m_stdSizerButton = new wxStdDialogButtonSizer();
	m_stdSizerButtonOK = new wxButton( this, wxID_OK );
	m_stdSizerButton->AddButton( m_stdSizerButtonOK );
	m_stdSizerButtonCancel = new wxButton( this, wxID_CANCEL );
	m_stdSizerButton->AddButton( m_stdSizerButtonCancel );
	m_stdSizerButton->Realize();

	bSizer101->Add( m_stdSizerButton, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bSizer101, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnUpdateUI ) );
	m_NoteBook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, wxNotebookEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnPageChanging ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_SymbolNameCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnSymbolNameKillFocus ), NULL, this );
	m_SymbolNameCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnSymbolNameText ), NULL, this );
	m_KeywordCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnText ), NULL, this );
	m_inheritanceSelectCombo->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCombobox ), NULL, this );
	m_inheritanceSelectCombo->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnText ), NULL, this );
	m_OptionPower->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::onPowerCheckBox ), NULL, this );
	m_OptionLocalPower->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::onPowerCheckBox ), NULL, this );
	m_ShowPinNumButt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_ShowPinNameButt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_PinsNameInsideButt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_nameOffsetCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnText ), NULL, this );
	m_excludeFromSimCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_excludeFromBomCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_excludeFromBoardCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_excludeFromPosFilesCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_unitSpinCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnUnitSpinCtrlKillFocus ), NULL, this );
	m_unitSpinCtrl->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnUnitSpinCtrl ), NULL, this );
	m_unitSpinCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnUnitSpinCtrlText ), NULL, this );
	m_unitSpinCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnUnitSpinCtrlEnter ), NULL, this );
	m_OptionPartsInterchangeable->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_radioSingle->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnBodyStyle ), NULL, this );
	m_radioDeMorgan->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnBodyStyle ), NULL, this );
	m_radioCustom->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnBodyStyle ), NULL, this );
	m_bpAddBodyStyle->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnAddBodyStyle ), NULL, this );
	m_bpMoveUpBodyStyle->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnBodyStyleMoveUp ), NULL, this );
	m_bpMoveDownBodyStyle->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnBodyStyleMoveDown ), NULL, this );
	m_bpDeleteBodyStyle->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnDeleteBodyStyle ), NULL, this );
	m_FootprintFilterListBox->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnFpFilterDClick ), NULL, this );
	m_FootprintFilterListBox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnEditFootprintFilter ), NULL, this );
	m_addFilterButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnAddFootprintFilter ), NULL, this );
	m_editFilterButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnEditFootprintFilter ), NULL, this );
	m_bpAddJumperGroup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnAddJumperGroup ), NULL, this );
	m_bpRemoveJumperGroup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnRemoveJumperGroup ), NULL, this );
	m_spiceFieldsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnEditSpiceModel ), NULL, this );
	m_stdSizerButtonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCancelButtonClick ), NULL, this );
}

DIALOG_LIB_SYMBOL_PROPERTIES_BASE::~DIALOG_LIB_SYMBOL_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnUpdateUI ) );
	m_NoteBook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, wxNotebookEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnPageChanging ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_SymbolNameCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnSymbolNameKillFocus ), NULL, this );
	m_SymbolNameCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnSymbolNameText ), NULL, this );
	m_KeywordCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnText ), NULL, this );
	m_inheritanceSelectCombo->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCombobox ), NULL, this );
	m_inheritanceSelectCombo->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnText ), NULL, this );
	m_OptionPower->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::onPowerCheckBox ), NULL, this );
	m_OptionLocalPower->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::onPowerCheckBox ), NULL, this );
	m_ShowPinNumButt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_ShowPinNameButt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_PinsNameInsideButt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_nameOffsetCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnText ), NULL, this );
	m_excludeFromSimCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_excludeFromBomCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_excludeFromBoardCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_excludeFromPosFilesCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_unitSpinCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnUnitSpinCtrlKillFocus ), NULL, this );
	m_unitSpinCtrl->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnUnitSpinCtrl ), NULL, this );
	m_unitSpinCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnUnitSpinCtrlText ), NULL, this );
	m_unitSpinCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnUnitSpinCtrlEnter ), NULL, this );
	m_OptionPartsInterchangeable->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_radioSingle->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnBodyStyle ), NULL, this );
	m_radioDeMorgan->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnBodyStyle ), NULL, this );
	m_radioCustom->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnBodyStyle ), NULL, this );
	m_bpAddBodyStyle->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnAddBodyStyle ), NULL, this );
	m_bpMoveUpBodyStyle->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnBodyStyleMoveUp ), NULL, this );
	m_bpMoveDownBodyStyle->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnBodyStyleMoveDown ), NULL, this );
	m_bpDeleteBodyStyle->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnDeleteBodyStyle ), NULL, this );
	m_FootprintFilterListBox->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnFpFilterDClick ), NULL, this );
	m_FootprintFilterListBox->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnEditFootprintFilter ), NULL, this );
	m_addFilterButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnAddFootprintFilter ), NULL, this );
	m_editFilterButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnEditFootprintFilter ), NULL, this );
	m_bpAddJumperGroup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnAddJumperGroup ), NULL, this );
	m_bpRemoveJumperGroup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnRemoveJumperGroup ), NULL, this );
	m_spiceFieldsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnEditSpiceModel ), NULL, this );
	m_stdSizerButtonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES_BASE::OnCancelButtonClick ), NULL, this );

}

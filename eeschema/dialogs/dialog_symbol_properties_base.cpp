///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_symbol_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SYMBOL_PROPERTIES_BASE::DIALOG_SYMBOL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_notebook1 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	generalPage = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* generalPageSizer;
	generalPageSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbFields;
	sbFields = new wxStaticBoxSizer( new wxStaticBox( generalPage, wxID_ANY, _("Fields") ), wxVERTICAL );

	m_fieldsGrid = new WX_GRID( sbFields->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_fieldsGrid->CreateGrid( 4, 14 );
	m_fieldsGrid->EnableEditing( true );
	m_fieldsGrid->EnableGridLines( true );
	m_fieldsGrid->EnableDragGridSize( false );
	m_fieldsGrid->SetMargins( 0, 0 );

	// Columns
	m_fieldsGrid->SetColSize( 0, 72 );
	m_fieldsGrid->SetColSize( 1, 10 );
	m_fieldsGrid->SetColSize( 2, 48 );
	m_fieldsGrid->SetColSize( 3, 84 );
	m_fieldsGrid->SetColSize( 4, 66 );
	m_fieldsGrid->SetColSize( 5, 66 );
	m_fieldsGrid->SetColSize( 6, 48 );
	m_fieldsGrid->SetColSize( 7, 48 );
	m_fieldsGrid->SetColSize( 8, 84 );
	m_fieldsGrid->SetColSize( 9, 84 );
	m_fieldsGrid->SetColSize( 10, 84 );
	m_fieldsGrid->SetColSize( 11, 84 );
	m_fieldsGrid->SetColSize( 12, 10 );
	m_fieldsGrid->SetColSize( 13, 48 );
	m_fieldsGrid->EnableDragColMove( false );
	m_fieldsGrid->EnableDragColSize( true );
	m_fieldsGrid->SetColLabelValue( 0, _("Name") );
	m_fieldsGrid->SetColLabelValue( 1, _("Value") );
	m_fieldsGrid->SetColLabelValue( 2, _("Show") );
	m_fieldsGrid->SetColLabelValue( 3, _("Show Name") );
	m_fieldsGrid->SetColLabelValue( 4, _("H Align") );
	m_fieldsGrid->SetColLabelValue( 5, _("V Align") );
	m_fieldsGrid->SetColLabelValue( 6, _("Italic") );
	m_fieldsGrid->SetColLabelValue( 7, _("Bold") );
	m_fieldsGrid->SetColLabelValue( 8, _("Text Size") );
	m_fieldsGrid->SetColLabelValue( 9, _("Orientation") );
	m_fieldsGrid->SetColLabelValue( 10, _("X Position") );
	m_fieldsGrid->SetColLabelValue( 11, _("Y Position") );
	m_fieldsGrid->SetColLabelValue( 12, _("Font") );
	m_fieldsGrid->SetColLabelValue( 13, _("Color") );
	m_fieldsGrid->SetColLabelSize( 22 );
	m_fieldsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_fieldsGrid->EnableDragRowSize( true );
	m_fieldsGrid->SetRowLabelSize( 0 );
	m_fieldsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_fieldsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	sbFields->Add( m_fieldsGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bButtonSize;
	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpAdd->SetToolTip( _("Add field") );

	bButtonSize->Add( m_bpAdd, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveUp = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveUp->SetToolTip( _("Move up") );

	bButtonSize->Add( m_bpMoveUp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveDown = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveDown->SetToolTip( _("Move down") );

	bButtonSize->Add( m_bpMoveDown, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 10 );

	m_bpDelete = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpDelete->SetToolTip( _("Delete field") );

	bButtonSize->Add( m_bpDelete, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSize->Add( 0, 0, 1, wxEXPAND, 5 );


	sbFields->Add( bButtonSize, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	generalPageSizer->Add( sbFields, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbGeneralProps;
	sbGeneralProps = new wxStaticBoxSizer( new wxStaticBox( generalPage, wxID_ANY, _("General") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,12 ) );

	m_unitLabel = new wxStaticText( sbGeneralProps->GetStaticBox(), wxID_ANY, _("Unit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabel->Wrap( -1 );
	gbSizer1->Add( m_unitLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_unitChoiceChoices;
	m_unitChoice = new wxChoice( sbGeneralProps->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_unitChoiceChoices, 0 );
	m_unitChoice->SetSelection( 0 );
	m_unitChoice->SetMinSize( wxSize( 100,-1 ) );

	gbSizer1->Add( m_unitChoice, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxRIGHT, 5 );

	m_bodyStyle = new wxStaticText( sbGeneralProps->GetStaticBox(), wxID_ANY, _("Body style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bodyStyle->Wrap( -1 );
	gbSizer1->Add( m_bodyStyle, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxArrayString m_bodyStyleChoiceChoices;
	m_bodyStyleChoice = new wxChoice( sbGeneralProps->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_bodyStyleChoiceChoices, 0 );
	m_bodyStyleChoice->SetSelection( 0 );
	gbSizer1->Add( m_bodyStyleChoice, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_orientationLabel = new wxStaticText( sbGeneralProps->GetStaticBox(), wxID_ANY, _("Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_orientationLabel->Wrap( -1 );
	gbSizer1->Add( m_orientationLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_orientationCtrlChoices[] = { _("0"), _("+90"), _("-90"), _("180") };
	int m_orientationCtrlNChoices = sizeof( m_orientationCtrlChoices ) / sizeof( wxString );
	m_orientationCtrl = new wxChoice( sbGeneralProps->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_orientationCtrlNChoices, m_orientationCtrlChoices, 0 );
	m_orientationCtrl->SetSelection( 0 );
	gbSizer1->Add( m_orientationCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_mirrorLabel = new wxStaticText( sbGeneralProps->GetStaticBox(), wxID_ANY, _("Mirror:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mirrorLabel->Wrap( -1 );
	gbSizer1->Add( m_mirrorLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_mirrorCtrlChoices[] = { _("Not mirrored"), _("Around X axis"), _("Around Y axis") };
	int m_mirrorCtrlNChoices = sizeof( m_mirrorCtrlChoices ) / sizeof( wxString );
	m_mirrorCtrl = new wxChoice( sbGeneralProps->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_mirrorCtrlNChoices, m_mirrorCtrlChoices, 0 );
	m_mirrorCtrl->SetSelection( 0 );
	gbSizer1->Add( m_mirrorCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxRIGHT, 5 );


	gbSizer1->AddGrowableCol( 1 );

	sbGeneralProps->Add( gbSizer1, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );

	m_ShowPinNumButt = new wxCheckBox( sbGeneralProps->GetStaticBox(), wxID_ANY, _("Show pin numbers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ShowPinNumButt->SetValue(true);
	m_ShowPinNumButt->SetToolTip( _("Show or hide pin numbers") );

	bSizer11->Add( m_ShowPinNumButt, 1, wxALL, 3 );

	m_ShowPinNameButt = new wxCheckBox( sbGeneralProps->GetStaticBox(), wxID_ANY, _("Show pin names"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ShowPinNameButt->SetValue(true);
	m_ShowPinNameButt->SetToolTip( _("Show or hide pin names") );

	bSizer11->Add( m_ShowPinNameButt, 1, wxALL, 3 );


	sbGeneralProps->Add( bSizer11, 0, wxEXPAND|wxTOP, 13 );


	bLowerSizer->Add( sbGeneralProps, 4, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bMiddleCol;
	bMiddleCol = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbAttributes;
	sbAttributes = new wxStaticBoxSizer( new wxStaticBox( generalPage, wxID_ANY, _("Attributes") ), wxVERTICAL );

	m_cbExcludeFromSim = new wxCheckBox( sbAttributes->GetStaticBox(), wxID_ANY, _("Exclude from simulation"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAttributes->Add( m_cbExcludeFromSim, 0, wxRIGHT|wxLEFT, 5 );


	sbAttributes->Add( 0, 10, 0, wxEXPAND, 5 );

	m_cbExcludeFromBom = new wxCheckBox( sbAttributes->GetStaticBox(), wxID_ANY, _("Exclude from bill of materials"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExcludeFromBom->SetToolTip( _("This is useful for adding symbols for board footprints such as fiducials\nand logos that you do not want to appear in the bill of materials export") );

	sbAttributes->Add( m_cbExcludeFromBom, 0, wxALL, 5 );

	m_cbExcludeFromBoard = new wxCheckBox( sbAttributes->GetStaticBox(), wxID_ANY, _("Exclude from board"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExcludeFromBoard->SetToolTip( _("This is useful for adding symbols that only get exported to the bill of materials but\nnot required to layout the board such as mechanical fasteners and enclosures") );

	sbAttributes->Add( m_cbExcludeFromBoard, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbExcludeFromPosFiles = new wxCheckBox( sbAttributes->GetStaticBox(), wxID_ANY, _("Exclude from position files"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExcludeFromPosFiles->SetToolTip( _("This is useful for adding symbols that should not be included in the \nexported position files used for pick and place machines") );

	sbAttributes->Add( m_cbExcludeFromPosFiles, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbDNP = new wxCheckBox( sbAttributes->GetStaticBox(), wxID_ANY, _("Do not populate"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAttributes->Add( m_cbDNP, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bMiddleCol->Add( sbAttributes, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bLowerSizer->Add( bMiddleCol, 3, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* buttonsSizer;
	buttonsSizer = new wxBoxSizer( wxVERTICAL );

	m_updateSymbolBtn = new wxButton( generalPage, wxID_ANY, _("Update Symbol from Library..."), wxDefaultPosition, wxDefaultSize, 0 );
	buttonsSizer->Add( m_updateSymbolBtn, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_changeSymbolBtn = new wxButton( generalPage, wxID_ANY, _("Change Symbol..."), wxDefaultPosition, wxDefaultSize, 0 );
	buttonsSizer->Add( m_changeSymbolBtn, 0, wxEXPAND|wxALL, 5 );

	m_editSchematicSymbolBtn = new wxButton( generalPage, wxID_ANY, _("Edit Symbol..."), wxDefaultPosition, wxDefaultSize, 0 );
	buttonsSizer->Add( m_editSchematicSymbolBtn, 0, wxEXPAND|wxALL, 5 );


	buttonsSizer->Add( 0, 20, 0, wxEXPAND, 5 );

	m_editLibrarySymbolBtn = new wxButton( generalPage, wxID_ANY, _("Edit Library Symbol..."), wxDefaultPosition, wxDefaultSize, 0 );
	buttonsSizer->Add( m_editLibrarySymbolBtn, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bLowerSizer->Add( buttonsSizer, 3, wxEXPAND|wxALL, 5 );


	generalPageSizer->Add( bLowerSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	generalPage->SetSizer( generalPageSizer );
	generalPage->Layout();
	generalPageSizer->Fit( generalPage );
	m_notebook1->AddPage( generalPage, _("General"), true );
	m_pinTablePage = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* pinTableSizer;
	pinTableSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_pinGrid = new WX_GRID( m_pinTablePage, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), 0 );

	// Grid
	m_pinGrid->CreateGrid( 5, 5 );
	m_pinGrid->EnableEditing( true );
	m_pinGrid->EnableGridLines( true );
	m_pinGrid->EnableDragGridSize( false );
	m_pinGrid->SetMargins( 0, 0 );

	// Columns
	m_pinGrid->SetColSize( 0, 160 );
	m_pinGrid->SetColSize( 1, 160 );
	m_pinGrid->SetColSize( 2, 160 );
	m_pinGrid->SetColSize( 3, 140 );
	m_pinGrid->SetColSize( 4, 140 );
	m_pinGrid->EnableDragColMove( false );
	m_pinGrid->EnableDragColSize( true );
	m_pinGrid->SetColLabelValue( 0, _("Pin Number") );
	m_pinGrid->SetColLabelValue( 1, _("Base Pin Name") );
	m_pinGrid->SetColLabelValue( 2, _("Alternate Assignment") );
	m_pinGrid->SetColLabelValue( 3, _("Electrical Type") );
	m_pinGrid->SetColLabelValue( 4, _("Graphic Style") );
	m_pinGrid->SetColLabelSize( 24 );
	m_pinGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_pinGrid->EnableDragRowSize( false );
	m_pinGrid->SetRowLabelSize( 0 );
	m_pinGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_pinGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bMargins->Add( m_pinGrid, 1, wxEXPAND|wxALL|wxFIXED_MINSIZE, 5 );


	pinTableSizer->Add( bMargins, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_pinTablePage->SetSizer( pinTableSizer );
	m_pinTablePage->Layout();
	pinTableSizer->Fit( m_pinTablePage );
	m_notebook1->AddPage( m_pinTablePage, _("Pin Functions"), false );

	mainSizer->Add( m_notebook1, 1, wxEXPAND|wxALL, 10 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_libraryIDLabel = new wxStaticText( this, wxID_ANY, _("Library link:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_libraryIDLabel->Wrap( -1 );
	bSizerBottom->Add( m_libraryIDLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxTOP, 2 );

	m_tcLibraryID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxBORDER_NONE );
	bSizerBottom->Add( m_tcLibraryID, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizerBottom->Add( 10, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spiceFieldsButton = new wxButton( this, wxID_ANY, _("Simulation Model..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_spiceFieldsButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 15 );

	m_stdDialogButtonSizer = new wxStdDialogButtonSizer();
	m_stdDialogButtonSizerOK = new wxButton( this, wxID_OK );
	m_stdDialogButtonSizer->AddButton( m_stdDialogButtonSizerOK );
	m_stdDialogButtonSizerCancel = new wxButton( this, wxID_CANCEL );
	m_stdDialogButtonSizer->AddButton( m_stdDialogButtonSizerCancel );
	m_stdDialogButtonSizer->Realize();

	bSizerBottom->Add( m_stdDialogButtonSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	mainSizer->Add( bSizerBottom, 0, wxEXPAND|wxLEFT, 12 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnUpdateUI ) );
	m_notebook1->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, wxNotebookEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnPageChanging ), NULL, this );
	m_fieldsGrid->Connect( wxEVT_GRID_EDITOR_HIDDEN, wxGridEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnGridEditorHidden ), NULL, this );
	m_fieldsGrid->Connect( wxEVT_GRID_EDITOR_SHOWN, wxGridEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnGridEditorShown ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_unitChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnUnitChoice ), NULL, this );
	m_orientationCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnChoice ), NULL, this );
	m_mirrorCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnChoice ), NULL, this );
	m_ShowPinNumButt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_ShowPinNameButt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromSim->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromBom->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromBoard->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromPosFiles->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_updateSymbolBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnUpdateSymbol ), NULL, this );
	m_changeSymbolBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnExchangeSymbol ), NULL, this );
	m_editSchematicSymbolBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnEditSymbol ), NULL, this );
	m_editSchematicSymbolBtn->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::onUpdateEditSymbol ), NULL, this );
	m_editLibrarySymbolBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnEditLibrarySymbol ), NULL, this );
	m_editLibrarySymbolBtn->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::onUpdateEditLibrarySymbol ), NULL, this );
	m_pinGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnPinTableCellEdited ), NULL, this );
	m_pinGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnSizePinsGrid ), NULL, this );
	m_spiceFieldsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnEditSpiceModel ), NULL, this );
	m_stdDialogButtonSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCancelButtonClick ), NULL, this );
}

DIALOG_SYMBOL_PROPERTIES_BASE::~DIALOG_SYMBOL_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnUpdateUI ) );
	m_notebook1->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, wxNotebookEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnPageChanging ), NULL, this );
	m_fieldsGrid->Disconnect( wxEVT_GRID_EDITOR_HIDDEN, wxGridEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnGridEditorHidden ), NULL, this );
	m_fieldsGrid->Disconnect( wxEVT_GRID_EDITOR_SHOWN, wxGridEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnGridEditorShown ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_unitChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnUnitChoice ), NULL, this );
	m_orientationCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnChoice ), NULL, this );
	m_mirrorCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnChoice ), NULL, this );
	m_ShowPinNumButt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_ShowPinNameButt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromSim->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromBom->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromBoard->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromPosFiles->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_updateSymbolBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnUpdateSymbol ), NULL, this );
	m_changeSymbolBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnExchangeSymbol ), NULL, this );
	m_editSchematicSymbolBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnEditSymbol ), NULL, this );
	m_editSchematicSymbolBtn->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::onUpdateEditSymbol ), NULL, this );
	m_editLibrarySymbolBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnEditLibrarySymbol ), NULL, this );
	m_editLibrarySymbolBtn->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::onUpdateEditLibrarySymbol ), NULL, this );
	m_pinGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnPinTableCellEdited ), NULL, this );
	m_pinGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnSizePinsGrid ), NULL, this );
	m_spiceFieldsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnEditSpiceModel ), NULL, this );
	m_stdDialogButtonSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_PROPERTIES_BASE::OnCancelButtonClick ), NULL, this );

}

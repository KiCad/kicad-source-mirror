///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"
#include "widgets/wx_infobar.h"

#include "dialog_generated_table_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GENERATED_TABLE_PROPERTIES_BASE::DIALOG_GENERATED_TABLE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMain;
	bMain = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	bMain->Add( m_infoBar, 0, wxEXPAND, 0 );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_generalPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bGeneral;
	bGeneral = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgGeneral;
	fgGeneral = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgGeneral->AddGrowableCol( 1 );
	fgGeneral->SetFlexibleDirection( wxBOTH );
	fgGeneral->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_layerLabel = new wxStaticText( m_generalPanel, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_layerLabel->Wrap( -1 );
	fgGeneral->Add( m_layerLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_LayerSelectionCtrl = new PCB_LAYER_BOX_SELECTOR( m_generalPanel, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_LayerSelectionCtrl->SetMinSize( wxSize( 175,-1 ) );

	fgGeneral->Add( m_LayerSelectionCtrl, 0, wxEXPAND|wxRIGHT, 5 );

	m_unitsLabel = new wxStaticText( m_generalPanel, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsLabel->Wrap( -1 );
	fgGeneral->Add( m_unitsLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_unitsCtrlChoices[] = { _("mm"), _("inch"), _("mils") };
	int m_unitsCtrlNChoices = sizeof( m_unitsCtrlChoices ) / sizeof( wxString );
	m_unitsCtrl = new wxChoice( m_generalPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_unitsCtrlNChoices, m_unitsCtrlChoices, 0 );
	m_unitsCtrl->SetSelection( 0 );
	fgGeneral->Add( m_unitsCtrl, 0, wxEXPAND|wxRIGHT, 5 );

	m_precisionLabel = new wxStaticText( m_generalPanel, wxID_ANY, _("Decimal places:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_precisionLabel->Wrap( -1 );
	fgGeneral->Add( m_precisionLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_precisionCtrl = new wxSpinCtrl( m_generalPanel, wxID_ANY, wxT("3"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 6, 0 );
	fgGeneral->Add( m_precisionCtrl, 0, wxRIGHT, 5 );


	bGeneral->Add( fgGeneral, 0, wxEXPAND|wxALL, 5 );

	m_cbLocked = new wxCheckBox( m_generalPanel, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	bGeneral->Add( m_cbLocked, 0, wxALL|wxEXPAND, 5 );


	m_generalPanel->SetSizer( bGeneral );
	m_generalPanel->Layout();
	bGeneral->Fit( m_generalPanel );
	m_notebook->AddPage( m_generalPanel, _("General"), false );
	m_columnsPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bColumns;
	bColumns = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bColumnsTop;
	bColumnsTop = new wxBoxSizer( wxHORIZONTAL );

	m_columnGrid = new WX_GRID( m_columnsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_columnGrid->CreateGrid( 0, 5 );
	m_columnGrid->EnableEditing( true );
	m_columnGrid->EnableGridLines( true );
	m_columnGrid->SetGridLineColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	m_columnGrid->EnableDragGridSize( false );
	m_columnGrid->SetMargins( 0, 0 );

	// Columns
	m_columnGrid->EnableDragColMove( false );
	m_columnGrid->EnableDragColSize( true );
	m_columnGrid->SetColLabelSize( 22 );
	m_columnGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_columnGrid->EnableDragRowSize( false );
	m_columnGrid->SetRowLabelSize( 0 );
	m_columnGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_columnGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_columnGrid->SetMinSize( wxSize( -1,220 ) );

	bColumnsTop->Add( m_columnGrid, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bColButtons;
	bColButtons = new wxBoxSizer( wxVERTICAL );

	m_moveUpButton = new wxButton( m_columnsPanel, wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	bColButtons->Add( m_moveUpButton, 0, wxEXPAND|wxBOTTOM, 5 );

	m_moveDownButton = new wxButton( m_columnsPanel, wxID_ANY, _("Move Down"), wxDefaultPosition, wxDefaultSize, 0 );
	bColButtons->Add( m_moveDownButton, 0, wxEXPAND|wxBOTTOM, 5 );

	m_resetColumnsButton = new wxButton( m_columnsPanel, wxID_ANY, _("Reset to Default"), wxDefaultPosition, wxDefaultSize, 0 );
	bColButtons->Add( m_resetColumnsButton, 0, wxEXPAND|wxTOP, 5 );


	bColumnsTop->Add( bColButtons, 0, wxALIGN_TOP|wxALL, 5 );


	bColumns->Add( bColumnsTop, 1, wxEXPAND, 0 );

	m_optionsSizer = new wxBoxSizer( wxVERTICAL );


	bColumns->Add( m_optionsSizer, 0, wxEXPAND, 0 );


	m_columnsPanel->SetSizer( bColumns );
	m_columnsPanel->Layout();
	bColumns->Fit( m_columnsPanel );
	m_notebook->AddPage( m_columnsPanel, _("Columns"), false );
	m_appearancePanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bAppearance;
	bAppearance = new wxBoxSizer( wxVERTICAL );

	m_borderCheckbox = new wxCheckBox( m_appearancePanel, wxID_ANY, _("External border"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderCheckbox->SetValue(true);
	bAppearance->Add( m_borderCheckbox, 0, wxALL|wxEXPAND, 5 );

	m_headerBorder = new wxCheckBox( m_appearancePanel, wxID_ANY, _("Header separator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_headerBorder->SetValue(true);
	bAppearance->Add( m_headerBorder, 0, wxALL|wxEXPAND, 5 );

	m_rowSeparators = new wxCheckBox( m_appearancePanel, wxID_ANY, _("Row separators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rowSeparators->SetValue(true);
	bAppearance->Add( m_rowSeparators, 0, wxALL|wxEXPAND, 5 );

	m_colSeparators = new wxCheckBox( m_appearancePanel, wxID_ANY, _("Column separators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_colSeparators->SetValue(true);
	bAppearance->Add( m_colSeparators, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgAppearance;
	fgAppearance = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgAppearance->AddGrowableCol( 1 );
	fgAppearance->SetFlexibleDirection( wxBOTH );
	fgAppearance->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_borderWidthLabel = new wxStaticText( m_appearancePanel, wxID_ANY, _("Border width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthLabel->Wrap( -1 );
	fgAppearance->Add( m_borderWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_borderWidthCtrl = new wxTextCtrl( m_appearancePanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgAppearance->Add( m_borderWidthCtrl, 0, wxEXPAND, 5 );

	m_borderWidthUnits = new wxStaticText( m_appearancePanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthUnits->Wrap( -1 );
	fgAppearance->Add( m_borderWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_separatorsWidthLabel = new wxStaticText( m_appearancePanel, wxID_ANY, _("Separator width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsWidthLabel->Wrap( -1 );
	fgAppearance->Add( m_separatorsWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_separatorsWidthCtrl = new wxTextCtrl( m_appearancePanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgAppearance->Add( m_separatorsWidthCtrl, 0, wxEXPAND, 5 );

	m_separatorsWidthUnits = new wxStaticText( m_appearancePanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsWidthUnits->Wrap( -1 );
	fgAppearance->Add( m_separatorsWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bAppearance->Add( fgAppearance, 0, wxEXPAND|wxALL, 5 );


	m_appearancePanel->SetSizer( bAppearance );
	m_appearancePanel->Layout();
	bAppearance->Fit( m_appearancePanel );
	m_notebook->AddPage( m_appearancePanel, _("Appearance"), false );

	bMain->Add( m_notebook, 1, wxEXPAND|wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMain->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMain );
	this->Layout();
	bMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_moveUpButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATED_TABLE_PROPERTIES_BASE::onMoveUp ), NULL, this );
	m_moveDownButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATED_TABLE_PROPERTIES_BASE::onMoveDown ), NULL, this );
	m_resetColumnsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATED_TABLE_PROPERTIES_BASE::onResetColumns ), NULL, this );
	m_borderCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GENERATED_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );
}

DIALOG_GENERATED_TABLE_PROPERTIES_BASE::~DIALOG_GENERATED_TABLE_PROPERTIES_BASE()
{
	// Disconnect Events
	m_moveUpButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATED_TABLE_PROPERTIES_BASE::onMoveUp ), NULL, this );
	m_moveDownButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATED_TABLE_PROPERTIES_BASE::onMoveDown ), NULL, this );
	m_resetColumnsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATED_TABLE_PROPERTIES_BASE::onResetColumns ), NULL, this );
	m_borderCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GENERATED_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );

}

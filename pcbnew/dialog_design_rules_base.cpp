///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_design_rules_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DESIGN_RULES_BASE::DIALOG_DESIGN_RULES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 600,450 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelLayers = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bMainSizerLayers;
	bMainSizerLayers = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_LayersCountSelectionChoices[] = { _("1"), _("2"), _("4"), _("6"), _("8"), _("10"), _("12"), _("14"), _("16") };
	int m_LayersCountSelectionNChoices = sizeof( m_LayersCountSelectionChoices ) / sizeof( wxString );
	m_LayersCountSelection = new wxRadioBox( m_panelLayers, ID_LAYERS_COUNT_SELECTION, _("Layers Count"), wxDefaultPosition, wxDefaultSize, m_LayersCountSelectionNChoices, m_LayersCountSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_LayersCountSelection->SetSelection( 1 );
	bMainSizerLayers->Add( m_LayersCountSelection, 0, wxALL, 5 );
	
	m_gridLayersProperties = new wxGrid( m_panelLayers, ID_LAYERS_PROPERTIES, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridLayersProperties->CreateGrid( 16, 3 );
	m_gridLayersProperties->EnableEditing( true );
	m_gridLayersProperties->EnableGridLines( true );
	m_gridLayersProperties->EnableDragGridSize( false );
	m_gridLayersProperties->SetMargins( 0, 0 );
	
	// Columns
	m_gridLayersProperties->SetColSize( 0, 100 );
	m_gridLayersProperties->SetColSize( 1, 100 );
	m_gridLayersProperties->SetColSize( 2, 150 );
	m_gridLayersProperties->EnableDragColMove( false );
	m_gridLayersProperties->EnableDragColSize( true );
	m_gridLayersProperties->SetColLabelSize( 30 );
	m_gridLayersProperties->SetColLabelValue( 0, _("Active") );
	m_gridLayersProperties->SetColLabelValue( 1, _("Status") );
	m_gridLayersProperties->SetColLabelValue( 2, _("Name") );
	m_gridLayersProperties->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridLayersProperties->AutoSizeRows();
	m_gridLayersProperties->EnableDragRowSize( true );
	m_gridLayersProperties->SetRowLabelSize( 80 );
	m_gridLayersProperties->SetRowLabelValue( 0, _("Top Layer") );
	m_gridLayersProperties->SetRowLabelValue( 1, _("Inner 14") );
	m_gridLayersProperties->SetRowLabelValue( 2, _("Inner 13") );
	m_gridLayersProperties->SetRowLabelValue( 3, _("Inner 12") );
	m_gridLayersProperties->SetRowLabelValue( 4, _("Inner 11") );
	m_gridLayersProperties->SetRowLabelValue( 5, _("Inner 10") );
	m_gridLayersProperties->SetRowLabelValue( 6, _("Inner 9") );
	m_gridLayersProperties->SetRowLabelValue( 7, _("Inner 8") );
	m_gridLayersProperties->SetRowLabelValue( 8, _("Inner 7") );
	m_gridLayersProperties->SetRowLabelValue( 9, _("Inner 6") );
	m_gridLayersProperties->SetRowLabelValue( 10, _("Inner 5") );
	m_gridLayersProperties->SetRowLabelValue( 11, _("Inner 4") );
	m_gridLayersProperties->SetRowLabelValue( 12, _("Inner 3") );
	m_gridLayersProperties->SetRowLabelValue( 13, _("Inner 2") );
	m_gridLayersProperties->SetRowLabelValue( 14, _("Inner 1") );
	m_gridLayersProperties->SetRowLabelValue( 15, _("Bottom Layer") );
	m_gridLayersProperties->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridLayersProperties->SetDefaultCellAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	bMainSizerLayers->Add( m_gridLayersProperties, 1, wxALL|wxEXPAND, 5 );
	
	m_panelLayers->SetSizer( bMainSizerLayers );
	m_panelLayers->Layout();
	bMainSizerLayers->Fit( m_panelLayers );
	m_notebook->AddPage( m_panelLayers, _("Layers"), true );
	m_panelNetClasses = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bmainSizerNclasses;
	bmainSizerNclasses = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_panelNetClasses, wxID_ANY, _("Net classes:") ), wxHORIZONTAL );
	
	m_gridNetClassesProperties = new wxGrid( m_panelNetClasses, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridNetClassesProperties->CreateGrid( 1, 5 );
	m_gridNetClassesProperties->EnableEditing( true );
	m_gridNetClassesProperties->EnableGridLines( true );
	m_gridNetClassesProperties->EnableDragGridSize( false );
	m_gridNetClassesProperties->SetMargins( 0, 0 );
	
	// Columns
	m_gridNetClassesProperties->SetColSize( 0, 100 );
	m_gridNetClassesProperties->SetColSize( 1, 100 );
	m_gridNetClassesProperties->SetColSize( 2, 100 );
	m_gridNetClassesProperties->SetColSize( 3, 100 );
	m_gridNetClassesProperties->SetColSize( 4, 100 );
	m_gridNetClassesProperties->EnableDragColMove( false );
	m_gridNetClassesProperties->EnableDragColSize( true );
	m_gridNetClassesProperties->SetColLabelSize( 30 );
	m_gridNetClassesProperties->SetColLabelValue( 0, _("Track size") );
	m_gridNetClassesProperties->SetColLabelValue( 1, _("Vias size") );
	m_gridNetClassesProperties->SetColLabelValue( 2, _("Clearance") );
	m_gridNetClassesProperties->SetColLabelValue( 3, _("Track Min Size") );
	m_gridNetClassesProperties->SetColLabelValue( 4, _("Via Min Size") );
	m_gridNetClassesProperties->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridNetClassesProperties->AutoSizeRows();
	m_gridNetClassesProperties->EnableDragRowSize( true );
	m_gridNetClassesProperties->SetRowLabelSize( 80 );
	m_gridNetClassesProperties->SetRowLabelValue( 0, _("Default") );
	m_gridNetClassesProperties->SetRowLabelValue( 1, _("Special") );
	m_gridNetClassesProperties->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridNetClassesProperties->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_gridNetClassesProperties->SetMinSize( wxSize( -1,100 ) );
	
	sbSizer1->Add( m_gridNetClassesProperties, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxVERTICAL );
	
	m_buttonADD = new wxButton( m_panelNetClasses, wxID_ADD_NETCLASS, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonADD, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonRemove = new wxButton( m_panelNetClasses, wxID_REMOVE_NETCLASS, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonRemove, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	sbSizer1->Add( bSizerButtons, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bmainSizerNclasses->Add( sbSizer1, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerNetSelect;
	bSizerNetSelect = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizerNetSelect;
	bLeftSizerNetSelect = new wxBoxSizer( wxVERTICAL );
	
	wxArrayString m_CBoxLeftSelectionChoices;
	m_CBoxLeftSelection = new wxChoice( m_panelNetClasses, ID_LEFT_CHOICE_CLICK, wxDefaultPosition, wxDefaultSize, m_CBoxLeftSelectionChoices, 0 );
	m_CBoxLeftSelection->SetSelection( 0 );
	bLeftSizerNetSelect->Add( m_CBoxLeftSelection, 0, wxALL|wxEXPAND, 5 );
	
	m_listBoxLeftNetSelect = new wxListBox( m_panelNetClasses, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED|wxLB_MULTIPLE ); 
	bLeftSizerNetSelect->Add( m_listBoxLeftNetSelect, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	bSizerNetSelect->Add( bLeftSizerNetSelect, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bmiddleSizerNetSelect;
	bmiddleSizerNetSelect = new wxBoxSizer( wxVERTICAL );
	
	m_buttonRightToLeft = new wxButton( m_panelNetClasses, ID_LEFT_TO_RIGHT_COPY, _("<<<"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizerNetSelect->Add( m_buttonRightToLeft, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );
	
	m_buttonLeftToRight = new wxButton( m_panelNetClasses, ID_RIGHT_TO_LEFT_COPY, _(">>>"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizerNetSelect->Add( m_buttonLeftToRight, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerButtonsSelecAll;
	bSizerButtonsSelecAll = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonLeftSelAll = new wxButton( m_panelNetClasses, wxID_ANY, _("<< Select All"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsSelecAll->Add( m_buttonLeftSelAll, 0, wxTOP|wxBOTTOM, 5 );
	
	m_buttonRightSelAll = new wxButton( m_panelNetClasses, wxID_ANY, _("Select All >>"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsSelecAll->Add( m_buttonRightSelAll, 0, wxALIGN_RIGHT|wxALIGN_BOTTOM|wxTOP|wxBOTTOM, 5 );
	
	bmiddleSizerNetSelect->Add( bSizerButtonsSelecAll, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizerNetSelect->Add( bmiddleSizerNetSelect, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bLeftSizerNetSelect1;
	bLeftSizerNetSelect1 = new wxBoxSizer( wxVERTICAL );
	
	wxArrayString m_CBoxRightSelectionChoices;
	m_CBoxRightSelection = new wxChoice( m_panelNetClasses, ID_RIGHT_CHOICE_CLICK, wxDefaultPosition, wxDefaultSize, m_CBoxRightSelectionChoices, 0 );
	m_CBoxRightSelection->SetSelection( 0 );
	bLeftSizerNetSelect1->Add( m_CBoxRightSelection, 0, wxALL|wxEXPAND, 5 );
	
	m_listBoxRightNetSelect = new wxListBox( m_panelNetClasses, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED|wxLB_MULTIPLE ); 
	bLeftSizerNetSelect1->Add( m_listBoxRightNetSelect, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	bSizerNetSelect->Add( bLeftSizerNetSelect1, 1, wxEXPAND, 5 );
	
	bmainSizerNclasses->Add( bSizerNetSelect, 1, wxEXPAND, 5 );
	
	m_staticTextMsg = new wxStaticText( m_panelNetClasses, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMsg->Wrap( -1 );
	bmainSizerNclasses->Add( m_staticTextMsg, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MessagesList = new wxHtmlWindow( m_panelNetClasses, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO|wxSUNKEN_BORDER );
	m_MessagesList->SetMinSize( wxSize( -1,100 ) );
	
	bmainSizerNclasses->Add( m_MessagesList, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_panelNetClasses->SetSizer( bmainSizerNclasses );
	m_panelNetClasses->Layout();
	bmainSizerNclasses->Fit( m_panelNetClasses );
	m_notebook->AddPage( m_panelNetClasses, _("Net Classes"), false );
	
	bMainSizer->Add( m_notebook, 1, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bMainSizer->Add( m_sdbSizer1, 0, wxALIGN_RIGHT, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_LayersCountSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnLayerCountClick ), NULL, this );
	m_gridLayersProperties->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_DESIGN_RULES_BASE::OnLayerGridLeftClick ), NULL, this );
	m_gridLayersProperties->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_DESIGN_RULES_BASE::OnLayerGridRighttClick ), NULL, this );
	m_gridNetClassesProperties->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_DESIGN_RULES_BASE::OnNetClassesGridLeftClick ), NULL, this );
	m_gridNetClassesProperties->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_DESIGN_RULES_BASE::OnNetClassesGridRightClick ), NULL, this );
	m_buttonADD->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnAddNetclassClick ), NULL, this );
	m_buttonRemove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnRemoveNetclassClick ), NULL, this );
	m_CBoxLeftSelection->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnLeftCBSelection ), NULL, this );
	m_buttonRightToLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnRightToLeftCopyButton ), NULL, this );
	m_buttonLeftToRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnLeftToRightCopyButton ), NULL, this );
	m_buttonLeftSelAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnLeftSelectAllButton ), NULL, this );
	m_buttonRightSelAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnRightSelectAllButton ), NULL, this );
	m_CBoxRightSelection->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnRightCBSelection ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnOkButtonClick ), NULL, this );
}

DIALOG_DESIGN_RULES_BASE::~DIALOG_DESIGN_RULES_BASE()
{
	// Disconnect Events
	m_LayersCountSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnLayerCountClick ), NULL, this );
	m_gridLayersProperties->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_DESIGN_RULES_BASE::OnLayerGridLeftClick ), NULL, this );
	m_gridLayersProperties->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_DESIGN_RULES_BASE::OnLayerGridRighttClick ), NULL, this );
	m_gridNetClassesProperties->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_DESIGN_RULES_BASE::OnNetClassesGridLeftClick ), NULL, this );
	m_gridNetClassesProperties->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_DESIGN_RULES_BASE::OnNetClassesGridRightClick ), NULL, this );
	m_buttonADD->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnAddNetclassClick ), NULL, this );
	m_buttonRemove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnRemoveNetclassClick ), NULL, this );
	m_CBoxLeftSelection->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnLeftCBSelection ), NULL, this );
	m_buttonRightToLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnRightToLeftCopyButton ), NULL, this );
	m_buttonLeftToRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnLeftToRightCopyButton ), NULL, this );
	m_buttonLeftSelAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnLeftSelectAllButton ), NULL, this );
	m_buttonRightSelAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnRightSelectAllButton ), NULL, this );
	m_CBoxRightSelection->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnRightCBSelection ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_RULES_BASE::OnOkButtonClick ), NULL, this );
}

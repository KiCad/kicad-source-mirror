///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_symbol_fields_table_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SYMBOL_FIELDS_TABLE_BASE::DIALOG_SYMBOL_FIELDS_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_nbPages = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelEdit = new wxPanel( m_nbPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bEditSizer;
	bEditSizer = new wxBoxSizer( wxVERTICAL );

	m_splitterMainWindow = new wxSplitterWindow( m_panelEdit, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxSP_NO_XP_THEME );
	m_splitterMainWindow->SetMinimumPaneSize( 200 );

	m_leftPanel = new wxPanel( m_splitterMainWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	m_fieldsCtrl = new wxDataViewListCtrl( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_fieldsCtrl->SetMinSize( wxSize( -1,250 ) );

	bLeftSizer->Add( m_fieldsCtrl, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxBoxSizer* bFieldsButtons;
	bFieldsButtons = new wxBoxSizer( wxHORIZONTAL );

	m_addFieldButton = new STD_BITMAP_BUTTON( m_leftPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bFieldsButtons->Add( m_addFieldButton, 0, wxBOTTOM|wxLEFT, 5 );

	m_renameFieldButton = new STD_BITMAP_BUTTON( m_leftPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bFieldsButtons->Add( m_renameFieldButton, 0, wxBOTTOM|wxLEFT, 5 );


	bFieldsButtons->Add( 15, 0, 0, wxEXPAND, 5 );

	m_removeFieldButton = new STD_BITMAP_BUTTON( m_leftPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bFieldsButtons->Add( m_removeFieldButton, 0, wxBOTTOM|wxLEFT, 5 );


	bLeftSizer->Add( bFieldsButtons, 0, wxEXPAND, 5 );

	wxBoxSizer* bPresets;
	bPresets = new wxBoxSizer( wxVERTICAL );

	m_staticline1 = new wxStaticLine( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bPresets->Add( m_staticline1, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_bomPresetsLabel = new wxStaticText( m_leftPanel, wxID_ANY, _("View presets:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bomPresetsLabel->Wrap( -1 );
	bPresets->Add( m_bomPresetsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxString m_cbBomPresetsChoices[] = { _("Default"), _("(unsaved)") };
	int m_cbBomPresetsNChoices = sizeof( m_cbBomPresetsChoices ) / sizeof( wxString );
	m_cbBomPresets = new wxChoice( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbBomPresetsNChoices, m_cbBomPresetsChoices, 0 );
	m_cbBomPresets->SetSelection( 0 );
	bPresets->Add( m_cbBomPresets, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 4 );


	bLeftSizer->Add( bPresets, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	m_leftPanel->SetSizer( bLeftSizer );
	m_leftPanel->Layout();
	bLeftSizer->Fit( m_leftPanel );
	m_rightPanel = new wxPanel( m_splitterMainWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bControls;
	bControls = new wxBoxSizer( wxHORIZONTAL );

	m_filter = new wxSearchCtrl( m_rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_filter->ShowSearchButton( true );
	#endif
	m_filter->ShowCancelButton( true );
	m_filter->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_filter->SetMinSize( wxSize( 140,-1 ) );

	bControls->Add( m_filter, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticline31 = new wxStaticLine( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bControls->Add( m_staticline31, 0, wxEXPAND | wxALL, 3 );

	m_checkExcludeDNP = new wxCheckBox( m_rightPanel, wxID_ANY, _("Exclude DNP"), wxDefaultPosition, wxDefaultSize, 0 );
	bControls->Add( m_checkExcludeDNP, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_checkShowExcluded = new wxCheckBox( m_rightPanel, wxID_ANY, _("Show 'Exclude from BOM'"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowExcluded->SetValue(true);
	bControls->Add( m_checkShowExcluded, 0, wxALL, 5 );

	m_staticline32 = new wxStaticLine( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bControls->Add( m_staticline32, 0, wxEXPAND | wxALL, 3 );

	m_groupSymbolsBox = new wxCheckBox( m_rightPanel, OPT_GROUP_COMPONENTS, _("Group symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_groupSymbolsBox->SetValue(true);
	m_groupSymbolsBox->SetToolTip( _("Group symbols together based on common properties") );

	bControls->Add( m_groupSymbolsBox, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_staticline3 = new wxStaticLine( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bControls->Add( m_staticline3, 0, wxEXPAND | wxALL, 3 );

	m_bRefresh = new STD_BITMAP_BUTTON( m_rightPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bRefresh->SetMinSize( wxSize( 30,30 ) );

	bControls->Add( m_bRefresh, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bRightSizer->Add( bControls, 0, wxEXPAND|wxLEFT|wxTOP, 5 );

	m_grid = new WX_GRID( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 5, 5 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->EnableDragColMove( true );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 24 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_grid->SetMinSize( wxSize( 400,200 ) );

	bRightSizer->Add( m_grid, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticline7 = new wxStaticLine( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightSizer->Add( m_staticline7, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_scopeLabel = new wxStaticText( m_rightPanel, wxID_ANY, _("Scope:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_scopeLabel->Wrap( -1 );
	fgSizer1->Add( m_scopeLabel, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_radioProject = new wxRadioButton( m_rightPanel, wxID_ANY, _("Entire project"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizer1->Add( m_radioProject, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_radioCurrentSheet = new wxRadioButton( m_rightPanel, wxID_ANY, _("Current sheet only"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_radioCurrentSheet, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_radioRecursive = new wxRadioButton( m_rightPanel, wxID_ANY, _("Recursive"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_radioRecursive, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_crossProbeLabel = new wxStaticText( m_rightPanel, wxID_ANY, _("Cross-probe action:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_crossProbeLabel->Wrap( -1 );
	fgSizer1->Add( m_crossProbeLabel, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_radioHighlight = new wxRadioButton( m_rightPanel, wxID_ANY, _("Highlight"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizer1->Add( m_radioHighlight, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_radioSelect = new wxRadioButton( m_rightPanel, wxID_ANY, _("Select"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_radioSelect, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_radioOff = new wxRadioButton( m_rightPanel, wxID_ANY, _("None"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_radioOff, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bRightSizer->Add( fgSizer1, 0, 0, 5 );


	m_rightPanel->SetSizer( bRightSizer );
	m_rightPanel->Layout();
	bRightSizer->Fit( m_rightPanel );
	m_splitterMainWindow->SplitVertically( m_leftPanel, m_rightPanel, -1 );
	bEditSizer->Add( m_splitterMainWindow, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	m_panelEdit->SetSizer( bEditSizer );
	m_panelEdit->Layout();
	bEditSizer->Fit( m_panelEdit );
	m_nbPages->AddPage( m_panelEdit, _("Edit"), true );
	m_panelExport = new wxPanel( m_nbPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxGridBagSizer* gbExport;
	gbExport = new wxGridBagSizer( 0, 5 );
	gbExport->SetFlexibleDirection( wxBOTH );
	gbExport->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxGridBagSizer* gbExportOptions;
	gbExportOptions = new wxGridBagSizer( 4, 5 );
	gbExportOptions->SetFlexibleDirection( wxBOTH );
	gbExportOptions->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelFieldDelimiter = new wxStaticText( m_panelExport, wxID_ANY, _("Field delimiter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelFieldDelimiter->Wrap( -1 );
	gbExportOptions->Add( m_labelFieldDelimiter, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textFieldDelimiter = new wxTextCtrl( m_panelExport, wxID_ANY, _(","), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_TAB );
	m_textFieldDelimiter->SetMinSize( wxSize( 60,-1 ) );

	gbExportOptions->Add( m_textFieldDelimiter, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_labelStringDelimiter = new wxStaticText( m_panelExport, wxID_ANY, _("String delimiter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelStringDelimiter->Wrap( -1 );
	gbExportOptions->Add( m_labelStringDelimiter, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textStringDelimiter = new wxTextCtrl( m_panelExport, wxID_ANY, _("\""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_TAB );
	m_textStringDelimiter->SetMinSize( wxSize( 60,-1 ) );

	gbExportOptions->Add( m_textStringDelimiter, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_labelRefDelimiter = new wxStaticText( m_panelExport, wxID_ANY, _("Reference delimiter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelRefDelimiter->Wrap( -1 );
	gbExportOptions->Add( m_labelRefDelimiter, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textRefDelimiter = new wxTextCtrl( m_panelExport, wxID_ANY, _(","), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_TAB );
	m_textRefDelimiter->SetMinSize( wxSize( 60,-1 ) );

	gbExportOptions->Add( m_textRefDelimiter, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_labelRefRangeDelimiter = new wxStaticText( m_panelExport, wxID_ANY, _("Range delimiter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelRefRangeDelimiter->Wrap( -1 );
	gbExportOptions->Add( m_labelRefRangeDelimiter, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textRefRangeDelimiter = new wxTextCtrl( m_panelExport, wxID_ANY, _("-"), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_TAB );
	m_textRefRangeDelimiter->SetToolTip( _("Leave blank to disable ranges.") );
	m_textRefRangeDelimiter->SetMinSize( wxSize( 60,-1 ) );

	gbExportOptions->Add( m_textRefRangeDelimiter, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_checkKeepTabs = new wxCheckBox( m_panelExport, wxID_ANY, _("Keep tabs"), wxDefaultPosition, wxDefaultSize, 0 );
	gbExportOptions->Add( m_checkKeepTabs, wxGBPosition( 4, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_checkKeepLineBreaks = new wxCheckBox( m_panelExport, wxID_ANY, _("Keep line breaks"), wxDefaultPosition, wxDefaultSize, 0 );
	gbExportOptions->Add( m_checkKeepLineBreaks, wxGBPosition( 5, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_staticline2 = new wxStaticLine( m_panelExport, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	gbExportOptions->Add( m_staticline2, wxGBPosition( 7, 0 ), wxGBSpan( 1, 2 ), wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_labelBomExportPresets = new wxStaticText( m_panelExport, wxID_ANY, _("Format presets:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelBomExportPresets->Wrap( -1 );
	gbExportOptions->Add( m_labelBomExportPresets, wxGBPosition( 8, 0 ), wxGBSpan( 1, 2 ), 0, 5 );

	wxString m_cbBomFmtPresetsChoices[] = { _("Default"), _("(unsaved)") };
	int m_cbBomFmtPresetsNChoices = sizeof( m_cbBomFmtPresetsChoices ) / sizeof( wxString );
	m_cbBomFmtPresets = new wxChoice( m_panelExport, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbBomFmtPresetsNChoices, m_cbBomFmtPresetsChoices, 0 );
	m_cbBomFmtPresets->SetSelection( 0 );
	gbExportOptions->Add( m_cbBomFmtPresets, wxGBPosition( 9, 0 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );


	gbExport->Add( gbExportOptions, wxGBPosition( 0, 0 ), wxGBSpan( 3, 1 ), wxEXPAND|wxALL, 5 );

	wxBoxSizer* bOutputDirectory;
	bOutputDirectory = new wxBoxSizer( wxHORIZONTAL );

	m_labelOutputDirectory = new wxStaticText( m_panelExport, wxID_ANY, _("Output file:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelOutputDirectory->Wrap( -1 );
	bOutputDirectory->Add( m_labelOutputDirectory, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_outputFileName = new wxTextCtrl( m_panelExport, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bOutputDirectory->Add( m_outputFileName, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( m_panelExport, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_browseButton->SetMinSize( wxSize( 30,30 ) );

	bOutputDirectory->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	gbExport->Add( bOutputDirectory, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM, 10 );

	wxBoxSizer* bPreview;
	bPreview = new wxBoxSizer( wxHORIZONTAL );

	m_labelPreview = new wxStaticText( m_panelExport, wxID_ANY, _("Preview:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelPreview->Wrap( -1 );
	bPreview->Add( m_labelPreview, 0, wxALIGN_BOTTOM|wxTOP|wxRIGHT|wxLEFT, 5 );


	bPreview->Add( 0, 0, 1, wxEXPAND, 5 );

	m_bRefreshPreview = new STD_BITMAP_BUTTON( m_panelExport, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bRefreshPreview->SetMinSize( wxSize( 30,30 ) );

	bPreview->Add( m_bRefreshPreview, 0, wxTOP|wxRIGHT, 5 );


	gbExport->Add( bPreview, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxBOTTOM, 3 );

	m_textOutput = new wxTextCtrl( m_panelExport, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTE_MULTILINE|wxTE_READONLY );
	m_textOutput->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	gbExport->Add( m_textOutput, wxGBPosition( 2, 1 ), wxGBSpan( 2, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	gbExport->AddGrowableCol( 1 );
	gbExport->AddGrowableRow( 3 );

	m_panelExport->SetSizer( gbExport );
	m_panelExport->Layout();
	gbExport->Fit( m_panelExport );
	m_nbPages->AddPage( m_panelExport, _("Export"), false );

	bMainSizer->Add( m_nbPages, 1, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );


	bButtonsSizer->Add( 0, 0, 9, wxEXPAND, 5 );

	m_buttonExport = new wxButton( this, wxID_ANY, _("Export"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonExport, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );

	m_buttonApply = new wxButton( this, wxID_ANY, _("Apply, Save Schematic && Continue"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonApply, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bButtonsSizer->Add( m_sdbSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bMainSizer->Add( bButtonsSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnClose ) );
	m_nbPages->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPageChanged ), NULL, this );
	m_fieldsCtrl->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnColumnItemToggled ), NULL, this );
	m_fieldsCtrl->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnFieldsCtrlSelectionChanged ), NULL, this );
	m_fieldsCtrl->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSizeFieldList ), NULL, this );
	m_addFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnAddField ), NULL, this );
	m_renameFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRenameField ), NULL, this );
	m_removeFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRemoveField ), NULL, this );
	m_filter->Connect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnFilterMouseMoved ), NULL, this );
	m_filter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnFilterText ), NULL, this );
	m_checkExcludeDNP->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnExcludeDNPToggled ), NULL, this );
	m_checkShowExcluded->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnShowExcludedToggled ), NULL, this );
	m_groupSymbolsBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnGroupSymbolsToggled ), NULL, this );
	m_bRefresh->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRegroupSymbols ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableItemContextMenu ), NULL, this );
	m_grid->Connect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableColSize ), NULL, this );
	m_radioProject->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnScopeChanged ), NULL, this );
	m_radioCurrentSheet->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnScopeChanged ), NULL, this );
	m_radioRecursive->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnScopeChanged ), NULL, this );
	m_textFieldDelimiter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textStringDelimiter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textRefDelimiter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textRefRangeDelimiter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_checkKeepTabs->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_checkKeepLineBreaks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnOutputFileBrowseClicked ), NULL, this );
	m_bRefreshPreview->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_buttonExport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnExport ), NULL, this );
	m_buttonApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSaveAndContinue ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnCancel ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnOk ), NULL, this );
}

DIALOG_SYMBOL_FIELDS_TABLE_BASE::~DIALOG_SYMBOL_FIELDS_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnClose ) );
	m_nbPages->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPageChanged ), NULL, this );
	m_fieldsCtrl->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnColumnItemToggled ), NULL, this );
	m_fieldsCtrl->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnFieldsCtrlSelectionChanged ), NULL, this );
	m_fieldsCtrl->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSizeFieldList ), NULL, this );
	m_addFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnAddField ), NULL, this );
	m_renameFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRenameField ), NULL, this );
	m_removeFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRemoveField ), NULL, this );
	m_filter->Disconnect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnFilterMouseMoved ), NULL, this );
	m_filter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnFilterText ), NULL, this );
	m_checkExcludeDNP->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnExcludeDNPToggled ), NULL, this );
	m_checkShowExcluded->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnShowExcludedToggled ), NULL, this );
	m_groupSymbolsBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnGroupSymbolsToggled ), NULL, this );
	m_bRefresh->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRegroupSymbols ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableItemContextMenu ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableColSize ), NULL, this );
	m_radioProject->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnScopeChanged ), NULL, this );
	m_radioCurrentSheet->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnScopeChanged ), NULL, this );
	m_radioRecursive->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnScopeChanged ), NULL, this );
	m_textFieldDelimiter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textStringDelimiter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textRefDelimiter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textRefRangeDelimiter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_checkKeepTabs->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_checkKeepLineBreaks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnOutputFileBrowseClicked ), NULL, this );
	m_bRefreshPreview->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_buttonExport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnExport ), NULL, this );
	m_buttonApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSaveAndContinue ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnCancel ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnOk ), NULL, this );

}

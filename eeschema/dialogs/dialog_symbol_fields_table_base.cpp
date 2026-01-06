///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
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

	m_splitterMainWindow = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxSP_NO_XP_THEME );
	m_splitterMainWindow->SetMinimumPaneSize( 120 );

	m_leftPanel = new wxPanel( m_splitterMainWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	m_splitter_left = new wxSplitterWindow( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxSP_NO_XP_THEME );
	m_splitter_left->SetSashGravity( 0.7 );
	m_splitter_left->SetMinimumPaneSize( 80 );

	m_viewControlsPanel = new wxPanel( m_splitter_left, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bViewControlsSizer;
	bViewControlsSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_viewControlsGrid = new WX_GRID( m_viewControlsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_viewControlsGrid->CreateGrid( 1, 4 );
	m_viewControlsGrid->EnableEditing( true );
	m_viewControlsGrid->EnableGridLines( false );
	m_viewControlsGrid->EnableDragGridSize( false );
	m_viewControlsGrid->SetMargins( 0, 0 );

	// Columns
	m_viewControlsGrid->SetColSize( 0, 60 );
	m_viewControlsGrid->SetColSize( 1, 60 );
	m_viewControlsGrid->SetColSize( 2, 46 );
	m_viewControlsGrid->SetColSize( 3, 56 );
	m_viewControlsGrid->EnableDragColMove( false );
	m_viewControlsGrid->EnableDragColSize( false );
	m_viewControlsGrid->SetColLabelValue( 0, _("Field") );
	m_viewControlsGrid->SetColLabelValue( 1, _("BOM Name") );
	m_viewControlsGrid->SetColLabelValue( 2, _("Include") );
	m_viewControlsGrid->SetColLabelValue( 3, _("Group By") );
	m_viewControlsGrid->SetColLabelSize( 24 );
	m_viewControlsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_viewControlsGrid->EnableDragRowSize( false );
	m_viewControlsGrid->SetRowLabelSize( 0 );
	m_viewControlsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_viewControlsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_viewControlsGrid->SetMinSize( wxSize( -1,250 ) );

	bMargins->Add( m_viewControlsGrid, 1, wxEXPAND, 5 );

	wxBoxSizer* bFieldsButtons;
	bFieldsButtons = new wxBoxSizer( wxHORIZONTAL );

	m_addFieldButton = new STD_BITMAP_BUTTON( m_viewControlsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_addFieldButton->SetToolTip( _("Add a new field") );

	bFieldsButtons->Add( m_addFieldButton, 0, wxBOTTOM, 5 );

	m_renameFieldButton = new STD_BITMAP_BUTTON( m_viewControlsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_renameFieldButton->SetToolTip( _("Rename selected field") );

	bFieldsButtons->Add( m_renameFieldButton, 0, wxBOTTOM|wxLEFT, 5 );


	bFieldsButtons->Add( 15, 0, 0, wxEXPAND, 5 );

	m_removeFieldButton = new STD_BITMAP_BUTTON( m_viewControlsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_removeFieldButton->SetToolTip( _("Remove selected field") );

	bFieldsButtons->Add( m_removeFieldButton, 0, wxBOTTOM|wxLEFT, 5 );


	bMargins->Add( bFieldsButtons, 0, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bPresets;
	bPresets = new wxBoxSizer( wxVERTICAL );

	m_staticline11 = new wxStaticLine( m_viewControlsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bPresets->Add( m_staticline11, 0, wxEXPAND|wxBOTTOM, 5 );

	m_bomPresetsLabel = new wxStaticText( m_viewControlsPanel, wxID_ANY, _("View presets:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bomPresetsLabel->Wrap( -1 );
	bPresets->Add( m_bomPresetsLabel, 0, 0, 5 );


	bPresets->Add( 0, 2, 0, 0, 5 );

	wxString m_cbBomPresetsChoices[] = { _("Default"), _("(unsaved)") };
	int m_cbBomPresetsNChoices = sizeof( m_cbBomPresetsChoices ) / sizeof( wxString );
	m_cbBomPresets = new wxChoice( m_viewControlsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbBomPresetsNChoices, m_cbBomPresetsChoices, 0 );
	m_cbBomPresets->SetSelection( 0 );
	bPresets->Add( m_cbBomPresets, 0, wxEXPAND, 5 );


	bPresets->Add( 0, 5, 0, wxEXPAND, 5 );


	bMargins->Add( bPresets, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bViewControlsSizer->Add( bMargins, 1, wxALL|wxEXPAND, 5 );


	m_viewControlsPanel->SetSizer( bViewControlsSizer );
	m_viewControlsPanel->Layout();
	bViewControlsSizer->Fit( m_viewControlsPanel );
	m_variantsPanel = new wxPanel( m_splitter_left, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bVariantsSizer;
	bVariantsSizer = new wxBoxSizer( wxVERTICAL );

	bMargins2 = new wxBoxSizer( wxVERTICAL );

	m_staticText9 = new wxStaticText( m_variantsPanel, wxID_ANY, _("Schematic variants:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	bMargins2->Add( m_staticText9, 0, wxTOP|wxBOTTOM|wxLEFT, 2 );

	m_variantListBox = new wxListBox( m_variantsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_NEEDED_SB|wxLB_SINGLE );
	bMargins2->Add( m_variantListBox, 1, wxEXPAND|wxBOTTOM, 2 );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxHORIZONTAL );

	m_addVariantButton = new STD_BITMAP_BUTTON( m_variantsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer14->Add( m_addVariantButton, 0, wxRIGHT, 5 );

	m_renameVariantButton = new STD_BITMAP_BUTTON( m_variantsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer14->Add( m_renameVariantButton, 0, 0, 5 );

	m_copyVariantButton = new STD_BITMAP_BUTTON( m_variantsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer14->Add( m_copyVariantButton, 0, 0, 5 );


	bSizer14->Add( 15, 0, 0, 0, 5 );

	m_deleteVariantButton = new STD_BITMAP_BUTTON( m_variantsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer14->Add( m_deleteVariantButton, 0, wxLEFT, 5 );


	bMargins2->Add( bSizer14, 0, wxEXPAND|wxTOP|wxLEFT, 2 );


	bVariantsSizer->Add( bMargins2, 1, wxALL|wxEXPAND, 3 );


	m_variantsPanel->SetSizer( bVariantsSizer );
	m_variantsPanel->Layout();
	bVariantsSizer->Fit( m_variantsPanel );
	m_splitter_left->SplitHorizontally( m_viewControlsPanel, m_variantsPanel, -1 );
	bLeftSizer->Add( m_splitter_left, 1, wxEXPAND, 5 );


	m_leftPanel->SetSizer( bLeftSizer );
	m_leftPanel->Layout();
	bLeftSizer->Fit( m_leftPanel );
	m_rightPanel = new wxPanel( m_splitterMainWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins1;
	bMargins1 = new wxBoxSizer( wxVERTICAL );

	m_nbPages = new wxNotebook( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelEdit = new wxPanel( m_nbPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bEditSizer;
	bEditSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bControls;
	bControls = new wxBoxSizer( wxHORIZONTAL );

	m_filter = new wxSearchCtrl( m_panelEdit, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_filter->ShowSearchButton( true );
	#endif
	m_filter->ShowCancelButton( true );
	m_filter->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_filter->SetMinSize( wxSize( 240,-1 ) );

	bControls->Add( m_filter, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticline31 = new wxStaticLine( m_panelEdit, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bControls->Add( m_staticline31, 0, wxEXPAND | wxALL, 3 );

	wxString m_scopeChoices[] = { _("Entire Project"), _("Current Sheet Only"), _("Current Sheet and Down") };
	int m_scopeNChoices = sizeof( m_scopeChoices ) / sizeof( wxString );
	m_scope = new wxChoice( m_panelEdit, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_scopeNChoices, m_scopeChoices, 0 );
	m_scope->SetSelection( 0 );
	bControls->Add( m_scope, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticline311 = new wxStaticLine( m_panelEdit, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bControls->Add( m_staticline311, 0, wxEXPAND | wxALL, 5 );

	m_groupSymbolsBox = new wxCheckBox( m_panelEdit, wxID_ANY, _("Group symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_groupSymbolsBox->SetValue(true);
	m_groupSymbolsBox->SetToolTip( _("Group symbols together based on common properties") );

	bControls->Add( m_groupSymbolsBox, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 8 );

	m_staticline3 = new wxStaticLine( m_panelEdit, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bControls->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 3 );

	m_bRefresh = new STD_BITMAP_BUTTON( m_panelEdit, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bRefresh->SetMinSize( wxSize( 30,30 ) );

	bControls->Add( m_bRefresh, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_bMenu = new STD_BITMAP_BUTTON( m_panelEdit, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bMenu->SetMinSize( wxSize( 30,30 ) );

	bControls->Add( m_bMenu, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	bEditSizer->Add( bControls, 0, wxEXPAND|wxLEFT|wxTOP, 5 );

	m_grid = new WX_GRID( m_panelEdit, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

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

	bEditSizer->Add( m_grid, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


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

	bMargins1->Add( m_nbPages, 1, wxEXPAND|wxALL, 5 );


	bRightSizer->Add( bMargins1, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_sidebarButton = new STD_BITMAP_BUTTON( m_rightPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonsSizer->Add( m_sidebarButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );


	bButtonsSizer->Add( 0, 0, 9, wxEXPAND, 5 );

	m_buttonExport = new wxButton( m_rightPanel, wxID_ANY, _("Export"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonExport, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );

	m_buttonApply = new wxButton( m_rightPanel, wxID_ANY, _("Apply, Save Schematic && Continue"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonApply, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( m_rightPanel, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( m_rightPanel, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bButtonsSizer->Add( m_sdbSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bRightSizer->Add( bButtonsSizer, 0, wxEXPAND, 5 );


	m_rightPanel->SetSizer( bRightSizer );
	m_rightPanel->Layout();
	bRightSizer->Fit( m_rightPanel );
	m_splitterMainWindow->SplitVertically( m_leftPanel, m_rightPanel, -1 );
	bMainSizer->Add( m_splitterMainWindow, 1, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnClose ) );
	m_viewControlsGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnViewControlsCellChanged ), NULL, this );
	m_viewControlsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSizeViewControlsGrid ), NULL, this );
	m_addFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnAddField ), NULL, this );
	m_renameFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRenameField ), NULL, this );
	m_removeFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRemoveField ), NULL, this );
	m_variantListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::onVariantSelectionChange ), NULL, this );
	m_addVariantButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::onAddVariant ), NULL, this );
	m_renameVariantButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::onRenameVariant ), NULL, this );
	m_copyVariantButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::onCopyVariant ), NULL, this );
	m_deleteVariantButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::onDeleteVariant ), NULL, this );
	m_nbPages->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPageChanged ), NULL, this );
	m_filter->Connect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnFilterMouseMoved ), NULL, this );
	m_filter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnFilterText ), NULL, this );
	m_scope->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnScope ), NULL, this );
	m_groupSymbolsBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnGroupSymbolsToggled ), NULL, this );
	m_bRefresh->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRegroupSymbols ), NULL, this );
	m_bMenu->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnMenu ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableColSize ), NULL, this );
	m_textFieldDelimiter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textStringDelimiter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textRefDelimiter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textRefRangeDelimiter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_checkKeepTabs->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_checkKeepLineBreaks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnOutputFileBrowseClicked ), NULL, this );
	m_bRefreshPreview->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_sidebarButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSidebarToggle ), NULL, this );
	m_buttonExport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnExport ), NULL, this );
	m_buttonApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSaveAndContinue ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnCancel ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnOk ), NULL, this );
}

DIALOG_SYMBOL_FIELDS_TABLE_BASE::~DIALOG_SYMBOL_FIELDS_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnClose ) );
	m_viewControlsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnViewControlsCellChanged ), NULL, this );
	m_viewControlsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSizeViewControlsGrid ), NULL, this );
	m_addFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnAddField ), NULL, this );
	m_renameFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRenameField ), NULL, this );
	m_removeFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRemoveField ), NULL, this );
	m_variantListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::onVariantSelectionChange ), NULL, this );
	m_addVariantButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::onAddVariant ), NULL, this );
	m_renameVariantButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::onRenameVariant ), NULL, this );
	m_copyVariantButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::onCopyVariant ), NULL, this );
	m_deleteVariantButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::onDeleteVariant ), NULL, this );
	m_nbPages->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPageChanged ), NULL, this );
	m_filter->Disconnect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnFilterMouseMoved ), NULL, this );
	m_filter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnFilterText ), NULL, this );
	m_scope->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnScope ), NULL, this );
	m_groupSymbolsBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnGroupSymbolsToggled ), NULL, this );
	m_bRefresh->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRegroupSymbols ), NULL, this );
	m_bMenu->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnMenu ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableColSize ), NULL, this );
	m_textFieldDelimiter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textStringDelimiter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textRefDelimiter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_textRefRangeDelimiter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_checkKeepTabs->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_checkKeepLineBreaks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnOutputFileBrowseClicked ), NULL, this );
	m_bRefreshPreview->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnPreviewRefresh ), NULL, this );
	m_sidebarButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSidebarToggle ), NULL, this );
	m_buttonExport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnExport ), NULL, this );
	m_buttonApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSaveAndContinue ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnCancel ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnOk ), NULL, this );

}

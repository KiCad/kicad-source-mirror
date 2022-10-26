///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_sim_model_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SIM_MODEL_BASE::DIALOG_SIM_MODEL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_modelPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanel;
	bSizerPanel = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	m_useLibraryModelRadioButton = new wxRadioButton( m_modelPanel, wxID_ANY, _("From library:"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizerMargins->Add( m_useLibraryModelRadioButton, 0, wxLEFT, 5 );

	m_sourceSizer = new wxFlexGridSizer( 0, 3, 2, 0 );
	m_sourceSizer->AddGrowableCol( 1 );
	m_sourceSizer->AddGrowableRow( 0 );
	m_sourceSizer->SetFlexibleDirection( wxBOTH );
	m_sourceSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_pathLabel = new wxStaticText( m_modelPanel, wxID_ANY, _("Library path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pathLabel->Wrap( -1 );
	m_sourceSizer->Add( m_pathLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_tclibraryPathName = new wxTextCtrl( m_modelPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_sourceSizer->Add( m_tclibraryPathName, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_browseButton = new wxBitmapButton( m_modelPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_sourceSizer->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_modelNameLabel = new wxStaticText( m_modelPanel, wxID_ANY, _("Model:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_modelNameLabel->Wrap( -1 );
	m_sourceSizer->Add( m_modelNameLabel, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );

	m_modelNameCombobox = new wxComboBox( m_modelPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER );
	m_sourceSizer->Add( m_modelNameCombobox, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_overrideCheckbox = new wxCheckBox( m_modelPanel, wxID_ANY, _("Override"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sourceSizer->Add( m_overrideCheckbox, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 7 );

	m_ibisPinLabel = new wxStaticText( m_modelPanel, wxID_ANY, _("Pin:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ibisPinLabel->Wrap( -1 );
	m_sourceSizer->Add( m_ibisPinLabel, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );

	m_ibisPinCombobox = new wxComboBox( m_modelPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER );
	m_sourceSizer->Add( m_ibisPinCombobox, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_differentialCheckbox = new wxCheckBox( m_modelPanel, wxID_ANY, _("Differential"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sourceSizer->Add( m_differentialCheckbox, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 7 );

	m_ibisModelLabel = new wxStaticText( m_modelPanel, wxID_ANY, _("Model:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ibisModelLabel->Wrap( -1 );
	m_sourceSizer->Add( m_ibisModelLabel, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );

	m_ibisModelCombobox = new wxComboBox( m_modelPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER );
	m_sourceSizer->Add( m_ibisModelCombobox, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	bSizerMargins->Add( m_sourceSizer, 0, wxEXPAND|wxBOTTOM|wxLEFT, 24 );

	m_useInstanceModelRadioButton = new wxRadioButton( m_modelPanel, wxID_ANY, _("From symbol instance:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMargins->Add( m_useInstanceModelRadioButton, 0, wxBOTTOM|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer16;
	fgSizer16 = new wxFlexGridSizer( 0, 2, 8, 0 );
	fgSizer16->AddGrowableCol( 1 );
	fgSizer16->SetFlexibleDirection( wxBOTH );
	fgSizer16->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextDevType = new wxStaticText( m_modelPanel, wxID_ANY, _("Device:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDevType->Wrap( -1 );
	fgSizer16->Add( m_staticTextDevType, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_deviceTypeChoiceChoices;
	m_deviceTypeChoice = new wxChoice( m_modelPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_deviceTypeChoiceChoices, 0 );
	m_deviceTypeChoice->SetSelection( 0 );
	fgSizer16->Add( m_deviceTypeChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );

	m_staticTextSpiceType = new wxStaticText( m_modelPanel, wxID_ANY, _("Type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSpiceType->Wrap( -1 );
	fgSizer16->Add( m_staticTextSpiceType, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_typeChoiceChoices;
	m_typeChoice = new wxChoice( m_modelPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_typeChoiceChoices, 0 );
	m_typeChoice->SetSelection( 0 );
	fgSizer16->Add( m_typeChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );


	bSizerMargins->Add( fgSizer16, 0, wxEXPAND|wxLEFT, 24 );


	bSizerMargins->Add( 0, 10, 0, 0, 5 );

	m_notebook4 = new wxNotebook( m_modelPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_parametersPanel = new wxPanel( m_notebook4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_parametersPanel->SetMinSize( wxSize( 500,-1 ) );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );

	m_paramGridMgr = new wxPropertyGridManager(m_parametersPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPGMAN_DEFAULT_STYLE|wxPG_SPLITTER_AUTO_CENTER);
	m_paramGridMgr->SetExtraStyle( wxPG_EX_MODE_BUTTONS|wxPG_EX_NATIVE_DOUBLE_BUFFERING );

	m_paramGrid = m_paramGridMgr->AddPage( _("Page"), wxNullBitmap );
	bSizer12->Add( m_paramGridMgr, 1, wxALL|wxEXPAND, 5 );


	m_parametersPanel->SetSizer( bSizer12 );
	m_parametersPanel->Layout();
	bSizer12->Fit( m_parametersPanel );
	m_notebook4->AddPage( m_parametersPanel, _("Parameters"), true );
	m_codePanel = new wxPanel( m_notebook4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_codePreview = new wxStyledTextCtrl( m_codePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString );
	m_codePreview->SetUseTabs( true );
	m_codePreview->SetTabWidth( 4 );
	m_codePreview->SetIndent( 4 );
	m_codePreview->SetTabIndents( true );
	m_codePreview->SetBackSpaceUnIndents( true );
	m_codePreview->SetViewEOL( false );
	m_codePreview->SetViewWhiteSpace( false );
	m_codePreview->SetMarginWidth( 2, 0 );
	m_codePreview->SetIndentationGuides( true );
	m_codePreview->SetReadOnly( false );
	m_codePreview->SetMarginType( 1, wxSTC_MARGIN_SYMBOL );
	m_codePreview->SetMarginMask( 1, wxSTC_MASK_FOLDERS );
	m_codePreview->SetMarginWidth( 1, 16);
	m_codePreview->SetMarginSensitive( 1, true );
	m_codePreview->SetProperty( wxT("fold"), wxT("1") );
	m_codePreview->SetFoldFlags( wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED );
	m_codePreview->SetMarginType( 0, wxSTC_MARGIN_NUMBER );
	m_codePreview->SetMarginWidth( 0, m_codePreview->TextWidth( wxSTC_STYLE_LINENUMBER, wxT("_99999") ) );
	m_codePreview->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_codePreview->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_codePreview->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_codePreview->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_codePreview->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_codePreview->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_codePreview->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_codePreview->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_codePreview->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_codePreview->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_codePreview->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_codePreview->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_codePreview->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_codePreview->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_codePreview->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_codePreview->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_codePreview->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	bSizer5->Add( m_codePreview, 1, wxEXPAND | wxALL, 5 );


	m_codePanel->SetSizer( bSizer5 );
	m_codePanel->Layout();
	bSizer5->Fit( m_codePanel );
	m_notebook4->AddPage( m_codePanel, _("Code"), false );

	bSizerMargins->Add( m_notebook4, 1, wxEXPAND|wxTOP, 10 );


	bSizerMargins->Add( 0, 5, 0, wxEXPAND, 5 );


	bSizerPanel->Add( bSizerMargins, 1, wxEXPAND, 5 );


	m_modelPanel->SetSizer( bSizerPanel );
	m_modelPanel->Layout();
	bSizerPanel->Fit( m_modelPanel );
	m_notebook->AddPage( m_modelPanel, _("Model"), true );
	m_pinAssignmentsPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	m_pinAssignmentsGrid = new WX_GRID( m_pinAssignmentsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_pinAssignmentsGrid->CreateGrid( 0, 2 );
	m_pinAssignmentsGrid->EnableEditing( true );
	m_pinAssignmentsGrid->EnableGridLines( true );
	m_pinAssignmentsGrid->EnableDragGridSize( false );
	m_pinAssignmentsGrid->SetMargins( 0, 0 );

	// Columns
	m_pinAssignmentsGrid->SetColSize( 0, 160 );
	m_pinAssignmentsGrid->SetColSize( 1, 160 );
	m_pinAssignmentsGrid->EnableDragColMove( false );
	m_pinAssignmentsGrid->EnableDragColSize( true );
	m_pinAssignmentsGrid->SetColLabelValue( 0, _("Symbol Pin") );
	m_pinAssignmentsGrid->SetColLabelValue( 1, _("Model Pin") );
	m_pinAssignmentsGrid->SetColLabelSize( 22 );
	m_pinAssignmentsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_pinAssignmentsGrid->EnableDragRowSize( false );
	m_pinAssignmentsGrid->SetRowLabelSize( 0 );
	m_pinAssignmentsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_pinAssignmentsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer10->Add( m_pinAssignmentsGrid, 1, wxALL|wxEXPAND, 5 );


	m_pinAssignmentsPanel->SetSizer( bSizer10 );
	m_pinAssignmentsPanel->Layout();
	bSizer10->Fit( m_pinAssignmentsPanel );
	m_notebook->AddPage( m_pinAssignmentsPanel, _("Pin Assignments"), false );

	bSizer8->Add( m_notebook, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 2, 0, 0 );

	m_excludeCheckbox = new wxCheckBox( this, wxID_ANY, _("Exclude from simulation"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_excludeCheckbox, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_inferCheckbox = new wxCheckBox( this, wxID_ANY, _("Store in Reference and Value"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_inferCheckbox, 0, wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer8->Add( gSizer1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer8->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizer8->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bSizer8 );
	this->Layout();
	bSizer8->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_useLibraryModelRadioButton->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onRadioButton ), NULL, this );
	m_pathLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathLabelUpdate ), NULL, this );
	m_tclibraryPathName->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathUpdate ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onBrowseButtonClick ), NULL, this );
	m_browseButton->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onBrowseButtonUpdate ), NULL, this );
	m_modelNameLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameLabelUpdate ), NULL, this );
	m_modelNameCombobox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameCombobox ), NULL, this );
	m_modelNameCombobox->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxKillFocus ), NULL, this );
	m_modelNameCombobox->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxTextEnter ), NULL, this );
	m_modelNameCombobox->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_overrideCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onOverrideCheckbox ), NULL, this );
	m_ibisPinLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onIbisPinLabelUpdate ), NULL, this );
	m_ibisPinCombobox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onIbisPinCombobox ), NULL, this );
	m_ibisPinCombobox->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxKillFocus ), NULL, this );
	m_ibisPinCombobox->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onIbisPinComboboxTextEnter ), NULL, this );
	m_ibisPinCombobox->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_differentialCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onDifferentialCheckbox ), NULL, this );
	m_differentialCheckbox->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onOverrideCheckboxUpdate ), NULL, this );
	m_ibisModelLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onIbisModelLabelUpdate ), NULL, this );
	m_ibisModelCombobox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onIbisModelCombobox ), NULL, this );
	m_ibisModelCombobox->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxKillFocus ), NULL, this );
	m_ibisModelCombobox->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onIbisModelComboboxTextEnter ), NULL, this );
	m_ibisModelCombobox->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_useInstanceModelRadioButton->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onRadioButton ), NULL, this );
	m_staticTextDevType->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeLabelUpdate ), NULL, this );
	m_deviceTypeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeChoice ), NULL, this );
	m_deviceTypeChoice->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeChoiceUpdate ), NULL, this );
	m_staticTextSpiceType->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onTypeLabelUpdate ), NULL, this );
	m_typeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onTypeChoice ), NULL, this );
	m_paramGridMgr->Connect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( DIALOG_SIM_MODEL_BASE::onParamGridChanged ), NULL, this );
	m_codePreview->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onCodePreviewSetFocus ), NULL, this );
	m_pinAssignmentsGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SIM_MODEL_BASE::onPinAssignmentsGridCellChange ), NULL, this );
	m_pinAssignmentsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SIM_MODEL_BASE::onPinAssignmentsGridSize ), NULL, this );
	m_excludeCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onExcludeCheckbox ), NULL, this );
	m_inferCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onInferCheckbox ), NULL, this );
}

DIALOG_SIM_MODEL_BASE::~DIALOG_SIM_MODEL_BASE()
{
	// Disconnect Events
	m_useLibraryModelRadioButton->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onRadioButton ), NULL, this );
	m_pathLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathLabelUpdate ), NULL, this );
	m_tclibraryPathName->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathUpdate ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onBrowseButtonClick ), NULL, this );
	m_browseButton->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onBrowseButtonUpdate ), NULL, this );
	m_modelNameLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameLabelUpdate ), NULL, this );
	m_modelNameCombobox->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameCombobox ), NULL, this );
	m_modelNameCombobox->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxKillFocus ), NULL, this );
	m_modelNameCombobox->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxTextEnter ), NULL, this );
	m_modelNameCombobox->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_overrideCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onOverrideCheckbox ), NULL, this );
	m_ibisPinLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onIbisPinLabelUpdate ), NULL, this );
	m_ibisPinCombobox->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onIbisPinCombobox ), NULL, this );
	m_ibisPinCombobox->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxKillFocus ), NULL, this );
	m_ibisPinCombobox->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onIbisPinComboboxTextEnter ), NULL, this );
	m_ibisPinCombobox->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_differentialCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onDifferentialCheckbox ), NULL, this );
	m_differentialCheckbox->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onOverrideCheckboxUpdate ), NULL, this );
	m_ibisModelLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onIbisModelLabelUpdate ), NULL, this );
	m_ibisModelCombobox->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onIbisModelCombobox ), NULL, this );
	m_ibisModelCombobox->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxKillFocus ), NULL, this );
	m_ibisModelCombobox->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onIbisModelComboboxTextEnter ), NULL, this );
	m_ibisModelCombobox->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_useInstanceModelRadioButton->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onRadioButton ), NULL, this );
	m_staticTextDevType->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeLabelUpdate ), NULL, this );
	m_deviceTypeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeChoice ), NULL, this );
	m_deviceTypeChoice->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeChoiceUpdate ), NULL, this );
	m_staticTextSpiceType->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onTypeLabelUpdate ), NULL, this );
	m_typeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onTypeChoice ), NULL, this );
	m_paramGridMgr->Disconnect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( DIALOG_SIM_MODEL_BASE::onParamGridChanged ), NULL, this );
	m_codePreview->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onCodePreviewSetFocus ), NULL, this );
	m_pinAssignmentsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SIM_MODEL_BASE::onPinAssignmentsGridCellChange ), NULL, this );
	m_pinAssignmentsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SIM_MODEL_BASE::onPinAssignmentsGridSize ), NULL, this );
	m_excludeCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onExcludeCheckbox ), NULL, this );
	m_inferCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onInferCheckbox ), NULL, this );

}

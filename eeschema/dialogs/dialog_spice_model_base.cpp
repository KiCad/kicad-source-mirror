///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_spice_model_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SPICE_MODEL_BASE::DIALOG_SPICE_MODEL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_modelPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( m_modelPanel, wxID_ANY, wxT("Source") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer15;
	fgSizer15 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer15->AddGrowableCol( 2 );
	fgSizer15->SetFlexibleDirection( wxBOTH );
	fgSizer15->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_useInstanceModelRadioButton = new wxRadioButton( sbSizer4->GetStaticBox(), wxID_ANY, wxT("Instance"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizer15->Add( m_useInstanceModelRadioButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_useLibraryModelRadioButton = new wxRadioButton( sbSizer4->GetStaticBox(), wxID_ANY, wxT("Library:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer15->Add( m_useLibraryModelRadioButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_libraryFilenameInput = new wxTextCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer15->Add( m_libraryFilenameInput, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_browseButton = new wxBitmapButton( sbSizer4->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	fgSizer15->Add( m_browseButton, 0, wxALL, 5 );


	fgSizer15->Add( 0, 0, 1, wxEXPAND, 5 );

	m_modelNameLabel = new wxStaticText( sbSizer4->GetStaticBox(), wxID_ANY, wxT("Model:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_modelNameLabel->Wrap( -1 );
	fgSizer15->Add( m_modelNameLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_modelNameCombobox = new wxComboBox( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizer15->Add( m_modelNameCombobox, 0, wxALL|wxEXPAND, 5 );

	m_overrideCheckbox = new wxCheckBox( sbSizer4->GetStaticBox(), wxID_ANY, wxT("Override"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer15->Add( m_overrideCheckbox, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbSizer4->Add( fgSizer15, 1, wxEXPAND, 5 );


	bSizer9->Add( sbSizer4, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( m_modelPanel, wxID_ANY, wxT("Model") ), wxVERTICAL );

	m_notebook4 = new wxNotebook( sbSizer5->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_parametersPanel = new wxPanel( m_notebook4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer16;
	fgSizer16 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer16->AddGrowableCol( 1 );
	fgSizer16->SetFlexibleDirection( wxBOTH );
	fgSizer16->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText127 = new wxStaticText( m_parametersPanel, wxID_ANY, wxT("Device:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText127->Wrap( -1 );
	fgSizer16->Add( m_staticText127, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_deviceTypeChoiceChoices;
	m_deviceTypeChoice = new wxChoice( m_parametersPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_deviceTypeChoiceChoices, 0 );
	m_deviceTypeChoice->SetSelection( 0 );
	fgSizer16->Add( m_deviceTypeChoice, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_staticText8 = new wxStaticText( m_parametersPanel, wxID_ANY, wxT("Type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizer16->Add( m_staticText8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_typeChoiceChoices;
	m_typeChoice = new wxChoice( m_parametersPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_typeChoiceChoices, 0 );
	m_typeChoice->SetSelection( 0 );
	fgSizer16->Add( m_typeChoice, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	bSizer12->Add( fgSizer16, 0, wxEXPAND, 5 );

	m_paramGridMgr = new wxPropertyGridManager(m_parametersPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPGMAN_DEFAULT_STYLE|wxPG_SPLITTER_AUTO_CENTER);
	m_paramGridMgr->SetExtraStyle( wxPG_EX_MODE_BUTTONS|wxPG_EX_NATIVE_DOUBLE_BUFFERING );

	m_paramGrid = m_paramGridMgr->AddPage( wxT("Page"), wxNullBitmap );
	bSizer12->Add( m_paramGridMgr, 1, wxALL|wxEXPAND, 5 );


	m_parametersPanel->SetSizer( bSizer12 );
	m_parametersPanel->Layout();
	bSizer12->Fit( m_parametersPanel );
	m_notebook4->AddPage( m_parametersPanel, wxT("Parameters"), true );
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
	m_notebook4->AddPage( m_codePanel, wxT("Code"), false );

	sbSizer5->Add( m_notebook4, 1, wxEXPAND | wxALL, 5 );


	bSizer9->Add( sbSizer5, 1, wxEXPAND, 5 );


	m_modelPanel->SetSizer( bSizer9 );
	m_modelPanel->Layout();
	bSizer9->Fit( m_modelPanel );
	m_notebook->AddPage( m_modelPanel, wxT("Model"), true );
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
	m_pinAssignmentsGrid->SetColLabelValue( 0, wxT("Symbol Pin") );
	m_pinAssignmentsGrid->SetColLabelValue( 1, wxT("Model Pin") );
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
	m_notebook->AddPage( m_pinAssignmentsPanel, wxT("Pin Assignments"), false );

	bSizer8->Add( m_notebook, 1, wxALL|wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer8->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	m_excludeSymbol = new wxCheckBox( this, wxID_ANY, wxT("Exclude symbol from simulation"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_excludeSymbol, 0, wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizer8->Add( m_sdbSizer1, 0, wxBOTTOM|wxEXPAND, 5 );


	this->SetSizer( bSizer8 );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_useInstanceModelRadioButton->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onRadioButton ), NULL, this );
	m_useLibraryModelRadioButton->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onRadioButton ), NULL, this );
	m_libraryFilenameInput->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onLibraryFilenameInputUpdate ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onBrowseButtonClick ), NULL, this );
	m_browseButton->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onBrowseButtonUpdate ), NULL, this );
	m_modelNameCombobox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onModelNameCombobox ), NULL, this );
	m_modelNameCombobox->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_overrideCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onOverrideCheckbox ), NULL, this );
	m_overrideCheckbox->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onOverrideCheckboxUpdate ), NULL, this );
	m_deviceTypeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onDeviceTypeChoice ), NULL, this );
	m_deviceTypeChoice->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onDeviceTypeChoiceUpdate ), NULL, this );
	m_typeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onTypeChoice ), NULL, this );
	m_typeChoice->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onTypeChoiceUpdate ), NULL, this );
	m_paramGridMgr->Connect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( DIALOG_SPICE_MODEL_BASE::onParamGridChanged ), NULL, this );
	m_pinAssignmentsGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SPICE_MODEL_BASE::onPinAssignmentsGridCellChange ), NULL, this );
	m_pinAssignmentsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SPICE_MODEL_BASE::onPinAssignmentsGridSize ), NULL, this );
}

DIALOG_SPICE_MODEL_BASE::~DIALOG_SPICE_MODEL_BASE()
{
	// Disconnect Events
	m_useInstanceModelRadioButton->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onRadioButton ), NULL, this );
	m_useLibraryModelRadioButton->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onRadioButton ), NULL, this );
	m_libraryFilenameInput->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onLibraryFilenameInputUpdate ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onBrowseButtonClick ), NULL, this );
	m_browseButton->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onBrowseButtonUpdate ), NULL, this );
	m_modelNameCombobox->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onModelNameCombobox ), NULL, this );
	m_modelNameCombobox->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_overrideCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onOverrideCheckbox ), NULL, this );
	m_overrideCheckbox->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onOverrideCheckboxUpdate ), NULL, this );
	m_deviceTypeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onDeviceTypeChoice ), NULL, this );
	m_deviceTypeChoice->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onDeviceTypeChoiceUpdate ), NULL, this );
	m_typeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onTypeChoice ), NULL, this );
	m_typeChoice->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SPICE_MODEL_BASE::onTypeChoiceUpdate ), NULL, this );
	m_paramGridMgr->Disconnect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( DIALOG_SPICE_MODEL_BASE::onParamGridChanged ), NULL, this );
	m_pinAssignmentsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SPICE_MODEL_BASE::onPinAssignmentsGridCellChange ), NULL, this );
	m_pinAssignmentsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SPICE_MODEL_BASE::onPinAssignmentsGridSize ), NULL, this );

}

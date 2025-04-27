///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"
#include "widgets/wx_infobar.h"

#include "dialog_sim_model_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SIM_MODEL_BASE::DIALOG_SIM_MODEL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	bSizerMain->Add( m_infoBar, 0, wxEXPAND, 5 );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_modelPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanel;
	bSizerPanel = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	m_rbLibraryModel = new wxRadioButton( m_modelPanel, wxID_ANY, _("SPICE model from file (*.lib, *.sub or *.ibs)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizerMargins->Add( m_rbLibraryModel, 0, wxLEFT, 5 );


	bSizerMargins->Add( 0, 2, 0, 0, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_pathLabel = new wxStaticText( m_modelPanel, wxID_ANY, _("File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pathLabel->Wrap( -1 );
	gbSizer1->Add( m_pathLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_libraryPathText = new wxTextCtrl( m_modelPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	bSizer7->Add( m_libraryPathText, 1, wxALIGN_CENTER_VERTICAL, 3 );

	m_browseButton = new STD_BITMAP_BUTTON( m_modelPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer7->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	gbSizer1->Add( bSizer7, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_modelNameLabel = new wxStaticText( m_modelPanel, wxID_ANY, _("Model:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_modelNameLabel->Wrap( -1 );
	gbSizer1->Add( m_modelNameLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxTOP, 4 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	m_modelFilter = new wxSearchCtrl( m_modelPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_modelFilter->ShowSearchButton( true );
	#endif
	m_modelFilter->ShowCancelButton( false );
	bSizer9->Add( m_modelFilter, 0, wxEXPAND|wxRIGHT, 5 );

	m_modelListBox = new wxListBox( m_modelPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bSizer9->Add( m_modelListBox, 1, wxEXPAND|wxRIGHT, 5 );


	gbSizer1->Add( bSizer9, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );


	gbSizer1->Add( 0, 0, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_pinLabel = new wxStaticText( m_modelPanel, wxID_ANY, _("Pin:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinLabel->Wrap( -1 );
	gbSizer1->Add( m_pinLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pinCombobox = new wxComboBox( m_modelPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_pinCombobox, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_differentialCheckbox = new wxCheckBox( m_modelPanel, wxID_ANY, _("Differential"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_differentialCheckbox, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 40 );

	m_pinModelLabel = new wxStaticText( m_modelPanel, wxID_ANY, _("Pin model:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinModelLabel->Wrap( -1 );
	gbSizer1->Add( m_pinModelLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pinModelCombobox = new wxComboBox( m_modelPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_pinModelCombobox, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 1 );

	m_waveformLabel = new wxStaticText( m_modelPanel, wxID_ANY, _("Waveform:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_waveformLabel->Wrap( -1 );
	gbSizer1->Add( m_waveformLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_waveformChoiceChoices;
	m_waveformChoice = new wxChoice( m_modelPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_waveformChoiceChoices, 0 );
	m_waveformChoice->SetSelection( 0 );
	gbSizer1->Add( m_waveformChoice, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 2 );


	gbSizer1->AddGrowableCol( 1 );
	gbSizer1->AddGrowableRow( 1 );

	bSizerMargins->Add( gbSizer1, 1, wxEXPAND|wxLEFT, 28 );


	bSizerMargins->Add( 0, 18, 0, wxEXPAND, 5 );

	m_rbBuiltinModel = new wxRadioButton( m_modelPanel, wxID_ANY, _("Built-in SPICE model"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMargins->Add( m_rbBuiltinModel, 0, wxBOTTOM|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer16;
	fgSizer16 = new wxFlexGridSizer( 0, 2, 8, 0 );
	fgSizer16->AddGrowableCol( 1 );
	fgSizer16->SetFlexibleDirection( wxBOTH );
	fgSizer16->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_deviceLabel = new wxStaticText( m_modelPanel, wxID_ANY, _("Device:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deviceLabel->Wrap( -1 );
	fgSizer16->Add( m_deviceLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_deviceChoiceChoices;
	m_deviceChoice = new wxChoice( m_modelPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_deviceChoiceChoices, 0 );
	m_deviceChoice->SetSelection( 0 );
	fgSizer16->Add( m_deviceChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );

	m_deviceSubtypeLabel = new wxStaticText( m_modelPanel, wxID_ANY, _("Device type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deviceSubtypeLabel->Wrap( -1 );
	fgSizer16->Add( m_deviceSubtypeLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_deviceSubtypeChoiceChoices;
	m_deviceSubtypeChoice = new wxChoice( m_modelPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_deviceSubtypeChoiceChoices, 0 );
	m_deviceSubtypeChoice->SetSelection( 0 );
	fgSizer16->Add( m_deviceSubtypeChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );


	bSizerMargins->Add( fgSizer16, 0, wxEXPAND|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 24 );


	bSizerMargins->Add( 0, 10, 0, wxEXPAND, 5 );

	m_modelNotebook = new wxNotebook( m_modelPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_parametersPanel = new wxPanel( m_modelNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_parametersPanel->SetMinSize( wxSize( 500,-1 ) );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );

	m_paramGridMgr = new wxPropertyGridManager(m_parametersPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPGMAN_DEFAULT_STYLE|wxPG_SPLITTER_AUTO_CENTER);
	m_paramGridMgr->SetExtraStyle( wxPG_EX_MODE_BUTTONS|wxPG_EX_NATIVE_DOUBLE_BUFFERING );
	m_paramGridMgr->SetMinSize( wxSize( 500,-1 ) );


	m_paramGrid = m_paramGridMgr->AddPage( _("Page"), wxNullBitmap );
	bSizer12->Add( m_paramGridMgr, 1, wxALL|wxEXPAND, 5 );


	m_parametersPanel->SetSizer( bSizer12 );
	m_parametersPanel->Layout();
	bSizer12->Fit( m_parametersPanel );
	m_modelNotebook->AddPage( m_parametersPanel, _("Parameters"), true );
	m_codePanel = new wxPanel( m_modelNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
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
	m_modelNotebook->AddPage( m_codePanel, _("Code"), false );

	bSizerMargins->Add( m_modelNotebook, 1, wxEXPAND|wxALL, 5 );

	m_saveInValueCheckbox = new wxCheckBox( m_modelPanel, wxID_ANY, _("Save {} in Value field as \"{}\""), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMargins->Add( m_saveInValueCheckbox, 0, wxALL, 6 );


	bSizerMargins->Add( 0, 2, 0, wxEXPAND, 5 );


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
	m_pinAssignmentsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizer10->Add( m_pinAssignmentsGrid, 1, wxALL|wxEXPAND, 5 );

	m_subcktLabel = new wxStaticText( m_pinAssignmentsPanel, wxID_ANY, _("Reference"), wxDefaultPosition, wxDefaultSize, 0 );
	m_subcktLabel->Wrap( -1 );
	bSizer10->Add( m_subcktLabel, 0, wxTOP|wxRIGHT|wxLEFT, 8 );

	m_subckt = new wxStyledTextCtrl( m_pinAssignmentsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString );
	m_subckt->SetUseTabs( true );
	m_subckt->SetTabWidth( 4 );
	m_subckt->SetIndent( 4 );
	m_subckt->SetTabIndents( true );
	m_subckt->SetBackSpaceUnIndents( true );
	m_subckt->SetViewEOL( false );
	m_subckt->SetViewWhiteSpace( false );
	m_subckt->SetMarginWidth( 2, 0 );
	m_subckt->SetIndentationGuides( true );
	m_subckt->SetReadOnly( false );
	m_subckt->SetMarginType( 1, wxSTC_MARGIN_SYMBOL );
	m_subckt->SetMarginMask( 1, wxSTC_MASK_FOLDERS );
	m_subckt->SetMarginWidth( 1, 16);
	m_subckt->SetMarginSensitive( 1, true );
	m_subckt->SetProperty( wxT("fold"), wxT("1") );
	m_subckt->SetFoldFlags( wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED );
	m_subckt->SetMarginType( 0, wxSTC_MARGIN_NUMBER );
	m_subckt->SetMarginWidth( 0, m_subckt->TextWidth( wxSTC_STYLE_LINENUMBER, wxT("_99999") ) );
	m_subckt->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_subckt->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_subckt->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_subckt->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_subckt->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_subckt->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_subckt->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_subckt->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_subckt->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_subckt->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_subckt->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_subckt->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_subckt->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_subckt->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_subckt->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_subckt->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_subckt->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	bSizer10->Add( m_subckt, 1, wxEXPAND | wxALL, 5 );


	m_pinAssignmentsPanel->SetSizer( bSizer10 );
	m_pinAssignmentsPanel->Layout();
	bSizer10->Fit( m_pinAssignmentsPanel );
	m_notebook->AddPage( m_pinAssignmentsPanel, _("Pin Assignments"), false );

	bSizerMain->Add( m_notebook, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );


	bSizerBottom->Add( 30, 0, 1, wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerBottom->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );


	bSizerMain->Add( bSizerBottom, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_rbLibraryModel->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onRadioButton ), NULL, this );
	m_pathLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathLabelUpdate ), NULL, this );
	m_libraryPathText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathTextKillFocus ), NULL, this );
	m_libraryPathText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathText ), NULL, this );
	m_libraryPathText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathTextEnter ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onBrowseButtonClick ), NULL, this );
	m_browseButton->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onBrowseButtonUpdate ), NULL, this );
	m_modelNameLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameLabelUpdate ), NULL, this );
	m_modelFilter->Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_SIM_MODEL_BASE::onFilterCharHook ), NULL, this );
	m_modelFilter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onModelFilter ), NULL, this );
	m_modelListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameChoice ), NULL, this );
	m_pinLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onIbisPinLabelUpdate ), NULL, this );
	m_pinCombobox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onPinCombobox ), NULL, this );
	m_pinCombobox->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxKillFocus ), NULL, this );
	m_pinCombobox->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onPinComboboxTextEnter ), NULL, this );
	m_pinCombobox->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_differentialCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onDifferentialCheckbox ), NULL, this );
	m_differentialCheckbox->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onOverrideCheckboxUpdate ), NULL, this );
	m_pinModelLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onIbisModelLabelUpdate ), NULL, this );
	m_pinModelCombobox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onPinModelCombobox ), NULL, this );
	m_pinModelCombobox->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxKillFocus ), NULL, this );
	m_pinModelCombobox->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onPinModelComboboxTextEnter ), NULL, this );
	m_pinModelCombobox->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_waveformChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onWaveformChoice ), NULL, this );
	m_rbBuiltinModel->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onRadioButton ), NULL, this );
	m_deviceLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeLabelUpdate ), NULL, this );
	m_deviceChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeChoice ), NULL, this );
	m_deviceChoice->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeChoiceUpdate ), NULL, this );
	m_deviceSubtypeLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onTypeLabelUpdate ), NULL, this );
	m_deviceSubtypeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onTypeChoice ), NULL, this );
	m_modelNotebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, wxNotebookEventHandler( DIALOG_SIM_MODEL_BASE::onPageChanging ), NULL, this );
	m_paramGridMgr->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SIM_MODEL_BASE::onSizeParamGrid ), NULL, this );
	m_pinAssignmentsGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SIM_MODEL_BASE::onPinAssignmentsGridCellChange ), NULL, this );
	m_pinAssignmentsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SIM_MODEL_BASE::onPinAssignmentsGridSize ), NULL, this );
}

DIALOG_SIM_MODEL_BASE::~DIALOG_SIM_MODEL_BASE()
{
	// Disconnect Events
	m_rbLibraryModel->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onRadioButton ), NULL, this );
	m_pathLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathLabelUpdate ), NULL, this );
	m_libraryPathText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathTextKillFocus ), NULL, this );
	m_libraryPathText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathText ), NULL, this );
	m_libraryPathText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onLibraryPathTextEnter ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onBrowseButtonClick ), NULL, this );
	m_browseButton->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onBrowseButtonUpdate ), NULL, this );
	m_modelNameLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameLabelUpdate ), NULL, this );
	m_modelFilter->Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_SIM_MODEL_BASE::onFilterCharHook ), NULL, this );
	m_modelFilter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onModelFilter ), NULL, this );
	m_modelListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameChoice ), NULL, this );
	m_pinLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onIbisPinLabelUpdate ), NULL, this );
	m_pinCombobox->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onPinCombobox ), NULL, this );
	m_pinCombobox->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxKillFocus ), NULL, this );
	m_pinCombobox->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onPinComboboxTextEnter ), NULL, this );
	m_pinCombobox->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_differentialCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onDifferentialCheckbox ), NULL, this );
	m_differentialCheckbox->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onOverrideCheckboxUpdate ), NULL, this );
	m_pinModelLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onIbisModelLabelUpdate ), NULL, this );
	m_pinModelCombobox->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onPinModelCombobox ), NULL, this );
	m_pinModelCombobox->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxKillFocus ), NULL, this );
	m_pinModelCombobox->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onPinModelComboboxTextEnter ), NULL, this );
	m_pinModelCombobox->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onModelNameComboboxUpdate ), NULL, this );
	m_waveformChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onWaveformChoice ), NULL, this );
	m_rbBuiltinModel->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onRadioButton ), NULL, this );
	m_deviceLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeLabelUpdate ), NULL, this );
	m_deviceChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeChoice ), NULL, this );
	m_deviceChoice->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onDeviceTypeChoiceUpdate ), NULL, this );
	m_deviceSubtypeLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_MODEL_BASE::onTypeLabelUpdate ), NULL, this );
	m_deviceSubtypeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_MODEL_BASE::onTypeChoice ), NULL, this );
	m_modelNotebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, wxNotebookEventHandler( DIALOG_SIM_MODEL_BASE::onPageChanging ), NULL, this );
	m_paramGridMgr->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SIM_MODEL_BASE::onSizeParamGrid ), NULL, this );
	m_pinAssignmentsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SIM_MODEL_BASE::onPinAssignmentsGridCellChange ), NULL, this );
	m_pinAssignmentsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SIM_MODEL_BASE::onPinAssignmentsGridSize ), NULL, this );

}

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
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( m_modelPanel, wxID_ANY, wxT("Properties") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer15;
	fgSizer15 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer15->AddGrowableCol( 1 );
	fgSizer15->SetFlexibleDirection( wxBOTH );
	fgSizer15->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText122 = new wxStaticText( sbSizer4->GetStaticBox(), wxID_ANY, wxT("Model Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText122->Wrap( -1 );
	fgSizer15->Add( m_staticText122, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_modelName = new wxTextCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer15->Add( m_modelName, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_browseButton = new wxButton( sbSizer4->GetStaticBox(), wxID_ANY, wxT("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer15->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticText124 = new wxStaticText( sbSizer4->GetStaticBox(), wxID_ANY, wxT("Location:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText124->Wrap( -1 );
	fgSizer15->Add( m_staticText124, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticText125 = new wxStaticText( sbSizer4->GetStaticBox(), wxID_ANY, wxT("/home/mikolaj/Downloads/1N4148.lib"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText125->Wrap( -1 );
	fgSizer15->Add( m_staticText125, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbSizer4->Add( fgSizer15, 1, wxEXPAND, 5 );


	bSizer9->Add( sbSizer4, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( m_modelPanel, wxID_ANY, wxT("Model") ), wxVERTICAL );

	m_checkBox2 = new wxCheckBox( sbSizer5->GetStaticBox(), wxID_ANY, wxT("Change parameters for this symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer5->Add( m_checkBox2, 0, wxALL, 5 );

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

	m_paramGridMgr = new wxPropertyGridManager(m_parametersPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPGMAN_DEFAULT_STYLE);
	m_paramGridMgr->SetExtraStyle( wxPG_EX_MODE_BUTTONS|wxPG_EX_NATIVE_DOUBLE_BUFFERING );
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

	m_pinAssignmentGrid = new WX_GRID( m_pinAssignmentsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_pinAssignmentGrid->CreateGrid( 0, 2 );
	m_pinAssignmentGrid->EnableEditing( true );
	m_pinAssignmentGrid->EnableGridLines( true );
	m_pinAssignmentGrid->EnableDragGridSize( false );
	m_pinAssignmentGrid->SetMargins( 0, 0 );

	// Columns
	m_pinAssignmentGrid->SetColSize( 0, 160 );
	m_pinAssignmentGrid->SetColSize( 1, 160 );
	m_pinAssignmentGrid->EnableDragColMove( false );
	m_pinAssignmentGrid->EnableDragColSize( true );
	m_pinAssignmentGrid->SetColLabelValue( 0, wxT("Schematic Pin") );
	m_pinAssignmentGrid->SetColLabelValue( 1, wxT("Model Pin") );
	m_pinAssignmentGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_pinAssignmentGrid->EnableDragRowSize( false );
	m_pinAssignmentGrid->SetRowLabelSize( 0 );
	m_pinAssignmentGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_pinAssignmentGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer10->Add( m_pinAssignmentGrid, 1, wxALL|wxEXPAND, 5 );


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
	m_deviceTypeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onDeviceTypeChoice ), NULL, this );
	m_typeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onTypeChoice ), NULL, this );
}

DIALOG_SPICE_MODEL_BASE::~DIALOG_SPICE_MODEL_BASE()
{
	// Disconnect Events
	m_deviceTypeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onDeviceTypeChoice ), NULL, this );
	m_typeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onTypeChoice ), NULL, this );

}

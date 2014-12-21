///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wizard_add_fplib_base.h"

///////////////////////////////////////////////////////////////////////////

WIZARD_FPLIB_TABLE_BASE::WIZARD_FPLIB_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxBitmap& bitmap, const wxPoint& pos, long style ) 
{
	this->Create( parent, id, title, bitmap, pos, style );
	this->SetSizeHints( wxSize( 450,-1 ), wxDefaultSize );
	
	wxWizardPageSimple* m_wizPage1 = new wxWizardPageSimple( this );
	m_pages.Add( m_wizPage1 );
	
	m_wizPage1->SetMinSize( wxSize( 500,-1 ) );
	
	wxBoxSizer* bSizerPage1;
	bSizerPage1 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_rbFpLibFormatChoices[] = { _("KiCad (*.Pretty folder containing .kicad_mod files)"), _("GitHub (.Pretty lib stored on GitHub depos)"), _("Legacy ( old *.mod lib file)"), _("Eagle V6 xml library file"), _("Geda footprint folder (folder containing *.fp files)") };
	int m_rbFpLibFormatNChoices = sizeof( m_rbFpLibFormatChoices ) / sizeof( wxString );
	m_rbFpLibFormat = new wxRadioBox( m_wizPage1, wxID_ANY, _("Library Format:"), wxDefaultPosition, wxDefaultSize, m_rbFpLibFormatNChoices, m_rbFpLibFormatChoices, 1, wxRA_SPECIFY_COLS );
	m_rbFpLibFormat->SetSelection( 0 );
	bSizerPage1->Add( m_rbFpLibFormat, 0, wxALL|wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( m_wizPage1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerPage1->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );
	
	m_bitmapGithubURL = new wxStaticBitmap( m_wizPage1, wxID_ANY, wxArtProvider::GetBitmap( wxART_INFORMATION, wxART_OTHER ), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerPage1->Add( m_bitmapGithubURL, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText10 = new wxStaticText( m_wizPage1, wxID_ANY, _("Default URL for KiCad libraries on Github:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	bSizerPage1->Add( m_staticText10, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlGithubURL = new wxTextCtrl( m_wizPage1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerPage1->Add( m_textCtrlGithubURL, 0, wxALL|wxEXPAND, 5 );
	
	
	m_wizPage1->SetSizer( bSizerPage1 );
	m_wizPage1->Layout();
	bSizerPage1->Fit( m_wizPage1 );
	wxWizardPageSimple* m_wizPage2 = new wxWizardPageSimple( this );
	m_pages.Add( m_wizPage2 );
	
	m_wizPage2->SetMinSize( wxSize( 450,-1 ) );
	
	wxBoxSizer* bSizerPage2;
	bSizerPage2 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_rbPathManagementChoices[] = { _("Use  path relative to the project"), _("Use environment variable in path"), _("Use absolute path") };
	int m_rbPathManagementNChoices = sizeof( m_rbPathManagementChoices ) / sizeof( wxString );
	m_rbPathManagement = new wxRadioBox( m_wizPage2, wxID_ANY, _("Path management:"), wxDefaultPosition, wxDefaultSize, m_rbPathManagementNChoices, m_rbPathManagementChoices, 1, wxRA_SPECIFY_COLS );
	m_rbPathManagement->SetSelection( 2 );
	bSizerPage2->Add( m_rbPathManagement, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText1 = new wxStaticText( m_wizPage2, wxID_ANY, _("Environment variables:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizerPage2->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_gridEnvironmentVariablesList = new wxGrid( m_wizPage2, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridEnvironmentVariablesList->CreateGrid( 3, 2 );
	m_gridEnvironmentVariablesList->EnableEditing( true );
	m_gridEnvironmentVariablesList->EnableGridLines( true );
	m_gridEnvironmentVariablesList->EnableDragGridSize( false );
	m_gridEnvironmentVariablesList->SetMargins( 0, 0 );
	
	// Columns
	m_gridEnvironmentVariablesList->SetColSize( 0, 125 );
	m_gridEnvironmentVariablesList->SetColSize( 1, 154 );
	m_gridEnvironmentVariablesList->AutoSizeColumns();
	m_gridEnvironmentVariablesList->EnableDragColMove( false );
	m_gridEnvironmentVariablesList->EnableDragColSize( true );
	m_gridEnvironmentVariablesList->SetColLabelSize( 30 );
	m_gridEnvironmentVariablesList->SetColLabelValue( 0, _("Environment Variable") );
	m_gridEnvironmentVariablesList->SetColLabelValue( 1, _("Path Segment") );
	m_gridEnvironmentVariablesList->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridEnvironmentVariablesList->AutoSizeRows();
	m_gridEnvironmentVariablesList->EnableDragRowSize( false );
	m_gridEnvironmentVariablesList->SetRowLabelSize( 40 );
	m_gridEnvironmentVariablesList->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridEnvironmentVariablesList->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerPage2->Add( m_gridEnvironmentVariablesList, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerButs;
	bSizerButs = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonAddEV = new wxButton( m_wizPage2, wxID_ANY, _("Add Environment Variable"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButs->Add( m_buttonAddEV, 0, wxALL, 5 );
	
	m_buttonRemoveEV = new wxButton( m_wizPage2, wxID_ANY, _("Remove Environment Variable"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButs->Add( m_buttonRemoveEV, 0, wxALL, 5 );
	
	
	bSizerPage2->Add( bSizerButs, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_wizPage2->SetSizer( bSizerPage2 );
	m_wizPage2->Layout();
	bSizerPage2->Fit( m_wizPage2 );
	wxWizardPageSimple* m_wizPage3 = new wxWizardPageSimple( this );
	m_pages.Add( m_wizPage3 );
	
	m_wizPage3->SetMinSize( wxSize( 450,-1 ) );
	
	wxBoxSizer* bSizerPage3;
	bSizerPage3 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_wizPage3, wxID_ANY, _("Options") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_textPluginTitle = new wxStaticText( m_wizPage3, wxID_ANY, _("Plugin type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textPluginTitle->Wrap( -1 );
	fgSizer1->Add( m_textPluginTitle, 0, wxALL, 5 );
	
	m_textPluginType = new wxStaticText( m_wizPage3, wxID_ANY, _("KiCad"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textPluginType->Wrap( -1 );
	fgSizer1->Add( m_textPluginType, 1, wxALL, 5 );
	
	m_textOptionTitle = new wxStaticText( m_wizPage3, wxID_ANY, _("Option:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOptionTitle->Wrap( -1 );
	fgSizer1->Add( m_textOptionTitle, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_textOption = new wxStaticText( m_wizPage3, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOption->Wrap( -1 );
	fgSizer1->Add( m_textOption, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_stPathTitle = new wxStaticText( m_wizPage3, wxID_ANY, _("Path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stPathTitle->Wrap( -1 );
	fgSizer1->Add( m_stPathTitle, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_textPath = new wxStaticText( m_wizPage3, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textPath->Wrap( -1 );
	fgSizer1->Add( m_textPath, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer1->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	
	bSizerPage3->Add( sbSizer1, 0, wxEXPAND|wxBOTTOM, 5 );
	
	m_staticText2 = new wxStaticText( m_wizPage3, wxID_ANY, _("Library list to add in Fp table:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizerPage3->Add( m_staticText2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	m_gridFpListLibs = new wxGrid( m_wizPage3, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridFpListLibs->CreateGrid( 0, 3 );
	m_gridFpListLibs->EnableEditing( true );
	m_gridFpListLibs->EnableGridLines( true );
	m_gridFpListLibs->EnableDragGridSize( false );
	m_gridFpListLibs->SetMargins( 0, 0 );
	
	// Columns
	m_gridFpListLibs->EnableDragColMove( false );
	m_gridFpListLibs->EnableDragColSize( true );
	m_gridFpListLibs->SetColLabelSize( 30 );
	m_gridFpListLibs->SetColLabelValue( 0, _("NickName") );
	m_gridFpListLibs->SetColLabelValue( 1, _("Path") );
	m_gridFpListLibs->SetColLabelValue( 2, _("Plugin") );
	m_gridFpListLibs->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridFpListLibs->EnableDragRowSize( true );
	m_gridFpListLibs->SetRowLabelSize( 30 );
	m_gridFpListLibs->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridFpListLibs->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer6->Add( m_gridFpListLibs, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizerPage3->Add( bSizer6, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonAddLib = new wxButton( m_wizPage3, wxID_ANY, _("Add FP Libraries"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_buttonAddLib, 0, wxALL, 5 );
	
	m_buttonRemoveLib = new wxButton( m_wizPage3, wxID_ANY, _("Remove FP Libraries"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_buttonRemoveLib, 0, wxALL, 5 );
	
	
	bSizerPage3->Add( bSizer5, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_wizPage3->SetSizer( bSizerPage3 );
	m_wizPage3->Layout();
	bSizerPage3->Fit( m_wizPage3 );
	
	this->Centre( wxBOTH );
	
	for ( unsigned int i = 1; i < m_pages.GetCount(); i++ )
	{
		m_pages.Item( i )->SetPrev( m_pages.Item( i - 1 ) );
		m_pages.Item( i - 1 )->SetNext( m_pages.Item( i ) );
	}
	
	// Connect Events
	this->Connect( wxID_ANY, wxEVT_WIZARD_FINISHED, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnFinish ) );
	this->Connect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGED, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPageChanged ) );
	this->Connect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGING, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPageChanging ) );
	m_rbFpLibFormat->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPluginSelection ), NULL, this );
	m_rbPathManagement->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPathManagementSelection ), NULL, this );
	m_gridEnvironmentVariablesList->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( WIZARD_FPLIB_TABLE_BASE::OnSelectEnvVarCell ), NULL, this );
	m_buttonAddEV->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnAddEVariable ), NULL, this );
	m_buttonRemoveEV->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnRemoveEVariable ), NULL, this );
	m_buttonAddLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnAddFpLibs ), NULL, this );
	m_buttonRemoveLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnRemoveFpLibs ), NULL, this );
}

WIZARD_FPLIB_TABLE_BASE::~WIZARD_FPLIB_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxID_ANY, wxEVT_WIZARD_FINISHED, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnFinish ) );
	this->Disconnect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGED, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPageChanged ) );
	this->Disconnect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGING, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPageChanging ) );
	m_rbFpLibFormat->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPluginSelection ), NULL, this );
	m_rbPathManagement->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPathManagementSelection ), NULL, this );
	m_gridEnvironmentVariablesList->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( WIZARD_FPLIB_TABLE_BASE::OnSelectEnvVarCell ), NULL, this );
	m_buttonAddEV->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnAddEVariable ), NULL, this );
	m_buttonRemoveEV->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnRemoveEVariable ), NULL, this );
	m_buttonAddLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnAddFpLibs ), NULL, this );
	m_buttonRemoveLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnRemoveFpLibs ), NULL, this );
	
	m_pages.Clear();
}

DIALOG_SELECT_DIRLIST_BASE::DIALOG_SELECT_DIRLIST_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextInfo = new wxStaticText( this, wxID_ANY, _("The footprint library is a folder.\nFootprints are files inside this folder."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticTextInfo->Wrap( -1 );
	m_staticTextInfo->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerMain->Add( m_staticTextInfo, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_dirCtrl = new wxGenericDirCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDIRCTRL_3D_INTERNAL|wxDIRCTRL_DIR_ONLY|wxDIRCTRL_MULTIPLE|wxSUNKEN_BORDER, wxEmptyString, 0 );
	
	m_dirCtrl->ShowHidden( false );
	bSizerMain->Add( m_dirCtrl, 1, wxEXPAND | wxALL, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnOKClick ), NULL, this );
}

DIALOG_SELECT_DIRLIST_BASE::~DIALOG_SELECT_DIRLIST_BASE()
{
	// Disconnect Events
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnOKClick ), NULL, this );
	
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wizard_add_fplib_base.h"

///////////////////////////////////////////////////////////////////////////

WIZARD_FPLIB_TABLE_BASE::WIZARD_FPLIB_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxBitmap& bitmap, const wxPoint& pos, long style ) 
{
	this->Create( parent, id, title, bitmap, pos, style );
	this->SetSizeHints( wxSize( 720,480 ), wxDefaultSize );
	
	wxWizardPageSimple* m_wizPage1 = new wxWizardPageSimple( this, NULL, NULL, wxArtProvider::GetBitmap( wxART_HELP_BOOK, wxART_FRAME_ICON ) );
	m_pages.Add( m_wizPage1 );
	
	m_wizPage1->SetMinSize( wxSize( 720,480 ) );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer112;
	fgSizer112 = new wxFlexGridSizer( 3, 1, 0, 0 );
	fgSizer112->AddGrowableRow( 1 );
	fgSizer112->SetFlexibleDirection( wxBOTH );
	fgSizer112->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( m_wizPage1, wxID_ANY, wxT("Welcome to the Add Footprint Libraries Wizard!\n\nPlease select the source for the libraries to add:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer112->Add( m_staticText1, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxVERTICAL );
	
	m_radioAddLocal = new wxRadioButton( m_wizPage1, wxID_ANY, wxT("Files on my computer"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer19->Add( m_radioAddLocal, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* m_sizerGithub;
	m_sizerGithub = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer19->Add( m_sizerGithub, 0, wxEXPAND, 5 );
	
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 2, 2, 0, 0 );
	
	
	bSizer19->Add( gSizer1, 0, wxEXPAND, 5 );
	
	wxFlexGridSizer* m_githubSizer;
	m_githubSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
	m_githubSizer->SetFlexibleDirection( wxBOTH );
	m_githubSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_radioAddGithub = new wxRadioButton( m_wizPage1, wxID_ANY, wxT("Github repository"), wxDefaultPosition, wxDefaultSize, 0 );
	m_githubSizer->Add( m_radioAddGithub, 0, wxALL|wxEXPAND, 5 );
	
	m_textCtrlGithubURL = new wxTextCtrl( m_wizPage1, wxID_ANY, wxT("http://github.com/KiCad"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlGithubURL->SetMinSize( wxSize( 300,-1 ) );
	
	m_githubSizer->Add( m_textCtrlGithubURL, 1, wxALL|wxEXPAND, 5 );
	
	
	m_githubSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_downloadGithub = new wxCheckBox( m_wizPage1, wxID_ANY, wxT("Save a local copy to:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_downloadGithub->SetValue(true); 
	m_githubSizer->Add( m_downloadGithub, 0, wxALL, 5 );
	
	
	m_githubSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );
	
	m_downloadDir = new wxStaticText( m_wizPage1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_downloadDir->Wrap( -1 );
	m_downloadDir->SetMinSize( wxSize( 300,-1 ) );
	
	bSizer9->Add( m_downloadDir, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_btnBrowse = new wxButton( m_wizPage1, wxID_ANY, wxT("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_btnBrowse, 0, wxALL, 5 );
	
	
	m_githubSizer->Add( bSizer9, 1, wxEXPAND, 5 );
	
	
	m_githubSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_invalidDir = new wxStaticText( m_wizPage1, wxID_ANY, wxT("It is not possible to write in the selected directory.\nPlease choose another one."), wxDefaultPosition, wxDefaultSize, 0 );
	m_invalidDir->Wrap( -1 );
	m_invalidDir->SetForegroundColour( wxColour( 255, 0, 0 ) );
	
	m_githubSizer->Add( m_invalidDir, 0, wxALL, 5 );
	
	
	bSizer19->Add( m_githubSizer, 1, wxEXPAND, 5 );
	
	
	fgSizer112->Add( bSizer19, 1, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer12;
	fgSizer12 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer12->SetFlexibleDirection( wxBOTH );
	fgSizer12->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText8 = new wxStaticText( m_wizPage1, wxID_ANY, wxT("Visit the official"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizer12->Add( m_staticText8, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_hyperlink1 = new wxHyperlinkCtrl( m_wizPage1, wxID_ANY, wxT("Kicad repository on Github"), wxT("https://github.com/KiCad"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	fgSizer12->Add( m_hyperlink1, 0, wxTOP|wxBOTTOM, 5 );
	
	m_staticText9 = new wxStaticText( m_wizPage1, wxID_ANY, wxT("to find numerous footprint libraries!"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer12->Add( m_staticText9, 0, wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizer112->Add( fgSizer12, 1, wxEXPAND, 5 );
	
	
	bSizer1->Add( fgSizer112, 1, wxEXPAND, 5 );
	
	
	m_wizPage1->SetSizer( bSizer1 );
	m_wizPage1->Layout();
	wxWizardPageSimple* m_wizPage2_Local = new wxWizardPageSimple( this, NULL, NULL, wxArtProvider::GetBitmap( wxART_HELP_BOOK, wxART_FRAME_ICON ) );
	m_pages.Add( m_wizPage2_Local );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText7 = new wxStaticText( m_wizPage2_Local, wxID_ANY, wxT("Select files or folders to add:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	bSizer8->Add( m_staticText7, 0, wxALL, 5 );
	
	m_filePicker = new wxGenericDirCtrl( m_wizPage2_Local, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDIRCTRL_3D_INTERNAL|wxDIRCTRL_MULTIPLE|wxDIRCTRL_SHOW_FILTERS|wxSUNKEN_BORDER, wxEmptyString, 0 );
	
	m_filePicker->ShowHidden( false );
	bSizer8->Add( m_filePicker, 1, wxEXPAND | wxALL, 5 );
	
	
	m_wizPage2_Local->SetSizer( bSizer8 );
	m_wizPage2_Local->Layout();
	bSizer8->Fit( m_wizPage2_Local );
	wxWizardPageSimple* m_wizPage2_Github = new wxWizardPageSimple( this, NULL, NULL, wxArtProvider::GetBitmap( wxART_HELP_BOOK, wxART_FRAME_ICON ) );
	m_pages.Add( m_wizPage2_Github );
	
	wxBoxSizer* bSizer111;
	bSizer111 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText112 = new wxStaticText( m_wizPage2_Github, wxID_ANY, wxT("Select Github libraries to add:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText112->Wrap( -1 );
	bSizer111->Add( m_staticText112, 0, wxALL|wxEXPAND, 5 );
	
	wxArrayString m_checkListGHChoices;
	m_checkListGH = new wxCheckListBox( m_wizPage2_Github, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_checkListGHChoices, wxLB_MULTIPLE|wxLB_NEEDED_SB );
	bSizer111->Add( m_checkListGH, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );
	
	m_btnSelectAllGH = new wxButton( m_wizPage2_Github, wxID_ANY, wxT("Select all"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_btnSelectAllGH, 1, wxALL, 5 );
	
	m_btnUnselectAllGH = new wxButton( m_wizPage2_Github, wxID_ANY, wxT("Unselect all"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_btnUnselectAllGH, 1, wxALL, 5 );
	
	
	bSizer7->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_searchCtrlGH = new wxSearchCtrl( m_wizPage2_Github, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_searchCtrlGH->ShowSearchButton( true );
	#endif
	m_searchCtrlGH->ShowCancelButton( false );
	bSizer7->Add( m_searchCtrlGH, 2, wxALL, 5 );
	
	
	bSizer111->Add( bSizer7, 0, wxEXPAND, 5 );
	
	
	m_wizPage2_Github->SetSizer( bSizer111 );
	m_wizPage2_Github->Layout();
	bSizer111->Fit( m_wizPage2_Github );
	wxWizardPageSimple* m_wizPage3_Review = new wxWizardPageSimple( this, NULL, NULL, wxArtProvider::GetBitmap( wxART_HELP_BOOK, wxART_FRAME_ICON ) );
	m_pages.Add( m_wizPage3_Review );
	
	wxBoxSizer* bSizer1111;
	bSizer1111 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1121 = new wxStaticText( m_wizPage3_Review, wxID_ANY, wxT("Review and confirm the changes to the libraries:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1121->Wrap( -1 );
	bSizer1111->Add( m_staticText1121, 0, wxALL|wxEXPAND, 5 );
	
	m_listCtrlReview = new wxDataViewListCtrl( m_wizPage3_Review, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES|wxDV_ROW_LINES|wxDV_VERT_RULES );
	m_dvLibName = m_listCtrlReview->AppendTextColumn( wxT("Library") ); 
	m_dvLibStatus = m_listCtrlReview->AppendTextColumn( wxT("Status") ); 
	m_dvLibFormat = m_listCtrlReview->AppendTextColumn( wxT("Format") ); 
	bSizer1111->Add( m_listCtrlReview, 1, wxALL|wxEXPAND, 5 );
	
	
	m_wizPage3_Review->SetSizer( bSizer1111 );
	m_wizPage3_Review->Layout();
	bSizer1111->Fit( m_wizPage3_Review );
	wxWizardPageSimple* m_wizPage4_SelectTarget = new wxWizardPageSimple( this, NULL, NULL, wxArtProvider::GetBitmap( wxART_HELP_BOOK, wxART_FRAME_ICON ) );
	m_pages.Add( m_wizPage4_SelectTarget );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText12 = new wxStaticText( m_wizPage4_SelectTarget, wxID_ANY, wxT("Where do you wish the new libraries to be added:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizer12->Add( m_staticText12, 0, wxALL|wxEXPAND, 5 );
	
	m_radioGlobal = new wxRadioButton( m_wizPage4_SelectTarget, wxID_ANY, wxT("To global library configuration (visible by all projects)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_radioGlobal, 0, wxALL|wxEXPAND, 5 );
	
	m_radioProject = new wxRadioButton( m_wizPage4_SelectTarget, wxID_ANY, wxT("To the current project only"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_radioProject, 0, wxALL|wxEXPAND, 5 );
	
	
	m_wizPage4_SelectTarget->SetSizer( bSizer12 );
	m_wizPage4_SelectTarget->Layout();
	bSizer12->Fit( m_wizPage4_SelectTarget );
	
	this->Centre( wxBOTH );
	
	for ( unsigned int i = 1; i < m_pages.GetCount(); i++ )
	{
		m_pages.Item( i )->SetPrev( m_pages.Item( i - 1 ) );
		m_pages.Item( i - 1 )->SetNext( m_pages.Item( i ) );
	}
	
	// Connect Events
	this->Connect( wxID_ANY, wxEVT_WIZARD_FINISHED, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnWizardFinished ) );
	this->Connect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGED, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPageChanged ) );
	this->Connect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGING, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPageChanging ) );
	m_downloadGithub->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnCheckSaveCopy ), NULL, this );
	m_btnBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnBrowseButtonClick ), NULL, this );
	m_btnSelectAllGH->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnSelectAllGH ), NULL, this );
	m_btnUnselectAllGH->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnUnselectAllGH ), NULL, this );
	m_searchCtrlGH->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnChangeSearch ), NULL, this );
}

WIZARD_FPLIB_TABLE_BASE::~WIZARD_FPLIB_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxID_ANY, wxEVT_WIZARD_FINISHED, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnWizardFinished ) );
	this->Disconnect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGED, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPageChanged ) );
	this->Disconnect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGING, wxWizardEventHandler( WIZARD_FPLIB_TABLE_BASE::OnPageChanging ) );
	m_downloadGithub->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnCheckSaveCopy ), NULL, this );
	m_btnBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnBrowseButtonClick ), NULL, this );
	m_btnSelectAllGH->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnSelectAllGH ), NULL, this );
	m_btnUnselectAllGH->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnUnselectAllGH ), NULL, this );
	m_searchCtrlGH->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( WIZARD_FPLIB_TABLE_BASE::OnChangeSearch ), NULL, this );
	
	m_pages.Clear();
}

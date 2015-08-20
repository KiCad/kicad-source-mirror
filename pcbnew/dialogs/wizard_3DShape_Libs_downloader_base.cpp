///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wizard_3DShape_Libs_downloader_base.h"

///////////////////////////////////////////////////////////////////////////

WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxBitmap& bitmap, const wxPoint& pos, long style ) 
{
	this->Create( parent, id, title, bitmap, pos, style );
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxWizardPageSimple* m_wizPage1 = new wxWizardPageSimple( this, NULL, NULL, wxArtProvider::GetBitmap( wxART_HELP_BOOK, wxART_FRAME_ICON ) );
	m_pages.Add( m_wizPage1 );
	
	m_wizPage1->SetMinSize( wxSize( 720,480 ) );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( m_wizPage1, wxID_ANY, _("Welcome to the 3D shape Libraries downloader Wizard!"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer1->Add( m_staticText1, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer1->Add( 0, 20, 0, 0, 5 );
	
	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText8 = new wxStaticText( m_wizPage1, wxID_ANY, _("Please select the URL for the 3D libraries to download"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	bSizer19->Add( m_staticText8, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlGithubURL = new wxTextCtrl( m_wizPage1, wxID_ANY, _("http://github.com/KiCad"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlGithubURL->SetMinSize( wxSize( 300,-1 ) );
	
	bSizer19->Add( m_textCtrlGithubURL, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer19->Add( 0, 10, 0, 0, 5 );
	
	wxBoxSizer* bSizerLocalFolder;
	bSizerLocalFolder = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerDinname;
	bSizerDinname = new wxBoxSizer( wxVERTICAL );
	
	m_staticText9 = new wxStaticText( m_wizPage1, wxID_ANY, _("3D shape local folder:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	bSizerDinname->Add( m_staticText9, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_downloadDir = new wxTextCtrl( m_wizPage1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDinname->Add( m_downloadDir, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizerLocalFolder->Add( bSizerDinname, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizerButts;
	bSizerButts = new wxBoxSizer( wxVERTICAL );
	
	m_btnBrowse = new wxButton( m_wizPage1, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButts->Add( m_btnBrowse, 0, wxEXPAND|wxALL, 5 );
	
	m_buttonDefault3DPath = new wxButton( m_wizPage1, wxID_ANY, _("Default 3D Path"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButts->Add( m_buttonDefault3DPath, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerLocalFolder->Add( bSizerButts, 0, wxEXPAND, 5 );
	
	
	bSizer19->Add( bSizerLocalFolder, 0, wxEXPAND, 5 );
	
	
	bSizer19->Add( 0, 10, 0, 0, 5 );
	
	m_invalidDir = new wxStaticText( m_wizPage1, wxID_ANY, _("It is not possible to write in the selected directory.\nPlease choose another one."), wxDefaultPosition, wxDefaultSize, 0 );
	m_invalidDir->Wrap( -1 );
	m_invalidDir->SetForegroundColour( wxColour( 255, 0, 0 ) );
	
	bSizer19->Add( m_invalidDir, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer1->Add( bSizer19, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerOfficialRepo;
	bSizerOfficialRepo = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapRepo = new wxStaticBitmap( m_wizPage1, wxID_ANY, wxArtProvider::GetBitmap( wxART_INFORMATION, wxART_OTHER ), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerOfficialRepo->Add( m_bitmapRepo, 0, wxALL, 5 );
	
	
	bSizerOfficialRepo->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	m_hyperlinkGithubKicad = new wxHyperlinkCtrl( m_wizPage1, wxID_ANY, _("Visit our official Kicad repository on Github and get more libraries"), wxT("https://github.com/KiCad"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	bSizerOfficialRepo->Add( m_hyperlinkGithubKicad, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer1->Add( bSizerOfficialRepo, 0, wxEXPAND, 5 );
	
	
	m_wizPage1->SetSizer( bSizer1 );
	m_wizPage1->Layout();
	wxWizardPageSimple* m_wizPage2_Github = new wxWizardPageSimple( this, NULL, NULL, wxArtProvider::GetBitmap( wxART_HELP_BOOK, wxART_FRAME_ICON ) );
	m_pages.Add( m_wizPage2_Github );
	
	wxBoxSizer* bSizer111;
	bSizer111 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText112 = new wxStaticText( m_wizPage2_Github, wxID_ANY, _("Select Github libraries to add:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText112->Wrap( -1 );
	bSizer111->Add( m_staticText112, 0, wxALL|wxEXPAND, 5 );
	
	wxArrayString m_checkList3DlibnamesChoices;
	m_checkList3Dlibnames = new wxCheckListBox( m_wizPage2_Github, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_checkList3DlibnamesChoices, wxLB_MULTIPLE|wxLB_NEEDED_SB );
	bSizer111->Add( m_checkList3Dlibnames, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );
	
	m_btnSelectAll3Dlibs = new wxButton( m_wizPage2_Github, wxID_ANY, _("Select all"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_btnSelectAll3Dlibs, 1, wxALL, 5 );
	
	m_btnUnselectAll3Dlibs = new wxButton( m_wizPage2_Github, wxID_ANY, _("Unselect all"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_btnUnselectAll3Dlibs, 1, wxALL, 5 );
	
	
	bSizer7->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_searchCtrl3Dlibs = new wxSearchCtrl( m_wizPage2_Github, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_searchCtrl3Dlibs->ShowSearchButton( true );
	#endif
	m_searchCtrl3Dlibs->ShowCancelButton( false );
	bSizer7->Add( m_searchCtrl3Dlibs, 2, wxALL, 5 );
	
	
	bSizer111->Add( bSizer7, 0, wxEXPAND, 5 );
	
	
	m_wizPage2_Github->SetSizer( bSizer111 );
	m_wizPage2_Github->Layout();
	bSizer111->Fit( m_wizPage2_Github );
	wxWizardPageSimple* m_wizPage3_SelectTarget = new wxWizardPageSimple( this, NULL, NULL, wxArtProvider::GetBitmap( wxART_HELP_BOOK, wxART_FRAME_ICON ) );
	m_pages.Add( m_wizPage3_SelectTarget );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextlocalfolder = new wxStaticText( m_wizPage3_SelectTarget, wxID_ANY, _("Local library folder:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextlocalfolder->Wrap( -1 );
	bSizer12->Add( m_staticTextlocalfolder, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_LocalFolderInfo = new wxStaticText( m_wizPage3_SelectTarget, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LocalFolderInfo->Wrap( -1 );
	bSizer12->Add( m_LocalFolderInfo, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText12 = new wxStaticText( m_wizPage3_SelectTarget, wxID_ANY, _("3D shape libraries to be downloaded:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizer12->Add( m_staticText12, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_gridLibReview = new wxGrid( m_wizPage3_SelectTarget, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridLibReview->CreateGrid( 1, 2 );
	m_gridLibReview->EnableEditing( false );
	m_gridLibReview->EnableGridLines( true );
	m_gridLibReview->EnableDragGridSize( false );
	m_gridLibReview->SetMargins( 5, 0 );
	
	// Columns
	m_gridLibReview->EnableDragColMove( false );
	m_gridLibReview->EnableDragColSize( true );
	m_gridLibReview->SetColLabelSize( 30 );
	m_gridLibReview->SetColLabelValue( 0, _("Status") );
	m_gridLibReview->SetColLabelValue( 1, _("Libraries") );
	m_gridLibReview->SetColLabelValue( 2, wxEmptyString );
	m_gridLibReview->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridLibReview->AutoSizeRows();
	m_gridLibReview->EnableDragRowSize( true );
	m_gridLibReview->SetRowLabelSize( 30 );
	m_gridLibReview->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridLibReview->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer12->Add( m_gridLibReview, 1, wxALL|wxEXPAND, 5 );
	
	
	m_wizPage3_SelectTarget->SetSizer( bSizer12 );
	m_wizPage3_SelectTarget->Layout();
	bSizer12->Fit( m_wizPage3_SelectTarget );
	
	this->Centre( wxBOTH );
	
	for ( unsigned int i = 1; i < m_pages.GetCount(); i++ )
	{
		m_pages.Item( i )->SetPrev( m_pages.Item( i - 1 ) );
		m_pages.Item( i - 1 )->SetNext( m_pages.Item( i ) );
	}
	
	// Connect Events
	this->Connect( wxID_ANY, wxEVT_WIZARD_FINISHED, wxWizardEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnWizardFinished ) );
	this->Connect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGED, wxWizardEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnPageChanged ) );
	this->Connect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGING, wxWizardEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnPageChanging ) );
	m_btnBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnBrowseButtonClick ), NULL, this );
	m_buttonDefault3DPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnDefault3DPathButtonClick ), NULL, this );
	m_btnSelectAll3Dlibs->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnSelectAll3Dlibs ), NULL, this );
	m_btnUnselectAll3Dlibs->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnUnselectAll3Dlibs ), NULL, this );
	m_searchCtrl3Dlibs->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnChangeSearch ), NULL, this );
	m_gridLibReview->Connect( wxEVT_SIZE, wxSizeEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnGridLibReviewSize ), NULL, this );
}

WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::~WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE()
{
	// Disconnect Events
	this->Disconnect( wxID_ANY, wxEVT_WIZARD_FINISHED, wxWizardEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnWizardFinished ) );
	this->Disconnect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGED, wxWizardEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnPageChanged ) );
	this->Disconnect( wxID_ANY, wxEVT_WIZARD_PAGE_CHANGING, wxWizardEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnPageChanging ) );
	m_btnBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnBrowseButtonClick ), NULL, this );
	m_buttonDefault3DPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnDefault3DPathButtonClick ), NULL, this );
	m_btnSelectAll3Dlibs->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnSelectAll3Dlibs ), NULL, this );
	m_btnUnselectAll3Dlibs->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnUnselectAll3Dlibs ), NULL, this );
	m_searchCtrl3Dlibs->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnChangeSearch ), NULL, this );
	m_gridLibReview->Disconnect( wxEVT_SIZE, wxSizeEventHandler( WIZARD_3DSHAPE_LIBS_DOWNLOADER_BASE::OnGridLibReviewSize ), NULL, this );
	
	m_pages.Clear();
}

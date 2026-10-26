///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_migrate_3d_models_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MIGRATE_3D_MODELS_BASE::DIALOG_MIGRATE_3D_MODELS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bHeader;
	bHeader = new wxBoxSizer( wxVERTICAL );

	m_headerLabel = new wxStaticText( this, wxID_ANY, _("The board references VRML 3D models (*.wrl files) that are no longer present on your system."), wxDefaultPosition, wxDefaultSize, 0 );
	m_headerLabel->Wrap( -1 );
	bHeader->Add( m_headerLabel, 0, wxBOTTOM, 10 );

	m_subHeader = new wxStaticText( this, wxID_ANY, _("KiCad will auto-select suitable STEP file matches from the configured 3D directories, plus any additional directories specified below.  "), wxDefaultPosition, wxDefaultSize, 0 );
	m_subHeader->Wrap( -1 );
	bHeader->Add( m_subHeader, 0, wxBOTTOM, 4 );

	m_subHeader2 = new wxStaticText( this, wxID_ANY, _("Individual matches can be overridden in the Replacement list, either with another candidate from the search directories, or with a specific STEP model file."), wxDefaultPosition, wxDefaultSize, 0 );
	m_subHeader2->Wrap( -1 );
	bHeader->Add( m_subHeader2, 0, wxBOTTOM, 10 );


	bSizerMain->Add( bHeader, 0, wxEXPAND|wxALL, 10 );

	m_mainSplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxSP_NOBORDER );
	m_mainSplitter->SetSashGravity( 0.66 );
	m_mainSplitter->SetMinimumPaneSize( 240 );

	m_leftPanel = new wxPanel( m_mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxBoxSizer* bSizerMissing;
	bSizerMissing = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );

	m_splitter3 = new wxSplitterWindow( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxSP_NOBORDER );
	m_splitter3->SetSashGravity( 0.5 );
	m_splitter3->SetMinimumPaneSize( 120 );

	m_paneMissingModels = new wxPanel( m_splitter3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_missingLabel = new wxStaticText( m_paneMissingModels, wxID_ANY, _("Missing 3D model references:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_missingLabel->Wrap( -1 );
	bSizer8->Add( m_missingLabel, 0, wxBOTTOM|wxLEFT, 3 );

	m_missingList = new wxListCtrl( m_paneMissingModels, wxID_ANY, wxDefaultPosition, wxSize( 330,-1 ), wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL|wxBORDER_THEME );
	bSizer8->Add( m_missingList, 1, wxEXPAND, 5 );

	m_helpText = new wxStaticText( m_paneMissingModels, wxID_ANY, _("References with no match (auto or override) are shown in bold."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpText->Wrap( -1 );
	bSizer8->Add( m_helpText, 0, wxTOP|wxLEFT, 3 );


	bSizer8->Add( 0, 12, 0, 0, 5 );


	m_paneMissingModels->SetSizer( bSizer8 );
	m_paneMissingModels->Layout();
	bSizer8->Fit( m_paneMissingModels );
	m_paneReplacements = new wxPanel( m_splitter3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	m_candidatesLabel = new wxStaticText( m_paneReplacements, wxID_ANY, _("Replacement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_candidatesLabel->Wrap( -1 );
	bSizer9->Add( m_candidatesLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT, 3 );

	m_candidatesList = new wxListCtrl( m_paneReplacements, wxID_ANY, wxDefaultPosition, wxSize( 330,-1 ), wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL|wxBORDER_THEME );
	bSizer9->Add( m_candidatesList, 1, wxEXPAND, 5 );

	wxBoxSizer* bButtonBar;
	bButtonBar = new wxBoxSizer( wxHORIZONTAL );

	m_addReplacement = new STD_BITMAP_BUTTON( m_paneReplacements, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_addReplacement->SetToolTip( _("Select a STEP model file from disk as a replacement") );

	bButtonBar->Add( m_addReplacement, 0, wxALL, 3 );


	bSizer9->Add( bButtonBar, 0, wxEXPAND, 5 );


	m_paneReplacements->SetSizer( bSizer9 );
	m_paneReplacements->Layout();
	bSizer9->Fit( m_paneReplacements );
	m_splitter3->SplitVertically( m_paneMissingModels, m_paneReplacements, -1 );
	bSizerUpper->Add( m_splitter3, 1, wxEXPAND, 5 );


	bSizerMissing->Add( bSizerUpper, 8, wxEXPAND, 5 );

	wxBoxSizer* bSizerLower;
	bSizerLower = new wxBoxSizer( wxVERTICAL );

	searchDirectoriesLabel = new wxStaticText( m_leftPanel, wxID_ANY, _("Extra 3D model search directories:"), wxDefaultPosition, wxDefaultSize, 0 );
	searchDirectoriesLabel->Wrap( -1 );
	bSizerLower->Add( searchDirectoriesLabel, 0, wxTOP|wxBOTTOM|wxLEFT, 3 );

	m_searchDirsGrid = new WX_GRID( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_searchDirsGrid->CreateGrid( 3, 1 );
	m_searchDirsGrid->EnableEditing( false );
	m_searchDirsGrid->EnableGridLines( true );
	m_searchDirsGrid->EnableDragGridSize( false );
	m_searchDirsGrid->SetMargins( 0, 0 );

	// Columns
	m_searchDirsGrid->EnableDragColMove( false );
	m_searchDirsGrid->EnableDragColSize( true );
	m_searchDirsGrid->SetColLabelSize( 0 );
	m_searchDirsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_searchDirsGrid->EnableDragRowSize( true );
	m_searchDirsGrid->SetRowLabelSize( 0 );
	m_searchDirsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_searchDirsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerLower->Add( m_searchDirsGrid, 1, wxEXPAND, 5 );


	bSizerMissing->Add( bSizerLower, 3, wxEXPAND|wxTOP, 15 );


	m_leftPanel->SetSizer( bSizerMissing );
	m_leftPanel->Layout();
	bSizerMissing->Fit( m_leftPanel );
	m_previewPanel = new wxPanel( m_mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxBoxSizer* bSizerPreview;
	bSizerPreview = new wxBoxSizer( wxVERTICAL );

	m_previewLabel = new wxStaticText( m_previewPanel, wxID_ANY, _("Preview:"), wxDefaultPosition, wxSize( 330,-1 ), 0 );
	m_previewLabel->Wrap( -1 );
	bSizerPreview->Add( m_previewLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT, 3 );


	m_previewPanel->SetSizer( bSizerPreview );
	m_previewPanel->Layout();
	bSizerPreview->Fit( m_previewPanel );
	m_mainSplitter->SplitVertically( m_leftPanel, m_previewPanel, -1 );
	bSizerMain->Add( m_mainSplitter, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_addSearchDir = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_addSearchDir->SetToolTip( _("Add a directory to be included in the search for suitable 3D model replacement files") );

	bSizerBottom->Add( m_addSearchDir, 0, wxRIGHT|wxLEFT, 5 );

	m_deleteSearchDir = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_deleteSearchDir->SetToolTip( _("Remove the selected directory from the list.  Its contents will no longer be considered in the search for suitable replacements.") );

	bSizerBottom->Add( m_deleteSearchDir, 0, wxLEFT, 20 );


	bSizerBottom->Add( 0, 0, 1, wxEXPAND, 0 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerBottom->Add( m_sdbSizer1, 1, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bSizerBottom, 0, wxEXPAND|wxTOP, 3 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_missingList->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnMissingSelected ), NULL, this );
	m_candidatesList->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnCandidateSelected ), NULL, this );
	m_addReplacement->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnOpenExternalFileClick ), NULL, this );
	m_addSearchDir->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnAddSearchDirectoryClick ), NULL, this );
	m_deleteSearchDir->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnRemoveSearchDirectoryClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnKeepClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnReplaceClick ), NULL, this );
}

DIALOG_MIGRATE_3D_MODELS_BASE::~DIALOG_MIGRATE_3D_MODELS_BASE()
{
	// Disconnect Events
	m_missingList->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnMissingSelected ), NULL, this );
	m_candidatesList->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnCandidateSelected ), NULL, this );
	m_addReplacement->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnOpenExternalFileClick ), NULL, this );
	m_addSearchDir->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnAddSearchDirectoryClick ), NULL, this );
	m_deleteSearchDir->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnRemoveSearchDirectoryClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnKeepClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnReplaceClick ), NULL, this );

}

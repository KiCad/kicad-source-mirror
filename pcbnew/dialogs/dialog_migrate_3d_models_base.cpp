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

	m_headerLabel = new wxStaticText( this, wxID_ANY, _("This board references 3D models that are no longer present on your system.\nKiCad can auto-select suitable matches from the configured 3D directories, plus any additional directories specified below.  \nIndividual matches can be overridden in the Replacement list, either with another candidate from the search directories, or with a specific model file."), wxDefaultPosition, wxDefaultSize, 0 );
	m_headerLabel->Wrap( -1 );
	bSizerMain->Add( m_headerLabel, 0, wxALL, 8 );

	m_mainSplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxSP_NOBORDER );
	m_mainSplitter->SetSashGravity( 0.66 );
	m_mainSplitter->SetMinimumPaneSize( 240 );

	m_leftPanel = new wxPanel( m_mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxBoxSizer* bSizerMissing;
	bSizerMissing = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );

	m_splitter3 = new wxSplitterWindow( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_NOBORDER );
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
	bButtonBar->Add( m_addReplacement, 0, wxTOP|wxRIGHT, 5 );


	bSizer9->Add( bButtonBar, 0, wxEXPAND, 5 );


	m_paneReplacements->SetSizer( bSizer9 );
	m_paneReplacements->Layout();
	bSizer9->Fit( m_paneReplacements );
	m_splitter3->SplitVertically( m_paneMissingModels, m_paneReplacements, -1 );
	bSizerUpper->Add( m_splitter3, 1, wxEXPAND, 5 );


	bSizerMissing->Add( bSizerUpper, 3, wxEXPAND, 5 );

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

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );


	bSizerLower->Add( bButtonsSizer, 0, wxEXPAND, 5 );


	bSizerMissing->Add( bSizerLower, 1, wxEXPAND|wxTOP, 10 );


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
	bSizerMain->Add( m_mainSplitter, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_addSearchDir = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerBottom->Add( m_addSearchDir, 0, wxRIGHT|wxLEFT, 5 );

	m_deleteSearchDir = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerBottom->Add( m_deleteSearchDir, 0, wxLEFT, 20 );


	bSizerBottom->Add( 0, 0, 1, wxEXPAND, 0 );

	m_replaceButton = new wxButton( this, wxID_ANY, _("Replace Models"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_replaceButton, 0, wxALL|wxALIGN_CENTER_VERTICAL, 15 );

	m_keepButton = new wxButton( this, wxID_CANCEL, _("Keep Existing"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_keepButton, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 15 );


	bSizerMain->Add( bSizerBottom, 0, wxEXPAND, 5 );


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
	m_replaceButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnReplaceClick ), NULL, this );
	m_keepButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnKeepClick ), NULL, this );
}

DIALOG_MIGRATE_3D_MODELS_BASE::~DIALOG_MIGRATE_3D_MODELS_BASE()
{
	// Disconnect Events
	m_missingList->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnMissingSelected ), NULL, this );
	m_candidatesList->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnCandidateSelected ), NULL, this );
	m_addReplacement->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnOpenExternalFileClick ), NULL, this );
	m_addSearchDir->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnAddSearchDirectoryClick ), NULL, this );
	m_deleteSearchDir->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnRemoveSearchDirectoryClick ), NULL, this );
	m_replaceButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnReplaceClick ), NULL, this );
	m_keepButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnKeepClick ), NULL, this );

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_migrate_3d_models_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MIGRATE_3D_MODELS_BASE::DIALOG_MIGRATE_3D_MODELS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 640,400 ), wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_headerLabel = new wxStaticText( this, wxID_ANY, _("This board references 3D models that are no longer present on your system.\nSelect replacements below, or keep the existing references unchanged."), wxDefaultPosition, wxDefaultSize, 0 );
	m_headerLabel->Wrap( -1 );
	bSizerMain->Add( m_headerLabel, 0, wxALL, 8 );

	m_mainSplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_mainSplitter->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::m_mainSplitterOnIdle ), NULL, this );
	m_mainSplitter->SetMinimumPaneSize( 120 );

	m_leftPanel = new wxPanel( m_mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxBoxSizer* bSizerMissing;
	bSizerMissing = new wxBoxSizer( wxVERTICAL );

	m_missingLabel = new wxStaticText( m_leftPanel, wxID_ANY, _("Missing 3D model references:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_missingLabel->Wrap( -1 );
	bSizerMissing->Add( m_missingLabel, 0, wxLEFT|wxRIGHT|wxTOP, 4 );

	m_missingList = new wxListCtrl( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL|wxBORDER_THEME );
	bSizerMissing->Add( m_missingList, 1, wxALL|wxEXPAND, 4 );


	m_leftPanel->SetSizer( bSizerMissing );
	m_leftPanel->Layout();
	bSizerMissing->Fit( m_leftPanel );
	m_rightContainerPanel = new wxPanel( m_mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	m_innerSplitter = new wxSplitterWindow( m_rightContainerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_innerSplitter->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::m_innerSplitterOnIdle ), NULL, this );
	m_innerSplitter->SetMinimumPaneSize( 120 );

	m_middlePanel = new wxPanel( m_innerSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxBoxSizer* bSizerCandidates;
	bSizerCandidates = new wxBoxSizer( wxVERTICAL );

	m_candidatesLabel = new wxStaticText( m_middlePanel, wxID_ANY, _("Potential replacements:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_candidatesLabel->Wrap( -1 );
	bSizerCandidates->Add( m_candidatesLabel, 0, wxLEFT|wxRIGHT|wxTOP, 4 );

	m_candidatesList = new wxListCtrl( m_middlePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL|wxBORDER_THEME );
	bSizerCandidates->Add( m_candidatesList, 1, wxALL|wxEXPAND, 4 );

	wxBoxSizer* bSizerCandidateButtons;
	bSizerCandidateButtons = new wxBoxSizer( wxHORIZONTAL );

	m_addDirButton = new wxButton( m_middlePanel, wxID_ANY, _("Add Search Directory..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerCandidateButtons->Add( m_addDirButton, 1, wxALL, 2 );

	m_openFileButton = new wxButton( m_middlePanel, wxID_ANY, _("Open External File..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerCandidateButtons->Add( m_openFileButton, 1, wxALL, 2 );


	bSizerCandidates->Add( bSizerCandidateButtons, 0, wxEXPAND|wxLEFT|wxRIGHT, 2 );


	m_middlePanel->SetSizer( bSizerCandidates );
	m_middlePanel->Layout();
	bSizerCandidates->Fit( m_middlePanel );
	m_previewPanel = new wxPanel( m_innerSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxBoxSizer* bSizerPreview;
	bSizerPreview = new wxBoxSizer( wxVERTICAL );

	m_previewLabel = new wxStaticText( m_previewPanel, wxID_ANY, _("Preview:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_previewLabel->Wrap( -1 );
	bSizerPreview->Add( m_previewLabel, 0, wxLEFT|wxRIGHT|wxTOP, 4 );


	m_previewPanel->SetSizer( bSizerPreview );
	m_previewPanel->Layout();
	bSizerPreview->Fit( m_previewPanel );
	m_innerSplitter->SplitVertically( m_middlePanel, m_previewPanel, 300 );
	bSizerRight->Add( m_innerSplitter, 1, wxEXPAND, 0 );


	m_rightContainerPanel->SetSizer( bSizerRight );
	m_rightContainerPanel->Layout();
	bSizerRight->Fit( m_rightContainerPanel );
	m_mainSplitter->SplitVertically( m_leftPanel, m_rightContainerPanel, 300 );
	bSizerMain->Add( m_mainSplitter, 1, wxEXPAND|wxALL, 4 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_doNotShowAgain = new wxCheckBox( this, wxID_ANY, _("Do not show this again"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_doNotShowAgain, 0, wxALIGN_CENTER_VERTICAL|wxALL, 6 );


	bSizerBottom->Add( 0, 0, 1, wxEXPAND, 0 );

	m_replaceButton = new wxButton( this, wxID_ANY, _("Replace Models"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_replaceButton, 0, wxALL, 6 );

	m_keepButton = new wxButton( this, wxID_CANCEL, _("Keep Existing"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_keepButton, 0, wxALL, 6 );


	bSizerMain->Add( bSizerBottom, 0, wxEXPAND, 0 );


	this->SetSizer( bSizerMain );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_missingList->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnMissingSelected ), NULL, this );
	m_candidatesList->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnCandidateSelected ), NULL, this );
	m_addDirButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnAddSearchDirectoryClick ), NULL, this );
	m_openFileButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnOpenExternalFileClick ), NULL, this );
	m_replaceButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnReplaceClick ), NULL, this );
	m_keepButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnKeepClick ), NULL, this );
}

DIALOG_MIGRATE_3D_MODELS_BASE::~DIALOG_MIGRATE_3D_MODELS_BASE()
{
	// Disconnect Events
	m_missingList->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnMissingSelected ), NULL, this );
	m_candidatesList->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnCandidateSelected ), NULL, this );
	m_addDirButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnAddSearchDirectoryClick ), NULL, this );
	m_openFileButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnOpenExternalFileClick ), NULL, this );
	m_replaceButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnReplaceClick ), NULL, this );
	m_keepButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::OnKeepClick ), NULL, this );

}

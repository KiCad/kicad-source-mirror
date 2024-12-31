///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_pcm_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PCM_BASE::DIALOG_PCM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 940,550 ), wxDefaultSize );

	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* m_TopSizer;
	m_TopSizer = new wxBoxSizer( wxVERTICAL );

	m_dialogNotebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelRepository = new wxPanel( m_dialogNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	wxArrayString m_choiceRepositoryChoices;
	m_choiceRepository = new wxChoice( m_panelRepository, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRepositoryChoices, 0 );
	m_choiceRepository->SetSelection( 0 );
	bSizer6->Add( m_choiceRepository, 1, wxALIGN_CENTER|wxALL, 5 );

	m_buttonManage = new wxButton( m_panelRepository, wxID_ANY, _("Manage..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_buttonManage, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizer4->Add( bSizer6, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_contentNotebook = new wxNotebook( m_panelRepository, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_contentNotebook->SetMinSize( wxSize( 900,400 ) );


	bSizer4->Add( m_contentNotebook, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 3 );


	m_panelRepository->SetSizer( bSizer4 );
	m_panelRepository->Layout();
	bSizer4->Fit( m_panelRepository );
	m_dialogNotebook->AddPage( m_panelRepository, _("Repository (%d)"), true );
	m_panelInstalledHolder = new wxPanel( m_dialogNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );


	m_panelInstalledHolder->SetSizer( bSizer7 );
	m_panelInstalledHolder->Layout();
	bSizer7->Fit( m_panelInstalledHolder );
	m_dialogNotebook->AddPage( m_panelInstalledHolder, _("Installed (%d)"), false );
	m_panelPending = new wxPanel( m_dialogNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelPending->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_gridPendingActions = new WX_GRID( m_panelPending, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_gridPendingActions->CreateGrid( 0, 4 );
	m_gridPendingActions->EnableEditing( false );
	m_gridPendingActions->EnableGridLines( true );
	m_gridPendingActions->EnableDragGridSize( false );
	m_gridPendingActions->SetMargins( 0, 0 );

	// Columns
	m_gridPendingActions->SetColSize( 0, 100 );
	m_gridPendingActions->SetColSize( 1, 200 );
	m_gridPendingActions->SetColSize( 2, 80 );
	m_gridPendingActions->SetColSize( 3, 200 );
	m_gridPendingActions->EnableDragColMove( false );
	m_gridPendingActions->EnableDragColSize( true );
	m_gridPendingActions->SetColLabelValue( 0, _("Action") );
	m_gridPendingActions->SetColLabelValue( 1, _("Package") );
	m_gridPendingActions->SetColLabelValue( 2, _("Version") );
	m_gridPendingActions->SetColLabelValue( 3, _("Repository") );
	m_gridPendingActions->SetColLabelSize( 22 );
	m_gridPendingActions->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridPendingActions->EnableDragRowSize( false );
	m_gridPendingActions->SetRowLabelSize( 0 );
	m_gridPendingActions->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridPendingActions->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizer8->Add( m_gridPendingActions, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_discardActionButton = new wxBitmapButton( m_panelPending, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_discardActionButton->SetToolTip( _("Discard action") );

	bSizer9->Add( m_discardActionButton, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer9->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer8->Add( bSizer9, 0, wxEXPAND, 0 );


	m_panelPending->SetSizer( bSizer8 );
	m_panelPending->Layout();
	bSizer8->Fit( m_panelPending );
	m_dialogNotebook->AddPage( m_panelPending, _("Pending (%d)"), false );

	m_TopSizer->Add( m_dialogNotebook, 1, wxEXPAND|wxALL, 5 );


	m_MainSizer->Add( m_TopSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* m_BottomSizer;
	m_BottomSizer = new wxBoxSizer( wxHORIZONTAL );

	m_refreshButton = new wxButton( this, wxID_ANY, _("Refresh"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BottomSizer->Add( m_refreshButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_installLocalButton = new wxButton( this, wxID_ANY, _("Install from File..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_BottomSizer->Add( m_installLocalButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_openPackageDirButton = new wxButton( this, wxID_ANY, _("Open Package Directory"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BottomSizer->Add( m_openPackageDirButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_BottomSizer->Add( 0, 0, 1, 0, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Apply = new wxButton( this, wxID_APPLY );
	m_sdbSizer1->AddButton( m_sdbSizer1Apply );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_BottomSizer->Add( m_sdbSizer1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	m_MainSizer->Add( m_BottomSizer, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_choiceRepository->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PCM_BASE::OnRepositoryChoice ), NULL, this );
	m_buttonManage->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnManageRepositoriesClicked ), NULL, this );
	m_gridPendingActions->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_PCM_BASE::OnPendingActionsCellClicked ), NULL, this );
	m_discardActionButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnDiscardActionClicked ), NULL, this );
	m_refreshButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnRefreshClicked ), NULL, this );
	m_installLocalButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnInstallFromFileClicked ), NULL, this );
	m_openPackageDirButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnOpenPackageDirClicked ), NULL, this );
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnApplyChangesClicked ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnDiscardChangesClicked ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnCloseClicked ), NULL, this );
}

DIALOG_PCM_BASE::~DIALOG_PCM_BASE()
{
	// Disconnect Events
	m_choiceRepository->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PCM_BASE::OnRepositoryChoice ), NULL, this );
	m_buttonManage->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnManageRepositoriesClicked ), NULL, this );
	m_gridPendingActions->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_PCM_BASE::OnPendingActionsCellClicked ), NULL, this );
	m_discardActionButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnDiscardActionClicked ), NULL, this );
	m_refreshButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnRefreshClicked ), NULL, this );
	m_installLocalButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnInstallFromFileClicked ), NULL, this );
	m_openPackageDirButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnOpenPackageDirClicked ), NULL, this );
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnApplyChangesClicked ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnDiscardChangesClicked ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_BASE::OnCloseClicked ), NULL, this );

}

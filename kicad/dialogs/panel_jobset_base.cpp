///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_jobset_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_JOBSET_BASE::PANEL_JOBSET_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : PANEL_NOTEBOOK_BASE( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbJobs;
	sbJobs = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Jobs") ), wxVERTICAL );

	m_jobsGrid = new WX_GRID( sbJobs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_jobsGrid->CreateGrid( 5, 3 );
	m_jobsGrid->EnableEditing( true );
	m_jobsGrid->EnableGridLines( true );
	m_jobsGrid->EnableDragGridSize( false );
	m_jobsGrid->SetMargins( 0, 0 );

	// Columns
	m_jobsGrid->SetColSize( 0, 30 );
	m_jobsGrid->SetColSize( 1, 30 );
	m_jobsGrid->SetColSize( 2, 260 );
	m_jobsGrid->EnableDragColMove( false );
	m_jobsGrid->EnableDragColSize( false );
	m_jobsGrid->SetColLabelSize( 0 );
	m_jobsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_jobsGrid->EnableDragRowSize( true );
	m_jobsGrid->SetRowLabelSize( 0 );
	m_jobsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_jobsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	sbJobs->Add( m_jobsGrid, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 3 );

	wxBoxSizer* bJobsButtons;
	bJobsButtons = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAddJob = new STD_BITMAP_BUTTON( sbJobs->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bJobsButtons->Add( m_buttonAddJob, 0, wxALIGN_CENTER|wxBOTTOM|wxTOP, 5 );

	m_buttonUp = new STD_BITMAP_BUTTON( sbJobs->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bJobsButtons->Add( m_buttonUp, 0, wxALIGN_CENTER|wxBOTTOM|wxLEFT|wxTOP, 5 );

	m_buttonDown = new STD_BITMAP_BUTTON( sbJobs->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bJobsButtons->Add( m_buttonDown, 0, wxALIGN_CENTER|wxBOTTOM|wxLEFT|wxTOP, 5 );


	bJobsButtons->Add( 20, 0, 0, wxEXPAND, 5 );

	m_buttonDelete = new STD_BITMAP_BUTTON( sbJobs->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bJobsButtons->Add( m_buttonDelete, 0, wxALL, 5 );


	bJobsButtons->Add( 0, 0, 1, wxEXPAND, 5 );


	sbJobs->Add( bJobsButtons, 0, wxEXPAND|wxLEFT|wxRIGHT, 3 );


	bSizerUpper->Add( sbJobs, 3, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbDestinations;
	sbDestinations = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Destinations") ), wxVERTICAL );

	m_destinationList = new wxScrolledWindow( sbDestinations->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_destinationList->SetScrollRate( 5, 5 );
	m_destinationListSizer = new wxBoxSizer( wxVERTICAL );


	m_destinationList->SetSizer( m_destinationListSizer );
	m_destinationList->Layout();
	m_destinationListSizer->Fit( m_destinationList );
	sbDestinations->Add( m_destinationList, 1, wxEXPAND, 0 );

	wxBoxSizer* bOutputButtons;
	bOutputButtons = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAddDestination = new STD_BITMAP_BUTTON( sbDestinations->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bOutputButtons->Add( m_buttonAddDestination, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxTOP, 5 );


	sbDestinations->Add( bOutputButtons, 0, wxEXPAND|wxLEFT, 3 );


	bSizerUpper->Add( sbDestinations, 2, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );


	bSizerButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonSave = new wxButton( this, wxID_ANY, _("Save Jobset"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonSave, 0, wxALL, 5 );


	bSizerButtons->Add( 10, 0, 0, wxEXPAND, 5 );

	m_buttonGenerateAllDestinations = new wxButton( this, wxID_ANY, _("Generate All Destinations"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonGenerateAllDestinations, 0, wxALL, 5 );


	bSizerMain->Add( bSizerButtons, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	m_jobsGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( PANEL_JOBSET_BASE::OnGridCellChange ), NULL, this );
	m_jobsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_JOBSET_BASE::OnSizeGrid ), NULL, this );
	m_buttonAddJob->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnAddJobClick ), NULL, this );
	m_buttonUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnJobButtonUp ), NULL, this );
	m_buttonDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnJobButtonDown ), NULL, this );
	m_buttonDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnJobButtonDelete ), NULL, this );
	m_buttonAddDestination->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnAddDestinationClick ), NULL, this );
	m_buttonSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnSaveButtonClick ), NULL, this );
	m_buttonGenerateAllDestinations->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnGenerateAllDestinationsClick ), NULL, this );
}

PANEL_JOBSET_BASE::~PANEL_JOBSET_BASE()
{
	// Disconnect Events
	m_jobsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( PANEL_JOBSET_BASE::OnGridCellChange ), NULL, this );
	m_jobsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_JOBSET_BASE::OnSizeGrid ), NULL, this );
	m_buttonAddJob->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnAddJobClick ), NULL, this );
	m_buttonUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnJobButtonUp ), NULL, this );
	m_buttonDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnJobButtonDown ), NULL, this );
	m_buttonDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnJobButtonDelete ), NULL, this );
	m_buttonAddDestination->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnAddDestinationClick ), NULL, this );
	m_buttonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnSaveButtonClick ), NULL, this );
	m_buttonGenerateAllDestinations->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBSET_BASE::OnGenerateAllDestinationsClick ), NULL, this );

}

PANEL_DESTINATION_BASE::PANEL_DESTINATION_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 1, 3, 5, 5 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_bitmapOutputType = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_bitmapOutputType, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_textOutputType = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputType->Wrap( -1 );
	m_textOutputType->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	fgSizer3->Add( m_textOutputType, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 1 );

	m_statusBitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_statusBitmap, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( fgSizer3, 0, wxEXPAND|wxTOP, 6 );


	bSizerMain->Add( 0, 2, 0, 0, 5 );

	m_pathInfo = new wxStaticText( this, wxID_ANY, _("path"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pathInfo->Wrap( -1 );
	bSizerMain->Add( m_pathInfo, 0, wxRIGHT|wxLEFT, 6 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_buttonProperties = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerButtons->Add( m_buttonProperties, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_buttonDelete = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerButtons->Add( m_buttonDelete, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerButtons->Add( 10, 0, 1, wxEXPAND, 5 );

	m_buttonGenerate = new wxButton( this, wxID_ANY, _("Generate"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonGenerate, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_buttonOpenOutput = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerButtons->Add( m_buttonOpenOutput, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( bSizerButtons, 0, wxEXPAND|wxTOP, 10 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	this->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( PANEL_DESTINATION_BASE::OnRightDown ) );
	m_statusBitmap->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( PANEL_DESTINATION_BASE::OnLastStatusClick ), NULL, this );
	m_buttonProperties->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESTINATION_BASE::OnProperties ), NULL, this );
	m_buttonDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESTINATION_BASE::OnDelete ), NULL, this );
	m_buttonGenerate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESTINATION_BASE::OnGenerate ), NULL, this );
	m_buttonOpenOutput->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESTINATION_BASE::OnOpenOutput ), NULL, this );
}

PANEL_DESTINATION_BASE::~PANEL_DESTINATION_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( PANEL_DESTINATION_BASE::OnRightDown ) );
	m_statusBitmap->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( PANEL_DESTINATION_BASE::OnLastStatusClick ), NULL, this );
	m_buttonProperties->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESTINATION_BASE::OnProperties ), NULL, this );
	m_buttonDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESTINATION_BASE::OnDelete ), NULL, this );
	m_buttonGenerate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESTINATION_BASE::OnGenerate ), NULL, this );
	m_buttonOpenOutput->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESTINATION_BASE::OnOpenOutput ), NULL, this );

}

DIALOG_JOBSET_RUN_LOG_BASE::DIALOG_JOBSET_RUN_LOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 450,250 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_staticTextOutputName = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOutputName->Wrap( -1 );
	bMainSizer->Add( m_staticTextOutputName, 0, wxALL, 5 );

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );

	m_jobList = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer16->Add( m_jobList, 1, wxALL|wxEXPAND, 5 );

	m_textCtrlOutput = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_BESTWRAP|wxTE_MULTILINE|wxTE_READONLY );
	m_textCtrlOutput->SetMinSize( wxSize( 420,-1 ) );

	bSizer16->Add( m_textCtrlOutput, 1, wxALL|wxEXPAND, 5 );


	bMainSizer->Add( bSizer16, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_JOBSET_RUN_LOG_BASE::OnUpdateUI ) );
	m_jobList->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_JOBSET_RUN_LOG_BASE::OnJobListItemSelected ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_JOBSET_RUN_LOG_BASE::OnButtonOk ), NULL, this );
}

DIALOG_JOBSET_RUN_LOG_BASE::~DIALOG_JOBSET_RUN_LOG_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_JOBSET_RUN_LOG_BASE::OnUpdateUI ) );
	m_jobList->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_JOBSET_RUN_LOG_BASE::OnJobListItemSelected ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_JOBSET_RUN_LOG_BASE::OnButtonOk ), NULL, this );

}

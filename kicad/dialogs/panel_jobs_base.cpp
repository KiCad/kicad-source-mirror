///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_jobs_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_JOBS_BASE::PANEL_JOBS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : PANEL_NOTEBOOK_BASE( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbJobs;
	sbJobs = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Jobs") ), wxVERTICAL );

	m_jobsGrid = new WX_GRID( sbJobs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_jobsGrid->CreateGrid( 5, 2 );
	m_jobsGrid->EnableEditing( true );
	m_jobsGrid->EnableGridLines( true );
	m_jobsGrid->EnableDragGridSize( false );
	m_jobsGrid->SetMargins( 0, 0 );

	// Columns
	m_jobsGrid->SetColSize( 0, 40 );
	m_jobsGrid->SetColSize( 1, 260 );
	m_jobsGrid->EnableDragColMove( false );
	m_jobsGrid->EnableDragColSize( true );
	m_jobsGrid->SetColLabelSize( 0 );
	m_jobsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_jobsGrid->EnableDragRowSize( true );
	m_jobsGrid->SetRowLabelSize( 0 );
	m_jobsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_jobsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbJobs->Add( m_jobsGrid, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );

	wxBoxSizer* bJobsButtons;
	bJobsButtons = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAddJob = new STD_BITMAP_BUTTON( sbJobs->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bJobsButtons->Add( m_buttonAddJob, 0, wxALIGN_CENTER|wxTOP|wxBOTTOM, 5 );

	m_buttonUp = new STD_BITMAP_BUTTON( sbJobs->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bJobsButtons->Add( m_buttonUp, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_buttonDown = new STD_BITMAP_BUTTON( sbJobs->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bJobsButtons->Add( m_buttonDown, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );


	bJobsButtons->Add( 20, 0, 0, wxEXPAND, 5 );

	m_buttonDelete = new wxBitmapButton( sbJobs->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bJobsButtons->Add( m_buttonDelete, 0, wxALL, 5 );


	bJobsButtons->Add( 0, 0, 1, wxEXPAND, 5 );


	sbJobs->Add( bJobsButtons, 0, wxEXPAND|wxRIGHT|wxLEFT, 3 );


	bSizerUpper->Add( sbJobs, 2, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbOutputs;
	sbOutputs = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Outputs") ), wxVERTICAL );

	m_outputList = new wxScrolledWindow( sbOutputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_outputList->SetScrollRate( 5, 5 );
	m_outputListSizer = new wxBoxSizer( wxVERTICAL );


	m_outputList->SetSizer( m_outputListSizer );
	m_outputList->Layout();
	m_outputListSizer->Fit( m_outputList );
	sbOutputs->Add( m_outputList, 1, wxEXPAND|wxALL, 0 );

	wxBoxSizer* bOutputButtons;
	bOutputButtons = new wxBoxSizer( wxHORIZONTAL );

	m_buttonOutputAdd = new STD_BITMAP_BUTTON( sbOutputs->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bOutputButtons->Add( m_buttonOutputAdd, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );


	sbOutputs->Add( bOutputButtons, 0, wxEXPAND, 5 );


	bSizerUpper->Add( sbOutputs, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );


	bSizerButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonSave = new wxButton( this, wxID_ANY, _("Save Jobset"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonSave, 0, wxALL, 5 );


	bSizerButtons->Add( 20, 0, 0, wxEXPAND, 5 );

	m_buttonRunAllOutputs = new wxButton( this, wxID_ANY, _("Generate All Outputs"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonRunAllOutputs, 0, wxALL, 5 );


	bSizerMain->Add( bSizerButtons, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	// Connect Events
	m_jobsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_JOBS_BASE::OnSizeGrid ), NULL, this );
	m_buttonAddJob->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnAddJobClick ), NULL, this );
	m_buttonUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnJobButtonUp ), NULL, this );
	m_buttonDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnJobButtonDown ), NULL, this );
	m_buttonDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnJobButtonDelete ), NULL, this );
	m_buttonOutputAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnAddOutputClick ), NULL, this );
	m_buttonSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnSaveButtonClick ), NULL, this );
	m_buttonRunAllOutputs->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnRunAllJobsClick ), NULL, this );
}

PANEL_JOBS_BASE::~PANEL_JOBS_BASE()
{
	// Disconnect Events
	m_jobsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_JOBS_BASE::OnSizeGrid ), NULL, this );
	m_buttonAddJob->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnAddJobClick ), NULL, this );
	m_buttonUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnJobButtonUp ), NULL, this );
	m_buttonDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnJobButtonDown ), NULL, this );
	m_buttonDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnJobButtonDelete ), NULL, this );
	m_buttonOutputAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnAddOutputClick ), NULL, this );
	m_buttonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnSaveButtonClick ), NULL, this );
	m_buttonRunAllOutputs->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnRunAllJobsClick ), NULL, this );

}

PANEL_JOB_OUTPUT_BASE::PANEL_JOB_OUTPUT_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );

	m_bitmapOutputType = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMain->Add( m_bitmapOutputType, 0, wxALL, 5 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );

	m_textOutputType = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputType->Wrap( -1 );
	m_textOutputType->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer17->Add( m_textOutputType, 0, wxTOP, 8 );


	bSizer14->Add( bSizer17, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_statusBitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer14->Add( m_statusBitmap, 0, wxALL, 5 );


	bSizer12->Add( bSizer14, 0, wxEXPAND, 5 );


	bSizerMain->Add( bSizer12, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	m_buttonOutputRun = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer13->Add( m_buttonOutputRun, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_buttonOutputOptions = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer13->Add( m_buttonOutputOptions, 0, wxALL, 5 );


	bSizerMain->Add( bSizer13, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	m_statusBitmap->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( PANEL_JOB_OUTPUT_BASE::OnLastStatusClick ), NULL, this );
	m_buttonOutputRun->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOB_OUTPUT_BASE::OnOutputRunClick ), NULL, this );
	m_buttonOutputOptions->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOB_OUTPUT_BASE::OnOutputOptionsClick ), NULL, this );
}

PANEL_JOB_OUTPUT_BASE::~PANEL_JOB_OUTPUT_BASE()
{
	// Disconnect Events
	m_statusBitmap->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( PANEL_JOB_OUTPUT_BASE::OnLastStatusClick ), NULL, this );
	m_buttonOutputRun->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOB_OUTPUT_BASE::OnOutputRunClick ), NULL, this );
	m_buttonOutputOptions->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOB_OUTPUT_BASE::OnOutputOptionsClick ), NULL, this );

}

DIALOG_JOB_OUTPUT_BASE::DIALOG_JOB_OUTPUT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	m_staticText9->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerMain->Add( m_staticText9, 0, wxALL, 5 );

	m_panel9 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textArchiveDesc = new wxStaticText( m_panel9, wxID_ANY, _("Description"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textArchiveDesc->Wrap( -1 );
	fgSizer1->Add( m_textArchiveDesc, 0, wxALL, 5 );

	m_textCtrlDescription = new wxTextCtrl( m_panel9, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textCtrlDescription, 0, wxALL|wxEXPAND, 5 );

	m_textArchiveFormat = new wxStaticText( m_panel9, wxID_ANY, _("Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textArchiveFormat->Wrap( -1 );
	fgSizer1->Add( m_textArchiveFormat, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_choiceArchiveformatChoices;
	m_choiceArchiveformat = new wxChoice( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceArchiveformatChoices, 0 );
	m_choiceArchiveformat->SetSelection( 0 );
	fgSizer1->Add( m_choiceArchiveformat, 0, wxALL, 5 );

	m_textOutputPath = new wxStaticText( m_panel9, wxID_ANY, _("Output Path"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizer1->Add( m_textOutputPath, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );

	m_textCtrlOutputPath = new wxTextCtrl( m_panel9, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputPath->SetMinSize( wxSize( 350,-1 ) );

	bSizer16->Add( m_textCtrlOutputPath, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_buttonOutputPath = new STD_BITMAP_BUTTON( m_panel9, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer16->Add( m_buttonOutputPath, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	fgSizer1->Add( bSizer16, 1, wxEXPAND, 5 );

	m_staticText10 = new wxStaticText( m_panel9, wxID_ANY, _("Only Jobs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizer1->Add( m_staticText10, 0, wxALL, 5 );

	m_listBoxOnly = new wxListBox( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_ALWAYS_SB|wxLB_MULTIPLE );
	m_listBoxOnly->SetMinSize( wxSize( 300,200 ) );

	fgSizer1->Add( m_listBoxOnly, 0, wxALL, 5 );


	m_panel9->SetSizer( fgSizer1 );
	m_panel9->Layout();
	fgSizer1->Fit( m_panel9 );
	bSizerMain->Add( m_panel9, 1, wxEXPAND | wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1Save = new wxButton( this, wxID_SAVE );
	m_sdbSizer1->AddButton( m_sdbSizer1Save );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_buttonOutputPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_JOB_OUTPUT_BASE::onOutputPathBrowseClicked ), NULL, this );
}

DIALOG_JOB_OUTPUT_BASE::~DIALOG_JOB_OUTPUT_BASE()
{
	// Disconnect Events
	m_buttonOutputPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_JOB_OUTPUT_BASE::onOutputPathBrowseClicked ), NULL, this );

}

DIALOG_SPECIAL_EXECUTE_BASE::DIALOG_SPECIAL_EXECUTE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	m_staticText9->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerMain->Add( m_staticText9, 0, wxALL, 5 );

	m_panel9 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textCommand = new wxStaticText( m_panel9, wxID_ANY, _("Command"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textCommand->Wrap( -1 );
	fgSizer1->Add( m_textCommand, 0, wxALL, 5 );

	m_textCtrlCommand = new wxTextCtrl( m_panel9, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlCommand->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlCommand, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbRecordOutput = new wxCheckBox( m_panel9, wxID_ANY, _("Record Output"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_cbRecordOutput, 0, wxALL, 5 );

	m_textOutputPath = new wxStaticText( m_panel9, wxID_ANY, _("Output Path"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizer1->Add( m_textOutputPath, 0, wxALL, 5 );

	m_textCtrlOutputPath = new wxTextCtrl( m_panel9, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputPath->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlOutputPath, 0, wxALL, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbIgnoreExitCode = new wxCheckBox( m_panel9, wxID_ANY, _("Ignore non-zero exit code"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_cbIgnoreExitCode, 1, wxALL, 5 );


	m_panel9->SetSizer( fgSizer1 );
	m_panel9->Layout();
	fgSizer1->Fit( m_panel9 );
	bSizerMain->Add( m_panel9, 1, wxEXPAND | wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1Save = new wxButton( this, wxID_SAVE );
	m_sdbSizer1->AddButton( m_sdbSizer1Save );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_cbRecordOutput->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SPECIAL_EXECUTE_BASE::OnRecordOutputClicked ), NULL, this );
}

DIALOG_SPECIAL_EXECUTE_BASE::~DIALOG_SPECIAL_EXECUTE_BASE()
{
	// Disconnect Events
	m_cbRecordOutput->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SPECIAL_EXECUTE_BASE::OnRecordOutputClicked ), NULL, this );

}

DIALOG_OUTPUT_RUN_RESULTS_BASE::DIALOG_OUTPUT_RUN_RESULTS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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
	bSizer16->Add( m_textCtrlOutput, 1, wxALL|wxEXPAND, 5 );


	bMainSizer->Add( bSizer16, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_jobList->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_OUTPUT_RUN_RESULTS_BASE::OnJobListItemSelected ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OUTPUT_RUN_RESULTS_BASE::OnButtonOk ), NULL, this );
}

DIALOG_OUTPUT_RUN_RESULTS_BASE::~DIALOG_OUTPUT_RUN_RESULTS_BASE()
{
	// Disconnect Events
	m_jobList->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_OUTPUT_RUN_RESULTS_BASE::OnJobListItemSelected ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OUTPUT_RUN_RESULTS_BASE::OnButtonOk ), NULL, this );

}

DIALOG_RC_JOB_BASE::DIALOG_RC_JOB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	m_staticText9->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerMain->Add( m_staticText9, 0, wxALL, 5 );

	m_panel9 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textOutputPath = new wxStaticText( m_panel9, wxID_ANY, _("Output File"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizer1->Add( m_textOutputPath, 0, wxALL, 5 );

	m_textCtrlOutputPath = new wxTextCtrl( m_panel9, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputPath->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlOutputPath, 0, wxALL, 5 );

	m_staticText18 = new wxStaticText( m_panel9, wxID_ANY, _("Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer1->Add( m_staticText18, 0, wxALL, 5 );

	wxArrayString m_choiceFormatChoices;
	m_choiceFormat = new wxChoice( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFormatChoices, 0 );
	m_choiceFormat->SetSelection( 0 );
	fgSizer1->Add( m_choiceFormat, 0, wxALL, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbHaltOutput = new wxCheckBox( m_panel9, wxID_ANY, _("Halt output generation on error"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_cbHaltOutput, 1, wxALL, 5 );


	m_panel9->SetSizer( fgSizer1 );
	m_panel9->Layout();
	fgSizer1->Fit( m_panel9 );
	bSizerMain->Add( m_panel9, 1, wxEXPAND | wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1Save = new wxButton( this, wxID_SAVE );
	m_sdbSizer1->AddButton( m_sdbSizer1Save );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_RC_JOB_BASE::~DIALOG_RC_JOB_BASE()
{
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "panel_jobs_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_JOBS_BASE::PANEL_JOBS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : PANEL_NOTEBOOK_BASE( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Jobs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer3->Add( m_staticText1, 0, wxALL, 5 );

	m_jobList = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer3->Add( m_jobList, 1, wxALL|wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAddJob = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer2->Add( m_buttonAddJob, 0, wxALIGN_CENTER|wxALL, 5 );

	m_buttonUp = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer2->Add( m_buttonUp, 0, wxALL, 5 );

	m_buttonDown = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer2->Add( m_buttonDown, 0, wxALL, 5 );


	bSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonSave = new wxButton( this, wxID_ANY, _("Save"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_buttonSave, 0, wxALIGN_CENTER|wxALL, 5 );


	bSizer3->Add( bSizer2, 0, wxEXPAND, 5 );


	bSizerMain->Add( bSizer3, 2, wxEXPAND, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText4 = new wxStaticText( this, wxID_ANY, _("Outputs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	bSizer14->Add( m_staticText4, 0, wxALL, 5 );

	m_buttonOutputAdd = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer14->Add( m_buttonOutputAdd, 0, 0, 5 );


	bSizer4->Add( bSizer14, 0, wxEXPAND, 5 );

	m_outputList = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_outputList->SetScrollRate( 5, 5 );
	m_outputListSizer = new wxBoxSizer( wxVERTICAL );


	m_outputList->SetSizer( m_outputListSizer );
	m_outputList->Layout();
	m_outputListSizer->Fit( m_outputList );
	bSizer4->Add( m_outputList, 1, wxEXPAND | wxALL, 0 );

	m_buttonRunAllOutputs = new wxButton( this, wxID_ANY, _("Run All Jobs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRunAllOutputs->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	bSizer4->Add( m_buttonRunAllOutputs, 0, wxALIGN_RIGHT|wxALL, 5 );


	bSizerMain->Add( bSizer4, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	// Connect Events
	m_jobList->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( PANEL_JOBS_BASE::OnJobListDoubleClicked ), NULL, this );
	m_jobList->Connect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( PANEL_JOBS_BASE::OnJobListItemRightClick ), NULL, this );
	m_buttonAddJob->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnAddJobClick ), NULL, this );
	m_buttonUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnJobButtonUp ), NULL, this );
	m_buttonDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnJobButtonDown ), NULL, this );
	m_buttonSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnSaveButtonClick ), NULL, this );
	m_buttonOutputAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnAddOutputClick ), NULL, this );
	m_buttonRunAllOutputs->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnRunAllJobsClick ), NULL, this );
}

PANEL_JOBS_BASE::~PANEL_JOBS_BASE()
{
	// Disconnect Events
	m_jobList->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( PANEL_JOBS_BASE::OnJobListDoubleClicked ), NULL, this );
	m_jobList->Disconnect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( PANEL_JOBS_BASE::OnJobListItemRightClick ), NULL, this );
	m_buttonAddJob->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnAddJobClick ), NULL, this );
	m_buttonUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnJobButtonUp ), NULL, this );
	m_buttonDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnJobButtonDown ), NULL, this );
	m_buttonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnSaveButtonClick ), NULL, this );
	m_buttonOutputAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnAddOutputClick ), NULL, this );
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

	m_textOutputType = new wxStaticText( this, wxID_ANY, _("Placeholder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputType->Wrap( -1 );
	m_textOutputType->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer14->Add( m_textOutputType, 0, wxALL, 5 );

	m_statusBitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer14->Add( m_statusBitmap, 0, wxALL, 5 );


	bSizer12->Add( bSizer14, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizer12, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	m_buttonOutputRun = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer13->Add( m_buttonOutputRun, 0, wxALL, 5 );

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
	this->SetSizeHints( wxSize( -1,369 ), wxDefaultSize );

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

	m_textArchiveFormat = new wxStaticText( m_panel9, wxID_ANY, _("Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textArchiveFormat->Wrap( -1 );
	fgSizer1->Add( m_textArchiveFormat, 0, wxALL, 5 );

	wxArrayString m_choiceArchiveformatChoices;
	m_choiceArchiveformat = new wxChoice( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceArchiveformatChoices, 0 );
	m_choiceArchiveformat->SetSelection( 0 );
	fgSizer1->Add( m_choiceArchiveformat, 0, wxALL, 5 );

	m_textOutputPath = new wxStaticText( m_panel9, wxID_ANY, _("Output Path"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizer1->Add( m_textOutputPath, 0, wxALL, 5 );

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

	m_staticText12 = new wxStaticText( m_panel9, wxID_ANY, _("Overwrite Prompt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer1->Add( m_staticText12, 0, wxALL, 5 );

	m_outputOverwrite = new wxCheckBox( m_panel9, wxID_ANY, _("Prompt about output overwrite?"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_outputOverwrite, 0, wxALL, 5 );


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

	m_staticTextOutputName = new wxStaticText( this, wxID_ANY, _("Placeholder"), wxDefaultPosition, wxDefaultSize, 0 );
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

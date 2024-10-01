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
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, wxT("Jobs"), wxDefaultPosition, wxDefaultSize, 0 );
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

	m_buttonSave = new wxButton( this, wxID_ANY, wxT("Save"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_buttonSave, 0, wxALIGN_CENTER|wxALL, 5 );


	bSizer3->Add( bSizer2, 0, wxEXPAND, 5 );


	bSizer1->Add( bSizer3, 2, wxEXPAND, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText4 = new wxStaticText( this, wxID_ANY, wxT("Outputs"), wxDefaultPosition, wxDefaultSize, 0 );
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

	m_buttonRunAllOutputs = new wxButton( this, wxID_ANY, wxT("Run All Jobs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRunAllOutputs->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	bSizer4->Add( m_buttonRunAllOutputs, 0, wxALIGN_RIGHT|wxALL, 5 );


	bSizer1->Add( bSizer4, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();

	// Connect Events
	m_jobList->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( PANEL_JOBS_BASE::OnJobListDoubleClicked ), NULL, this );
	m_jobList->Connect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( PANEL_JOBS_BASE::OnJobListItemRightClick ), NULL, this );
	m_buttonAddJob->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnAddJobClick ), NULL, this );
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
	m_buttonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnSaveButtonClick ), NULL, this );
	m_buttonOutputAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnAddOutputClick ), NULL, this );
	m_buttonRunAllOutputs->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOBS_BASE::OnRunAllJobsClick ), NULL, this );

}

PANEL_JOB_OUTPUT_BASE::PANEL_JOB_OUTPUT_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );

	m_bitmapOutputType = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer11->Add( m_bitmapOutputType, 0, wxALL, 5 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );

	m_textOutputType = new wxStaticText( this, wxID_ANY, wxT("Placeholder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputType->Wrap( -1 );
	m_textOutputType->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer12->Add( m_textOutputType, 0, wxALL, 5 );


	bSizer11->Add( bSizer12, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	m_buttonOutputRun = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer13->Add( m_buttonOutputRun, 0, wxALL, 5 );

	m_buttonOutputOptions = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer13->Add( m_buttonOutputOptions, 0, wxALL, 5 );


	bSizer11->Add( bSizer13, 0, wxEXPAND, 5 );


	this->SetSizer( bSizer11 );
	this->Layout();
	bSizer11->Fit( this );

	// Connect Events
	m_buttonOutputRun->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOB_OUTPUT_BASE::OnOutputRunClick ), NULL, this );
	m_buttonOutputOptions->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOB_OUTPUT_BASE::OnOutputOptionsClick ), NULL, this );
}

PANEL_JOB_OUTPUT_BASE::~PANEL_JOB_OUTPUT_BASE()
{
	// Disconnect Events
	m_buttonOutputRun->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOB_OUTPUT_BASE::OnOutputRunClick ), NULL, this );
	m_buttonOutputOptions->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_JOB_OUTPUT_BASE::OnOutputOptionsClick ), NULL, this );

}

DIALOG_JOB_OUTPUT_BASE::DIALOG_JOB_OUTPUT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );

	m_staticText9 = new wxStaticText( this, wxID_ANY, wxT("Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	m_staticText9->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer15->Add( m_staticText9, 0, wxALL, 5 );

	m_panel9 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textArchiveFormat = new wxStaticText( m_panel9, wxID_ANY, wxT("Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textArchiveFormat->Wrap( -1 );
	fgSizer1->Add( m_textArchiveFormat, 0, wxALL, 5 );

	wxArrayString m_choiceArchiveformatChoices;
	m_choiceArchiveformat = new wxChoice( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceArchiveformatChoices, 0 );
	m_choiceArchiveformat->SetSelection( 0 );
	fgSizer1->Add( m_choiceArchiveformat, 0, wxALL, 5 );

	m_textOutputPath = new wxStaticText( m_panel9, wxID_ANY, wxT("Output Path"), wxDefaultPosition, wxDefaultSize, 0 );
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

	m_staticText10 = new wxStaticText( m_panel9, wxID_ANY, wxT("Only Jobs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizer1->Add( m_staticText10, 0, wxALL, 5 );

	m_listBoxOnly = new wxListBox( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_ALWAYS_SB|wxLB_MULTIPLE );
	m_listBoxOnly->SetMinSize( wxSize( 300,200 ) );

	fgSizer1->Add( m_listBoxOnly, 0, wxALL, 5 );


	m_panel9->SetSizer( fgSizer1 );
	m_panel9->Layout();
	fgSizer1->Fit( m_panel9 );
	bSizer15->Add( m_panel9, 1, wxEXPAND | wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1Save = new wxButton( this, wxID_SAVE );
	m_sdbSizer1->AddButton( m_sdbSizer1Save );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizer15->Add( m_sdbSizer1, 0, wxEXPAND, 5 );


	this->SetSizer( bSizer15 );
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

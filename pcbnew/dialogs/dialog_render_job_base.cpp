///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_render_job_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_RENDER_JOB_BASE::DIALOG_RENDER_JOB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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
	fgSizer1->Add( m_textOutputPath, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textCtrlOutputFile = new wxTextCtrl( m_panel9, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputFile->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlOutputFile, 0, wxALL, 5 );

	m_staticText16 = new wxStaticText( m_panel9, wxID_ANY, _("Dimensions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizer1->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_spinCtrlWidth = new wxSpinCtrl( m_panel9, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 128, 32767, 0 );
	bSizer3->Add( m_spinCtrlWidth, 0, wxALL, 5 );

	m_staticText17 = new wxStaticText( m_panel9, wxID_ANY, _("px"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	bSizer3->Add( m_staticText17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticText19 = new wxStaticText( m_panel9, wxID_ANY, _("x"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	bSizer3->Add( m_staticText19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_spinCtrlHeight = new wxSpinCtrl( m_panel9, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 128, 32767, 0 );
	bSizer3->Add( m_spinCtrlHeight, 0, wxALL, 5 );

	m_staticText182 = new wxStaticText( m_panel9, wxID_ANY, _("px"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText182->Wrap( -1 );
	bSizer3->Add( m_staticText182, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	fgSizer1->Add( bSizer3, 1, wxEXPAND, 5 );

	m_staticText18 = new wxStaticText( m_panel9, wxID_ANY, _("Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer1->Add( m_staticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_choiceFormatChoices;
	m_choiceFormat = new wxChoice( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFormatChoices, 0 );
	m_choiceFormat->SetSelection( 0 );
	fgSizer1->Add( m_choiceFormat, 0, wxALL, 5 );

	m_staticText181 = new wxStaticText( m_panel9, wxID_ANY, _("Quality"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText181->Wrap( -1 );
	fgSizer1->Add( m_staticText181, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_choiceQualityChoices;
	m_choiceQuality = new wxChoice( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceQualityChoices, 0 );
	m_choiceQuality->SetSelection( 0 );
	fgSizer1->Add( m_choiceQuality, 0, wxALL, 5 );

	m_staticText1811 = new wxStaticText( m_panel9, wxID_ANY, _("Background Style"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1811->Wrap( -1 );
	fgSizer1->Add( m_staticText1811, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_choiceBgStyleChoices;
	m_choiceBgStyle = new wxChoice( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceBgStyleChoices, 0 );
	m_choiceBgStyle->SetSelection( 0 );
	fgSizer1->Add( m_choiceBgStyle, 0, wxALL, 5 );

	m_staticText18111 = new wxStaticText( m_panel9, wxID_ANY, _("Side"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18111->Wrap( -1 );
	fgSizer1->Add( m_staticText18111, 0, wxALL, 5 );

	wxArrayString m_choiceSideChoices;
	m_choiceSide = new wxChoice( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceSideChoices, 0 );
	m_choiceSide->SetSelection( 0 );
	fgSizer1->Add( m_choiceSide, 0, wxALL, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbFloor = new wxCheckBox( m_panel9, wxID_ANY, _("Show floor"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_cbFloor, 1, wxALL, 5 );

	m_staticText11 = new wxStaticText( m_panel9, wxID_ANY, _("Perspective"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer1->Add( m_staticText11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_radioProjectionChoices[] = { _("Perspective"), _("Orthogonal") };
	int m_radioProjectionNChoices = sizeof( m_radioProjectionChoices ) / sizeof( wxString );
	m_radioProjection = new wxRadioBox( m_panel9, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, m_radioProjectionNChoices, m_radioProjectionChoices, 1, wxRA_SPECIFY_COLS );
	m_radioProjection->SetSelection( 1 );
	fgSizer1->Add( m_radioProjection, 0, wxALL, 5 );

	m_staticText15 = new wxStaticText( m_panel9, wxID_ANY, _("Zoom"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizer1->Add( m_staticText15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_spinCtrlZoom = new wxSpinCtrlDouble( m_panel9, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 1, 0.1 );
	m_spinCtrlZoom->SetDigits( 2 );
	fgSizer1->Add( m_spinCtrlZoom, 0, wxALL, 5 );


	m_panel9->SetSizer( fgSizer1 );
	m_panel9->Layout();
	fgSizer1->Fit( m_panel9 );
	bSizerMain->Add( m_panel9, 1, wxEXPAND | wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_choiceFormat->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
	m_choiceQuality->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
	m_choiceBgStyle->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
	m_choiceSide->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
}

DIALOG_RENDER_JOB_BASE::~DIALOG_RENDER_JOB_BASE()
{
	// Disconnect Events
	m_choiceFormat->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
	m_choiceQuality->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
	m_choiceBgStyle->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );
	m_choiceSide->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_RENDER_JOB_BASE::OnFormatChoice ), NULL, this );

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 6, 5 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textOutputPath = new wxStaticText( this, wxID_ANY, _("Output file:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizer1->Add( m_textOutputPath, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_textCtrlOutputFile = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputFile->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlOutputFile, 0, 0, 5 );

	m_formatLabel = new wxStaticText( this, wxID_ANY, _("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_formatLabel->Wrap( -1 );
	fgSizer1->Add( m_formatLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxArrayString m_choiceFormatChoices;
	m_choiceFormat = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFormatChoices, 0 );
	m_choiceFormat->SetSelection( 0 );
	fgSizer1->Add( m_choiceFormat, 0, 0, 5 );

	m_dimensionsLabel = new wxStaticText( this, wxID_ANY, _("Dimensions:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dimensionsLabel->Wrap( -1 );
	fgSizer1->Add( m_dimensionsLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_spinCtrlWidth = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 128, 32767, 0 );
	bSizer3->Add( m_spinCtrlWidth, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText17 = new wxStaticText( this, wxID_ANY, _("px"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	bSizer3->Add( m_staticText17, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticText19 = new wxStaticText( this, wxID_ANY, _("x"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	bSizer3->Add( m_staticText19, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_spinCtrlHeight = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 128, 32767, 0 );
	bSizer3->Add( m_spinCtrlHeight, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText182 = new wxStaticText( this, wxID_ANY, _("px"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText182->Wrap( -1 );
	bSizer3->Add( m_staticText182, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer1->Add( bSizer3, 1, wxEXPAND, 5 );

	m_qualityLabel = new wxStaticText( this, wxID_ANY, _("Quality:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_qualityLabel->Wrap( -1 );
	fgSizer1->Add( m_qualityLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxArrayString m_choiceQualityChoices;
	m_choiceQuality = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceQualityChoices, 0 );
	m_choiceQuality->SetSelection( 0 );
	fgSizer1->Add( m_choiceQuality, 0, 0, 5 );

	m_backgroundStyleLabel = new wxStaticText( this, wxID_ANY, _("Background style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundStyleLabel->Wrap( -1 );
	fgSizer1->Add( m_backgroundStyleLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxArrayString m_choiceBgStyleChoices;
	m_choiceBgStyle = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceBgStyleChoices, 0 );
	m_choiceBgStyle->SetSelection( 0 );
	fgSizer1->Add( m_choiceBgStyle, 0, 0, 5 );

	m_staticText15 = new wxStaticText( this, wxID_ANY, _("Zoom:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizer1->Add( m_staticText15, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spinCtrlZoom = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 1, 0.1 );
	m_spinCtrlZoom->SetDigits( 2 );
	fgSizer1->Add( m_spinCtrlZoom, 0, 0, 5 );


	bSizerMain->Add( fgSizer1, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("View") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 5, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_sideLabel = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Side:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sideLabel->Wrap( -1 );
	gbSizer1->Add( m_sideLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceSideChoices;
	m_choiceSide = new wxChoice( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceSideChoices, 0 );
	m_choiceSide->SetSelection( 0 );
	gbSizer1->Add( m_choiceSide, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_cbFloor = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Show floor"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbFloor, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), 0, 5 );


	sbSizer1->Add( gbSizer1, 1, wxEXPAND, 5 );


	bSizerBottom->Add( sbSizer1, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxString m_radioProjectionChoices[] = { _("Perspective"), _("Orthogonal") };
	int m_radioProjectionNChoices = sizeof( m_radioProjectionChoices ) / sizeof( wxString );
	m_radioProjection = new wxRadioBox( this, wxID_ANY, _("Projection"), wxDefaultPosition, wxDefaultSize, m_radioProjectionNChoices, m_radioProjectionChoices, 1, wxRA_SPECIFY_COLS );
	m_radioProjection->SetSelection( 1 );
	bSizerBottom->Add( m_radioProjection, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerBottom, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


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

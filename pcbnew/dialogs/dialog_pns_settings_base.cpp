///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_pns_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PNS_SETTINGS_BASE::DIALOG_PNS_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 350,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_modeChoices[] = { _("Highlight collisions"), _("Shove"), _("Walk around"), _("Figure out what's best") };
	int m_modeNChoices = sizeof( m_modeChoices ) / sizeof( wxString );
	m_mode = new wxRadioBox( this, wxID_ANY, _("Mode"), wxDefaultPosition, wxDefaultSize, m_modeNChoices, m_modeChoices, 1, wxRA_SPECIFY_COLS );
	m_mode->SetSelection( 0 );
	bMainSizer->Add( m_mode, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* bOptions;
	bOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText4 = new wxStaticText( this, wxID_ANY, _("Mouse drag behaviour:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer1->Add( m_staticText4, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_dragToolModeChoices[] = { _("move item"), _("interactive drag") };
	int m_dragToolModeNChoices = sizeof( m_dragToolModeChoices ) / sizeof( wxString );
	m_dragToolMode = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dragToolModeNChoices, m_dragToolModeChoices, 0 );
	m_dragToolMode->SetSelection( 0 );
	fgSizer1->Add( m_dragToolMode, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bOptions->Add( fgSizer1, 1, wxEXPAND, 5 );
	
	m_freeAngleMode = new wxCheckBox( this, wxID_ANY, _("Free angle mode (no shove/walkaround)"), wxDefaultPosition, wxDefaultSize, 0 );
	bOptions->Add( m_freeAngleMode, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_shoveVias = new wxCheckBox( this, wxID_ANY, _("Shove vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shoveVias->Enable( false );
	
	bOptions->Add( m_shoveVias, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_backPressure = new wxCheckBox( this, wxID_ANY, _("Jump over obstacles"), wxDefaultPosition, wxDefaultSize, 0 );
	bOptions->Add( m_backPressure, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_removeLoops = new wxCheckBox( this, wxID_ANY, _("Remove redundant tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	bOptions->Add( m_removeLoops, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_autoNeckdown = new wxCheckBox( this, wxID_ANY, _("Automatic neckdown"), wxDefaultPosition, wxDefaultSize, 0 );
	bOptions->Add( m_autoNeckdown, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_smoothDragged = new wxCheckBox( this, wxID_ANY, _("Smooth dragged segments"), wxDefaultPosition, wxDefaultSize, 0 );
	bOptions->Add( m_smoothDragged, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_violateDrc = new wxCheckBox( this, wxID_ANY, _("Allow DRC violations"), wxDefaultPosition, wxDefaultSize, 0 );
	bOptions->Add( m_violateDrc, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_suggestEnding = new wxCheckBox( this, wxID_ANY, _("Suggest track finish"), wxDefaultPosition, wxDefaultSize, 0 );
	m_suggestEnding->Enable( false );
	
	bOptions->Add( m_suggestEnding, 0, wxALL, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bOptions->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bEffort;
	bEffort = new wxBoxSizer( wxHORIZONTAL );
	
	m_effortLabel = new wxStaticText( this, wxID_ANY, _("Optimizer effort"), wxDefaultPosition, wxDefaultSize, 0 );
	m_effortLabel->Wrap( -1 );
	bEffort->Add( m_effortLabel, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bEffort->Add( 0, 0, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSlider;
	bSlider = new wxBoxSizer( wxVERTICAL );
	
	m_effort = new wxSlider( this, wxID_ANY, 1, 0, 2, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_BOTTOM|wxSL_HORIZONTAL|wxSL_TOP );
	bSlider->Add( m_effort, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSliderLabels;
	bSliderLabels = new wxBoxSizer( wxHORIZONTAL );
	
	m_lowLabel = new wxStaticText( this, wxID_ANY, _("low"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lowLabel->Wrap( -1 );
	m_lowLabel->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	
	bSliderLabels->Add( m_lowLabel, 0, 0, 5 );
	
	
	bSliderLabels->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_highLabel = new wxStaticText( this, wxID_ANY, _("high"), wxDefaultPosition, wxDefaultSize, 0 );
	m_highLabel->Wrap( -1 );
	m_highLabel->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	
	bSliderLabels->Add( m_highLabel, 0, 0, 5 );
	
	
	bSlider->Add( bSliderLabels, 1, wxEXPAND, 5 );
	
	
	bEffort->Add( bSlider, 1, wxEXPAND, 5 );
	
	
	bOptions->Add( bEffort, 0, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bOptions, 1, wxEXPAND|wxALL, 5 );
	
	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();
	
	bMainSizer->Add( m_stdButtons, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PNS_SETTINGS_BASE::OnClose ) );
	m_freeAngleMode->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onFreeAngleModeChange ), NULL, this );
	m_stdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::OnCancelClick ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::OnOkClick ), NULL, this );
}

DIALOG_PNS_SETTINGS_BASE::~DIALOG_PNS_SETTINGS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PNS_SETTINGS_BASE::OnClose ) );
	m_freeAngleMode->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onFreeAngleModeChange ), NULL, this );
	m_stdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::OnCancelClick ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::OnOkClick ), NULL, this );
	
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  8 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );
	
	wxString m_EdgesDisplayOptionChoices[] = { _("Line"), _("Filled"), _("Sketch") };
	int m_EdgesDisplayOptionNChoices = sizeof( m_EdgesDisplayOptionChoices ) / sizeof( wxString );
	m_EdgesDisplayOption = new wxRadioBox( this, ID_EDGE_SELECT, _("Edges"), wxDefaultPosition, wxDefaultSize, m_EdgesDisplayOptionNChoices, m_EdgesDisplayOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_EdgesDisplayOption->SetSelection( 0 );
	bSizerLeft->Add( m_EdgesDisplayOption, 1, wxALL|wxEXPAND, 5 );
	
	wxString m_TextDisplayOptionChoices[] = { _("Line"), _("Filled"), _("Sketch") };
	int m_TextDisplayOptionNChoices = sizeof( m_TextDisplayOptionChoices ) / sizeof( wxString );
	m_TextDisplayOption = new wxRadioBox( this, ID_TEXT_SELECT, _("Text"), wxDefaultPosition, wxDefaultSize, m_TextDisplayOptionNChoices, m_TextDisplayOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_TextDisplayOption->SetSelection( 0 );
	bSizerLeft->Add( m_TextDisplayOption, 1, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bSizerLeft, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerPads;
	sbSizerPads = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pads") ), wxVERTICAL );
	
	m_IsShowPadFill = new wxCheckBox( this, ID_PADFILL_OPT, _("Fill &pad"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerPads->Add( m_IsShowPadFill, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_IsShowPadNum = new wxCheckBox( this, wxID_ANY, _("Show pad &number"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerPads->Add( m_IsShowPadNum, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerRight->Add( sbSizerPads, 1, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizerViewOpt;
	sbSizerViewOpt = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pan and Zoom") ), wxVERTICAL );
	
	m_IsZoomNoCenter = new wxCheckBox( this, wxID_ANY, _("Do not center and warp cusor on zoom"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IsZoomNoCenter->SetToolTip( _("Keep the cursor at its current location when zooming") );
	
	sbSizerViewOpt->Add( m_IsZoomNoCenter, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_IsMiddleButtonPan = new wxCheckBox( this, wxID_ANY, _("Use middle mouse button to pan"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerViewOpt->Add( m_IsMiddleButtonPan, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_IsMiddleButtonPanLimited = new wxCheckBox( this, wxID_ANY, _("Limit panning to scroll size"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerViewOpt->Add( m_IsMiddleButtonPanLimited, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerRight->Add( sbSizerViewOpt, 1, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bSizerRight, 2, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Apply = new wxButton( this, wxID_APPLY );
	m_sdbSizer1->AddButton( m_sdbSizer1Apply );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	// Connect Events
	m_IsMiddleButtonPan->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnMiddleBtnPanEnbl ), NULL, this );
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnApplyClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnOkClick ), NULL, this );
}

DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::~DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE()
{
	// Disconnect Events
	m_IsMiddleButtonPan->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnMiddleBtnPanEnbl ), NULL, this );
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnApplyClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnOkClick ), NULL, this );
	
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jan 13 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_move_exact_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MOVE_EXACT_BASE::DIALOG_MOVE_EXACT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_polarCoords = new wxCheckBox( this, wxID_ANY, _("Use polar coordinates"), wxDefaultPosition, wxDefaultSize, 0 );
	bMainSizer->Add( m_polarCoords, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxFlexGridSizer* fgInputSizer;
	fgInputSizer = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgInputSizer->AddGrowableCol( 1 );
	fgInputSizer->SetFlexibleDirection( wxBOTH );
	fgInputSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_xLabel = new wxStaticText( this, wxID_ANY, _("x:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_xLabel->Wrap( -1 );
	fgInputSizer->Add( m_xLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_xEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgInputSizer->Add( m_xEntry, 0, wxALL|wxEXPAND, 5 );
	
	m_xUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xUnit->Wrap( -1 );
	fgInputSizer->Add( m_xUnit, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );
	
	m_clearX = new wxButton( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgInputSizer->Add( m_clearX, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_yLabel = new wxStaticText( this, wxID_ANY, _("y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yLabel->Wrap( -1 );
	fgInputSizer->Add( m_yLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_yEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgInputSizer->Add( m_yEntry, 0, wxALL|wxEXPAND, 5 );
	
	m_yUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yUnit->Wrap( -1 );
	fgInputSizer->Add( m_yUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_clearY = new wxButton( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgInputSizer->Add( m_clearY, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_rotLabel = new wxStaticText( this, wxID_ANY, _("Item rotation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotLabel->Wrap( -1 );
	fgInputSizer->Add( m_rotLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_rotEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgInputSizer->Add( m_rotEntry, 0, wxALL|wxEXPAND, 5 );
	
	m_rotUnit = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotUnit->Wrap( -1 );
	fgInputSizer->Add( m_rotUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_clearRot = new wxButton( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgInputSizer->Add( m_clearRot, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bMiddleSizer->Add( fgInputSizer, 1, wxALL|wxBOTTOM|wxEXPAND|wxTOP, 5 );
	
	wxString m_originChooserChoices[] = { _("Current Position"), _("User Origin"), _("Grid Origin"), _("Drill/Place Origin"), _("Sheet Origin") };
	int m_originChooserNChoices = sizeof( m_originChooserChoices ) / sizeof( wxString );
	m_originChooser = new wxRadioBox( this, wxID_ANY, _("Move relative to:"), wxDefaultPosition, wxDefaultSize, m_originChooserNChoices, m_originChooserChoices, 1, wxRA_SPECIFY_COLS );
	m_originChooser->SetSelection( 0 );
	bMiddleSizer->Add( m_originChooser, 0, wxALL, 5 );
	
	
	bMainSizer->Add( bMiddleSizer, 1, wxEXPAND, 5 );
	
	bAnchorSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_cbOverride = new wxCheckBox( this, wxID_ANY, _("Override default component anchor with:"), wxDefaultPosition, wxDefaultSize, 0 );
	bAnchorSizer->Add( m_cbOverride, 1, wxALL, 5 );
	
	wxString m_anchorChoiceChoices[] = { _("top left pad"), _("footprint center") };
	int m_anchorChoiceNChoices = sizeof( m_anchorChoiceChoices ) / sizeof( wxString );
	m_anchorChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_anchorChoiceNChoices, m_anchorChoiceChoices, 0 );
	m_anchorChoice->SetSelection( 0 );
	bAnchorSizer->Add( m_anchorChoice, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	bMainSizer->Add( bAnchorSizer, 0, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
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
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_MOVE_EXACT_BASE::OnClose ) );
	m_polarCoords->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnPolarChanged ), NULL, this );
	m_xEntry->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_clearX->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_yEntry->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_clearY->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_rotEntry->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_clearRot->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_originChooser->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnOriginChanged ), NULL, this );
	m_cbOverride->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnOverrideChanged ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnOkClick ), NULL, this );
}

DIALOG_MOVE_EXACT_BASE::~DIALOG_MOVE_EXACT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_MOVE_EXACT_BASE::OnClose ) );
	m_polarCoords->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnPolarChanged ), NULL, this );
	m_xEntry->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_clearX->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_yEntry->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_clearY->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_rotEntry->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_clearRot->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_originChooser->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnOriginChanged ), NULL, this );
	m_cbOverride->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnOverrideChanged ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnOkClick ), NULL, this );
	
}

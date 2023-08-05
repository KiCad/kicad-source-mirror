///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_move_exact_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MOVE_EXACT_BASE::DIALOG_MOVE_EXACT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgInputSizer;
	fgInputSizer = new wxFlexGridSizer( 0, 4, 5, 5 );
	fgInputSizer->AddGrowableCol( 1 );
	fgInputSizer->SetFlexibleDirection( wxBOTH );
	fgInputSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_xLabel = new wxStaticText( this, wxID_ANY, _("Move X:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_xLabel->Wrap( -1 );
	fgInputSizer->Add( m_xLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_xEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgInputSizer->Add( m_xEntry, 0, wxEXPAND, 5 );

	m_xUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xUnit->Wrap( -1 );
	fgInputSizer->Add( m_xUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_clearX = new wxButton( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
	fgInputSizer->Add( m_clearX, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_yLabel = new wxStaticText( this, wxID_ANY, _("Move Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yLabel->Wrap( -1 );
	fgInputSizer->Add( m_yLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_yEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgInputSizer->Add( m_yEntry, 0, wxEXPAND, 5 );

	m_yUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yUnit->Wrap( -1 );
	fgInputSizer->Add( m_yUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_clearY = new wxButton( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
	fgInputSizer->Add( m_clearY, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_rotLabel = new wxStaticText( this, wxID_ANY, _("Rotate:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotLabel->Wrap( -1 );
	fgInputSizer->Add( m_rotLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_rotEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgInputSizer->Add( m_rotEntry, 0, wxEXPAND, 5 );

	m_rotUnit = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotUnit->Wrap( -1 );
	fgInputSizer->Add( m_rotUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_clearRot = new wxButton( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
	fgInputSizer->Add( m_clearRot, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgInputSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	wxString m_anchorOptionsChoices[] = { _("Rotate around center of selection"), _("Rotate around local coordinates origin"), _("Rotate around drill/place origin") };
	int m_anchorOptionsNChoices = sizeof( m_anchorOptionsChoices ) / sizeof( wxString );
	m_anchorOptions = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_anchorOptionsNChoices, m_anchorOptionsChoices, 0 );
	m_anchorOptions->SetSelection( 0 );
	fgInputSizer->Add( m_anchorOptions, 0, wxEXPAND, 5 );


	bMainSizer->Add( fgInputSizer, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bBottomSizer;
	bBottomSizer = new wxBoxSizer( wxHORIZONTAL );

	m_polarCoords = new wxCheckBox( this, wxID_ANY, _("Use polar coordinates"), wxDefaultPosition, wxDefaultSize, 0 );
	bBottomSizer->Add( m_polarCoords, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bBottomSizer->Add( m_stdButtons, 1, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	bMainSizer->Add( bBottomSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_xEntry->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_xEntry->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextChanged ), NULL, this );
	m_clearX->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_yEntry->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_yEntry->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextChanged ), NULL, this );
	m_clearY->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_rotEntry->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_clearRot->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_polarCoords->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnPolarChanged ), NULL, this );
}

DIALOG_MOVE_EXACT_BASE::~DIALOG_MOVE_EXACT_BASE()
{
	// Disconnect Events
	m_xEntry->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_xEntry->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextChanged ), NULL, this );
	m_clearX->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_yEntry->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_yEntry->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextChanged ), NULL, this );
	m_clearY->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_rotEntry->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_MOVE_EXACT_BASE::OnTextFocusLost ), NULL, this );
	m_clearRot->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnClear ), NULL, this );
	m_polarCoords->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnPolarChanged ), NULL, this );

}

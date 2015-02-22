///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_move_exact_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MOVE_EXACT_BASE::DIALOG_MOVE_EXACT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_polarCoords = new wxCheckBox( this, wxID_ANY, _("Use polar coordinates"), wxDefaultPosition, wxDefaultSize, 0 );
	bMainSizer->Add( m_polarCoords, 0, wxALL|wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_xLabel = new wxStaticText( this, wxID_ANY, _("x:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_xLabel->Wrap( -1 );
	fgSizer2->Add( m_xLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_xEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_xEntry, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_xUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xUnit->Wrap( -1 );
	fgSizer2->Add( m_xUnit, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );
	
	m_clearX = new wxBitmapButton( this, wxID_CLEAR, wxArtProvider::GetBitmap( wxART_DELETE, wxART_BUTTON ), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	fgSizer2->Add( m_clearX, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_yLabel = new wxStaticText( this, wxID_ANY, _("y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yLabel->Wrap( -1 );
	fgSizer2->Add( m_yLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_yEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_yEntry, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_yUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yUnit->Wrap( -1 );
	fgSizer2->Add( m_yUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_clearY = new wxBitmapButton( this, wxID_CLEAR, wxArtProvider::GetBitmap( wxART_DELETE, wxART_BUTTON ), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	fgSizer2->Add( m_clearY, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_rotLabel = new wxStaticText( this, wxID_ANY, _("Item rotation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotLabel->Wrap( -1 );
	fgSizer2->Add( m_rotLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_rotEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rotEntry, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_rotUnit = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotUnit->Wrap( -1 );
	fgSizer2->Add( m_rotUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_clearRot = new wxBitmapButton( this, wxID_CLEAR, wxArtProvider::GetBitmap( wxART_DELETE, wxART_BUTTON ), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	fgSizer2->Add( m_clearRot, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainSizer->Add( fgSizer2, 1, wxEXPAND, 5 );
	
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
	m_stdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnCancelClick ), NULL, this );
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
	m_stdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnCancelClick ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MOVE_EXACT_BASE::OnOkClick ), NULL, this );
	
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_offset_item_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_OFFSET_ITEM_BASE::DIALOG_OFFSET_ITEM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 5, 5, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_xLabel = new wxStaticText( this, wxID_ANY, _("Offset X:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_xLabel->Wrap( -1 );
	fgSizer2->Add( m_xLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_xEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_xEntry, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_xUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xUnit->Wrap( -1 );
	fgSizer2->Add( m_xUnit, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxTOP|wxRIGHT, 5 );


	fgSizer2->Add( 10, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_clearX = new wxButton( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizer2->Add( m_clearX, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_yLabel = new wxStaticText( this, wxID_ANY, _("Offset Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yLabel->Wrap( -1 );
	fgSizer2->Add( m_yLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_yEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_yEntry, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_yUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yUnit->Wrap( -1 );
	fgSizer2->Add( m_yUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer2->Add( 10, 0, 1, wxEXPAND, 5 );

	m_clearY = new wxButton( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizer2->Add( m_clearY, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bMainSizer->Add( fgSizer2, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_polarCoords = new wxCheckBox( this, wxID_ANY, _("Use polar coordinates"), wxDefaultPosition, wxDefaultSize, 0 );
	m_polarCoords->SetValue(true);
	bSizerBottom->Add( m_polarCoords, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );


	bSizerBottom->Add( 40, 0, 1, wxEXPAND, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bSizerBottom->Add( m_stdButtons, 0, wxBOTTOM|wxTOP, 5 );


	bMainSizer->Add( bSizerBottom, 0, wxEXPAND|wxTOP, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_OFFSET_ITEM_BASE::OnClose ) );
	m_xEntry->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_OFFSET_ITEM_BASE::OnTextFocusLost ), NULL, this );
	m_clearX->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OFFSET_ITEM_BASE::OnClear ), NULL, this );
	m_yEntry->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_OFFSET_ITEM_BASE::OnTextFocusLost ), NULL, this );
	m_clearY->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OFFSET_ITEM_BASE::OnClear ), NULL, this );
	m_polarCoords->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_OFFSET_ITEM_BASE::OnPolarChanged ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OFFSET_ITEM_BASE::OnOkClick ), NULL, this );
}

DIALOG_OFFSET_ITEM_BASE::~DIALOG_OFFSET_ITEM_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_OFFSET_ITEM_BASE::OnClose ) );
	m_xEntry->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_OFFSET_ITEM_BASE::OnTextFocusLost ), NULL, this );
	m_clearX->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OFFSET_ITEM_BASE::OnClear ), NULL, this );
	m_yEntry->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_OFFSET_ITEM_BASE::OnTextFocusLost ), NULL, this );
	m_clearY->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OFFSET_ITEM_BASE::OnClear ), NULL, this );
	m_polarCoords->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_OFFSET_ITEM_BASE::OnPolarChanged ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OFFSET_ITEM_BASE::OnOkClick ), NULL, this );

}

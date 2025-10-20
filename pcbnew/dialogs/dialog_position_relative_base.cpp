///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_position_relative_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_POSITION_RELATIVE_BASE::DIALOG_POSITION_RELATIVE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	m_referenceInfo = new wxStaticText( this, wxID_ANY, _("Reference item: <none selected>"), wxDefaultPosition, wxDefaultSize, 0 );
	m_referenceInfo->Wrap( -1 );
	m_referenceInfo->SetMinSize( wxSize( 340,-1 ) );

	bUpperSizer->Add( m_referenceInfo, 0, wxALL|wxEXPAND, 5 );


	bUpperSizer->Add( 0, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerButtOpts;
	bSizerButtOpts = new wxBoxSizer( wxHORIZONTAL );

	m_user_origin_button = new wxButton( this, wxID_ANY, _("Use Local Origin"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtOpts->Add( m_user_origin_button, 1, wxALL|wxEXPAND, 5 );

	m_grid_origin_button = new wxButton( this, wxID_ANY, _("Use Grid Origin"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtOpts->Add( m_grid_origin_button, 1, wxALL|wxEXPAND, 5 );

	m_select_anchor_button = new wxButton( this, wxID_ANY, _("Select Item..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	m_select_anchor_button->SetToolTip( _("Click and select a board item.\nThe anchor position will be the position of the selected item.") );

	bSizerButtOpts->Add( m_select_anchor_button, 1, wxALL|wxEXPAND, 5 );

	m_select_point_button = new wxButton( this, wxID_ANY, _("Select Point..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtOpts->Add( m_select_point_button, 1, wxALL, 5 );


	bUpperSizer->Add( bSizerButtOpts, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer6->Add( m_staticline2, 0, wxEXPAND, 10 );


	bUpperSizer->Add( bSizer6, 0, wxEXPAND|wxTOP, 5 );


	bMainSizer->Add( bUpperSizer, 0, wxEXPAND|wxALL, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 5, 5, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_xLabel = new wxStaticText( this, wxID_ANY, _("Offset X:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_xLabel->Wrap( -1 );
	fgSizer2->Add( m_xLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_xEntry = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_xEntry, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_xUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xUnit->Wrap( -1 );
	fgSizer2->Add( m_xUnit, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );


	fgSizer2->Add( 10, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_clearX = new wxButton( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizer2->Add( m_clearX, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

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
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnClose ) );
	m_user_origin_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnUseUserOriginClick ), NULL, this );
	m_grid_origin_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnUseGridOriginClick ), NULL, this );
	m_select_anchor_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnSelectItemClick ), NULL, this );
	m_select_point_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnSelectPointClick ), NULL, this );
	m_xEntry->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnTextFocusLost ), NULL, this );
	m_clearX->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnClear ), NULL, this );
	m_yEntry->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnTextFocusLost ), NULL, this );
	m_clearY->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnClear ), NULL, this );
	m_polarCoords->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnPolarChanged ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnOkClick ), NULL, this );
}

DIALOG_POSITION_RELATIVE_BASE::~DIALOG_POSITION_RELATIVE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnClose ) );
	m_user_origin_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnUseUserOriginClick ), NULL, this );
	m_grid_origin_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnUseGridOriginClick ), NULL, this );
	m_select_anchor_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnSelectItemClick ), NULL, this );
	m_select_point_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnSelectPointClick ), NULL, this );
	m_xEntry->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnTextFocusLost ), NULL, this );
	m_clearX->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnClear ), NULL, this );
	m_yEntry->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnTextFocusLost ), NULL, this );
	m_clearY->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnClear ), NULL, this );
	m_polarCoords->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnPolarChanged ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_POSITION_RELATIVE_BASE::OnOkClick ), NULL, this );

}

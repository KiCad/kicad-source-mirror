///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_grid_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GRID_SETTINGS_BASE::DIALOG_GRID_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* m_fgGrid;
	m_fgGrid = new wxFlexGridSizer( 0, 3, 0, 0 );
	m_fgGrid->AddGrowableCol( 1 );
	m_fgGrid->SetFlexibleDirection( wxBOTH );
	m_fgGrid->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextName = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextName->Wrap( -1 );
	m_fgGrid->Add( m_staticTextName, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_textName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgGrid->Add( m_textName, 1, wxALL|wxEXPAND, 5 );

	m_staticTextOptional = new wxStaticText( this, wxID_ANY, _("(optional)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOptional->Wrap( -1 );
	m_fgGrid->Add( m_staticTextOptional, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxLEFT, 5 );

	m_staticTextX = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextX->Wrap( -1 );
	m_fgGrid->Add( m_staticTextX, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_textX = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgGrid->Add( m_textX, 1, wxALL|wxEXPAND, 5 );

	m_staticTextXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextXUnits->Wrap( -1 );
	m_fgGrid->Add( m_staticTextXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );


	m_fgGrid->Add( 0, 0, 0, wxEXPAND, 5 );

	m_checkLinked = new wxCheckBox( this, wxID_ANY, _("Linked"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkLinked->SetValue(true);
	m_fgGrid->Add( m_checkLinked, 1, wxALL|wxEXPAND, 5 );


	m_fgGrid->Add( 0, 0, 0, wxEXPAND, 5 );

	m_staticTextY = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextY->Wrap( -1 );
	m_fgGrid->Add( m_staticTextY, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_textY = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textY->Enable( false );

	m_fgGrid->Add( m_textY, 1, wxALL|wxEXPAND, 5 );

	m_staticTextYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextYUnits->Wrap( -1 );
	m_fgGrid->Add( m_staticTextYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );


	bMainSizer->Add( m_fgGrid, 0, wxEXPAND, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 1, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_checkLinked->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnLinkedChecked ), NULL, this );
}

DIALOG_GRID_SETTINGS_BASE::~DIALOG_GRID_SETTINGS_BASE()
{
	// Disconnect Events
	m_checkLinked->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnLinkedChecked ), NULL, this );

}

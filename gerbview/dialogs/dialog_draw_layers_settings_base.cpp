///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_draw_layers_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DRAW_LAYERS_SETTINGS_BASE::DIALOG_DRAW_LAYERS_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_namiSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_stLayerNameTitle = new wxStaticText( this, wxID_ANY, _("Active layer name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLayerNameTitle->Wrap( -1 );
	bSizer3->Add( m_stLayerNameTitle, 0, wxALL, 5 );

	m_stLayerName = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLayerName->Wrap( -1 );
	bSizer3->Add( m_stLayerName, 0, wxALL, 5 );


	m_namiSizer->Add( bSizer3, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_stOffsetX = new wxStaticText( this, wxID_ANY, _("Offset X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stOffsetX->Wrap( -1 );
	fgSizer->Add( m_stOffsetX, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_tcOffsetX = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_tcOffsetX, 0, wxALL|wxEXPAND, 5 );

	m_stUnitX = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stUnitX->Wrap( -1 );
	fgSizer->Add( m_stUnitX, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_stOffsetY = new wxStaticText( this, wxID_ANY, _("Offset Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stOffsetY->Wrap( -1 );
	fgSizer->Add( m_stOffsetY, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_tcOffsetY = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_tcOffsetY, 0, wxALL|wxEXPAND, 5 );

	m_stUnitY = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stUnitY->Wrap( -1 );
	fgSizer->Add( m_stUnitY, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_stLayerRot = new wxStaticText( this, wxID_ANY, _("Rotate counterclockwise:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLayerRot->Wrap( -1 );
	fgSizer->Add( m_stLayerRot, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_tcRotation = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_tcRotation, 0, wxALL|wxEXPAND, 5 );

	m_stUnitRot = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stUnitRot->Wrap( -1 );
	fgSizer->Add( m_stUnitRot, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	m_namiSizer->Add( fgSizer, 0, wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_namiSizer->Add( m_staticline1, 0, wxALL|wxEXPAND, 5 );

	wxString m_rbScopeChoices[] = { _("Active layer"), _("All layers"), _("All visible layers") };
	int m_rbScopeNChoices = sizeof( m_rbScopeChoices ) / sizeof( wxString );
	m_rbScope = new wxRadioBox( this, wxID_ANY, _("Scope"), wxDefaultPosition, wxDefaultSize, m_rbScopeNChoices, m_rbScopeChoices, 1, wxRA_SPECIFY_COLS );
	m_rbScope->SetSelection( 0 );
	m_namiSizer->Add( m_rbScope, 0, wxALL, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_namiSizer->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bottomButtonsSizer;
	bottomButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();

	bottomButtonsSizer->Add( m_sdbSizerStdButtons, 1, wxALL|wxEXPAND, 5 );


	m_namiSizer->Add( bottomButtonsSizer, 0, wxLEFT|wxEXPAND, 5 );


	this->SetSizer( m_namiSizer );
	this->Layout();
	m_namiSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_DRAW_LAYERS_SETTINGS_BASE::OnInitDlg ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_DRAW_LAYERS_SETTINGS_BASE::OnUpdateUI ) );
}

DIALOG_DRAW_LAYERS_SETTINGS_BASE::~DIALOG_DRAW_LAYERS_SETTINGS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_DRAW_LAYERS_SETTINGS_BASE::OnInitDlg ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_DRAW_LAYERS_SETTINGS_BASE::OnUpdateUI ) );

}

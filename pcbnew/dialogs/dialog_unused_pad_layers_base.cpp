///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_unused_pad_layers_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_UNUSED_PAD_LAYERS_BASE::DIALOG_UNUSED_PAD_LAYERS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizerProps;
	fgSizerProps = new wxFlexGridSizer( 0, 2, 0, 5 );
	fgSizerProps->SetFlexibleDirection( wxVERTICAL );
	fgSizerProps->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticBoxSizer* sbSizerScope;
	sbSizerScope = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scope") ), wxVERTICAL );

	m_cbVias = new wxCheckBox( sbSizerScope->GetStaticBox(), wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerScope->Add( m_cbVias, 0, wxALL, 5 );

	m_cbPads = new wxCheckBox( sbSizerScope->GetStaticBox(), wxID_ANY, _("Pads"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerScope->Add( m_cbPads, 0, wxALL, 5 );


	fgSizerProps->Add( sbSizerScope, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_rbActionChoices[] = { _("&Remove unused layers"), _("Res&tore unused layers") };
	int m_rbActionNChoices = sizeof( m_rbActionChoices ) / sizeof( wxString );
	m_rbAction = new wxRadioBox( this, wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, m_rbActionNChoices, m_rbActionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbAction->SetSelection( 1 );
	fgSizerProps->Add( m_rbAction, 1, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_cbSelectedOnly = new wxCheckBox( this, wxID_ANY, _("&Selection only"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerProps->Add( m_cbSelectedOnly, 0, wxEXPAND|wxLEFT, 10 );

	m_cbPreservePads = new wxCheckBox( this, wxID_ANY, _("Keep &outside layers"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerProps->Add( m_cbPreservePads, 0, wxEXPAND|wxLEFT, 10 );


	bSizer2->Add( fgSizerProps, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerPreview;
	bSizerPreview = new wxBoxSizer( wxVERTICAL );


	bSizerPreview->Add( 0, 0, 0, wxEXPAND, 5 );

	m_image = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerPreview->Add( m_image, 0, wxTOP|wxRIGHT|wxLEFT, 10 );


	bSizerPreview->Add( 0, 0, 0, wxEXPAND, 5 );


	bSizer2->Add( bSizerPreview, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	m_MainSizer->Add( bSizer2, 6, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_MainSizer->Add( m_staticline2, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_StdButtons = new wxStdDialogButtonSizer();
	m_StdButtonsOK = new wxButton( this, wxID_OK );
	m_StdButtons->AddButton( m_StdButtonsOK );
	m_StdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_StdButtons->AddButton( m_StdButtonsCancel );
	m_StdButtons->Realize();

	m_MainSizer->Add( m_StdButtons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_cbVias->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_cbPads->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_rbAction->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_cbPreservePads->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
}

DIALOG_UNUSED_PAD_LAYERS_BASE::~DIALOG_UNUSED_PAD_LAYERS_BASE()
{
	// Disconnect Events
	m_cbVias->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_cbPads->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_rbAction->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_cbPreservePads->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );

}

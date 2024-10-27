///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
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

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_cbVias = new wxCheckBox( this, wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_cbVias, 0, wxALL, 5 );

	m_cbPads = new wxCheckBox( this, wxID_ANY, _("Pads"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_cbPads, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer4->Add( 0, 15, 0, wxEXPAND, 5 );

	m_cbSelectedOnly = new wxCheckBox( this, wxID_ANY, _("&Selected only"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_cbSelectedOnly, 0, wxEXPAND|wxBOTTOM|wxLEFT, 5 );

	m_cbPreserveExternalLayers = new wxCheckBox( this, wxID_ANY, _("Keep &outside layers"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_cbPreserveExternalLayers, 0, wxEXPAND|wxLEFT, 5 );


	bMargins->Add( bSizer4, 1, wxEXPAND|wxTOP|wxLEFT, 10 );

	wxBoxSizer* bSizerPreview;
	bSizerPreview = new wxBoxSizer( wxVERTICAL );


	bSizerPreview->Add( 0, 0, 0, wxEXPAND, 5 );

	m_image = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerPreview->Add( m_image, 0, wxTOP|wxRIGHT|wxLEFT, 10 );


	bSizerPreview->Add( 0, 0, 0, wxEXPAND, 5 );


	bMargins->Add( bSizerPreview, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	m_MainSizer->Add( bMargins, 6, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_StdButtons = new wxStdDialogButtonSizer();
	m_StdButtonsOK = new wxButton( this, wxID_OK );
	m_StdButtons->AddButton( m_StdButtonsOK );
	m_StdButtonsApply = new wxButton( this, wxID_APPLY );
	m_StdButtons->AddButton( m_StdButtonsApply );
	m_StdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_StdButtons->AddButton( m_StdButtonsCancel );
	m_StdButtons->Realize();

	m_MainSizer->Add( m_StdButtons, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_cbVias->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_cbPads->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_cbPreserveExternalLayers->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_StdButtonsApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::onApply ), NULL, this );
	m_StdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::onOK ), NULL, this );
}

DIALOG_UNUSED_PAD_LAYERS_BASE::~DIALOG_UNUSED_PAD_LAYERS_BASE()
{
	// Disconnect Events
	m_cbVias->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_cbPads->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_cbPreserveExternalLayers->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_StdButtonsApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::onApply ), NULL, this );
	m_StdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::onOK ), NULL, this );

}

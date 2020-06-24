///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun 18 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_junction_props_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_JUNCTION_PROPS_BASE::DIALOG_JUNCTION_PROPS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextDiameter = new wxStaticText( this, wxID_ANY, _("Diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDiameter->Wrap( -1 );
	fgSizer2->Add( m_staticTextDiameter, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_textCtrlDiameter = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_textCtrlDiameter, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxLEFT, 5 );

	m_staticTextDiameterUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDiameterUnits->Wrap( -1 );
	fgSizer2->Add( m_staticTextDiameterUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_staticTextColor = new wxStaticText( this, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextColor->Wrap( -1 );
	fgSizer2->Add( m_staticTextColor, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_buttonColor = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	fgSizer2->Add( m_buttonColor, 0, wxEXPAND|wxLEFT, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer2->Add( fgSizer2, 1, wxALL|wxEXPAND, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer2->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizer2->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND, 5 );


	this->SetSizer( bSizer2 );
	this->Layout();
	bSizer2->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_buttonColor->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_JUNCTION_PROPS_BASE::onColorButtonClicked ), NULL, this );
	m_sdbSizerApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_JUNCTION_PROPS_BASE::resetDefaults ), NULL, this );
}

DIALOG_JUNCTION_PROPS_BASE::~DIALOG_JUNCTION_PROPS_BASE()
{
	// Disconnect Events
	m_buttonColor->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_JUNCTION_PROPS_BASE::onColorButtonClicked ), NULL, this );
	m_sdbSizerApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_JUNCTION_PROPS_BASE::resetDefaults ), NULL, this );

}

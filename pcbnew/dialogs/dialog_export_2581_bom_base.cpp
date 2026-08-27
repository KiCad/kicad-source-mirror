///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_export_2581_bom_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_2581_BOM_BASE::DIALOG_EXPORT_2581_BOM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer4->AddGrowableCol( 1 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblBomRev = new wxStaticText( this, wxID_ANY, _("BOM revision:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblBomRev->Wrap( -1 );
	m_lblBomRev->SetToolTip( _("Revision string for the BOM section. Auto-populated from schematic title block revision") );

	fgSizer4->Add( m_lblBomRev, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_textBomRev = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textBomRev->SetToolTip( _("Revision string for the BOM section. Auto-populated from schematic title block revision") );

	fgSizer4->Add( m_textBomRev, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_lblOEM = new wxStaticText( this, wxID_ANY, _("Internal ID:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblOEM->Wrap( -1 );
	m_lblOEM->SetToolTip( _("Part ID number used internally during design.\nThis number must be unique to each part.") );

	fgSizer4->Add( m_lblOEM, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxString m_oemRefChoices[] = { _("Generate unique") };
	int m_oemRefNChoices = sizeof( m_oemRefChoices ) / sizeof( wxString );
	m_oemRef = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_oemRefNChoices, m_oemRefChoices, 0 );
	m_oemRef->SetSelection( 0 );
	m_oemRef->SetToolTip( _("Part ID number used internally during design.\nThis number must be unique to each part.") );

	fgSizer4->Add( m_oemRef, 0, wxEXPAND|wxRIGHT, 5 );

	m_staticText6 = new wxStaticText( this, wxID_ANY, _("Manufacturer P/N:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_staticText6->SetToolTip( _("Column containing the manufacturer part number") );

	fgSizer4->Add( m_staticText6, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxString m_choiceMPNChoices[] = { _("Omit") };
	int m_choiceMPNNChoices = sizeof( m_choiceMPNChoices ) / sizeof( wxString );
	m_choiceMPN = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceMPNNChoices, m_choiceMPNChoices, 0 );
	m_choiceMPN->SetSelection( 0 );
	m_choiceMPN->SetToolTip( _("Column containing the manufacturer part number") );

	fgSizer4->Add( m_choiceMPN, 0, wxEXPAND|wxRIGHT, 5 );

	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Manufacturer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer4->Add( m_staticText7, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxString m_choiceMfgChoices[] = { _("N/A") };
	int m_choiceMfgNChoices = sizeof( m_choiceMfgChoices ) / sizeof( wxString );
	m_choiceMfg = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceMfgNChoices, m_choiceMfgChoices, 0 );
	m_choiceMfg->SetSelection( 0 );
	m_choiceMfg->Enable( false );

	fgSizer4->Add( m_choiceMfg, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_staticText8 = new wxStaticText( this, wxID_ANY, _("Distributor P/N:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizer4->Add( m_staticText8, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceDistPNChoices[] = { _("Omit") };
	int m_choiceDistPNNChoices = sizeof( m_choiceDistPNChoices ) / sizeof( wxString );
	m_choiceDistPN = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceDistPNNChoices, m_choiceDistPNChoices, 0 );
	m_choiceDistPN->SetSelection( 0 );
	m_choiceDistPN->SetToolTip( _("Column containing the distributor part number") );

	fgSizer4->Add( m_choiceDistPN, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Distributor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer4->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textDistributor = new wxTextCtrl( this, wxID_ANY, _("N/A"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	fgSizer4->Add( m_textDistributor, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	bMainSizer->Add( fgSizer4, 1, wxEXPAND|wxALL, 10 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_choiceMPN->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_2581_BOM_BASE::onMfgPNChange ), NULL, this );
	m_choiceDistPN->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_2581_BOM_BASE::onDistPNChange ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_2581_BOM_BASE::onOKClick ), NULL, this );
}

DIALOG_EXPORT_2581_BOM_BASE::~DIALOG_EXPORT_2581_BOM_BASE()
{
	// Disconnect Events
	m_choiceMPN->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_2581_BOM_BASE::onMfgPNChange ), NULL, this );
	m_choiceDistPN->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_2581_BOM_BASE::onDistPNChange ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_2581_BOM_BASE::onOKClick ), NULL, this );

}

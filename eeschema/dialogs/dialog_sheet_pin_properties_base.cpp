///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_sheet_pin_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SHEET_PIN_PROPERTIES_BASE::DIALOG_SHEET_PIN_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* m_mainSizer;
	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* m_nameSizer;
	m_nameSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 2, 2, 4, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer2->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_comboName = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizer2->Add( m_comboName, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxTOP, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_hyperlink1 = new wxHyperlinkCtrl( this, wxID_ANY, _("Syntax help"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink1->SetToolTip( _("Show syntax help window") );

	fgSizer2->Add( m_hyperlink1, 0, wxALIGN_RIGHT|wxRIGHT|wxLEFT, 7 );


	m_nameSizer->Add( fgSizer2, 0, wxEXPAND, 5 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_nameSizer->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 2, 3, 2, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	fgSizer1->Add( m_textSizeLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_textSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textSizeCtrl, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Connection type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer1->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_choiceConnectionTypeChoices;
	m_choiceConnectionType = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceConnectionTypeChoices, 0 );
	m_choiceConnectionType->SetSelection( 0 );
	m_choiceConnectionType->SetMinSize( wxSize( 160,-1 ) );

	fgSizer1->Add( m_choiceConnectionType, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	m_nameSizer->Add( fgSizer1, 0, wxEXPAND, 6 );


	m_mainSizer->Add( m_nameSizer, 1, wxALL|wxEXPAND, 10 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_mainSizer->Add( m_staticline2, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_mainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 6 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_comboName->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SHEET_PIN_PROPERTIES_BASE::onComboBox ), NULL, this );
	m_hyperlink1->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_SHEET_PIN_PROPERTIES_BASE::OnSyntaxHelp ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PIN_PROPERTIES_BASE::onOKButton ), NULL, this );
}

DIALOG_SHEET_PIN_PROPERTIES_BASE::~DIALOG_SHEET_PIN_PROPERTIES_BASE()
{
	// Disconnect Events
	m_comboName->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SHEET_PIN_PROPERTIES_BASE::onComboBox ), NULL, this );
	m_hyperlink1->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_SHEET_PIN_PROPERTIES_BASE::OnSyntaxHelp ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PIN_PROPERTIES_BASE::onOKButton ), NULL, this );

}

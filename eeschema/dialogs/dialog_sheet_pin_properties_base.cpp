///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.1.0-0-g733bf3d)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
#include "widgets/color_swatch.h"
#include "widgets/font_choice.h"

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

	wxBoxSizer* optionsSizer;
	optionsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_shapeSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Shape") ), wxVERTICAL );

	m_input = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Input"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_input, 0, wxBOTTOM|wxRIGHT, 2 );

	m_output = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Output"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_output, 0, wxBOTTOM|wxRIGHT, 3 );

	m_bidirectional = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Bidirectional"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_bidirectional, 0, wxBOTTOM|wxRIGHT, 3 );

	m_triState = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Tri-state"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_triState, 0, wxBOTTOM|wxRIGHT, 3 );

	m_passive = new wxRadioButton( m_shapeSizer->GetStaticBox(), wxID_ANY, _("Passive"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeSizer->Add( m_passive, 0, wxBOTTOM|wxRIGHT, 3 );


	optionsSizer->Add( m_shapeSizer, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );

	wxStaticBoxSizer* formatting;
	formatting = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Formatting") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_fontLabel = new wxStaticText( formatting->GetStaticBox(), wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	gbSizer1->Add( m_fontLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_fontCtrlChoices[] = { _("Default Font"), _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( formatting->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	gbSizer1->Add( m_fontCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* formattingSizer;
	formattingSizer = new wxBoxSizer( wxHORIZONTAL );

	m_separator1 = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator1->Enable( false );

	formattingSizer->Add( m_separator1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_bold = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_bold->SetToolTip( _("Bold") );

	formattingSizer->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_italic = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_italic->SetToolTip( _("Italic") );

	formattingSizer->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator2 = new BITMAP_BUTTON( formatting->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator2->Enable( false );

	formattingSizer->Add( m_separator2, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( formattingSizer, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_textSizeLabel = new wxStaticText( formatting->GetStaticBox(), wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	gbSizer1->Add( m_textSizeLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizer71;
	bSizer71 = new wxBoxSizer( wxHORIZONTAL );

	m_textSizeCtrl = new wxTextCtrl( formatting->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer71->Add( m_textSizeCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textSizeUnits = new wxStaticText( formatting->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	bSizer71->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_textColorLabel = new wxStaticText( formatting->GetStaticBox(), wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textColorLabel->Wrap( -1 );
	bSizer71->Add( m_textColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );


	bSizer71->Add( 5, 0, 0, 0, 5 );

	m_panelBorderColor1 = new wxPanel( formatting->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxVERTICAL );

	m_textColorSwatch = new COLOR_SWATCH( m_panelBorderColor1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer22->Add( m_textColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelBorderColor1->SetSizer( bSizer22 );
	m_panelBorderColor1->Layout();
	bSizer22->Fit( m_panelBorderColor1 );
	bSizer71->Add( m_panelBorderColor1, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( bSizer71, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );


	formatting->Add( gbSizer1, 1, wxEXPAND, 5 );


	optionsSizer->Add( formatting, 1, wxEXPAND|wxTOP, 5 );


	m_nameSizer->Add( optionsSizer, 1, wxEXPAND, 5 );


	m_mainSizer->Add( m_nameSizer, 1, wxALL|wxEXPAND, 10 );

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

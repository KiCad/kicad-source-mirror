///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_label_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LABEL_EDITOR_BASE::DIALOG_LABEL_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_textControlSizer = new wxFlexGridSizer( 2, 2, 3, 3 );
	m_textControlSizer->AddGrowableCol( 1 );
	m_textControlSizer->AddGrowableRow( 0 );
	m_textControlSizer->SetFlexibleDirection( wxBOTH );
	m_textControlSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("&Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	m_staticText1->SetToolTip( _("Enter the text to be used within the schematic") );
	
	m_textControlSizer->Add( m_staticText1, 0, wxRIGHT, 3 );
	
	wxBoxSizer* bSizeText;
	bSizeText = new wxBoxSizer( wxVERTICAL );
	
	m_textLabelSingleLine = new wxTextCtrl( this, wxID_VALUESINGLE, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_textLabelSingleLine->SetValidator( wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST, &m_labelText ) );
	
	bSizeText->Add( m_textLabelSingleLine, 0, wxEXPAND|wxLEFT, 3 );
	
	m_textLabelMultiLine = new wxTextCtrl( this, wxID_VALUEMULTI, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_PROCESS_ENTER );
	m_textLabelMultiLine->SetMaxLength( 0 ); 
	m_textLabelMultiLine->SetMinSize( wxSize( -1,60 ) );
	
	bSizeText->Add( m_textLabelMultiLine, 1, wxEXPAND|wxLEFT, 3 );
	
	
	m_textControlSizer->Add( bSizeText, 1, wxEXPAND, 3 );
	
	m_SizeTitle = new wxStaticText( this, wxID_ANY, _("&Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeTitle->Wrap( -1 );
	m_textControlSizer->Add( m_SizeTitle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );
	
	wxBoxSizer* bSizeCtrlSizer;
	bSizeCtrlSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_TextSize = new wxTextCtrl( this, wxID_SIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizeCtrlSizer->Add( m_TextSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT, 3 );
	
	m_staticSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticSizeUnits->Wrap( -1 );
	bSizeCtrlSizer->Add( m_staticSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );
	
	
	m_textControlSizer->Add( bSizeCtrlSizer, 1, wxEXPAND, 3 );
	
	
	bMainSizer->Add( m_textControlSizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 12 );
	
	wxBoxSizer* m_OptionsSizer;
	m_OptionsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_TextOrientChoices[] = { _("Right"), _("Up"), _("Left"), _("Down") };
	int m_TextOrientNChoices = sizeof( m_TextOrientChoices ) / sizeof( wxString );
	m_TextOrient = new wxRadioBox( this, wxID_ANY, _("O&rientation"), wxDefaultPosition, wxDefaultSize, m_TextOrientNChoices, m_TextOrientChoices, 1, wxRA_SPECIFY_COLS );
	m_TextOrient->SetSelection( 0 );
	m_OptionsSizer->Add( m_TextOrient, 1, wxRIGHT|wxTOP, 3 );
	
	wxString m_TextStyleChoices[] = { _("Normal"), _("Italic"), _("Bold"), _("Bold Italic") };
	int m_TextStyleNChoices = sizeof( m_TextStyleChoices ) / sizeof( wxString );
	m_TextStyle = new wxRadioBox( this, wxID_ANY, _("St&yle"), wxDefaultPosition, wxDefaultSize, m_TextStyleNChoices, m_TextStyleChoices, 1, wxRA_SPECIFY_COLS );
	m_TextStyle->SetSelection( 0 );
	m_OptionsSizer->Add( m_TextStyle, 1, wxLEFT|wxRIGHT|wxTOP, 3 );
	
	wxString m_TextShapeChoices[] = { _("Input"), _("Output"), _("Bidirectional"), _("Tri-State"), _("Passive") };
	int m_TextShapeNChoices = sizeof( m_TextShapeChoices ) / sizeof( wxString );
	m_TextShape = new wxRadioBox( this, wxID_ANY, _("S&hape"), wxDefaultPosition, wxDefaultSize, m_TextShapeNChoices, m_TextShapeChoices, 1, wxRA_SPECIFY_COLS );
	m_TextShape->SetSelection( 0 );
	m_OptionsSizer->Add( m_TextShape, 1, wxALL|wxLEFT|wxTOP, 3 );
	
	
	bMainSizer->Add( m_OptionsSizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 12 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bMainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 12 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	m_textLabelSingleLine->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnEnterKey ), NULL, this );
	m_textLabelMultiLine->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnEnterKey ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnOkClick ), NULL, this );
}

DIALOG_LABEL_EDITOR_BASE::~DIALOG_LABEL_EDITOR_BASE()
{
	// Disconnect Events
	m_textLabelSingleLine->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnEnterKey ), NULL, this );
	m_textLabelMultiLine->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnEnterKey ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnOkClick ), NULL, this );
	
}

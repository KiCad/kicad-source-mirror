///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_text_and_label_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE::DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_textEntrySizer = new wxFlexGridSizer( 5, 2, 1, 3 );
	m_textEntrySizer->AddGrowableCol( 1 );
	m_textEntrySizer->AddGrowableRow( 1 );
	m_textEntrySizer->SetFlexibleDirection( wxBOTH );
	m_textEntrySizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textEntrySizer->SetMinSize( wxSize( 500,-1 ) );
	m_labelSingleLine = new wxStaticText( this, wxID_ANY, _("Label:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelSingleLine->Wrap( -1 );
	m_labelSingleLine->SetToolTip( _("Enter the text to be used within the schematic") );

	m_textEntrySizer->Add( m_labelSingleLine, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 2 );

	m_valueSingleLine = new wxTextCtrl( this, wxID_VALUESINGLE, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER|wxTE_RICH );
	m_valueSingleLine->SetMinSize( wxSize( 360,-1 ) );

	m_textEntrySizer->Add( m_valueSingleLine, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 2 );

	m_labelMultiLine = new wxStaticText( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelMultiLine->Wrap( -1 );
	m_textEntrySizer->Add( m_labelMultiLine, 0, wxTOP|wxRIGHT, 2 );

	m_valueMultiLine = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN, wxEmptyString );
	m_valueMultiLine->SetUseTabs( true );
	m_valueMultiLine->SetTabWidth( 4 );
	m_valueMultiLine->SetIndent( 4 );
	m_valueMultiLine->SetTabIndents( false );
	m_valueMultiLine->SetBackSpaceUnIndents( false );
	m_valueMultiLine->SetViewEOL( false );
	m_valueMultiLine->SetViewWhiteSpace( false );
	m_valueMultiLine->SetMarginWidth( 2, 0 );
	m_valueMultiLine->SetIndentationGuides( false );
	m_valueMultiLine->SetReadOnly( false );
	m_valueMultiLine->SetMarginWidth( 1, 0 );
	m_valueMultiLine->SetMarginWidth( 0, 0 );
	m_valueMultiLine->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_valueMultiLine->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_valueMultiLine->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_valueMultiLine->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_valueMultiLine->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_valueMultiLine->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_valueMultiLine->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_valueMultiLine->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_valueMultiLine->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_valueMultiLine->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_valueMultiLine->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_valueMultiLine->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_valueMultiLine->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_valueMultiLine->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_valueMultiLine->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_valueMultiLine->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_valueMultiLine->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	m_textEntrySizer->Add( m_valueMultiLine, 1, wxEXPAND|wxBOTTOM, 2 );

	m_labelCombo = new wxStaticText( this, wxID_ANY, _("Label:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCombo->Wrap( -1 );
	m_textEntrySizer->Add( m_labelCombo, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_valueCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER );
	m_valueCombo->SetMinSize( wxSize( 360,-1 ) );

	m_textEntrySizer->Add( m_valueCombo, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Text Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_textSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 6 );

	wxBoxSizer* bSizeCtrlSizer;
	bSizeCtrlSizer = new wxBoxSizer( wxHORIZONTAL );

	m_textSizeCtrl = new wxTextCtrl( this, wxID_SIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizeCtrlSizer->Add( m_textSizeCtrl, 0, wxBOTTOM|wxRIGHT|wxTOP, 6 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	bSizeCtrlSizer->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 6 );


	bSizeCtrlSizer->Add( 0, 0, 1, wxEXPAND, 15 );

	m_formattingHelp = new wxHyperlinkCtrl( this, wxID_ANY, _("Syntax help"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_formattingHelp->SetToolTip( _("Show syntax help window") );

	bSizeCtrlSizer->Add( m_formattingHelp, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	m_textEntrySizer->Add( bSizeCtrlSizer, 1, wxEXPAND, 6 );

	m_note1 = new wxStaticText( this, wxID_ANY, _("Note:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_note1->Wrap( -1 );
	m_textEntrySizer->Add( m_note1, 0, wxTOP|wxRIGHT, 2 );

	m_note2 = new wxStaticText( this, wxID_ANY, _("The margins around the text are controlled by the text offset ratio\nin Schematic Setup > General > Formatting."), wxDefaultPosition, wxDefaultSize, 0 );
	m_note2->Wrap( -1 );
	m_textEntrySizer->Add( m_note2, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 2 );


	bMainSizer->Add( m_textEntrySizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 12 );

	wxBoxSizer* m_OptionsSizer;
	m_OptionsSizer = new wxBoxSizer( wxHORIZONTAL );

	wxString m_TextOrientChoices[] = { _("Align right"), _("Align bottom"), _("Align left"), _("Align top") };
	int m_TextOrientNChoices = sizeof( m_TextOrientChoices ) / sizeof( wxString );
	m_TextOrient = new wxRadioBox( this, wxID_ANY, _("Justification"), wxDefaultPosition, wxDefaultSize, m_TextOrientNChoices, m_TextOrientChoices, 1, wxRA_SPECIFY_COLS );
	m_TextOrient->SetSelection( 2 );
	m_OptionsSizer->Add( m_TextOrient, 1, wxRIGHT|wxTOP|wxEXPAND, 3 );

	wxString m_TextStyleChoices[] = { _("Normal"), _("Italic"), _("Bold"), _("Bold and italic") };
	int m_TextStyleNChoices = sizeof( m_TextStyleChoices ) / sizeof( wxString );
	m_TextStyle = new wxRadioBox( this, wxID_ANY, _("Style"), wxDefaultPosition, wxDefaultSize, m_TextStyleNChoices, m_TextStyleChoices, 1, wxRA_SPECIFY_COLS );
	m_TextStyle->SetSelection( 0 );
	m_OptionsSizer->Add( m_TextStyle, 1, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 3 );

	wxString m_TextShapeChoices[] = { _("Input"), _("Output"), _("Bidirectional"), _("Tri-state"), _("Passive") };
	int m_TextShapeNChoices = sizeof( m_TextShapeChoices ) / sizeof( wxString );
	m_TextShape = new wxRadioBox( this, wxID_ANY, _("Shape"), wxDefaultPosition, wxDefaultSize, m_TextShapeNChoices, m_TextShapeChoices, 1, wxRA_SPECIFY_COLS );
	m_TextShape->SetSelection( 0 );
	m_OptionsSizer->Add( m_TextShape, 1, wxEXPAND|wxTOP|wxLEFT, 3 );


	bMainSizer->Add( m_OptionsSizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizer4->Add( m_sdbSizer1, 1, wxALL|wxEXPAND, 5 );


	bMainSizer->Add( bSizer4, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_valueSingleLine->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE::OnEnterKey ), NULL, this );
	m_valueMultiLine->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_valueCombo->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE::OnEnterKey ), NULL, this );
	m_formattingHelp->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE::OnFormattingHelp ), NULL, this );
}

DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE::~DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE()
{
	// Disconnect Events
	m_valueSingleLine->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE::OnEnterKey ), NULL, this );
	m_valueMultiLine->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_valueCombo->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE::OnEnterKey ), NULL, this );
	m_formattingHelp->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE::OnFormattingHelp ), NULL, this );

}

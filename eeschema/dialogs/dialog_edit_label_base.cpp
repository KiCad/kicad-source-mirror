///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_label_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LABEL_EDITOR_BASE::DIALOG_LABEL_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_textEntrySizer = new wxFlexGridSizer( 7, 2, 3, 3 );
	m_textEntrySizer->AddGrowableCol( 1 );
	m_textEntrySizer->AddGrowableRow( 1 );
	m_textEntrySizer->SetFlexibleDirection( wxBOTH );
	m_textEntrySizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelSingleLine = new wxStaticText( this, wxID_ANY, _("Label:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelSingleLine->Wrap( -1 );
	m_labelSingleLine->SetToolTip( _("Enter the text to be used within the schematic") );

	m_textEntrySizer->Add( m_labelSingleLine, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 3 );

	m_valueSingleLine = new wxTextCtrl( this, wxID_VALUESINGLE, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER|wxTE_RICH );
	m_valueSingleLine->SetMinSize( wxSize( 360,-1 ) );

	m_textEntrySizer->Add( m_valueSingleLine, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 3 );

	m_labelMultiLine = new wxStaticText( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelMultiLine->Wrap( -1 );
	m_textEntrySizer->Add( m_labelMultiLine, 0, wxRIGHT, 5 );

	m_valueMultiLine = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString );
	m_valueMultiLine->SetUseTabs( true );
	m_valueMultiLine->SetTabWidth( 4 );
	m_valueMultiLine->SetIndent( 4 );
	m_valueMultiLine->SetTabIndents( false );
	m_valueMultiLine->SetBackSpaceUnIndents( false );
	m_valueMultiLine->SetViewEOL( false );
	m_valueMultiLine->SetViewWhiteSpace( false );
	m_valueMultiLine->SetMarginWidth( 2, 0 );
	m_valueMultiLine->SetIndentationGuides( false );
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
	m_valueMultiLine->SetMinSize( wxSize( 480,100 ) );

	m_textEntrySizer->Add( m_valueMultiLine, 1, wxBOTTOM|wxEXPAND|wxLEFT, 3 );

	m_labelCombo = new wxStaticText( this, wxID_ANY, _("Label:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCombo->Wrap( -1 );
	m_textEntrySizer->Add( m_labelCombo, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_valueCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER );
	m_valueCombo->SetMinSize( wxSize( 360,-1 ) );

	m_textEntrySizer->Add( m_valueCombo, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Text Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_textSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );

	wxBoxSizer* bSizeCtrlSizer;
	bSizeCtrlSizer = new wxBoxSizer( wxHORIZONTAL );

	m_textSizeCtrl = new wxTextCtrl( this, wxID_SIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizeCtrlSizer->Add( m_textSizeCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT, 3 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	bSizeCtrlSizer->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL, 2 );


	m_textEntrySizer->Add( bSizeCtrlSizer, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );

	m_labelOrientation = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelOrientation->Wrap( -1 );
	m_textEntrySizer->Add( m_labelOrientation, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_comboOrientation = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_textEntrySizer->Add( m_comboOrientation, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 3 );

	m_labelStyle = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelStyle->Wrap( -1 );
	m_textEntrySizer->Add( m_labelStyle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_comboStyle = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_textEntrySizer->Add( m_comboStyle, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 3 );

	m_labelShape = new wxStaticText( this, wxID_ANY, _("Shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelShape->Wrap( -1 );
	m_textEntrySizer->Add( m_labelShape, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_comboShape = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_textEntrySizer->Add( m_comboShape, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 3 );


	bMainSizer->Add( m_textEntrySizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 12 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bMainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_valueSingleLine->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnEnterKey ), NULL, this );
	m_valueCombo->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnEnterKey ), NULL, this );
}

DIALOG_LABEL_EDITOR_BASE::~DIALOG_LABEL_EDITOR_BASE()
{
	// Disconnect Events
	m_valueSingleLine->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnEnterKey ), NULL, this );
	m_valueCombo->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LABEL_EDITOR_BASE::OnEnterKey ), NULL, this );

}

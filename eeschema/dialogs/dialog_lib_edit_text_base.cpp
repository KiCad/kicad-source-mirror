///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_lib_edit_text_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_EDIT_TEXT_BASE::DIALOG_LIB_EDIT_TEXT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bPropertiesSizer;
	bPropertiesSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperBoxSizer;
	bUpperBoxSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bTextValueBoxSizer;
	bTextValueBoxSizer = new wxBoxSizer( wxHORIZONTAL );

	m_textLabel = new wxStaticText( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textLabel->Wrap( -1 );
	bTextValueBoxSizer->Add( m_textLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_TextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	bTextValueBoxSizer->Add( m_TextCtrl, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_StyledTextCtrl = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN, wxEmptyString );
	m_StyledTextCtrl->SetUseTabs( true );
	m_StyledTextCtrl->SetTabWidth( 4 );
	m_StyledTextCtrl->SetIndent( 4 );
	m_StyledTextCtrl->SetTabIndents( false );
	m_StyledTextCtrl->SetBackSpaceUnIndents( false );
	m_StyledTextCtrl->SetViewEOL( false );
	m_StyledTextCtrl->SetViewWhiteSpace( false );
	m_StyledTextCtrl->SetMarginWidth( 2, 0 );
	m_StyledTextCtrl->SetIndentationGuides( false );
	m_StyledTextCtrl->SetMarginWidth( 1, 0 );
	m_StyledTextCtrl->SetMarginWidth( 0, 0 );
	m_StyledTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_StyledTextCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_StyledTextCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_StyledTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_StyledTextCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_StyledTextCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_StyledTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_StyledTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_StyledTextCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_StyledTextCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_StyledTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_StyledTextCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_StyledTextCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_StyledTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_StyledTextCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_StyledTextCtrl->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_StyledTextCtrl->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	bTextValueBoxSizer->Add( m_StyledTextCtrl, 1, wxRIGHT|wxLEFT, 5 );

	m_PowerComponentValues = new wxStaticText( this, wxID_ANY, _("(Power symbol value field text cannot be changed.)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PowerComponentValues->Wrap( -1 );
	m_PowerComponentValues->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bTextValueBoxSizer->Add( m_PowerComponentValues, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_TextValueSelectButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bTextValueBoxSizer->Add( m_TextValueSelectButton, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bUpperBoxSizer->Add( bTextValueBoxSizer, 1, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bPropertiesSizer->Add( bUpperBoxSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	m_visible = new wxCheckBox( this, wxID_ANY, _("Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_visible, 0, wxALIGN_LEFT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bPropertiesSizer->Add( bSizer9, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 8, 3, 3 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_xPosLabel = new wxStaticText( this, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xPosLabel->Wrap( -1 );
	fgSizer3->Add( m_xPosLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_xPosCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_xPosCtrl, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

	m_xPosUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xPosUnits->Wrap( -1 );
	fgSizer3->Add( m_xPosUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer3->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_italic = new wxCheckBox( this, wxID_ANY, _("Italic"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_italic, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 8 );


	fgSizer3->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_orientLabel = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_orientLabel->Wrap( -1 );
	fgSizer3->Add( m_orientLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	wxString m_orientChoiceChoices[] = { _("Horizontal"), _("Vertical") };
	int m_orientChoiceNChoices = sizeof( m_orientChoiceChoices ) / sizeof( wxString );
	m_orientChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_orientChoiceNChoices, m_orientChoiceChoices, 0 );
	m_orientChoice->SetSelection( 0 );
	fgSizer3->Add( m_orientChoice, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_yPosLabel = new wxStaticText( this, wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yPosLabel->Wrap( -1 );
	fgSizer3->Add( m_yPosLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_yPosCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_yPosCtrl, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

	m_yPosUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yPosUnits->Wrap( -1 );
	fgSizer3->Add( m_yPosUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer3->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_bold = new wxCheckBox( this, wxID_ANY, _("Bold"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_bold, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 8 );


	fgSizer3->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_hAlignLabel = new wxStaticText( this, wxID_ANY, _("H Align:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hAlignLabel->Wrap( -1 );
	fgSizer3->Add( m_hAlignLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	wxString m_hAlignChoiceChoices[] = { _("Left"), _("Center"), _("Right") };
	int m_hAlignChoiceNChoices = sizeof( m_hAlignChoiceChoices ) / sizeof( wxString );
	m_hAlignChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_hAlignChoiceNChoices, m_hAlignChoiceChoices, 0 );
	m_hAlignChoice->SetSelection( 0 );
	fgSizer3->Add( m_hAlignChoice, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	fgSizer3->Add( m_textSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_textSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_textSizeCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	fgSizer3->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer3->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	fgSizer3->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_vAlignLabel = new wxStaticText( this, wxID_ANY, _("V Align:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vAlignLabel->Wrap( -1 );
	fgSizer3->Add( m_vAlignLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	wxString m_vAlignChoiceChoices[] = { _("Top"), _("Center"), _("Bottom") };
	int m_vAlignChoiceNChoices = sizeof( m_vAlignChoiceChoices ) / sizeof( wxString );
	m_vAlignChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_vAlignChoiceNChoices, m_vAlignChoiceChoices, 0 );
	m_vAlignChoice->SetSelection( 0 );
	fgSizer3->Add( m_vAlignChoice, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bPropertiesSizer->Add( fgSizer3, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bBottomtBoxSizer;
	bBottomtBoxSizer = new wxBoxSizer( wxVERTICAL );

	m_CommonUnit = new wxCheckBox( this, wxID_ANY, _("Common to all units"), wxDefaultPosition, wxDefaultSize, 0 );
	bBottomtBoxSizer->Add( m_CommonUnit, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_CommonConvert = new wxCheckBox( this, wxID_ANY, _("Common to all body styles"), wxDefaultPosition, wxDefaultSize, 0 );
	bBottomtBoxSizer->Add( m_CommonConvert, 0, wxLEFT|wxRIGHT, 5 );


	bPropertiesSizer->Add( bBottomtBoxSizer, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bMainSizer->Add( bPropertiesSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();

	bMainSizer->Add( m_sdbSizerButtons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnCloseDialog ) );
	m_TextCtrl->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnSetFocusText ), NULL, this );
	m_TextCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnTextEnter ), NULL, this );
	m_StyledTextCtrl->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnSetFocusText ), NULL, this );
	m_TextValueSelectButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnTextValueSelectButtonClick ), NULL, this );
}

DIALOG_LIB_EDIT_TEXT_BASE::~DIALOG_LIB_EDIT_TEXT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnCloseDialog ) );
	m_TextCtrl->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnSetFocusText ), NULL, this );
	m_TextCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnTextEnter ), NULL, this );
	m_StyledTextCtrl->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnSetFocusText ), NULL, this );
	m_TextValueSelectButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnTextValueSelectButtonClick ), NULL, this );

}

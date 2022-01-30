///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
#include "widgets/color_swatch.h"
#include "widgets/font_choice.h"

#include "dialog_lib_textbox_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_TEXTBOX_PROPERTIES_BASE::DIALOG_LIB_TEXTBOX_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_textEntrySizer = new wxGridBagSizer( 3, 0 );
	m_textEntrySizer->SetFlexibleDirection( wxBOTH );
	m_textEntrySizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textLabel = new wxStaticText( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_textLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxRIGHT, 5 );

	m_textCtrl = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN, wxEmptyString );
	m_textCtrl->SetUseTabs( true );
	m_textCtrl->SetTabWidth( 4 );
	m_textCtrl->SetIndent( 4 );
	m_textCtrl->SetTabIndents( false );
	m_textCtrl->SetBackSpaceUnIndents( false );
	m_textCtrl->SetViewEOL( false );
	m_textCtrl->SetViewWhiteSpace( false );
	m_textCtrl->SetMarginWidth( 2, 0 );
	m_textCtrl->SetIndentationGuides( false );
	m_textCtrl->SetMarginWidth( 1, 0 );
	m_textCtrl->SetMarginWidth( 0, 0 );
	m_textCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_textCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_textCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_textCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_textCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_textCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_textCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_textCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_textCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_textCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_textCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_textCtrl->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_textCtrl->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_textCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_textCtrl->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_textCtrl->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_textCtrl->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	m_textCtrl->SetMinSize( wxSize( 500,140 ) );

	m_textEntrySizer->Add( m_textCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 4 ), wxEXPAND, 5 );

	m_fontLabel = new wxStaticText( this, wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_fontLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	wxString m_fontCtrlChoices[] = { _("Default Font"), _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	m_textEntrySizer->Add( m_fontCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizeCtrlSizer;
	bSizeCtrlSizer = new wxBoxSizer( wxHORIZONTAL );

	m_separator1 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator1->Enable( false );

	bSizeCtrlSizer->Add( m_separator1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_bold = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_bold->SetToolTip( _("Bold") );

	bSizeCtrlSizer->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_italic = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_italic->SetToolTip( _("Italic") );

	bSizeCtrlSizer->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator2 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator2->Enable( false );

	bSizeCtrlSizer->Add( m_separator2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spin0 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_spin0->SetToolTip( _("Align right") );

	bSizeCtrlSizer->Add( m_spin0, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spin1 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_spin1->SetToolTip( _("Align bottom") );

	bSizeCtrlSizer->Add( m_spin1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spin2 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_spin2->SetToolTip( _("Align left") );

	bSizeCtrlSizer->Add( m_spin2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spin3 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_spin3->SetToolTip( _("Align top") );

	bSizeCtrlSizer->Add( m_spin3, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator3 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator3->Enable( false );

	bSizeCtrlSizer->Add( m_separator3, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_textEntrySizer->Add( bSizeCtrlSizer, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxVERTICAL );

	m_syntaxHelp = new wxHyperlinkCtrl( this, wxID_ANY, _("Syntax help"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_syntaxHelp->SetToolTip( _("Show syntax help window") );

	bSizer41->Add( m_syntaxHelp, 0, wxRIGHT|wxLEFT, 5 );


	m_textEntrySizer->Add( bSizer41, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_RIGHT|wxLEFT, 80 );

	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_textSizeLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_textSizeCtrl = new wxTextCtrl( this, wxID_SIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textEntrySizer->Add( m_textSizeCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	m_textEntrySizer->Add( m_textSizeUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	m_textEntrySizer->Add( 0, 15, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_borderWidthLabel = new wxStaticText( this, wxID_ANY, _("Border width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_borderWidthLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_borderWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_textEntrySizer->Add( m_borderWidthCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_borderWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthUnits->Wrap( -1 );
	m_textEntrySizer->Add( m_borderWidthUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_borderColorLabel = new wxStaticText( this, wxID_ANY, _("Border color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_borderColorLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_panelBorderColor = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_borderColorSwatch = new COLOR_SWATCH( m_panelBorderColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_borderColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelBorderColor->SetSizer( bSizer2 );
	m_panelBorderColor->Layout();
	bSizer2->Fit( m_panelBorderColor );
	m_textEntrySizer->Add( m_panelBorderColor, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_borderStyleLabel = new wxStaticText( this, wxID_ANY, _("Border style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderStyleLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_borderStyleLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_borderStyleCombo = new wxBitmapComboBox( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_borderStyleCombo->SetMinSize( wxSize( 240,-1 ) );

	m_textEntrySizer->Add( m_borderStyleCombo, wxGBPosition( 6, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );


	m_textEntrySizer->Add( 0, 15, wxGBPosition( 7, 0 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_filledCtrl = new wxCheckBox( this, wxID_ANY, _("Background fill"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textEntrySizer->Add( m_filledCtrl, wxGBPosition( 4, 3 ), wxGBSpan( 1, 2 ), wxLEFT, 100 );

	m_fillColorLabel = new wxStaticText( this, wxID_ANY, _("Fill color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fillColorLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_fillColorLabel, wxGBPosition( 5, 3 ), wxGBSpan( 1, 1 ), wxLEFT|wxALIGN_CENTER_VERTICAL, 100 );

	m_panelFillColor = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );

	m_fillColorSwatch = new COLOR_SWATCH( m_panelFillColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_fillColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelFillColor->SetSizer( bSizer21 );
	m_panelFillColor->Layout();
	bSizer21->Fit( m_panelFillColor );
	m_textEntrySizer->Add( m_panelFillColor, wxGBPosition( 5, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_privateCheckbox = new wxCheckBox( this, wxID_ANY, _("Private to symbol editor"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textEntrySizer->Add( m_privateCheckbox, wxGBPosition( 8, 0 ), wxGBSpan( 1, 2 ), 0, 5 );

	m_CommonUnit = new wxCheckBox( this, wxID_ANY, _("Common to all units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textEntrySizer->Add( m_CommonUnit, wxGBPosition( 8, 3 ), wxGBSpan( 1, 2 ), wxLEFT, 100 );

	m_CommonConvert = new wxCheckBox( this, wxID_ANY, _("Common to all body styles"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textEntrySizer->Add( m_CommonConvert, wxGBPosition( 9, 3 ), wxGBSpan( 1, 2 ), wxLEFT, 100 );


	m_textEntrySizer->AddGrowableCol( 1 );
	m_textEntrySizer->AddGrowableCol( 4 );

	bMainSizer->Add( m_textEntrySizer, 1, wxEXPAND|wxALL, 10 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

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
	m_textCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_LIB_TEXTBOX_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_syntaxHelp->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_LIB_TEXTBOX_PROPERTIES_BASE::OnFormattingHelp ), NULL, this );
}

DIALOG_LIB_TEXTBOX_PROPERTIES_BASE::~DIALOG_LIB_TEXTBOX_PROPERTIES_BASE()
{
	// Disconnect Events
	m_textCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_LIB_TEXTBOX_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_syntaxHelp->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_LIB_TEXTBOX_PROPERTIES_BASE::OnFormattingHelp ), NULL, this );

}

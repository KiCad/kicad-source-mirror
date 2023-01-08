///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
#include "widgets/color_swatch.h"
#include "widgets/font_choice.h"

#include "dialog_lib_text_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_TEXT_PROPERTIES_BASE::DIALOG_LIB_TEXT_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bPropertiesSizer;
	bPropertiesSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,10 ) );

	m_textLabel = new wxStaticText( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textLabel->Wrap( -1 );
	gbSizer1->Add( m_textLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxRIGHT|wxLEFT, 5 );

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
	m_StyledTextCtrl->SetReadOnly( false );
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
	gbSizer1->Add( m_StyledTextCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 4 ), wxEXPAND, 5 );

	m_fontLabel = new wxStaticText( this, wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	gbSizer1->Add( m_fontLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxRIGHT|wxLEFT, 5 );

	wxString m_fontCtrlChoices[] = { _("Default Font"), _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	gbSizer1->Add( m_fontCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	wxBoxSizer* formattingSizer;
	formattingSizer = new wxBoxSizer( wxHORIZONTAL );

	m_separator1 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator1->Enable( false );

	formattingSizer->Add( m_separator1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_horizontal = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_horizontal->SetToolTip( _("Horizontal text") );

	formattingSizer->Add( m_horizontal, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vertical = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_vertical->SetToolTip( _("Vertical text") );

	formattingSizer->Add( m_vertical, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator2 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator2->Enable( false );

	formattingSizer->Add( m_separator2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_bold = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_bold->SetToolTip( _("Bold") );

	formattingSizer->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_italic = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_italic->SetToolTip( _("Italic") );

	formattingSizer->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator3 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator3->Enable( false );

	formattingSizer->Add( m_separator3, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hAlignLeft = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_hAlignLeft->SetToolTip( _("Align left") );

	formattingSizer->Add( m_hAlignLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hAlignCenter = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_hAlignCenter->SetToolTip( _("Align horizontal center") );

	formattingSizer->Add( m_hAlignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hAlignRight = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_hAlignRight->SetToolTip( _("Align right") );

	formattingSizer->Add( m_hAlignRight, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator4 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator4->Enable( false );

	formattingSizer->Add( m_separator4, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignTop = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_vAlignTop->SetToolTip( _("Align top") );

	formattingSizer->Add( m_vAlignTop, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignCenter = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_vAlignCenter->SetToolTip( _("Align vertical center") );

	formattingSizer->Add( m_vAlignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignBottom = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_vAlignBottom->SetToolTip( _("Align bottom") );

	formattingSizer->Add( m_vAlignBottom, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator5 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator5->Enable( false );

	formattingSizer->Add( m_separator5, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( formattingSizer, wxGBPosition( 2, 3 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	gbSizer1->Add( m_textSizeLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer71;
	bSizer71 = new wxBoxSizer( wxHORIZONTAL );

	m_textSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer71->Add( m_textSizeCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	bSizer71->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_textColorLabel = new wxStaticText( this, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textColorLabel->Wrap( -1 );
	bSizer71->Add( m_textColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );


	bSizer71->Add( 5, 0, 0, 0, 5 );

	m_panelBorderColor1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer221;
	bSizer221 = new wxBoxSizer( wxVERTICAL );

	m_textColorSwatch = new COLOR_SWATCH( m_panelBorderColor1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer221->Add( m_textColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelBorderColor1->SetSizer( bSizer221 );
	m_panelBorderColor1->Layout();
	bSizer221->Fit( m_panelBorderColor1 );
	bSizer71->Add( m_panelBorderColor1, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( bSizer71, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_privateCheckbox = new wxCheckBox( this, wxID_ANY, _("Private to Symbol Editor"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_privateCheckbox, wxGBPosition( 5, 0 ), wxGBSpan( 1, 2 ), wxLEFT, 5 );

	m_CommonUnit = new wxCheckBox( this, wxID_ANY, _("Common to all units"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_CommonUnit, wxGBPosition( 5, 3 ), wxGBSpan( 1, 1 ), wxLEFT, 100 );

	m_CommonConvert = new wxCheckBox( this, wxID_ANY, _("Common to all body styles"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_CommonConvert, wxGBPosition( 6, 3 ), wxGBSpan( 1, 1 ), wxLEFT, 100 );


	bPropertiesSizer->Add( gbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 5, 4, 3, 3 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	bPropertiesSizer->Add( fgSizer3, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bBottomtBoxSizer;
	bBottomtBoxSizer = new wxBoxSizer( wxVERTICAL );


	bPropertiesSizer->Add( bBottomtBoxSizer, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bMainSizer->Add( bPropertiesSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

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
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_TEXT_PROPERTIES_BASE::OnCloseDialog ) );
	m_StyledTextCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_LIB_TEXT_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_StyledTextCtrl->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_LIB_TEXT_PROPERTIES_BASE::OnSetFocusText ), NULL, this );
}

DIALOG_LIB_TEXT_PROPERTIES_BASE::~DIALOG_LIB_TEXT_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_TEXT_PROPERTIES_BASE::OnCloseDialog ) );
	m_StyledTextCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_LIB_TEXT_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_StyledTextCtrl->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_LIB_TEXT_PROPERTIES_BASE::OnSetFocusText ), NULL, this );

}

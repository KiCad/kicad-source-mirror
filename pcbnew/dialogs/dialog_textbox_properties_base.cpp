///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"
#include "widgets/bitmap_button.h"
#include "widgets/font_choice.h"

#include "dialog_textbox_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TEXTBOX_PROPERTIES_BASE::DIALOG_TEXTBOX_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_MultiLineSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticText* textLabel;
	textLabel = new wxStaticText( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	textLabel->Wrap( -1 );
	m_MultiLineSizer->Add( textLabel, 0, wxRIGHT|wxLEFT, 5 );

	m_MultiLineText = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString );
	m_MultiLineText->SetUseTabs( true );
	m_MultiLineText->SetTabWidth( 4 );
	m_MultiLineText->SetIndent( 4 );
	m_MultiLineText->SetTabIndents( false );
	m_MultiLineText->SetBackSpaceUnIndents( false );
	m_MultiLineText->SetViewEOL( false );
	m_MultiLineText->SetViewWhiteSpace( false );
	m_MultiLineText->SetMarginWidth( 2, 0 );
	m_MultiLineText->SetIndentationGuides( true );
	m_MultiLineText->SetReadOnly( false );
	m_MultiLineText->SetMarginWidth( 1, 0 );
	m_MultiLineText->SetMarginWidth( 0, 0 );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_MultiLineText->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_MultiLineText->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_MultiLineText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_MultiLineText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_MultiLineText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_MultiLineText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_MultiLineText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_MultiLineText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_MultiLineText->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_MultiLineText->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	m_MultiLineText->SetMinSize( wxSize( -1,150 ) );

	m_MultiLineSizer->Add( m_MultiLineText, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( m_MultiLineSizer, 20, wxEXPAND|wxALL, 10 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbLocked = new wxCheckBox( this, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbLocked, wxGBPosition( 0, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_LayerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerLabel->Wrap( -1 );
	gbSizer1->Add( m_LayerLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_LayerSelectionCtrl = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_LayerSelectionCtrl->SetMinSize( wxSize( 175,-1 ) );

	gbSizer1->Add( m_LayerSelectionCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_fontLabel = new wxStaticText( this, wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	gbSizer1->Add( m_fontLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_fontCtrlChoices[] = { _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	gbSizer1->Add( m_fontCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizerButtonBar;
	bSizerButtonBar = new wxBoxSizer( wxHORIZONTAL );

	m_separator0 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator0->Enable( false );

	bSizerButtonBar->Add( m_separator0, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_bold = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_italic = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator1 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator1->Enable( false );

	bSizerButtonBar->Add( m_separator1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hAlignLeft = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_hAlignLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hAlignCenter = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_hAlignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hAlignRight = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_hAlignRight, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator2 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator2->Enable( false );

	bSizerButtonBar->Add( m_separator2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignTop = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_vAlignTop, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignCenter = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_vAlignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignBottom = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_vAlignBottom, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator3 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator3->Enable( false );

	bSizerButtonBar->Add( m_separator3, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_mirrored = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_mirrored, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator4 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator4->Enable( false );

	bSizerButtonBar->Add( m_separator4, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( bSizerButtonBar, wxGBPosition( 3, 4 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 8 );

	m_SizeXLabel = new wxStaticText( this, wxID_ANY, _("Text width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXLabel->Wrap( -1 );
	m_SizeXLabel->SetToolTip( _("Text width") );

	gbSizer1->Add( m_SizeXLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 4 );

	m_SizeXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_SizeXCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_SizeXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXUnits->Wrap( -1 );
	gbSizer1->Add( m_SizeXUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_borderCheckbox = new wxCheckBox( this, wxID_ANY, _("Border"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_borderCheckbox, wxGBPosition( 5, 4 ), wxGBSpan( 1, 2 ), wxLEFT|wxALIGN_CENTER_VERTICAL, 10 );

	m_SizeYLabel = new wxStaticText( this, wxID_ANY, _("Text height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYLabel->Wrap( -1 );
	m_SizeYLabel->SetToolTip( _("Text height") );

	gbSizer1->Add( m_SizeYLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 4 );

	m_SizeYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_SizeYCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_SizeYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYUnits->Wrap( -1 );
	gbSizer1->Add( m_SizeYUnits, wxGBPosition( 6, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_ThicknessLabel = new wxStaticText( this, wxID_ANY, _("Thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessLabel->Wrap( -1 );
	m_ThicknessLabel->SetToolTip( _("Text thickness") );

	gbSizer1->Add( m_ThicknessLabel, wxGBPosition( 7, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 4 );

	m_ThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_ThicknessCtrl, wxGBPosition( 7, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_ThicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessUnits->Wrap( -1 );
	bSizer5->Add( m_ThicknessUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_autoTextThickness = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_autoTextThickness->SetToolTip( _("Adjust the text thickness") );

	bSizer5->Add( m_autoTextThickness, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );


	gbSizer1->Add( bSizer5, wxGBPosition( 7, 2 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_OrientLabel = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OrientLabel->Wrap( -1 );
	m_OrientLabel->SetToolTip( _("Text orientation") );

	gbSizer1->Add( m_OrientLabel, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_OrientCtrl = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_OrientCtrl->Append( _("0.0") );
	m_OrientCtrl->Append( _("90.0") );
	m_OrientCtrl->Append( _("-90.0") );
	m_OrientCtrl->Append( _("180.0") );
	gbSizer1->Add( m_OrientCtrl, wxGBPosition( 1, 5 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_borderWidthLabel = new wxStaticText( this, wxID_ANY, _("Border width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_borderWidthLabel, wxGBPosition( 6, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_borderWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_borderWidthCtrl, wxGBPosition( 6, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_borderWidthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthUnits->Wrap( -1 );
	gbSizer1->Add( m_borderWidthUnits, wxGBPosition( 6, 6 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_borderStyleLabel = new wxStaticText( this, wxID_ANY, _("Border style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderStyleLabel->Wrap( -1 );
	gbSizer1->Add( m_borderStyleLabel, wxGBPosition( 7, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_borderStyleCombo = new wxBitmapComboBox( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_borderStyleCombo->SetMinSize( wxSize( 240,-1 ) );

	gbSizer1->Add( m_borderStyleCombo, wxGBPosition( 7, 5 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxEXPAND, 5 );


	gbSizer1->AddGrowableCol( 3 );

	bMainSizer->Add( gbSizer1, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxRIGHT|wxTOP, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::OnInitDlg ) );
	m_MultiLineText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_fontCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onFontSelected ), NULL, this );
	m_bold->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onBoldToggle ), NULL, this );
	m_hAlignLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onHalignButton ), NULL, this );
	m_hAlignCenter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onHalignButton ), NULL, this );
	m_hAlignRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onHalignButton ), NULL, this );
	m_vAlignTop->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_vAlignCenter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_vAlignBottom->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_SizeXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onTextSize ), NULL, this );
	m_SizeXCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_borderCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_SizeYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onTextSize ), NULL, this );
	m_SizeYCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_ThicknessCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onThickness ), NULL, this );
	m_autoTextThickness->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onAutoTextThickness ), NULL, this );
	m_OrientCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::OnOkClick ), NULL, this );
}

DIALOG_TEXTBOX_PROPERTIES_BASE::~DIALOG_TEXTBOX_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::OnInitDlg ) );
	m_MultiLineText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_fontCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onFontSelected ), NULL, this );
	m_bold->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onBoldToggle ), NULL, this );
	m_hAlignLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onHalignButton ), NULL, this );
	m_hAlignCenter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onHalignButton ), NULL, this );
	m_hAlignRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onHalignButton ), NULL, this );
	m_vAlignTop->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_vAlignCenter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_vAlignBottom->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_SizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onTextSize ), NULL, this );
	m_SizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_borderCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_SizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onTextSize ), NULL, this );
	m_SizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_ThicknessCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onThickness ), NULL, this );
	m_autoTextThickness->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::onAutoTextThickness ), NULL, this );
	m_OrientCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXTBOX_PROPERTIES_BASE::OnOkClick ), NULL, this );

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"
#include "widgets/bitmap_button.h"
#include "widgets/font_choice.h"

#include "dialog_text_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TEXT_PROPERTIES_BASE::DIALOG_TEXT_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
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
	m_MultiLineSizer->Add( m_MultiLineText, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( m_MultiLineSizer, 20, wxEXPAND|wxALL, 10 );

	m_SingleLineSizer = new wxBoxSizer( wxHORIZONTAL );

	m_TextLabel = new wxStaticText( this, wxID_ANY, _("Reference designator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextLabel->Wrap( -1 );
	m_SingleLineSizer->Add( m_TextLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_SingleLineText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_SingleLineSizer->Add( m_SingleLineText, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	bMainSizer->Add( m_SingleLineSizer, 0, wxEXPAND|wxALL, 10 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 2, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( 20,-1 ) );

	m_cbLocked = new wxCheckBox( this, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbLocked, wxGBPosition( 0, 0 ), wxGBSpan( 1, 3 ), wxRIGHT|wxLEFT, 5 );

	m_LayerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerLabel->Wrap( -1 );
	gbSizer1->Add( m_LayerLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_LayerSelectionCtrl = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizer1->Add( m_LayerSelectionCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_cbKnockout = new wxCheckBox( this, wxID_ANY, _("Knockout"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_cbKnockout, 1, wxALL, 15 );


	bSizer7->Add( 20, 0, 1, wxEXPAND, 5 );

	m_KeepUpright = new wxCheckBox( this, wxID_ANY, _("Keep upright"), wxDefaultPosition, wxDefaultSize, 0 );
	m_KeepUpright->SetToolTip( _("Keep text upright") );

	bSizer7->Add( m_KeepUpright, 0, wxALIGN_CENTER_VERTICAL, 10 );


	bSizer7->Add( 20, 0, 1, wxEXPAND, 5 );

	m_Visible = new wxCheckBox( this, wxID_ANY, _("Show"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_Visible, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 20 );


	gbSizer1->Add( bSizer7, wxGBPosition( 1, 4 ), wxGBSpan( 1, 3 ), wxEXPAND|wxTOP|wxBOTTOM, 10 );

	m_fontLabel = new wxStaticText( this, wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	gbSizer1->Add( m_fontLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_fontCtrlChoices[] = { _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	gbSizer1->Add( m_fontCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bSizerButtonBar;
	bSizerButtonBar = new wxBoxSizer( wxHORIZONTAL );

	m_bold = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_italic = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator1 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator1->Enable( false );

	bSizerButtonBar->Add( m_separator1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_alignLeft = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_alignLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_alignCenter = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_alignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_alignRight = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_alignRight, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator2 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator2->Enable( false );

	bSizerButtonBar->Add( m_separator2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_valignTop = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_valignTop, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_valignCenter = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_valignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_valignBottom = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_valignBottom, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator3 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator3->Enable( false );

	bSizerButtonBar->Add( m_separator3, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_mirrored = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	bSizerButtonBar->Add( m_mirrored, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( bSizerButtonBar, wxGBPosition( 2, 4 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxEXPAND|wxTOP, 3 );

	m_SizeXLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXLabel->Wrap( -1 );
	m_SizeXLabel->SetToolTip( _("Text width") );

	gbSizer1->Add( m_SizeXLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_SizeXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_SizeXCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_SizeXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXUnits->Wrap( -1 );
	gbSizer1->Add( m_SizeXUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_SizeYLabel = new wxStaticText( this, wxID_ANY, _("Height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYLabel->Wrap( -1 );
	m_SizeYLabel->SetToolTip( _("Text height") );

	gbSizer1->Add( m_SizeYLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_SizeYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_SizeYCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_SizeYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYUnits->Wrap( -1 );
	gbSizer1->Add( m_SizeYUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	m_ThicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessUnits->Wrap( -1 );
	bSizer8->Add( m_ThicknessUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_autoTextThickness = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_autoTextThickness->SetToolTip( _("Adjust the text thickness") );

	bSizer8->Add( m_autoTextThickness, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );


	gbSizer1->Add( bSizer8, wxGBPosition( 5, 2 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_PositionXLabel = new wxStaticText( this, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PositionXLabel->Wrap( -1 );
	m_PositionXLabel->SetToolTip( _("Text pos X") );

	gbSizer1->Add( m_PositionXLabel, wxGBPosition( 3, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_PositionXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_PositionXCtrl, wxGBPosition( 3, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_PositionXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PositionXUnits->Wrap( -1 );
	gbSizer1->Add( m_PositionXUnits, wxGBPosition( 3, 6 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_PositionYLabel = new wxStaticText( this, wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PositionYLabel->Wrap( -1 );
	m_PositionYLabel->SetToolTip( _("Text pos Y") );

	gbSizer1->Add( m_PositionYLabel, wxGBPosition( 4, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_PositionYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_PositionYCtrl, wxGBPosition( 4, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_PositionYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PositionYUnits->Wrap( -1 );
	gbSizer1->Add( m_PositionYUnits, wxGBPosition( 4, 6 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_OrientLabel = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OrientLabel->Wrap( -1 );
	m_OrientLabel->SetToolTip( _("Text orientation") );

	gbSizer1->Add( m_OrientLabel, wxGBPosition( 5, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_OrientCtrl = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_OrientCtrl->Append( _("0.0") );
	m_OrientCtrl->Append( _("90.0") );
	m_OrientCtrl->Append( _("-90.0") );
	m_OrientCtrl->Append( _("180.0") );
	gbSizer1->Add( m_OrientCtrl, wxGBPosition( 5, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_ThicknessLabel = new wxStaticText( this, wxID_ANY, _("Thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessLabel->Wrap( -1 );
	m_ThicknessLabel->SetToolTip( _("Text thickness") );

	gbSizer1->Add( m_ThicknessLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	gbSizer1->Add( m_ThicknessCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );


	gbSizer1->AddGrowableCol( 1 );
	gbSizer1->AddGrowableCol( 5 );

	bMainSizer->Add( gbSizer1, 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );


	bMainSizer->Add( 0, 0, 0, wxTOP, 5 );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_statusLine = new wxStaticText( this, wxID_ANY, _("Parent footprint description"), wxDefaultPosition, wxDefaultSize, 0 );
	m_statusLine->Wrap( -1 );
	bMargins->Add( m_statusLine, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 3 );


	bMainSizer->Add( bMargins, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 12 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnInitDlg ) );
	m_MultiLineText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_SingleLineText->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnSetFocusText ), NULL, this );
	m_SingleLineText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_fontCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onFontSelected ), NULL, this );
	m_bold->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onBoldToggle ), NULL, this );
	m_alignLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_alignCenter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_alignRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_valignTop->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_valignCenter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_valignBottom->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_SizeXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onTextSize ), NULL, this );
	m_SizeXCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_SizeYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onTextSize ), NULL, this );
	m_SizeYCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_autoTextThickness->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onAutoTextThickness ), NULL, this );
	m_PositionXCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_PositionYCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_OrientCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_ThicknessCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onThickness ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
}

DIALOG_TEXT_PROPERTIES_BASE::~DIALOG_TEXT_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnInitDlg ) );
	m_MultiLineText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_SingleLineText->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnSetFocusText ), NULL, this );
	m_SingleLineText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_fontCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onFontSelected ), NULL, this );
	m_bold->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onBoldToggle ), NULL, this );
	m_alignLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_alignCenter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_alignRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onAlignButton ), NULL, this );
	m_valignTop->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_valignCenter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_valignBottom->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onValignButton ), NULL, this );
	m_SizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onTextSize ), NULL, this );
	m_SizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_SizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onTextSize ), NULL, this );
	m_SizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_autoTextThickness->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onAutoTextThickness ), NULL, this );
	m_PositionXCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_PositionYCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_OrientCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_ThicknessCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::onThickness ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );

}

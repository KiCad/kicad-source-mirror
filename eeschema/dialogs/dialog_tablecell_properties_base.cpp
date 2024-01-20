///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
#include "widgets/color_swatch.h"
#include "widgets/font_choice.h"
#include "widgets/wx_infobar.h"

#include "dialog_tablecell_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TABLECELL_PROPERTIES_BASE::DIALOG_TABLECELL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	bMainSizer->Add( m_infoBar, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bColumns;
	bColumns = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bCellContentMargins;
	bCellContentMargins = new wxBoxSizer( wxVERTICAL );

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
	m_textCtrl->SetReadOnly( false );
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
	m_textCtrl->SetMinSize( wxSize( 500,-1 ) );

	bCellContentMargins->Add( m_textCtrl, 1, wxEXPAND|wxTOP|wxBOTTOM, 1 );


	bColumns->Add( bCellContentMargins, 1, wxEXPAND|wxTOP, 6 );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_tablePage = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );

	m_textEntrySizer = new wxGridBagSizer( 3, 3 );
	m_textEntrySizer->SetFlexibleDirection( wxBOTH );
	m_textEntrySizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	m_textEntrySizer->SetEmptyCellSize( wxSize( 0,2 ) );

	m_borderCheckbox = new wxCheckBox( m_tablePage, wxID_ANY, _("External border"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textEntrySizer->Add( m_borderCheckbox, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM, 2 );

	m_headerBorder = new wxCheckBox( m_tablePage, wxID_ANY, _("Header border"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textEntrySizer->Add( m_headerBorder, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxLEFT, 20 );

	m_borderWidthLabel = new wxStaticText( m_tablePage, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_borderWidthLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_borderWidthCtrl = new wxTextCtrl( m_tablePage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer7->Add( m_borderWidthCtrl, 0, wxEXPAND, 5 );

	m_borderWidthUnits = new wxStaticText( m_tablePage, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthUnits->Wrap( -1 );
	bSizer7->Add( m_borderWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_borderColorLabel = new wxStaticText( m_tablePage, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorLabel->Wrap( -1 );
	bSizer7->Add( m_borderColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );


	bSizer7->Add( 5, 0, 0, 0, 5 );

	m_panelBorderColor = new wxPanel( m_tablePage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_borderColorSwatch = new COLOR_SWATCH( m_panelBorderColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_borderColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelBorderColor->SetSizer( bSizer2 );
	m_panelBorderColor->Layout();
	bSizer2->Fit( m_panelBorderColor );
	bSizer7->Add( m_panelBorderColor, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_textEntrySizer->Add( bSizer7, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_borderStyleLabel = new wxStaticText( m_tablePage, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderStyleLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_borderStyleLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_borderStyleCombo = new wxBitmapComboBox( m_tablePage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_borderStyleCombo->SetMinSize( wxSize( 200,-1 ) );

	m_textEntrySizer->Add( m_borderStyleCombo, wxGBPosition( 3, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );


	m_textEntrySizer->Add( 0, 20, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_rowSeparators = new wxCheckBox( m_tablePage, wxID_ANY, _("Row lines"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textEntrySizer->Add( m_rowSeparators, wxGBPosition( 5, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 15 );

	m_colSeparators = new wxCheckBox( m_tablePage, wxID_ANY, _("Column lines"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textEntrySizer->Add( m_colSeparators, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 20 );

	m_separatorsWidthLabel = new wxStaticText( m_tablePage, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsWidthLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_separatorsWidthLabel, wxGBPosition( 7, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer71;
	bSizer71 = new wxBoxSizer( wxHORIZONTAL );

	m_separatorsWidthCtrl = new wxTextCtrl( m_tablePage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer71->Add( m_separatorsWidthCtrl, 0, wxEXPAND, 5 );

	m_separatorsWidthUnits = new wxStaticText( m_tablePage, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsWidthUnits->Wrap( -1 );
	bSizer71->Add( m_separatorsWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_separatorsColorLabel = new wxStaticText( m_tablePage, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsColorLabel->Wrap( -1 );
	bSizer71->Add( m_separatorsColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );


	bSizer71->Add( 5, 0, 0, 0, 5 );

	m_panelSeparatorsColor = new wxPanel( m_tablePage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );

	m_separatorsColorSwatch = new COLOR_SWATCH( m_panelSeparatorsColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_separatorsColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelSeparatorsColor->SetSizer( bSizer21 );
	m_panelSeparatorsColor->Layout();
	bSizer21->Fit( m_panelSeparatorsColor );
	bSizer71->Add( m_panelSeparatorsColor, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_textEntrySizer->Add( bSizer71, wxGBPosition( 7, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_separatorsStyleLabel = new wxStaticText( m_tablePage, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsStyleLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_separatorsStyleLabel, wxGBPosition( 8, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_separatorsStyleCombo = new wxBitmapComboBox( m_tablePage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_separatorsStyleCombo->SetMinSize( wxSize( 200,-1 ) );

	m_textEntrySizer->Add( m_separatorsStyleCombo, wxGBPosition( 8, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	m_textEntrySizer->AddGrowableCol( 1 );

	bSizer16->Add( m_textEntrySizer, 1, wxEXPAND|wxALL, 5 );


	m_tablePage->SetSizer( bSizer16 );
	m_tablePage->Layout();
	bSizer16->Fit( m_tablePage );
	m_notebook->AddPage( m_tablePage, _("Table"), false );
	m_cellPage = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizeCtrlSizer;
	bSizeCtrlSizer = new wxBoxSizer( wxHORIZONTAL );

	m_separator1 = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator1->Enable( false );

	bSizeCtrlSizer->Add( m_separator1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_bold = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_bold->SetToolTip( _("Bold") );

	bSizeCtrlSizer->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_italic = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_italic->SetToolTip( _("Italic") );

	bSizeCtrlSizer->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator2 = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator2->Enable( false );

	bSizeCtrlSizer->Add( m_separator2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hAlignLeft = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_hAlignLeft->SetToolTip( _("Align left") );

	bSizeCtrlSizer->Add( m_hAlignLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hAlignCenter = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_hAlignCenter->SetToolTip( _("Align horizontal center") );

	bSizeCtrlSizer->Add( m_hAlignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hAlignRight = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_hAlignRight->SetToolTip( _("Align right") );

	bSizeCtrlSizer->Add( m_hAlignRight, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator3 = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator3->Enable( false );

	bSizeCtrlSizer->Add( m_separator3, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignTop = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_vAlignTop->SetToolTip( _("Align top") );

	bSizeCtrlSizer->Add( m_vAlignTop, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignCenter = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_vAlignCenter->SetToolTip( _("Align vertical center") );

	bSizeCtrlSizer->Add( m_vAlignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignBottom = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_vAlignBottom->SetToolTip( _("Align bottom") );

	bSizeCtrlSizer->Add( m_vAlignBottom, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator4 = new BITMAP_BUTTON( m_cellPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator4->Enable( false );

	bSizeCtrlSizer->Add( m_separator4, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bMargins->Add( bSizeCtrlSizer, 0, wxBOTTOM, 5 );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 4, 5 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer2->SetEmptyCellSize( wxSize( -1,5 ) );

	m_fontLabel = new wxStaticText( m_cellPage, wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	gbSizer2->Add( m_fontLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	wxString m_fontCtrlChoices[] = { _("Default Font"), _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( m_cellPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	gbSizer2->Add( m_fontCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_textSizeLabel = new wxStaticText( m_cellPage, wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	gbSizer2->Add( m_textSizeLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxHORIZONTAL );

	m_textSizeCtrl = new wxTextCtrl( m_cellPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer15->Add( m_textSizeCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textSizeUnits = new wxStaticText( m_cellPage, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	bSizer15->Add( m_textSizeUnits, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );


	gbSizer2->Add( bSizer15, wxGBPosition( 1, 1 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer2->AddGrowableCol( 1 );

	bMargins->Add( gbSizer2, 0, wxEXPAND|wxBOTTOM, 5 );


	bMargins->Add( 0, 5, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 6, 5 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textColorLabel = new wxStaticText( m_cellPage, wxID_ANY, _("Text color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textColorLabel->Wrap( -1 );
	fgSizer1->Add( m_textColorLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_panelTextColor = new wxPanel( m_cellPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer221;
	bSizer221 = new wxBoxSizer( wxVERTICAL );

	m_textColorSwatch = new COLOR_SWATCH( m_panelTextColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer221->Add( m_textColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelTextColor->SetSizer( bSizer221 );
	m_panelTextColor->Layout();
	bSizer221->Fit( m_panelTextColor );
	fgSizer1->Add( m_panelTextColor, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_fillColorLabel = new wxStaticText( m_cellPage, wxID_ANY, _("Background fill:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fillColorLabel->Wrap( -1 );
	fgSizer1->Add( m_fillColorLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_panelFillColor = new wxPanel( m_cellPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxVERTICAL );

	m_fillColorSwatch = new COLOR_SWATCH( m_panelFillColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer22->Add( m_fillColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelFillColor->SetSizer( bSizer22 );
	m_panelFillColor->Layout();
	bSizer22->Fit( m_panelFillColor );
	fgSizer1->Add( m_panelFillColor, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bMargins->Add( fgSizer1, 0, wxEXPAND|wxTOP, 5 );


	bMargins->Add( 0, 10, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbSizer3;
	gbSizer3 = new wxGridBagSizer( 1, 0 );
	gbSizer3->SetFlexibleDirection( wxBOTH );
	gbSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_marginsLable = new wxStaticText( m_cellPage, wxID_ANY, _("Margins:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginsLable->Wrap( -1 );
	gbSizer3->Add( m_marginsLable, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxRIGHT|wxALIGN_CENTER_VERTICAL, 35 );

	wxBoxSizer* marginTopSizer;
	marginTopSizer = new wxBoxSizer( wxHORIZONTAL );

	m_marginTopCtrl = new wxTextCtrl( m_cellPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	marginTopSizer->Add( m_marginTopCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_marginTopUnits = new wxStaticText( m_cellPage, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginTopUnits->Wrap( -1 );
	marginTopSizer->Add( m_marginTopUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );


	gbSizer3->Add( marginTopSizer, wxGBPosition( 0, 2 ), wxGBSpan( 1, 4 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* marginLeftSizer;
	marginLeftSizer = new wxBoxSizer( wxHORIZONTAL );

	m_marginLeftCtrl = new wxTextCtrl( m_cellPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	marginLeftSizer->Add( m_marginLeftCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer3->Add( marginLeftSizer, wxGBPosition( 1, 0 ), wxGBSpan( 1, 3 ), wxEXPAND|wxRIGHT|wxLEFT, 25 );

	wxBoxSizer* marginRightSizer;
	marginRightSizer = new wxBoxSizer( wxHORIZONTAL );

	m_marginRightCtrl = new wxTextCtrl( m_cellPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	marginRightSizer->Add( m_marginRightCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer3->Add( marginRightSizer, wxGBPosition( 1, 3 ), wxGBSpan( 1, 3 ), wxEXPAND, 5 );

	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxHORIZONTAL );

	m_marginBottomCtrl = new wxTextCtrl( m_cellPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer19->Add( m_marginBottomCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer3->Add( bSizer19, wxGBPosition( 2, 2 ), wxGBSpan( 1, 4 ), wxEXPAND, 5 );


	bMargins->Add( gbSizer3, 1, wxEXPAND|wxTOP, 5 );


	bSizer13->Add( bMargins, 1, wxEXPAND|wxALL, 5 );


	m_cellPage->SetSizer( bSizer13 );
	m_cellPage->Layout();
	bSizer13->Fit( m_cellPage );
	m_notebook->AddPage( m_cellPage, _("Cell"), true );

	bColumns->Add( m_notebook, 0, wxEXPAND|wxBOTTOM|wxLEFT, 10 );


	bMainSizer->Add( bColumns, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bButtons;
	bButtons = new wxBoxSizer( wxHORIZONTAL );


	bButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_applyButton = new wxButton( this, wxID_ANY, _("Apply && Go to Next Cell"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_applyButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_hotkeyHint = new wxStaticText( this, wxID_ANY, _("(Option+Tab)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hotkeyHint->Wrap( -1 );
	bButtons->Add( m_hotkeyHint, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bButtons->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bButtons, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_textCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_borderCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_rowSeparators->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_colSeparators->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_applyButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::OnApply ), NULL, this );
}

DIALOG_TABLECELL_PROPERTIES_BASE::~DIALOG_TABLECELL_PROPERTIES_BASE()
{
	// Disconnect Events
	m_textCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onMultiLineTCLostFocus ), NULL, this );
	m_borderCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_rowSeparators->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_colSeparators->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_applyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TABLECELL_PROPERTIES_BASE::OnApply ), NULL, this );

}

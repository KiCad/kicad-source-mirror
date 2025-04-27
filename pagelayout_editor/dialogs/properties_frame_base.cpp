///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
#include "widgets/color_swatch.h"
#include "widgets/font_choice.h"

#include "properties_frame_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PROPERTIES_BASE::PANEL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerpanel;
	bSizerpanel = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_swItemProperties = new wxScrolledWindow( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTAB_TRAVERSAL|wxVSCROLL );
	m_swItemProperties->SetScrollRate( 5, 5 );
	m_SizerItemProperties = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerButt;
	bSizerButt = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextType = new wxStaticText( m_swItemProperties, wxID_ANY, _("Item Type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextType->Wrap( -1 );
	m_staticTextType->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizerButt->Add( m_staticTextType, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 3 );

	m_syntaxHelpLink = new wxHyperlinkCtrl( m_swItemProperties, wxID_ANY, _("Syntax Help"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	bSizerButt->Add( m_syntaxHelpLink, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choicePageOptChoices[] = { _("Show on all pages"), _("First page only"), _("Subsequent pages only") };
	int m_choicePageOptNChoices = sizeof( m_choicePageOptChoices ) / sizeof( wxString );
	m_choicePageOpt = new wxChoice( m_swItemProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePageOptNChoices, m_choicePageOptChoices, 0 );
	m_choicePageOpt->SetSelection( 2 );
	bSizerButt->Add( m_choicePageOpt, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	m_SizerItemProperties->Add( bSizerButt, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	m_SizerTextOptions = new wxBoxSizer( wxVERTICAL );

	m_stcText = new wxStyledTextCtrl( m_swItemProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString );
	m_stcText->SetUseTabs( true );
	m_stcText->SetTabWidth( 4 );
	m_stcText->SetIndent( 4 );
	m_stcText->SetTabIndents( true );
	m_stcText->SetBackSpaceUnIndents( true );
	m_stcText->SetViewEOL( false );
	m_stcText->SetViewWhiteSpace( false );
	m_stcText->SetMarginWidth( 2, 0 );
	m_stcText->SetIndentationGuides( false );
	m_stcText->SetReadOnly( false );
	m_stcText->SetMarginWidth( 1, 0 );
	m_stcText->SetMarginWidth( 0, 0 );
	m_stcText->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_stcText->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_stcText->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_stcText->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_stcText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_stcText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_stcText->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_stcText->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_stcText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_stcText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_stcText->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_stcText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_stcText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_stcText->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_stcText->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_stcText->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_stcText->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	m_SizerTextOptions->Add( m_stcText, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerFontOpt;
	bSizerFontOpt = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer11;
	gbSizer11 = new wxGridBagSizer( 3, 5 );
	gbSizer11->SetFlexibleDirection( wxBOTH );
	gbSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxBoxSizer* formattingSizer;
	formattingSizer = new wxBoxSizer( wxHORIZONTAL );

	m_bold = new BITMAP_BUTTON( m_swItemProperties, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_bold->SetToolTip( _("Bold") );

	formattingSizer->Add( m_bold, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_italic = new BITMAP_BUTTON( m_swItemProperties, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_italic->SetToolTip( _("Italic") );

	formattingSizer->Add( m_italic, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator2 = new BITMAP_BUTTON( m_swItemProperties, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator2->Enable( false );

	formattingSizer->Add( m_separator2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_alignLeft = new BITMAP_BUTTON( m_swItemProperties, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	formattingSizer->Add( m_alignLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_alignCenter = new BITMAP_BUTTON( m_swItemProperties, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	formattingSizer->Add( m_alignCenter, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_alignRight = new BITMAP_BUTTON( m_swItemProperties, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	formattingSizer->Add( m_alignRight, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator3 = new BITMAP_BUTTON( m_swItemProperties, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator3->Enable( false );

	formattingSizer->Add( m_separator3, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignTop = new BITMAP_BUTTON( m_swItemProperties, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	formattingSizer->Add( m_vAlignTop, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignMiddle = new BITMAP_BUTTON( m_swItemProperties, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	formattingSizer->Add( m_vAlignMiddle, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vAlignBottom = new BITMAP_BUTTON( m_swItemProperties, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	formattingSizer->Add( m_vAlignBottom, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_separator4 = new BITMAP_BUTTON( m_swItemProperties, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator4->Enable( false );

	formattingSizer->Add( m_separator4, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_panelBorderColor1 = new wxPanel( m_swItemProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxVERTICAL );

	m_textColorSwatch = new COLOR_SWATCH( m_panelBorderColor1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer22->Add( m_textColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelBorderColor1->SetSizer( bSizer22 );
	m_panelBorderColor1->Layout();
	bSizer22->Fit( m_panelBorderColor1 );
	formattingSizer->Add( m_panelBorderColor1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	gbSizer11->Add( formattingSizer, wxGBPosition( 0, 0 ), wxGBSpan( 1, 4 ), wxTOP|wxBOTTOM|wxEXPAND, 5 );

	m_fontLabel = new wxStaticText( m_swItemProperties, wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fontLabel->Wrap( -1 );
	gbSizer11->Add( m_fontLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_fontCtrlChoices[] = { _("Default Font"), _("KiCad Font") };
	int m_fontCtrlNChoices = sizeof( m_fontCtrlChoices ) / sizeof( wxString );
	m_fontCtrl = new FONT_CHOICE( m_swItemProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fontCtrlNChoices, m_fontCtrlChoices, 0 );
	m_fontCtrl->SetSelection( 0 );
	gbSizer11->Add( m_fontCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextTsizeX = new wxStaticText( m_swItemProperties, wxID_ANY, _("Text width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTsizeX->Wrap( -1 );
	gbSizer11->Add( m_staticTextTsizeX, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlTextSizeX = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer11->Add( m_textCtrlTextSizeX, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_textSizeXUnits = new wxStaticText( m_swItemProperties, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeXUnits->Wrap( -1 );
	gbSizer11->Add( m_textSizeXUnits, wxGBPosition( 2, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextTsizeY = new wxStaticText( m_swItemProperties, wxID_ANY, _("Text height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTsizeY->Wrap( -1 );
	gbSizer11->Add( m_staticTextTsizeY, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlTextSizeY = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer11->Add( m_textCtrlTextSizeY, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_textSizeYUnits = new wxStaticText( m_swItemProperties, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeYUnits->Wrap( -1 );
	gbSizer11->Add( m_textSizeYUnits, wxGBPosition( 3, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_constraintXLabel = new wxStaticText( m_swItemProperties, wxID_ANY, _("Maximum width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_constraintXLabel->Wrap( -1 );
	m_constraintXLabel->SetToolTip( _("Set to 0 to disable this constraint") );

	gbSizer11->Add( m_constraintXLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_constraintXCtrl = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer11->Add( m_constraintXCtrl, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_constraintXUnits = new wxStaticText( m_swItemProperties, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_constraintXUnits->Wrap( -1 );
	gbSizer11->Add( m_constraintXUnits, wxGBPosition( 4, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_constraintYLabel = new wxStaticText( m_swItemProperties, wxID_ANY, _("Maximum height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_constraintYLabel->Wrap( -1 );
	m_constraintYLabel->SetToolTip( _("Set to 0 to disable this constraint") );

	gbSizer11->Add( m_constraintYLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_constraintYCtrl = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer11->Add( m_constraintYCtrl, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_constraintYUnits = new wxStaticText( m_swItemProperties, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_constraintYUnits->Wrap( -1 );
	gbSizer11->Add( m_constraintYUnits, wxGBPosition( 5, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer11->AddGrowableCol( 2 );

	bSizerFontOpt->Add( gbSizer11, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_SizerTextOptions->Add( bSizerFontOpt, 0, wxEXPAND|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	m_SizerTextOptions->Add( fgSizer2, 0, wxEXPAND|wxBOTTOM, 3 );

	m_staticTextSizeInfo = new wxStaticText( m_swItemProperties, wxID_ANY, _("Set to 0 to use default values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeInfo->Wrap( -1 );
	m_SizerTextOptions->Add( m_staticTextSizeInfo, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_SizerItemProperties->Add( m_SizerTextOptions, 1, wxEXPAND, 5 );

	m_staticTextComment = new wxStaticText( m_swItemProperties, wxID_ANY, _("Comment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextComment->Wrap( -1 );
	m_SizerItemProperties->Add( m_staticTextComment, 0, wxRIGHT|wxLEFT, 5 );

	m_textCtrlComment = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SizerItemProperties->Add( m_textCtrlComment, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	sbSizerPos = new wxStaticBoxSizer( new wxStaticBox( m_swItemProperties, wxID_ANY, _("Position") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextPosX = new wxStaticText( sbSizerPos->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosX->Wrap( -1 );
	fgSizer3->Add( m_staticTextPosX, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	m_textCtrlPosX = new wxTextCtrl( sbSizerPos->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_textCtrlPosX, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_TextPosXUnits = new wxStaticText( sbSizerPos->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPosXUnits->Wrap( -1 );
	fgSizer3->Add( m_TextPosXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxRIGHT, 5 );

	m_staticTextPosY = new wxStaticText( sbSizerPos->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosY->Wrap( -1 );
	fgSizer3->Add( m_staticTextPosY, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	m_textCtrlPosY = new wxTextCtrl( sbSizerPos->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_textCtrlPosY, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_TextPosYUnits = new wxStaticText( sbSizerPos->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPosYUnits->Wrap( -1 );
	fgSizer3->Add( m_TextPosYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxRIGHT, 5 );

	m_staticTextOrgPos = new wxStaticText( sbSizerPos->GetStaticBox(), wxID_ANY, _("From:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOrgPos->Wrap( -1 );
	fgSizer3->Add( m_staticTextOrgPos, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	m_comboBoxCornerPos = new wxComboBox( sbSizerPos->GetStaticBox(), wxID_ANY, _("Lower Right"), wxDefaultPosition, wxSize( -1,-1 ), 0, NULL, 0 );
	m_comboBoxCornerPos->Append( _("Upper Right") );
	m_comboBoxCornerPos->Append( _("Upper Left") );
	m_comboBoxCornerPos->Append( _("Lower Right") );
	m_comboBoxCornerPos->Append( _("Lower Left") );
	m_comboBoxCornerPos->SetSelection( 2 );
	m_comboBoxCornerPos->SetMinSize( wxSize( 132,-1 ) );

	fgSizer3->Add( m_comboBoxCornerPos, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	sbSizerPos->Add( fgSizer3, 0, wxEXPAND|wxBOTTOM, 5 );


	m_SizerItemProperties->Add( sbSizerPos, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_sbSizerEndPosition = new wxStaticBoxSizer( new wxStaticBox( m_swItemProperties, wxID_ANY, _("End Position") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer4->AddGrowableCol( 1 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextEndX = new wxStaticText( m_sbSizerEndPosition->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEndX->Wrap( -1 );
	fgSizer4->Add( m_staticTextEndX, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	m_textCtrlEndX = new wxTextCtrl( m_sbSizerEndPosition->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer4->Add( m_textCtrlEndX, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_TextEndXUnits = new wxStaticText( m_sbSizerEndPosition->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextEndXUnits->Wrap( -1 );
	fgSizer4->Add( m_TextEndXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxRIGHT, 5 );

	m_staticTextEndY = new wxStaticText( m_sbSizerEndPosition->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEndY->Wrap( -1 );
	fgSizer4->Add( m_staticTextEndY, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	m_textCtrlEndY = new wxTextCtrl( m_sbSizerEndPosition->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer4->Add( m_textCtrlEndY, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_TextEndYUnits = new wxStaticText( m_sbSizerEndPosition->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextEndYUnits->Wrap( -1 );
	fgSizer4->Add( m_TextEndYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_staticTextOrgEnd = new wxStaticText( m_sbSizerEndPosition->GetStaticBox(), wxID_ANY, _("From:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOrgEnd->Wrap( -1 );
	fgSizer4->Add( m_staticTextOrgEnd, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );

	m_comboBoxCornerEnd = new wxComboBox( m_sbSizerEndPosition->GetStaticBox(), wxID_ANY, _("Lower Left"), wxDefaultPosition, wxSize( -1,-1 ), 0, NULL, 0 );
	m_comboBoxCornerEnd->Append( _("Upper Right") );
	m_comboBoxCornerEnd->Append( _("Upper Left") );
	m_comboBoxCornerEnd->Append( _("Lower Right") );
	m_comboBoxCornerEnd->Append( _("Lower Left") );
	m_comboBoxCornerEnd->SetSelection( 3 );
	m_comboBoxCornerEnd->SetMinSize( wxSize( 132,-1 ) );

	fgSizer4->Add( m_comboBoxCornerEnd, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	fgSizer4->Add( 0, 0, 1, wxEXPAND, 5 );


	m_sbSizerEndPosition->Add( fgSizer4, 0, wxBOTTOM|wxEXPAND, 5 );


	m_SizerItemProperties->Add( m_sbSizerEndPosition, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* sizerMisc;
	sizerMisc = new wxBoxSizer( wxHORIZONTAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lineWidthLabel = new wxStaticText( m_swItemProperties, wxID_ANY, _("Line width:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_lineWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_lineWidthLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );

	m_lineWidthCtrl = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_lineWidthCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_lineWidthUnits = new wxStaticText( m_swItemProperties, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	gbSizer1->Add( m_lineWidthUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_staticTextRot = new wxStaticText( m_swItemProperties, wxID_ANY, _("Rotation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRot->Wrap( -1 );
	gbSizer1->Add( m_staticTextRot, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );

	m_textCtrlRotation = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_textCtrlRotation, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );

	m_staticTextBitmapDPI = new wxStaticText( m_swItemProperties, wxID_ANY, _("Bitmap DPI:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBitmapDPI->Wrap( -1 );
	gbSizer1->Add( m_staticTextBitmapDPI, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );

	m_textCtrlBitmapDPI = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_textCtrlBitmapDPI, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 3 );


	gbSizer1->AddGrowableCol( 1 );

	sizerMisc->Add( gbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT, 3 );


	m_SizerItemProperties->Add( sizerMisc, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 2 );

	wxStaticBoxSizer* m_sbStep;
	m_sbStep = new wxStaticBoxSizer( new wxStaticBox( m_swItemProperties, wxID_ANY, _("Repeat Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer8->AddGrowableCol( 1 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextRepeatCnt = new wxStaticText( m_sbStep->GetStaticBox(), wxID_ANY, _("Count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRepeatCnt->Wrap( -1 );
	fgSizer8->Add( m_staticTextRepeatCnt, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );

	m_textCtrlRepeatCount = new wxTextCtrl( m_sbStep->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_textCtrlRepeatCount, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );


	fgSizer8->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextInclabel = new wxStaticText( m_sbStep->GetStaticBox(), wxID_ANY, _("Step text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInclabel->Wrap( -1 );
	fgSizer8->Add( m_staticTextInclabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	m_textCtrlTextIncrement = new wxTextCtrl( m_sbStep->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlTextIncrement->SetToolTip( _("Number of characters or digits to step text by for each repeat.") );

	fgSizer8->Add( m_textCtrlTextIncrement, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );


	fgSizer8->Add( 0, 0, 0, wxEXPAND, 5 );

	m_staticTextStepX = new wxStaticText( m_sbStep->GetStaticBox(), wxID_ANY, _("Step X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStepX->Wrap( -1 );
	fgSizer8->Add( m_staticTextStepX, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );

	m_textCtrlStepX = new wxTextCtrl( m_sbStep->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_textCtrlStepX->SetToolTip( _("Distance on the X axis to step for each repeat.") );

	fgSizer8->Add( m_textCtrlStepX, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_TextStepXUnits = new wxStaticText( m_sbStep->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextStepXUnits->Wrap( -1 );
	fgSizer8->Add( m_TextStepXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxRIGHT, 5 );

	m_staticTextStepY = new wxStaticText( m_sbStep->GetStaticBox(), wxID_ANY, _("Step Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStepY->Wrap( -1 );
	fgSizer8->Add( m_staticTextStepY, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );

	m_textCtrlStepY = new wxTextCtrl( m_sbStep->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlStepY->SetToolTip( _("Distance to step on Y axis for each repeat.") );

	fgSizer8->Add( m_textCtrlStepY, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_TextStepYUnits = new wxStaticText( m_sbStep->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextStepYUnits->Wrap( -1 );
	fgSizer8->Add( m_TextStepYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxRIGHT, 5 );


	m_sbStep->Add( fgSizer8, 0, wxEXPAND|wxBOTTOM, 5 );


	m_SizerItemProperties->Add( m_sbStep, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	m_swItemProperties->SetSizer( m_SizerItemProperties );
	m_swItemProperties->Layout();
	m_SizerItemProperties->Fit( m_swItemProperties );
	m_notebook->AddPage( m_swItemProperties, _("Item Properties"), true );
	m_swGeneralOpts = new wxScrolledWindow( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTAB_TRAVERSAL|wxVSCROLL );
	m_swGeneralOpts->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizerGeneralOpts;
	bSizerGeneralOpts = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_swGeneralOpts, wxID_ANY, _("Default Values") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer5->AddGrowableCol( 0 );
	fgSizer5->SetFlexibleDirection( wxBOTH );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextDefTsX = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Text width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefTsX->Wrap( -1 );
	fgSizer5->Add( m_staticTextDefTsX, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer5->Add( 0, 0, 1, wxEXPAND, 5 );

	m_textCtrlDefaultTextSizeX = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_textCtrlDefaultTextSizeX, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_defaultTextSizeXUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_defaultTextSizeXUnits->Wrap( -1 );
	fgSizer5->Add( m_defaultTextSizeXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextDefTsY = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Text height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefTsY->Wrap( -1 );
	fgSizer5->Add( m_staticTextDefTsY, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer5->Add( 0, 0, 1, wxEXPAND, 5 );

	m_textCtrlDefaultTextSizeY = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_textCtrlDefaultTextSizeY, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_defaultTextSizeYUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_defaultTextSizeYUnits->Wrap( -1 );
	fgSizer5->Add( m_defaultTextSizeYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_defaultLineWidthLabel = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Line thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_defaultLineWidthLabel->Wrap( -1 );
	fgSizer5->Add( m_defaultLineWidthLabel, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	fgSizer5->Add( 0, 0, 1, wxEXPAND, 5 );

	m_defaultLineWidthCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_defaultLineWidthCtrl, 0, wxEXPAND|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_defaultLineWidthUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_defaultLineWidthUnits->Wrap( -1 );
	fgSizer5->Add( m_defaultLineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_defaultTextThicknessLabel = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Text thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_defaultTextThicknessLabel->Wrap( -1 );
	fgSizer5->Add( m_defaultTextThicknessLabel, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	fgSizer5->Add( 0, 0, 1, wxEXPAND|wxRIGHT, 5 );

	m_defaultTextThicknessCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_defaultTextThicknessCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_defaultTextThicknessUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_defaultTextThicknessUnits->Wrap( -1 );
	fgSizer5->Add( m_defaultTextThicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	fgSizer5->Add( 0, 0, 1, wxEXPAND, 5 );


	sbSizer1->Add( fgSizer5, 1, wxEXPAND, 5 );

	m_buttonDefault = new wxButton( sbSizer1->GetStaticBox(), wxID_ANY, _("Set to Default"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_buttonDefault, 0, wxALL|wxEXPAND, 5 );


	bSizerGeneralOpts->Add( sbSizer1, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxStaticBoxSizer* bSizerGeneraMargins;
	bSizerGeneraMargins = new wxStaticBoxSizer( new wxStaticBox( m_swGeneralOpts, wxID_ANY, _("Page Margins") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer6->AddGrowableCol( 0 );
	fgSizer6->SetFlexibleDirection( wxBOTH );
	fgSizer6->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_leftMarginLabel = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("Left:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_leftMarginLabel->Wrap( -1 );
	fgSizer6->Add( m_leftMarginLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	m_leftMarginCtrl = new wxTextCtrl( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_leftMarginCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_leftMarginUnits = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_leftMarginUnits->Wrap( -1 );
	fgSizer6->Add( m_leftMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxRIGHT, 5 );

	m_rightMarginLabel = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("Right:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rightMarginLabel->Wrap( -1 );
	fgSizer6->Add( m_rightMarginLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	m_rightMarginCtrl = new wxTextCtrl( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_rightMarginCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_rightMarginUnits = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rightMarginUnits->Wrap( -1 );
	fgSizer6->Add( m_rightMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxRIGHT, 5 );

	m_topMarginLabel = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("Top:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_topMarginLabel->Wrap( -1 );
	fgSizer6->Add( m_topMarginLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer6->Add( 0, 0, 1, wxEXPAND|wxRIGHT, 5 );

	m_topMarginCtrl = new wxTextCtrl( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_topMarginCtrl, 0, wxEXPAND|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_topMarginUnits = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_topMarginUnits->Wrap( -1 );
	fgSizer6->Add( m_topMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_bottomMarginLabel = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("Bottom:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bottomMarginLabel->Wrap( -1 );
	fgSizer6->Add( m_bottomMarginLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	m_bottomMarginCtrl = new wxTextCtrl( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_bottomMarginCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_bottomMarginUnits = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bottomMarginUnits->Wrap( -1 );
	fgSizer6->Add( m_bottomMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerGeneraMargins->Add( fgSizer6, 1, wxEXPAND, 5 );


	bSizerGeneralOpts->Add( bSizerGeneraMargins, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	m_swGeneralOpts->SetSizer( bSizerGeneralOpts );
	m_swGeneralOpts->Layout();
	bSizerGeneralOpts->Fit( m_swGeneralOpts );
	m_notebook->AddPage( m_swGeneralOpts, _("General Options"), false );

	bSizerpanel->Add( m_notebook, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerpanel );
	this->Layout();
	bSizerpanel->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_PROPERTIES_BASE::OnUpdateUI ) );
	m_syntaxHelpLink->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( PANEL_PROPERTIES_BASE::onHelp ), NULL, this );
	m_choicePageOpt->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_stcText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onScintillaFocusLost ), NULL, this );
	m_bold->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_italic->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_fontCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_textCtrlTextSizeX->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlTextSizeY->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_constraintXCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_constraintYCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlComment->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlPosX->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlPosY->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_comboBoxCornerPos->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_comboBoxCornerPos->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlEndX->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlEndY->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_comboBoxCornerEnd->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_comboBoxCornerEnd->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_lineWidthCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlRotation->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlBitmapDPI->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlRepeatCount->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlTextIncrement->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlStepX->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlStepY->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlDefaultTextSizeX->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlDefaultTextSizeY->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_defaultLineWidthCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_defaultTextThicknessCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_buttonDefault->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::OnSetDefaultValues ), NULL, this );
	m_leftMarginCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_rightMarginCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_topMarginCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_bottomMarginCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
}

PANEL_PROPERTIES_BASE::~PANEL_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_PROPERTIES_BASE::OnUpdateUI ) );
	m_syntaxHelpLink->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( PANEL_PROPERTIES_BASE::onHelp ), NULL, this );
	m_choicePageOpt->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_stcText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onScintillaFocusLost ), NULL, this );
	m_bold->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_italic->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_fontCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_textCtrlTextSizeX->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlTextSizeY->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_constraintXCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_constraintYCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlComment->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlPosX->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlPosY->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_comboBoxCornerPos->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_comboBoxCornerPos->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlEndX->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlEndY->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_comboBoxCornerEnd->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::onModify ), NULL, this );
	m_comboBoxCornerEnd->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_lineWidthCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlRotation->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlBitmapDPI->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlRepeatCount->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlTextIncrement->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlStepX->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlStepY->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlDefaultTextSizeX->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_textCtrlDefaultTextSizeY->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_defaultLineWidthCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_defaultTextThicknessCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_buttonDefault->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::OnSetDefaultValues ), NULL, this );
	m_leftMarginCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_rightMarginCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_topMarginCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );
	m_bottomMarginCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_PROPERTIES_BASE::onTextFocusLost ), NULL, this );

}

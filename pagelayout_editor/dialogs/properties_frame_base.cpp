///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

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

	m_staticTextType = new wxStaticText( m_swItemProperties, wxID_ANY, _("Type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextType->Wrap( -1 );
	m_staticTextType->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerButt->Add( m_staticTextType, 1, wxALL|wxALIGN_CENTER_VERTICAL, 4 );

	wxString m_choicePageOptChoices[] = { _("Show on all pages"), _("First page only"), _("Subsequent pages only") };
	int m_choicePageOptNChoices = sizeof( m_choicePageOptChoices ) / sizeof( wxString );
	m_choicePageOpt = new wxChoice( m_swItemProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePageOptNChoices, m_choicePageOptChoices, 0 );
	m_choicePageOpt->SetSelection( 2 );
	bSizerButt->Add( m_choicePageOpt, 0, wxALL|wxEXPAND, 4 );


	m_SizerItemProperties->Add( bSizerButt, 0, wxEXPAND, 5 );

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

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextHjust = new wxStaticText( m_swItemProperties, wxID_ANY, _("H align:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHjust->Wrap( -1 );
	fgSizer1->Add( m_staticTextHjust, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxString m_choiceHjustifyChoices[] = { _("Left"), _("Center"), _("Right") };
	int m_choiceHjustifyNChoices = sizeof( m_choiceHjustifyChoices ) / sizeof( wxString );
	m_choiceHjustify = new wxChoice( m_swItemProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHjustifyNChoices, m_choiceHjustifyChoices, 0 );
	m_choiceHjustify->SetSelection( 0 );
	fgSizer1->Add( m_choiceHjustify, 0, wxEXPAND|wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_checkBoxBold = new wxCheckBox( m_swItemProperties, wxID_ANY, _("Bold"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_checkBoxBold, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticTextVjust = new wxStaticText( m_swItemProperties, wxID_ANY, _("V align:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVjust->Wrap( -1 );
	fgSizer1->Add( m_staticTextVjust, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	wxString m_choiceVjustifyChoices[] = { _("Top"), _("Center"), _("Bottom") };
	int m_choiceVjustifyNChoices = sizeof( m_choiceVjustifyChoices ) / sizeof( wxString );
	m_choiceVjustify = new wxChoice( m_swItemProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVjustifyNChoices, m_choiceVjustifyChoices, 0 );
	m_choiceVjustify->SetSelection( 1 );
	fgSizer1->Add( m_choiceVjustify, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_checkBoxItalic = new wxCheckBox( m_swItemProperties, wxID_ANY, _("Italic"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_checkBoxItalic, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerFontOpt->Add( fgSizer1, 0, wxEXPAND, 5 );


	m_SizerTextOptions->Add( bSizerFontOpt, 0, wxEXPAND|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextTsizeX = new wxStaticText( m_swItemProperties, wxID_ANY, _("Text width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTsizeX->Wrap( -1 );
	fgSizer2->Add( m_staticTextTsizeX, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlTextSizeX = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_textCtrlTextSizeX, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_TextTextSizeXUnits = new wxStaticText( m_swItemProperties, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextTextSizeXUnits->Wrap( -1 );
	fgSizer2->Add( m_TextTextSizeXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_staticTextTsizeY = new wxStaticText( m_swItemProperties, wxID_ANY, _("Text height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTsizeY->Wrap( -1 );
	fgSizer2->Add( m_staticTextTsizeY, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlTextSizeY = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_textCtrlTextSizeY, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_TextTextSizeYUnits = new wxStaticText( m_swItemProperties, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextTextSizeYUnits->Wrap( -1 );
	fgSizer2->Add( m_TextTextSizeYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT|wxLEFT, 5 );

	m_staticTextConstraintX = new wxStaticText( m_swItemProperties, wxID_ANY, _("Max width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextConstraintX->Wrap( -1 );
	m_staticTextConstraintX->SetToolTip( _("Set to 0 to disable this constraint") );

	fgSizer2->Add( m_staticTextConstraintX, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlConstraintX = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_textCtrlConstraintX, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_TextConstraintXUnits = new wxStaticText( m_swItemProperties, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextConstraintXUnits->Wrap( -1 );
	fgSizer2->Add( m_TextConstraintXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_staticTextConstraintY = new wxStaticText( m_swItemProperties, wxID_ANY, _("Max height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextConstraintY->Wrap( -1 );
	m_staticTextConstraintY->SetToolTip( _("Set to 0 to disable this constraint") );

	fgSizer2->Add( m_staticTextConstraintY, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlConstraintY = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_textCtrlConstraintY, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_TextConstraintYUnits = new wxStaticText( m_swItemProperties, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextConstraintYUnits->Wrap( -1 );
	fgSizer2->Add( m_TextConstraintYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT|wxLEFT, 5 );


	m_SizerTextOptions->Add( fgSizer2, 0, wxEXPAND|wxBOTTOM, 3 );

	m_staticTextSizeInfo = new wxStaticText( m_swItemProperties, wxID_ANY, _("Set to 0 to use default values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeInfo->Wrap( -1 );
	m_SizerTextOptions->Add( m_staticTextSizeInfo, 0, wxALIGN_RIGHT|wxRIGHT|wxLEFT, 5 );


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
	fgSizer3->Add( m_TextPosYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

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

	fgSizer3->Add( m_comboBoxCornerPos, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


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

	fgSizer4->Add( m_comboBoxCornerEnd, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	fgSizer4->Add( 0, 0, 1, wxEXPAND, 5 );


	m_sbSizerEndPosition->Add( fgSizer4, 0, wxBOTTOM|wxEXPAND, 5 );


	m_SizerItemProperties->Add( m_sbSizerEndPosition, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* sizerMisc;
	sizerMisc = new wxBoxSizer( wxHORIZONTAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextThickness = new wxStaticText( m_swItemProperties, wxID_ANY, _("Line width:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticTextThickness->Wrap( -1 );
	gbSizer1->Add( m_staticTextThickness, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );

	m_textCtrlThickness = new wxTextCtrl( m_swItemProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_textCtrlThickness, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_TextLineThicknessUnits = new wxStaticText( m_swItemProperties, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextLineThicknessUnits->Wrap( -1 );
	gbSizer1->Add( m_TextLineThicknessUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

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
	fgSizer8->Add( m_textCtrlRepeatCount, 0, wxEXPAND|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer8->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextInclabel = new wxStaticText( m_sbStep->GetStaticBox(), wxID_ANY, _("Step text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInclabel->Wrap( -1 );
	fgSizer8->Add( m_staticTextInclabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	m_textCtrlTextIncrement = new wxTextCtrl( m_sbStep->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlTextIncrement->SetToolTip( _("Number of characters or digits to step text by for each repeat.") );

	fgSizer8->Add( m_textCtrlTextIncrement, 0, wxEXPAND|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


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
	fgSizer8->Add( m_TextStepYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );


	m_sbStep->Add( fgSizer8, 0, wxEXPAND|wxBOTTOM, 5 );


	m_SizerItemProperties->Add( m_sbStep, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_buttonOK = new wxButton( m_swItemProperties, wxID_ANY, _("Apply"), wxDefaultPosition, wxDefaultSize, 0 );

	m_buttonOK->SetDefault();
	m_SizerItemProperties->Add( m_buttonOK, 0, wxALL|wxEXPAND, 5 );


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

	m_TextDefaultTextSizeXUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextDefaultTextSizeXUnits->Wrap( -1 );
	fgSizer5->Add( m_TextDefaultTextSizeXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextDefTsY = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Text height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefTsY->Wrap( -1 );
	fgSizer5->Add( m_staticTextDefTsY, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer5->Add( 0, 0, 1, wxEXPAND, 5 );

	m_textCtrlDefaultTextSizeY = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_textCtrlDefaultTextSizeY, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_TextDefaultTextSizeYUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextDefaultTextSizeYUnits->Wrap( -1 );
	fgSizer5->Add( m_TextDefaultTextSizeYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextDefLineW = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Line thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefLineW->Wrap( -1 );
	fgSizer5->Add( m_staticTextDefLineW, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	fgSizer5->Add( 0, 0, 1, wxEXPAND, 5 );

	m_textCtrlDefaultLineWidth = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_textCtrlDefaultLineWidth, 0, wxEXPAND|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_TextDefaultLineWidthUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextDefaultLineWidthUnits->Wrap( -1 );
	fgSizer5->Add( m_TextDefaultLineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextDefTextThickness = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Text thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefTextThickness->Wrap( -1 );
	fgSizer5->Add( m_staticTextDefTextThickness, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	fgSizer5->Add( 0, 0, 1, wxEXPAND|wxRIGHT, 5 );

	m_textCtrlDefaultTextThickness = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_textCtrlDefaultTextThickness, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_TextDefaultTextThicknessUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextDefaultTextThicknessUnits->Wrap( -1 );
	fgSizer5->Add( m_TextDefaultTextThicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	fgSizer5->Add( 0, 0, 1, wxEXPAND, 5 );


	sbSizer1->Add( fgSizer5, 1, wxEXPAND, 5 );

	m_buttonDefault = new wxButton( sbSizer1->GetStaticBox(), wxID_ANY, _("Set to Default"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_buttonDefault, 0, wxALL|wxEXPAND, 5 );


	bSizerGeneralOpts->Add( sbSizer1, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* bSizerGeneraMargins;
	bSizerGeneraMargins = new wxStaticBoxSizer( new wxStaticBox( m_swGeneralOpts, wxID_ANY, _("Page Margins") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer6->AddGrowableCol( 0 );
	fgSizer6->SetFlexibleDirection( wxBOTH );
	fgSizer6->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextLeftMargin = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("Left:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLeftMargin->Wrap( -1 );
	fgSizer6->Add( m_staticTextLeftMargin, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	m_textCtrlLeftMargin = new wxTextCtrl( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_textCtrlLeftMargin, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_TextLeftMarginUnits = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextLeftMarginUnits->Wrap( -1 );
	fgSizer6->Add( m_TextLeftMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxRIGHT, 5 );

	m_staticTextDefRightMargin = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("Right:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefRightMargin->Wrap( -1 );
	fgSizer6->Add( m_staticTextDefRightMargin, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	m_textCtrlRightMargin = new wxTextCtrl( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_textCtrlRightMargin, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_TextRightMarginUnits = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextRightMarginUnits->Wrap( -1 );
	fgSizer6->Add( m_TextRightMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxRIGHT, 5 );

	m_staticTextTopMargin = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("Top:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTopMargin->Wrap( -1 );
	fgSizer6->Add( m_staticTextTopMargin, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer6->Add( 0, 0, 1, wxEXPAND|wxRIGHT, 5 );

	m_textCtrlTopMargin = new wxTextCtrl( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_textCtrlTopMargin, 0, wxEXPAND|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_TextTopMarginUnits = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextTopMarginUnits->Wrap( -1 );
	fgSizer6->Add( m_TextTopMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextBottomMargin = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("Bottom:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBottomMargin->Wrap( -1 );
	fgSizer6->Add( m_staticTextBottomMargin, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	m_textCtrlBottomMargin = new wxTextCtrl( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6->Add( m_textCtrlBottomMargin, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_TextBottomMarginUnits = new wxStaticText( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextBottomMarginUnits->Wrap( -1 );
	fgSizer6->Add( m_TextBottomMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerGeneraMargins->Add( fgSizer6, 1, wxEXPAND, 5 );

	m_buttonGeneralOptsOK = new wxButton( bSizerGeneraMargins->GetStaticBox(), wxID_ANY, _("Apply"), wxDefaultPosition, wxDefaultSize, 0 );

	m_buttonGeneralOptsOK->SetDefault();
	bSizerGeneraMargins->Add( m_buttonGeneralOptsOK, 0, wxALL|wxEXPAND, 5 );


	bSizerGeneralOpts->Add( bSizerGeneraMargins, 0, wxEXPAND|wxTOP, 10 );


	m_swGeneralOpts->SetSizer( bSizerGeneralOpts );
	m_swGeneralOpts->Layout();
	bSizerGeneralOpts->Fit( m_swGeneralOpts );
	m_notebook->AddPage( m_swGeneralOpts, _("General Options"), false );

	bSizerpanel->Add( m_notebook, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerpanel );
	this->Layout();

	// Connect Events
	m_notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( PANEL_PROPERTIES_BASE::OnPageChanged ), NULL, this );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::OnAcceptPrms ), NULL, this );
	m_buttonDefault->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::OnSetDefaultValues ), NULL, this );
	m_buttonGeneralOptsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::OnAcceptPrms ), NULL, this );
}

PANEL_PROPERTIES_BASE::~PANEL_PROPERTIES_BASE()
{
	// Disconnect Events
	m_notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( PANEL_PROPERTIES_BASE::OnPageChanged ), NULL, this );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::OnAcceptPrms ), NULL, this );
	m_buttonDefault->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::OnSetDefaultValues ), NULL, this );
	m_buttonGeneralOptsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::OnAcceptPrms ), NULL, this );

}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  8 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "properties_frame_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PROPERTIES_BASE::PANEL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizerpanel;
	bSizerpanel = new wxBoxSizer( wxVERTICAL );
	
	m_scrolledLeftWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scrolledLeftWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerGeneralOpts;
	bSizerGeneralOpts = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDefVal = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Default Values:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefVal->Wrap( -1 );
	bSizerGeneralOpts->Add( m_staticTextDefVal, 0, wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerDefTextSize;
	bSizerDefTextSize = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerDefTsizeX;
	bSizerDefTsizeX = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDefTsX = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Text Size X (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefTsX->Wrap( -1 );
	bSizerDefTsizeX->Add( m_staticTextDefTsX, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlDefaultTextSizeX = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDefTsizeX->Add( m_textCtrlDefaultTextSizeX, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerDefTextSize->Add( bSizerDefTsizeX, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerDefTsizeY;
	bSizerDefTsizeY = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDefTsY = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Text Size Y (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefTsY->Wrap( -1 );
	bSizerDefTsizeY->Add( m_staticTextDefTsY, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlDefaultTextSizeY = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDefTsizeY->Add( m_textCtrlDefaultTextSizeY, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerDefTextSize->Add( bSizerDefTsizeY, 1, wxEXPAND, 5 );
	
	
	bSizerGeneralOpts->Add( bSizerDefTextSize, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerDefLineWidth;
	bSizerDefLineWidth = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDefLineW = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Line Thickness (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefLineW->Wrap( -1 );
	bSizer25->Add( m_staticTextDefLineW, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlDefaultLineWidth = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer25->Add( m_textCtrlDefaultLineWidth, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerDefLineWidth->Add( bSizer25, 1, 0, 5 );
	
	wxBoxSizer* bSizerDefTextThickness;
	bSizerDefTextThickness = new wxBoxSizer( wxVERTICAL );
	
	m_staticText22 = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Text Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	bSizerDefTextThickness->Add( m_staticText22, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlDefaultTextThickness = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDefTextThickness->Add( m_textCtrlDefaultTextThickness, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerDefLineWidth->Add( bSizerDefTextThickness, 1, 0, 5 );
	
	
	bSizerGeneralOpts->Add( bSizerDefLineWidth, 0, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerGeneralOpts, 0, 0, 5 );
	
	m_staticline7 = new wxStaticLine( m_scrolledLeftWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline7, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_buttonOK = new wxButton( m_scrolledLeftWindow, wxID_ANY, _("Accept"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->SetDefault(); 
	bSizerMain->Add( m_buttonOK, 0, wxALIGN_BOTTOM|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticline8 = new wxStaticLine( m_scrolledLeftWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline8, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerButt;
	bSizerButt = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerType;
	bSizerType = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextType = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextType->Wrap( -1 );
	m_staticTextType->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerType->Add( m_staticTextType, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlType = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bSizerType->Add( m_textCtrlType, 0, wxRIGHT|wxLEFT, 5 );
	
	
	bSizerButt->Add( bSizerType, 0, 0, 5 );
	
	wxBoxSizer* bSizerPageOpt;
	bSizerPageOpt = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPageOpt = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Page 1 option"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPageOpt->Wrap( -1 );
	m_staticTextPageOpt->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizerPageOpt->Add( m_staticTextPageOpt, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_choicePageOptChoices[] = { _("None"), _("Page 1 only"), _("Not on page 1") };
	int m_choicePageOptNChoices = sizeof( m_choicePageOptChoices ) / sizeof( wxString );
	m_choicePageOpt = new wxChoice( m_scrolledLeftWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePageOptNChoices, m_choicePageOptChoices, 0 );
	m_choicePageOpt->SetSelection( 0 );
	bSizerPageOpt->Add( m_choicePageOpt, 0, wxRIGHT|wxLEFT, 5 );
	
	
	bSizerButt->Add( bSizerPageOpt, 0, 0, 5 );
	
	
	bSizerMain->Add( bSizerButt, 0, 0, 5 );
	
	m_staticline5 = new wxStaticLine( m_scrolledLeftWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline5, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_SizerTextOptions = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextText = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextText->Wrap( -1 );
	m_SizerTextOptions->Add( m_staticTextText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlText = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SizerTextOptions->Add( m_textCtrlText, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerFontOpt;
	bSizerFontOpt = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerJustify;
	bSizerJustify = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextHjust = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("H justification"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHjust->Wrap( -1 );
	bSizerJustify->Add( m_staticTextHjust, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	wxString m_choiceHjustifyChoices[] = { _("Left"), _("Center"), _("Right") };
	int m_choiceHjustifyNChoices = sizeof( m_choiceHjustifyChoices ) / sizeof( wxString );
	m_choiceHjustify = new wxChoice( m_scrolledLeftWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHjustifyNChoices, m_choiceHjustifyChoices, 0 );
	m_choiceHjustify->SetSelection( 0 );
	bSizerJustify->Add( m_choiceHjustify, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_checkBoxBold = new wxCheckBox( m_scrolledLeftWindow, wxID_ANY, _("Bold"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerJustify->Add( m_checkBoxBold, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerFontOpt->Add( bSizerJustify, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizerBoldItalic;
	bSizerBoldItalic = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextVjust = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("V justification"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVjust->Wrap( -1 );
	bSizerBoldItalic->Add( m_staticTextVjust, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	wxString m_choiceVjustifyChoices[] = { _("Top"), _("Center"), _("Bottom") };
	int m_choiceVjustifyNChoices = sizeof( m_choiceVjustifyChoices ) / sizeof( wxString );
	m_choiceVjustify = new wxChoice( m_scrolledLeftWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVjustifyNChoices, m_choiceVjustifyChoices, 0 );
	m_choiceVjustify->SetSelection( 1 );
	bSizerBoldItalic->Add( m_choiceVjustify, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_checkBoxItalic = new wxCheckBox( m_scrolledLeftWindow, wxID_ANY, _("Italic"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBoldItalic->Add( m_checkBoxItalic, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerFontOpt->Add( bSizerBoldItalic, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_SizerTextOptions->Add( bSizerFontOpt, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerTextSize;
	bSizerTextSize = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerTsizeX;
	bSizerTsizeX = new wxBoxSizer( wxVERTICAL );
	
	m_staticTexTsizeX = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Text Height (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTexTsizeX->Wrap( -1 );
	bSizerTsizeX->Add( m_staticTexTsizeX, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlTextSizeX = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerTsizeX->Add( m_textCtrlTextSizeX, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerTextSize->Add( bSizerTsizeX, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerTsizeY;
	bSizerTsizeY = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextTsizeY = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Text Width (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTsizeY->Wrap( -1 );
	bSizerTsizeY->Add( m_staticTextTsizeY, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlTextSizeY = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerTsizeY->Add( m_textCtrlTextSizeY, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerTextSize->Add( bSizerTsizeY, 1, wxEXPAND, 5 );
	
	
	m_SizerTextOptions->Add( bSizerTextSize, 0, 0, 5 );
	
	m_staticTextConstraints = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Constraints:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextConstraints->Wrap( -1 );
	m_SizerTextOptions->Add( m_staticTextConstraints, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerConstraints;
	bSizerConstraints = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer42;
	bSizer42 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextConstraintX = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Max Size X (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextConstraintX->Wrap( -1 );
	bSizer42->Add( m_staticTextConstraintX, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlConstraintX = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer42->Add( m_textCtrlConstraintX, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerConstraints->Add( bSizer42, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer52;
	bSizer52 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextConstraintY = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Max Size Y (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextConstraintY->Wrap( -1 );
	bSizer52->Add( m_staticTextConstraintY, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlConstraintY = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer52->Add( m_textCtrlConstraintY, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerConstraints->Add( bSizer52, 1, wxEXPAND, 5 );
	
	
	m_SizerTextOptions->Add( bSizerConstraints, 0, 0, 5 );
	
	m_staticline6 = new wxStaticLine( m_scrolledLeftWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_SizerTextOptions->Add( m_staticline6, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerMain->Add( m_SizerTextOptions, 0, wxEXPAND, 5 );
	
	m_staticTextComment = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Comment"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextComment->Wrap( -1 );
	bSizerMain->Add( m_staticTextComment, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlComment = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMain->Add( m_textCtrlComment, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( m_scrolledLeftWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline2, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerPos;
	bSizerPos = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerPosXY;
	bSizerPosXY = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPosX = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Pos X (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosX->Wrap( -1 );
	bSizer4->Add( m_staticTextPosX, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlPosX = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_textCtrlPosX, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerPosXY->Add( bSizer4, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPosY = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Pos Y (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosY->Wrap( -1 );
	bSizer5->Add( m_staticTextPosY, 0, wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlPosY = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_textCtrlPosY, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerPosXY->Add( bSizer5, 1, wxEXPAND, 5 );
	
	
	bSizerPos->Add( bSizerPosXY, 1, 0, 5 );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextOrgPos = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Origin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOrgPos->Wrap( -1 );
	bSizer6->Add( m_staticTextOrgPos, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_comboBoxCornerPos = new wxComboBox( m_scrolledLeftWindow, wxID_ANY, _("Lower Right"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_comboBoxCornerPos->Append( _("Upper Right") );
	m_comboBoxCornerPos->Append( _("Upper Left") );
	m_comboBoxCornerPos->Append( _("Lower Right") );
	m_comboBoxCornerPos->Append( _("Lower Left") );
	m_comboBoxCornerPos->SetSelection( 2 );
	bSizer6->Add( m_comboBoxCornerPos, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerPos->Add( bSizer6, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain->Add( bSizerPos, 0, 0, 5 );
	
	m_SizerEndPosition = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerEndXY;
	bSizerEndXY = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextEndX = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("End X (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEndX->Wrap( -1 );
	bSizer41->Add( m_staticTextEndX, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlEndX = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer41->Add( m_textCtrlEndX, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerEndXY->Add( bSizer41, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer51;
	bSizer51 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextEndY = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("End Y (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEndY->Wrap( -1 );
	bSizer51->Add( m_staticTextEndY, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlEndY = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer51->Add( m_textCtrlEndY, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerEndXY->Add( bSizer51, 1, wxEXPAND, 5 );
	
	
	m_SizerEndPosition->Add( bSizerEndXY, 1, 0, 5 );
	
	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextOrgEnd = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Origin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOrgEnd->Wrap( -1 );
	bSizer61->Add( m_staticTextOrgEnd, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_comboBoxCornerEnd = new wxComboBox( m_scrolledLeftWindow, wxID_ANY, _("Lower Right"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_comboBoxCornerEnd->Append( _("Upper Right") );
	m_comboBoxCornerEnd->Append( _("Upper Left") );
	m_comboBoxCornerEnd->Append( _("Lower Right") );
	m_comboBoxCornerEnd->Append( _("Lower Left") );
	m_comboBoxCornerEnd->SetSelection( 2 );
	bSizer61->Add( m_comboBoxCornerEnd, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	m_SizerEndPosition->Add( bSizer61, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain->Add( m_SizerEndPosition, 0, 0, 5 );
	
	wxBoxSizer* bSizerLineThickness;
	bSizerLineThickness = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerthickness;
	bSizerthickness = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextThickness = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextThickness->Wrap( -1 );
	bSizerthickness->Add( m_staticTextThickness, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlThickness = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerthickness->Add( m_textCtrlThickness, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerLineThickness->Add( bSizerthickness, 0, wxEXPAND, 5 );
	
	m_staticTextInfoThickness = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Set to 0 to use default"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoThickness->Wrap( -1 );
	bSizerLineThickness->Add( m_staticTextInfoThickness, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain->Add( bSizerLineThickness, 0, 0, 5 );
	
	m_SizerRotation = new wxBoxSizer( wxVERTICAL );
	
	m_staticline1 = new wxStaticLine( m_scrolledLeftWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_SizerRotation->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerRotation;
	bSizerRotation = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextRot = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Rotation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRot->Wrap( -1 );
	bSizerRotation->Add( m_staticTextRot, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlRotation = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRotation->Add( m_textCtrlRotation, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_SizerRotation->Add( bSizerRotation, 0, wxEXPAND, 5 );
	
	
	bSizerMain->Add( m_SizerRotation, 0, wxEXPAND, 5 );
	
	m_staticline4 = new wxStaticLine( m_scrolledLeftWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextRepeatPrms = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Repeat parameters:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRepeatPrms->Wrap( -1 );
	bSizerMain->Add( m_staticTextRepeatPrms, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer611;
	bSizer611 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextRepeatCnt = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Repeat count"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRepeatCnt->Wrap( -1 );
	bSizer611->Add( m_staticTextRepeatCnt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlRepeatCount = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer611->Add( m_textCtrlRepeatCount, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizer20->Add( bSizer611, 1, 0, 5 );
	
	m_SizerTextIncrementLabel = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextInclabel = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Text Increment"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInclabel->Wrap( -1 );
	m_SizerTextIncrementLabel->Add( m_staticTextInclabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlTextIncrement = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SizerTextIncrementLabel->Add( m_textCtrlTextIncrement, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizer20->Add( m_SizerTextIncrementLabel, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizer20, 0, 0, 5 );
	
	wxBoxSizer* bSizerPosY1;
	bSizerPosY1 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer411;
	bSizer411 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextStepX = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Step X (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStepX->Wrap( -1 );
	bSizer411->Add( m_staticTextStepX, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlStepX = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer411->Add( m_textCtrlStepX, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerPosY1->Add( bSizer411, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer511;
	bSizer511 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextStepY = new wxStaticText( m_scrolledLeftWindow, wxID_ANY, _("Step Y (mm)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStepY->Wrap( -1 );
	bSizer511->Add( m_staticTextStepY, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlStepY = new wxTextCtrl( m_scrolledLeftWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer511->Add( m_textCtrlStepY, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerPosY1->Add( bSizer511, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerPosY1, 0, 0, 5 );
	
	m_staticline3 = new wxStaticLine( m_scrolledLeftWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	m_scrolledLeftWindow->SetSizer( bSizerMain );
	m_scrolledLeftWindow->Layout();
	bSizerMain->Fit( m_scrolledLeftWindow );
	bSizerpanel->Add( m_scrolledLeftWindow, 1, wxEXPAND | wxALL, 5 );
	
	
	this->SetSizer( bSizerpanel );
	this->Layout();
	
	// Connect Events
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::OnAcceptPrms ), NULL, this );
}

PANEL_PROPERTIES_BASE::~PANEL_PROPERTIES_BASE()
{
	// Disconnect Events
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PROPERTIES_BASE::OnAcceptPrms ), NULL, this );
	
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class BITMAP_BUTTON;
class COLOR_SWATCH;
class FONT_CHOICE;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/hyperlink.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stc/stc.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/combobox.h>
#include <wx/statbox.h>
#include <wx/scrolwin.h>
#include <wx/notebook.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PROPERTIES_BASE : public wxPanel
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxScrolledWindow* m_swItemProperties;
		wxBoxSizer* m_SizerItemProperties;
		wxStaticText* m_staticTextType;
		wxHyperlinkCtrl* m_syntaxHelpLink;
		wxChoice* m_choicePageOpt;
		wxBoxSizer* m_SizerTextOptions;
		wxStyledTextCtrl* m_stcText;
		BITMAP_BUTTON* m_bold;
		BITMAP_BUTTON* m_italic;
		BITMAP_BUTTON* m_separator2;
		BITMAP_BUTTON* m_alignLeft;
		BITMAP_BUTTON* m_alignCenter;
		BITMAP_BUTTON* m_alignRight;
		BITMAP_BUTTON* m_separator3;
		BITMAP_BUTTON* m_vAlignTop;
		BITMAP_BUTTON* m_vAlignMiddle;
		BITMAP_BUTTON* m_vAlignBottom;
		BITMAP_BUTTON* m_separator4;
		wxPanel* m_panelBorderColor1;
		COLOR_SWATCH* m_textColorSwatch;
		wxStaticText* m_fontLabel;
		FONT_CHOICE* m_fontCtrl;
		wxStaticText* m_staticTextTsizeX;
		wxTextCtrl* m_textCtrlTextSizeX;
		wxStaticText* m_textSizeXUnits;
		wxStaticText* m_staticTextTsizeY;
		wxTextCtrl* m_textCtrlTextSizeY;
		wxStaticText* m_textSizeYUnits;
		wxStaticText* m_constraintXLabel;
		wxTextCtrl* m_constraintXCtrl;
		wxStaticText* m_constraintXUnits;
		wxStaticText* m_constraintYLabel;
		wxTextCtrl* m_constraintYCtrl;
		wxStaticText* m_constraintYUnits;
		wxStaticText* m_staticTextSizeInfo;
		wxStaticText* m_staticTextComment;
		wxTextCtrl* m_textCtrlComment;
		wxStaticBoxSizer* sbSizerPos;
		wxStaticText* m_staticTextPosX;
		wxTextCtrl* m_textCtrlPosX;
		wxStaticText* m_TextPosXUnits;
		wxStaticText* m_staticTextPosY;
		wxTextCtrl* m_textCtrlPosY;
		wxStaticText* m_TextPosYUnits;
		wxStaticText* m_staticTextOrgPos;
		wxComboBox* m_comboBoxCornerPos;
		wxStaticBoxSizer* m_sbSizerEndPosition;
		wxStaticText* m_staticTextEndX;
		wxTextCtrl* m_textCtrlEndX;
		wxStaticText* m_TextEndXUnits;
		wxStaticText* m_staticTextEndY;
		wxTextCtrl* m_textCtrlEndY;
		wxStaticText* m_TextEndYUnits;
		wxStaticText* m_staticTextOrgEnd;
		wxComboBox* m_comboBoxCornerEnd;
		wxStaticText* m_lineWidthLabel;
		wxTextCtrl* m_lineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxStaticText* m_staticTextRot;
		wxTextCtrl* m_textCtrlRotation;
		wxStaticText* m_staticTextBitmapDPI;
		wxTextCtrl* m_textCtrlBitmapDPI;
		wxStaticText* m_staticTextRepeatCnt;
		wxTextCtrl* m_textCtrlRepeatCount;
		wxStaticText* m_staticTextInclabel;
		wxTextCtrl* m_textCtrlTextIncrement;
		wxStaticText* m_staticTextStepX;
		wxTextCtrl* m_textCtrlStepX;
		wxStaticText* m_TextStepXUnits;
		wxStaticText* m_staticTextStepY;
		wxTextCtrl* m_textCtrlStepY;
		wxStaticText* m_TextStepYUnits;
		wxScrolledWindow* m_swGeneralOpts;
		wxStaticText* m_staticTextDefTsX;
		wxTextCtrl* m_textCtrlDefaultTextSizeX;
		wxStaticText* m_defaultTextSizeXUnits;
		wxStaticText* m_staticTextDefTsY;
		wxTextCtrl* m_textCtrlDefaultTextSizeY;
		wxStaticText* m_defaultTextSizeYUnits;
		wxStaticText* m_defaultLineWidthLabel;
		wxTextCtrl* m_defaultLineWidthCtrl;
		wxStaticText* m_defaultLineWidthUnits;
		wxStaticText* m_defaultTextThicknessLabel;
		wxTextCtrl* m_defaultTextThicknessCtrl;
		wxStaticText* m_defaultTextThicknessUnits;
		wxButton* m_buttonDefault;
		wxStaticText* m_leftMarginLabel;
		wxTextCtrl* m_leftMarginCtrl;
		wxStaticText* m_leftMarginUnits;
		wxStaticText* m_rightMarginLabel;
		wxTextCtrl* m_rightMarginCtrl;
		wxStaticText* m_rightMarginUnits;
		wxStaticText* m_topMarginLabel;
		wxTextCtrl* m_topMarginCtrl;
		wxStaticText* m_topMarginUnits;
		wxStaticText* m_bottomMarginLabel;
		wxTextCtrl* m_bottomMarginCtrl;
		wxStaticText* m_bottomMarginUnits;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onHelp( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void onModify( wxCommandEvent& event ) { event.Skip(); }
		virtual void onScintillaFocusLost( wxFocusEvent& event ) { event.Skip(); }
		virtual void onTextFocusLost( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnSetDefaultValues( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PROPERTIES_BASE();

};


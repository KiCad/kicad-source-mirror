///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec  1 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/scrolwin.h>
#include <wx/notebook.h>
#include <wx/panel.h>

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
		wxChoice* m_choicePageOpt;
		wxStaticLine* m_staticline5;
		wxBoxSizer* m_SizerTextOptions;
		wxStaticText* m_staticTextText;
		wxTextCtrl* m_textCtrlText;
		wxStaticText* m_staticTextHjust;
		wxChoice* m_choiceHjustify;
		wxCheckBox* m_checkBoxBold;
		wxStaticText* m_staticTextVjust;
		wxChoice* m_choiceVjustify;
		wxCheckBox* m_checkBoxItalic;
		wxStaticText* m_staticTexTsizeX;
		wxStaticText* m_staticTextTsizeY;
		wxTextCtrl* m_textCtrlTextSizeX;
		wxStaticText* m_units1;
		wxTextCtrl* m_textCtrlTextSizeY;
		wxStaticText* m_units11;
		wxStaticText* m_staticTextConstraintX;
		wxStaticText* m_staticTextConstraintY;
		wxTextCtrl* m_textCtrlConstraintX;
		wxStaticText* m_units111;
		wxTextCtrl* m_textCtrlConstraintY;
		wxStaticText* m_units1111;
		wxStaticText* m_staticTextSizeInfo;
		wxStaticLine* m_staticline6;
		wxStaticText* m_staticTextComment;
		wxTextCtrl* m_textCtrlComment;
		wxStaticLine* m_staticline2;
		wxStaticText* m_staticTextPosX;
		wxTextCtrl* m_textCtrlPosX;
		wxStaticText* m_units12;
		wxStaticText* m_staticTextPosY;
		wxTextCtrl* m_textCtrlPosY;
		wxStaticText* m_units121;
		wxStaticText* m_staticTextOrgPos;
		wxComboBox* m_comboBoxCornerPos;
		wxBoxSizer* m_SizerEndPosition;
		wxStaticText* m_staticTextEndX;
		wxTextCtrl* m_textCtrlEndX;
		wxStaticText* m_units1211;
		wxStaticText* m_staticTextEndY;
		wxTextCtrl* m_textCtrlEndY;
		wxStaticText* m_units12111;
		wxStaticText* m_staticTextOrgEnd;
		wxComboBox* m_comboBoxCornerEnd;
		wxBoxSizer* m_SizerLineThickness;
		wxStaticText* m_staticTextThickness;
		wxTextCtrl* m_textCtrlThickness;
		wxStaticText* m_staticTextInfoThickness;
		wxBoxSizer* m_SizerRotation;
		wxStaticText* m_staticTextRot;
		wxTextCtrl* m_textCtrlRotation;
		wxBoxSizer* m_SizerPPI;
		wxStaticText* m_staticTextBitmapPPI1;
		wxTextCtrl* m_textCtrlBitmapPPI;
		wxStaticLine* m_staticline111;
		wxStaticText* m_staticTextRepeatCnt;
		wxStaticText* m_staticTextInclabel;
		wxTextCtrl* m_textCtrlRepeatCount;
		wxTextCtrl* m_textCtrlTextIncrement;
		wxStaticText* m_staticTextStepX;
		wxStaticText* m_staticTextStepY;
		wxTextCtrl* m_textCtrlStepX;
		wxStaticText* m_units121111;
		wxTextCtrl* m_textCtrlStepY;
		wxStaticText* m_units1211111;
		wxStaticLine* m_staticline8;
		wxButton* m_buttonOK;
		wxScrolledWindow* m_swGeneralOpts;
		wxStaticText* m_staticTextDefVal;
		wxStaticText* m_staticTextDefTsX;
		wxStaticText* m_staticTextDefTsY;
		wxTextCtrl* m_textCtrlDefaultTextSizeX;
		wxStaticText* m_units14;
		wxTextCtrl* m_textCtrlDefaultTextSizeY;
		wxStaticText* m_units141;
		wxStaticText* m_staticTextDefLineW;
		wxStaticText* m_staticText22;
		wxTextCtrl* m_textCtrlDefaultLineWidth;
		wxStaticText* m_staticText221;
		wxTextCtrl* m_textCtrlDefaultTextThickness;
		wxStaticText* m_staticText2211;
		wxButton* m_buttonDefault;
		wxStaticLine* m_staticline9;
		wxStaticText* m_staticTextMargins;
		wxStaticText* m_staticTextLeftMargin;
		wxStaticText* m_staticTextDefRightMargin;
		wxTextCtrl* m_textCtrlLeftMargin;
		wxStaticText* m_units142;
		wxTextCtrl* m_textCtrlRightMargin;
		wxStaticText* m_units143;
		wxStaticText* m_staticTextTopMargin;
		wxStaticText* m_staticTextBottomMargin;
		wxTextCtrl* m_textCtrlTopMargin;
		wxStaticText* m_units1431;
		wxTextCtrl* m_textCtrlDefaultBottomMargin;
		wxStaticText* m_units1432;
		wxButton* m_buttonGeneralOptsOK;

		// Virtual event handlers, overide them in your derived class
		virtual void OnPageChanged( wxNotebookEvent& event ) { event.Skip(); }
		virtual void OnAcceptPrms( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSetDefaultValues( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 340,775 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_PROPERTIES_BASE();

};


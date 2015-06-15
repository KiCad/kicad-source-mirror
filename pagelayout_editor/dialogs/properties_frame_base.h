///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PROPERTIES_FRAME_BASE_H__
#define __PROPERTIES_FRAME_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/scrolwin.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
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
		wxStaticText* m_staticTextType;
		wxTextCtrl* m_textCtrlType;
		wxStaticText* m_staticTextPageOpt;
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
		wxTextCtrl* m_textCtrlTextSizeX;
		wxStaticText* m_staticTextTsizeY;
		wxTextCtrl* m_textCtrlTextSizeY;
		wxStaticText* m_staticTextConstraints;
		wxStaticText* m_staticTextConstraintX;
		wxTextCtrl* m_textCtrlConstraintX;
		wxStaticText* m_staticTextConstraintY;
		wxTextCtrl* m_textCtrlConstraintY;
		wxStaticLine* m_staticline6;
		wxButton* m_buttonOK;
		wxStaticLine* m_staticline8;
		wxStaticText* m_staticTextComment;
		wxTextCtrl* m_textCtrlComment;
		wxStaticLine* m_staticline2;
		wxStaticText* m_staticTextPosX;
		wxTextCtrl* m_textCtrlPosX;
		wxStaticText* m_staticTextPosY;
		wxTextCtrl* m_textCtrlPosY;
		wxStaticText* m_staticTextOrgPos;
		wxComboBox* m_comboBoxCornerPos;
		wxBoxSizer* m_SizerEndPosition;
		wxStaticText* m_staticTextEndX;
		wxTextCtrl* m_textCtrlEndX;
		wxStaticText* m_staticTextEndY;
		wxTextCtrl* m_textCtrlEndY;
		wxStaticText* m_staticTextOrgEnd;
		wxComboBox* m_comboBoxCornerEnd;
		wxBoxSizer* m_SizerLineThickness;
		wxStaticText* m_staticTextThickness;
		wxTextCtrl* m_textCtrlThickness;
		wxStaticText* m_staticTextInfoThickness;
		wxBoxSizer* m_SizerRotation;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticTextRot;
		wxTextCtrl* m_textCtrlRotation;
		wxBoxSizer* m_SizerBitmapPPI;
		wxStaticText* m_staticTextBitmapPPI;
		wxTextCtrl* m_textCtrlBitmapPPI;
		wxStaticLine* m_staticline4;
		wxStaticText* m_staticTextRepeatPrms;
		wxStaticText* m_staticTextRepeatCnt;
		wxTextCtrl* m_textCtrlRepeatCount;
		wxBoxSizer* m_SizerTextIncrementLabel;
		wxStaticText* m_staticTextInclabel;
		wxTextCtrl* m_textCtrlTextIncrement;
		wxStaticText* m_staticTextStepX;
		wxTextCtrl* m_textCtrlStepX;
		wxStaticText* m_staticTextStepY;
		wxTextCtrl* m_textCtrlStepY;
		wxScrolledWindow* m_swGeneralOpts;
		wxStaticText* m_staticTextDefVal;
		wxStaticText* m_staticTextDefTsX;
		wxTextCtrl* m_textCtrlDefaultTextSizeX;
		wxStaticText* m_staticTextDefTsY;
		wxTextCtrl* m_textCtrlDefaultTextSizeY;
		wxStaticText* m_staticTextDefLineW;
		wxTextCtrl* m_textCtrlDefaultLineWidth;
		wxStaticText* m_staticText22;
		wxTextCtrl* m_textCtrlDefaultTextThickness;
		wxButton* m_buttonDefault;
		wxStaticLine* m_staticline9;
		wxStaticText* m_staticTextMargins;
		wxStaticText* m_staticTextLeftMargin;
		wxTextCtrl* m_textCtrlLeftMargin;
		wxStaticText* m_staticTextDefRightMargin;
		wxTextCtrl* m_textCtrlRightMargin;
		wxStaticText* m_staticTextTopMargin;
		wxTextCtrl* m_textCtrlTopMargin;
		wxStaticText* m_staticTextBottomMargin;
		wxTextCtrl* m_textCtrlDefaultBottomMargin;
		wxButton* m_buttonGeneralOptsOK;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnAcceptPrms( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSetDefaultValues( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		PANEL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 315,782 ), long style = wxTAB_TRAVERSAL ); 
		~PANEL_PROPERTIES_BASE();
	
};

#endif //__PROPERTIES_FRAME_BASE_H__

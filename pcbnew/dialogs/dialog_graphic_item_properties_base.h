///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE_H__
#define __DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/bmpcbox.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxGridBagSizer* m_sizerLeft;
		wxStaticText* m_startPointLabel;
		wxStaticText* m_startXLabel;
		wxTextCtrl* m_startXCtrl;
		wxStaticText* m_startXUnits;
		wxStaticText* m_startYLabel;
		wxTextCtrl* m_startYCtrl;
		wxStaticText* m_startYUnits;
		wxStaticText* m_endPointLabel;
		wxStaticText* m_endXLabel;
		wxTextCtrl* m_endXCtrl;
		wxStaticText* m_endXUnits;
		wxStaticText* m_endYLabel;
		wxTextCtrl* m_endYCtrl;
		wxStaticText* m_endYUnits;
		wxStaticText* m_bezierCtrlPt1Label;
		wxStaticText* m_BezierPointC1XLabel;
		wxTextCtrl* m_BezierC1X_Ctrl;
		wxStaticText* m_BezierPointC1XUnit;
		wxStaticText* m_BezierPointC1YLabel;
		wxTextCtrl* m_BezierC1Y_Ctrl;
		wxStaticText* m_BezierPointC1YUnit;
		wxStaticText* m_bezierCtrlPt2Label;
		wxStaticText* m_BezierPointC2XLabel;
		wxTextCtrl* m_BezierC2X_Ctrl;
		wxStaticText* m_BezierPointC2XUnit;
		wxStaticText* m_BezierPointC2YLabel;
		wxTextCtrl* m_BezierC2Y_Ctrl;
		wxStaticText* m_BezierPointC2YUnit;
		wxStaticText* m_angleLabel;
		wxTextCtrl* m_angleCtrl;
		wxStaticText* m_angleUnits;
		wxStaticText* m_thicknessLabel;
		wxTextCtrl* m_thicknessCtrl;
		wxStaticText* m_thicknessUnits;
		wxStaticText* m_LayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_LayerSelectionCtrl;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_StandardButtonsSizer;
		wxButton* m_StandardButtonsSizerOK;
		wxButton* m_StandardButtonsSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Graphic Item Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU ); 
		~DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE();
	
};

#endif //__DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE_H__

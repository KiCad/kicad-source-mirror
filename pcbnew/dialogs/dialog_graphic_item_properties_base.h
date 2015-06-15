///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE_H__
#define __DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/bmpcbox.h>
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
		wxStaticText* m_StartPointXLabel;
		wxTextCtrl* m_Center_StartXCtrl;
		wxStaticText* m_StartPointXUnit;
		wxStaticText* m_StartPointYLabel;
		wxTextCtrl* m_Center_StartYCtrl;
		wxStaticText* m_StartPointYUnit;
		wxStaticText* m_EndPointXLabel;
		wxTextCtrl* m_EndX_Radius_Ctrl;
		wxStaticText* m_EndPointXUnit;
		wxStaticText* m_EndPointYLabel;
		wxTextCtrl* m_EndY_Ctrl;
		wxStaticText* m_EndPointYUnit;
		wxStaticLine* m_staticline2;
		wxStaticText* m_Angle_Text;
		wxTextCtrl* m_Angle_Ctrl;
		wxStaticText* m_AngleUnit;
		wxStaticText* m_ThicknessLabel;
		wxTextCtrl* m_ThicknessCtrl;
		wxStaticText* m_ThicknessTextUnit;
		wxStaticText* m_DefaultThicknessLabel;
		wxTextCtrl* m_DefaultThicknessCtrl;
		wxStaticText* m_DefaulThicknessTextUnit;
		wxStaticText* m_LayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_LayerSelectionCtrl;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_StandardButtonsSizer;
		wxButton* m_StandardButtonsSizerOK;
		wxButton* m_StandardButtonsSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Graphic Item Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 576,215 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU ); 
		~DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE();
	
};

#endif //__DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE_H__

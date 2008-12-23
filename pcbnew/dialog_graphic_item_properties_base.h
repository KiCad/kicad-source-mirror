///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_graphic_item_properties_base__
#define __dialog_graphic_item_properties_base__

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DialogGraphicItemProperties_base
///////////////////////////////////////////////////////////////////////////////
class DialogGraphicItemProperties_base : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			wxID_ANGLE_CTRL = 1000,
			wxID_LAYER_SELECTION,
		};
		
		wxStaticText* m_Start_Center_XText;
		wxTextCtrl* m_Center_StartXCtrl;
		wxStaticText* m_Start_Center_YText;
		wxTextCtrl* m_Center_StartYCtrl;
		wxStaticText* m_EndX_Radius_Text;
		wxTextCtrl* m_EndX_Radius_Ctrl;
		wxStaticText* m_EndY_Text;
		wxTextCtrl* m_EndY_Ctrl;
		wxStaticText* m_Angle_Text;
		wxTextCtrl* m_Angle_Ctrl;
		wxStaticText* m_ItemThicknessText;
		wxTextCtrl* m_ThicknessCtrl;
		wxStaticText* m_DefaultThicknessText;
		wxTextCtrl* m_DefaultThicknessCtrl;
		wxStaticLine* m_staticline1;
		wxStaticText* m_LayerText;
		wxChoice* m_LayerSelection;
		wxButton* m_buttonOK;
		wxButton* m_buttonCANCEL;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ){ event.Skip(); }
		virtual void OnLayerChoice( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DialogGraphicItemProperties_base( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Graphic item properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 348,247 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU );
		~DialogGraphicItemProperties_base();
	
};

#endif //__dialog_graphic_item_properties_base__

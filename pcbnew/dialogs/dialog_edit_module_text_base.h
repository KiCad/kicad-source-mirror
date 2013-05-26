///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 10 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EDIT_MODULE_TEXT_BASE_H__
#define __DIALOG_EDIT_MODULE_TEXT_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DialogEditModuleText_base
///////////////////////////////////////////////////////////////////////////////
class DialogEditModuleText_base : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_ModuleInfoText;
		wxStaticLine* m_staticline1;
		wxStaticText* m_TextDataTitle;
		wxTextCtrl* m_Name;
		wxStaticText* m_SizeXTitle;
		wxTextCtrl* m_TxtSizeCtrlX;
		wxStaticText* m_SizeYTitle;
		wxTextCtrl* m_TxtSizeCtrlY;
		wxStaticText* m_PosXTitle;
		wxTextCtrl* m_TxtPosCtrlX;
		wxStaticText* m_PosYTitle;
		wxTextCtrl* m_TxtPosCtrlY;
		wxStaticText* m_WidthTitle;
		wxTextCtrl* m_TxtWidthCtlr;
		wxRadioBox* m_Orient;
		wxRadioBox* m_Show;
		wxRadioBox* m_Style;
		wxButton* m_buttonOK;
		wxButton* m_buttonCANCEL;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DialogEditModuleText_base( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint text properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 357,299 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DialogEditModuleText_base();
	
};

#endif //__DIALOG_EDIT_MODULE_TEXT_BASE_H__

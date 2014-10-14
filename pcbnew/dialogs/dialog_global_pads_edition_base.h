///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GLOBAL_PADS_EDITION_BASE_H__
#define __DIALOG_GLOBAL_PADS_EDITION_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CHANGE_GET_PAD_SETTINGS 1000
#define ID_CHANGE_CURRENT_MODULE 1001
#define ID_CHANGE_ID_MODULES 1002

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GLOBAL_PADS_EDITION_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GLOBAL_PADS_EDITION_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxCheckBox* m_Pad_Shape_Filter_CB;
		wxCheckBox* m_Pad_Layer_Filter_CB;
		wxCheckBox* m_Pad_Orient_Filter_CB;
		wxButton* m_buttonPadEditor;
		wxButton* m_buttonChangeModule;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void InstallPadEditor( wxCommandEvent& event ) { event.Skip(); }
		virtual void PadPropertiesAccept( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		wxButton* m_buttonIdModules;
		
		DIALOG_GLOBAL_PADS_EDITION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Global Pads Edition"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GLOBAL_PADS_EDITION_BASE();
	
};

#endif //__DIALOG_GLOBAL_PADS_EDITION_BASE_H__

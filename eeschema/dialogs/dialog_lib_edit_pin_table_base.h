///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_LIB_EDIT_PIN_TABLE_BASE_H__
#define __DIALOG_LIB_EDIT_PIN_TABLE_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/dataview.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_EDIT_PIN_TABLE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_EDIT_PIN_TABLE_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxDataViewCtrl* m_Pins;
		wxTextCtrl* m_Summary;
		wxStdDialogButtonSizer* m_Buttons;
		wxButton* m_ButtonsOK;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnColumnHeaderRightClicked( wxDataViewEvent& event ) = 0;
		
	
	public:
		
		DIALOG_LIB_EDIT_PIN_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pin Table"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 431,304 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_LIB_EDIT_PIN_TABLE_BASE();
	
};

#endif //__DIALOG_LIB_EDIT_PIN_TABLE_BASE_H__

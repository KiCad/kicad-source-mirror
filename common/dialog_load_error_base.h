///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 19 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_load_error_base__
#define __dialog_load_error_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LOAD_ERROR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LOAD_ERROR_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* StaticTextMessage;
		wxTextCtrl* TextCtrlList;
		wxButton* OkButton;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_LOAD_ERROR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Load Error!"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER );
		~DIALOG_LOAD_ERROR_BASE();
	
};

#endif //__dialog_load_error_base__

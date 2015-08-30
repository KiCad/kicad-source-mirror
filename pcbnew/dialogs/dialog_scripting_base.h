///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_SCRIPTING_BASE_H__
#define __DIALOG_SCRIPTING_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SCRIPTING_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SCRIPTING_BASE : public wxFrame 
{
	private:
	
	protected:
		wxTextCtrl* m_txScript;
		wxButton* m_btRun;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnRunButtonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_SCRIPTING_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Scripting Test Window"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,468 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~DIALOG_SCRIPTING_BASE();
	
};

#endif //__DIALOG_SCRIPTING_BASE_H__

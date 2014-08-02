///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_ABOUT_BASE_H__
#define __DIALOG_ABOUT_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/aui/auibook.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class dialog_about_base
///////////////////////////////////////////////////////////////////////////////
class dialog_about_base : public wxDialog 
{
	private:
		wxButton* m_buttonOK;
	
	protected:
		wxStaticBitmap* m_bitmapApp;
		wxStaticText* m_staticTextAppTitle;
		wxStaticText* m_staticTextCopyright;
		wxStaticText* m_staticTextBuildVersion;
		wxStaticText* m_staticTextLibVersion;
		wxAuiNotebook* m_auiNotebook;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		dialog_about_base( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("About..."), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 750,450 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSTAY_ON_TOP ); 
		~dialog_about_base();
	
};

#endif //__DIALOG_ABOUT_BASE_H__

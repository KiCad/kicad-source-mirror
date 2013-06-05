///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 10 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_SHOW_PAGE_BORDERS_BASE_H__
#define __DIALOG_SHOW_PAGE_BORDERS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxRadioBox* m_ShowPageLimits;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKBUttonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Page Borders"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 263,254 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE();
	
};

#endif //__DIALOG_SHOW_PAGE_BORDERS_BASE_H__

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_SYMBOL_REMAP_BASE_H__
#define __DIALOG_SYMBOL_REMAP_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
class DIALOG_SHIM;
class WX_HTML_REPORT_PANEL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SYMBOL_REMAP_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SYMBOL_REMAP_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxButton* m_buttonRemp;
		wxButton* m_buttonClose;
		WX_HTML_REPORT_PANEL* m_messagePanel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnRemapSymbols( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_SYMBOL_REMAP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Remap Symbols"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_SYMBOL_REMAP_BASE();
	
};

#endif //__DIALOG_SYMBOL_REMAP_BASE_H__

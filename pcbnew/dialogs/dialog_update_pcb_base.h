///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_UPDATE_PCB_BASE_H__
#define __DIALOG_UPDATE_PCB_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;
class WX_HTML_REPORT_PANEL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_UPDATE_PCB_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_UPDATE_PCB_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxRadioButton* m_matchByReference;
		wxRadioButton* m_matchByTimestamp;
		WX_HTML_REPORT_PANEL* m_messagePanel;
		wxButton* m_btnCancel;
		wxButton* m_btnPerformUpdate;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnMatchChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_UPDATE_PCB_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Update PCB from Schematics"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_UPDATE_PCB_BASE();
	
};

#endif //__DIALOG_UPDATE_PCB_BASE_H__

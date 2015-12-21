///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GET_COMPONENT_BASE_H__
#define __DIALOG_GET_COMPONENT_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_SEL_BY_LISTBOX 1000
#define ID_ACCEPT_KEYWORD 1001
#define ID_LIST_ALL 1002
#define ID_EXTRA_TOOL 1003

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GET_COMPONENT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GET_COMPONENT_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticTextName;
		wxTextCtrl* m_textCmpNameCtrl;
		wxStaticText* m_staticTextHistory;
		wxListBox* m_historyList;
		wxButton* m_buttonKW;
		wxButton* m_buttonList;
		wxButton* m_buttonBrowse;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void Accept( wxCommandEvent& event ) { event.Skip(); }
		virtual void GetExtraSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GET_COMPONENT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 361,285 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GET_COMPONENT_BASE();
	
};

#endif //__DIALOG_GET_COMPONENT_BASE_H__

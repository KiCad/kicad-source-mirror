///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 19 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE_H__
#define __FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE_H__

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
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticTextInfo;
		wxListCtrl* m_listFp;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnItemClicked( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnColumnClick( wxListEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint Assignment Conflicts"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE();
	
};

#endif //__FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE_H__

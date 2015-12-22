///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GET_FOOTPRINT_BY_NAME_BASE_H__
#define __DIALOG_GET_FOOTPRINT_BY_NAME_BASE_H__

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
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GET_FOOTPRINT_BY_NAME_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GET_FOOTPRINT_BY_NAME_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticTextRef;
		wxTextCtrl* m_SearchTextCtrl;
		wxStaticText* m_staticTextRef1;
		wxChoice* m_choiceFpList;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnSelectFootprint( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GET_FOOTPRINT_BY_NAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Search for footprint"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 341,176 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GET_FOOTPRINT_BY_NAME_BASE();
	
};

#endif //__DIALOG_GET_FOOTPRINT_BY_NAME_BASE_H__

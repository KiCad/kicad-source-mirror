///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_CHOOSE_COMPONENT_BASE_H__
#define __DIALOG_CHOOSE_COMPONENT_BASE_H__

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
#include <wx/sizer.h>
#include <wx/treectrl.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CHOOSE_COMPONENT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CHOOSE_COMPONENT_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_searchLabel;
		wxTextCtrl* m_searchBox;
		wxTreeCtrl* m_libraryComponentTree;
		wxPanel* m_componentView;
		wxTextCtrl* m_componentDetails;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInterceptSearchBoxKey( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnSearchBoxChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchBoxEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnInterceptTreeEnter( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnTreeMouseUp( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnDoubleClickTreeActivation( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnTreeSelect( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnStartComponentBrowser( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnHandlePreviewRepaint( wxPaintEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_CHOOSE_COMPONENT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 503,500 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_CHOOSE_COMPONENT_BASE();
	
};

#endif //__DIALOG_CHOOSE_COMPONENT_BASE_H__

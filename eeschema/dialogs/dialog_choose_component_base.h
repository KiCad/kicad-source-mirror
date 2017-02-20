///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb  6 2017)
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
class TWO_COLUMN_TREE_LIST;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/srchctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/treelist.h>
#include <wx/html/htmlwin.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/splitter.h>
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
		wxSplitterWindow* m_splitter1;
		wxPanel* m_panel3;
		wxSearchCtrl* m_searchBox;
		TWO_COLUMN_TREE_LIST* m_libraryComponentTree;
		wxHtmlWindow* m_componentDetails;
		wxPanel* m_panel4;
		wxPanel* m_componentView;
		wxChoice* m_chooseFootprint;
		wxPanel* m_footprintView;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnInterceptSearchBoxKey( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnSearchBoxChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchBoxEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnInterceptTreeEnter( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnTreeMouseUp( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnDoubleClickTreeActivation( wxTreeListEvent& event ) { event.Skip(); }
		virtual void OnTreeSelect( wxTreeListEvent& event ) { event.Skip(); }
		virtual void OnDatasheetClick( wxHtmlLinkEvent& event ) { event.Skip(); }
		virtual void OnStartComponentBrowser( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnHandlePreviewRepaint( wxPaintEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_CHOOSE_COMPONENT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 800,650 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_CHOOSE_COMPONENT_BASE();
		
		void m_splitter1OnIdle( wxIdleEvent& )
		{
			m_splitter1->SetSashPosition( -300 );
			m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::m_splitter1OnIdle ), NULL, this );
		}
	
};

#endif //__DIALOG_CHOOSE_COMPONENT_BASE_H__

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

#include "dialog_shim.h"
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/dataview.h>
#include <wx/html/htmlwin.h>
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
		wxStaticBitmap* m_searchBoxIcon;
		wxTextCtrl* m_searchBox;
		wxDataViewCtrl* m_libraryComponentTree;
		wxHtmlWindow* m_componentDetails;
		wxPanel* m_panel4;
		wxPanel* m_componentView;
		wxChoice* m_chooseFootprint;
		wxPanel* m_footprintView;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnIdle( wxIdleEvent& event ) { event.Skip(); }
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnSearchBoxChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchBoxEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTreeActivate( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnTreeSelect( wxDataViewEvent& event ) { event.Skip(); }
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

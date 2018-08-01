///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 18 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_HOTKEYS_EDITOR_BASE_H__
#define __PANEL_HOTKEYS_EDITOR_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/srchctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_HOTKEYS_EDITOR_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_HOTKEYS_EDITOR_BASE : public wxPanel 
{
	private:
	
	protected:
		wxBoxSizer* m_mainSizer;
		wxSearchCtrl* m_filterSearch;
		wxPanel* m_panelHotkeys;
		wxButton* m_resetButton;
		wxButton* m_defaultButton;
		wxButton* btnImport;
		wxButton* btnExport;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnFilterSearch( wxCommandEvent& event ) { event.Skip(); }
		virtual void ResetClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void DefaultsClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnImport( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExport( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		PANEL_HOTKEYS_EDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL ); 
		~PANEL_HOTKEYS_EDITOR_BASE();
	
};

#endif //__PANEL_HOTKEYS_EDITOR_BASE_H__

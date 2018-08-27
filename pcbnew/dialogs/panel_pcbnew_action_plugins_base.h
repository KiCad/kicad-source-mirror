///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug  2 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_PCBNEW_ACTION_PLUGINS_BASE_H__
#define __PANEL_PCBNEW_ACTION_PLUGINS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
class WX_GRID;

#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PCBNEW_ACTION_PLUGINS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PCBNEW_ACTION_PLUGINS_BASE : public wxPanel 
{
	private:
	
	protected:
		WX_GRID* m_grid;
		wxBitmapButton* m_moveUpButton;
		wxBitmapButton* m_moveDownButton;
		wxBitmapButton* m_reloadButton;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnGridCellClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnMoveUpButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveDownButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReloadButtonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		PANEL_PCBNEW_ACTION_PLUGINS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 485,200 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString ); 
		~PANEL_PCBNEW_ACTION_PLUGINS_BASE();
	
};

#endif //__PANEL_PCBNEW_ACTION_PLUGINS_BASE_H__

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_GRID;

#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SYMBOL_PIN_MAP_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SYMBOL_PIN_MAP_BASE : public wxPanel
{
	private:

	protected:
		WX_GRID* m_grid;
		wxButton* m_addFootprintButton;
		wxButton* m_removeFootprintButton;

		// Virtual event handlers, override them in your derived class
		virtual void OnSizeGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddFootprint( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveFootprint( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SYMBOL_PIN_MAP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SYMBOL_PIN_MAP_BASE();

};


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
class STD_BITMAP_BUTTON;
class WX_GRID;

#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_TEXT_VARIABLES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_TEXT_VARIABLES_BASE : public wxPanel
{
	private:

	protected:
		WX_GRID* m_TextVars;
		STD_BITMAP_BUTTON* m_btnAddTextVar;
		STD_BITMAP_BUTTON* m_btnDeleteTextVar;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnAddTextVar( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveTextVar( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_TEXT_VARIABLES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_TEXT_VARIABLES_BASE();

};


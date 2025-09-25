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

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_BOM_PRESETS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_BOM_PRESETS_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_bomTitle;
		WX_GRID* m_bomPresetsGrid;
		STD_BITMAP_BUTTON* m_btnDeleteBomPreset;
		wxStaticText* m_bomFmtTitle;
		WX_GRID* m_bomFmtPresetsGrid;
		STD_BITMAP_BUTTON* m_btnDeleteBomFmtPreset;

		// Virtual event handlers, override them in your derived class
		virtual void OnSizeGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnDeleteBomPreset( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteBomFmtPreset( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_BOM_PRESETS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_BOM_PRESETS_BASE();

};


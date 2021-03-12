///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
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
/// Class PANEL_FP_EDITOR_DEFAULTS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_FP_EDITOR_DEFAULTS_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* defaultTextItemsLabel;
		WX_GRID* m_textItemsGrid;
		wxBitmapButton* m_bpAdd;
		wxBitmapButton* m_bpDelete;
		wxStaticText* m_staticTextInfo;
		WX_GRID* m_layerClassesGrid;

		// Virtual event handlers, overide them in your derived class
		virtual void OnGridSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddTextItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteTextItem( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_FP_EDITOR_DEFAULTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_FP_EDITOR_DEFAULTS_BASE();

};


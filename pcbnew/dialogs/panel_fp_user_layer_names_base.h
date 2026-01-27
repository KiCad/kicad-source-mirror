///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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

#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_FP_USER_LAYER_NAMES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_FP_USER_LAYER_NAMES_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* m_lblUserLayers;
		wxChoice* m_choiceUserLayers;
		wxStaticText* m_layerNamesLabel;
		WX_GRID* m_layerNamesGrid;
		STD_BITMAP_BUTTON* m_bpAdd;
		STD_BITMAP_BUTTON* m_bpDelete;

		// Virtual event handlers, override them in your derived class
		virtual void onUserLayerCountChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onLayerChange( wxGridEvent& event ) { event.Skip(); }
		virtual void OnAddLayerItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteLayerItem( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_FP_USER_LAYER_NAMES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_FP_USER_LAYER_NAMES_BASE();

};


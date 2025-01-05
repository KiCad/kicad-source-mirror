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
#include "widgets/wx_panel.h"
#include <wx/scrolwin.h>
#include <wx/grid.h> // needed for MSVC to see wxScrolledCanvas indirectly exported
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/choice.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class APPEARANCE_CONTROLS_3D_BASE
///////////////////////////////////////////////////////////////////////////////
class APPEARANCE_CONTROLS_3D_BASE : public WX_PANEL
{
	private:

	protected:
		wxBoxSizer* m_sizerOuter;
		wxPanel* m_panelLayers;
		wxBoxSizer* m_panelLayersSizer;
		wxScrolledCanvas* m_windowLayers;
		wxStaticLine* m_staticline1;
		wxStaticText* m_presetsLabel;
		wxChoice* m_cbLayerPresets;
		wxStaticText* m_viewportsLabel;
		wxChoice* m_cbViewports;

		// Virtual event handlers, override them in your derived class
		virtual void OnSetFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void onLayerPresetChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onViewportChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdateViewportsCb( wxUpdateUIEvent& event ) { event.Skip(); }


	public:

		APPEARANCE_CONTROLS_3D_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~APPEARANCE_CONTROLS_3D_BASE();

};


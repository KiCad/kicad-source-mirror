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
#include <wx/button.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_LAYERS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_LAYERS_BASE : public wxPanel
{
	private:

	protected:
		wxButton* m_addUserDefinedLayerButton;
		wxStaticLine* m_staticline2;
		wxScrolledWindow* m_LayersListPanel;
		wxFlexGridSizer* m_LayersSizer;

		// Virtual event handlers, override them in your derived class
		virtual void addUserDefinedLayer( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_LAYERS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_LAYERS_BASE();

};


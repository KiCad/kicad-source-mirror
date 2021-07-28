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
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_FP_PROPERTIES_3D_MODEL_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_FP_PROPERTIES_3D_MODEL_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* bSizerMain3D;
		WX_GRID* m_modelsGrid;
		wxBitmapButton* m_button3DShapeAdd;
		wxBitmapButton* m_button3DShapeBrowse;
		wxBitmapButton* m_button3DShapeRemove;
		wxButton* m_buttonConfig3DPaths;
		wxBoxSizer* bLowerSizer3D;

		// Virtual event handlers, overide them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void On3DModelCellChanged( wxGridEvent& event ) { event.Skip(); }
		virtual void On3DModelSelected( wxGridEvent& event ) { event.Skip(); }
		virtual void OnAdd3DRow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAdd3DModel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemove3DModel( wxCommandEvent& event ) { event.Skip(); }
		virtual void Cfg3DPath( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_FP_PROPERTIES_3D_MODEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_FP_PROPERTIES_3D_MODEL_BASE();

};


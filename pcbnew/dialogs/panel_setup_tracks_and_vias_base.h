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
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_TRACKS_AND_VIAS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_TRACKS_AND_VIAS_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_label;
		WX_GRID* m_trackWidthsGrid;
		wxBitmapButton* m_trackWidthsAddButton;
		wxBitmapButton* m_trackWidthsRemoveButton;
		WX_GRID* m_viaSizesGrid;
		wxBitmapButton* m_viaSizesAddButton;
		wxBitmapButton* m_viaSizesRemoveButton;
		WX_GRID* m_diffPairsGrid;
		wxBitmapButton* m_diffPairsAddButton;
		wxBitmapButton* m_diffPairsRemoveButton;

		// Virtual event handlers, overide them in your derived class
		virtual void OnAddTrackWidthsClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveTrackWidthsClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddViaSizesClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveViaSizesClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddDiffPairsClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveDiffPairsClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 674,343 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_SETUP_TRACKS_AND_VIAS_BASE();

};


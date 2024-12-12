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
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_DRC_GROUP_HEADER_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_DRC_GROUP_HEADER_BASE : public wxPanel
{
	private:

	protected:
		wxGrid* m_dataGrid;

		// Virtual event handlers, override them in your derived class
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnGridSize( wxGridSizeEvent& event ) { event.Skip(); }


	public:

		PANEL_DRC_GROUP_HEADER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_DRC_GROUP_HEADER_BASE();

};


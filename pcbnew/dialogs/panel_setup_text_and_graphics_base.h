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

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_TEXT_AND_GRAPHICS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_TEXT_AND_GRAPHICS_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* m_mainSizer;
		wxBoxSizer* m_gridSizer;
		wxStaticText* m_staticTextDefProp;
		wxStaticLine* m_staticline11;
		WX_GRID* m_grid;

	public:

		PANEL_SETUP_TEXT_AND_GRAPHICS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_TEXT_AND_GRAPHICS_BASE();

};


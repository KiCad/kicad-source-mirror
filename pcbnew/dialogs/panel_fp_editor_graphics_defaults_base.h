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

#include "widgets/resettable_panel.h"
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
/// Class PANEL_FP_EDITOR_GRAPHICS_DEFAULTS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_FP_EDITOR_GRAPHICS_DEFAULTS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticLine* m_staticline1;
		WX_GRID* m_graphicsGrid;

	public:

		PANEL_FP_EDITOR_GRAPHICS_DEFAULTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_FP_EDITOR_GRAPHICS_DEFAULTS_BASE();

};


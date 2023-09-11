///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/html_window.h"
#include "calculator_panels/calculator_panel.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/html/htmlwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_ESERIES_DISPLAY_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_ESERIES_DISPLAY_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxGrid* m_GridEseries2496;
		wxGrid* m_GridEseries112;
		HTML_WINDOW* m_panelESeriesHelp;

	public:

		PANEL_ESERIES_DISPLAY_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = 0, const wxString& name = wxEmptyString );

		~PANEL_ESERIES_DISPLAY_BASE();

};


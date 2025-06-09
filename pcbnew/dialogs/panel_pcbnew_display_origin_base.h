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
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PCBNEW_DISPLAY_ORIGIN_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PCBNEW_DISPLAY_ORIGIN_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxBoxSizer* m_displayOrigin;
		wxStaticText* displayOriginLabel;
		wxStaticLine* m_staticline1;
		wxRadioButton* m_pageOrigin;
		wxRadioButton* m_drillPlaceOrigin;
		wxRadioButton* m_gridOrigin;
		wxStaticText* xAxisLabel;
		wxStaticLine* m_staticline2;
		wxRadioButton* m_xIncreasesRight;
		wxRadioButton* m_xIncreasesLeft;
		wxStaticText* yAxisLabel;
		wxStaticLine* m_staticline3;
		wxRadioButton* m_yIncreasesUp;
		wxRadioButton* m_yIncreasesDown;

	public:

		PANEL_PCBNEW_DISPLAY_ORIGIN_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PCBNEW_DISPLAY_ORIGIN_BASE();

};


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
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SPACEMOUSE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SPACEMOUSE_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* m_panRotateLabel;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText1;
		wxSlider* m_zoomSpeed;
		wxStaticText* m_staticText22;
		wxSlider* m_autoPanSpeed;
		wxCheckBox* m_checkEnablePanH;
		wxCheckBox* m_reverseY;
		wxCheckBox* m_reverseX;
		wxSlider* m_dummy1;
		wxCheckBox* m_reverseZ;
		wxSlider* m_dummy2;

	public:

		PANEL_SPACEMOUSE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SPACEMOUSE_BASE();

};


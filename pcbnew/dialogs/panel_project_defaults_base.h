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
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/simplebook.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PROJECT_DEFAULTS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PROJECT_DEFAULTS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxSimplebook* m_optionsBook;
		wxStaticText* m_defaultOpacityLabel;
		wxStaticLine* m_staticline6;
		wxStaticText* m_trackOpacityLabel;
        wxSpinCtrl*   m_OptTrackOpacity;
        wxStaticText* m_opacityUnit;
		wxStaticText* m_viaOpacityLabel;
        wxSpinCtrl*   m_OptViaOpacity;
        wxStaticText* m_opacityUnit1;
		wxStaticText* m_padOpacityLabel;
        wxSpinCtrl*   m_OptPadOpacity;
        wxStaticText* m_opacityUnit2;
		wxStaticText* m_zoneOpacityLabel;
        wxSpinCtrl*   m_OptZoneOpacity;
        wxStaticText* m_opacityUnit3;
		wxStaticText* m_filledShapesOpacityLabel;
        wxSpinCtrl*   m_OptFilledShapeOpacity;
        wxStaticText* m_opacityUnit4;
		wxStaticText* m_imageOpacityLabel;
        wxSpinCtrl*   m_OptImageOpacity;
        wxStaticText* m_opacityUnit5;

	public:

		PANEL_PROJECT_DEFAULTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PROJECT_DEFAULTS_BASE();

};


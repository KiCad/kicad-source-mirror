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
#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/radiobut.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxBoxSizer* m_UpperSizer;
		wxBoxSizer* m_galOptionsSizer;
		wxStaticText* m_staticText1;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_OptDisplayDCodes;
		wxCheckBox* m_ShowPageLimitsOpt;
		wxStaticText* m_staticText2;
		wxStaticLine* m_staticline2;
		wxCheckBox* m_OptDisplayFlashedItems;
		wxCheckBox* m_OptDisplayLines;
		wxCheckBox* m_OptDisplayPolygons;
		wxStaticText* m_staticTextOpacity;
		wxSpinCtrlDouble* m_spOpacityCtrl;
		wxStaticText* m_staticText3;
		wxStaticLine* m_staticline3;
		wxRadioButton* m_pageSizeFull;
		wxRadioButton* m_pageSizeA4;
		wxRadioButton* m_pageSizeA3;
		wxRadioButton* m_pageSizeA2;
		wxRadioButton* m_pageSizeA;
		wxRadioButton* m_pageSizeB;
		wxRadioButton* m_pageSizeC;

	public:

		PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 257,534 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE();

};


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
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/radiobut.h>
#include <wx/combobox.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE : public wxPanel
{
	private:

	protected:

	public:
		wxCheckBox* m_enabled;
		wxRadioButton* m_sourceSheetnameRb;
		wxRadioButton* m_sourceComponentClassRb;
		wxComboBox* m_sourceObjectNameCb;

		PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE();

};


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
#include <wx/radiobut.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/combobox.h>
#include <wx/sizer.h>
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
		wxRadioButton* m_DisabledRb;
		wxRadioButton* m_SheetRb;
		wxComboBox* m_sheetCombo;
		wxRadioButton* m_ComponentsRb;
		wxComboBox* m_componentClassCombo;
		wxRadioButton* m_GroupRb;
		wxComboBox* m_groupCombo;

		PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE();

};


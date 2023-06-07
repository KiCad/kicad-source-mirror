///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-282-g1fa54006)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class UNIT_SELECTOR_LEN;

#include "calculator_panels/calculator_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_ELECTRICAL_SPACING_IPC2221_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_ELECTRICAL_SPACING_IPC2221_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxStaticText* m_stSpacingUnit;
		UNIT_SELECTOR_LEN* m_ElectricalSpacingUnitsSelector;
		wxStaticLine* m_staticline2;
		wxStaticText* m_stVoltage;
		wxTextCtrl* m_ElectricalSpacingVoltage;
		wxButton* m_buttonElectSpacingRefresh;
		wxBoxSizer* m_electricalSpacingSizer;
		wxStaticText* m_staticTextElectricalSpacing;
		wxGrid* m_gridElectricalSpacingValues;
		wxStaticText* m_stHelp;

		// Virtual event handlers, override them in your derived class
		virtual void OnElectricalSpacingUnitsSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnElectricalSpacingRefresh( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_ELECTRICAL_SPACING_IPC2221_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 551,450 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_ELECTRICAL_SPACING_IPC2221_BASE();

};


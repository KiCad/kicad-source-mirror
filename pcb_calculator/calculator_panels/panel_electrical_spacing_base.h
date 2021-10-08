///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class UNIT_SELECTOR_LEN;

#include <wx/string.h>
#include <wx/choice.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_ELECTRICAL_SPACING_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_ELECTRICAL_SPACING_BASE : public wxPanel
{
	private:

	protected:
		UNIT_SELECTOR_LEN* m_ElectricalSpacingUnitsSelector;
		wxStaticLine* m_staticline2;
		wxStaticText* m_staticText891;
		wxTextCtrl* m_ElectricalSpacingVoltage;
		wxButton* m_buttonElectSpacingRefresh;
		wxBoxSizer* m_electricalSpacingSizer;
		wxStaticText* m_staticTextElectricalSpacing;
		wxGrid* m_gridElectricalSpacingValues;
		wxStaticText* m_staticText88;

		// Virtual event handlers, overide them in your derived class
		virtual void OnElectricalSpacingUnitsSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnElectricalSpacingRefresh( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_ELECTRICAL_SPACING_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_ELECTRICAL_SPACING_BASE();

};


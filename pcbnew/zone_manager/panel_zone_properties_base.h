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
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/statline.h>
#include <wx/gbsizer.h>
#include <wx/spinctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_ZONE_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_ZONE_PROPERTIES_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_zoneNameLabel;
		wxTextCtrl* m_tcZoneName;
		wxCheckBox* m_cbLocked;
		wxStaticText* m_staticTextStyle;
		wxChoice* m_OutlineDisplayCtrl;
		wxStaticText* m_stBorderHatchPitchText;
		wxTextCtrl* m_outlineHatchPitchCtrl;
		wxStaticText* m_outlineHatchUnits;
		wxStaticLine* m_staticline4;
		wxStaticText* m_staticTextSmoothing;
		wxChoice* m_cornerSmoothingChoice;
		wxStaticText* m_cornerRadiusLabel;
		wxTextCtrl* m_cornerRadiusCtrl;
		wxStaticText* m_cornerRadiusUnits;
		wxStaticText* m_clearanceLabel;
		wxTextCtrl* m_clearanceCtrl;
		wxStaticText* m_clearanceUnits;
		wxStaticText* m_minWidthLabel;
		wxTextCtrl* m_minWidthCtrl;
		wxStaticText* m_minWidthUnits;
		wxStaticLine* m_staticline2;
		wxStaticText* m_connectionLabel;
		wxChoice* m_PadInZoneOpt;
		wxStaticText* m_antipadLabel;
		wxTextCtrl* m_antipadCtrl;
		wxStaticText* m_antipadUnits;
		wxStaticText* m_spokeWidthLabel;
		wxTextCtrl* m_spokeWidthCtrl;
		wxStaticText* m_spokeWidthUnits;
		wxStaticText* m_staticTextGridFillType;
		wxChoice* m_GridStyleCtrl;
		wxStaticText* m_staticTextGrindOrient;
		wxTextCtrl* m_tcGridStyleOrientation;
		wxStaticText* m_staticTextRotUnits;
		wxStaticText* m_staticTextStyleThickness;
		wxTextCtrl* m_tcGridStyleThickness;
		wxStaticText* m_GridStyleThicknessUnits;
		wxStaticText* m_staticTextGridGap;
		wxTextCtrl* m_tcGridStyleGap;
		wxStaticText* m_GridStyleGapUnits;
		wxStaticText* m_staticTextGridSmoothingLevel;
		wxSpinCtrl* m_spinCtrlSmoothLevel;
		wxStaticText* m_staticTextGridSmootingVal;
		wxSpinCtrlDouble* m_spinCtrlSmoothValue;
		wxStaticLine* m_staticline5;
		wxStaticText* m_staticText40;
		wxChoice* m_cbRemoveIslands;
		wxStaticText* m_islandThresholdLabel;
		wxTextCtrl* m_tcIslandThreshold;
		wxStaticText* m_islandThresholdUnits;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnZoneNameChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStyleSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveIslandsSelection( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_ZONE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_ZONE_PROPERTIES_BASE();

};


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
class STD_BITMAP_BUTTON;
class WX_GRID;
class WX_INFOBAR;

#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <widgets/net_selector.h>
#include <wx/gbsizer.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/spinctrl.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/notebook.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_ZONE_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_ZONE_PROPERTIES_BASE : public wxPanel
{
	private:
		wxGridBagSizer* gbSizerGeneralProps;

	protected:
		enum
		{
			ID_M_PADINZONEOPT = 6000,
			wxID_ANTIPAD_SIZE,
			wxID_COPPER_BRIDGE_VALUE,
			ID_M_OUTLINEAPPEARANCECTRL,
			ID_M_CORNERSMOOTHINGCTRL,
			ID_CORNER_SMOOTHING,
		};

		WX_INFOBAR* m_copperZoneInfoBar;
		wxStaticText* m_zoneNameLabel;
		wxTextCtrl* m_tcZoneName;
		wxStaticText* m_netLabel;
		NET_SELECTOR* m_netSelector;
		wxCheckBox* m_cbLocked;
		wxNotebook* m_notebook;
		wxPanel* m_clearancesPanel;
		wxStaticText* m_clearanceLabel;
		wxTextCtrl* m_clearanceCtrl;
		wxStaticText* m_clearanceUnits;
		wxStaticText* m_minWidthLabel;
		wxTextCtrl* m_minWidthCtrl;
		wxStaticText* m_minWidthUnits;
		wxStaticText* m_connectionLabel;
		wxChoice* m_PadInZoneOpt;
		wxStaticText* m_antipadLabel;
		wxTextCtrl* m_antipadCtrl;
		wxStaticText* m_antipadUnits;
		wxStaticText* m_spokeWidthLabel;
		wxTextCtrl* m_spokeWidthCtrl;
		wxStaticText* m_spokeWidthUnits;
		wxPanel* m_displayOverridesPanel;
		wxStaticText* m_staticTextStyle;
		wxChoice* m_OutlineDisplayCtrl;
		wxStaticText* m_stBorderHatchPitchText;
		wxTextCtrl* m_outlineHatchPitchCtrl;
		wxStaticText* m_outlineHatchUnits;
		wxPanel* m_hatchedFillPanel;
		wxCheckBox* m_cbHatched;
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
		wxStaticText* m_offsetOverridesLabel;
		WX_GRID* m_layerSpecificOverrides;
		STD_BITMAP_BUTTON* m_bpAddCustomLayer;
		STD_BITMAP_BUTTON* m_bpDeleteCustomLayer;
		wxStaticText* m_cornerSmoothingLabel;
		wxChoice* m_cornerSmoothingChoice;
		wxStaticText* m_cornerRadiusLabel;
		wxTextCtrl* m_cornerRadiusCtrl;
		wxStaticText* m_cornerRadiusUnits;
		wxStaticText* m_removeIslandsLabel;
		wxChoice* m_cbRemoveIslands;
		wxStaticText* m_islandThresholdLabel;
		wxTextCtrl* m_tcIslandThreshold;
		wxStaticText* m_islandThresholdUnits;

		// Virtual event handlers, override them in your derived class
		virtual void OnZoneNameChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onHatched( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddLayerItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteLayerItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCornerSmoothingSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveIslandsSelection( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_ZONE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_ZONE_PROPERTIES_BASE();

};


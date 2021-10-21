///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/dataview.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/statline.h>
#include <wx/gbsizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_COPPER_ZONE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_COPPER_ZONE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			ID_DIALOG_COPPER_ZONE_BASE = 1000,
			ID_TEXTCTRL_NETNAMES_FILTER,
			ID_NETNAME_SELECTION,
			ID_M_PRIORITYLEVELCTRL,
			ID_M_OUTLINEAPPEARANCECTRL,
			ID_CORNER_SMOOTHING,
			ID_M_CORNERSMOOTHINGCTRL,
			ID_M_PADINZONEOPT,
			wxID_ANTIPAD_SIZE,
			wxID_COPPER_BRIDGE_VALUE,
			wxID_BUTTON_EXPORT
		};

		wxBoxSizer* m_MainBoxSizer;
		wxInfoBar* m_copperZoneInfo;
		wxDataViewListCtrl* m_layers;
		wxTextCtrl* m_ShowNetNameFilter;
		wxCheckBox* m_hideAutoGenNetNamesOpt;
		wxCheckBox* m_sortByPadsOpt;
		wxListBox* m_ListNetNameSelection;
		wxStaticText* m_zoneNameLabel;
		wxTextCtrl* m_tcZoneName;
		wxStaticText* m_staticTextPriorityLevel;
		wxSpinCtrl* m_PriorityLevelCtrl;
		wxCheckBox* m_constrainOutline;
		wxCheckBox* m_cbLocked;
		wxStaticText* m_staticTextStyle;
		wxChoice* m_OutlineDisplayCtrl;
		wxStaticLine* m_staticline1;
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
		wxButton* m_ExportSetupButton;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnLayerSelection( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnShowNetNameFilterChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNetSortingOptionSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNetSelectionUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStyleSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveIslandsSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void ExportSetupToOtherCopperZones( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonCancelClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_COPPER_ZONE_BASE( wxWindow* parent, wxWindowID id = ID_DIALOG_COPPER_ZONE_BASE, const wxString& title = _("Copper Zone Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_COPPER_ZONE_BASE();

};


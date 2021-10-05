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
class UNIT_SELECTOR_ANGLE;
class UNIT_SELECTOR_FREQUENCY;
class UNIT_SELECTOR_LEN;
class UNIT_SELECTOR_RESISTOR;

#include "panel_regulator.h"
#include "panel_attenuators.h"
#include "panel_color_code.h"
#include "panel_via_size.h"
#include "panel_track_width.h"
#include "panel_electrical_spacing.h"
#include "kiway_player.h"
#include <wx/string.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/html/htmlwin.h>
#include <wx/radiobox.h>
#include <wx/statbmp.h>
#include <wx/choice.h>
#include <wx/bmpbuttn.h>
#include <wx/grid.h>
#include <wx/notebook.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PCB_CALCULATOR_FRAME_BASE
///////////////////////////////////////////////////////////////////////////////
class PCB_CALCULATOR_FRAME_BASE : public KIWAY_PLAYER
{
	private:

	protected:
		wxMenuBar* m_menubar;
		wxNotebook* m_Notebook;
		PANEL_REGULATOR* m_panelRegulators;
		PANEL_ATTENUATORS* m_panelAttenuators;
		wxPanel* m_panelESeries;
		wxStaticText* m_ESrequired;
		wxTextCtrl* m_ResRequired;
		wxStaticText* m_reqResUnits;
		wxStaticText* m_ESrequired1;
		wxTextCtrl* m_ResExclude1;
		wxStaticText* m_exclude1Units;
		wxStaticText* m_ESrequired11;
		wxTextCtrl* m_ResExclude2;
		wxStaticText* m_exclude2Units;
		wxStaticLine* m_staticline6;
		wxRadioButton* m_e1;
		wxRadioButton* m_e3;
		wxRadioButton* m_e6;
		wxRadioButton* m_e12;
		wxStaticText* m_ESerieSimpleSolution;
		wxTextCtrl* m_ESeries_Sol2R;
		wxStaticText* m_ESeriesSimpleErr;
		wxTextCtrl* m_ESeriesError2R;
		wxStaticText* m_ESeriesSimplePercent;
		wxStaticText* m_ESerie3RSolution1;
		wxTextCtrl* m_ESeries_Sol3R;
		wxStaticText* m_ESeriesAltErr;
		wxTextCtrl* m_ESeriesError3R;
		wxStaticText* m_ESeriesAltPercent;
		wxStaticText* m_ESeries4RSolution;
		wxTextCtrl* m_ESeries_Sol4R;
		wxStaticText* m_ESeriesAltErr1;
		wxTextCtrl* m_ESeriesError4R;
		wxStaticText* m_ESeriesAltPercent1;
		wxStaticLine* m_staticline7;
		wxButton* m_buttonEScalculate;
		wxHtmlWindow* m_panelESeriesHelp;
		PANEL_COLOR_CODE* m_panelColorCode;
		wxPanel* m_panelTransline;
		wxRadioBox* m_TranslineSelection;
		wxStaticBitmap* m_translineBitmap;
		wxStaticText* m_EpsilonR_label;
		wxTextCtrl* m_Value_EpsilonR;
		wxButton* m_button_EpsilonR;
		wxStaticText* m_TanD_label;
		wxTextCtrl* m_Value_TanD;
		wxButton* m_button_TanD;
		wxStaticText* m_Rho_label;
		wxTextCtrl* m_Value_Rho;
		wxButton* m_button_Rho;
		wxStaticText* m_substrate_prm4_label;
		wxTextCtrl* m_Substrate_prm4_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm4_choiceUnit;
		wxStaticText* m_substrate_prm5_label;
		wxTextCtrl* m_Substrate_prm5_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm5_choiceUnit;
		wxStaticText* m_substrate_prm6_label;
		wxTextCtrl* m_Substrate_prm6_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm6_choiceUnit;
		wxStaticText* m_substrate_prm7_label;
		wxTextCtrl* m_Substrate_prm7_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm7_choiceUnit;
		wxStaticText* m_substrate_prm8_label;
		wxTextCtrl* m_Substrate_prm8_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm8_choiceUnit;
		wxStaticText* m_substrate_prm9_label;
		wxTextCtrl* m_Substrate_prm9_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm9_choiceUnit;
		wxStaticText* m_Frequency_label;
		wxTextCtrl* m_Value_Frequency_Ctrl;
		UNIT_SELECTOR_FREQUENCY* m_choiceUnit_Frequency;
		wxStaticBitmap* m_bmCMicrostripZoddZeven;
		wxFlexGridSizer* m_fgSizerZcomment;
		wxStaticText* m_staticTextZdiff;
		wxStaticText* m_staticTextZcommon;
		wxStaticText* m_phys_prm1_label;
		wxTextCtrl* m_Phys_prm1_Value;
		UNIT_SELECTOR_LEN* m_choiceUnit_Param1;
		wxRadioButton* m_radioBtnPrm1;
		wxStaticText* m_phys_prm2_label;
		wxTextCtrl* m_Phys_prm2_Value;
		UNIT_SELECTOR_LEN* m_choiceUnit_Param2;
		wxRadioButton* m_radioBtnPrm2;
		wxStaticText* m_phys_prm3_label;
		wxTextCtrl* m_Phys_prm3_Value;
		UNIT_SELECTOR_LEN* m_choiceUnit_Param3;
		wxBitmapButton* m_bpButtonAnalyze;
		wxButton* m_AnalyseButton;
		wxButton* m_SynthetizeButton;
		wxBitmapButton* m_bpButtonSynthetize;
		wxStaticText* m_elec_prm1_label;
		wxTextCtrl* m_Elec_prm1_Value;
		UNIT_SELECTOR_RESISTOR* m_choiceUnit_ElecPrm1;
		wxStaticText* m_elec_prm2_label;
		wxTextCtrl* m_Elec_prm2_Value;
		UNIT_SELECTOR_RESISTOR* m_choiceUnit_ElecPrm2;
		wxStaticText* m_elec_prm3_label;
		wxTextCtrl* m_Elec_prm3_Value;
		UNIT_SELECTOR_ANGLE* m_choiceUnit_ElecPrm3;
		wxStaticText* m_left_message1;
		wxStaticText* m_Message1;
		wxStaticText* m_left_message2;
		wxStaticText* m_Message2;
		wxStaticText* m_left_message3;
		wxStaticText* m_Message3;
		wxStaticText* m_left_message4;
		wxStaticText* m_Message4;
		wxStaticText* m_left_message5;
		wxStaticText* m_Message5;
		wxStaticText* m_left_message6;
		wxStaticText* m_Message6;
		wxStaticText* m_left_message7;
		wxStaticText* m_Message7;
		wxStaticText* m_left_message8;
		wxStaticText* m_Message8;
		wxButton* m_buttonTransLineReset;
		PANEL_VIA_SIZE* m_panelViaSize;
		PANEL_TRACK_WIDTH* m_panelTrackWidth;
		PANEL_ELECTRICAL_SPACING* m_panelElectricalSpacing;
		wxPanel* m_panelBoardClass;
		UNIT_SELECTOR_LEN* m_BoardClassesUnitsSelector;
		wxStaticText* m_staticTextBrdClass;
		wxGrid* m_gridClassesValuesDisplay;
		wxPanel* m_panelShowClassPrms;

		// Virtual event handlers, overide them in your derived class
		virtual void OnClosePcbCalc( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnESeriesSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCalculateESeries( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineEpsilonR_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineTanD_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineRho_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineAnalyse( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineSynthetize( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTransLineResetButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBoardClassesUnitsSelection( wxCommandEvent& event ) { event.Skip(); }


	public:

		PCB_CALCULATOR_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("PCB Calculator"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_FRAME_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL, const wxString& name = wxT("pcb_calculator") );

		~PCB_CALCULATOR_FRAME_BASE();

};


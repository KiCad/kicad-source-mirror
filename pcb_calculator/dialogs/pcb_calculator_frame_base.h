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
class UNIT_SELECTOR_THICKNESS;

#include "widgets/unit_selector.h"
#include "kiway_player.h"
#include <wx/string.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statusbr.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/radiobox.h>
#include <wx/bmpbuttn.h>
#include <wx/html/htmlwin.h>
#include <wx/statline.h>
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
		wxStatusBar* m_statusBar;
		wxNotebook* m_Notebook;
		wxPanel* m_panelRegulators;
		wxPanel* m_panelRegulatorBitmaps;
		wxStaticBitmap* m_bitmapRegul4pins;
		wxStaticBitmap* m_bitmapRegul3pins;
		wxStaticText* m_RegulFormula;
		wxRadioButton* m_rbRegulR1;
		wxStaticText* m_labelRegultR1;
		wxTextCtrl* m_RegulR1Value;
		wxStaticText* m_UnitRegultR11;
		wxRadioButton* m_rbRegulR2;
		wxStaticText* m_labelRegultR2;
		wxTextCtrl* m_RegulR2Value;
		wxStaticText* m_UnitRegultR1;
		wxRadioButton* m_rbRegulVout;
		wxStaticText* m_labelVout;
		wxTextCtrl* m_RegulVoutValue;
		wxStaticText* m_unitsVout;
		wxStaticText* m_labelVRef;
		wxTextCtrl* m_RegulVrefValue;
		wxStaticText* m_unitsVref;
		wxStaticText* m_RegulIadjTitle;
		wxTextCtrl* m_RegulIadjValue;
		wxStaticText* m_IadjUnitLabel;
		wxStaticText* m_staticTextRegType;
		wxChoice* m_choiceRegType;
		wxButton* m_buttonCalculate;
		wxButton* m_buttonRegulReset;
		wxChoice* m_choiceRegulatorSelector;
		wxStaticText* m_staticTextRegFile;
		wxTextCtrl* m_regulators_fileNameCtrl;
		wxButton* m_buttonDataFile;
		wxButton* m_buttonEditItem;
		wxButton* m_buttonAddItem;
		wxButton* m_buttonRemoveItem;
		wxStaticText* m_RegulMessage;
		wxPanel* m_panelAttenuators;
		wxRadioBox* m_AttenuatorsSelection;
		wxPanel* m_attenuatorPanel;
		wxStaticBitmap* m_attenuatorBitmap;
		wxStaticText* m_attenuationLabel;
		wxTextCtrl* m_AttValueCtrl;
		wxStaticText* m_attUnit;
		wxStaticText* m_attenuationZinLabel;
		wxTextCtrl* m_ZinValueCtrl;
		wxStaticText* m_attZinUnit;
		wxStaticText* m_ZoutLabel;
		wxTextCtrl* m_ZoutValueCtrl;
		wxStaticText* m_attZoutUnit;
		wxButton* m_buttonAlcAtt;
		wxBitmapButton* m_bpButtonCalcAtt;
		wxStaticText* m_attenuatorR1Label;
		wxTextCtrl* m_Att_R1_Value;
		wxStaticText* m_attR1Unit;
		wxStaticText* m_attenuatorR2Label;
		wxTextCtrl* m_Att_R2_Value;
		wxStaticText* m_attR2Unit1;
		wxStaticText* m_attenuatorR3Label;
		wxTextCtrl* m_Att_R3_Value;
		wxStaticText* m_attR3Unit;
		wxStaticText* m_staticTextAttMsg;
		wxHtmlWindow* m_Attenuator_Messages;
		wxHtmlWindow* m_panelAttFormula;
		wxPanel* m_panelESeries;
		wxStaticText* m_ESrequired;
		wxTextCtrl* m_ResRequired;
		wxStaticText* m_UnitRegultR111;
		wxStaticText* m_ESrequired1;
		wxTextCtrl* m_ResExclude1;
		wxStaticText* m_UnitRegultR1111;
		wxStaticText* m_ESrequired11;
		wxTextCtrl* m_ResExclude2;
		wxStaticText* m_UnitRegultR1112;
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
		wxPanel* m_panelColorCode;
		wxRadioBox* m_rbToleranceSelection;
		wxStaticText* m_staticText31;
		wxStaticText* m_staticText34;
		wxStaticText* m_staticText35;
		wxStaticText* m_Band4Label;
		wxStaticText* m_staticText37;
		wxStaticText* m_staticText38;
		wxStaticBitmap* m_Band1bitmap;
		wxStaticBitmap* m_Band2bitmap;
		wxStaticBitmap* m_Band3bitmap;
		wxStaticBitmap* m_Band4bitmap;
		wxStaticBitmap* m_Band_mult_bitmap;
		wxStaticBitmap* m_Band_tol_bitmap;
		wxPanel* m_panelTransline;
		wxRadioBox* m_TranslineSelection;
		wxPanel* m_translinePanel;
		wxStaticBitmap* m_translineBitmap;
		wxStaticLine* m_staticline1;
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
		wxButton* m_buttonTransLineReset;
		wxPanel* m_panelViaSize;
		wxStaticText* m_staticTextHoleDia;
		wxTextCtrl* m_textCtrlHoleDia;
		UNIT_SELECTOR_LEN* m_choiceHoleDia;
		wxStaticText* m_staticTextPlatingThickness;
		wxTextCtrl* m_textCtrlPlatingThickness;
		UNIT_SELECTOR_LEN* m_choicePlatingThickness;
		wxStaticText* m_staticTextViaLength;
		wxTextCtrl* m_textCtrlViaLength;
		UNIT_SELECTOR_LEN* m_choiceViaLength;
		wxStaticText* m_staticTextViaPadDia;
		wxTextCtrl* m_textCtrlViaPadDia;
		UNIT_SELECTOR_LEN* m_choiceViaPadDia;
		wxStaticText* m_staticTextClearanceDia;
		wxTextCtrl* m_textCtrlClearanceDia;
		UNIT_SELECTOR_LEN* m_choiceClearanceDia;
		wxStaticText* m_staticTextImpedance;
		wxTextCtrl* m_textCtrlImpedance;
		UNIT_SELECTOR_RESISTOR* m_choiceImpedance;
		wxStaticText* m_staticAppliedCurrent;
		wxTextCtrl* m_textCtrlAppliedCurrent;
		wxStaticText* m_staticTextAppliedCurrentUnits;
		wxStaticText* m_staticTextResistivity;
		wxTextCtrl* m_textCtrlPlatingResistivity;
		wxButton* m_button_ResistivityVia;
		wxStaticText* m_staticTextPermittivity;
		wxTextCtrl* m_textCtrlPlatingPermittivity;
		wxButton* m_button_Permittivity;
		wxStaticText* m_staticTextTemperatureDiff;
		wxTextCtrl* m_textCtrlTemperatureDiff;
		wxStaticText* m_staticTextTemperatureUnits;
		wxStaticText* m_staticTextRiseTime;
		wxTextCtrl* m_textCtrlRiseTime;
		wxStaticText* m_staticTextRiseTimeUnits;
		wxPanel* m_panelViaBitmap;
		wxStaticBitmap* m_viaBitmap;
		wxStaticText* m_staticTextArea11;
		wxStaticText* m_ViaResistance;
		wxStaticText* m_IntTrackAreaUnitLabel1;
		wxStaticText* m_staticText65111;
		wxStaticText* m_ViaVoltageDrop;
		wxStaticText* m_staticText8411;
		wxStaticText* m_staticText66111;
		wxStaticText* m_ViaPowerLoss;
		wxStaticText* m_staticText8311;
		wxStaticText* m_staticText79211;
		wxStaticText* m_ViaThermalResistance;
		wxStaticText* m_staticText791111;
		wxStaticText* m_staticTextAmpacity;
		wxStaticText* m_ViaAmpacity;
		wxStaticText* m_staticTextAmpacityUnits;
		wxStaticText* m_staticTextCapacitance;
		wxStaticText* m_ViaCapacitance;
		wxStaticText* m_staticTextCapacitanceUnits;
		wxStaticText* m_staticTextRiseTimeOutput;
		wxStaticText* m_RiseTimeOutput;
		wxStaticText* m_staticTextRiseTimeOutputUnits;
		wxStaticText* m_staticTextInductance;
		wxStaticText* m_Inductance;
		wxStaticText* m_staticTextInductanceUnits;
		wxStaticText* m_staticTextReactance;
		wxStaticText* m_Reactance;
		wxStaticText* m_staticTextReactanceUnits;
		wxStaticText* m_staticTextWarning;
		wxButton* m_buttonViaReset;
		wxPanel* m_panelTrackWidth;
		wxStaticText* m_staticTextCurrent;
		wxTextCtrl* m_TrackCurrentValue;
		wxStaticText* m_staticText62;
		wxStaticText* m_staticText63;
		wxTextCtrl* m_TrackDeltaTValue;
		wxStaticText* m_staticText64;
		wxStaticText* m_staticText66;
		wxTextCtrl* m_TrackLengthValue;
		UNIT_SELECTOR_LEN* m_TW_CuLength_choiceUnit;
		wxTextCtrl* m_TWResistivity;
		wxStaticText* m_staticText103;
		wxStaticText* m_staticText104;
		wxHtmlWindow* m_htmlWinFormulas;
		wxStaticText* m_staticTextExtWidth;
		wxTextCtrl* m_ExtTrackWidthValue;
		UNIT_SELECTOR_LEN* m_TW_ExtTrackWidth_choiceUnit;
		wxStaticText* m_staticText65;
		wxTextCtrl* m_ExtTrackThicknessValue;
		UNIT_SELECTOR_THICKNESS* m_ExtTrackThicknessUnit;
		wxStaticLine* m_staticline3;
		wxStaticLine* m_staticline4;
		wxStaticLine* m_staticline5;
		wxStaticText* m_staticTextArea;
		wxStaticText* m_ExtTrackAreaValue;
		wxStaticText* m_ExtTrackAreaUnitLabel;
		wxStaticText* m_staticText651;
		wxStaticText* m_ExtTrackResistValue;
		wxStaticText* m_staticText84;
		wxStaticText* m_staticText661;
		wxStaticText* m_ExtTrackVDropValue;
		wxStaticText* m_staticText83;
		wxStaticText* m_staticText79;
		wxStaticText* m_ExtTrackLossValue;
		wxStaticText* m_staticText791;
		wxStaticText* m_staticTextIntWidth;
		wxTextCtrl* m_IntTrackWidthValue;
		UNIT_SELECTOR_LEN* m_TW_IntTrackWidth_choiceUnit;
		wxStaticText* m_staticText652;
		wxTextCtrl* m_IntTrackThicknessValue;
		UNIT_SELECTOR_THICKNESS* m_IntTrackThicknessUnit;
		wxStaticText* m_staticTextArea1;
		wxStaticText* m_IntTrackAreaValue;
		wxStaticText* m_IntTrackAreaUnitLabel;
		wxStaticText* m_staticText6511;
		wxStaticText* m_IntTrackResistValue;
		wxStaticText* m_staticText841;
		wxStaticText* m_staticText6611;
		wxStaticText* m_IntTrackVDropValue;
		wxStaticText* m_staticText831;
		wxStaticText* m_staticText792;
		wxStaticText* m_IntTrackLossValue;
		wxStaticText* m_staticText7911;
		wxButton* m_buttonTrackWidthReset;
		wxPanel* m_panelElectricalSpacing;
		UNIT_SELECTOR_LEN* m_ElectricalSpacingUnitsSelector;
		wxStaticLine* m_staticline2;
		wxStaticText* m_staticText891;
		wxTextCtrl* m_ElectricalSpacingVoltage;
		wxButton* m_buttonElectSpacingRefresh;
		wxStaticText* m_staticTextElectricalSpacing;
		wxGrid* m_gridElectricalSpacingValues;
		wxStaticText* m_staticText88;
		wxPanel* m_panelBoardClass;
		UNIT_SELECTOR_LEN* m_BoardClassesUnitsSelector;
		wxStaticText* m_staticTextBrdClass;
		wxGrid* m_gridClassesValuesDisplay;
		wxPanel* m_panelShowClassPrms;

		// Virtual event handlers, overide them in your derived class
		virtual void OnClosePcbCalc( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnRegulTypeSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegulatorCalcButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegulatorResetButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegulatorSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDataFileSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditRegulator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddRegulator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveRegulator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAttenuatorSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCalculateAttenuator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnESeriesSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCalculateESeries( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnToleranceSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineEpsilonR_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineTanD_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineRho_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineAnalyse( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineSynthetize( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTransLineResetButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnViaCalculate( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnViaRho_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnViaEpsilonR_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdateViaCalcErrorText( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnViaResetButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTWCalculateFromCurrent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTWParametersChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTWCalculateFromExtWidth( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTWCalculateFromIntWidth( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTWResetButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnElectricalSpacingUnitsSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnElectricalSpacingRefresh( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBoardClassesUnitsSelection( wxCommandEvent& event ) { event.Skip(); }


	public:

		PCB_CALCULATOR_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("PCB Calculator"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_FRAME_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL, const wxString& name = wxT("pcb_calculator") );

		~PCB_CALCULATOR_FRAME_BASE();

};


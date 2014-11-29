///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  5 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PCB_CALCULATOR_FRAME_BASE_H__
#define __PCB_CALCULATOR_FRAME_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class KIWAY_PLAYER;
class UNIT_SELECTOR_ANGLE;
class UNIT_SELECTOR_FREQUENCY;
class UNIT_SELECTOR_LEN;
class UNIT_SELECTOR_RESISTOR;

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
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/html/htmlwin.h>
#include <wx/statline.h>
#include <wx/grid.h>
#include <wx/radiobox.h>
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
		wxChoice* m_choiceRegulatorSelector;
		wxStaticText* m_staticTextRegFile;
		wxTextCtrl* m_regulators_fileNameCtrl;
		wxButton* m_buttonDataFile;
		wxButton* m_buttonEditItem;
		wxButton* m_buttonAddItem;
		wxButton* m_buttonRemoveItem;
		wxStaticText* m_RegulMessage;
		wxPanel* m_panelTrackWidth;
		wxStaticText* m_staticTextTW_WarningMessage;
		wxStaticText* m_staticTextCurrent;
		wxTextCtrl* m_TrackCurrentValue;
		wxStaticText* m_staticText62;
		wxStaticText* m_staticText63;
		wxTextCtrl* m_TrackDeltaTValue;
		wxStaticText* m_staticText64;
		wxStaticText* m_staticText65;
		wxTextCtrl* m_TrackThicknessValue;
		UNIT_SELECTOR_LEN* m_TW_CuThickness_choiceUnit;
		wxStaticText* m_staticText66;
		wxTextCtrl* m_TrackLengthValue;
		UNIT_SELECTOR_LEN* m_TW_CuLength_choiceUnit;
		wxHtmlWindow* m_htmlWinFormulas;
		wxButton* m_buttonTW;
		wxStaticText* m_staticTextWidth;
		wxTextCtrl* m_ExtTrackWidthValue;
		UNIT_SELECTOR_LEN* m_TW_ExtTrackWidth_choiceUnit;
		wxStaticText* m_staticTextArea;
		wxTextCtrl* m_ExtTrackAreaValue;
		wxStaticText* m_ExtTrackAreaUnitLabel;
		wxStaticText* m_staticText651;
		wxTextCtrl* m_ExtTrackResistValue;
		wxStaticText* m_staticText84;
		wxStaticText* m_staticText661;
		wxTextCtrl* m_ExtTrackVDropValue;
		wxStaticText* m_staticText83;
		wxStaticText* m_staticText79;
		wxTextCtrl* m_ExtTrackLossValue;
		wxStaticText* m_staticText791;
		wxStaticText* m_staticTextWidth11;
		wxTextCtrl* m_IntTrackWidthValue;
		UNIT_SELECTOR_LEN* m_TW_IntTrackWidth_choiceUnit;
		wxStaticText* m_staticTextArea1;
		wxTextCtrl* m_IntTrackAreaValue;
		wxStaticText* m_IntTrackAreaUnitLabel;
		wxStaticText* m_staticText6511;
		wxTextCtrl* m_IntTrackResistValue;
		wxStaticText* m_staticText841;
		wxStaticText* m_staticText6611;
		wxTextCtrl* m_IntTrackVDropValue;
		wxStaticText* m_staticText831;
		wxStaticText* m_staticText792;
		wxTextCtrl* m_IntTrackLossValue;
		wxStaticText* m_staticText7911;
		wxPanel* m_panelElectricalSpacing;
		UNIT_SELECTOR_LEN* m_ElectricalSpacingUnitsSelector;
		wxStaticLine* m_staticline2;
		wxStaticText* m_staticText891;
		wxTextCtrl* m_ElectricalSpacingVoltage;
		wxButton* m_buttonElectSpacingRefresh;
		wxStaticText* m_staticTextElectricalSpacing;
		wxGrid* m_gridElectricalSpacingValues;
		wxStaticText* m_staticText88;
		wxPanel* m_panelTransline;
		wxRadioBox* m_TranslineSelection;
		wxPanel* m_panelDisplayshape;
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
		wxStaticBitmap* m_bitmapAnalyse;
		wxButton* m_AnalyseButton;
		wxButton* m_SynthetizeButton;
		wxStaticBitmap* m_bitmapSynthetize;
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
		wxPanel* m_panelAttenuators;
		wxRadioBox* m_AttenuatorsSelection;
		wxPanel* m_panelDisplayAttenuator;
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
		wxStaticBitmap* m_bitmapAnalyse1;
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
		wxTextCtrl* m_Attenuator_Messages;
		wxPanel* m_panelAttFormula;
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
		wxPanel* m_panelBoardClass;
		UNIT_SELECTOR_LEN* m_BoardClassesUnitsSelector;
		wxStaticText* m_staticTextBrdClass;
		wxGrid* m_gridClassesValuesDisplay;
		wxPanel* m_panelShowClassPrms;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClosePcbCalc( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnRegulTypeSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegulatorCalcButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegulatorSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDataFileSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditRegulator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddRegulator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveRegulator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTWCalculateButt( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnElectricalSpacingUnitsSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnElectricalSpacingRefresh( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPaintTranslinePanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnTranslineEpsilonR_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineTanD_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineRho_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineAnalyse( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineSynthetize( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAttenuatorSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPaintAttenuatorPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnCalculateAttenuator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPaintAttFormulaPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnToleranceSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBoardClassesUnitsSelection( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		PCB_CALCULATOR_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("PCB Calculator"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 670,489 ), long style = wxDEFAULT_FRAME_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL );
		
		~PCB_CALCULATOR_FRAME_BASE();
	
};

#endif //__PCB_CALCULATOR_FRAME_BASE_H__

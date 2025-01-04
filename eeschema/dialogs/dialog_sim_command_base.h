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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/srchctrl.h>
#include <wx/checklst.h>
#include <wx/simplebook.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SIM_COMMAND_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SIM_COMMAND_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_commandTypeSizer;
		wxStaticText* m_commandTypeLabel;
		wxChoice* m_commandType;
		wxNotebook* m_notebook1;
		wxPanel* m_panelCommand;
		wxSimplebook* m_simPages;
		wxPanel* m_pgAC;
		wxRadioBox* m_acScale;
		wxStaticText* m_staticText1;
		wxTextCtrl* m_acPointsNumber;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_acFreqStart;
		wxStaticText* m_staticText19;
		wxStaticText* m_staticText3;
		wxTextCtrl* m_acFreqStop;
		wxStaticText* m_staticText110;
		wxPanel* m_pgDC;
		wxCheckBox* m_dcEnable2;
		wxChoice* m_dcSourceType1;
		wxChoice* m_dcSourceType2;
		wxStaticText* m_staticText4;
		wxStaticText* m_staticText41;
		wxStaticText* m_staticText411;
		wxChoice* m_dcSource1;
		wxChoice* m_dcSource2;
		wxStaticText* m_staticText5;
		wxTextCtrl* m_dcStart1;
		wxStaticText* m_src1DCStartValUnit;
		wxTextCtrl* m_dcStart2;
		wxStaticText* m_src2DCStartValUnit;
		wxStaticText* m_staticText6;
		wxTextCtrl* m_dcStop1;
		wxStaticText* m_src1DCEndValUnit;
		wxTextCtrl* m_dcStop2;
		wxStaticText* m_src2DCEndValUnit;
		wxStaticText* m_staticText7;
		wxTextCtrl* m_dcIncr1;
		wxStaticText* m_src1DCStepUnit;
		wxTextCtrl* m_dcIncr2;
		wxStaticText* m_src2DCStepUnit;
		wxButton* m_swapDCSources;
		wxPanel* m_pgOP;
		wxPanel* m_pgTRAN;
		wxStaticText* m_timeLabel;
		wxTextCtrl* m_transStep;
		wxStaticText* m_timeUnits;
		wxStaticText* m_transFinalLabel;
		wxTextCtrl* m_transFinal;
		wxStaticText* m_transFinalUnits;
		wxStaticText* m_transInitialLabel;
		wxTextCtrl* m_transInitial;
		wxStaticText* m_transInitialUnits;
		wxStaticText* m_transInitialHelp;
		wxStaticText* m_maxStepLabel;
		wxTextCtrl* m_transMaxStep;
		wxStaticText* m_transMaxStepUnit;
		wxStaticText* m_transMaxHelp;
		wxCheckBox* m_useInitialConditions;
		wxPanel* m_pgFFT;
		wxStaticText* m_signalsLabel;
		wxSearchCtrl* m_inputSignalsFilter;
		wxCheckListBox* m_inputSignalsList;
		wxCheckBox* m_linearize;
		wxPanel* m_pgNOISE;
		wxStaticText* m_staticText14;
		wxChoice* m_noiseMeas;
		wxStaticText* m_staticText15;
		wxChoice* m_noiseRef;
		wxStaticText* m_staticText23;
		wxStaticText* m_staticText16;
		wxChoice* m_noiseSrc;
		wxRadioBox* m_noiseScale;
		wxStaticText* m_staticText11;
		wxTextCtrl* m_noisePointsNumber;
		wxStaticText* m_staticText21;
		wxTextCtrl* m_noiseFreqStart;
		wxStaticText* m_noiseFreqStartUnits;
		wxStaticText* m_staticText31;
		wxTextCtrl* m_noiseFreqStop;
		wxStaticText* m_noiseFreqStopUnits;
		wxCheckBox* m_saveAllNoise;
		wxPanel* m_pgSP;
		wxRadioBox* m_spScale;
		wxStaticText* m_staticText12;
		wxTextCtrl* m_spPointsNumber;
		wxStaticText* m_staticText22;
		wxTextCtrl* m_spFreqStart;
		wxStaticText* m_staticText191;
		wxStaticText* m_staticText32;
		wxTextCtrl* m_spFreqStop;
		wxStaticText* m_staticText1101;
		wxCheckBox* m_spDoNoise;
		wxPanel* m_pgCustom;
		wxStaticText* m_staticText18;
		wxTextCtrl* m_customTxt;
		wxButton* m_loadDirectives;
		wxPanel* m_pgPZ;
		wxStaticText* m_pzFunctionTypeLabel;
		wxChoice* m_pzFunctionType;
		wxStaticText* m_pzInputLabel;
		wxChoice* m_pzInput;
		wxStaticText* m_pzInputRefLabel;
		wxChoice* m_pzInputRef;
		wxStaticText* m_pzOutputLabel;
		wxChoice* m_pzOutput;
		wxStaticText* m_pzOutputRefLabel;
		wxChoice* m_pzOutputRef;
		wxStaticText* m_pzAnalysesLabel;
		wxChoice* m_pzAnalyses;
		wxCheckBox* m_fixIncludePaths;
		wxCheckBox* m_saveAllVoltages;
		wxCheckBox* m_saveAllCurrents;
		wxCheckBox* m_saveAllDissipations;
		wxCheckBox* m_saveAllEvents;
		wxBoxSizer* m_compatibilityModeSizer;
		wxChoice* m_compatibilityMode;
		wxPanel* m_panelPlotSetup;
		wxBoxSizer* m_bSizerY1;
		wxCheckBox* m_lockY1;
		wxStaticText* m_y1MinLabel;
		wxTextCtrl* m_y1Min;
		wxStaticText* m_y1MaxLabel;
		wxTextCtrl* m_y1Max;
		wxStaticText* m_y1Units;
		wxBoxSizer* m_bSizerY2;
		wxCheckBox* m_lockY2;
		wxStaticText* m_y2MinLabel;
		wxTextCtrl* m_y2Min;
		wxStaticText* m_y2MaxLabel;
		wxTextCtrl* m_y2Max;
		wxStaticText* m_y2Units;
		wxBoxSizer* m_bSizerY3;
		wxCheckBox* m_lockY3;
		wxStaticText* m_y3MinLabel;
		wxTextCtrl* m_y3Min;
		wxStaticText* m_y3MaxLabel;
		wxTextCtrl* m_y3Max;
		wxStaticText* m_y3Units;
		wxCheckBox* m_grid;
		wxCheckBox* m_legend;
		wxCheckBox* m_dottedSecondary;
		wxStaticText* m_marginsLabel;
		wxStaticText* m_marginLeftLabel;
		wxTextCtrl* m_marginLeft;
		wxStaticText* m_marginTopLabel;
		wxTextCtrl* m_marginTop;
		wxStaticText* m_marginBottomLabel;
		wxTextCtrl* m_marginBottom;
		wxStaticText* m_marginRightLabel;
		wxTextCtrl* m_marginRight;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnCommandType( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDCEnableSecondSource( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDCSource1Selected( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDCSource2Selected( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSwapDCSources( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterMouseMoved( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnFilterText( wxCommandEvent& event ) { event.Skip(); }
		virtual void onLoadDirectives( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateUILockY1( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnUpdateUILockY2( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnUpdateUILockY3( wxUpdateUIEvent& event ) { event.Skip(); }


	public:

		DIALOG_SIM_COMMAND_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Simulation Analysis"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_SIM_COMMAND_BASE();

};


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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/combobox.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/stc/stc.h>
#include <wx/statbox.h>
#include <wx/listctrl.h>
#include <wx/choice.h>
#include <wx/notebook.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SPICE_MODEL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SPICE_MODEL_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxPanel* m_passive;
		wxStaticText* m_staticTextPtype;
		wxComboBox* m_pasType;
		wxStaticText* m_staticText62;
		wxStaticText* m_staticTextPvalue;
		wxTextCtrl* m_pasValue;
		wxStaticText* m_staticTextSpVal;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText32;
		wxStaticText* m_staticText321;
		wxStaticText* m_staticText341;
		wxStaticText* m_staticText_femto;
		wxStaticText* m_staticText36;
		wxStaticText* m_staticText37;
		wxStaticText* m_staticText38;
		wxStaticText* m_staticText39;
		wxStaticText* m_staticText40;
		wxStaticText* m_staticText41;
		wxStaticText* m_staticText42;
		wxStaticText* m_staticText43;
		wxStaticText* m_staticText44;
		wxStaticText* m_staticText46;
		wxStaticText* m_staticText47;
		wxStaticText* m_staticText48;
		wxStaticText* m_staticText45;
		wxStaticText* m_staticText49;
		wxStaticText* m_staticText50;
		wxStaticText* m_staticText51;
		wxStaticText* m_staticText52;
		wxStaticText* m_staticText53;
		wxStaticText* m_staticText54;
		wxStaticText* m_staticText55;
		wxStaticText* m_staticText56;
		wxStaticText* m_staticText57;
		wxStaticText* m_staticText58;
		wxStaticText* m_staticText59;
		wxStaticText* m_staticText60;
		wxPanel* m_model;
		wxStaticText* m_staticText7;
		wxTextCtrl* m_modelLibrary;
		wxButton* m_selectLibrary;
		wxStaticText* m_staticText5;
		wxComboBox* m_modelName;
		wxStaticText* m_staticText4;
		wxComboBox* m_modelType;
		wxStyledTextCtrl* m_libraryContents;
		wxPanel* m_power;
		wxStaticText* m_staticText10;
		wxTextCtrl* m_genDc;
		wxStaticText* m_staticText113;
		wxStaticText* m_staticText11;
		wxTextCtrl* m_genAcMag;
		wxStaticText* m_staticText111;
		wxStaticText* m_staticText12;
		wxTextCtrl* m_genAcPhase;
		wxStaticText* m_staticText112;
		wxNotebook* m_powerNotebook;
		wxPanel* m_pwrPulse;
		wxStaticText* m_staticText13;
		wxTextCtrl* m_pulseInit;
		wxStaticText* m_staticText131;
		wxStaticText* m_staticText14;
		wxTextCtrl* m_pulseNominal;
		wxStaticText* m_staticText132;
		wxStaticText* m_staticText15;
		wxTextCtrl* m_pulseDelay;
		wxStaticText* m_staticText133;
		wxStaticText* m_staticText16;
		wxTextCtrl* m_pulseRise;
		wxStaticText* m_staticText134;
		wxStaticText* m_staticText17;
		wxTextCtrl* m_pulseFall;
		wxStaticText* m_staticText135;
		wxStaticText* m_staticText18;
		wxTextCtrl* m_pulseWidth;
		wxStaticText* m_staticText136;
		wxStaticText* m_staticText20;
		wxTextCtrl* m_pulsePeriod;
		wxStaticText* m_staticText137;
		wxPanel* m_pwrSin;
		wxStaticText* m_staticText21;
		wxTextCtrl* m_sinOffset;
		wxStaticText* m_staticText211;
		wxStaticText* m_staticText22;
		wxTextCtrl* m_sinAmplitude;
		wxStaticText* m_staticText212;
		wxStaticText* m_staticText23;
		wxTextCtrl* m_sinFreq;
		wxStaticText* m_staticText213;
		wxStaticText* m_staticText24;
		wxTextCtrl* m_sinDelay;
		wxStaticText* m_staticText214;
		wxStaticText* m_staticText25;
		wxTextCtrl* m_sinDampFactor;
		wxStaticText* m_staticText215;
		wxPanel* m_pwrExp;
		wxStaticText* m_staticText26;
		wxTextCtrl* m_expInit;
		wxStaticText* m_staticText261;
		wxStaticText* m_staticText27;
		wxTextCtrl* m_expPulsed;
		wxStaticText* m_staticText262;
		wxStaticText* m_staticText28;
		wxTextCtrl* m_expRiseDelay;
		wxStaticText* m_staticText263;
		wxStaticText* m_staticText29;
		wxTextCtrl* m_expRiseConst;
		wxStaticText* m_staticText264;
		wxStaticText* m_staticText30;
		wxTextCtrl* m_expFallDelay;
		wxStaticText* m_staticText265;
		wxStaticText* m_staticText31;
		wxTextCtrl* m_expFallConst;
		wxStaticText* m_staticText266;
		wxPanel* m_pwrPwl;
		wxStaticText* m_staticText34;
		wxTextCtrl* m_pwlTime;
		wxStaticText* m_staticText342;
		wxStaticText* m_staticText35;
		wxTextCtrl* m_pwlValue;
		wxStaticText* m_staticText343;
		wxButton* m_pwlAddButton;
		wxListCtrl* m_pwlValList;
		wxButton* m_pwlRemoveBtn;
		wxPanel* m_pwrFm;
		wxStaticText* m_staticText138;
		wxTextCtrl* m_fmOffset;
		wxStaticText* m_staticText1311;
		wxStaticText* m_staticText141;
		wxTextCtrl* m_fmAmplitude;
		wxStaticText* m_staticText1321;
		wxStaticText* m_staticText151;
		wxTextCtrl* m_fmFcarrier;
		wxStaticText* m_staticText1331;
		wxStaticText* m_staticText161;
		wxTextCtrl* m_fmModIndex;
		wxStaticText* m_staticText1341;
		wxStaticText* m_staticText171;
		wxTextCtrl* m_fmFsignal;
		wxStaticText* m_staticText1351;
		wxStaticText* m_staticText181;
		wxTextCtrl* m_fmPhaseC;
		wxStaticText* m_staticText1361;
		wxStaticText* m_staticText201;
		wxTextCtrl* m_fmPhaseS;
		wxStaticText* m_staticText1371;
		wxPanel* m_pwrAm;
		wxStaticText* m_staticText1381;
		wxTextCtrl* m_amAmplitude;
		wxStaticText* m_staticText13111;
		wxStaticText* m_staticText1411;
		wxTextCtrl* m_amOffset;
		wxStaticText* m_staticText13211;
		wxStaticText* m_staticText1511;
		wxTextCtrl* m_amModulatingFreq;
		wxStaticText* m_staticText13311;
		wxStaticText* m_staticText1611;
		wxTextCtrl* m_amCarrierFreq;
		wxStaticText* m_staticText13411;
		wxStaticText* m_staticText1711;
		wxTextCtrl* m_amSignalDelay;
		wxStaticText* m_staticText13511;
		wxStaticText* m_staticText1811;
		wxTextCtrl* m_amPhase;
		wxStaticText* m_staticText13611;
		wxPanel* m_pwrTransNoise;
		wxPanel* m_pwrRandom;
		wxStaticText* m_staticText27111;
		wxChoice* m_rnType;
		wxStaticText* m_staticText26711;
		wxTextCtrl* m_rnTS;
		wxStaticText* m_staticText262111;
		wxStaticText* m_staticText28111;
		wxTextCtrl* m_rnTD;
		wxStaticText* m_staticText263111;
		wxStaticText* m_rnParam1Text;
		wxTextCtrl* m_rnParam1;
		wxStaticText* m_rnParam2Text;
		wxTextCtrl* m_rnParam2;
		wxPanel* m_pwrExtData;
		wxRadioBox* m_pwrType;
		wxStaticLine* m_staticline2;
		wxCheckBox* m_disabled;
		wxCheckBox* m_nodeSeqCheck;
		wxTextCtrl* m_nodeSeqVal;
		wxStaticLine* m_staticline3;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void onSelectLibrary( wxCommandEvent& event ) { event.Skip(); }
		virtual void onModelSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPwlAdd( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPwlRemove( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRandomSourceType( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SPICE_MODEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Spice Model Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 494,604 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_SPICE_MODEL_BASE();

};


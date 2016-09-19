///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  6 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_SPICE_MODEL_BASE_H__
#define __DIALOG_SPICE_MODEL_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

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
#include <wx/statbox.h>
#include <wx/listctrl.h>
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
		wxStaticText* m_staticText2;
		wxComboBox* m_pasType;
		wxStaticText* m_staticText62;
		wxStaticText* m_staticText3;
		wxTextCtrl* m_pasValue;
		wxStaticText* m_staticText63;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText32;
		wxStaticText* m_staticText321;
		wxStaticText* m_staticText341;
		wxStaticText* m_staticText351;
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
		wxPanel* m_semiconductor;
		wxStaticText* m_staticText4;
		wxComboBox* m_semiType;
		wxStaticText* m_staticText5;
		wxComboBox* m_semiModel;
		wxStaticText* m_staticText7;
		wxTextCtrl* m_semiLib;
		wxButton* m_semiSelectLib;
		wxPanel* m_ic;
		wxStaticText* m_staticText8;
		wxComboBox* m_icModel;
		wxStaticText* m_staticText9;
		wxTextCtrl* m_icLib;
		wxButton* m_icSelectLib;
		wxPanel* m_power;
		wxStaticText* m_staticText10;
		wxTextCtrl* m_genDc;
		wxStaticText* m_staticText11;
		wxTextCtrl* m_genAcMag;
		wxStaticText* m_staticText12;
		wxTextCtrl* m_genAcPhase;
		wxNotebook* m_powerNotebook;
		wxPanel* m_pwrPulse;
		wxStaticText* m_staticText13;
		wxTextCtrl* m_pulseInit;
		wxStaticText* m_staticText14;
		wxTextCtrl* m_pulseNominal;
		wxStaticText* m_staticText15;
		wxTextCtrl* m_pulseDelay;
		wxStaticText* m_staticText16;
		wxTextCtrl* m_pulseRise;
		wxStaticText* m_staticText17;
		wxTextCtrl* m_pulseFall;
		wxStaticText* m_staticText18;
		wxTextCtrl* m_pulseWidth;
		wxStaticText* m_staticText20;
		wxTextCtrl* m_pulsePeriod;
		wxPanel* m_pwrSin;
		wxStaticText* m_staticText21;
		wxTextCtrl* m_sinOffset;
		wxStaticText* m_staticText22;
		wxTextCtrl* m_sinAmplitude;
		wxStaticText* m_staticText23;
		wxTextCtrl* m_sinFreq;
		wxStaticText* m_staticText24;
		wxTextCtrl* m_sinDelay;
		wxStaticText* m_staticText25;
		wxTextCtrl* m_sinDampFactor;
		wxPanel* m_pwrExp;
		wxStaticText* m_staticText26;
		wxTextCtrl* m_expInit;
		wxStaticText* m_staticText27;
		wxTextCtrl* m_expPulsed;
		wxStaticText* m_staticText28;
		wxTextCtrl* m_expRiseDelay;
		wxStaticText* m_staticText29;
		wxTextCtrl* m_expRiseConst;
		wxStaticText* m_staticText30;
		wxTextCtrl* m_expFallDelay;
		wxStaticText* m_staticText31;
		wxTextCtrl* m_expFallConst;
		wxPanel* m_pwrPwl;
		wxStaticText* m_staticText34;
		wxTextCtrl* m_pwlTime;
		wxStaticText* m_staticText35;
		wxTextCtrl* m_pwlValue;
		wxButton* m_pwlAddButton;
		wxListCtrl* m_pwlValList;
		wxButton* m_pwlRemoveBtn;
		wxPanel* m_pwrFm;
		wxPanel* m_pwrAm;
		wxPanel* m_pwrTransNoise;
		wxPanel* m_pwrRandom;
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
		virtual void onSemiSelectLib( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSelectIcLib( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPwlAdd( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPwlRemove( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SPICE_MODEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 640,582 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_SPICE_MODEL_BASE();

};

#endif //__DIALOG_SPICE_MODEL_BASE_H__

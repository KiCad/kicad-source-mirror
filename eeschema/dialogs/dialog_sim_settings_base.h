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
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SIM_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SIM_SETTINGS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_simPages;
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
		wxCheckBox* m_dcEnable1;
		wxStaticText* m_staticText41;
		wxComboBox* m_dcSource1;
		wxStaticText* m_staticText51;
		wxTextCtrl* m_dcStart1;
		wxStaticText* m_staticText511;
		wxStaticText* m_staticText61;
		wxTextCtrl* m_dcStop1;
		wxStaticText* m_staticText512;
		wxStaticText* m_staticText71;
		wxTextCtrl* m_dcIncr1;
		wxStaticText* m_staticText513;
		wxCheckBox* m_dcEnable2;
		wxStaticText* m_staticText4;
		wxComboBox* m_dcSource2;
		wxStaticText* m_staticText5;
		wxTextCtrl* m_dcStart2;
		wxStaticText* m_staticText52;
		wxStaticText* m_staticText6;
		wxTextCtrl* m_dcStop2;
		wxStaticText* m_staticText53;
		wxStaticText* m_staticText7;
		wxTextCtrl* m_dcIncr2;
		wxStaticText* m_staticText54;
		wxPanel* m_pgDistortion;
		wxPanel* m_pgNoise;
		wxStaticText* m_staticText14;
		wxComboBox* m_noiseMeas;
		wxStaticText* m_staticText15;
		wxComboBox* m_noiseRef;
		wxStaticText* m_staticText23;
		wxStaticText* m_staticText16;
		wxComboBox* m_noiseSrc;
		wxRadioBox* m_noiseScale;
		wxStaticText* m_staticText11;
		wxTextCtrl* m_noisePointsNumber;
		wxStaticText* m_staticText21;
		wxTextCtrl* m_noiseFreqStart;
		wxStaticText* m_staticText31;
		wxTextCtrl* m_noiseFreqStop;
		wxPanel* m_pgOP;
		wxStaticText* m_staticText13;
		wxPanel* m_pgPoleZero;
		wxPanel* m_pgSensitivity;
		wxPanel* m_pgTransferFunction;
		wxPanel* m_pgTransient;
		wxStaticText* m_staticText151;
		wxTextCtrl* m_transStep;
		wxStaticText* m_staticText1511;
		wxStaticText* m_staticText161;
		wxTextCtrl* m_transFinal;
		wxStaticText* m_staticText1512;
		wxStaticText* m_staticText17;
		wxTextCtrl* m_transInitial;
		wxStaticText* m_staticText1513;
		wxStaticText* m_staticText24;
		wxPanel* m_pgCustom;
		wxStaticText* m_staticText18;
		wxTextCtrl* m_customTxt;
		wxButton* m_loadDirectives;
		wxCheckBox* m_fixPassiveVals;
		wxCheckBox* m_fixIncludePaths;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void onLoadDirectives( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SIM_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Simulation settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_SIM_SETTINGS_BASE();

};


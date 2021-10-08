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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/statbox.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_REGULATOR_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_REGULATOR_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_staticTextRegType;
		wxChoice* m_choiceRegType;
		wxStaticBitmap* m_bitmapRegul4pins;
		wxStaticBitmap* m_bitmapRegul3pins;
		wxStaticText* m_RegulFormula;
		wxRadioButton* m_rbRegulR1;
		wxStaticText* m_labelRegultR1;
		wxTextCtrl* m_RegulR1Value;
		wxStaticText* m_r1Units;
		wxRadioButton* m_rbRegulR2;
		wxStaticText* m_labelRegultR2;
		wxTextCtrl* m_RegulR2Value;
		wxStaticText* m_r2Units;
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
		wxButton* m_buttonCalculate;
		wxButton* m_buttonRegulReset;
		wxStaticText* m_RegulMessage;
		wxChoice* m_choiceRegulatorSelector;
		wxStaticText* m_staticTextRegFile;
		wxTextCtrl* m_regulators_fileNameCtrl;
		wxButton* m_buttonDataFile;
		wxButton* m_buttonEditItem;
		wxButton* m_buttonAddItem;
		wxButton* m_buttonRemoveItem;

		// Virtual event handlers, overide them in your derived class
		virtual void OnRegulTypeSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegulatorCalcButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegulatorResetButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegulatorSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDataFileSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditRegulator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddRegulator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveRegulator( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_REGULATOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 688,436 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_REGULATOR_BASE();

};


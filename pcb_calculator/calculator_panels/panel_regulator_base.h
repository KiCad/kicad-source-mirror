///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "calculator_panels/calculator_panel.h"
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
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/radiobut.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_REGULATOR_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_REGULATOR_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxStaticText* m_staticTextRegType;
		wxChoice* m_choiceRegType;
		wxStaticBitmap* m_bitmapRegul4pins;
		wxStaticBitmap* m_bitmapRegul3pins;
		wxStaticText* m_RegulFormula;
		wxChoice* m_choiceRegulatorSelector;
		wxStaticText* m_staticTextRegFile;
		wxTextCtrl* m_regulators_fileNameCtrl;
		wxButton* m_buttonDataFile;
		wxButton* m_buttonEditItem;
		wxButton* m_buttonAddItem;
		wxButton* m_buttonRemoveItem;
		wxStaticText* m_labelValMin;
		wxStaticText* m_labelValTyp;
		wxStaticText* m_labelValMax;
		wxRadioButton* m_rbRegulR1;
		wxStaticText* m_labelRegultR1;
		wxTextCtrl* m_r1MinVal;
		wxTextCtrl* m_r1TypVal;
		wxTextCtrl* m_r1MaxVal;
		wxStaticText* m_labelUnitsR1;
		wxRadioButton* m_rbRegulR2;
		wxStaticText* m_labelRegultR2;
		wxTextCtrl* m_r2MinVal;
		wxTextCtrl* m_r2TypVal;
		wxTextCtrl* m_r2MaxVal;
		wxStaticText* m_labelUnitsR2;
		wxRadioButton* m_rbRegulVout;
		wxStaticText* m_labelVout;
		wxTextCtrl* m_voutMinVal;
		wxTextCtrl* m_voutTypVal;
		wxTextCtrl* m_voutMaxVal;
		wxStaticText* m_labelUnitsVout;
		wxStaticText* m_labelVRef;
		wxTextCtrl* m_vrefMinVal;
		wxTextCtrl* m_vrefTypVal;
		wxTextCtrl* m_vrefMaxVal;
		wxStaticText* m_labelUnitsVref;
		wxStaticText* m_RegulIadjTitle;
		wxTextCtrl* m_iadjTypVal;
		wxTextCtrl* m_iadjMaxVal;
		wxStaticText* m_labelUnitsIadj;
		wxStaticText* m_labelTolSumary;
		wxTextCtrl* m_tolTotalMin;
		wxTextCtrl* m_TolTotalMax;
		wxStaticText* m_labelTotalPercent;
		wxStaticText* m_labelResTol;
		wxTextCtrl* m_resTolVal;
		wxStaticText* m_labelResTolUnit;
		wxStaticText* m_labelKicadComment;
		wxTextCtrl* m_textPowerComment;
		wxButton* m_btCopyCB;
		wxStaticText* m_RegulMessage;
		wxButton* m_buttonCalculate;
		wxButton* m_buttonRegulReset;

		// Virtual event handlers, override them in your derived class
		virtual void OnRegulTypeSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegulatorSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDataFileSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditRegulator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddRegulator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveRegulator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCopyCB( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegulatorCalcButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRegulatorResetButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_REGULATOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_REGULATOR_BASE();

};


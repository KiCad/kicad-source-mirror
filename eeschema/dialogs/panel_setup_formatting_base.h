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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/spinctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_FORMATTING_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_FORMATTING_BASE : public wxPanel
{
	DECLARE_EVENT_TABLE()
	private:

		// Private event handlers
		void _wxFB_onCheckBoxIref( wxCommandEvent& event ){ onCheckBoxIref( event ); }


	protected:
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		wxStaticText* m_overbarHieghtLabel;
		wxTextCtrl* m_overbarHeightCtrl;
		wxStaticText* m_overbarHeightUnits;
		wxStaticText* m_textOffsetRatioLabel;
		wxTextCtrl* m_textOffsetRatioCtrl;
		wxStaticText* m_offsetRatioUnits;
		wxStaticText* m_labelSizeRatioLabel;
		wxTextCtrl* m_labelSizeRatioCtrl;
		wxStaticText* m_labelSizeRatioUnits;
		wxStaticText* m_lineWidthLabel;
		wxTextCtrl* m_lineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxStaticText* m_pinSymbolSizeLabel;
		wxTextCtrl* m_pinSymbolSizeCtrl;
		wxStaticText* m_pinSymbolSizeUnits;
		wxStaticText* m_junctionDotLabel;
		wxChoice* m_choiceJunctionDotSize;
		wxStaticText* m_hopOverLabel;
		wxChoice* m_choiceHopOverSize;
		wxStaticText* m_connectionGridLabel;
		wxTextCtrl* m_connectionGridCtrl;
		wxStaticText* m_connectionGridUnits;
		wxCheckBox* m_showIntersheetsReferences;
		wxCheckBox* m_listOwnPage;
		wxRadioButton* m_radioFormatStandard;
		wxRadioButton* m_radioFormatAbbreviated;
		wxStaticText* m_prefixLabel;
		wxTextCtrl* m_prefixCtrl;
		wxStaticText* m_suffixLabel;
		wxTextCtrl* m_suffixCtrl;
		wxStaticText* dashLengthLabel;
		wxTextCtrl* m_dashLengthCtrl;
		wxStaticText* gapLengthLabel;
		wxTextCtrl* m_gapLengthCtrl;
		wxStaticText* m_dashedLineHelp;
		wxSpinCtrl* m_vPrecisionCtrl;
		wxChoice* m_vRangeCtrl;
		wxSpinCtrl* m_iPrecisionCtrl;
		wxChoice* m_iRangeCtrl;

		// Virtual event handlers, override them in your derived class
		virtual void onCheckBoxIref( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_FORMATTING_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_FORMATTING_BASE();

};

